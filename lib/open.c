/* mulTTY -> open handle
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>

#include <errno.h>


/* Open an MULTTY buffer for input ("r") or output ("w")
 * and using the given stream name.  TODO: Currently the
 * mode "r" / "w" is not used.  TODO: Output is always
 * shift-out based, using `<SO>` ASCII, not `<SI>`.
 *
 * The mty is setup with the proper iov[] describing its
 * switch from stdout, to its buffer, back to stdout.  This
 * is pretty trivial for stdout, of course, and treated in
 * more optimally than the others.
 *
 * The buffer is assumed to already be escaped inasfar as
 * necessary.
 *
 * There are default handles opened for MULTTY_STDOUT and
 * MULTTY_STDIN and MULTTY_STDERR.  Note that "stderr" is
 * in no way special because it is just a named stream,
 * but "stdout" and "stdin" are default streams and need
 * no stream shifting because we return to it after all
 * other writes and reads.  If you open "stdout" and
 * "stdin" here you do get them with the explicit stream
 * shift, which is harmless and actually necessary when
 * not all other uses return to the default stream.
 *
 * Drop-in replacement for fopen() with FILE changed to MULTTY.
 * Returns a handle on success, else NULL+errno.
 */
MULTTY *mtyopen (const char *streamname, const char *mode) {
	//ALT// We could also store the streamname in the buffer
	MULTTY *mty = malloc (sizeof (MULTTY) + strlen (streamname) + 1);
	if (mty == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	int nmlen = strlen (streamname);
	/* Insert a shift statement at the start */
	/* Note: MULTTY_STDOUT and MULTTY_STDIN don't have this */
	mty->buf [0] = c_SOH;
	memcpy (mty->buf + 1, streamname, nmlen);
	mty->buf [1 + nmlen] = c_SO;
	mty->shift = 2 + nmlen;
	/* Always start buffer filling after the shift */
	mty->fill = mty->shift;
	/* Return the successfully filled handle */
	return mty;
}

