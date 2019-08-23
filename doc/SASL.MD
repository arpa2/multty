# Terminal Support for SASL

> *Having streams with names that indicate their
> purposes, we can now work on better authentication
> exchanges.  We explain how we can work SASL into
> a multiplexed terminal flow.*

This is an enhancement to the interpretation of a stream
of textual data on `sasl-c2s` and `sasl-s2c` from client
to server and vice versa, using control codes that are not
otherwise used.  We specifically support SASL in both
directions.

The codes defined below might have arisen on the normal
`stdin` and `stdout` flows, and it is still possible to
capture them from those for passing on as `sascli` and
`saslsrv` flows.  At this time however, we are interpreting
and possibly causing conflicts of opinion on meaning.
Though we would not know which, in practice, it is
impossible to know for sure, so we shall not take
that view.


## SASL in Secure Contexts

Note that SASL is not designed to be passed in the
clear, for certain reasons:

  * SASL mechanisms may leak identity information and
    disadvantage privacy and perhaps even security

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


## Setup phase: Stop/Start SASL Operation

The server usually requests authentication via SASL,
and it does this by sending a list of mechanism names
that it supports.  These names are uppercase and short,
and they do not contain a space.  Many protocols send
the mechanism list with space to separate the elements,
and so shall we.  We end the list with `CR` but no `LF`
to allow primitive terminals to ignore and overwrite
these characters.

The mechanism list needs to be started, however.  We
do this by prefixing it with `ENQ` for an enquiry.
The server may do this as often as it wants to; for
instance, another place may require another kind of
authentication.

The server may also send `CAN` to retract previous
authentication.

When `ENQ` and `CAN` may reference various realms,
it is possible for the server to prefix `SOH` style
naming.


## Teardown phase: Cancel SASL Operation

Either party may cancel the SASL exchange in
proress by sending a `CAN` code, possibly prefixed
with a `SOH` naming prefix to select which exchange
is being cancelled.  When a session does not exist,
a `CAN` has probably been sent superfluously and it
must be silently ignored.


## Passing SASL Tokens over ASCII

SASL Tokens are general binary strings, including the
possibility of `NUL` and other control codes that ASCII
should leave alone.  These codes can be protected with
`DLE` escaping.  The fact that this alters the number
of bytes passed hardly matters, as this is variable
anyway in SASL.

We follow a SASL Token with `RS`, if one is sent.  If
no token is sent, which differs from sending an empty
SASL token, in which case we send nothing and leave it
to the context to decide that the SASL Token was absent.

Note that the `RS` characters are not subjected to
`DLE` escaping, which is only used for the internals
of the SASL Token.


## Server Challenges

A server challenge occurs after mechanism selection
has taken place.

A server challenge may use an `SOH` naming prefix
to set it aside from other challenges it may pose.
If it is sending a further challenge to a client
response which contained an `SOH` naming prefix
then it should clone that here; the server is only
free to choose about `SOH` name prefixing during
its initial challenge.

It may then pass a SASL Token if the SASL mechanism
requires one at that point.

If the server knows the outcome it may also report
`ACK` or `NAK` to relay the SASL exchange result
to have succeeded or failed.

An optional extra token is possible according to
SASL, though it does not seem to be used at all
in practice.  But an extra token might now be
inserted, which according to SASL may only be done
after `ACK` but not `NAK`.

The server challenge ends with a `GS` code.


## Client Response

The first client response differs from any further
ones by selecting a mechanism from the list offered
by the server.

When the server prefixed its `ENQ` with an `SOH`
prefix, the client should now send that same
`SOH` name prefix.

During the initial response, the client must choose
a mechanism from the list sent by the server.
This is sent as the machinism name followed by a
space.  This prefix must not be included in any
client message but the initial response.

The client may send a SASL Token if the mechanism
needs this.  For instance, a server-initiated
mechanism may not need it in the initial client
message.  

The client response ends with a `GS` code.


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
authorised for a certain action, it can send `NAK`,
possible prefixed with `SOH` naming.

When a server wants to make explicit that a client
is (now) authorised for a certain action, it can send
`ACK`, possible prefixed with `SOH` naming.  This
may be done at any time.  Clients may learn from this
that a failed attempt in the past is now possible.

The `SOH` naming is intended to connect such events
caused by authorisation access.
Names are intended to be humanly readable, though a
`US` can be used between an unreadble unique textual
value and a readable human string.

