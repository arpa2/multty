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
bool mtyescapewish (uint32_t style, char ch) {
	if (ch == 0x7f) {
		ch = 0x08;
	} else if (ch == 0xff) {
		ch = 0x00;
	}
	if (ch < 0x20) {
		return false;
	} else {
		return (style & (1 << ch)) != 0;
	}
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
	int pos = mty->iov[1].iov_len;
	while (len-- > 0) {
		char c = *ptr++;
		bool esc = mtyescapewish (style, c);
		if (pos + (esc ? 2 : 1) >= sizeof (mty->buf)) {
			goto stophere;
		}
		if (esc) {
			mty->buf [pos++] = '\x10';  /* <DLE> */
			mty->buf [pos++] = c ^ 0x40;
		} else {
			mty->buf [pos++] = c;
		}
		done++;
	}
stophere:
	mty->iov[1].iov_len += done;
	return done;
}

