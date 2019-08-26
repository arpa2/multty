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
int mtyputs (const char *s, MULTTY *mty) {
	int retval = 0;
	int tgtlen = strlen (s);
	int esclen;
	while (tgtlen > 0) {
		esclen = mtyescape (MULTTY_ESC_ASCII, mty, s, tgtlen);
		s      += esclen;
		tgtlen -= esclen;
		retval = mtyflush (mty);
	}
}

