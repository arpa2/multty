/* mulTTY -> dispatch of streams within one program
 *
 * This returns a signal to an assumed program
 * dispatcher when it runs into a multiplexer
 * command.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <assert.h>

#include <arpa2/multty.h>


/* Given input bytes, dispatch as much of it as
 * possible between the streams of one program.
 * Program switches are not accepted by this
 * function, and lead to a buffer being returned.
 *
 * The MULTTY provided is the first in the list
 * for a given program.  This routine itself is
 * not aware of program multiplexing.
 *
 * If the input stream is blocking, then this call
 * is also blocking.
 *
 * Returns false when unprocessed program switching
 * input is waiting to be further processed, or true
 * when it could all be processed as stream input.
 */
bool mtydispatch_internal (MULTTY *current) {
	/* This function does pretty clever things...
	 * It parses <SOH> and places it at the start.
	 * <SOH>...<SI> and <SOH>...<SO> change stream,
	 * for which remaining content changes buffer.
	 * <SOH>...<EM> terminates the named stream.
	 * <EM> alone terminates the current stream.
	 * <DC1> through <DC4> return false, also when
	 * they are prefixed with <SOH>, for multiplexer
	 * processing.
	 * TODO: Silly to have per-stream input buffers!
	 */
	//
	// Read available bytes if the buffer has room
	if (current->fill < sizeof (current->buf)) {
		ssize_t extra = read (1,
				current->buf + current->fill,
				sizeof (current->buf) - current->fill);
		if (extra < 0) {
			//
			// No idea what is wrong, or what to do
			return true;
		}
		current->fill += extra;
	}
	//
	// Iterate over the buffer
	bool retval = true;
	while (current->rdofs < current->fill) {
		uint8_t ch = current->buf [current->rdofs];
		char *name = NULL;
		int namelen = -1;
		if (ch == c_SOH) {
			//
			// Handle initial <SOH>
			name = current->buf + current->rdofs + 1;
			int restlen = sizeof (current->fill) - 1 - current->rdofs - 1;
			namelen = 0;
			while (ch = name [namelen], ((ch >= 0x20) || (ch == c_US))) {
				if (namelen > restlen) {
					//
					// Incomplete chunk, defer processing
					retval = true;
					break;
				}
				namelen++;
			}
			//
			// We found a complete name and control, now skip ahead
			//TODO// MOVE AFTER STREAM SWITCH
			current->rdofs += 1 + namelen;
			if (ch == c_SO) {
				if (current->shift_in) {
					current->shift_in = false;
				} else {
					current->rdofs++;
				}
			} else if (ch == c_SI) {
				if (current->shift_in) {
					current->rdofs++;
				} else {
					current->shift_in = true;
				}
			}
		}
		if ((ch >= c_DC1) && (ch <= c_DC4)) {
			//
			// STOP!!!  This is program multiplexing!
			retval = false;
			break;
		}
		//
		// Standard case: chase until control + dispatch
		if (name == NULL) {
			ch = c_NUL;
		} else if (ch == c_EM) {
			//
			// End of Media for the named or current stream
			//TODO// END STREAM
		} else if ((ch == c_SI) || (ch == c_SO)) {
			//
			// Switch stream, perhaps to another one
			TODO_ONLY_FOR_DIFFERENT_STREAM
			//
			// Find the named stream
			MULTTY *other = TODO_FIND_NEW_STREAM;
			if (other != current) {
				//
				// Move content to the other stream
				memcpy (other->buf,
					current->rdofs,
					current->fill - current->rdofs);
				//
				// Clear content from current stream
				current->fill = 0;
				current->rdofs = 0;
				//
				// Switch to the other stream
				current = other;
				TODO_EXPORT_THIS_NEW_CURRENT;
			}
		}
		//
		// Deliver data to the stream via callback
		current->cb_ready (current, current->cbdata_ready, name, namelen, ch);
		//
		// Assume processing; skip over this chunk of data
		// Stop at the next <SOH> or program multiplexing command
		while (current->rdofs < current->fill) {
			ch = current->buf [current->rdofs];
			if ((ch == c_SOH) || (ch == c_EM) || ((ch >= c_DC1) && (ch <= c_DC4))) {
				break;
			}
			current->rdofs++;
		}
	}
	//
	// Move the buffer back to base
	int skip = current->rdofs;
	if (skip > 0) {
		memmove (current->buf,
		         current->buf  + skip,
		         current->fill - skip);
		current->fill -= skip;
		current->rdofs = 0;
	}
	//
	// Return the verdict
	return retval;
}

