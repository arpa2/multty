# Better Terminals for Containers

> *MulTTY is a pleasant extension for container systems,
> both with ease of use and security in mind.*

Container technology is a broad term, referring to many
similar systems that chiefly developed under Linux.  What
we mean by the term is mostly the work of the
[Open Container Initiative](http://opencontainers.org)
which has come up with standards for an exchangeable
container format and operational procedures that should
work on containers to direct their life cycle.


## Better Control from MulTTY

**virtuals.**
On a physical desktop, we usually have virtual terminals,
or a graphical interface to the same effect.  It is not
normal for light-weight containers to offer graphical
support, nor do they offer virtual terminals.  The only
way to do this is adding an SSH service, which however
calls for networking infrastructure of a kind that the
container may be trying to steer away from.

**init.**
Using mulTTY it is possible to exploit the single TTY
interface that is commonly provided to access more than
just one session.  One possible use case of this is to
provide daemons each with their own TTY output.  This
can remove the need for logging, and simplify the whole
`init` flow that is usually kept light in a container
through
[dumb-init](https://engineeringblog.yelp.com/2016/01/dumb-init-an-init-for-docker.html)
or when multiple processes should be run through
[runit](http://kchard.github.io/runit-quickstart/)
simplifications.  The latter also comes with BusyBox,
which also offers its own minimal `/sbin/init` program.

**control.**
Another thing proposed by mulTTY is a `stdctl` channel
for programs.  This channel can be used to start and
stop individual programs.  This integrates well with
`init` style systems and is certainly better than
editing a script and sending a `kill -HUP 1` signal.
It is possible for the `init` program to recognise
`stdctl` access to its direct child programs, and
regulate it on their behalf if so required.

**pausing.**
Control commands are often sent to `init` itself,
with an impact on the entire system.  This lends
itself for the type of control that containers mut
all support.  At the very least, control commands of
interest are `start`, `pause`, `resume` and `stop`,
which are important in the flow of any container.
Such commands would be trivial to implement when
done through a `stdctl` channel.  Moreover, it may
reduce the dependency on runtime tooling.  Terminal
programs would do well to not send XON and XOFF
commands directly, but locally map them to `stdctl`
commands `pause` and `resume`.  Similar mappings
could be made for other commonly found control keys.

**forking.**
It would be possible to listen to a mulTTY connection
and fork new connections when new programs start.
Due to the hierarchical nature of programs and the
fact that they have a name, some of this may be
prescribed in simple rules.  If you ever have two
programs that both claim input from `stdin` you can
fork a terminal to separate them.

**rendering.**
Since streams are clearly differentiated, they can
also be directed to different windows, tabs or
scroll regions within a display.  Moreover, it is
possible to assign colours to the various streams,
which is much more reliable when done locally and
not at the point of sending but before mixing with
other sources due to `2>&1` and similar constructs.


## Better Security from MulTTY

The ability to render with more clarity is really
helpful in terms of security.  Making it much more
difficult to misread merged streams, making login
easily distinguishable and ensuring that input
only goes to the intended channel are all helpful.

**stream.**
Thanks to the separation of streams through
multiplexing, mulTTY helps to make login interactions
stand out like a sore.  No more grabbing for `ogin:`
or `assword:` strings and guessing that this is the
moment to enter sensitive information.

**separation.**
The use of a separate stream for login allows the
repeated login after expiration, or when attempting
to do something that is too sensitive for the level
of authentication currently reached.  This may even
allow anonymous sessions without login, but popups
for authentication when moving to more sensitive
control.  This level of refinement is basically
impossible with plain TTY operations.

**authentication.**
Instead of the standard password flow, mulTTY also
introduces SASL.  This is a generally used protocol
to authenticate access, and its identities may be
used for authorisation, either at the point of
login or, perhaps throught a `REMOTE_USER`
environment variable, in a program further down.

**strength.**
SASL offers a choice of mechanisms, including the
same weak password-based mechanisms that nobody
should use anymore.  The TTY however is held back
in that sense, for assuming that a user must type
things literally.  This is certainly not true
anymore for all modern uses of the TTY interface,
most certainly not for its uses in containers.
Stronger mechanisms achieve higher bit strengths
for authentication.

**sso.**
SASL can be part of a single-signon scheme, when
the right mechanisms are used.  Kerberos and
SAML2 spring to mind, two popular technologies
in areas of federation or other collaborative
infrastructures.

**authorisation.**
Authorisation assumes an established identity,
such as those assured through a SASL exchange.
The next step then is to look for service access
in an ACL.  The lookup may differ per service.
We provide an API for flexible ACLs in our
[ARPA2 service library](https://github.com/arpa2/libarpa2service)
with centralised management options for dynamic
downloads and updates of such ACLs, but this may
also be done in the environment and stored in a
simple key/value database in the container.

**decentral.**
Even though SASL has a ring of database passwords
to it, this is in fact dependent on the mechanisms
supported.  Furthermore, we are working on an
[SXOVER mechanism](https://tools.ietf.org/html/draft-vanrein-diameter-sasl-00)
that offloads the authentication to a server
responsible for the domain part of the username
(after an `@` symbol).  Provided that your
network's DNS resolver implements DNSSEC this can
be easy to use, without configuration on the
container.  This effectively decentralises accounts
to arbitrary visitors.  Mind you, for authenticating
them, not necessarily leading to authorisation!


## Better Reachability from MulTTY

Another point of concern with containers is how to
get to their TTY at all.  This is an issue that
is independently problematic.

**transport.**
Nothing in mulTTY makes assumptions about the
transport used, except that it should be safe
to pass SASL traffic and any further interaction.
A local connection within a system is usually
workable as well as OpenSSH connections or even
TLS connections.  Inasfar as we want somthing on
a TCP port, we might use port 23 (with `<DLE>`
escaping of code 0xff because Telnet uses it)
over a direct TCP connection.

**detachment.**
The de facto implementation of Open Containers is
RunC, and more advanced tools tend to build on top
of it.  RunC distinguishes
[terminal modes](https://github.com/opencontainers/runc/blob/master/docs/terminals.md)
and ways of sharing them to the user.  The simplest
uses involve grabbing the console that was used to
start the container, possibly even in the safer
and advised detached mode.  Especially in this mode,
which does not share a `/dev/pts` terminal but a
mere socket handle, it is perfectly possible to
pickup the socket and start a session (possibly
detached) somewhere in a background.

**reception.**
The demonstration tool for picking up on detached
TTYs is
[recvtty](https://github.com/opencontainers/runc/blob/master/contrib/cmd/recvtty/recvtty.go)
which really just does the minimum it needs to and is
[missing information](https://github.com/opencontainers/runc/issues/2110)
that will hopefully be added in the future.
A better `recvtty` could pull in any newly started
TTY and actually implement the mode note found in
`recvtty` for practical reasons, namely to accept
multiple TTY sessions, and combine them as different
programs in a mulTTY session.  This mulTTY session
can be picked up by a user interface and the start
of a new program can be used to popup windows.
Programs capable of doing such things could include
Screen, Tmux and TWIN.  Each of these support a
detached mode where the screen does not require an
active observer.  Moreover, they also welcome the
attachment of multiple displays that get to see the
same outputs, so it can be accessed from various
locations, even at the same time.

**listening.**
Another possibility for a mulTTY receiver that accepts
TTY sessions from independent containers is a network
service, perhaps called `netty`, that listens on an
SCTP socket where it offers the terminals over the
(possibly multiple) streams negotiated during SCTP
setup.

**intuition.**
The proposed mulTTY service would mention each of the
created containers as a named program.  When sufficient
handles are provided with the submission, it might
attach `stdctl` streams to steer the program in and
out of a pause.  Window closing might lead to shutdown,
and new windows may start a container, and so on.
Containers can be really intuitive.

**nesting.**
The programs in mulTTY are nested, so they can allow
multiplexers to wrap around other multiplexers.  This
shows up in this use case too.  The outer programs
may be containers, or the may be pods that have
containers nested inside them.  At some point, a
container's `init` may be accessible and this may
be yet another multiplexer that starts and restarts
programs that it was told to care for.  The nested
nature means that each of these programs can have
their own streams, without risk of interference
with the other streams.  The mulTTY nesting of
programs and streams completely separates and
routes the traffic, even when just one handle is
used to get started.

