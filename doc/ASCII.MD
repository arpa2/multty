# ASCII to the Rescue: Multiplexing Terminals

> *We have never stopped using terminals, and the rise
> of container technology only adds more.  Top that off
> with devices everywhere.  But we need to do them
> better.*

MulTTY is a package for multiplexing terminals by making
better use of the ASCII table, and interpreting a few
extra codes.  The proposal has been checked against the
two most common terminals, namely the Xterminal and the
Linux console, and are a strict extension.

We essentially propose two changes:

  * Start considering TTY flows as multiplexes of one or
    more program, which carries one or more stream.
 
  * Run TTY sessions over SCTP to simplify the different
    streams.  This is not a strict requirement, but it
    can simplify matters and improves concurrency.

In addition, we want to including naming information in
the flows, so they are self-descriptive and thus help
their interpretation.  Most streams will be ASCII text
with extensions such as UTF-8, but some channels might
carry binary data (with escapes) or other structured
content such as tables or menus.

  * Multiplexing can switch between programs, though a
    single program would look just as it always did.
    Programs carry a name, and when programs are brought
    together they will be subject to multiplexing, so
    switching to a named program or using a quick-hand
    reference to a recent one for speed.

  * Programs can have many streams, of which `stdout`
    and `stderr` are common examples.  The streams will
    not be merged but instead multiplexed by switching
    attention from one stream to another.  Though not
    necessary for switching, streams can define a name
    to aid their interpretation.  A newly started
    program already carries default streams; on the
    output side these are `stdout` and `stderr` and on
    the input side we shall have `stdin` and `stdctl`,
    the latter being a control channel to do things
    that we usually do with signals, including such
    things as starting, stopping, pausing, resuming
    and triggering a program.  In addition, values may
    be assigned to allow dynamic changes of a program
    configuration.

One problem solved by keeping streams separate is that
each can carry its own colours.  A switch to another
stream implies a switch to another rendering colour.
It may even help to split streams into lines, so as
not to get confusing output.

A problem solved by naming programs and having streams
for each is that we can see where output comes from, and
direct where input goes to.  Ever wanted to type the
next command to run while a task was running?  Now you
can be sure that an unexpected prompt won't lap it up.

Splitting the data is really not difficult and it all
integrates easily with current technology, by using
a few simple control codes that are currently not
in use or not standardised.  We can have much more
control about how to display things if we are able to
separate flows of control.

Most importantly though, we get an option of much more
control.  How would you like to tell your terminal to
split off a tab for `stderr` so it does not clutter the
`stdout` flow?  Or how would you like to make command
editing a local task, without remote dictatation of
available editing controls?  Or how would you like to
send a command over `stdctl` to add an output stream
for logging at a certain level?  Or how would your
terminal to tell the remote to attach a debugger to a
program and connect a local debugger to it over a new
set of streams?

## Control Codes

We interpret a few ASCII control codes that are
otherwise ignored.  We do not touch the ones that are
in common use, such as `LF`, `BS`, `BEL` and `ESC`.
This is not a terminal standard, but a set of control
codes that can be used orthogonally to multiplex and
redirect traffic between terminals.  It may be made
part of a terminal, but it could also be arranged
in a client application or intermediate proxy.

These are the control codes that we believe are useful
for the kind of middleware control and multiplexing
that we believe the MulTTY project can add:

  * `SOH` starts a name for an entity that will be
    mentioned afterwards.  The name consists of
    characters in the range 0x20 to 0x7e and its first
    64 characters suffice for unique distinction.
    If names are to be nested, multiple `SOH` may be
    sent in sequence, the "outer" name being the first
    sent.  The character following `SOH` determines
    what is being named and the name is considered
    unique for the containing traffic flow at any
    time (which is why multiplexing can add nesting).
    Names are intended to be humanly readable, though
    a `US` can be used between an unreadble unique
    textual value and a readable human string.

  * `DC1`, `DC2`, `DC3`, `DC4` switches to one in four
    programs, possibly prefixed with an `SOH` name.
    When no name is prefixed, the previous name for
    the same control code will be assumed, for which
    the flow then continues.  You can switch quickly
    between programs by inserting these one-byte codes,
    but will need names if your work set involves
    more program names.  Note that these control codes
    are local to a traffic flow, so SCTP can be used
    to have many more than these four programs.
    (Note that this is used on output and input; the
    codes overlap `XON` and `XOFF` which is replaced
    by actions on `stdctl`, though terminals may want
    to translate `^Q` and `^S` to `stdctl` commands.)

  * `EM` marks the end of the current program run, and
    any text until the following `DCx` program switch
    is considered a report of its last state, such as
    its output streams.  When this report starts with
    numeric information and `US`, it marks an exit
    code, where 0 is good and anything else is bad.

  * `SI` and `SO` switch to another stream for the
    current program.  This is normally used to
    switch between two states (`SI` for a red ribbon,
    `SO` for black printing) and this is also how
    Xterm and Linux Console switch between a G0 and
    G1 character set.  We define many more.  A single
    `SI` switches to the first of the `SI` sequence,
    so red ribbon, or G1 character set.  More `SI`
    then continue to additional ones, like a counter,
    to be reset to the basic stream for black print
    or G0 character set upon first encounter of an
    `SO` control.  Additional `SO` can develop in
    another direction (negative counter values?) and
    we use that to access other streams for a program.
    See the shifting table below.

  * `ETB` marks the end of the current stream for the
    current program.  This counters the introduction
    of new streams with additional `SI` or `SO` that
    reach beyond what was known.  Several of these are
    implied by `EM`, so that no `ETB` is needed at
    termination of a program.  When this is issued,
    the current stream is set to the base stream for
    this side (`SI` or `SO`) of the program.

  * `STX` and `ETX` can be used to mark framing of
    information, inasfar as this is meaningful to the
    interpretation of a named stream.  Using this,
    `STX` starts framing information and both `STX`
    and `ETX` end any previous framing.  Text before
    a first `STX` and after a `ETX` is not framed
    and would be treated differently by the stream.
    Being able to frame data allows the markation
    of a piece of text as something to be parsed as
    a unit, and for which to raise syntax errors
    when it does not seem fit.  Note that framing
    is per stream and a frame may be split due to
    a multiplexer's switch to another frame.

  * `FS`, `GS`, `RS`, `US` are separators for fields,
    groups, records and units.  They may be used to
    add some structure to a stream's flow, especially
    when it serves a purpose.  Ideas that come to mind
    include tables, menus and user dialogs, all data
    with some structure, though not as tight as JSON
    or XML.

  * `SYN`, `ENQ`, `CAN`, `ACK` and `NAK` seem useful
    to add some structure to interactions, rather than
    structuring textual information.  The `ENQ` is
    used by Xterminals, for example, but different
    streams can use these signals in other ways.  An
    idea that comes to mind is better automation for
    authentication flows.  We will indeed define how
    SASL can use such things.

  * `DLE` is used for binary content that wishes to
    include codes that would otherwise be interpreted
    as control codes.  The general pattern is that a
    code in the range 0x00 to 0x1f plus 0x7f are
    XORed with 0x40 and then sent with `DLE` prefixed.
    This means that `DLE`,`G` is sent to avoid 0x08
    being interpreted as `BEL`.


## Shift Table

The shift table that is assumed by default is determined
by how many `SI` or how many `SO` were seen since the
last occurrence of the other kind.  A sequence
`...SO...SI...SI...` would read as `SI`,`SI` below.  And
`...SI...SI...SO...` reads as `SO`.  Initially, the state
is `SO`.

This is certainly not the simplest way of shifting in and
out.  It is perhaps the third design, and it was made for
optimal compatibility with the black/red ribbons or the
G0/G1 character sets of Xterm and the Linux Console.

In the output flow:

  * `SO` is the stream `stdout` using the G0 character set.
  * `SI` is the stream `stdout+` using the G1 character set.
  * `SI`,`SI` and so on could be used for extra character sets.
  * `SO`,`SO` is the stream `stderr`.
  * `SO`,`SO`,`SO` and so on are additional output flows, usually
    named during first reference with an `SOH` prefix to aid in
    the interpretation of their content.  Some of the flows may
    be created for the `stdctl` stream, for example a remote
    debugging connection.

In the input flow:

  * `SO` is the stream `stdin`.
  * `SI` and `SI`,`SI` and so on are not defined here, but
    reserved for the terminal rendering program.
  * `SO`,`SO` is the streadm `stdctl`.
  * `SO`,`SO` and so on are additional input flows, usually
    named during first reference with an `SOH` prefix to aid in
    the interpretation of their content.  Some of the flows may
    be created for the `stdctl` stream, for example a remote
    debugging connection.


