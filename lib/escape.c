/* mulTTY -> escape traffic while building a buffer
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Check whether escaping is useful for a character under the
 * given escape style.  The style exists to minimise traffic
 * and avoid confusion ("why is my LF escaped?")
 *
 * There are a few styles, which we all port back to the range
 * of control codes.
 *  - most characters are printable, and never escaped
 *  - classical control codes are escaped in binary data
 *  - character <DEL> 0x7f is escaped when <BS>  0x08 is
 *  - character <IAC> 0xff is escaped when <NUL> 0x00 is
 *
 * The <IAC> character is specific to Telnet, where it is the
 * only break with "8-bit clean" transport.  Since mulTTY may
 * be used on port 23, we should avoid this kind of confusion
 * and will escape when the assumption is binary, when <NUL>
 * should also be protected by escaping.
 *
 * Escaping is not done here.  It is simply a prefix <DLE> and
 * the character will be added XOR 0x40.
 */
bool mtyescapewish (uint32_t style, uint8_t ch) {
	if (ch == 0x7f) {
		ch = 0x08;
	} else if (ch == 0xff) {
		ch = 0x00;
	}
	if (ch >= 0x20) {
		return false;
	} else {
		return (style & (1 << ch)) != 0;
	}
}


/* Check that a character sequence is free from the wish to
 * escape any of its characters, under the given style.
 * This may be used to assure that inserts that end up in
 * mulTTY structures have no confusing characters.
 *
 * As an example, "<SOH>id<US>tralala<XXX>" strings might
 * be constrained to have no control characters in id, and
 * only ASCII including high-top-bit in tralala.  This
 * should not constrain proper use cases, but it could not
 * be abused to hijack mulTTY streams.  The difference
 * would be made with the escape style parameter, set to
 * MULTTY_ESC_BINARY for the id and MULTTY_ESC_MIXED for
 * tralala.  The latter could include <CR><LF> and so on,
 * which have no place in binary content, but neither has
 * a place for embedded <DLE> or <SOH> characters to avoid
 * accidentally or malicuously overtaking <US> or <XXX>.
 */
bool mtyescapefree (uint32_t style, const uint8_t *ptr, int len) {
	while (len-- > 0) {
		if (mtyescapewish (style, (uint8_t) *ptr++)) {
			return false;
		}
	}
	return true;
}


/* Escape a string and move it into the indicated MULTTY buffer.
 * The escaping style is provided as a parameter.
 *
 * Escaping is always done by prefixing `<DLE>` and adding the
 * character XOR 0x40.  Control codes end up as 0x40 to 0x5f,
 * DEL becomes 0x3f and Telnet IAC becomes 0xbf.
 *
 * TODO: Consider swallowing <CR> and mapping <LF> to <CR><LF>.
 *
 * Returns the number of escaped characters, not counting the
 * escapes themselves.  Does not fail other than incomplete
 * translation, which is always accountable to a filled buffer.
 * Note that the return value may be 0, but mtyflush() should
 * then allow further use of this function.
 */
size_t mtyescape (uint32_t style, MULTTY *mty, const uint8_t *ptr, size_t len) {
	size_t done = 0;
	while (len-- > 0) {
		//
		// Try to find the room for one more character
		uint8_t c = *ptr++;
		bool esc = mtyescapewish (style, c);
		if (mty->fill + (esc ? 2 : 1) >= sizeof (mty->buf) - 1) {
			goto stophere;
		}
		//
		// Given the room, place the extra character in the buffer
		if (esc) {
			mty->buf [mty->fill++] = c_DLE;
			mty->buf [mty->fill++] = c ^ 0x40;
		} else {
			mty->buf [mty->fill++] = c;
		}
		done++;
	}
stophere:
	return done;
}

