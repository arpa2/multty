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

#ifdef MULTTY_MIXED
#incude "mtyp-int.h"
#endif


struct multty_instream {
	char *name;
	struct multty_instream *next;
	void (*cb_ready) ();
	void *cb_userdata;
	uint8_t shiftctl;
};
typedef struct multty_instream MULTTY_INSTREAM;


struct multty_inflow {
	int infd;
	char buf [PIPE_BUF];
	int wrofs;	/* where to write next */
	int rdofs;	/* where to read  next */
	int rdend;	/* where reading stopped */
	int prenm;	/* points at name start, or is -1 */
	int postnm;	/* points at control beyond name, or 0 */
	int usofs;	/* points at optional <US> in name, or is 0 */
#ifdef MULTTY_MIXED
	MULTTY_PROG *curprog;
#define default_stream curprog->TODO_INPUT_STREAM_LIST
#define current_stream curprog->TODO_CURRENT_INPUT_STREAM
#else
	MULTTY_INSTREAM  default_stream;
	MULTTY_INSTREAM *current_stream;
#endif
};
typedef struct multty_inflow MULTTY_INFLOW;


/* Open an inflow for a given file descriptor.
 *
 * Returns non-NULL pointer or NULL/errno.
 */
MULTTY_INFLOW *mtyinflow (int infd) {
	if (infd < 0) {
		errno = EINVAL;
		return NULL;
	}
	MULTTY_INFLOW *retval = malloc (sizeof (MULTTY_INFLOW));
	if (retval == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memset (retval, 0, sizeof (MULTTY_INFLOW));
	retval->infd = infd;
	retval->prenm = -1;
	return retval;
}


/* Close an inflow.
 */
void mtyinflow_close (MULTTY_INFLOW *flow) {
	"TODO_CLOSE";
	free (flow);
}


/* Reset the inflow to prepare it to process the next chunk.
 * This also enables pushing back content upon next read.
 */
static void _mty_reset_inflow (MULTTY_INFLOW *flow) {
	flow->prenm = -1;
	flow->postnm = 0;
	flow->usofs = 0;
	flow->rdofs = flow->rdend;
}


/* Remove togo characters at ofs in flow->buf.
 * Update flow->wrofs.
 */
static void _mty_cutback (MULTTY_INFLOW *flow, int ofs, int togo) {
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
static bool _mty_readmore (MULTTY_INFLOW *flow) {
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
 * Return the number of characters that would have to be skipped.
 */
static int _mty_badchar (MULTTY_INFLOW *flow, int badpos) {
	//
	// Log an error
	int badlen = (flow->buf [badpos] == c_DLE) ? 2 : 1;
	if (badlen == 2) {
		syslog (LOG_ERR, "Bad escaped character 0x%02x in mulTTY input\n", flow->buf [badpos+1] ^ 0x40);
	} else {
		syslog (LOG_ERR, "Bad character 0x%02x in mulTTY input\n", flow->buf [badpos]);
	}
	return badlen;
}


/* Parse a potential <SOH> name prefix, setting postnm to the following control.
 *
 * Return true when sufficient information was available, false to defer.
 * The value flow->postnm is not set if false is returned.
 */
static bool _mty_getname (MULTTY_INFLOW *flow) {
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
			flow->usofs = i - 1;
			phase++;
		} else if (mtyescapewish (ctl_no_us [phase], flow->buf [i])) {
			//
			// This is a control character, and not part of the name
			// (Before <US> be really tight; after, avoid mulTTY confusion)
			flow->prenm = flow->rdofs + 1;
			flow->postnm = i - 1;
			if (phase == 0) {
				flow->usofs = 0;
			}
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
static bool _mty_appstring (MULTTY_INFLOW *flow) {
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


/* Look for the input stream, by name or, if that is NULL,
 * by returning the default.  When opt_namelen<0 it will
 * be determined with strlen(), otherwise it is considered
 * an actual string length.
 *
 * TODO: Flag to bypass creation if not found?
 *
 * Returns the stream if it exists, else NULL/errno=ENOENT.
 * Errors are ENOMEM for allocation error or (TODO)
 */
static MULTTY_INSTREAM *_mty_instream_byname (MULTTY_INFLOW *flow,
			char *opt_name, int opt_namelen) {
	MULTTY_INSTREAM *instream = &flow->default_stream;
	bool notfound = (opt_name != NULL);
	if (notfound && (opt_namelen < 0)) {
		opt_namelen = strlen (opt_name);
	}
	while (notfound && (instream != NULL)) {
		instream = instream->next;
		notfound = (strncmp (instream->name, opt_name, opt_namelen) != 0)
				|| (instream->name [opt_namelen] != '\0');
	}
	if (notfound) {
		errno = ENOENT;
	}
	return instream;
}


/* Look for the input stream, named as in the flow, and otherwise
 * the current stream.  Update the current stream if one by its
 * name is found.
 *
 * If it does not exist yet, create a new instream with the name.
 */
static MULTTY_INSTREAM *_mty_instream (MULTTY_INFLOW *flow) {
	MULTTY_INSTREAM *retval;
	if (flow->prenm == -1) {
		//
		// Assume the current stream as unnamed default
		retval = flow->current_stream;
	} else {
		//
		// We have a name, so we should look for it
		int nmlen, nmlen_max;
		if (flow->usofs > 0) {
			nmlen = flow->usofs  - flow->prenm;
			nmlen_max = 33;
		} else {
			nmlen = flow->postnm -  flow->prenm;
			nmlen_max = 32;
		}
		//
		// Limit the name to 32 identifying characters
		// of accept <US> for an extra length of 33.
		if (nmlen > nmlen_max) {
			//
			// <US> lies too far off, stick to 32
			nmlen = 32;
		}
		//
		// Use the name to locate a stream
		retval = _mty_instream_byname (flow, flow->buf + flow->prenm, nmlen);
		if (retval != NULL) {
			flow->current_stream = retval;
		}
	}
	return retval;
}


/* Handle application byte sequence by callback invocation.
 */
static void _mty_appcb (MULTTY_INFLOW *flow) {
	MULTTY_INSTREAM *mis = _mty_instream (flow);
	//
	// Only continue when the (named) flow exists
	if (mis == NULL) {
		return;
	}
	//
	// Only continue when a callback is registered
	if (mis->cb_ready == NULL) {
		return;
	}
	//
	// Invoke the callback (trust it to unescape data)
	char ctl = flow->buf [flow->postnm];
	mis->cb_ready (flow, mis->cb_userdata, ctl);
}


/* Process stream commands: <SI>, <SO>, and <EM> with current stream.
 * There may or may not have been a preceding <SOH> name prefix.
 *
 * Return if suitable control codes were found.
 */
static bool _mty_streamctl (MULTTY_INFLOW *flow) {
	//
	// Fetch the control character
	if (flow->rdend >= flow->wrofs) {
		return false;
	}
	uint8_t ctl = flow->buf [flow->rdend];
	//
	// Handle <SI> or <SO> codes, with or without <SOH> name
	if ((ctl == c_SO) || (ctl == c_SI)) {
		MULTTY_INSTREAM *newcur = _mty_instream (flow);
		if (newcur->shiftctl == ctl) {
			//
			// The shift matches current status, so discard it
			flow->rdend++;
		} else {
			//
			// Include <SI> or <SO> in the application string
			// and for now, take note of its new value
			newcur->shiftctl = ctl;
		}
		return true;
	}
	//
	// Refuse to consider <EM> if there is no current stream
	if (flow->current_stream == NULL) {
		return false;
	}
	//
	// Handle the <EM> stream control, with or without <SOH> report
	if (ctl == c_EM) {
		flow->rdend++;
		// LL_DELETE (flow->input_streams, flow->current_stream);
		MULTTY_INSTREAM **herep = &flow->default_stream.next;
		while (*herep != NULL) {
			if (flow->current_stream == *herep) {
				*herep = flow->current_stream->next;
				break;
			}
			herep = &(*herep)->next;
		}
		//
		// Remove and forget the current stream
		free (flow->current_stream);
		flow->current_stream = NULL;
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
static bool _mty_multiplexctl (MULTTY_INFLOW *flow) {
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


/* Register a callback function with arbitrary userdata
 * pointer, to be invoked when data arrives for the
 * named stream at the given inflow.
 *
 * The inflow may be NULL to reference the MULTTY_STDIN
 * default.
 *
 * The stream name may be NULL to indicate the default
 * stream, which may be compared to stdin/stdout.  The
 * name is assumed to be a static string.
 *
 * The callback function may be NULL to indicate that
 * the previously registered function is to stop being
 * called.  This may involve the cleanup of the stream
 * from the inflow (when it is not the default), though
 * that may also be deferred until inflow cleanup.
 *
 * The userdata is passed whenever the callback function
 * is invoked; this also happens when it is NULL.
 *
 * Return true on success, else false/errno
 */
bool mtyregister_ready (MULTTY_INFLOW *flow, char *stream,
			mtycb_ready *rdy, void *userdata) {
	MULTTY_INSTREAM *instream = _mty_instream_byname (flow, stream, -1);
	if (instream == NULL) {
		if (errno == ENOENT) {
			instream = malloc (sizeof (MULTTY_INSTREAM));
			if (instream == NULL) {
				errno = ENOMEM;
			}
		}
		if (instream == NULL) {
			return false;
		}
		memset (instream, 0, sizeof (MULTTY_INSTREAM));
		instream->next = flow->default_stream.next;
		instream->name = stream;
		instream->shiftctl = c_SO;
		flow->current_stream =
		flow->default_stream.next = instream;
	}
	instream->cb_ready = rdy;
	instream->cb_userdata = userdata;
}


/* Dispatch an input read event by appending to the buffer and
 * distributing as much as possible over programs.
 */
void multty_vin_dispatch (MULTTY_INFLOW *flow) {
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
	if (_mty_streamctl (flow)) {
		//
		// We processed a stream control code
		;
#ifdef MULTTY_MIXED
	} else if (_mty_multiplexctl (flow)) {
		//
		// We processed a program multiplex control code
		// (Only when compiled with multiplexing support)
		;
#endif
	} else if (_mty_appstring (flow)) {
		//
		// We recognised an applicating byte string to process
		_mty_appcb (flow);
	} else {
		//
		// Not recognised, complain and skip codes
		int badlen = _mty_badchar (flow, flow->rdend);
		_mty_cutback (flow, flow->rdend, badlen);
	}
	//
	// We processed a command; cleanup and try another.
	// This also cleans up the <SOH> name prefix, if any.
	_mty_reset_inflow (flow);
	goto again;
}

