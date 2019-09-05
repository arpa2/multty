/* mulTTY -> dispatch of streams within one program
 *
 * This gives simplified access to a single-program
 * stream dispatcher.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <assert.h>

#include <arpa2/multty.h>


#ifndef MULTTY_MIXED

/* Read the input stream and dispatch its data over
 * streams which each receive their own chunks.
 * Dispatched chunks end when <SOH> is encountered,
 * as that may introduce a switch of a [program or]
 * stream.  As soon as a program switch is found,
 * the dispatcher also ends.
 *
 * This is a simple wrapper around mtydispatch that
 * rejects input with program multiplexing commands.
 *
 * If the input stream is blocking, then this call
 * is also blocking.
 */
void mtydispatch_streams (MULTTY *current) {
	assert (mtydispatch_internal (current));
}

#endif

