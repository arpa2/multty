/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* TODO: How to know if a stream mixes into a multi-program context?
 *       In a single-program context, such as an application, it is
 *       silly to switch to a program at this point.  For mixed
 *       programs it is downright convenient.  At least his set of
 *       declarations cause an error when mixing wrongly.
 */

#ifndef MULTTY_MIXED
/* Placeholder for mtyp_switch() in absense of program mixing.
 *
 * TODO: Streams may need to claim until they release.
 *
 * Return 0 for success.
 */
inline int mtyp_switch (MULTTY_PROG *prog) {
	return 0;
}
#endif


/* Flush the MULTTY buffer to the output, using atomic
 * sending of up to PIPE_BUF bytes, so no interrupts with
 * other streams even in a multi-threading program.  Return
 * to the default stream in the same PIPE_BUF atomic unit.
 *
 * The buffer is assumed to already be escaped inasfar as
 * necessary.  This is usually assured by writing into it
 * with mtyescape()
 *
 * Drop-in replacement for fflush() with FILE changed to MULTTY.
 * Returns 0 on success, else EOF/errno.
 */
int mtyflush (MULTTY *mty) {
	int retval = 0;
	struct iovec io0;
	io0.iov_base = mty->buf;
	io0.iov_len  = mty->fill;
	//
	// If this stream is assigned to a program, switch to it
	if (mty->prog != NULL) {
		/*TODO:RETVAL*/
		mtyp_switch (mty->prog);
	}
	//
	// We need to add <SO> to mty->buf for all stream shifters
	if (mty->shift > 0) {
		//
		// Append c_SO; we declared an overflow position in MULTTY
		mty->buf [io0.iov_len++] = c_SO;
	}
	if (mtyv_out (io0.iov_len, 1, &io0)) {
		//
		// Reset to the shift prefix, not to an empty buffer
		mty->fill = mty->shift;
		return 0;
	} else {
		//
		// Report failure just like fflush() would do it
		return EOF;
	}
}

