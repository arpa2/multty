/* mulTTY -> find a program in the program set
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>

#include "mtyp-int.h"


/* Find a program in the program set, based on
 * its MULTTY_PROGID.  Such names can be made
 * with the mtyp_mkid() function.
 */
MULTTY_PROG *mtyp_find (MULTTY_PROGSET *progset, const MULTTY_PROGID id_us) {
	MULTTY_PROG *retval;
	HASH_FIND (hh, progset->programs, id_us, sizeof (MULTTY_PROGID), retval);
	return retval;
}

