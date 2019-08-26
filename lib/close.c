/* mulTTY -> close handle
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Close the MULTTY handle, after flushing any remaining
 * buffer contents.
 *
 * Drop-in replacement for fclose() with FILE changed to MULTTY.
 * Returns 0 on success, else EOF+errno.
 */
int mtyclose (MULTTY *mty) {
	int retval = mtyflush (mty);
	free (mty);
	return retval;
}

