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
 * There are no default open handles.  You have to open
 * a handle for "stdout" (if you don't use puts() and co)
 * and "stdin" and "stderr" yourself.  The default place
 * of "stdin" and "stdout" is recognised in this routine.
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
	mty->shift [0      ] = '\x01';  /* <SOH> */
	mty->shift [1+nmlen] = '\x0f';  /* <SO>  */
	memcpy (mty->shift+1, streamname, nmlen);
	if ((strcmp (streamname, "stdin" ) == 0) ||
	    (strcmp (streamname, "stdout") == 0)    ) {
		/* Skip first and last elements */
		mty->ioc = 1;
	} else {
		mty->ioc = 3;
		mty->iov[0].iov_base = mty->shift;
		mty->iov[2].iov_base = mty->shift + 1 + nmlen;
		mty->iov[0].iov_len  = 2+nmlen;
		mty->iov[2].iov_len  = 1;
	}
	/* The buffer itself is always in mty->iov[1] */
	mty->iov[1].iov_base = mty->buf;
	mty->iov[1].iov_len  = 0;
	/* Return the successfully filled handle */
	return mty;
}

