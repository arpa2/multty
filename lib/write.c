/* mulTTY -> write binary data
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Send binary data to the given mulTTY steam.
 * Since it passes over ASCII, some codes will
 * be escaped, causing a change to the wire size.
 * Reading back would unescape and remove this.
 *
 * Drop-in replacement for write() with FD changed to MULTTY*.
 * Note: This is *NOT* a drop-in replacement for fwrite().
 * Returns buf-bytes written on success, else -1&errno
 */
ssize_t mtywrite (MULTTY *mty, const void *buf, size_t count) {
	size_t tgtlen = count;
	int esclen;
	const uint8_t *bufbyt = buf;
	while (tgtlen > 0) {
		esclen = mtyescape (MULTTY_ESC_BINARY, mty, bufbyt, tgtlen);
		if (mtyflush (mty) != 0) {
			if (tgtlen == 0) {
				/* Error at the start... return error */
				return -1;
			} else {
				/* Error halfway... return how far we got */
				break;
			}
		}
		bufbyt += esclen;
		tgtlen -= esclen;
	}
	return count - tgtlen;
}

