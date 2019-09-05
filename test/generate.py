#!/usr/bin/env python3
#
# Generate simulated output over streams (for test purposes).
# Input lines use "stream=data" or, for the default stream, "data".
#
# This assumes the atomic model, which returns to standard after
# every change to a stream, as part of the same write.
#
# From: Rick van Rein <rick@openfortress.nl>


import sys
from sys import argv, stdin, stdout, stderr, exit


escapable = '\x01\x0e\x0f\x10\x11\x12\x13\x14\x19\x1c\x1d\x1e\x1f\x00\xff'


def escape (s):
	return ''.join ([
		( c if not c in escapable else '\x10' + chr (ord (c) ^ 0x40) )
		for c in s
	])


for arg in stdin.readlines ():
	if '=' in arg:
		(stream,value) = arg.split ('=')
		escval = escape (value)
		stdout.write ('\x01' + stream + '\x0e' + escval + '\x0e')
	else:
		escval = escape (arg)
		stdout.write (escval)

