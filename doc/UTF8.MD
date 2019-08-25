# MulTTY and UTF-8

> *MulTTY could not care less about those funny
> printable characters that people seem to want
> to pass around.  That includes UTF-8.*

Terminals may assign meaning to bytes with the
high bit set but, as with other character set
issues, MulTTY does not care.  It only cares
about a number of barely-used ASCII codes that
it uses to multiplex TTY traffic into programs
and streams, but not even the popular ones,
like `<ESC>` and `<LF>`.

This has a number of interesting results.  Most
importantly, it will happily pass these bytes of
interest to users into and out of multiplexes.
In terms of "ignorance is bliss" it is unaware
of terminals, their escape codes or character
sets and, as a consequence, of language specials
or UTF-8 awareness.

UTF-8 is passed byte by byte.  And that *is* of
some importance: if the multiplexing operation
has a reason to break up UTF-8 codes, it will do
this.  It won't care.  It simply assumes that
streams will be recombined at the other end
before they are tested for correct UTF-8 grammar.

Bear that in mind, please.  It is reasonable,
but might catch you by surprise.
