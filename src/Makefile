all: dotty pretty

dotty: dotty.c
	gcc -DUSE_RELATIVE_STDERR -o $@ $<

pretty: pretty.c
	gcc -o $@ $<
