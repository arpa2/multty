/* mulTTY -> pre-opened standard input.
 *
 * This stream is easier and more optimal than opening "stdin" manually.
 * It does not expect explicit stream shifting.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


struct multty multty_stdin = {
	.shift = 0,
	.fill  = 0,
};

