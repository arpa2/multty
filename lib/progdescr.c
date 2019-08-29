/* mulTTY -> have a program in the program set
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <errno.h>

#include <arpa2/multty.h>

#include "mtyp-int.h"


/* Describe a program with a new string.  This
 * includes a test whether the string contains
 * only passable ASCII content, so no control codes
 * that could confuse mulTTY or ASCII passing.
 *
 * Note that setting a description is only welcome
 * if it was opened to have one.
 *
 * Returns true on success, or else false/errno.
 */
bool mtyp_describe (MULTTY_PROG *prog, const char *descr) {
	if ((descr == NULL) || !mtyescapefree (MULTTY_ESC_MIXED, descr, strlen (descr))) {
		errno = EINVAL;
		return NULL;
	}
	//TODO// Possibly check the size of the description
	const char *new_descr = strdup (descr);
	if (new_descr != NULL) {
		if (prog->descr) {
			free ((void *) prog->descr);
		}
		prog->descr = new_descr;
	}
}

