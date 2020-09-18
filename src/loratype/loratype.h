/* loratype.h -- Data structures for TTY service on a LoRaWAN
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


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>


/* Types for code points, colours and attributes.
 * And for line, column and cell array index.
 */
typedef uint32_t ltc_code;
typedef uint8_t  ltc_colr;
typedef uint8_t  ltc_attr;
typedef uint8_t  ltc_line;
typedef uint8_t  ltc_colm;
typedef uint16_t ltc_indx;



/* Individual characters are named cells and described as a
 * combination of Unicode character code point, rendering
 * attributes, foreground/background colour and local flags.
 */
typedef struct loratype_cell {
	ltc_code code;		/* Unicode code points */
	ltc_colr fgcol;		/* Xterm's 16 or 256 colours */
	ltc_colr bgcol;		/* Xterm's 16 or 256 colours */ 
	ltc_attr attrs;		/* See LORATYPE_BOLD etc below */
	uint8_t  flags;		/* See LORATYPE_DIRTY etc below */
} loratype_cell;


/* Attributes are defined as bits.  Note that that attributes for
 * reverse video and dimmed display are sent as-is to monochrome
 * terminals, but handled locally through colour changes for any
 * terminal with 16 or 256 colours.
 */
#define LORATYPE_BOLD      0x01
#define LORATYPE_UNDERLINE 0x02
#define LORATYPE_ITALICS   0x04
#define LORATYPE_BLINKING  0x08
#define LORATYPE_REVERSE   0x10
#define LORATYPE_DIM       0x20


/* Flags are for internal administration of each character.
 * The most notably flag is whether a character is DIRTY, that
 * is written but not on the display yet, UNSENT is reset until
 * acknowledgement by the display.  Most writes mark characters as
 * DIRTY, UNSENT but exceptions exist, for example when
 * a side-effect of scrolling or clearing an area it may be set
 * to DIRTY, UNSENT.
 */
#define LORATYPE_DIRTY  0x01
#define LORATYPE_UNSENT 0x02


/* Displays consist of a width and height, a selection of colours,
 * a set of supported attributes and of course the cells with the
 * various characters, all sequenced in reading order, where cells
 * per line follow each other and lines follow each other.  There
 * is no boundary between lines, all are layed out in one flat
 * array with all the cells of a display.
 */
typedef struct loratype_display {
	struct loratype_cell *cells;
	ltc_line height;
	ltc_colm width;
	ltc_indx numcells;		/* Equals height * width */
	ltc_indx scrollbeg;
	ltc_indx scrollend;
	ltc_colr last_colr;		/* Colour mode: 1, 15 or 255 */
	ltc_attr attr_support;
	ltc_colr cur_fgcol;
	ltc_colr cur_bgcol;
	ltc_attr cur_attrs;
	ltc_indx disp_ack_sndfr;	/* Sendfrom indexpos; acknowledged by display */
	ltc_indx disp_snt_sndfr;	/* Sendfrom indexpos; awaiting display acknowledge */
	ltc_colr disp_ack_fgcol;	/* Foreground colour; acknowledged by display */
	ltc_colr disp_ack_bgcol;	/* Background colour; acknowledged by display */
	ltc_attr disp_ack_attrs;	/* Render attributes; acknowledged by display */
	ltc_colr disp_snt_fgcol;	/* Foreground colour; awaiting display acknowledge */
	ltc_colr disp_snt_bgcol;	/* Background colour; awaiting display acknowledge */
	ltc_colr disp_snt_attrs;	/* Render attributes; awaiting display acknowledge */
} loratype_display;



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
loratype_display *loratype_create (ltc_line lines, ltc_colm columns, ltc_colr last_colr, ltc_attr attr_support);


/* Destroy a display, as created by loratype_create().
 *
 * This function does not fail.
 */
void loratype_destroy (loratype_display *disp);


/* Change the foreground colour; future updates will use this value.
 *
 * This function returns true when the colour is supported.  In case
 * of range error, the foreground is the highest supported colour code.
 */
bool loratype_setfgcol (struct loratype_display *disp, ltc_colr new_fgcol);


/* Change the background colour; future updates will use this value.
 *
 * This function returns true when the colour is supported.  In case
 * of range error, the foreground is the lowest supported colour code.
 */
bool loratype_setbgcol (struct loratype_display *disp, ltc_colr new_bgcol);


/* Set attribute flags; multiple may be set, none is reset.
 *
 * This function returns true when all attributes are supported.
 * Otherwise, it has made a best effort (not further specified
 * and subject to change between releases).
 */
bool loratype_setattrs (struct loratype_display *disp, ltc_attr set_attrs);


/* Clear attribute flags; multiple may be cleared by _setting_
 * their bit in clr_attrs; none is set.
 *
 * This function returns true when all attributes are supported.
 * Otherwise, it has made a best effort (not further specified
 * and subject to change between releases).
 */
bool loratype_clrattrs (struct loratype_display *disp, ltc_attr clr_attrs);


/* Fill Unicode code points with the current foreground and background
 * colour and attribtues to the given cursor position.  Set the DIRTY
 * and UNSENT flags to accommodate future display updates.  Use a
 * length of 1 to just write a single code point.  Care should be
 * taken not to proceed outside the display range.
 *
 * This function does not fail, though it may crash.
 */
void loratype_fill (loratype_display *disp, ltc_indx crspos,
			ltc_code code, uint16_t length);


/* Write an array of Unicode code point with the current foregound and
 * background colour and attributes to a given cursor position in a display.
 * Set the DIRTY and UNSENT flags to accommodate future display updates.
 * The counter may span multiple lines.  Care should be taken not to proceed
 * outside the display range.
 *
 * This function does not fail, though it may crash.
 */
void loratype_write (loratype_display *disp, ltc_indx crspos,
			ltc_code *codes, uint16_t codes_len);


/* Move a sequence of cells in the display to another position.  Care
 * should be taken not to proceed outisde the display range.  The old
 * content is not cleared, so this may also be used to copy lines.
 *
 * This function does not fail, though it may crash.
 */
void loratype_move (loratype_display *disp, ltc_indx from,
			ltc_indx to, uint16_t length);


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
uint16_t loratype_update (loratype_display *disp, uint8_t *buf, uint16_t bufsz);


/* Acknowledge the last display update as it was received by the display.
 *
 * This uses the simple fact that LoRaWAN operates in lock-step mode,
 * where every acknowledged-send awaits an acknowledgement before the
 * next frame can be sent.  We shall prepare the administartion on the
 * display so the next loratype_update() will continue where the
 * acknowledged part left off, regardless of whether it was incomplete
 * or has since fallen behind due to cells set to DIRTY.
 */
void loratype_update_ack (loratype_display *disp);


