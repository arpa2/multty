/* mulTTY -> internal include file for multiplexing/programs
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include "uthash.h"


/* Fill out the opaque type for a mulTTY program.
 */
struct multty_prog {
	// id_us is the identity in <=32 chars, plus optional <US>
	// note: early cut-off with <NUL> but <US> might also be in [32]
	MULTTY_PROGID id_us;
	// descr points to a varying description if <US> was added
	char *descr;
	// hash table data
	UT_hash_handle hh;
};


/* Fill out the opaque type for a mulTTY program.
 */
struct multty_progset {
	// start of the hash table is just an element
	struct multty_prog *programs;
	// the current and previous programs for this set
	struct multty_prog *current, *previous;
};

