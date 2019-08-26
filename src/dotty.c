/* mulTTY -> dotty.c -- Do a command, multiplex its streams.
 *
 * This runs a command that was written for plain stdin, stdout
 * and stderr streams.  It merges stdout and stderr, but does
 * this by switching between their streams.
 *
 * A very coarse approximation of "dotty -- cmd" is "cmd 2>&1"
 * which differs, because it does not produce output such that
 * it shifts between the streams.
 *
 * TODO:
 * This program is meant to grow into a fullblown mulTTY wrapper
 * around a single command.  It should at some point include a
 * stdctl channel for standard operations that pause, resume,
 * start and stop the program and, possibly, debug it remotely.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>



/* Simple redefinitions for stdout and stderr,
 * meant to be there at any time, with simple
 * wrapper macros to write text or data to them.
 *
 * TODO: Use <SI>/<SO> with possible <SOH>stderr
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


/* mulTTY escapes are needed for:
 *
 * NUL, SOH, STX, ETX, EOT, ENQ, ACK, DLE, DC1, DC2,
 * DC3, DC4, NAK, SYN, ETB, CAN, EM, FS, GS, RS, US, DEL.
 *
 * With the exception of DEL, these are expressed in the
 * bitmask below.  DEL has a funny code.
 *
 * If you insist on using Telnet as a protocol, you may
 * also have to escape 0xff, which is a prefix for a
 * remote command.  You can either use DLE escaping or
 * send a double 0xff in this case, where the former
 * would work along with other mulTTY escapes and the
 * latter would assume Telnet to be the carrier.
 */
#define MULTTY_ESCAPED ((1<<0)|(1<<1)|(1<<2)|(1<<3)| \
	(1<<4)|(1<<5)(1<<6)|(1<<16)|(1<<17)|(1<<18)| \
	(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<23)|(1<<24)| \
	(1<<25)|(1<<28)|(1<<29)|(1<<30)|(1<<31))


/* Return whether a character needs to be escaped.
 * See the definition of MULTTY_ESCAPED before, and
 * add DEL.
 */
bool multty_escaped (char c) {
	if (c > 0x20) {
		return (c == 0x7f);
	} else {
		return ((MULTTY_ESCAPED & (1 << c)) != 0);
	}
}


/* Add escape codes to a mulTTY stream, meaning that all
 * control codes used in mulTTY are prefixed with DLE and
 * XORed with 0x40.  In the most extreme case, the buffer
 * grows to double the size, which must be possible.  The
 * new buffer length is returned.
 */
size_t multty_escape (char *buf, size_t buflen) {
	int escctr = 0;
	int i;
	for (i=0; i<buflen; i++) {
		if (multty_escaped (buf [i])) {
			escctr++;
		}
	}
	buflen += escctr;
	i = buflen;
	while (escctr > 0) {
		if (multty_escaped (buf [i-escctr])) {
			buf [--i] = buf [i-escctr] ^ 0x40;
			buf [--i] = DLE;
			escctr--;
		} else {
			buf [--i] = buf [i-escctr];
		}
	}
	return buflen;
}


/* Multiplex child stdout and stderr onto newout.
 * We only use stderr to report our own errors.
 * Returns only non-zero on success.
 */
int multiplex (int subout, int suberr) {
	int ok = 1;
	int level = 1;
	int maxfd = (subout > suberr) ? subout : suberr;
	fd_set both;
	int more = 1;
	char buf [3 + MAXBUF + MAXBUF];
	ssize_t buflen;
	ssize_t gotlen;
	while ((subout >= 0) || (suberr >= 0)) {
		FD_ZERO (&both);
		FD_SET (subout, &both);
		FD_SET (suberr, &both);
		if (select (maxfd+1, &both, NULL, NULL, NULL) < 0) {
			txterr ("Failed to select stdout/stderr");
			exit (1);
		}
		buflen = 0;
		if (FD_ISSET (subout, &both)) {
			if (level != 1) {
				memcpy (buf, SI_s SO_s, buflen = 2);
				level = 1;
			}
			gotlen = read (subout, buf+buflen, MAXBUF);
			if (gotlen < 0) {
				txterr ("Error reading from child stdout\n");
				ok = 0;
				subout = -1;
			} else if (gotlen == 0) {
				subout = -1;
			} else {
				gotlen = multty_escape (buf+buflen, gotlen);
				if (bufout (buf, buflen + gotlen) != buflen + gotlen) {
					txterr ("Unable to pass child stdout\n");
					ok = 0;
					subout = -1;
				}
			}
		}
		if (FD_ISSET (suberr, &both)) {
#ifdef USE_RELATIVE_STDERR
			if (level == 1) {
				memcpy (buf, SO_s, buflen = 1);
				level++;
			}
#endif
			if (level != 2) {
				memcpy (buf, SI_s SO_s SO_s, buflen = 3);
				level = 2;
			}
			gotlen = read (suberr, buf+buflen, MAXBUF);
			if (gotlen < 0) {
				txterr ("Error reading from child stderr\n");
				ok = 0;
				suberr = -1;
			} else if (gotlen == 0) {
				suberr = -1;
			} else {
				gotlen = multty_escape (buf+buflen, gotlen);
				if (bufout (buf, buflen + gotlen) != buflen + gotlen) {
					txterr ("Unable to pass child stderr\n");
					ok = 0;
					suberr = -1;
				}
			}
		}
	}
	return ok;
}


/* The main routine runs a child process with
 * reorganised stdout and stderr handles.  It
 * passes stdin without change.
 */
int main (int argc, char *argv []) {
	int ok = 1;
	//
	// Parse commandline arguments
	int argi = 2;
	if ((argc < 2) || strcmp (argv [1], "--")) {
		txterr ("Usage: dotty -- cmd [args...]\n");
		exit (1);
	}
	//
	// Construct redirection pipes
	int pipout [2];
	int piperr [2];
	int tmpout = dup (1);
	int tmperr = dup (2);
	if ((tmpout < 0) || (tmperr < 0)) {
		txterr ("Failed to create replicas for stdout/stderr\n");
		exit (1);
	}
	newout = tmpout;
	newerr = tmperr;
	if (pipe (pipout) || (dup2 (pipout [1], 1) != 1)) {
		txterr ("Failed to wrap stdout file\n");
		exit (1);
	}
	if (pipe (piperr) || (dup2 (piperr [1], 2) != 2)) {
		txterr ("Failed to wrap stderr file\n");
		exit (1);
	}
	close (pipout [1]);
	close (piperr [1]);
	//
	// Start a child process with the right piping
	pid_t child = fork ();
	switch (child) {
	case -1:
		/* Failure */
		txterr ("Failed to fork a child process\n");
		exit (1);
	case 0:
		/* Child */
		close (pipout [0]);
		close (piperr [0]);
		close (newout);
		close (newerr);
		execvp (argv [argi], argv+argi);
		txterr ("Child process failed to run the command\n");
		exit (1);
	default:
		/* Parent */
		close (0);
		close (1);
		close (2);
	}
	//
	// Multiplex the messages sent to stdout and stderr
	//  - initial sending on stdout
	//  - absolute switch to stdout with SI,SO
	//  - absolute switch to stderr with SI,SO,SO
	//  - relative switch to stderr with       SO
	//
	ok = ok && multiplex (pipout [0], piperr [0]);
	// Wait for the child to finish, then wrapup
	int status;
	waitpid (child, &status, 0);
	exit (ok ? 0 : 1);
}

