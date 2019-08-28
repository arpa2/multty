# Demo Programs

**dotty.c**
Can be run around a program, and will combine this child program's
`stdout` and `stderr` streams into the parent's `stdout`, but with
stream switching commands.  Note: Because it treats the child output
as binary, it will escape all non-printable characters, even a mere
newline, not knowing what the transmission channel might otherwise
do to it.  For `dotty`, that's probably good in general, but for
the library it is a hint that we should make wrappers with different
escapes; for instance, to send ASCII with a length.

