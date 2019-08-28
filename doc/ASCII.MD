# MulTTY: ASCII for Multiplexing Terminals

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
 
  * Consider TTY sessions over SOCK_SEQPACKET to keep
    atomic chunks together in transit.  This option is
    available for UNIX domain sockets and socketpair()
    and for plain socket() it connects through SCTP.
    This is not a strict requirement, but it simplifies
    matters and improves consistency and possibly also
    security.

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


## Prefixing Names

We regularly use names to specify a managed entity.
This naming can be relative, and assumed known because
relative movements bring attention back to a
previously named entity, or a name may be expicitly
prefixed and its scope determined by the control code
that follows it.

Names start with `<SOH>` and are followed with a
sequence of characters in the range 0x20 through 0x7e,
with the one exception that a control `<US>` may
appear between a machine-friendly name and a
human-friendly name.

The ASCII representation can be of any size, and
especially the human form can be a bit longer, but
any difference between instances must be made in the
first 32 characters or, if the `<US>` marker occurs
in the first 33 characters, in the text up to and
including the `<US>`.  Note that the `<US>` can
add an extra character.

As a result of selecting a beginning as a unique
name, it is possible for the remainder of the name
to vary.  This can be used to update the human-friendly
description for any given name.


## Hierarchy of Program Sets

The outer level of a multiplexer presents a set of
programs.  Such a set has a current and previous
program, between which it can switch quickly.  Any
other switches are made by referencing a program
by name.  These switches indicate to the receiver
that the output sent towards it is moving from
one program to another.

This is used so that a multiplexer can forward the
output from multiple programs.  By switching visibly
to another program, the receiver knows to process
the output as coming from the newly selected program.
The codes below explain how this is encoded in ASCII.

Each program has a current stream for input and another
one for output.  When switching programs, this current
stream is not changed.  For a multiplexer, this is
not of any influence, but for a receiver it means that
the current stream must be annotated with each program
whose output is processed.  Programs may change their
current stream, but only in the output direction.
Again, the aim is to be able to merge traffic from a
few connections in such a way that they can be split at
the receiving end.

Each program can be a parent to yet another program
set.  This is useful when a multiplexer wants to add
scoping or, more likely, when a multiplexer merges
traffic from another multiplexer into its own.  It
is easier to nest those multiplexers, than to rename
everything.  It also has a potentially better link
with intentions for access control, which can then
reference the nesting structure.

Separate commands exist for moving to a program to
its parent, or to the program set for which it is
the parent.  An "outside" multiplexer may have to
be a little creative with these codes.

It is important to understand that ASCII codes are
usualy single-byte entities, but we do build a few
structures, such as the `<SOH>` names and the `<EM>`
concept.  Care has been taken to support setups with
a minimum of state, because this state must usually
be kept when an "outside" multiplexer needs to
make a switch.


## Control Codes

We interpret a few ASCII control codes that are
otherwise ignored.  We do not touch the ones that are
in common use, such as `<LF>`, `<BS>`, `<BEL>` and `<ESC>`;
mulTTY is not a terminal standard, but a set of control
codes that can be used orthogonally to multiplex and
redirect traffic between terminals.  It may be made
part of a terminal, but it could also be arranged
in a client application or intermediate proxy.

These are the control codes that we believe are useful
for the kind of middleware control and multiplexing
that we believe the mulTTY project can add:

  * `<SOH>` starts a name for an entity that will be
    mentioned afterwards.  See above for deails.
    The character following the
    `<SOH>` name determines what is being named and
    the name is considered unique for its context.

  * `<DC1>` through `<DC4>` control program changes,
    but only in the direction it is sent.  Unknown
    names may be created on the fly or ignored,
    depending on sender/receiver semantics.  Programs
    may be removed, though the real actual effect
    again depends on sender/receiver semantics.
    Preceding `<SOH>` naming can be used to signal
    the name concerned, to replace relative program
    references with absolute ones.

    These codes are not usually sent by a program,
    which is mostly concerned with streams.  These
    codes are used for multiplexing programs.  Since
    multiplexers are also programs, multiplexing can
    be nested and the program names will inherit the
    nesting structure.  For every set of programs at
    every nesting level or branch being managed, the
    state to maintain includes a current program and
    the previous program.  This is needed to implement
    these device controls.  In addition, there is a
    globally active program set.

    `<DC1>` moves from the current program set to
    its direct parent set.  Normally, it moves to the
    current program in that set, but an `<SOH>` name
    might follow-up with a search in the parent's
    program set.

    `<DC2>` removes the current program, after possibly
    setting an `<SOH>` named program from the current
    program set current.  It also ends any programs for
    which this is a parent, directly or indirectly, and
    it implicitly ends the streams for each of these
    ended programs.

    After this, there is no current program.  This
    recognisable situation should be used to send an
    `<EM>` code for the entire program, possibly
    including a reason for ending the program.

    `<DC3>` moves from the current program to the
    (child) program set whose parent it is.  When
    an `<SOH>` name is prefixed, a search for that
    name will be performed next.

    `<DC4>` swaps the current program with another
    program within the current program set.  If an
    `<SOH>` name is given, the other program is
    defined by its name, and otherwise the previous
    program is used.

    Names that are being referenced but that are not
    known yet are taken to be introductions of new
    names.  This is even true for `<DC2>`, but it is
    likely to implement compatible behaviour that
    skips a few steps.  After all commands a new
    program is set as the current, though it is set
    to undefined after `<DC2>`.  After all commands
    the previous current program has moved and, for
    this program set, made the new nameless target
    for a next `<DC4>` control.

  * `<EM>` marks the end of a stream of the current
    program run.  After `<DC2>` there is no current
    program but then the special use is to for the
    terminated program as a whole.  When ending a
    stream, the current program returns to the
    default stream without affecting `<SO>` or `<SI>`
    status (described below).

    When no `<SOH>` name precedes `<EM>`, it is taken
    to mean that nothing interesting was to be said
    about ending the stream or program.  Usually,
    that means all went as expected.  If an `<SOH>`
    name precedes `<EM>` however, it is considered
    to report a reason for ending the stream or
    program.  When a `<US>` is part of the reason,
    it separates a machine-readable form from a
    human-friendly text.  The machine-readable form
    should be portable, and so the POSIX error codes
    cannot be used.  The `com_err` library provides
    numeric values that are portable, and can be
    used in this position as decimally written
    signed 32-bit integers, of which 0 represents
    OK.  The names of POSIX errors can also be used,
    because these are portable.

    Multiplexing can be nested, and because of that
    its interaction with `<EM>` and in general longer
    codes can be confusing; in general, during any
    byte sequence there may be a need to switch to
    another program, but for the locally generated
    `<DC2><SOH>xxx<EM>` that would only be caused by
    parent environments.  When these return in the
    current environment, the relaying of the `<SOH>`
    should continue as if nothing happened.  Then,
    when `<EM>` ends it, the retained information
    about current programs and their streams is
    still available, and can be used to detect how
    to interpret the information.
    **TODO:** Can parents jump in for a named child?

  * `<SI>` and `<SO>` switch to another representation
    under the common ASCII standards, with the
    default as if `<SO>` had been given.  Occurrences
    are shifting to a red ink ribbon instead of
    black, or shifting to another character set in
    Xterm and Linux Console.  We make an orthogonal
    extension by prefixing an `<SOH>` name for a
    stream, or assuming the default streams without
    a name.  Stream names may be created freshly.
    For output, the default is `stdout` and there
    is an additional `stderr` that can be selected
    with an `<SOH>stderr` prefix to either `<SO>` or
    `<SI>` (the best choice is whatever the current
    shift status is).  For input, the default is
    `stdin` and there can also be a `stdctl` with
    `<SOH>stdctl` prefixed to either `<SI>` or
    `<SO>` control.

  * `<STX>` and `<ETX>` and possibly `<SOH>`, `<EOT>` and
    `<ETB>` are intended to frame information, and can be
    used freely when this is meaningful for interpretation
    of a (typed through naming) stream.  Using this,
    `<STX>` starts framing information and both `<STX>`
    and `<ETX>` end any previous framing.  Text before
    a first `<STX>` and after a `<ETX>` is not framed
    and would be treated differently by the stream.
    Being able to frame data allows the markation
    of a piece of text as something to be parsed as
    a unit, and for which to raise syntax errors
    when it does not seem fit.  Note that framing
    is per stream and a frame may be split due to
    a multiplexer's switch to another frame.

  * `<FS>`, `<GS>`, `<RS>`, `<US>` are separators for fields,
    groups, records and units.  They may be used to
    add some structure to a stream's flow, especially
    when it serves a purpose.  Ideas that come to mind
    include tables, menus and user dialogs, all data
    with some structure, though not as tight as JSON
    or XML.

  * `<SYN>`, `<ENQ>`, `<CAN>`, `<ACK>` and `<NAK>` can
    add some structure to protocols, which differs
    from structuring text or data.  Only the `<ENQ>` is
    used in Xterm statusses via stdio, but other
    streams can use these signals in their own way.
    An idea that comes to mind is better automation
    for authentication flows.  We will indeed define
    how SASL can use such things.

  * `<DLE>` is used for binary content that wishes to
    include codes that would otherwise be interpreted
    as control codes.  The general pattern is that a
    code in the range 0x00 to 0x1f plus 0x7f are
    XORed with 0x40 and then sent with `<DLE>` prefixed.
    This means that `<DLE>G` is sent to avoid 0x08
    being interpreted as `<BEL>`.  Applications that
    carry binary content, such as SASL, also need to
    escape `<NUL>` as that might otherwise be removed
    at a whim.  Applications that attach to a Telnet
    port would be wise to escape 0xff to `<DLE>` and
    0xbf, to not trigger command interpretation.


## Reference

This has loose correspondance with ITU Recommendation T.50,
*International Reference Alphabet*, as revised in 1992,
formerly known as *International Alphabet No 5* or IA5.

We interpret the general and somewhat vague descriptions
in this specification with more specificity:

  * We use `<SOH>` liberally as a naming prefix, not just
    to preface text.  This is not a current practice so
    there would be no clashes, but it is a bit creative.
    Programs that do not recognise `<SOH>` may ignore it
    and end up printing the content that follows (which
    is not proper handling of this ASCII code).

  * We use `<DCx>` codes in a particular interpretation,
    because the most known device controls XON and XOFF
    are not a fixed standard and because we believe more
    variation can be obtained through a `stdctl` stream.
    Still, the hint to use `<DC1>` and `<DC2>` in a more
    negative fashion and `<DC3>` and `<DC4>` more in a
    positive atmosphere has been followed.  The use of
    `<DC1>` and `<DC3>` cause a switch to parent and
    back to its current child, which bears a faint
    resemblence with their interpretation as XOFF and
    XON.

  * We prefix `<SI>` and `<SO>` with a name to vary the
    multiplexing operation, but other than that we retain
    its meaning and should see no changes, even without
    removal of multiplexing, other than the possible
    printing of unrecognised `<SOH>` name prefixes.


