# all: dotty pretty
# all: dotty
all: colour-test

dotty: dotty.c
	gcc -ggdb -I../include -L ../lib -o $@ $< -lmultty

pretty: pretty.c
	gcc -ggdb -I../include -L ../lib -o $@ $< -lmultty

colour.h: colour-gentab.py
	./colour-gentab.py > $@

colour-test: colour.c
	gcc -ggdb -o $@ $<
