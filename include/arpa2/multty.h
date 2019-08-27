/* mulTTY -> include file
 *
 * Functions for handling mulTTY functionality without pain.
 *
 * From: Rick van Rein <rick@openfortress.nl>
 */


#ifndef ARPA2_MULTTY_H
#define ARPA2_MULTTY_H


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/uio.h>


/* A number of ASCII definitions that we use in mulTTY.
 * The c_XXX forms are characters, s_XXX are strings.
 * The DCx names are so generic that we add our own,
 * all starting with letter 'P' for program.
 */
#define c_NUL '\x00'
#define c_SOH '\x01'  /* mulTTY name prefix */
#define c_STX '\x02'
#define c_ETX '\x03'
#define c_SOT '\x04'
#define c_ENQ '\x05'
#define c_ACK '\x06'
#define c_SO  '\x0e'  /* mulTTY stream switch, shift out */
#define c_SI  '\x0f'  /* mulTTY stream switch, shift in  */
#define c_DLE '\x10'  /* mulTTY escape, with XOR 0x40 */
#define c_DC1 '\x11'
#define c_DC2 '\x12'
#define c_DC3 '\x13'
#define c_DC4 '\x14'
#define c_NAK '\x15'
#define c_SYN '\x16'
#define c_ETB '\x17'
#define c_CAN '\x18'
#define c_EM  '\x19'  /* mulTTY end of stream/program media */
#define c_FS  '\x1c'
#define c_GS  '\x1d'
#define c_RS  '\x1e'
#define c_US  '\x1f'  /* mulTTY machine/human name portion separator */
#define c_DEL '\x7f'
#define c_IAC '\xff'  /* for Telnet-aware escaping */
#define c_PUP  c_DC1  /* mulTTY program up     */
#define c_PRM  c_DC2  /* mulTTY program remove */
#define c_PDN  c_DC3  /* mulTTY program down   */
#define c_PSW  c_DC4  /* mulTTY program switch */
/* strings */
#define s_NUL "\x00"
#define s_SOH "\x01"  /* mulTTY name prefix */
#define s_STX "\x02"
#define s_ETX "\x03"
#define s_SOT "\x04"
#define s_ENQ "\x05"
#define s_ACK "\x06"
#define s_SO  "\x0e"  /* mulTTY stream switch, shift out */
#define s_SI  "\x0f"  /* mulTTY stream switch, shift in  */
#define s_DLE "\x10"  /* mulTTY escape, with XOR 0x40 */
#define s_DC1 "\x11"
#define s_DC2 "\x12"
#define s_DC3 "\x13"
#define s_DC4 "\x14"
#define s_NAK "\x15"
#define s_SYN "\x16"
#define s_ETB "\x17"
#define s_CAN "\x18"
#define s_EM  "\x19"  /* mulTTY end of stream/program media */
#define s_FS  "\x1c"
#define s_GS  "\x1d"
#define s_RS  "\x1e"
#define s_US  "\x1f"  /* mulTTY machine/human name portion separator */
#define s_DEL "\x7f"
#define s_IAC "\xff"  /* for Telnet-aware escaping */
#define s_PUP  s_DC1  /* mulTTY program up     */
#define s_PRM  s_DC2  /* mulTTY program remove */
#define s_PDN  s_DC3  /* mulTTY program down   */
#define s_PSW  s_DC4  /* mulTTY program switch */


/* Two different escape profiles.  Binary content wants to send
 * no bytes that might be construed as ASCII control codes, and
 * it also encodes 0x00 and 0xff.  ASCII itself, when passed as
 * not-yet-mulTTY-compliant, should leave common ASCII codes but
 * escape those that are specifically interpreted by mulTTY (and
 * barely used in normal ASCII disciplines anyway).  Finally, if
 * content is aware of mulTTY already, it bypasses escaping.
 *
 * Escaping is NOT idempotent, because <DLE> gets escaped.  The
 * only algebraic nicety is the mulTTY escaping is a zero, both
 * left-sided and right-sided.  But even that is not guaranteed
 * to hold true in the future.  Do not escape upon escape unless
 * you plan to reverse it with unescape after unescape.
 */
#define MULTTY_ESC_MULTTY ((uint32_t) 0x00000000)
#define MULTTY_ESC_BINARY ((uint32_t) 0xffffffff)
#define MULTTY_ESC_ASCII  ((uint32_t) ( (1<< 1)|(1<< 2)| \
	(1<< 3)|(1<< 4)|(1<< 5)|(1<< 6)|(1<<16)|(1<<17)| \
	(1<<18)|(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<23)| \
	(1<<24)|(1<<25)|(1<<28)|(1<<29)|(1<<30)|(1<<31)) )


/* The handle structure, with builtin buffer and stream name,
 * for for a MULTTY stream.
 */
typedef struct {
	struct iovec iov [3];
	int ioc;
	uint8_t buf [1024];
	uint8_t shift [1];  /* ...continues beyond structure */
} MULTTY;


/* The types for a program set and program are opaque.
 */
//MAYBE// struct multty_progset;
//MAYBE// struct multty_prog;
typedef struct multty_progset MULTTY_PROGSET;
typedef struct multty_prog    MULTTY_PROG   ;


/* We can have a global variable with the default program set.
 * It will be instantiated when it referenced, anywhere.
 * Functions call for a pointer, hence MULTTY_PROGRAMS.
 */
extern const struct multty_progset MULTTY_PROGRAMSET_DEFAULT;
#define MULTTY_PROGRAMS (&MULTTY_PROGRAMSET_DEFAULT)



/********** FUNCTIONS FOR STREAM MULTIPLEXING **********/



/* Close the MULTTY handle, after flushing any remaining
 * buffer contents.
 *
 * Drop-in replacement for fclose() with FILE changed to MULTTY.
 * Returns 0 on success, else EOF+errno.
 */
int mtyclose (MULTTY *mty);


/* Check whether escaping is useful for a character under the
 * given escape style.  The style exists to minimise traffic
 * and avoid confusion ("why is my LF escaped?")
 *
 * There are a few styles, which we all port back to the range
 * of control codes.
 *  - most characters are printable, and never escaped
 *  - classical control codes are escaped in binary data
 *  - character <DEL> 0x7f is escaped when <BS>  0x08 is
 *  - character <IAC> 0xff is escaped when <NUL> 0x00 is
 *
 * The <IAC> character is specific to Telnet, where it is the
 * only break with "8-bit clean" transport.  Since mulTTY may
 * be used on port 23, we should avoid this kind of confusion
 * and will escape when the assumption is binary, when <NUL>
 * should also be protected by escaping.
 *
 * Escaping is not done here.  It is simply a prefix <DLE> and
 * the character will be added XOR 0x40.
 */
bool mtyescapewish (uint32_t style, char ch);


/* Escape a string and move it into the indicated MULTTY buffer.
 * The escaping style is provided as a parameter.
 *
 * Escaping is always done by prefixing `<DLE>` and adding the
 * character XOR 0x40.  Control codes end up as 0x40 to 0x5f,
 * DEL becomes 0x3f and Telnet IAC becomes 0xbf.
 *
 * Returns the number of escaped characters, not counting the
 * escapes themselves.  Does not fail other than incomplete
 * translation, which is always accountable to a filled buffer.
 * Note that the return value may be 0, but mtyflush() should
 * then allow further use of this function.
 */
size_t mtyescape (uint32_t style, MULTTY *mty, const uint8_t *ptr, size_t len);


/* Flush the MULTTY buffer to the output, using writev() to
 * ensure atomic sending, so no interrupts with other streams
 * even in a multi-threading program.
 *
 * The mty is setup with the proper iov[] describing its
 * switch from stdout, to its buffer, back to stdout.  This
 * is pretty trivial for stdout, of course, and treated in
 * more optimally than the others.
 *
 * The buffer is assumed to already be escaped inasfar as
 * necessary.
 *
 * Drop-in replacement for fflush() with FILE changed to MULTTY.
 * Returns 0 on success, else EOF+errno.
 */
int mtyflush (MULTTY *mty);


/* Open an MULTTY buffer for input ("r") or output ("w")
 * and using the given stream name.  TODO: Currently the
 * mode "r" / "w" is not used.  TODO: Output is always
 * shift-out based, using `<SO>` ASCII, not `<SI>`.
 *
 * The mty is setup with the proper iov[] describing its
 * switch from stdout, to its buffer, back to stdout.  This
 * is pretty trivial for stdout, of course, and treated in
 * more optimally than the others.
 *
 * The buffer is assumed to already be escaped inasfar as
 * necessary.
 *
 * There are no default open handles.  You have to open
 * a handle for "stdout" (if you don't use puts() and co)
 * and "stdin" and "stderr" yourself.  The default place
 * of "stdin" and "stdout" is recognised in this routine.
 *
 * Drop-in replacement for fopen() with FILE changed to MULTTY.
 * Returns a handle on success, else NULL+errno.
 */
MULTTY *mtyopen (const char *streamname, const char *mode);


/* Send an ASCII string to the given mulTTY steam.
 * Since it is ASCII, it will be escaped as seen fit.
 *
 * Drop-in replacement for fputs() with FILE changed to MULTTY.
 * Returns >=0 on success, else EOF+errno
 */
int mtyputs (const char *s, MULTTY *mty);


/* Send binary data to the given mulTTY steam.
 * Since it passes over ASCII, some codes will
 * be escaped, causing a change to the wire size.
 * Reading back would unescape and remove this.
 *
 * Drop-in replacement for write() with FD changed to MULTTY*.
 * Note: This is *NOT* a drop-in replacement for fwrite().
 * Returns buf-bytes written on success, else -1&errno
 */
ssize_t mtywrite (MULTTY *mty, const void *buf, size_t count);



/********** FUNCTIONS FOR PROGRAM MULTIPLEXING **********/



/* Drop a program in the program set, silently
 * ignoring if it is absent.  The id is how it
 * is located, the with_descr option indicates
 * if a description should be attached, as that
 * differentiates the name.
 */
void mtyp_drop (MULTTY_PROGSET *progset, MULTTY_PROG *prog);


/* Find a program in the program set, based on
 * its 33-character name with optionally included
 * <US> attachment for programs with a description.
 */
MULTTY_PROG *mtyp_find (MULTTY_PROGSET *progset, const char id_us[33]);


/* Have a program in the program set, silently
 * sharing if it already exists.  The id is how it
 * is located, the opt_descr is for human display
 * purposes and may be later updated if it is
 * provided here.  Whether or not a description
 * was added is part of the program identity.
 */
MULTTY_PROG *mtyp_have (MULTTY_PROGSET *progset, const char *id, const char *opt_descr);



#endif /* ARPA2_MULTTY_H */