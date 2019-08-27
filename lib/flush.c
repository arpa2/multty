/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Flush the MULTTY buffer to the output, using writev() to
 * ensure atomic sending, so no interrupts with other streams
 * even in a multi-threading program.
 *
 * The mty is setup with the proper iov[] describing its
 * switch from stdout, to its buffer, back to stdout.  This
 * is pretty trivial for stdout, of course, and treated in
 * more optimally than the others.
 *
 * The buffer is assumed to already be escaped inasfar as
 * necessary.
 *
 * Drop-in replacement for fflush() with FILE changed to MULTTY.
 * Returns 0 on success, else EOF+errno.
 */
int mtyflush (MULTTY *mty) {
	int retval = 0;
	struct iovec *iov = mty->iov;
	int ioc = mty->ioc;
	size_t len = mty->iov[1].iov_len;
	if (ioc == 1) {
		iov++;
	} else {
		len += mty->iov[0].iov_len + mty->iov[2].iov_len;
	}
	return mtyv_out (len, ioc, iov);
}

