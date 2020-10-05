/* mulTTY -> open handle
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <arpa2/multty.h>


/* Open a MULTTY handle, and redirect it to stdout.
 *
 * Drop-in replacement for fopen() with FILE changed to MULTTY
 * and the path replaced by a stream name.  The stream shift is
 * inserted in the beginning and any buffer fills start after
 * that point, at the .shift offset.
 *
 * Returns a management structure on success, else NULL+errno.
 */
MULTTY *mtyopen (const char *streamname, const char *mode) {
	int streamnamelen = strlen (streamname);
	if (2 + streamnamelen >= PIPE_BUF / 4) {
		errno = EINVAL;
		return NULL;
	}
	if (!strchr (mode, 'w')) {
		errno = EINVAL;
		return NULL;
	}
	struct multty *retval = malloc (sizeof (struct multty));
	if (retval == NULL) {
		return NULL;
	}
	memset (retval, 0, sizeof (struct multty));
	retval->buf [retval->fill++] = c_SOH;
	mtyescape (MULTTY_ESC_ASCII, retval, streamname, streamnamelen);
	retval->buf [retval->fill++] = c_SO;
	retval->shift = retval->fill;
	return retval;
}

