/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* We can use MULTTY_MUTEX_STDOUT to ensure that nobody else
 * tries to write out.  That is sad, but given the low level
 * of guarantees in POSIX commands, which state that writev()
 * is atomic, but also that it is not an error to return
 * that not all bytes were written, this may be the only way
 * to be sure...
 */

#ifdef MULTTY_MUTEX_STDOUT
#include <pthread.h>
// As long as only this routine writes to stdout, we can keep this mutex here
static pthread_mutex_t _multty_stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MULTTY_STDOUT_MUTEX_LOCK()   pthread_mutex_lock   (&_multty_flush_mutex)
#define MULTTY_STDOUT_MUTEX_UNLOCK() pthread_mutex_unlock (&_multty_flush_mutex)
#else
#define MULTTY_STDOUT_MUTEX_LOCK()
#define MULTTY_STDOUT_MUTEX_UNLOCK()
#endif


/* INTERNAL ROUTINE for sending literaly bytes from an iovec
 * array to stdout.  This is used after composing a complete
 * structure intended to be sent.
 *
 * This is heavily subject to the frivolity of POSIX specs
 * concerning atomicity.  On the one hand, it is said that
 * pwritev() is atomic, but on the other hand it is said
 * not to be an error when writes are partial.  For this
 * reason, we may define MULTTY_MUTEX_STDOUT and have mutex
 * locks around our access to stdout, while hoping that no
 * other senders attempt to do the same at the same time.
 * Those others however, may be other programs who hold a
 * duplicate of the file descriptor, which we cannot fix.
 *
 * Returns the number of bytes written, which ought to be
 * the same as the len parameter, which is expected to hold
 * the total of all iov[0..ioc-1].  Or returns -1/errno.
 */
int mtyv_out (int len, int ioc, const struct iovec *iov) {
	int retval = 0;
	size_t ofs = 0;
	MULTTY_STDOUT_MUTEX_LOCK ();
	do {
		ssize_t out = pwritev (1, iov, ioc, ofs);
		if (out < 0) {
			retval = -1;
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
	MULTTY_STDOUT_MUTEX_UNLOCK ();
	return retval;
}

