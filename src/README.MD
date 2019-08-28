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

