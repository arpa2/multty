/* mulTTY -> send a PSW==DC4 to switch to a program in the current set.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <errno.h>

#include <arpa2/multty.h>

#include "mtyp-int.h"



/* Switch to another program, and send the corresponding control code
 * over stdout.  The identity can be constructed with mtyp_mkid()
 * and hints at an optional description.
 *
 * It is assumed that the program switched to exists.
 *
 * TODO: Streams may need to claim until they release.
 *
 * Return 0 on success or else -1/errno.
 */
int mtyp_switch (MULTTY_PROG *prog) {
	MULTTY_PROGSET *progset = prog->set;
	//
	// Is this the current?  Then forget previous, but send nothing
	if (progset->current == prog) {
		progset->previous = NULL;
		return 0;
	}
	//
	// Is this the previous?  Then simply swap with current
	if (progset->previous == prog) {
		progset->previous = progset->current;
		progset->current  = prog;
		// Make the nameless switch to "previous"
		mtyp_raw (1,
			s_PSW, 1);
		return 0;
	}
	//
	// Produce the full output message (no data, so no need to flush it out)
	const char *descr = prog->descr;
	if (descr == NULL) {
		descr = "";
	}
	mtyp_raw (4,
		s_SOH, 1,
		prog->id_us, strnlen (prog->id_us, sizeof (MULTTY_PROGID)),
		descr, strlen (descr),
		s_PSW, 1);
	//
	// Normal handling pushes current (if set) back to previous
	if (progset->current != NULL) {
		progset->previous = progset->current;
	}
	progset->current = prog;
	return 0;
}


#if 0   /* OLD CODE */
/* Switch to another program within the current set, and send the
 * corresponding control code over stdout.  The identity can be
 * constructed with mtyp_mkid() and hints at an optional
 * description.
 *
 * It is assumed that the program switched to exists.
 *
 * TODO: Streams may need to claim until they release.
 *
 * Return 0 on success or else -1/errno.
 */
int mtyp_switch (MULTTY_PROGSET *progset, MULTTY_PROGID id_us) {
	//
	// The hash is probably the fastest lookup available
	MULTTY_PROG *prog = mtyp_find (progset, id_us);
	//
	// Report an error if the program was not found
	if (!prog) {
		errno = ENOENT;
		return -1;
	}
	//
	// Is this the current?  Then forget previous, but send nothing
	if (progset->current == prog) {
		progset->previous = NULL;
		return 0;
	}
	//
	// Is this the previous?  Then simply swap with current
	if (progset->previous == prog) {
		progset->previous = progset->current;
		progset->current  = prog;
		// Make the nameless switch to "previous"
		mtyp_raw (1,
			s_PSW, 1);
		return 0;
	}
	//
	// Produce the full output message (no data, so no need to flush it out)
	const char *descr = prog->descr;
	if (descr == NULL) {
		descr = "";
	}
	mtyp_raw (4,
		s_SOH, 1,
		id_us, strnlen (id_us, sizeof (MULTTY_PROGID)),
		descr, strlen (descr),
		s_PSW, 1);
	//
	// Normal handling pushes current (if set) back to previous
	if (progset->current != NULL) {
		progset->previous = progset->current;
	}
	progset->current = prog;
	return 0;
}
#endif

