# Programming for MulTTY

> *It is really simple to write output that is MulTTY
> compatible.  It will give you much more control
> over the output that you write and leave rendering
> more to the client interface.  Likewise, integrating
> input streams is simple in event-driven programs.*

To use MulTTY, you only write to `stdout` and you only
read from `stdin`.  Any traffic for `stderr` is sent
with a prefix code.  Depending on your style, you may
choose to switch back to `stdout` afterwards, or set
the output stream in any case.

Unless your program is a multiplexer acting on behalf
of other programs, you only need to worry about streams.


## Standard Output with occassional Error Output

To send information to `stdout` by default and make
an explicit switch to and from `stderr` before and
after using it, you can insert escape codes:

```
#define MULTTY_NAME(n) "\x01" #n
#define MULTTY(s) MULTTY_NAME(n) "\x0e"
puts (MULTTY(stderr) "It hit the fan!" MULTTY(stdout));
```

**Default.**
The last form can be made simpler, as it is the
default form and does not need to be explicitly
named:

```
#define MULTTY_NAME(n) "\x01" #n
#define MULTTY(s) MULTTY_NAME(n) "\x0e"
#define MULTTY_stdout "\x0e"
puts (MULTTY(stderr) "It hit the fan!" MULTTY_stdout);
```

Your invariant would be that your stream is always
`stdout` except for temporary switches in your print
statements.  Note that you can write out over multiple
channels in one statement.

**Threading.**
In multi-threaded programs, you may have to be more
careful; while printing one string, `stdout` should
not be overtaken by another thread.

**Shift.**
Everytime you specify a name, you also specify
whether you shift in or out by way of the `<SI>` and
`<SO>` codes `\x0e` and `\x0f`.  Whether you want to
use the red or black part of your ink ribbon or, more
likely if you are assuming an Xterm or Linux Console,
whether you want to use character set G0 or G1.  If
this matters to your program, you might want to define

```
#define MULTTY_NAME(n) "\x01" #n
#define MULTTY_G0(s) MULTTY_NAME(n) "\x0e"
#define MULTTY_G1(s) MULTTY_NAME(n) "\x0f"
#define MULTTY_G0_stdout "\x0e"
#define MULTTY_G1_stdout "\x0f"
```


## Multitude of Output Streams

If you are printing over a variety of streams, then
you may not want to uphold a single stream to write to.
You simply start every print statement with the right
stream reference:

```
#define MULTTY_NAME(n) "\x01" #n
#define MULTTY(s) MULTTY_NAME(n) "\x0e"
#define MULTTY_stdout "\x0e"
#define MULTTY_SASL_TOKEN_END "\x1f"
#define MULTTY_SASL_MESSAGE_END "\x17"
puts (MULTTY_stdout "Please wait...");
puts (MULTTY(debug) "Attempting ANONYMOUS login");
fputs (MULTTY(sasl) "ANONYMOUS bakker@orvelte.nep\x1f\x17", stdout);
```

**Binary.**
The SASL example is easy in this ANONYMOUS example, but in general
there may be some work for the SASL Token to be escaped so as to
retain the binary characters that may occur in its general form.

The characters `\x1f` and `\x17` for `<US>` and `<ETB>` are parts
of the binary coding for a `sasl` stream.  To suppress spurious
line endings, the `fputs()` call was used instead of `puts()`,
which makes most sense on a human-readable stream.


## Escaping Binary Content on Output

Your application usually knows per stream whether it is textual
or binary.  Once you escaped binary content, it can all be merged
on `stdout` and be used as a normal ASCII stream.  Escaping does
not even impose an 8-bit requirement, but the binary content is
likely to hold 8-bit values, which would impose that requirement.
It should be no problem in practice, but keep in mind that this
is not like base64, which encodes binary content to 7-bit channels.

Escaping is done with the `<DLE>` character `\x10` with the
escaped code being XOR-ed with 0x40.  This must be done with any
code that might otherwise be interpreted by the ASCII layer or,
more to the point, the MulTTY handling code for multiplexing.

You may encode all codes below 0x20 and additionally 0x7f, to
get a range of 0x3f through 0x5f after escaping.  You should
include 0x00 if you know it may occur in your binary data.  And
at the very least, include the characters used in MulTTY and
described in [the ASCII document](ASCII.MD).

An escape routine can look like this,

```
/* MulTTY escapes are needed for:
 *
 * NUL, SOH, STX, ETX, EOT, ENQ, ACK, DLE, DC1, DC2,
 * DC3, DC4, NAK, SYN, ETB, CAN, EM, FS, GS, RS, US, DEL.
 *
 * With the exception of DEL, these are expressed in the
 * bitmask below.  DEL has a funny code.
 */
#define MULTTY_ESCAPED ((1<<0)|(1<<1)|(1<<2)|(1<<3)| \
	(1<<4)|(1<<5)(1<<6)|(1<<16)|(1<<17)|(1<<18)| \
	(1<<19)|(1<<20)|(1<<21)|(1<<22)|(1<<23)|(1<<24)| \
	(1<<25)|(1<<28)|(1<<29)|(1<<30)|(1<<31))


/* Return whether a character needs to be escaped.
 * See the definition of MULTTY_ESCAPED before, and
 * add DEL.
 */
bool multty_escaped (char c) {
	if (c > 0x20) {
		return (c == 0x7f);
	} else {
		return ((MULTTY_ESCAPED & (1 << c)) != 0);
	}
}


/* Add escape codes to a MulTTY stream, meaning that all
 * control codes used in MulTTY are prefixed with DLE and
 * XORed with 0x40.  In the most extreme case, the buffer
 * grows to double the size, which must be possible.  The
 * new buffer length is returned.
 */
size_t multty_escape (char *buf, size_t buflen) {
	int escctr = 0;
	int i;
	for (i=0; i<buflen; i++) {
		if (multty_escaped (buf [i])) {
			escctr++;
		}
	}
	buflen += escctr;
	i = buflen;
	while (escctr > 0) {
		if (multty_escaped (buf [i-escctr])) {
			buf [--i] = buf [i-escctr] ^ 0x40;
			buf [--i] = DLE;
			escctr--;
		} else {
			buf [--i] = buf [i-escctr];
		}
	}
	return buflen;
}
```

# Reading Input as Streams

Inputting data from MulTTY is more complex, but only as the result of
using the extra facilities.  If only `stdin` is used there should be no
difference with normal patterns, because that is the initial stream and
when it is not switched all of it will be the `stdin` stream.

Data read from `stdin` may not all be meant for the default input stream
that we also named `stdin` because it replaces that original idea.  In
fact, using `dotty`, the stream is still used like that, but if your
program wants to be aware of MulTTY itself it should parse its input
to figure out the real `stdin` data.

Stream switching is done with `<SI>` code `0x0e` and `<SO>` code `0x0f`
for shifted and not-so-shifted character sets if you care to distinguish
those even on input.  The customary pattern however is to only use `<SO>`
on input to programs.

Preceding the `<SI>` or `<SO>` code may be a stream name, started with
`<SOH>` code `\x01` and possible including a `<US>` code `\x1f` between
machine-readable constant form and human-readable updateable form.  For
streams, anything after `<US>` is uninteresting to us, so it would make
us skip.  The `<SOH>` always ends in a control code in the range `0x00`
through `0x1f`, but only `<SI>` and `<SO>` warrant a stream switch.

Without `<SOH>`, the stream is `stdin`.  Otherwise, the machine-readable
name is the input stream name.

In practice, we have the freedom to reject streams with a `<US>` because
it is part of the name and we can reject any name we don't like.  So we
can simply parse for strings like `<SOH>sasl<SO>` or `<SOH>stdctl<SO>`
or just a lone `<SO>` as a shorthand for `<SOH>stdout<SO>`.

When an input event is raised, we should scan for such tokens, possibly
by first chasing for `<SO>` and then testing if it is prefixed with a
name that we know.  Such points form a switch in attention.  At the end
of a stream, an `<SOH>` cannot be handed out until it is known to not
continue into an `<SO>`, unless we can be sure that no stream matches
the part of the name that we can recognise.

This would give rise to splitting the input into smaller parts.  Those
parts can then be delivered with a further event to a proper handler
for the input stream.

Program switches are not shown at this level, at least not if your
program is not a multiplexer.  As a result, we can be assured that
the remote end is one program only.  What we cannot assume in general
is that one message comes in as one event, as a result of multiplexing,
so we must always process one character at a time.  We also cannot
assume that no other streams intervene halfway, as a program might
be using a number of threads to produce a number of streams.  This
is already taken care of if the previous concerns have been addressed.
