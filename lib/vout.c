/* mulTTY -> flush buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <limits.h>

#include <errno.h>

#include <arpa2/multty.h>


/* We can use MULTTY_MUTEX_STDOUT to ensure that nobody else
 * tries to write out.  That is sad, but given the low level
 * of guarantees in POSIX commands, which state that writev()
 * is atomic, but also that it is not an error to return
 * that not all bytes were written, this may be the only way
 * to be sure...
 */

#if 0  /* OLD CODE, INT RETVAL */
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
#endif


/* INTERNAL ROUTINE for sending literaly bytes from an iovec
 * array to stdout.  This is used after composing a complete
 * structure intended to be sent.
 *
 * The primary function here is to send atomically, to
 * avoid one structure getting intertwined with another.
 * This is necessary for security and general correctness.
 * This requirement imposes a PIPE_BUF as maximum for len.
 *
 * Stream output should be sent such that it returns to
 * the default stream within the atomic unit, which is
 * established through a file buffering scheme.
 *
 * Returns true on succes, or false/errno.  Specifically
 * note EMSGSIZE, which is returned when the message is
 * too large (the limit is PIPE_BUF).  This might occur
 * when writing "<SOH>id<US>very_long_description<XXX>"
 * or similar constructs that user input inside an atom.
 * It may then be possible to send "<SOH>id<US><XXX>".
 */
bool mtyv_out (int len, int ioc, const struct iovec *iov) {
	if (len > PIPE_BUF) {
		errno = EMSGSIZE;
		return false;
	}
	ssize_t out = writev (1, iov, ioc);
	if (out < 0) {
		return false;
	} else if (out != len) {
		/* Inconsistency! refuse to do anything more */
		close (1);
		errno = ECONNABORTED;
		return false;
	} else {
		return true;
	}
}


#if 0  /* OLD CODE, INT RETVAL */
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
#endif

