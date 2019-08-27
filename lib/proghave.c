/* mulTTY -> have a program in the program set
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <errno.h>

#include <arpa2/multty.h>

#include "mtyp-int.h"


/* Have a program in the program set, silently
 * sharing if it already exists.  The id is how it
 * is located, the opt_descr is for human display
 * purposes and may be later updated if it is
 * provided here.  Whether or not a description
 * was added is part of the program identity.
 *
 * The identity in id_us can be made with the
 * mtyp_mkid() function.  You should take care
 * to provide the opt_descr iff you constructed
 * an id_us including the with_descr option.
 */
MULTTY_PROG *mtyp_have (MULTTY_PROGSET *progset, const MULTTY_PROGID id_us, const char *opt_descr) {
	MULTTY_PROG *prg = mtyp_find (progset, id_us);
	if (prg == NULL) {
		prg = malloc (sizeof (MULTTY_PROGSET));
		if (prg == NULL) {
			errno = ENOMEM;
			return NULL;
		}
		memset (prg, 0, sizeof (MULTTY_PROGSET));
		memcpy (prg->id_us, id_us, sizeof (MULTTY_PROGID));
		prg->descr = strdup (opt_descr);
		if (prg->descr == NULL) {
			free (prg);
			errno = ENOMEM;
			return NULL;
		}
	} else {
		if ((prg->descr != NULL) && (opt_descr != NULL) && (*opt_descr != c_NUL)) {
			char *new_descr = strdup (opt_descr);
			if (new_descr != NULL) {
				free  (prg->descr);
				prg->descr = new_descr;
			}
		}
	}
}

