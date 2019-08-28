# Demo Programs

**dotty.c**
Can be wrapped around a program and will combine this child program's
`stdout` and `stderr` streams into the parent's `stdout`, but with
stream switching commands.  It treats the child's output as ASCII,
but will nonethess protect special characters.

An interesting run is this test command:

```
LD_LIBRARY_PATH=../lib/ ./dotty -- find ../src/ ../x ../lib/ | less
```

which prints the predictable things for `../src/` and `../lib/` but
shows lively how an error is generated for `../x` namely:

```
^Astderr^Nfind: ‘../x’: No such file or directory
^N../lib/
```

Here, `^A` is `<SOH>` or the start of a name; `^N` is `<SO>` to switch to
another output channel, either the prefixed `<SOH>` name or the default
channel `stdout` as on the second line shown.

The same line, when printed without `less` or `hexdump -C`, shows the
channel name but not the (usually ignored, even if that is not how
`<SOH>` should work) control codes used by mulTTY:

```
stderrfind: ‘../x’: No such file or directory
```

Another nice example is running `sz` on a stream named `zmodem`, which
is indeed defined for its purpose:

```
shell$ LD_LIBRARY_PATH=../lib ./dotty -o zmodem -- sz /etc/motd  | hexdump -C
00000000  01 7a 6d 6f 64 65 6d 0e  72 7a 0d 0e 01 7a 6d 6f  |.zmodem.rz...zmo|
00000010  64 65 6d 0e 2a 2a 10 58  42 30 30 30 30 30 30 30  |dem.**.XB0000000|
00000020  30 30 30 30 30 30 30 0d  8a 10 51 0e 01 7a 6d 6f  |0000000...Q..zmo|
00000030  64 65 6d 0e 2a 2a 10 58  42 30 30 30 30 30 30 30  |dem.**.XB0000000|
00000040  30 30 30 30 30 30 30 0d  8a 10 51 0e 01 7a 6d 6f  |0000000...Q..zmo|
00000050  64 65 6d 0e 10 58 10 58  10 58 10 58 10 58 10 58  |dem..X.X.X.X.X.X|
00000060  10 58 10 58 10 58 10 58  08 08 08 08 08 08 08 08  |.X.X.X.X........|
00000070  08 08 0e                                          |...|
00000073
```

Clearly distinguishable are the messages, though that is not a feature
but mostly a result of `sz` flushing out its buffers which are then
being processed separately by `dotty` as diversions from `stdout`.
The result is however quite satisfactory, namely traffic generated on
a stream of its own, dedicated in this case to a program with its own
stream definition, and returning to the default stream so that other
programs (of which neither `sz` nor `dotty` may be aware) can write
on the same output *atomically* so without ever mixing up streams.

The future will of course bring an option `-i zmodem` that can also
input form such a stream, and by that time we can start coupling to
`rz` for a ZMODEM transfer that can take place *concurrently with
typing and editing*.  Seems like we're not stuck in the BBS age
after all!  All thanks to a solid atomic basis for sending in the
`vout.c` library source file.

