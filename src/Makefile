# all: dotty pretty
all: dotty

dotty: dotty.c
	gcc -ggdb -I../include -L ../lib -o $@ $< -lmultty

pretty: pretty.c
	gcc -ggdb -I../include -L ../lib -o $@ $< -lmultty
