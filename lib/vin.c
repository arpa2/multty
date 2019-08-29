/* mulTTY -> input processing
 *
 * Split input in a number of levels:
 *  - split chunks at <SOH>
 *  - after <SOH> take in naming, possible <US>, up to control; treat name as an opt_param
 *  - accept application sequences; not <DCx>, <SI>, <SO>, <EM>, <DLE>funnies, non<DLE>mixed
 *  - accept stream processing; <SI>, <SO>, <EM> with current stream
 *  - accept program multiplexing; <DCx>, <EM> without current stream
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */
 

#include <errno.h>
#include <syslog.h>

#include <arpa2/multty.h>


struct multty_inflow {
	int infd;
	char buf [PIPE_BUF];
	int wrofs;	/* where to write next */
	int rdofs;	/* where to read  next */
	int rdend;	/* where reading stopped */
	int prenm;	/* points at name start, or is -1 */
	int postnm;	/* points at control beyond name, or 0 */
#ifdef MULTTY_MIXED
	TODO_CURRENT_PROGRAMSET;
	TODO_CURRENT_PROGRAM;
#define input_streams  TODO_VIA_CURRENT_PROGRAM
#define current_stream TODO_VIA_CURRENT_PROGRAM
#else
	MULTTY *input_streams;
	MULTTY *current_stream;
#endif
};
typedef struct multty_inflow inflow_t;


/* Reset the inflow to prepare it to process the next chunk.
 * This also enables pushing back content upon next read.
 */
static void _mty_reset_inflow (inflow_t *flow) {
	flow->prenm = -1;
	flow->postnm = 0;
	flow->rdofs = flow->rdend;
}


/* Remove togo characters at ofs in flow->buf.
 * Update flow->wrofs.
 */
static void _mty_cutback (inflow_t *flow, int ofs, int togo) {
	//TODO// if (flow->prenm >= 0) { ...syslog..."ignored <SOH> name prefix"... }
	if (flow->wrofs > ofs+togo) {
		memmove (flow->buf+ofs, flow->buf+ofs+togo, flow->wrofs-ofs-togo);
	}
	flow->wrofs -= togo;
}


/* Read additional bytes into buffer.
 *
 * Returns true on success, or false/errno.
 */
static bool _mty_readmore (inflow_t *flow) {
	//
	// Push back the memory buffer
	//TODO// We can probably cut back if prenm==-1 too
	if ((flow->prenm == -1) && (flow->rdofs > 0)) {
		_mty_cutback (flow, 0, flow->rdofs);
		flow->rdend -= flow->rdofs;
		flow->rdofs = 0;
	}
	//
	// Check if any buffer space is available
	if (flow->wrofs >= sizeof (flow->buf)) {
		errno = ENOBUFS;
		return false;
	}
	//
	// Try to read from the file as much as we can store
	ssize_t gotten = read (flow->infd, flow->buf + flow->wrofs, sizeof (flow->buf) - flow->wrofs);
	if (gotten < 0) {
		return false;
	}
	flow->wrofs += gotten;
	//
	// Return success
	return true;
}


/* Report a bad character by position (may be <DLE> prefixed).
 */
static void _mty_badchar (inflow_t *flow, int badpos) {
	//
	// Log an error
	int badlen = (flow->buf [badpos] == c_DLE) ? 2 : 1;
	if (badlen == 2) {
		syslog (LOG_ERR, "Bad escaped character 0x%02x in mulTTY input\n", flow->buf [badpos+1] ^ 0x40);
	} else {
		syslog (LOG_ERR, "Bad character 0x%02x in mulTTY input\n", flow->buf [badpos]);
	}
}


/* Parse a potential <SOH> name prefix, setting postnm to the following control.
 *
 * Return true when sufficient information was available, false to defer.
 * The value flow->postnm is not set if false is returned.
 */
static bool _mty_getname (inflow_t *flow) {
	//
	// We do need to see the first character
	if (flow->wrofs <= flow->rdofs) {
		return false;
	}
	//
	// Done when there is no initial name
	if (flow->buf [flow->rdofs] != c_SOH) {
		flow->prenm = -1;
		flow->postnm = 0;
		return true;
	}
	//
	// Chase for a non-<US> control
	int i = flow->rdofs + 1;
	static const uint32_t ctl_no_us [2] = { MULTTY_ESC_BINARY, MULTTY_ESC_MIXED };
	int phase = 0;
	while (i < flow->wrofs) {
		uint8_t c = flow->buf [i++];
		if ((c == c_US) && (phase == 0)) {
			//
			// This character is <US> for the next phase of the name
			phase++;
		} else if (mtyescapewish (ctl_no_us [phase], flow->buf [i])) {
			//
			// This is a control character, and not part of the name
			// (Before <US> be really tight; after, avoid mulTTY confusion)
			flow->prenm = flow->rdofs + 1;
			flow->postnm = i - 1;
			return true;
		}
	}
	//
	// We need more characters to determine the end of the name
	return false;
}


/* Find an application byte string length in the input flow.
 * This stops at the end of wrbuf.  And at codes it cannot use.
 * Reasons to stop accepting characters are:
 *  - Things we always escape: MULTTY_ESC_MIXED
 *  - Things after <DLE> we never escape: MULTTY_ESC_BINARY
 *  - Control codes <SOH>, <EM>, <DCx>, <SI>, <SO>.
 *
 * Returns if bytes found after optional <SOH> name prefix.
 * Bytes include <DLE>.  Return value false for no appbytes.
 * Sole <DLE> is never returned, then always false/wait.  The
 * offset flow->rdend is set after the accepted byte string.
 *
 * Note that a returned app string length of 0 signals either
 * too little data or unsuitable data for app processing.
 * Further layers (stream splitting, program multiplexing)
 * will help to determine what is going on.  If nothing works,
 * report a bad character if there is enough content, then
 * wait for additional input and try again.  Locally detected
 * bad characters will be repaired with _mty_badchar().
 */
static bool _myt_appstring (inflow_t *flow) {
	int pos = flow->postnm;
	int retval = 0;
	uint8_t c, c2;
	//
	// Iterate over characters until we break
	while (flow->wrofs > pos) {
		c = flow->buf [pos];
		//
		// Special characters that are never acceptable
		static const uint32_t higher_level_controls = (
			(1<<c_SOH) | (1<<c_SI ) | (1<<c_SO ) | (1<<c_EM ) |
			(1<<c_DC1) | (1<<c_DC2) | (1<<c_DC3) | (1<<c_DC4)
		);
		static const uint32_t always_esc_controls = MULTTY_ESC_MIXED;
		if (mtyescapewish (higher_level_controls, c)) {
			if (mtyescapewish (always_esc_controls, c)) {
				//
				// c is a bad character -- repair
				// (This also removes <NUL> from the stream)
				_mty_badchar (flow, pos);
				_mty_cutback (flow, pos, 1);
				continue;
			} else {
				//
				// c may be for an upper layer
				// (Stream switching or application multiplexing)
				// (Though not a *higher* level, <SOH> breaks here too)
				break;
			}
		}
		//
		// Special cases after <DLE>
		if (c == c_DLE) {
			if (flow->wrofs <= pos) {
				//
				// <DLE> in end position, leave it be
				break;
			}
			//
			// Test character after <DLE>
			c2 = flow->buf [pos+1];
			static const uint32_t tolerated_with_escape = MULTTY_ESC_BINARY;
			if (!mtyescapewish (tolerated_with_escape, c2 ^ 0x40)) {
				//
				// <DLE>,c2 sequence is bad -- repair
				// (Overzealously escaped, possibly evil)
				_mty_badchar (flow, pos);
				_mty_cutback (flow, pos, 2);
				continue;
			}
			//
			// Approve of <DLE>,c2
			pos += 2;
			continue;
		}
	}
	//
	// Return the length found suitable as app string
	flow->rdend = pos;
	return pos > flow->postnm;
}


/* Handle application byte sequence by callback invocation.
 */
static void _mty_appcb (inflow_t *flow) {
	//
	// Prepare name, nmlen, ctl parameters
	char *name = NULL;
	int nmlen = flow->postnm - 1;
	uint8_t ctl = flow->buf [nmlen];
	if (mtyescapewish (MULTTY_ESC_BINARY, ctl)) {
		//
		// Interesting ctl; possibly interesting name
		if (nmlen >= 0) {
			name = flow->buf + 1;
		}
	} else {
		//
		// No interesting ctl; set it to <NUL>
		ctl = c_NUL;
	}
	//
	// Use the current MULTTY stream
	MULTTY *mty = flow->current_stream;
	if (mty == NULL) {
		//TODO// Can this ever happen?
		return;
	}
	//
	// Invoke the callback (trust it to unescape data)
	if (mty->cb_ready == NULL) {
		return;
	}
	mty->cb_ready (mty, mty->cb_userdata, name, nmlen, ctl);
}


/* Process stream commands: <SI>, <SO>, and <EM> with current stream.
 * There may or may not have been a preceding <SOH> name prefix.
 *
 * Return if suitable control codes were found.
 */
static bool _myt_streamctl (inflow_t *flow) {
	//
	// Fetch the control character
	if (flow->rdend >= flow->wrofs) {
		return false;
	}
	uint8_t ctl = flow->buf [flow->rdend];
	//
	// Handle <SI> or <SO> codes, with or without <SOH> name
	if ((ctl == c_SO) || (ctl == c_SI)) {
		flow->rdend++;
		TODO_FIND_OR_CREATE_AND_SET_AS_DEFAULT;
		return true;
	}
	//
	// Refuse to consider <EM> if there is no current stream
	if (flow->current_stream == NULL) {
		return false;
	}
	//
	// Handle the <EM> code, with or without <SOH> name
	if (ctl == c_EM) {
		flow->rdend++;
		TODO_TERMINATE_STREAM_AND_FOR_NOW_IGNORE_ERROR_CODE;
		return true;
	}
	//
	// No recognised control codes
	return false;
}


#ifdef MULTTY_MIXED

/* Process multiplexing commands: <DCx>, and <EM> without current stream.
 * There may or may not have been a preceding <SOH> name prefix.
 *
 * Return if suitable control codes were found.
 */
static bool _myt_multiplexctl (inflow_t *flow) {
	//
	// Fetch the control character
	if (flow->rdend >= flow->wrofs) {
		return false;
	}
	uint8_t ctl = flow->buf [flow->rdend];
	//
	// Handle <DC1> through <DC4> and <EM> control codes
	// (Break to finish after recognised control code)
	switch (ctl) {
	case c_PUP:	/* c_DC1 == c_PUP */
		TODO_FROM_CURRENT_TO_PARENT;
		TODO_SEARCH_SOH_AMONG_NEIGHBOURS;
		break;
	case c_PRM:	/* c_DC2 == c_PRM */
		TODO_SEARCH_SOH_AMONG_NEIGHHOURS;
		TODO_REMOVE_CURRENT_PROGRAM;
		break;
	case c_PDN:	/* c_DC3 == c_PDN */
		TODO_FROM_CURRENT_TO_CHILDSET;
		TODO_SEARCH_SOH_AMONG_NEIGHBOURS;
		break;
	case c_PSW:	/* c_DC4 == c_PSW */
		TODO_SEARCH_SOH_AMONG_NEIGHBOURS;
		break;
	case c_EM:	/* Reason for recent program end */
		if (flow->current_stream != NULL) {
			//
			// This must be handled by the stream
			return false;
		}
		TODO_HANDLE_EM;
		break;
	default:
		//
		// Unknown control code, 0 bytes recognised
		return false;
	}
	//
	// Control code was processed
	flow->rdend++;
	return true;
}

#endif


/* Register a callback function with a MULTTY handle,
 * possibly replacing the previous setting.  The new
 * setting may be NULL to forget the callback.
 *
 * Along with the callback function, a userdata pointer
 * may be registered and this will be provided alongside
 * the callback.
 */
void mtyregister_ready (MULTTY *mty, mtycb_ready *rdy, void *userdata) {
	mty->cb_ready = rdy;
	mty->cb_userdata = userdata;
}


/* Dispatch an input read event by appending to the buffer and
 * distributing as much as possible over programs.
 */
void multty_vin_dispatch (inflow_t *flow) {
	//
	// Try to read more.  May silently fail if non-blocking.
	if (!_mty_readmore (flow)) {
		return;
	}
	//
	// Require at least enough to decide on <SOH> presence/absence.
again:
	if (!_mty_getname (flow)) {
		return;
	}
	//
	// Now we can have one of three things, or an error.
	//  - application byte string
	//  - stream operations
	//  - program multiplexing
	if (_myt_streamctl (flow)) {
		//
		// We processed a stream control code
		;
#ifdef MULTTY_MIXED
	} else if (_myt_multiplexctl (flow)) {
		//
		// We processed a program multiplex control code
		// (Only when compiled with multiplexing support)
		;
#endif
	} else if (_myt_appstring (flow)) {
		//
		// We recognised an applicating byte string to process
		_mty_appcb (flow);
	} else {
		//
		// Not recognised, complain and skip codes
		TODO_CONSIDER_CUTTING_CODES_AND_LOOPING_TO_RETRY;
		return;
	}
	//
	// We processed a command; cleanup and try another.
	// This also cleans up the <SOH> name prefix, if any.
	_mty_reset_inflow (flow);
	goto again;
}

