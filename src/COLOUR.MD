# Colour Picking in preTTY

> *Picking colours well is surprisingly difficult!*

The ANSI colour table contains a cube of 6x6x6 colours, that is 6 grades of red, green and blue.
The numbers are 16 + 36xR + 6xG + b.  This is a workable space in which to search for colours.

We will choose the following setup:

  * Colour depicts a program.  We choose a brigt colour for things like `stderr` and one step
    dimmed for `stdout`.  The distance between these is usually visible, although a bit more
    difficult in blue than in red and green.  We will first iterate red and green for that
    reason.

  * Colour handling for streams.  Bold for `stderr` and one step dimmed for `stdout`, plus
    headers with inverse video and for other streams we shall pick underline and inverse
    video as needed.  By default, we only show `stderr` and `stdout` though.

  * Colours `(r,g,b)` are distanced as far as possible from the background colour `(r0,g0,b0)`,
    which can be set in environment variable `PRETTY_BGCOLOUR` as an `RRGGBB` hex number.
    When not set, black or `000000` will be assumed.  Distances will be relative to (the cube
    point nearest to) this number.  Distances are
    `sqrt ( (r-r0)^2 + (g-g0)^2 + (b-b0)^2 )`
    although nobody is ever looking for the square and so we compare the sum of squares.

  * The algorithm starts with the largest sphere of radius `sqrt (5^2 + 5^2 + 5^2)` and gradually
    reduces its size to produce the colours that fall outside it.  This approach does not take
    into account that colours should be far apart, but this is still what it does in practice.

  * There is no builtin support for colour blindness, yet, but it may be possible to combine the
    red and green axes, and effectively drop the radius of a circle instead of a sphere.

  * The resulting mechanism is as predictable as the order in which programs start sending data.
    Future versions of the program may choose to allow manual colour overrides.

  * There is a colour testing routine that may be used to see if colours are difficult for a
    colour blind person, or if a colour was manually selected.  If not, it can return `false`
    and the colour picking algorithm continues.
