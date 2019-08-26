/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* We can use MULTTY_MUTEX_FLUSH to ensure that nobody else
 * tries to write out.  That is sad, but given the low level
 * of guarantees in POSIX commands, which state that writev()
 * is atomic, but also that it is not an error to return
 * that not all bytes were written, this may be the only way
 * to be sure...
 */

#ifdef MULTTY_MUTEX_FLUSH
#include <pthread.h>
//TODO// May have to be "extern" if more needs it
pthread_mutex_t _multty_flush_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MULTTY_FLUSH_MUTEX_LOCK()   pthread_mutex_lock   (&_multty_flush_mutex)
#define MULTTY_FLUSH_MUTEX_UNLOCK() pthread_mutex_unlock (&_multty_flush_mutex)
#else
#define MULTTY_FLUSH_MUTEX_LOCK()
#define MULTTY_FLUSH_MUTEX_UNLOCK()
#endif


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
	size_t ofs = 0;
	MULTTY_FLUSH_MUTEX_LOCK ();
	do {
		ssize_t out = pwritev (1, iov, ioc, ofs);
		if (out < 0) {
			retval = EOF;
			break;
		}
		/* The NOTES for write(2) suggest that only very
		 * large writes may go awry, or PIPE writes that
		 * exceed the buffer for that pipe.  Concluding
		 * that this never happens... but nonetheless
		 * fixing it just-in-case.  POSIX can be silly!
		 */
		ofs += out;
	} while (len > ofs);
	MULTTY_FLUSH_MUTEX_UNLOCK ();
	return retval;
}

