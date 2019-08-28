/* mulTTY -> pre-opened standard output.
 *
 * This stream is easier and more optimal than opening "stdout" manually.
 * It does not explicitly use stream shifting.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <arpa2/multty.h>


struct multty multty_stdout = {
	.shift = 0,
	.fill  = 0,
};

