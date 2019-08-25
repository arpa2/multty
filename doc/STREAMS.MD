# Stream Name Registry

> *This is a list of stream names that we have defined.
> It would be useful to migrate this from our version
> management system to a standards body, if this grows.*


## Output Stream Names

  * `stdout` is the default output stream.  It holds
    the classical standard output streams from C and
    POSIX environments.

  * `stderr` serves as the classical error output stream
    from C and POSIX environments.

  * `sasl` is output by a SASL client and server.

  * `sftp-c2s` is output by an SFTP client.

  * `sftp-s2c` is output by an SFTP server.

  * `zmodem` is output by both ZMODEM sender and receiver.
    The protocol itself indicates what role it has.  By
    declaring this stream, even a passive ZMODEM player
    can indicate its willingness to accept the protocol.

  * `stdopt` is an experimental stream that can indicate
    what menus of command options are available from a
    program.


## Input Steam Names

  * `stdin` is the default input stream.  It holds the
    classical standard input stream from C and POSIX
    environments.

  * `stdctl` serves as an extension to the classical
    input stream for C and POSIX environments, taking
    in dynamic changes to the program at hand.  Some
    commands may be processed around the program, some
    may be passed over to the command for processing.

  * `sasl` is inut by a SASL client and server.

  * `sftp-c2s` is input by an SFTP server.

  * `sftp-s2c` is input by an SFTP client.

  * `zmodem` is input by both ZMODEM sender and receiver.
    It informs them what role the other side intends to
    play.


## IANA Standard Names

We define `iana.service.` and `iana.media.` prefixes
for use with [IANA Service Names](IANA.MD) and with
[IANA Media Types](IANA.MD).

