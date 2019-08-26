# Menus and Tables

> *This is an experiment to see how far we can take
> the structuring of data flows in ASCII tables.  Given
> the basic ideas of mulTTY, can we export the interface
> in such a way that control is more of a user style?*

The simplest programs to write are commandline programs.
But their complexity grows when more forms of user
interaction are added.  Graphical interfaces are an
alternative, but they are not fit everywhere on account
of their strong graphical orientation and inclusion of
refinments of fonts, rendering methods, languages and
so on.

This is an attempt to use standard ASCII codes, giving
them a little more interpretation, and leaving it to
the user environment to render it in any way that seems
useful under the user's preferred way or working.  All
we may have to do is structure the user interaction,
and update it when the need arises.

This approach should work equally well in a graphical
and textual environment; whether driven by menus or
the entry of commands.  As long as the available
options are clear along with their method of delivery,
we should be happy.

The reason that this experiment is named after menus,
is that they seem to be the common ground.  Graphical
user interfaces allow browsing over options and this
useful form feels just as valid in textual applications.
Old-fashioned menu-driven applications force us in and
out of contexts more slowly, but it is not really that
different from using a mouse to look around what
options exist in a menu.  Interestingly, even command
lines support such browsing to some degree, through
commandline completion and help dialogs.

There reasoning followed here is that menus can be
useful drivers for many different kinds of user
interaction, and that ASCII can provide the
information to enable it.


## Adding Streams for Description

First, the mulTTY enables us to define streams which
then carry data independently from such streams as
`stdio` or `stdin`.  We can use those well.

The newly added `stdctl` for example, may be used to
not only send POSIX signals from a remote interface,
but it may also make changes to program internals by
sending commands and setting variables.

We can also add output streams to send information
that helps to describe and structure the `stdctl`
input.  Since it is all very terminallish in style,
it would still be highly textual yet enable more
automation because it is not all written out in
text but instead with ASCII markers in an streams
that have a clear name and interpretation.

The menu structures that we propose here can be
sent out by a program using a new `stdopt` stream.
It may be requested over the `stdctl` stream
using a simple command line `enable stdctl`.


## Elementary Data Structures

At the risk of re-inventing common standards, we
set forth to see what we can describe.

At the outer level, we separate data structures
with the file separator control code `<FS>` in a
loose manner:

  * empty files are permitted, so `<FS>` may be
    used before and after a file, not just in
    between them; it is actually pleasant to
    use it as a terminator to mark that a
    definition is finished
  * files apply to the current program, and do
    not impact other programs at all
  * files start with `<SOH>` naming and repeating
    the same name redefines it dynamically
  * to remove a file, just send `<SOH>`, its name
    and an immediate `<FS>` to indicate no further
    data for the menu
  * file names may use `<US>` between a
    machine-readable and human-friendly form;
    in this case file overwriting is defined
    by the part before the `<US>` so descriptions
    can change without modifying a machine
    identity
  * every file defines one data structure,
    and may files by name
  * unspecified file names should be treated
    as literal words to use, like the three
    words in `ip addr add 123.45.67.89` that
    specify an action, but not data.
  * the initial menu structure to address would
    have an empty file name.  This may be updated
    to reflect changes in menu options, but such
    may also be done further down.


### Strings in ASCII

It is not difficult to encode strings in ASCII
but we should care about control codes.  If ever
there could be confusion to interpret a character
in a string as an unintended control code, it
should be escaped with `<DLE>`.  This is not needed
for `<LF>` and `<ESC>`, but it is useful for the
other control codes used here: `<TAB>`, `<SOH>`,
`<US>`, `<FS>`, `<GS>`, `<RS>`, `<HT>` and of course the
other codes defined in the
[mulTTY specification](ASCII.MD).

Any strings mentioned in menu structures may
have an unescaped `<US>` between a machine-readable
and human-readable form.  Having no `<US>` means
that the data is only for human consumption, but
should be treated as a literal string in commands.


### Lists and Records in ASCII

The representation of lists and records is the same;
they simply sum up values.  Values that count as a
reference to a type should use the `<US>` between the
reference and a description thereof.  If the type
reference does not resolve, it is supposed to be a
literal word to mention.

The values in a list or record are separated by a
record separator `<RS>` control code.

When a last element in a list has too many options,
it may be entered as `<HT>` or a form holding `<HT>`
and thereby suggest to ask for completion if that
option is considered.  This may lead to any number
of additional list elements, no added element
being one possible option.


### Lists of Alternatives in ASCII

When a command can take on multiple forms, it uses
the group separator `<GS>` control code between the
options.  The part before the first separator is
always used and the part after the various `<GS>`
then provide alternative completions.  This may
include an empty completion, so be careful not to
treat `<GS>` as a terminator.  If this is done, a
sequence `<GS>`,`<FS>` could be interpreted as an
empty final form.


## Data Completion

To complete a field, its definition is sent in
part to `stdctl` with a `<HT>` control at the end
of the computer-friendly name.

This control may well have been included in the
original form, even at its very beginning, to
indicate the possibility of asking for it.  The
`<HT>` control would then precede the `<US>` control.
The response would use the name with the `<HT>`
control included in the name to indicate that it
is a completion table.  It may also add a few
characters after `<HT>` that are certain to go
there, and the remaining list contains all
further extensions excluding the already-found
naming, so the list might include an empty string.
An empty list on the other hand, indicates that
no completions could be found.

This mechanism enables interaction to avoid
sending very long lists, but rather to reduce the
traffic by stepping through a few interactions.

To request completion, we send `complete ` followed
by the as-yet-incomplete command and thereby
request more refined completions than we had.  By
sending the complete command, we allow any completions
to be performed in their actual context, which matches
the level of refinement of common completion code.


## Building Interactive Menus

We can now create menus with several kinds of
user interaction.

  * A GUI design may construct a menu that supports
    browsing and mousing.  The descriptions would be
    used in various places, to end up in instructions
    in a machine-friendly alternative form.

  * A TUI design could follow the same structure, but
    it might also present one context at a time, and
    build up the intention with more interaction.

  * A commandline interface could use completion and
    build up the list of possible commands.  Even the
    initially empty commandline can have completion,
    by listing the features in the empty-named file.


## TODO: Entry of Elementary Data

We have not yet provided for elements of data entry,
such as commandline arguments: file names, user identity,
flag bit patterns, and plain input strings and numbers.
A good form in several forms of user interface might
be a form that provides some support for the kinds of
things to enter.

Identities are one class of thing, and we might use
that in several places in a normal ASCII flow, perhaps
with an `<SOH>` naming prefix so it can be dragged and
dropped without inaccuracies in spelling or bounding,
or simply presenting options that have been passed
during the interaction.

Another class of thing is plain entered data.  Syntax
is commonly checked by a command, so that is no cause
for worry.  We should however indicate the need for
such a field in the command completion lists.


### TODO: Popups or Modal Windows

We have no mechanism for revealing information in a
popup manner, overtaking the current interaction for
at least the scope of one program.  It could be nice
to structure feedback inasfar as it has structure,
and leave any rendering of it to the user interface.
It may be helpful if the previous command would need
to be modified, to be able to bring its filled-out
form back up for modification.

