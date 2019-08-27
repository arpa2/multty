/* mulTTY program multiplexing -> raw mulTTY string passing
 *
 * TODO: Currently, mixing of multiplexing with streams is not safe.
 * There are cross-influences that disrupt their orthogonality:
 *  - streams can only write when their program is current
 *  - control sequences for streams and programs may interfere
 *  - both streams and programs use the <SOH> prefix
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdarg.h>

#include <arpa2/multty.h>


/* Send raw data for mulTTY program multiplexing.  This stands above
 * the streams for individual programs.  STREAMS SHOULD NOT USE THIS
 * BUT mtywrite() TO SEND BINARY DATA.
 *
 * This is not a user command, it is intended for mulTTY internals.
 *
 * The raw content is supplied as a sequence of pointer and length:
 *  - uint8_t *buf
 *  - int      len
 * The number of these pairs is given by the numbufs parameter.
 *
 * This returns >=0 on success, or else -1/errno.
 */
ssize_t mtyp_raw (int numbufs, ...) {
	//
	// Construct an iovec array to send
	int totlen;
	struct iovec output [numbufs];
	va_list pairs;
	va_start (pairs, numbufs);
	int i = 0;
	while (i < numbufs) {
		output[i].iov_base = va_arg (pairs, uint8_t *);
		totlen +=
		output[i].iov_len  = va_arg (pairs, int      );
	}
	va_end (pairs);
	//
	// Now send the iovec array, as atomically as it gets under POSIX
	return mtyv_out (totlen, numbufs, output);
}

