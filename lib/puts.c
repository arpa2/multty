/* mulTTY -> put string
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Send an ASCII string to the given mulTTY steam.
 * Since it is ASCII, it will be escaped as seen fit.
 *
 * TODO: Consider swallowing <CR> and mapping <LF> to <CR><LF>.
 *
 * Drop-in replacement for fputs() with FILE changed to MULTTY.
 * Returns >=0 on success, else EOF+errno
 */
// Moved to inline: int mtyputs (const char *s, MULTTY *mty);


/* Send an ASCII string buffer to the given mulTTY stream.
 * Since it is ASCII, it will be escaped as seen fit.
 *
 * Return true on success, else false/errno.
 */
bool mtyputstrbuf (MULTTY *mty, const char *strbuf, int buflen) {
	int retval = 0;
	int esclen;
	while (buflen > 0) {
		esclen = mtyescape (MULTTY_ESC_ASCII, mty, strbuf, buflen);
		strbuf += esclen;
		buflen -= esclen;
		if (mtyflush (mty) != 0) {
			return false;
		}
	}
	return true;
}

