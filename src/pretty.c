/* MulTTY -> pretty.c -- Present Pretty TTY Pictures
 *
 * Using pretty, you can assign colours to multiplexes
 * of programs, and of steams within programs.  This is
 * especially useful near the useful, just before ASCII
 * streams are being rendered, such as within a terminal
 * emulator or MOSH server (MOSH really is just a mobile
 * terminal, not a remote shell).
 *
 * TODO: No current support for a SASL client yet.
 *
 * Though pretty seems simple, it does a good job at
 * knowing which programs or streams are printed in
 * what colours or brightnesses.  It consistently
 * applies those settings whenever multiplexing
 * changes its focus, to hide this fact from the user
 * and make sure that the colouring makes sense.
 *
 * The minimum useful setup is dotty or shotty to run
 * programs and pretty to present their output with some
 * graphical splendour.
 *
 * As with all of MulTTY, this program only works with
 * specific ASCII codes and passes all others as-is.
 * Your UTF-8 interpretation, internationalisation and
 * whatever else render-related is no issue to pretty.
 * TODO: Should pretty grow into a terminal emulator?
 *
 * A known bug is that the output is often as pretty and
 * as stylish as an Easter egg.  This does not interfere
 * with its practical usefulness, however, so the esoteric
 * debate on colour picking is delayed until St. Juttemis.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



// Options to this program:  TODO:IMPLEMENT
//  -b,--bold-errors    highlights stderr in bold print
//     --stream-colours requests a colour per stream
//  -p,--program-name   includes the program name per line
//  -s,--stream-name    includes the stream name per line
//  -l,--line-indent    recovers the indent for lines
//  -t,--multty TRANS   connects to a MulTTY transport



/* Simple redefinitions for stdout and stderr,
 * meant to be there at any time, with simple
 * wrapper macros to write text or data to them.
 *
 * TODO: Use SI,SO[,SO] for this too.
 */
int newout = 1;
int newerr = 2;

#define bufout(s,l) write(newout, (s), (l))
#define buferr(s,l) write(newerr, (s), (l))

#define txtout(s) bufout((s), strlen ((s)))
#define txterr(s) buferr((s), strlen ((s)))

#define SO  0x0e
#define SI  0x0f
#define DLE 0x10
#define DEL 0x7f
#define SOH 0x01
#define ETB 0x17
#define EM  0x19
#define DC1 0x11
#define DC2 0x12
#define DC3 0x13
#define DC4 0x14

#define SO_s  "\x0e"
#define SI_s  "\x0f"
#define DLE_s "\x10"
#define DEL_s "\x7f"
#define SOH_s "\x01"


#define MAXBUF 4096


struct multty_program {
	struct multty_program *next, *prev, *up, *down;
	struct multty_stream *cur_stream;
	char *name;
};

struct multty_stream {
	struct multty_stream *next;
	char *name;
	bool last_dle;
	bool shift_in;
};

struct split_state {
	int cur_level;
	bool sohclx;
	char soh [65];
	size_t sohlen;
	struct multty_program *cur_program;
};


/* Remove DLE escape codes from the buffer of the given
 * buflen.  Return the new buffer length, which is never
 * higher.  When the input buflen is not empty, then the
 * output buflen may in an exceptional case be empty.
 *
 * The last character may be DLE, in which case that must
 * be remembered until the next character in the stream.
 * This is why a state boolean pointer is needed.
 */
size_t multty_unescape (char *buf, size_t buflen, bool *last_dle) {
	size_t inlen = 0;
	size_t otlen = 0;
	while (inlen < buflen) {
		if (*last_dle) {
			buf [otlen++] = buf [inlen++] ^ 0x40;
		} else if (buf [inlen] == DLE) {
			*last_dle = true;
			inlen++;
		} else {
			buf [otlen++] = buf [inlen++];
		}
	}
	return otlen;
}


/* Send a buffer, possibly with escaping, to the output
 * after adding colours and other pretty markers.
 * 
 */
void multty_output (struct multty_program *prog,
			struct multty_stream *str,
			char *buf, size_t buflen) {
	//
	// Remove data-link escaping from the buffer, retain its marker
	size_t actlen = multty_unescape (buf, buflen, &str->last_dle);
	if (actlen == 0) {
		return;
	}
	//
	// Send out the buffer, which may be binary, but is now freed from
	// data-link escapes and multiplexing control codes.
	//
	// TODO: Add colour, boldness and so on
	// TODO: Add stream splitting over lines with indent preservation
	bufout (buf, actlen);
}


/* Split the input into programs and streams.  This is
 * the pivotal point of interpreting the MulTTY traffic
 * and using the special ASCII codes to find the normal
 * ones.
 *
 * While interpreting the buffer, calls are made to a
 * handler function that is provided with program and
 * stream descriptors.  The content has not had its
 * escapes removed yet, but it can handle that itself,
 * as the buffer provided may be overwritten.
 */
void multty_split_iter (char *buf, size_t buflen, struct split_state *st) {
	char *plainbuf = NULL;
	size_t plainlen = 0;
	while (buflen > 0) {
		size_t prelen = buflen;
		bool special = true;
		if (buf [buflen] == SOH) {
			//
			// Flag that we are storing name information
			st->sohclx = true;
		} else if ((st->sohclx) && (buf [buflen] >= 0x21) && (buf [buflen] <= 0x7e)) {
			//
			// Actually store name information for as long as the name is
			int sohlen = strlen (st->soh);
			if (sohlen < 64) {
				st->soh [sohlen++] = buf [buflen];
				st->soh [sohlen  ] = '\0';
			}
		} else if ((buf [buflen] == SI) || (buf [buflen] == SO)) {
			//
			// Shift In/Out, using to the red/black track of your ink ribbon
			st->cur_level = buf [buflen];
			if (st->sohlen > 0) {
				...TODO...lookup:or:insert:stream...st->cur_program...
			} else {
				st->cur_program->cur_stream = st->cur_program->std_stream;
			}
		} else if (buf [buflen] == ETB) {
			//
			// Stream termination, using the current or named stream, ETB
			if (st->sohlen > 0) {
				...TODO...find:stream...st->cur_program...into...st->cur_stream...
			}
			if (st->cur_stream == st->cur_program->std_stream) {
				st->cur_program->std_stream = NULL;
			}
			st->cur_stream = ...TODO...
			...TODO...remove:stream...st->cur_program...
			free (st->cur_stream);
			st->cur_stream = st->cur_program->cur_stream;
		} else if ((buf [buflen] >= DC1) && (buf [buflen] <= DC4)) {
			//
			//TODO// Handle program activation:  DC1..DC4
			...TODO...make-initial-step...
			if (st->sohlen > 0) {
				...TODO...find:program...
			}
			...TODO...make-last-step...
		} else if (buf [buflen] == EM) {
			//
			//TODO// Handle program termination: EM
			prog = ...TODO...
			st->cur_program = ...TODO...next...
			st->cur_stream = st->cur_program->cur_stream;  ...TODO...NULL...
			...TODO...SAFE-ITER...free_stream...prog...
			free (prog);
		} else {
			//
			// Plain codes, which we want to record for later
			special = false;
			if (plainbuf == NULL) {
				//
				// We got plain ASCII, which is new, so start memorising it
				plainbuf = buf;
				plainlen = buflen;
			}
		}
		if (special) {
			//
			// Found a special code, consider it having ended plain ASCII
			if (plainbuf != NULL) {
				multty_output (st->cur_program, st->cur_stream,
					plainbuf, buflen-plainlen);
				plainbuf = NULL;
			}
		}
		buf++;
		buflen--;
		continue;
	}
	//
	// Buffer ran out of chars -- did we collect text worth processing?
	if (plainbuf != NULL) {
		multty_output (st->cur_program, st->cur_stream,
			plainbuf, buflen-plainlen);
		plainbuf = NULL;
	}
}


/* Read MulTTY input as it arrives, and process it by demultiplexing
 * it into programs and streams, then removing data-link escaping and
 * finally rendering it, with colouring and indentation and so on,
 * based on the program and stream information gotten from removing
 * the MulTTY descriptive information.
 *
 * The function only returns non-zero on success.
 */
bool multty_process (int mt) {
	ssize_t rdlen;
	struct split_state splitst;
	char mtbuf [MAXBUF];
	memset (&splitst, 0, sizeof (splitst));
	while (rdlen = read (mt, mtbuf, sizeof (mtbuf)), rdlen > 0) {
		multty_split_iter (mtbuf, rdlen, &splitst);
	}
	if (rdlen < 0) {
		txterr ("Error reading from MulTTY\n");
		return false;
	}
	return true;
}


/* Open a connection to the MulTTY transport, split data into the
 * multiplexed programs and streams, print output with colour and
 * formatting to informally show these multiplexing aspects that
 * originated remotely and that were possibly updated in transit.
 */
int main (int argc, char *argv []) {
	int ok = 1;
	//
	// Parse the command line arguments
	if ((argc < 3) || (strcmp (argv [1], "-m") && strcmp (argv [1], "--multty"))) {
		txterr ("Usage: pretty -m|--multty /dev/pts/N\n");
		exit (1);
	}
	//
	// Connect to the MulTTY transport
	int mt = open (argv [2], O_RDWR);
	if (mt < 0) {
		txterr ("Failed to open MulTTY transport\n");
		exit (1);
	}
	//
	// Listen for input, split into multiplexes, unescape, print
	ok = ok && multty_process (mt);
	//
	// Close the server socket
	close (mt);
	mt = -1;
	//
	// Close down and report success or failure
	exit (ok ? 0 : 1);
}

