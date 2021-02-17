# Colour Picking for preTTY

> *How to generate a list of colours for the display of multiplexers?*

We use colours to depict programs and distinguish streams with markup;
normally we show `stdout` in a normal tone and `stderr` brighter, unless
lines contain the word `DEBUG` in which case they are dimmed.  We
use reverse video for headers.  Markdown rendering makes its separate
pick from the primitives, and in general the markup style may get
mixed.  Ideally, colours are not.

Here are the counters, fastest one first:

**Dimensions 1-6** select BGR dimensions by the bit positions, so the
active dimensions willl be R, G, RG, RB, GB.  Note that backgrounds are
often black or white, or another mixture of three dimensions.  These
bits select which dimensions can be non-zero, others are zero so it
ought to stick out everywhere.

**Levels 0-3** for choosing normal levels 5, 3, 4, 2.

**Sublevels 0-0 or 0-4** for unicolour dimensions 1-3 or bicolour
dimensions 4-6.  Sublevel 0 is the same value as the level, and if
sublevels 1-4 are present they select from the other values in the
order in which they occur as levels.

This leaves a total of 3x5x1 + 3x5x5 = 90 colours with a reasonably spread
over the picking range of the ANSI 6x6x6 cube.

On a white background, it seems prudent to invert the selected colour but
that would not work for all colours and may confusing remote discussions.
It is probably better to shift the range a little from 2-5 onto 1-4.

The colours would be recycled when they are all gone.

Rather than picking numbers in a given order, it would also be possible
to use a random number to scatter the range.  In general, this should
be seen as a series of colours to pick from.

The range can simply be generated, rather than computed.

