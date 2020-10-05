#
# "dotty" wraps around a single command and adds mulTTY streams
#
add_executable (dotty
	dotty.c
)
target_link_libraries (dotty multty)

#
# "pretty" render mulTTY streams with more pleasing graphics
#
add_executable (pretty
	pretty.c
)
target_link_libraries (dotty multty)

