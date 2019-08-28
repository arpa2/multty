/* mulTTY -> drop a program from the program set
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>

#include "mtyp-int.h"


/* Drop a program in the program set, silently
 * ignoring if it is absent.  The id is how it
 * is located, the with_descr option indicates
 * if a description should be attached, as that
 * differentiates the name.
 */
void mtyp_drop (MULTTY_PROGSET *progset, MULTTY_PROG *prog) {
	HASH_DEL (progset->programs, prog);
	if (progset->current == prog) {
		progset->current = NULL;
	}
	if (progset->previous == prog) {
		progset->previous = NULL;
	}
	if (prog->descr != NULL) {
		free ((void *) prog->descr);
	}
	free (prog);
}

