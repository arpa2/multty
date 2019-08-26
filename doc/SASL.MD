# Terminal Support for SASL

> *Having streams with names that indicate their
> purposes, we can now work on better authentication
> exchanges.  We explain how we can work SASL into
> a multiplexed terminal flow.*

Have you ever seen how login is automated in old
scripts?  It grabs strings like `ogin:` and
`assword:` and inserts characters in response to it.
The missing first character is to avoid case sensitivity.
And TTY technology still has no support for doing this
with more structure.  One result is that we are still
facing the challenge to enter a literal password, and
to do this over and over again.  Let's get over that.

Having automation-friendly support for authentication
means that we can popup a question for credentials if
and when we need it, of which the start of a session
is just an example.  We might offer to change identity,
confirm an identity to match a more stringent ACL rule,
and so on.  The TTY can use more flexibility, certainly
to accommodate modern concepts like containers.


## Adding Streams for SASL

This is an enhancement to the interpretation of a stream
of textual data on `sasl` from client to server and back,
using control codes that are not otherwise used.

The codes defined below might have arisen on the normal
`stdin` and `stdout` flows, and it is still possible to
capture them from those for passing on as `sasl` streams.
At this time however, we are interpreting and possibly
causing conflicts of opinion on meaning.  Though we
would not know which, in practice, it is impossible to
know for sure, so we shall not take that view.


## SASL in Secure Contexts

Note that SASL is not designed to be passed in the
clear, for certain reasons:

  * Backward-compatible SASL mechanisms can be weak,
    supporting even the replay of captured traffic

  * SASL mechanisms may leak identity information and
    so disadvantage privacy and perhaps even security

  * There might be man-in-the-middle attacks that modify
    authentication attempts while in transit

  * SASL Authentication and the actual traffic using
    the gathered access rights must be tightly bound
    together

One way of doing this is through local system control,
if the entire SASL exchange stays on a local system.
A popular mechanism for many environments is to have
SASL embedded in a TLS protocol.  Yet another manner
that could make sense for terminal sessions is to use
it under the OpenSSH protocol.

Some of these mechanisms are capable of relaying their
own identity information.  A computer account with its
controlling domain, an authenticating TLS client and an
authenticating OpenSSH client all offer identity, though
it is not enforced in all cases.  The general answer to
use that in SASL is the `EXTERNAL` mechanism, defined to
look at whatever identity information the context can
supply.  Servers able to make such a lookup could offer
it and clients interested in saving themselves the extra
work (or desiring to separate identity concerns to an
outer layer) could then choose it.


## Client probing: Asking the server for SASL

The client may probe the server by sending an empty
message to its `sasl` stream.  If the server can
understand this stream, it may initiate SASL with
the client.  This may be done in any phase of the
connection.

An empty message is just `<ETB>`, but if the client
wants to hint at a realm that it finds interesting,
or has perhaps used before, it may send an `<SOH>`
naming prefix before the `<ETB>`.

The probe as well as a possible `<SOH>` name prefix
are just hints to the server.  It may deal with it
in any way it wants, including ignoring it or
answering with a different kind of SASL setup.  The
client must therefore not assume that one probe
having failed implies that all other probes would
also fail.  It should feel free to try any probes
that it deems likely.


## Setup phase: Stop/Start SASL Operation

The server usually requests authentication via SASL,
and it does this by sending a list of mechanism names
that it supports.  These names are uppercase and short,
and they do not contain a space.  Many protocols send
the mechanism list with space to separate the elements,
and so shall we.  We end the list with `<ETB>`.

The mechanism list needs to be started, however.  We
do this by prefixing it with `<ENQ><CAN>` which can
be interpreted as a reset of security status and a
fresh start of a SASL session by an equiry into the
client's ability to authenticate.

The server may also send `<CAN>` to retract previous
authentication.  To completely ban a client, it may
send the form `<ENQ><CAN><ETB>`, which explicitly
lists no SASL mechanism names for renewed SASL
authentication.

When the server wants to distinguish sessions, such
as for different secure realms that independently
request login, it may prefix `<SOH>` style naming
to the `<CAN>` character.


## Teardown phase: Cancel SASL Operation

The client may cancel any SASL exchange in progress,
either before or during negotiation or afterwards,
by sending `<CAN><ETB>`, prefixed with an `<SOH>`
name if the server used it.

The server may cancel any SASL exchange in progress,
either before or during negotiation or afterwards,
by sending `<ENQ><CAN><ETB>`, prefixed with an `<SOH>`
name if it used that before.

When a session does not exist, a `<CAN>` has probably
been sent superfluously and it must be silently ignored.


## Passing SASL Tokens over ASCII

SASL Tokens are general binary strings, and may include
control codes that are meaningful to ASCII.  One of these
is `<NUL>` that may be added or removed ad libitum.
These codes can be protected with `<DLE>` escaping.
The fact that this alters the number of bytes being
passed hardly matters, as this is variable anyway in
SASL.  When reading back SASL sequences, any inserted
`<NUL>` must not be considered part of the SASL Token.

When a SASL Token is sent, we follow it with `<US>`.  If
no token is sent, which differs from sending an empty
SASL token, we send nothing and leave it to the context
to decide that the SASL Token was absent.

Note that the `<US>` character following a SASL Token
is not subjected to `<DLE>` escaping, which is only
used for the internals of the SASL Token.  This means
that `<US>` serves as an end marker to the SASL Token.


## Server Challenges

A server challenge occurs after mechanism selection
has taken place.

A server challenge can use an `<SOH>` naming prefix
to set it aside from other challenges it may pose.
If it is sending a further challenge to a client
response which contained an `<SOH>` naming prefix
then it should clone that here; the server is only
free to choose about `<SOH>` name prefixing during
its initial challenge.

The actual server challenge starts after the optional
`<SOH>` name prefix, with the `<ENQ>` character.

It may then pass a SASL Token if the SASL mechanism
requires one at that point.  To be able to see if
one is included, the following character cannot
be `<US>`, which will be assured below.

If the server knows the outcome it may also report
`<ACK>` or `<NAK>` to relay the SASL exchange result
to have succeeded or failed.

An optional extra token is possible according to
SASL, though it does not seem to be used at all
in practice.  But an extra token might now be
inserted, which according to SASL may only be done
after `<ACK>` but not `<NAK>` or when no status
is ready to be relayed yet.

The server challenge ends with an `<ETB>` code.


## Client Response

The first client response differs from any further
ones by selecting a mechanism from the list offered
by the server.

When the server prefixed its mechanism list message
with an `<SOH>` prefix, the client should send that
that same `<SOH>` name prefix before each of its
client response messages.

Client messages never start with an `<ENQ>`
character, which makes it easy to distinguish the
client and server messages.  As a result, it is
even possible to use bidirectional SASL or, as a
practical consequence, the same stream name `sasl`
can be used in both directions.

During the initial response, the client must choose
a mechanism from the list sent by the server.
This is sent as the machinism name followed by a
space.  This prefix must not be included in any
client message but the initial response.

The client may send a SASL Token if the mechanism
needs this.  For instance, a server-initiated
mechanism may not need it in the initial client
message.  

The client response ends with a `<ETB>` code.


## Authorisation follows SASL Authentication

SASL authenticates a user, possibly under an
authorisation identity, but all that the server
then knows is who it is dealing with.  Though a
specific context could provide hard-wired access
rights, there is a lot to be said for an ACL to
specify more flexible facilities:

  * Groups may share resources, but not all.
  * Roles may assign workers the access to certain
    resources, for the duration of their work.
  * You may be using aliases to separate types of
    work and to control the access required per task.
  * External parties may not have local accounts,
    but still need to be granted access.
  * Temporary sessions may be shared for the duration
    of a project or even just a phone call.  Ever
    wanted to share a terminal session with a friend
    to see how their code was behaving?

ARPA2 facilitates these things with a general
[arpa2service](https://github.com/arpa2/libarpa2service)
library, including central control through either
an `arpa2acl` shell or similar JSON service,
stored in LDAP and immediately distributed to
subscribing services.  This is part of the much
larger design of an
[InternetWide Architecture](http://internetwide.org/blog/2016/06/24/iwo-phases.html)
intended for shared identities that we can use
with realm crossover to gain access elsewhere.
One of the crossover mechanisms proposed is based
on SASL, and named SXOVER.


## Working Phase: Authorisation Feedback

When a server wants to indicate that a client is not
authorised for a certain action, it can send `<NAK>`,
possibly prefixed with `<SOH>` naming.

When a server wants to make explicit that a client
is (now) authorised for a certain action, it can send
`<ACK>`, possible prefixed with `<SOH>` naming.  This
may be done at any time.  Clients may learn from this
that a failed attempt in the past is now possible.

The `<SOH>` naming is intended to connect such events
caused by authorisation access.
Names are intended to be humanly readable, though a
`<US>` can be used between an unreadble unique textual
value and a readable human string.

One or more of these notifications may be concatenated.
The last one should be followed with `<ETB>`.

Note that this is different from the `<ACK>` or `<NAK>`
included in the last SASL response from the server.
These indicate *authentication* success or failure,
not *authorisation* success or failure.  Syntactically,
the former only occurs between `<ENQ>` and `<ETB>` and
the latter only occurs outside it.


## Example: SASL ANONYMOUS with Authorisation Updates

The server decides it wants to ask the client to
authentication, but it does offer ANONYMOUS for a
client who is intent on using only public resources.

These examples include an `<SO>` shift to the `sasl`
stream.  When done, it sends another `<SO>` to
return to `stdin` or `stdout`.  Either or both of
these shifts may be unnecessary depending on context.

The server initiates the SASL procedure.

```
S->C: <SOH>sasl<SO><ENQ><CAN>SXOVER ANONYMOUS<ETB><SO>
```

A client that is unaware of SASL or mulTTY streams
is likely to ignore the special characters and it
may (incorrectly) dump the ASCII content:

```
saslSXOVER ANONYMOUS
```

This would be harmless, at best it could raise some
questions with users and make them discover the
extra option being offered.  Let's assume we have a
potent client, which chooses ANONYMOUS to send a
token that holds a tracing name which happens to be
readable as plaintext:

```
C->S: <SOH>sasl<SO>ANONYMOUS bakker@orvelte.nep<US><ETB><SO>
```

The server may explicitly accept, though this is not
always done with the ANONYMOUS method.  It does not
send a SASL Token at all, which is why there is no
`<US>` to follow an (empty) SASL Token:

```
S->C: <SOH>sasl<SO><ENQ><ACK><ETB><SO>
```

At any time later, the SASL server may discover that
this user is granted access to a particular service,
and it could report that to the SASL client:

```
S->C: <SOH>sasl<SO><SOH>webtty<ACK><ETB><SO>
```

Perhaps another resource gets withdrawn due to an
expiration or ACL change:

```
S->C: <SOH>sasl<SO><SOH>ldap<NAK><ETB><SO>
```

## Example: SASL SCRAM

We follow the example in
[Section 5 of RFC 5802](https://tools.ietf.org/html/rfc5802#section-5)
with the remark that the base64 coding used here is part of this
specific SASL mechanism, not of its ASCII embedding, but it does
simplify token handling and representation to not have to use
`<DLE>` escaping.

```
S->C: <SOH>sasl<SO><ENQ><CAN>SCRAM SXOVER ANONYMOUS<ETB><SO>
C->S: <SOH>sasl<SO>SCRAM n,,n=user,r=fyko+d2lbbFgONRv9qkxdawL<US><ETB><SO>
S->C: <SOH>sasl<SO><ENQ>r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,
      s=QSXCR+Q6sek8bf92, i=4096<US><ETB><SO>
C->S: <SOH>sasl<SO>c=biws,r=fyko+d2lbbFgONRv9qkxdawL3rfcNHYJY1ZVvWVs7j,
      p=v0X8v3Bz2T0CJGbJQyF0X+HI4Ts=<US><ETB><SO>
S->C: <SOH>sasl<SO><ENQ>v=rmF9pqV8S7suAoZWja4dJRkFsKQ=<US><ACK><ETB><SO>
```

To understand the ASCII inserts:

  * `<SOH>sasl<SO>` in the beginning switches to the `sasl` stream.
  * `<SO>` at the end switches back to the default stream, `stdin` or `stdout`.
  * `<ETB>` is used to mark the end of SASL messages independently of `<SO>` to switch streams.
  * `<ENQ>` is always used as a starter by the server and never by the client.
  * `<ENQ><CAN>` is the start of a method list, always sent by the server.
  * `SCRAM ` starts the initial response, before the SASL Token `n,,n=user,xxx` is sent.
  * `<US>` terminates a SASL Token and indicates that a SASL Token was sent.
  * `<ENQ>xxx<ACK>xxx<ETB>` indicates successful authentication, but says nothing about authorisation.

