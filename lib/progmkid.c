/* mulTTY -> make an identity for a program
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Construct a standard identity structure from an identity string
 * (up to 32 chars) with an optionally appended <US> when it will
 * carry along a description.
 */
void mtyp_mkid (const char *id, bool with_descr, MULTTY_PROGID prgid) {
	int idlen = strnlen (id, 32);
	memcpy (prgid, id, idlen);
	memset (prgid+idlen, 0, sizeof (MULTTY_PROGID) - idlen);
	if (with_descr) {
		prgid [idlen] = c_US;
	}
}
