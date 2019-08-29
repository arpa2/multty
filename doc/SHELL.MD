# Automation-Friendly Shells

> *An ASCII-based shell can use control codes to be friendlier
> for automation, at the expense of some user comfort.  The
> relation however, is simple enough for quick translation.*

The intention of the following shell language is to exploit
ASCII control codes for their intended purpose of escaping
and separation, rather than invent a "readable" notation
based on characters that might also be considered useful in
the actual texts being entered.

The format below breaks with that tradition, but not it is
not completely ignorant; the idea is that a thin wrapper
can easily be made to translate more traditional formats
to these automation-friendly shells.

Not incorporated here are mechanisms for looping or choices,
for manipulating variable contents and so on.  Not sure if
that will grow in later on.  For now, the purpose is to
have a clear structure for starting programs in a highly
reliable manner with detailed control over streams and
how the are routed and connected.


## Interfaces

There are a few places where commands may be entered.
One of them is on `stdctl`, in places where we value
this meta-command.  This is (at least) interesting in
multiplexers, to control programs being run and
operated on.  This would apply to commands like `pitty`
and `initty` that are meant to start programs and
connect their streams in arbitrary manners.

A user-friendly shell named `shotty` with more traditional
input on `stdin` could translate to automation-friendly
ASCII commands for an internal `pitty` command.  The
many problems related to escaping and parsing of a
commandline are likely to occur as part of `shotty` as
it is in other shells.


## Escaping and Separating

We use `<DLE>` for escaping, as we always do.  This means
that we have less problems that require doubling the number
of escape characters as in `\\\\` seen in humanly-readable
escaping style in `bash`.  Escaping prefixes one character
only and delivers another character for the escaped one.
Escaping an escaped text is possible, though we shall aim
to avoid that by coupling commands through their streams
and define them by name as much as possible.

We use the four separators `<US>`, `<RS>`, `<GS>`, `<FS>`
for units, records, groups and files.  These are roughly
used as follows:

  * `<US>` separates words, so spaces are nothing special
    in shell words;
  * `<RS>` ends ?
  * `<GS>` is placed between a definition and further use
    of it, somewhat like a `letrec ... in ...` construct.
  * `<FS>` separates commands, so we can use `<LF>` or `<CR>`
    without escaping them, and also without their confusion.

Since elementary parts like words in shells are escaped,
we can rely on these control codes not occurring in them
and causing confusion.


## Completion, Naming and Variables

It is generally useful to be able to ask for help, and
even automation benefits from commandline completion,
as it helps to guide menu-driven processes as well as
a user actually hammering away at commands in a slightly
less escaped format.

Traditional completion uses the `<HT>` symbol.  Since we
would like something that does not occur in normal ASCII,
we shall use the `<SUB>` symbol instead; it should never
be entered, but merely used to substitute unknown characters
as a graphical instruction just before rendering text.

The result of completion is to present a
[menu of options](MENU.MD) on a suitable output stream.

The form `NAME` is used below to indicate unique names.
In places where they are declared, the symbol `<SUB>`
may be used to ask for the completion of the `NAME`
form with a unique numerical value, such as a PID.  This
might look like `sh.<SUB>` and end up being setup as
`sh.12345` for example.
**TODO:** Or reference a variable like below?

The form `<SOH>name<SUB>` with no permitted `<US>` as in
other names may be used to quote the value of a variable.
Note the semantical poverty in comparison to `bash`.  This
is in line with the absense of things like program control
through loops and such.  Not sure if/how that will keep up.


## Explicit Control Commands

We shall write commands in a more traditional form, but
these are translated to `<US>` separated words with
`<DLE>` escaping, of course.  Uppercase words are used
to signify variable words to be formulated at will.
We freely use ellipsis and optional brackets too.

`var VARNM set VALUE` sets a variable `VARNM` to the
textual `VALUE` without exporting as an envvar, or
`var VARNM setenv VALUE` to set a variable with export
as envvar.  In both cases, use `var VARNM reset` to
remove, including as an envvar.  Use again to replace
a variable with a new value.  We may add some form of
nesting context scoping to get closer to `.ini` style.

`table VARNM add|del VALUE...` adds or removes the
`VALUE...` list in table variable `VARNM`.  It only
modifies one tuple at a time.  Tables are sets of
tuples; both addition and deletion are idempotent.

`program NAME [input INPUT]... [output OUTPUT]...
run PROG [ARG]...` will run the `PROG [ARG]...` program
using `NAME` as the name for further reference.
It is a combination of `program NAME [input INPUT]...
[output OUTPUT]... define PROG [ARG]...`
with an immediate `program NAME start` command.
Similarly, `program NAME stop` and `program NAME wait`
will actively stop the `NAME` program, or passively
wait for it to stop.  There also is 

`stream VARNM from INPUT [of PROGNAME]` and
`stream VARNM to OUTPUT [of PROGNAME]`
connect a stream `INPUT` or `OUTPUT` or program
`PROGNAME` to new variable named `VARNM`; this may
be done before `PROGNAME` is brought online, so as
to have it ready before the stream even shows up.
When `of PROGNAME` is dropped,

Stream definitions will probably be used to listen
to variables and tables, and to insert changes.

**TODO:**
`program NAME config VARNM set VALUE` is similar to
the `setenv` form, but can be used even while the
program is running, to alter its configuration at a
later time.  We may use some form of nested context
scoping to get closer to `.ini` style configuration.
**TODO:**  Such contexts might introduce variable parts
within a class of values, to support such things as
domain-specific configuration.

