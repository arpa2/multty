/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


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

