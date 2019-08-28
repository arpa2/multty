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

#include <arpa2/multty.h>



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


#define BUFLEN 1024


/* Combine the child's stdout and stderr streams on newout.
 * We only use stderr to report our own errors.
 * Returns only non-zero on success.
 */
int combine_streams (int subout, int suberr, MULTTY *stream_out, MULTTY *stream_err) {
	int ok = 1;
	int level = 1;
	int maxfd = (subout > suberr) ? subout : suberr;
	fd_set both;
	int more = 1;
	char buf [BUFLEN];
	ssize_t gotlen;
	while ((subout >= 0) || (suberr >= 0)) {
		FD_ZERO (&both);
		FD_SET (subout, &both);
		FD_SET (suberr, &both);
		if (select (maxfd+1, &both, NULL, NULL, NULL) < 0) {
			txterr ("Failed to select stdout/stderr");
			exit (1);
		}
		if (FD_ISSET (subout, &both)) {
			gotlen = read (subout, buf, BUFLEN);
			if (gotlen < 0) {
				txterr ("Error reading from child stdout\n");
				ok = 0;
				subout = -1;
			} else if (gotlen == 0) {
				subout = -1;
			} else {
				if (!mtyputstrbuf (stream_out, buf, gotlen)) {
					txterr ("Unable to pass child stdout\n");
					ok = 0;
					subout = -1;
				}
			}
		}
		if (FD_ISSET (suberr, &both)) {
			gotlen = read (suberr, buf, BUFLEN);
			if (gotlen < 0) {
				txterr ("Error reading from child stderr\n");
				ok = 0;
				suberr = -1;
			} else if (gotlen == 0) {
				suberr = -1;
			} else {
				if (!mtyputstrbuf (stream_err, buf, gotlen)) {
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
	MULTTY *stream_out = MULTTY_STDOUT;
	MULTTY *stream_err = MULTTY_STDERR;
	int opt;
	bool help = false;
	bool error = false;
	while ((opt = getopt (argc, argv, "hi:o:e:")) != -1) {
		switch (opt) {
		case 'i':
			txterr ("Option -i not yet supported (no input parsing implemented yet)\n");
			break;
		case 'o':
			stream_out = mtyopen (optarg, "w");
			break;
		case 'e':
			stream_err = mtyopen (optarg, "w");
			break;
		default:
			error = true;
			/* continue into 'h' */
		case 'h':
			help = true;
			break;
		}
	}
	int argi = optind;
	if (argc - argi < 1) {
		error = error || !help;
	}
	if (help || error) {
		txterr ("Usage: dotty [-i|-o|-e NAME]... -- cmd [args...]\n");
		exit (error ? 1 : 0);
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
		dup2 (newout, 1);
		dup2 (newerr, 2);
		close (newout);
		close (newerr);
		newout = 1;
		newerr = 2;
	}
	//
	// Multiplex the messages sent to stdout and stderr
	// Process optional stream names given with -o and -e
	ok = ok && combine_streams (pipout [0], piperr [0], stream_out, stream_err);
	// Wait for the child to finish, then wrapup
	int status;
	waitpid (child, &status, 0);
	exit (ok ? 0 : 1);
}

