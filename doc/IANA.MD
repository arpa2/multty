# IANA Standards over ASCII Streams

> *Many protocols are written out in ASCII.  They are run
> over connections, but might also be just a kind of steam.
> For peer-to-peer networking, it could make a lot to have
> a delivery path for various protocols over a plain ASCII
> connection.*

The ASCII extensions proposed here support the definition
of any number of programs and streams.  This means that we
can have spontaneous incoming traffic, but we can also
initiate outgoing traffic, over one and the same protocol
channel.

All we need to do is find a way to name the programs and
channels for doing this.  Given standard names for streams,
it would not matter what programs started it and the general
nature of service can be exploited across the ASCII link.

We therefore specify a few naming conventions.  Plus, to
assure ourselves of proper separation between streams, we
require `<DLE>` escaping for all control codes that we use
for multiplexing.  Only "normal" controls characters need
no escapes.  Software passing these streams must be
vigilent to not pass attempts to violate this, as that
is likely to lead to break-ins.

Note that the use of `<DLE>` ultimately allows us to even
pass binary content.  The only requirement would be the
8-bit clean connection that is also assumed for UTF-8
connections and generally offered throughout the Internet
and even over normal serial connections.  It may be an
open issue whether binary protocols are sensible over a
terminal connection, but it is certainly possible.  With
no support for multiplexing as in mulTTY however, you
are likely to have a garbled display when something is
sent your way.


## IANA Services

IANA defines service names.  We declare a name space for
those names, to inherit their definitions and allow the
continued extensions made there.

Stream names start with `iana.service.` for this use.
This continues with:

  * `c2s-` for client-to-server traffic;
  * `s2c-` for server-to-client traffic;
  * `c2c-` for client-to-client traffic;
  * `s2s-` for server-to-server traffic;

then, the service name is appended in lowercase.
So, `iana.service.c2s-smtp` would be SMTP email service and
`iana.service.c2s-xmpp-server` could connect to a server
interface for XMPP chat.  Responses would travel back as
`iana.service.s2c-smtp` and `iana.service.s2c-xmpp-server`
streams, respectively.

**TODO:**  To support multiple streams, there may be an
extension with a number.

Since services tend to be interactive, we do not define
their further operation, or marking termination.  The
`<EM>` code can close a stream as always, and when an
application ends it might instead signal `<DC2>` to
close an entire program with the same effect; both cases
are one-ended shutdowns of which the receiver is merely
informed.

IANA maintains a formal
[Service Name Registry](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml)
that also defines protocols and port numbers for some.
Our interest is in the "Service Name" in the first column.


## IANA Media Types

IANA defines media types, also known widely as MIME types.
These mostly travel in one direction, even though they may trigger an application to be run
(such as for `application/pdf`) and even though they may lead to return traffic
(such as for `message/sip` or its contained `application/sdp`).  In general however,
there is a one-directional flow of data.

Such streams are named to start `iana.media.` and there is
no additional framing for directions.  The media type is
appended directly, in the precise casing as provided by
IANA.  So, we might have `iana.media.application/pdf` to name
an offer of a PDF document.

Note that streams can be ended with `<ETB>` or implicitly
by a complete program termination with `<EM>`.  The first form
is preferred because it allows a program to continue to run
and send another form of media later on, but the second should
also be observed.

IANA maintains a formal
[Media Type Registry](https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.xhtml).
Our interest is in the "Template" column, which we include here.

