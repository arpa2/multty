/* mulTTY -> unescape input streams
 *
 * Removal of escape codes according to the profile.
 * This works most reliably when using the same profile
 * as during escaping, especially during layering of
 * escaping.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>



/* Given the number of bytes to be extracted from the
 * MULTTY handle, assuming it is stable at this time.
 * This is the number of bytes that will be delivered
 * by mtyunescape() if the destlen is high enough.
 * When less is delivered, a further read will provide
 * the remaining bytes.
 */
int mtyinputsize (uint32_t escstyle, MULTTY *mty) {
	int outsz = 0;
	int ofs;
	for (ofs = mty->rdofs; ofs < mty->fill; ofs++) {
		uint8_t ch = mty->buf [ofs];
		if (ch == c_DLE) {
			/* <DLE> only occurs unescaped */
			/* TODO: <DLE> before funnies? */
			continue;
		} else if (ch == c_SOH) {
			break;
		} else {
			/* TODO: <NUL> and <IAC>? */
			outsz++;
		}
	}
}



/* Extract escaped data from a MULTTY handle, and place
 * it in the given buffer.  The return value is the
 * number of bytes actually retrieved.  The size of the
 * destination is never more than the size of the source.
 *
 * The destination will not be filled with mulTTY codes
 * for stream switching or program multiplexing.  Future
 * releases will support delegation of these facilities,
 * which may lead to more variety in what is internal.
 * 
 * Inasfar as control codes are used as stream-internal
 * separators they will be kept.  As a consequence, the
 * <SOH> naming before stream switching and program
 * multiplexing is kept internally, but the <SO> or <SI>
 * control codes at the end of stream switches are
 * passed when they change the stream's shift status.
 *
 * Since <SOH> naming prefixes may serve many uses, it
 * is always taken out and delivered as the beginning
 * of a separate callback operatior.  In other words,
 * this operation does not pass in <SOH> fragments,
 * unless these result from escaping, of course.
 *
 * TODO: Consider additional checking of input:
 *  - removing  NUL characters (if they were escaped)
 *  - rejecting IAC characters
 *  - rejecting DLE before unexpected codes
 *  - rejecting DLE before impossible codes
 */
int mtyunescape (uint32_t escstyle, MULTTY *mty,
		uint8_t *dest, int destlen) {
	int destout = 0;
	bool got_dle = mty->got_dle;
	while ((destout < destlen) && (mty->rdofs < mty->fill)) {
		uint8_t bufc = mty->buf [mty->rdofs++];
		if (got_dle) {
			dest [destout++] = bufc ^ 0x40;
			got_dle = false;
		} else if (bufc == c_DLE) {
			got_dle = true;
		} else {
			dest [destout++] = bufc;
		}
	}
	mty->got_dle = got_dle;
	return destout;
}

