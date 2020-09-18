/* display.c -- Library calls for TeleType display over LoRaWAN
 *
 * LoRaWAN is a system of wireless connections with a low duty cycle
 * and intended for small frames.  Running a TTY over LoRaWAN could
 * work, provided that it is a tuned-down service.  Borrowing ideas
 * from MOSH, this service can feed to very simple terminals that
 * download to a character-based display and upload key strokes
 * and customary extensions such as mouse, resize and scrolling.
 *
 * LoRaWAN messages are signed and encrypted with AES-128 to allow
 * them to share radio bandwidth.  They are sent with a "chirp"
 * signal, allowing them to travel in the noise levels of radio,
 * and using one of a variety of orthogonal modes and timings.
 * They can co-exist with others, may be proxied and then relayed
 * over an Internet proxy, but they may also connect locally.
 *
 * The LoRaWANtty service offers a rectangular area of characters,
 * each of which can have a foreground and background colour and a
 * few attributes.  Colours are taken from a pallette of 2, 16 or
 * 256 colours.  A terminal emulator layer can draw in this area,
 * or direct rendering could be used.  Other aspects of mulTTY
 * may additionally prove interesting to mix in.
 *
 * From: Rick van Rein (PA1RVR) <rick@openfortress.nl>
 */


#include <assert.h>

#include <unistd.h>

// #include <arpa2/except.h>
	#include <stdio.h>
	#define log_debug(...)
	// #define log_debug printf

#include "loratype.h"



/* Create a new display, with the given width, height and highest color number.
 * Take note of the supported attribute bits.
 *
 * The foreground colour is initially set to the last colour code.  Characters
 * are set to an arbitrary code point, but they are not DIRTY and so never sent.
 * Terminals may copy these assumptions.
 *
 * Returns a value on success, or NULL on failure.
 * After succes, use loratype_destroy() for cleanup.
 */
loratype_display *loratype_create (ltc_line lines, ltc_colm columns, ltc_colr last_colr, ltc_attr attr_support) {
	//
	// Check colour and attribute options
	if ((last_colr != 1) && (last_colr != 15) && (last_colr != 255)) {
		return NULL;
	}
	if ((attr_support & 0xc0) != 0x00) {
		return NULL;
	}
	//
	// Create the combination of loratype_display and cells
	loratype_display *disp = calloc (sizeof (loratype_display) + lines * columns * sizeof (loratype_cell), 1);
	if (disp == NULL) {
		return NULL;
	}
	disp->cells = (struct loratype_cell *) &disp [1];
	//
	// Initialise variables inasfar as they are non-zero
	disp->height = lines;
	disp->width = columns;
	disp->numcells =
	disp->scrollend = lines * columns;
	disp->attr_support = attr_support;
	disp->last_colr =
	disp->cur_fgcol =
	disp->disp_ack_fgcol =
	disp->disp_snt_fgcol = last_colr;
	//
	// Return the successfully constructed display
	return disp;
}


/* Destroy a display, as created by loratype_create().
 *
 * This function does not fail.
 */
void loratype_destroy (loratype_display *disp) {
	free (disp);
}


/* Change the foreground colour; future updates will use this value.
 *
 * This function returns true when the colour is supported.  In case
 * of range error, the foreground is the highest supported colour code.
 */
bool loratype_setfgcol (struct loratype_display *disp, ltc_colr new_fgcol) {
	ltc_colr colr = disp->last_colr;
	if (new_fgcol < disp->last_colr) {
		colr = new_fgcol;
	}
	disp->cur_fgcol = colr;
	return (colr == new_fgcol);
}


/* Change the background colour; future updates will use this value.
 *
 * This function returns true when the colour is supported.  In case
 * of range error, the foreground is the lowest supported colour code.
 */
bool loratype_setbgcol (struct loratype_display *disp, ltc_colr new_bgcol) {
	ltc_colr colr = new_bgcol;
	if (colr > disp->last_colr) {
		colr = 0;
	}
	disp->cur_bgcol = colr;
	return (colr == new_bgcol);
}


/* Set attribute flags; multiple may be set, none is reset.
 *
 * This function returns true when all attributes are supported.
 * Otherwise, it has made a best effort (not further specified
 * and subject to change between releases).
 */
bool loratype_setattrs (struct loratype_display *disp, ltc_attr set_attrs) {
	ltc_attr attrs = disp->attr_support & set_attrs;
	disp->cur_attrs |= attrs;
	return (attrs == set_attrs);
}


/* Clear attribute flags; multiple may be cleared by _setting_
 * their bit in clr_attrs; none is set.
 *
 * This function returns true when all attributes are supported.
 * Otherwise, it has made a best effort (not further specified
 * and subject to change between releases).
 */
bool loratype_clrattrs (struct loratype_display *disp, ltc_attr clr_attrs) {
	ltc_attr attrs = disp->attr_support & clr_attrs;
	disp->cur_attrs &= ~attrs;
	return (attrs == clr_attrs);
}


/* Prepare a cell for setting its Unicode code point by filling the current
 * foreground/background colour and attributes.  The result is a cell that
 * can be filled into multiple positions.
 *
 * This function does not fail.
 */
static void loratype_prepcell (struct loratype_display *disp, struct loratype_cell *prepared) {
	ltc_colr fgcol = disp->cur_fgcol;
	ltc_colr bgcol = disp->cur_bgcol;
	ltc_attr attrs = disp->cur_attrs;
	if (disp->last_colr > 1) {
		if (attrs & LORATYPE_REVERSE) {
			fgcol ^= bgcol;
			bgcol ^= fgcol;
			fgcol ^= bgcol;
		}
		if (attrs & LORATYPE_DIM) {
			fgcol >>= 1;
		}
		attrs = attrs & ~( LORATYPE_REVERSE | LORATYPE_DIM );
	}
	prepared->code  = ' ';
	prepared->fgcol = fgcol;
	prepared->bgcol = bgcol;
	prepared->attrs = attrs;
	prepared->flags = (LORATYPE_DIRTY | LORATYPE_UNSENT);
}


/* Fill Unicode code points with the current foreground and background
 * colour and attribtues to the given cursor position.  Set the DIRTY
 * and UNSENT flags to accommodate future display updates.  Use a
 * length of 1 to just write a single code point.  Care should be
 * taken not to proceed outside the display range.
 *
 * This function does not fail, though it may crash.
 */
void loratype_fill (loratype_display *disp, ltc_indx crspos,
			ltc_code code, uint16_t length) {
	loratype_cell prepcell;
	loratype_prepcell (disp, &prepcell);
	struct loratype_cell *curcell = disp->cells + crspos;
	log_debug ("Filling %d characters from %ld\n", length, crspos);
	while (length-- > 0) {
		curcell->code   = code;
		curcell->fgcol  = prepcell->fgcol;
		curcell->bgcol  = prepcell->bgcol;
		curcell->attrs  = prepcell->attrs;
		curcell->flags |= prepcell->flags;
		curcell++;
	}
}


/* Write an array of Unicode code point with the current foregound and
 * background colour and attributes to a given cursor position in a display.
 * Set the DIRTY and UNSENT flags to accommodate future display updates.
 * The counter may span multiple lines.  Care should be taken not to proceed
 * outside the display range.
 *
 * This function does not fail, though it may crash.
 */
void loratype_write (loratype_display *disp, ltc_indx crspos,
			ltc_code *codes, uint16_t codes_len) {
	struct loratype_cell prepcell;
	loratype_prepcell (disp, &prepcell);
	struct loratype_cell *curcell = disp->cells + crspos;
	log_debug ("Writing %d characters to %ld\n", codes_len, curcell - disp->cells);
	while (codes_len-- > 0) {
		curcell->code   = *codes++;
		curcell->fgcol  = prepcell->fgcol;
		curcell->bgcol  = prepcell->bgcol;
		curcell->attrs  = prepcell->attrs;
		curcell->flags |= prepcell->flags;
		curcell++;
	}
}


/* Move a sequence of cells in the display to another position.  Care
 * should be taken not to proceed outisde the display range.  The old
 * content is not cleared, so this may also be used to copy lines.
 *
 * This function does not fail, though it may crash.
 */
void loratype_move (loratype_display *disp, ltc_indx from,
			ltc_indx to, uint16_t length) {
	uint16_t ofs = 1;
	struct loratype_cell *fc, *tc;
	if (to == from) {
		return;
	} else if (to < from) {
		fc = &disp->cells [from];
		tc = &disp->cells [to  ];
		ofs = +1;
	} else {
		fc = &disp->cells [from + length - 1];
		tc = &disp->cells [to   + length - 1];
		ofs = -1;
	}
	while (length-- > 0) {
		memcpy (tc, fc, sizeof (loratype_cell));
		fc += ofs;
		tc += ofs;
	}
}


/* Send display updates, provided that the buffer has enough space left.
 * If a change of the colours and/or attributes is required, this is
 * first done.
 *
 * It is assumed that this call is alternated with loratype_update_ack()
 * to process an acknowledgement from the terminal device; if it is not
 * responsive, another call to this procedure will produce the same
 * update buffer (or, possibly, it might use a different size).
 *
 * This function returns the number of bytes written into the MTU.
 */
uint16_t loratype_update (loratype_display *disp, uint8_t *buf, uint16_t bufsz) {
	uint16_t origsz = bufsz;
	ltc_colr newfgcol = disp->disp_ack_fgcol;
	ltc_colr newbgcol = disp->disp_ack_bgcol;
	ltc_attr newattrs = disp->disp_ack_attrs;
	ltc_indx newindex = disp->disp_ack_sndfr;
	//
	// Iterate over material to send, until nothing left or buffer full
	while (bufsz > 0) {
		//
		// Take note if the index changes
		bool chgindex = false;
		//
		// Chase for a character that is DIRTY for whatever reason
		// Ignore UNSENT in support of resends with this code
		uint16_t todo = disp->numcells - newindex;
		struct loratype_cell *cell = &disp->cells [newindex];
		bool more = (newindex >= disp->disp_ack_sndfr) && (disp->disp_ack_sndfr > 0);
		log_debug ("Start at %ld\n", cell - disp->cells);
		while ((cell->flags & LORATYPE_DIRTY) == 0) {
			if (todo-- > 0) {
				// Continue to the next cell
				cell++;
				newindex++;
			} else if (more) {
				// Restart from the beginning
				cell = disp->cells;
				newindex = 0;
				todo = disp->disp_ack_sndfr - 1;
				more = false;
				log_debug ("Wrapping at %ld for %d more cells\n", cell - disp->cells, todo);
			} else {
				// Nothing left to send
				log_debug ("Done at %ld\n", cell - disp->cells);
				goto no_more;
			}
			chgindex = true;
		}
		log_debug ("Updating at %ld\n", cell - disp->cells);
		//
		// Try to fit in an index change, if needed
		if (chgindex) {
			// We need to change the index
			if (disp->numcells+14 < 0x1f) {
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ =  newindex;
			} else if (disp->numcells+14 < 0x1fff) {
				if (bufsz < 2) {
					goto no_more;
				}
				bufsz -= 2;
				*buf++ =  newindex >>  8;
				*buf++ =  newindex        & 0xff;
			} else {
				if (bufsz < 3) {
					goto no_more;
				}
				bufsz -= 3;
				*buf++ =  newindex >> 16;
				*buf++ = (newindex >>  8) & 0xff;
				*buf++ =  newindex        & 0xff;
			}
		}
		//
		// Try to fit in a colour/attr change, if needed
		ltc_colr altfgcol = cell->fgcol;
		ltc_colr altbgcol = cell->bgcol;
		ltc_attr altattrs = cell->attrs;
		bool chgfgcol = (altfgcol != newfgcol);
		bool chgbgcol = (altbgcol != newbgcol);
		bool chgattrs = (altattrs != newattrs);
		switch (disp->last_colr) {
		case 1:
			// Monochrome: 10 + 6 bits with attributes
			if (chgattrs) {
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = 0x80 | cell->attrs;
			}
			break;
		case 15:
			// Colour16: 10mm + xxxx [+ yyyyzzzz]
			// mm=00 -> xxxx=attrs, yyyy=bg, zzzz=fg
			// mm=01 -> xxxx=attrs
			// mm=10 -> xxxx=fg
			// mm=11 -> xxxx=bg
			if (((int) chgfgcol) + ((int) chgbgcol) + ((int) chgattrs) > 1) {
				// mm=00
				if (bufsz < 2) {
					goto no_more;
				}
				bufsz -= 2;
				*buf++ = 0x80 | altattrs;
				*buf++ = (altbgcol << 4) | altfgcol;
			} else if (chgattrs) {
				// mm=01
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = 0x90 | altattrs;
			} else if (chgfgcol) {
				// mm=10
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = 0xa0 | altfgcol;
			} else if (chgbgcol) {
				// mm=11
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = 0xa0 | altbgcol;
			}
			break;
		case 255:
			// Colour256: 10fbaaaa [+ ffffffff] [+ bbbbbbbb]
			if (chgattrs || chgfgcol || chgbgcol) {
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = 0x80 | (chgfgcol ? 0x20 : 0x00)  | (chgbgcol ? 0x10 : 0x00) | altattrs;
			}
			if (chgfgcol) {
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = altfgcol;
			}
			if (chgbgcol) {
				if (bufsz < 1) {
					goto no_more;
				}
				bufsz--;
				*buf++ = altbgcol;
			}
			break;
		default:
			exit (1);
		}
		newfgcol = altfgcol;
		newbgcol = altbgcol;
		newattrs = altattrs;
		//
		// Try to fit in the character code point in UTF-8
		ltc_code codept = cell->code;
		if (codept < 0x80) {
			if (bufsz < 1) {
				goto no_more;
			}
			bufsz--;
			*buf++ = (uint8_t) codept;
		} else if (codept < 0x800) {
			if (bufsz < 2) {
				goto no_more;
			}
			bufsz -= 2;
			*buf++ = 0xc0 |  (codept >>  6);
			*buf++ = 0x80 | ( codept        & 0x3f);
		} else if (codept < 0x10000) {
			if (bufsz < 3) {
				goto no_more;
			}
			bufsz -= 3;
			*buf++ = 0xe0 |  (codept >> 12);
			*buf++ = 0x80 | ((codept >>  6) & 0x3f);
			*buf++ = 0x80 | ( codept        & 0x3f);
		} else {
			if (bufsz < 4) {
				goto no_more;
			}
			bufsz -= 4;
			*buf++ = 0xf0 |  (codept >> 18);
			*buf++ = 0x80 | ((codept >> 12) & 0x3f);
			*buf++ = 0x80 | ((codept >>  6) & 0x3f);
			*buf++ = 0x80 | ( codept        & 0x3f);
		}
		//
		// Setup this cell for acknowledgements
		cell->flags &= ~LORATYPE_UNSENT;
		//
		// When more can be sent, look at the next cell
		newindex = newindex + 1;
		if (newindex >= disp->numcells) {
			newindex = 0;
		}
	}
	//
	// Update variables to commit what we can send -- and wait for acknowledgement
no_more:
	disp->disp_snt_fgcol = newfgcol;
	disp->disp_snt_bgcol = newbgcol;
	disp->disp_snt_attrs = newattrs;
	disp->disp_snt_sndfr = newindex;
	//
	// Return whatever size we managed to prepare for sending
	return (origsz - bufsz);
}


/* Acknowledge the last display update as it was received by the display.
 *
 * This uses the simple fact that LoRaWAN operates in lock-step mode,
 * where every acknowledged-send awaits an acknowledgement before the
 * next frame can be sent.  We shall prepare the administartion on the
 * display so the next loratype_update() will continue where the
 * acknowledged part left off, regardless of whether it was incomplete
 * or has since fallen behind due to cells set to DIRTY.
 */
void loratype_update_ack (loratype_display *disp) {
	log_debug ("Acknowledgement function started\n");
	disp->disp_ack_sndfr = disp->disp_snt_sndfr;
	disp->disp_ack_fgcol = disp->disp_snt_fgcol;
	disp->disp_ack_bgcol = disp->disp_snt_bgcol;
	disp->disp_ack_attrs = disp->disp_snt_attrs;
	uint16_t ctr = disp->numcells;
	struct loratype_cell *cell = disp->cells;
	while (ctr-- > 0) {
		uint8_t flags = cell->flags;
		if ((flags & (LORATYPE_DIRTY | LORATYPE_UNSENT)) == LORATYPE_DIRTY) {
			cell->flags &= ~LORATYPE_DIRTY;
			log_debug ("Acknowledgement at %ld\n", cell - disp->cells);
		}
		cell++;
	}
}


int main (int argc, char *argv []) {
	uint32_t hello_world [] = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' };
	uint32_t running_amock [] = { 'R', 'u', 'n', 'n', 'i', 'n', 'g', ' ', 'a', 'm', 'o', 'c', 'k' };
	loratype_display *disp = loratype_create (25, 80, 15, 0x0f);
	assert (disp != NULL);
	assert (loratype_setattrs (disp, LORATYPE_BOLD));
	loratype_write (disp, 12, 40 - sizeof (running_amock) / 8, running_amock, sizeof (running_amock) / 4);
	assert (loratype_clrattrs (disp, LORATYPE_BOLD));
	assert (loratype_setbgcol (disp, 2));
	assert (loratype_setfgcol (disp, 6));
	loratype_write (disp, 5, 40 - sizeof (hello_world) / 8, hello_world, sizeof (hello_world) / 4);
	assert (loratype_setbgcol (disp, 0));
	assert (loratype_setfgcol (disp, 15));
	loratype_write (disp, 21, 40 - sizeof (running_amock) / 8, running_amock, sizeof (running_amock) / 4);
	assert (loratype_setfgcol (disp, 14));
	loratype_write (disp, 23, 40 - sizeof (running_amock) / 8, running_amock, sizeof (running_amock) / 4);
	uint8_t buf [1500];
	uint16_t bufsz;
	while (bufsz = loratype_update (disp, buf, sizeof (buf)), bufsz > 0) {
		log_debug ("Update buffer holds %d bytes\n", bufsz);
		write (1, buf, bufsz);
		loratype_update_ack (disp);
	}
	loratype_destroy (disp);
}
