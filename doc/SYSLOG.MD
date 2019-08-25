# System Logging over MulTTY

> *Log files are useful, but in containers they are not
> always pleasant.  They create state, and are locally
> stored instead of sent out.  Why not send logging to
> a terminal stream?*

Given that we have terminal streams, and can interact
with a `stdctl` interface to change behaviour of a
remote program, why not use it to filter logging and
have it delivered along with a deamon that we might
be running over a terminal?

Note that streams are specific to a program, and that
program uses relatively simple access calls to log the
things it finds noteworthy.  It would be quite possible
to share the log, given the proper access rights, with
the same party controlling the program.

For containers, this not only simpifies storage but
also the requirement of running a `syslogd` process
in every container.  And the lighter a container gets,
the better it scales.  And the simpler its management
gets, the cheaper their operations will be too.

The [syslog protocol](https://tools.ietf.org/html/rfc5424)
defines a simple ASCII format with a header containing
fields named PRI, VERSION, TIMESTAMP, HOSTNAME, APP-NAME,
PROCID, MSGID, STRUCTURED-DATA and finally the free-form
"body" follows, named MSG in the protocol specification.
All this is specified in ASCII and ready to pass into
its own stream.  It is useful to constrain what gets
sent using the `stdctl` interface, and the patterns for
doing this are well-known from configuration files.

The STRUCTURED-DATA is particularly interesting, as it
might give rise to connections between messages and
viewing only parts of a flow.  This may however be best
done on the client end; but it is a good example why it
could be interesting to have the log data available in
a sufficiently clever terminal program.  Especially when
identifiers turn into clickable links, highly potent
"logfile browsing" becomes a possibility and, even more
importantly, it can be shared with collegues who happen
to stare at the same terminal while discussing what is
going on.

In short, logging over MulTTY is at least an option.
