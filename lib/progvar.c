/* mulTTY -> default program set, through a global variable.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>

#include "mtyp-int.h"


/* This global variable will be initialised as an empty program set
 * by the usual C conventions.  It is most conveniently referenced
 * with the MULTTY_PROGRAMS macro, that is a pointer to this data.
 */
struct multty_progset MULTTY_PROGRAMSET_DEFAULT = {
	.programs = NULL,
	.current  = NULL,
	.previous = NULL,
};
