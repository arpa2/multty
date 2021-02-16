/* Colour picker, with maximum distance from the background.
 *
 * Choose MULTTY_BACKGROUND=<color_16_231> as a number but we can be
 * kind enough to value primary 0..7 and bright 8..15 as well as grey
 * scales 232..255.
 *
 * We generate the squares of possible distances +5, -5, ..., -1 from
 * the background colour over each of the 3 colour dimensions and sort
 * to find the highest maximum sum of squares (which is the same order
 * as the maximum radius distance).
 *
 * The distances found are for bold; normal script is one less in all
 * differences.  Bold distances are always at least two steps away.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>



/* Colours in 3D notation, each dimension a value 0..5.
 */
typedef int col3d_t [3];


/* Internal index type for colour (square) distance tables.
*/
typedef int idx3d_t [3];


/* Iterator structure for colours.  Initialised from a fixed
 * backgroud colour with colour_first() and both this call and
 * subsequent colour_next() will set a foreground colour.
 * 
 * Print "\x1b[38;5;<ansi_fg>m"... for the foreground;
 * print "\x1b[48;5;<ansi_bg>m"... for the background;
 * print "\x1b[0m" to reset colours.
 */
typedef struct colour_iterator {
	//
	// Background colour, fixed
	col3d_t bg;
	//
	// Foreground colour, iterated
	col3d_t fg;
	//
	// Current indexes, iterated
	idx3d_t idx;
	//
	// Current squared distance, iterated
	int sqdist;
	//
	// ANSI colour code for the foreground
	int ansi_fg;
	//
	// ANSI colour code for the background
	int ansi_bg;
} colour_iterator_s, *colour_iterator_t;



/* Indexable colour distances and squared distances.
 */
static const int   dist [] = {  5, -5,  4, -4, 3, -3, 2, -2, 0 };
static const int sqdist [] = { 25, 25, 16, 16, 9,  9, 4,  4, 0 };


/* Test if a colour lives up to certain rules, which may in future
 * do fancy things like testing for colour blind support or skipping
 * manually declared colours for certaint targets.
 *
 * When accepted, this function returns true and then it has set
 * the ansi_fg number for the colour.
 */
//TODO// Make a fallable ANSI color deriver (possibly relative to a background)
bool test_colour (colour_iterator_t colit) {
	colit->ansi_fg = 16 + colit->fg [0] * 36 + colit->fg [1] * 6 + colit->fg [2];
	return true;
}


/* Iterate to the next RGB colour.  Consider the brightness differences
 * to be R > G > B and ignore colour blindness for now.  The order of
 * colour differences when all else is equal is R, G, RG, B, RB, GB, RGB.
 *
 * The algorithm tests whether r, g or b should be incremented by finding
 * the one with the biggest sum-of-squares distance.  The value that is
 * incremented will backtrack on the preceding r, g, b indexes to achieve
 * the highest distance that is not more than the current distance (but it
 * may be the same, because a change has already been made).
 */
bool next_colour (colour_iterator_t colit) {
	//
	// Collect distances for 3 options: incrementing R, G and B
	idx3d_t best_idx;
	int best_sq = 0;
	int best_dim = -1;
	//
	// Try to increment each of the 3 colour dimensions
	for (int trydim = 0; trydim < 3; trydim++) {
		//
		// Look for a next dimension that is a workable color
		int tryidx = colit->idx [trydim];
		int trycoldim;
		do {
			if (dist [tryidx] == 0) {
				break;
			}
			tryidx++;
			trycoldim = colit->fg [trydim] + dist [tryidx];
		} while ((0 <= trycoldim) && (trycoldim <= 5));
printf ("Trying dimension %d with index #%d, distance %d\n", trydim, tryidx, dist [tryidx]);
		//
		// Backtrack on previous dimensions for closest best_sq:
		//  1) Not above colit->sqdist, as that would represent older data
		//  2) The highest best_sq <= colit->sqdist that can be found
		//  3) For that highest best_sq, aim for early dimension changes
		int b = (trydim == 2) ? tryidx : colit->idx [2];
		int b_sq = sqdist [b];
		int g_min = (trydim == 1) ? tryidx : ((trydim > 1) ? 0 : colit->idx [1]);
		int g_max = (trydim == 1) ? tryidx : ((trydim > 1) ? 8 : colit->idx [1]);
		for (int g = g_min; g <= g_max; g++) {
			trycoldim = colit->bg [1] + dist [g];
			if ((trycoldim < 0) || (5 < trycoldim)) {
				continue;
			}
			int gb_sq = sqdist [g] + b_sq;
			int r_min = (trydim == 0) ? tryidx : 0;
			int r_max = (trydim == 0) ? tryidx : 8;
			for (int r = r_min; r <= r_max; r++) {
				trycoldim = colit->bg [0] + dist [r];
				if ((trycoldim < 0) || (5 < trycoldim)) {
					continue;
				}
				int rgb_sq = sqdist [r] + gb_sq;
printf ("RGB = #%d,#%d,#%d = %2d,%2d,%2d -- sqdist = %d\n", r, g, b, dist [r], dist [g], dist [b], rgb_sq);
				//
				// Is this (1) suitable and (2) the best so far?
				if ((rgb_sq <= colit->sqdist) && (rgb_sq > best_sq) /*TODO: && (test_colour (r, g, b)) */ ) {
printf (" --> best! sqdist = %d, dim = %d\n", rgb_sq, trydim);
					best_sq      = rgb_sq;
					best_dim     = trydim;
					best_idx [0] = r;
					best_idx [1] = g;
					best_idx [2] = b;
				}
			}
		}
	}
	//
	// See if a new distance was found at all
	if (best_dim < 0) {
		return false;
	}
	//
	// Return the best example as the new colour
	memcpy (colit->idx, best_idx, sizeof (colit->idx));
	colit->sqdist = best_sq;
	for (int dim = 0; dim < 3; dim++) {
		colit->fg [dim] = colit->bg [dim] + dist [best_idx [dim]];
	}
test_colour (colit);
printf ("Success with ansi_fg=%d on ansi_bg=%d\n", colit->ansi_fg, colit->ansi_bg);
	return true;
}


// INITIAL IMPLEMENTATION
#if 0
bool next_colour (void) {
printf ("next_colour after %d%d%d\n", r, g, b);
	//
	// Find the current distance -- we shall not go higher than that
	int cursq = sqdist [r] + sqdist [g] + sqdist [b];
	//
	// See if a "later" combination has the same sqdist
	int r0 = r, g0 = g, b0 = b;
reloop:
	if (sqdist [r+1] == sqdist [r]) {
		r++;
		if (test_colour ()) return true;
	}
	if (sqdist [g+1] == sqdist [g]) {
		r--;
		g++;
		if (test_colour ()) return true;
		goto reloop;
	}
	if (sqdist [b+1] == sqdist [b]) {
		r--;
		g--;
		b++;
		if (test_colour ()) return true;
		goto reloop;
	}
	//
	// Find the "first" combination with the lowest sqdist
	r = r0; g = g0; b = b0;
	while (sqdist [r] != 0) {
printf ("Comparing r=%d g=%d b=%d for %d,%d,%d\n", r, g, b, dist [r+1], dist [g+1], dist [b+1]);
		if (sqdist [r+1] >= sqdist [g+1]) {
			if (sqdist [r+1] >= sqdist [b+1]) {
				/* r >= g, r >= b */
				r++;
				if (test_colour ()) return true;
			} else {
				/* b > r, r >= g */
				b++;
				if (test_colour ()) return true;
			}
		} else {
			if (sqdist [g+1] >= sqdist [b+1]) {
				/* g >= b, g > r */
				g++;
				if (test_colour ()) return true;
			} else {
				/* b > g, g > r */
				b++;
				if (test_colour ()) return true;
			}
		}
	}
	//
	// Only 0 distances remaining; we're done
	return false;
}
#endif


/* Setup the first foreground colour and choose it as far away as possible
 * from the background colour.  Setup the colour iterator, including the
 * ansi_bg value for the background, and ansi_fg for the foreground.
 */
bool first_colour (const col3d_t bg, colour_iterator_t colit) {
	bool ok = true;
	memcpy (colit->bg, bg, sizeof (colit->bg));
	int sqd = 0;
	for (int dim = 0; dim < 3; dim++) {
		int fg = (bg [dim] < 3) ? 5 : 0;
		int idx = 0;
		while (dist [idx] != fg - bg [dim]) {
			if (dist [idx] == 0) {
				fprintf (stderr, "Background colour range not 0..5: %d,%d,%d\n", bg [0], bg [1], bg [2]);
				ok = false;
				break;
			}
			idx++;
		}
		colit->idx [dim] = idx;
		colit->fg [dim] = fg;
		sqd += sqdist [idx];
	}
	colit->sqdist = sqd;
	colit->ansi_bg = 16 + 36 * bg [0] + 6 * bg [1] + bg [2];
	while (ok && !test_colour (colit)) {
		ok = ok && next_colour (colit);
	}
printf ("Initial with ansi_fg=%d on ansi_bg=%d\n", colit->ansi_fg, colit->ansi_bg);
	return ok;
}


// INITIAL IMPLEMENTATION
#if 0
bool first_colour (void) {
	while (!test_colour ()) {
		if (!next_colour ()) {
			return false;
		}
	}
	return true;
}
#endif


int main (int argc, char *argv []) {
	col3d_t bgcol = { 0, 0, 0 };
	colour_iterator_s colits;
	if (first_colour (bgcol, &colits)) do {
		printf ("\x1b[38;5;%dm\x1b[48;5;%dmRGB = %d%d%d on %d%d%d\x1b[0m\n",
				colits.ansi_fg, colits.ansi_bg,
				colits.fg [0], colits.fg [1], colits.fg [2],
				colits.bg [0], colits.bg [1], colits.bg [2]);
	} while (next_colour (&colits));
}
