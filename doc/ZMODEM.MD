# ZMODEM is back!

> *In the BBS days, ZMODEM would be a nice protocol
> for uploading and downloading files over a TTY or
> serial modem connection.  It still is poweful.*

ZMODEM senders and receivers both initiate a protocol
with a control sequence.  This sequence is carefully
crafted to stand out like a sore in a normal TTY
connection, and not occur spontaneously.

This style of design works well if you control all the
protocols in use, such as the case of a given user dialing
in to a given BBS.  The BBS sets the tone, the user only
triggers it when he knows it is good.  This is not the
modern Internet in which any two parties want to be able
to collaborate, and where they might have different
settings.

The solution for modern protocols is to explicitly
identify options and negotiate them.  Naming schemes
are standardised and centralised, and as a result
extensions that are not known to the parties involved
are suppressed.  ZMODEM took care not to confuse the
programs it knew about, but you can only follow that
line of action with a handful of protocols.

The mulTTY proposal made here is different.  First,
it sits in a channel and escapes anything passing
through it that might be confusing.  This means it
never gets confused by other protocol content.
But it also helps other protocols by separating
content into streams and allowing applications to
select only those streams that they understand,
most likely already separated out from other
conventions.

In this context, ZMODEM can simply be a pair of
streams.  And indeed, we defined `zmodem-s2r` and
`zmodem-r2s` as the streams flowing from a sender
to a receiver, and back.

Interestingly, the signaling by ZMODEM can be
very useful.  When both arrive in a terminal,
regardless of whether it is graphical or textual
in nature, it can detect two signals that want
to collaborate, and connect them accordingly.

Consider your daily terminal use.  You login to
a few online systems, and at some point you need
to send a file from one such system to another.
With ZMODEM, you start `sz FILE` on one and `rz`
on the other.  With SFTP, you download from one
and upload to the other.  And you need to check
file access; you might need to remove root-only
visibility and add it back later.  And you need
to type file names manually, with their full
paths, with no command line completion.  It
really is much easier to use ZMODEM.

Using the mulTTY wrapper `dotty`, you might say

```
here$  dotty -i zmodem -o zmodem -- sz FILE
there$ dotty -i zmodem -o zmodem -- rz
```

but you guessed it, aliases or script wrappers
can simplify that to

```
here$  setty FILE
there$ retty
```

It may even be useful to convert these to real
programs, and permit a bit more user interaction
than possible with `rz` and `sz` that have to
claim the entire TTY in both directions under
the assumption of them being single streams.
