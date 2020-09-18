/* crc8.c -- Checksumming display regions for lazy updates.
 *
 * We want to apply checksums over small ranges of data, notably regions
 * on the display of the LoraTerm.  By sending more than a single checksum,
 * we can show what region needs to be updated, and that saves the amount
 * of download traffic.  To accommodate flexible use under any MTU regime,
 * the computations shall be easily sizeable.
 *
 * We took note of this exhaustive research:
 *  - https://users.ece.cmu.edu/~koopman/pubs/faa15_tc-14-49.pdf
 *  - https://users.ece.cmu.edu/~koopman/pubs/koopman14_crc_faa_conference_presentation.pdf
 *  - https://users.ece.cmu.edu/~koopman/crc/
 *
 * Although a CRC-16 can detect more bit errors in longer messages, it
 * does occupy twice the space.  We rather send two CRC-8 checksums over
 * half the areas so we can pinpoint the area to check again.  To allow
 * added certainty, we add a salt at the beginning of the process, and
 * after a number of salted checksums we collect more and more certainty
 * (though that is not as accurately researched as this work is).  This
 * should cause eventual correctness.  The salt is implemented as offsets
 * between hashed characters, to allow for maximum distribution of display
 * data over the various checksums.
 *
 * From: Rick van Rein PA1RVR <rick@openfortress.nl>
 */



/* Determine the optimal crc8 polynomial to use, based on the research by
 * Koopman et al.  Given 8 bits and a message size, we use the polynomial
 * that yields the highest Hamming Distance (HD).  The number of bit errors
 * is HD-1.  Since anything works for HD=2 and any message size will do,
 * the value for HD=3 is simply reused for HD=2.  Do note however, that the
 * longer messages see a gradual decay in the chance of a single bit error.
 *
 * Sampled on September 15th, 2020, we have for CRC-8:
 *  - HD=6, poly=0x9b, msglen<=4bits
 *  - HD=5, poly=0xeb, msglen<=9bits
 *  - HD=4, poly=0x83, msglen<=119bits
 *  - HD=3, poly=0xe7, msglen<=247bits
 *  - HD=2, poly=0xe7
 */
uint8_t crc8poly (uint16_t msglen) {
	if (msglen <= 4 / 8) {
		// HD=6
		return 0x9b;
	} else if (msglen <= 9 / 8) {
		// HD=5
		return 0xeb;
	} else if (msglen <= 119 / 8) {
		// HD=4
		return 0x83;
	} else {
		// HD=3 for msglen<=247bits
		// HD=2 beyond that
		return 0xe7;
	}
}


/* Salts should be relative prime to the screen size.  We shall
 * maximise this chance by sending and send a salt index and
 * relay this along with our choice.  The server may choose to
 * not take it seriously if it detects lack of primal instinct.
 * A display may save the bandwidth by skipping it locally.
 *
 * Screens are often 80x25 (factors 2,5) or 80x24 (factors 2,3,5).
 * LCDs tend to use powers of 2.
 *
 * We will provide for sixteen small prime numbers that ought to
 * fit under most screen sizes.
 */
uint16_t salty_primes [16] = {
	 1,  7, 11, 13,
	17, 19, 23, 29,
	31, 37, 41, 43,
	47, 53, 59, 61,
};


/* Return the next salty_prime index in a range 0..15.  When the
 * numcells parameter is non-zero, it continues the search until
 * the next that is relatively prime to this parameter.  Leave
 * it 0 to simply find the next.  Note that this fails with a
 * screen size that multiplies all salty_primes, but that would
 * be 3909612711980232366109, well beyond the 16 bit size range.
 */
uint8_t salty_prime_index (uint16_t numcells) {
	static uint8_t idx = 0;
	do {
		idx = (idx + 1) & 0x0f;
		idx &= 0x0f;
	} while (numcells % salty_primes [idx] != 0);
	return idx;
}


/* Update the CRC-8 checksum with the data from the given cell.
 * This includes the 32-bit character code in network byte order,
 * the the foreground and background colour (even for monochrome)
 * and the attributes (but just those that are supported).
 *
 * Since we shift out bits on the low end of our bytes, we have
 * chosen to do the same with the 32-bit number; this is arbitrary
 * for 8-bit coding but may help in 32-bit shift register coding.
 */
uint8_t crc8addcell (loratype_cell *cell, uint8_t crc8poly, uint8_t crc8sofar) {
	uint8_t msg [7];
	msg [0] = (cell->code      ) & 0xff;
	msg [1] = (cell->code >>  8) & 0xff;
	msg [2] = (cell->code >> 16) & 0xff;
	msg [3] = (cell->code >> 24)       ;
	msg [4] = cell->attrs;
	msg [5] = cell->fgcol;
	msg [6] = cell->bgcol;
	for (uint8_t msgidx = 0; msgidx < sizeof (msg); msgidx++) {
		for (uint8_t bitidx = 0; bitidx < 8; bitdx++) {
			bool feedback = ((msg [msgidx] ^ crc8sofar) & 0x01) != 0x00;
			crc8sofar >>= 1;
			if (feedback) {
				crc8sofar ^= crc8poly;
			}
		}
	}
	return crc8sofar;
}


/* Compute the CRC-8 checksum for one run through the display;
 * this starts at a given cell offset, increments with salty steps,
 * and continues for the number of iterations selected.  How this
 * fits into the plan of the overall checksum runs is up to a
 * higher-level function.
 *
 * Note that a physical display may do local "smart" things, such
 * as scrolling over a virtual display.  In that sense, this is
 * a computation over a virtual display.  Also note that any local
 * work on colour code mappings and even character code mappings
 * must not be shown in this virtual display, because it would
 * destroy the checksumming process and cause continues refreshing
 * of the display contents.
 */
uint8_t crc8run (loratype_display *disp, uint16_t offset, uint16_t salty_step, uint16_t cells, uint8_t crc8poly) {
	uint8_t crc8sofar = 0xff;
	assert (offset < cells->numcells);
	while (cells-- > 0) {
		crc8sofar = crc8addcell (&disp->cells [offset], crc8poly, crc8sofar);
		offset = (offset + salty_step) % disp->numcells;
	}
	return crc8sofar;
}


/* Given a buffer of a given size, decide how to fill it with a
 * sequence of CRC-8 checksums.  This may fill the entire space,
 * or leave parts open.  Try to use a different salt on buffers
 * that are computed in succession, to increase the opportunity
 * for error detection.
 *
 * This function returns the use of the MTU buffer.  When 0, no
 * checksumming information was added.
 */
uint16_t crc8compute (loratype_display *disp, uint8_t *buf, uint16_t bufsz) {
	if (bufsz < 2) {
		return 0;
	}
	uint16_t maxcrc = (bufsz - 1);
	uint16_t runsz = (disp->numcells + maxcrc - 1) / maxcrc;
	uint16_t numruns = (disp->numcells + msgsz - 1) / msgsz;
	uint8_t poly8 = crc8poly (7 * runsz);
	uint8_t pridx = salty_prime_index (disp->numcells);
	uint16_t prsalt = salty_prime [pridx];
	*buf++ = disp->numcells;
TODO: next computation at offset*runsz % disp->numcells
	for (uint16_t offset = 0; offset < numruns; offset += runsz) {
		*buf++ = crc8run (disp, offset, prsalt, runsz, poly8);
	}
	return 1 + numruns;
}


/* Throw with dirt.  Mark those cells as dirty that are used in a
 * crc8run() and turned up a bad checksum.  The same parameters
 * are used, except for the CRC-8 polynomial.
 */
void crc8throwdirt (loratype_display *disp, uint16_t offset, uint16_t salty_step, uint16_t cells) {
	assert (offset < cells->numcells);
	while (cells-- > 0) {
		disp->cells [offset].flags |= LORATYPE_DIRTY;
		offset = (offset + salty_step) % disp->numcells;
	}
}


/* Given a checksum buffer of a given size, verify the contained
 * sequence of CRC-8 checksums, given the salty prime index.
 * When checksums fail, mark the corresponding cells as dirty.
 */
void crc8validate (loratype_display *disp, uint8_t *buf, uint16_t bufsz) {
	bool ok = true;
	ok = ok && (bufsz >= 2);
	uint8_t pridx = buf [0] & 0x0f;
	uint16_t prsalt = salty_prime [pridx];
	uint16_t numruns = bufsz - 1;
	uint16_t msgsz = (disp->numcells + numruns - 1) / numruns;
	ok = ok && (...%... == 0);

	uint16_t maxcrc = (bufsz - 1);
	uint16_t runsz = (disp->numcells + maxcrc - 1) / maxcrc;
	uint16_t numruns = (disp->numcells + msgsz - 1) / msgsz;
	uint8_t poly8 = crc8poly (7 * runsz);
	*buf++ = disp->numcells;
TODO: next computation at offset*runsz % disp->numcells
	for (uint16_t offset = 0; offset < numruns; offset += runsz) {
		*buf++ = crc8throwdirt (disp, offset, prsalt, runsz);
	}
	return 1 + numruns;
}
