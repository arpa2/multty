#!/usr/bin/env python3
#
# Read test input and produce readable information.
# Compare with known-good output when given.
#
# Output lines use "stream=data" or, for the default stream, "data".
#
# TODO: Output correct, verify is not
#
# From: Rick van Rein <rick@openfortress.nl>


from sys import argv, stdin, stdout, stderr, exit


if not 1 <= len (argv) <= 2:
	stderr.write ('Usage: %s [compare]\n' % argv [0])
	exit (1);

data = stdin.read ()

cur = None
first = True
errors = 0
all = ''
for sohseq in data.split ('\x01'):
	soseqs = sohseq.split ('\x0e')
	if not first:
		cur = soseqs [0]
		soseqs = soseqs [1:]
	first = False
	for soseq in soseqs:
		if cur is not None:
			vfyline = '%s=%s' % (cur,soseq)
			cur = None
		else:
			vfyline = soseq
		stdout.write (vfyline)
		all += vfyline
if len (argv) >= 2:
	verify = open (argv [1]).read ()
	if verify != all:
		stderr.write ('Output does not match verification input\n')
		exit (1)
