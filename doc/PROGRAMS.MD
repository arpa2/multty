# Programs in MulTTY

> *The mulTTY approach can offer a number of
> useful utilities, all building blocks that
> help to uplift the usefulness of TTYs.*

This is all work in progress... or not.
Most of what this is, is a list of possible
programs that can help to deliver a much
better experience for TTY handling.

The names used below may or may not end up
being actual programs.  The ideas that they
represent are what counts, but those may be
integrated with existing programs.

It is quite likely that several of these
programs end up in library functions for
which the programs then become trivial
wrappers.  That may simplify the inclusion
of mulTTY functions into other programs.


## Running Commands: dotty

The `dotty` program is used to run plain commands
that have `stdin`, `stdout` and `stderr`
streams.  It multiplexes the `stdout` and
`stderr` streams into the ones that are
defined by default, so that they fit into a
single stream.

These commands may seem to be similar,

```
cmd arg1 arg2 2>&1
dotty -- cmd arg1 arg2
```

but the latter is much better because it allows
future splitting of the streams, and perhaps
assigning them separate colours, tabs or so on.

The `dotty` program is not aware of other
programs and runs with the bliss of ignorance.
All it cares for are its own streams.


**TODO:** Support the `stdctl` input stream
to start, pause, resume, stop and so on.  Add
support for debugging.

**TODO:** Allow the creation of extra streams
on which the program can output, or from which
it can input information.  Use `-o` and `-i`
flags to add these on top of the standard ones.


## Network Exposure: netty

It is useful to access shells from anywhere,
and `netty` allows just that.  It does not
use system-local credentials but replaces
them with SASL for authentication, and the
ARPA2 ACL constructs to grant access to
configured users and groups.

The sessions that `netty` produces are shared
with anyone who gains access.  The various
rights in the ACL regulate whether they may
only watch or also type.

It should be common to run `netty` under a
TLS cloak.  It may also make sense to run it
within an SSH session, perhaps as a service,
using whatever credentials that brings.

Expect `netty` to run another handler for
the actual operation exported, perhaps `dotty`
or maybe `shotty` or even another.

It may be interesting to think of `netty`
as an alternative method of starting a MOSH
session.  The resulting session would run
`pretty` or a similar program that renders
multiplexing information as desired, just
before the information is lost by only
passing rendered screens.  A similar thing
might apply to protocols such as the Remote
FrameBuffer or VNC.

It may be interesting to consider `netty` as
a login handler too.  Not all SASL procedures
are workable on a console, but some might be
and others might be adapted.  You know, draw
a QR code and grab your mobile.  Or send a
code to a device and receive a response.

Note that `telnet` is long gone, but its port
23 has not been reclaimed and it is still
meant for ASCII session exchanges.  The only
thing funny about Telnet is its use of 0xff
as a command prefix.  As long as `netty` rejects
any such Telnet attempts (or implements them,
why not) and otherwise uses `<DLE>` escaping
for the 0xff command prefix, it should have
no problem talking to any client, Telnet or
plain TCP.  Not that a Telnet client would
understand multiplexing or streams, of course.
But as long as it constrains itself to listen
to local addresses `::1` and `127.0.0.1` it
should be quite useful.  Other than using
its port for a better purpose, no ads for
Telnet are implied here :-)


## Handling SASL: saltty

It can be useful to mix SASL negotiation
into a stream on either end.  The `saltty`
program can be used with `-c` and/or `-s`
flags to operate on the client and/or
server side of a SASL exchange; both might
be meaningful in peer-to-peer situations.


## Container wrapper: clotty

Containers under the OCI tend to export a
TTY but that's pretty much all that is being
said about it.  Use mulTTY to get the most out
of it, especially when a client is able to
split apart the programs and their streams,
and perhaps list each in a separate window
or tab.

There might also be a mode for `clotty` to
run KVM and/or Qemu virtuals and allow it
several virtual terminals and other programs.


## Initiation Rites: initty

The customary `init` program is long gone,
and we seek variation these days.  A simple
replacement such as `runsv` may be used to
run multiple programs in the foreground, and
keep their output around on a mulTTY.

Typical properties of an `initty` program
would be that their `stdctl` stream has
support for `enable` and `disable` commands
and, most interestingly, the power to add a
remote debugger to help out with problems.

This would a very useful approach for containers
that run a small set of services.


## Shell witout Editing: shotty

The `shotty` command runs multiple commands
and produces mulTTY output that multiplexes
individual programs.

This program will probably experiment with
the [menu structure](MENU.MD) to offload
command editing to a variety of clients.


## Pipelining: pitty

The `pitty` command can make complex systems
from programs that are connected by way of
their input and output streams.  Those do not
have to be externally viewable, but some might
be.


## Output Formatting: pretty

The value of `pretty` is that a client gets
to decide on layout of the multiplexed streams
that get delivered to them.  Also, how it wants
to switch between input channels.

Colourful output is useful to separate programs,
for example, but being aware of multiplexing
switches avoids that a stream switches into another
and overtakes its colour.  The `pretty` program
will change that.  There is also room for lines
being printed only once they are complete, or
for knowing where they are overtaken, and start
each other stream at their last-known horizontal
position.

We may grow `pretty` into an interactive terminal
that runs together with a rending program of the
kind we currently indicate as a terminal emulator.
Indeed, mulTTY adds to those programs, namely the
facility of multiplexing (and being smart about it).

It may make sense to run `pretty` as a program
within a MOSH shell.  This shell renders literal
screens but would not pass multiplexing.  Being
optimised for graphical output, it is best to
have a mulTTY handler just before that point of
information loss.


## Running a Terminal: rutty

The program `rutty` would be an intermediate
for a terminal, or perhaps it would itself be
a terminal.  It should not care about rendering,
but may deal with tabs and windows because it
handles multiplexing.  It may also interface with
services and media that might be passed over, and
for which it relays to locally run services.

Expect `rutty` to answer your phone calls, or
at least relay them; expect it to popup the
right questions to interact with a SASL request
to login to a server; expect it to download and
upload files through the ZMODEM protocol and to
connect various interfaces for it.  Oh, and it
will take your typing, pass it to the right
remote and process its textual responses, of
course.

We may instead grow `pretty` into a terminal
handling program and forget about `rutty` as
a separate program.


## Runnig a Windows Environment: witty

A `witty` program is one that uses a windowing
environment to render multiple terminals and
add and remove terminals as indicated by the
multiplexing changes.

These are likely to be existing programs that
happen to integrate the `witty` approach; such
as TWIN, Screen and Tmux.  There will be no
tears when the funny name `witty` is lost and
replaced with the reputation of these lovely
tools.

