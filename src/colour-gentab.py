#!/usr/bin/env python
#
# Generate a colour table in C.  See COLOUR.MD for how.
#
# From: Rick van Rein <rick@openfortress.nl>


# Note: Yellow = Red + Green, Magenta = Red + Blue, Cyan = Green + Blue
colourmap = {
	'R': 36,
	'G': 6,
	'B': 1,
}

def colourdata ():
	levels = [ 5, 3, 4, 2 ]
	for (levi,lev) in enumerate (levels):
		for sublevi in range (len (levels)):
			sublev = levels [ (levi+sublevi) % len (levels) ]
			if sublev != lev:
				dimensions = [ 'RG', 'RB', 'GB' ]
			else:
				dimensions = [ 'R', 'G', 'B' ]
			for dim in dimensions:
				yield (dim,lev,sublev)


print ('/* Generated colour table.  Edits may be lost.\n *\n * To pick a colour, use colour(N)\n *\n * From: Rick van Rein <rick@openfortress.nl>\n */\n')

print ('\n#ifndef colour_table_type\n#define colour_table_type int\n#endif\n')
print ('colour_table_type colour_table [] = {')
for (dim,lev,sublev) in colourdata ():
	# print ('Dimension %s, level %d, sublevel %d' % (dim,lev,sublev))
	ansi = 16 
	compo = []
	for (d,l) in zip (dim, [lev,sublev]):
		ansi += colourmap [d] * l
		compo.append ('%s=%d' % (d,l))
	compo = ','.join (compo)
	print ('\t%3d,\t// \x1b[38;5;%dmANSI code %3d for %s\x1b[0m' % (ansi,ansi,ansi,compo))
print ('};')

print ('\n#define colour_table_size (sizeof (colour_table) / sizeof (colour_table_type))\n')
print ('#define colour(N) (colour_table [ (N) % colour_table_size ])\n')
