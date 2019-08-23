# Access Control for MulTTY

> *We can perform excellent access control with the
> proposed changes in MulTTY.*

The ARPA2 project provides rich infrastructure and
efficient tooling for access control management.
This can be used to finely grant and revoke access
over TTY interfaces.  This is welcome in their daily
use for administrative access to servers and other
critical systems.

## Resources and Instances

The central concepts in MulTTY are programs and streams.
Defining access to those gives a good measure of control,
without being too fine.  Additional control may be used
for the `stdctl` meta-control channel by naming the
various commands that it can pass.

We define resources for the following parts of this
infrastructure:

  * TODO for programs
  * TODO for the streams of a program
  * TODO for commands sent to a program's `stdctl`

These UUID codes define general concepts.  Instances
are defined by adding string representations of their
various possibilities.

  * Program instances are defined by their command name
    in a given context.  (TODO)
  * Stream instances are defined as program instances,
    but add a stream name.
  * Commands are defined as program instances, but add
    a command name.

