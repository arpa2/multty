/* mulTTY -> pre-opened standard error output.
 *
 * This stream is easier but no more optimal than opening "stderr" manually.
 * Being a different stream from "stdout", it does use stream shifting.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


struct multty multty_stderr = {
	.shift = 8,
	.fill  = 8,
	.buf = { c_SOH, 's', 't', 'd', 'e', 'r', 'r', c_SO },
};

