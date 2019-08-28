/* mulTTY -> make an identity for a program
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


/* Construct a standard identity structure from an identity string
 * (up to 32 chars) with an optionally appended <US> when it will
 * carry along a description.
 *
 * Return true on success, or else false.
 */
bool mtyp_mkid (const char *id, bool with_descr, MULTTY_PROGID prgid) {
	int idlen = strlen (id);
	if ((idlen > 32) || (!mtyescapefree (MULTTY_ESC_BINARY, id, idlen))) {
		return false;
	}
	memcpy (prgid, id, idlen);
	memset (prgid+idlen, 0, sizeof (MULTTY_PROGID) - idlen);
	if (with_descr) {
		prgid [idlen] = c_US;
	}
	return true;
}
