# Stream Name Registry

> *This is a list of stream names that we have defined.
> It would be useful to migrate this from our version
> management system to a standards body, if this grows.*


## Output Stream Names

  * `stdout` is at `SO` and thereby the default stream.
    It holds the classical standard output stream from C
    and POSIX environments.

  * `stdout+` is at `SI` and may address another output
    mode, such as a red ribbon instead of black, or a
    G1 character set instead of G0.

  * `stderr` is at `SO`,`SO` and serves as the classical
    error output stream from C and POSIX environments.

  * `sasl-c2s` is output by a SASL client.

  * `sasl-s2c` is output by a SASL server.

  * `sftp-c2s` is output by an SFTP client.

  * `sftp-s2c` is output by an SFTP server.

  * `zmodem-s2r` is output by a ZMODEM sender.

  * `zmodem-r2s` is output by a ZMODEM receiver.

  * `stdopt` is an experimental stream that indicates
    what menus of command options are available from a
    program.


## Input Steam Names

  * `stdin` is at `SO` and thereby the default stream.
    It holds the classical standard input stream from C
    and POSIX environments.

  * `stdctl` is at `SO`,`SO` and serves as an extension
    to the classical input stream for C and POSIX
    environments, taking in dynamic changes to the
    program at hand.  Some commands may be processed
    around the program, some may be passed over to the
    command for processing.

  * `sasl-c2s` is input by a SASL server.

  * `sasl-s2c` is input by a SASL client.

  * `sftp-c2s` is input by an SFTP server.

  * `sftp-s2c` is input by an SFTP client.

  * `zmodem-r2s` is input by a ZMODEM sender.

  * `zmodem-s2r` is input by a ZMODEM receiver.


## IANA Standard Names

We define `iana.service.` and `iana.media.` prefixes
for use with [IANA Service Names](IANA.MD) and with
[IANA Media Types](IANA.MD).
