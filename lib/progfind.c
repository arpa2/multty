/* mulTTY -> find a program in the program set
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>

#include "mtyp-int.h"


/* Find a program in the program set, based on
 * its 33-character name with optionally included
 * <US> attachment for programs with a description.
 */
MULTTY_PROG *mtyp_find (MULTTY_PROGSET *progset, const char id_us[33]) {
	MULTTY_PROG *retval;
	HASH_FIND (hh, progset->programs, id_us, 33, retval);
	return retval;
}

