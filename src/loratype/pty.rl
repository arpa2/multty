# pty.rl -- PseudoTerminal using Ragel to parse Control Codes
#
# These are TTY codes, mostly compliant to Xterm standards but with
# omissions and extensions.  Support for monochrome modes as well as
# 16 or 256 colour mode.
#
# The intention is to process these codes while they are being parsed
# from their delivery channel, which is a pseudo-terminal servicing
# applications on the local POSIX-styled operating system.
#
# From: Rick van Rein PA1RVR <rick@openfortress.nl>



#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <string.h>

#include "loratype.h"


# Clear cells N to M, inclusive.
#
void clear_cells (struct loratype_display *disp, uint16_t first, uint16_t last) {
	if (last >= disp->numcells) {
		last = disp->numcells - 1;
	}
	if (last < first) {
		return;
	}
	loratype_fill (disp, first, ' ', last - first + 1);
}


# Scroll down by N lines.
#
void scroll_down (struct loratype_display *disp, uint16_t lines) {
	ltc_indx mvcell = disp->scrollbeg + lines * disp->width;
	if (mvcell >= disp->scrollend) {
		mvcell = disp->scrollend;
	} else {
		loratype_move (disp, disp->scrollbeg + mvcell, disp->scrollbeg, disp->scrollend - mvcell);
	}
	clear_lines (disp, disp->scrollbeg, mvcell - 1);
}


# Scroll up by N lines.
#
void scroll_up (struct loratype_display *disp, uint16_t lines) {
	ltc_indx mvcell = disp->scrollbeg + lines * disp->width;
	if (mvcell >= disp->scrollend) {
		mvcell = disp->scrollend;
	} else {
		loratype_move (disp, disp->scrollbeg, mvcell, disp->scrollend - mvcell);
	}
	clear_lines (disp, mvcell - 1, disp->scrollend - 1);
}


# Try to move back over N positions, scrolling the display down if needed;
# return the new cursor position.
#
ltc_indx cursor_back (struct loratype_display *disp, ltc_indx cursor, uint16_t back) {
	if (cursor >= back) {
		return cursor - back;
	}
	uint16_t missing = back - cursor;
	uint16_t misslines = (missing + disp->width - 1) / disp->width;
	scroll_down (disp, misslines);
	return cursor + misslines * disp->width - back;
}


# Try to move forward over N positions, scrolling the display up if needed;
# return the new cursor position.
#
ltc_indx cursor_forward (struct loratype_display *disp, ltc_indx cursor, uint16_t fwd) {
	cursor += fwd;
	if (cursor < disp->numcells) {
		return cursor;
	}
	uint16_t missing = cursor - disp->numcells;
	uint16_t misslines = (missing + disp->width - 1) / disp->width;
	scroll_up (disp, misslines);
	return cursor - misslines * disp->width;
}



%%{

machine loractl;
alphtype unsigned char;


# C0 / 7-bit control codes and their C1 / 8-bit aliases
# UTF-8 distinctly never starts with 10... like C1 codes
#
esc = 0x1b;
#MAYBE# ind = esc . 'D' | 0x84;
#MAYBE# nel = esc . 'E' | 0x85;
#MAYBE# hts = esc . 'H' | 0x88;
#MAYBE# ri  = esc . 'M' | 0x8d;
#MAYBE# ss2 = esc . 'N' | 0x8e;
#MAYBE# ss3 = esc . 'O' | 0x8f;
#MAYBE# dsc = esc . 'P' | 0x90;
#MAYBE# spa = esc . 'V' | 0x96;
#MAYBE# epa = esc . 'W' | 0x97;
#MAYBE# sos = esc . 'X' | 0x98;
#MAYBE# decid = esc . 'Z' | 0x9a;
csi = esc . '[' | 0x9b;
#MAYBE# st  = esc . '\\' | 0x9c;
osc = esc . ']' | 0x9d;
#MAYBE# pm  = esc . '^' | 0x9e;
#MAYBE# apc = esc . '_' | 0x9f;


# Mopping up unsupported ANSI X3.41 escape codes (merges without actions)
# https://en.wikipedia.org/wiki/ANSI_escape_code#Escape_sequences
#
# Unicode mopup skips any characters with the high bit set.
#
# Illegal characters are non-escape characters, mopped up one at a time.
#
# The idea is to fallback to mopping-up
#
escmop = ( esc . (0x20..0x2f)* . (0x30..0x7e) ) | ( csi . (0x20..0x3f)* . (0x40..0x7e) );
unimop = ( 0x80..0xff - 0x9b )+;
illmop = any - esc - 0x9b;
mopup = escmop | unimop | illmop;


# Printable characters -- ASCII or UTF-8 -- represented in 1..4 bytes
#
utf8tail = 0x80..0xbf;
utf8 =    (0x20..0x7f)
	| (0xc2..0xdf) . (0x80..0xdf)
	|        0xe0  . (0xa0..0xbf) . utf8tail
	| (0xe1..0xec) . utf8tail{2}
	|        0xed  . (0x80..0x9f) . utf8tail
	| (0xee..0xef) . utf8tail{2}
	|        0xf0  . (0x90..0xbf) . utf8tail{2}
	| (0xf1..0xf3) . utf8tail{3}
	|        0xf4  . (0x80..0x8f) . utf8tail{2}
	>{
		utf8code = 0; }
	${
		int shift = 7;
		while (fc & (0x01 << shift) != 0x00) {
			shift--;
			if (shift == 0) {
				break;
			}
		}
		utf8code <<= shift;
		utf8code |= fc & (shift - 1); }
	@{
		printf ("Unicodepoint %x", utf8code);
		//TODO//SCROLLUP// if (cursor > disp->numcells) { ... }
		loratype_write (disp, cursor / disp->width, cursor % disp->width, &utf8code, 1);
		cursor++; }
	;
printable = utf8+
	;


# Parameterised strings (with default values) after CSI
#
csi_1p_1 = csi
	. ( [0-9]*
		>{ csip0 = 0; }
		${ csip0 = csip0 * 10 + fc - '0'; }
		@{ if (csip0 == 0) { csip0 = 1; } }
	);
csi_semi2p_1_1 = csi_1p_1 .
	( ';' .
		( [0-9]*
			${ csip1 = csip1 * 10 + fc - '0'; }
		)
	) ?
	>{ csip1 = 0; }
	@{ if (csip1 == 0) { csip1 = 1; } }
	;
csi_semi2p_1_max = csi_1p_1 .
	( ';' .
		( [0-9]*
			${ csip1 = csip1 * 10 + fc - '0'; }
		)
	) ?
	>{ csip1 = 0; }
	@{ if (csip1 == 0) { csip1 = ~0; } }
	;


# VT100 codes
#
bel = 0x07
	@{
		/* TODO:BELL:ASLED */ }
	;
bs  = 0x08
	@{
		clear_cells (disp, cursor, cursor);
		cursor = cursor_back (disp, cursor, 1); }
	;
cr  = 0x0d
	@{
		cursor = cursor_back (disp, cursor, cursor % disp->width); }
	;
lf  = 0x0a
	@{
		cursor = cursor_forward (disp, cursor, disp->width); }
	;
ff  = 0x0c
	@{
		clear_cells (disp, 0, ~0);
		cursor = 0; }
	;
#MAYBE# so  = 0x0e;
#ELSEWHERE# sp  = 0x20;
tab = 0x09
	@{
		ltc_indx newcrs = (cursor + 9) % 8;
		clear_cells (disp, cursor, newcrs);
		cursor = newcrs; }
	;
vt  = 0x0b
	@{
		cursor = cursor_forward (disp, cursor, disp->width); }
	;
#MAYBE# si  = 0x0f;
#
simplectl = ( bel | bs | cr | lf | ff | tab | vt );


# Plain-Escaped codes, inasfar as they are explicitly supported
#
crs_save = esc . '7'
	@{
		cursor_saved = cursor;
		printf ("Cursor save\n"); }
	;
crs_restore = esc . '8'
	@{
		cursor = cursor_saved;
		printf ("Cursor restore\n"); }
	;
crs_lowleft = esc . 'F'
	@{
		cursor = disp->numcells - disp->width;
		printf ("Cursor into lower left\n"); }
	;
full_reset = esc . 'c'
	@{
		loratype_setbgcol (disp,  0);
		loratype_setfgcol (disp, 15);
		clear_cells (disp, 0, ~0);
		cursor =
		cursor_saved = 0;
		printf ("Full reset\n"); }
	;
#
plainesc = ( crs_save | crs_restore | crs_lowleft | full_reset );


# CSI-Escaped codes with optional parameters
# See: console_codes(4) and https://en.wikipedia.org/wiki/ANSI_escape_code
#
ins_blank = csi_1p_1 . '@'
	@{
		ltc_indx newcrs = cursor_forward (disp, cursor, csip0);
		clear_cells (disp, cursor, newcrs - 1);
		cursor = newcrs;
		printf ("Insert %d blanks\n", csip0); }
	;
crs_up = csi_1p_1 . 'A'
	@{
		//TODO_NO_EFFECT_AT_RIM//
		cursor = cursor_back (disp, cursor, csip0 * disp->width);
		printf ("Cursor up %d\n", csip0); }
	;
crs_down = csi_1p_1 . 'B'
	@{
		//TODO_NO_EFFECT_AT_RIM//
		cursor = cursor_forward (disp, cursor, csip0 * disp->width);
		printf ("Cursor down %d\n", csip0); }
	;
crs_forward = csi_1p_1 . 'C'
	@{
		//TODO_NO_EFFECT_AT_RIM//
		cursor = cursor_forward (disp, cursor, csip0);
		printf ("Cursor forward %d\n", csip0); }
	;
crs_back = csi_1p_1 . 'D'
	@{
		//TODO_NO_EFFECT_AT_RIM//
		cursor = cursor_back (disp, cursor, csip0);
		printf ("Cursor back %d\n", csip0); }
	;
crs_nextline = csi_1p_1 . 'E'
	@{
		cursor = cursor_forward (disp, cursor - cursor % disp->width, csip0 * disp->width);
		printf ("Next line %d\n", csip0); }
	;
crs_prevline = csi_1p_1 . 'F'
	@{
		cursor = cursor_back (disp, cursor - cursor % disp->width, csip0 * disp->width);
		printf ("Previous line %d\n", csip0); }
	;
crs_abscol = csi_1p_1 . 'G'
	@{
		if (csip0 > disp->width) {
			csip0 = disp->width;
		}
		cursor -= cursor % disp->width;
		cursor += csip0 - 1;
		printf ("Absolute column %d\n", csip0); }
	;
crs_abspos = csi_semi2p_1_1 . 'H'
	@{
		if (csip0 > disp->height) {
			csip0 = disp->height;
		}
		if (csip1 > disp->width) {
			csip0 = disp->width;
		}
		cursor = (csip0 - 1) * disp->width + (csip1 - 1);
		printf ("Absolute position %d,%d\n", csip0, csip1); }
	;
crs_tabfwd = csi_1p_1 . 'I'
	@{
		if (csip0 * 8 > disp->width) {
			csip0 = disp->width / 8;
		}
		//TODO_START_TABPOS//
		cursor -= cursor % disp->width;
		cursor += 8 * csip0 - 1;
		printf ("Forward %d tabs\n", csip0); }
	;
crs_tabback = csi_1p_1 . 'Z'
	@{
		//TODO_BACK_TABPOS//
		printf ("Back %d tabs\n", csip0); }
	;
erase_in_display_below = csi . '0'? . 'J'
	@{
		clear_cells (disp, cursor, disp->numcells - 1); }
	;
erase_in_display_above = csi . '1J'
	@{
		clear_cells (disp, 0, cursor); }
	;
erase_in_display_all = csi . '2J'
	@{
		clear_cells (disp, 0, disp->numcells - 1); }
	;
erase_in_display_saved = csi . '3J'
	@{
		clear_cells (disp, 0, disp->numcells - 1); }
	;
erase_in_line_right = csi . '0'? . 'K'
	@{
		clear_cells (disp, cursor, cursor + cursor->width - 1 - cursor % disp->width); }
	;
erase_in_line_left = csi . '1K'
	@{
		clear_cells (disp, cursor - cursor % disp->width, cursor); }
	;
erase_in_line_all = csi . '2K'
	@{
		clear_cells (disp, cursor - cursor % disp->width, cursor - cursor % disp->width + disp->width - 1); }
	;
line_insert = csi_1p_1 . 'L'
	@{
		cursor -= cursor % disp->width;
		ltc_indx shifted = cursor + csi0p * disp->width;
		if (shifted > disp->numcells) {
			shifted = disp->numcells;
		}
		loratype_move (disp, cursor, shifted, disp->numcells - shifted);
		clear_cells (disp, cursor, shifted - 1); }
	;
line_delete = csi_1p_1 . 'M'
	@{
		cursor -= cursor % disp->width;
		ltc_indx preshift = cursor + csi0p * disp->width;
		if (preshift > disp->numcells) {
			preshift = disp->numcells;
		}
		loratype_move (disp, preshift, cursor, disp->numcells - preshift);
		clear_cells (disp, disp->numcells - precursor, disp->numcells - 1); }
	;
char_delete = csi_1p_1 . 'P'
	@{
		ltc_indx preshift = cursor + csi0p;
		ltc_indx pastline = cursor - cursor % disp->width + disp->width;
		if (preshift < pastline) {
			loratype_move (disp, preshift, cursor, pastline - cursor);
			clear_cells (disp, pastline - csi0p, csi0p);
		} }
	;
char_erase = csi_1p_1 . 'X'
	@{
		loratype preshift = cursor + csi0p;
		loratype pastline = cursor - cursor % disp->width + disp->width;
		if (preshift >= pastline) {
			preshift = pastline;
		}
		clear_cells (disp, cursor, csi0p); }
	;
scroll_up = csi_1p_1 . 'S'
	@{
		scroll_up (disp, csip0); }
	;
scroll_down = csi_1p_1 . 'T'
	@{
		scroll_down (disp, csip0; }
	;
scroll_region = csi_semi2p_1_max . 'r'
	@{
		if (csip1 > disp->height) {
			csip1 = disp->height;
		}
		if (csip0 > csip1) {
			csip0 = 1;
		}
		disp->scrollbeg = disp->width * (csip0 - 1);
		disp->scrollend = disp->width * (csip1 - 1); }
	;
#MAYBE# CSI <n>;<n>;<n>;<n>;<n>T --> initiate mouse tracking
#MAYBE# CSI <n*> ` --> char colabspos
#MAYBE# CSI <n> b --> repeatlastchar
#MAYBE# CSI <n> c --> senddevattrs
#MAYBE# CSI > <n> c --> 2nd senddevattrs
#MAYBE# CSI <n*> d --> lineabspos
#MAYBE# CSI <n>;<n> f --> rowcolabspos
#MAYBE# CSI 0g / CSI 3g --> tabclear
#MAYBE# ... LEDs ...
#
cursor = ( crs_up | crs_down | crs_forward | crs_back | crs_nextline | crs_prevline
	| crs_abscol | crs_abspos | crs_tabfwd | crs_tabback );
erasing = ( erase_in_display_below | erase_in_display_above | erase_in_display_all | erase_in_display_all
	| erase_in_line_right | erase_in_line_left | erase_in_line_all | char_erase );
editing = ( ins_blank | erasing | line_insert | line_delete | char_delete );
scrolling = ( scroll_up | scroll_down scroll_region );
csicodes = ( cursor | erasing | editing | scrolling );


# CSI-Escaped colour codes (adds codes 2m, 3m, 21m, 23m which are not Xterm-supported)
# Note that colour codes 0..15 are shared between 16 and 256 colour modes,
# https://jonasjacek.github.io/colors/
#
colour = [0-9]+
	${ colr = colr * 10 + fc - '0'; }
	;
attr_normal = csi . '0m'
	@{ attr |= 0xff; }
	;
attr_bold = csi . '1m'
	@{ attr |= LORATYPE_BOLD; }
	;
attr_faint = csi . '2m'
	@{ attr |= LORATYPE_DIM; }
	;
attr_italic = csi . '3m'
	@{ attr |= LORATYPE_ITALICS; }
	;
attr_underline = csi . '4m'
	@{ attr |= LORATYPE_UNDERLINE; }
	;
attr_blink = csi . '5m'
	@{ attr |= LORATYPE_BLINKING; }
	;
attr_inverse = csi . '7m'
	@{ attr |= LORATYPE_REVERSE; }
	;
attr_no_bold = csi . '21m'
	@{ attr |= LORATYPE_BOLD; }
	;
attr_no_bold_faint = csi . '22m'
	@{ attr |= LORATYPE_BOLD | LORATYPE_DIM; }
	;
attr_no_italic = csi . '23m'
	@{ attr |= LORATYPE_ITALICS; }
	;
attr_no_underline = csi . '24m'
	@{ attr |= LORATYPE_UNDERLINE; }
	;
attr_no_blink = csi . '25m'
	@{ attr |= LORATYPE_BLINKING; }
	;
attr_no_inverse = csi . '27m'
	@{ attr |= LORATYPE_REVERSE; }
	;
#
attr_set = ( attr_bold | attr_faint | attr_italic | attr_underline | attr_blink | attr_inverse )
	>{ attr = 0; }
	@{
		(void) loratype_setattrs (disp, attr);
		printf ("Setting attr %x\n", attr); }
	;
attr_clr = ( attr_normal |
		attr_no_bold | attr_no_bold_faint | attr_no_italic | attr_no_underline | attr_no_blink | attr_no_inverse )
	>{ attr = 0; }
	@{
		(void) loratype_clrattrs (disp, attr);
		printf ("Clearing attr %x\n", attr); }
	;
#
#MAYBE# colr_fg_default = csi . '39m';
#MAYBE# colr_bg_default = csi . '49m';
colr_fg16 = csi . (
		(3|9  @{ colr = (fc=='3') ? 0 : 8; } )
		. ( [0-7] & colour ))
		. 'm'
	;
colr_bg16 = csi . (
		(4|10 @{ colr = (fc=='4') ? 0 : 8; } )
		. ( [0-7] & colour ))
		. 'm'
	;
colr_fg256 = csi . ( '38;5;' . ( [0-9]+ & colour ) . 'm')
	>{ colr = 0; }
	;
colr_bg256 = csi . ( '48;5;' . ( [0-9]+ & colour ) . 'm')
	>{ colr = 0; }
	;
#
colr_fgset = ( colr_fg16 | colr_fg256 )
	@{
		(void) loratype_setfgcol (disp, colr);
		printf ("Setting foreground colour %d\n", colr); }
	;
colr_bgset = ( colr_bg16 | colr_bg256 )
	@{
		(void) loratype_setbgcol (disp, colr);
		printf ("Setting background colour %d\n", colr); }
	;
#
attributes = ( attr_set | attr_clr );
colours = ( colr_fgset | colr_bgset );


# OSC-Escaped operating system codes --> any needed?
#


# PM-Escaped privacy messages --> not for Xterm
#


# Complete Terminal Control ; smartly drop characters for error handling
#
main := ( printable | simplectl | plainesc | csicodes | attributes | colours )+
	$err{
		fhold;
		fgoto errhdl;
	}
	;
#
errhdl := mopup
	@{
		printf ("Skipping erroneous code\n");
		fgoto main; }
	;


}%%


# Ragel STD static constants
#
%% write data;



# Read bytes from ptyfd, holding what is printed to an Xterm in
# monochrome or with 16 or 256 colours.  Process embedded commands
# such as colour and attribute settings, cursor movements and
# screen manipulation, and of course UTF-8 text to be displayed.
#
# The error handling routings in the Ragel code will skip wrong
# or unsupported codes as well as possible, hoping not to cause
# incompatibility and certaintly aiming to avoid shutdown of the
# Xterm service.
#
void pty2display (struct loratype_display *disp, int ptyfd) {
	//
	// Variables used in the Ragel actions
	ltc_attr attr;
	ltc_colr colr;
	ltc_code utf8code;
	uinsigned int csip0;
	uinsigned int csip1;
	//
	// Setup state for a fresh state diagram
	int cs;
	unsigned char ptybuf [512];
	int ptybuflen;
	loratype_indx cursor = 0;
	loratype_indx cursor_saved = 0;
	%% write init;
	//
	// Loop for input until the pty closes
	while (true) {
		//
		// Read data to process
		ptybuflen = read (ptyfd, ptsbuf, sizeof (ptsbuf));
		if (ptybuflen <= 0) {
			return;
		}
		p = ptybuf;
		pe = p + ptybuflen;
		//
		// Pass the data through the state diagram
		%% write exec;
	}
}

