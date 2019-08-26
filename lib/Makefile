all: libmultty.so

SOURCES+=close.c
SOURCES+=escape.c
SOURCES+=flush.c
SOURCES+=open.c
SOURCES+=puts.c
SOURCES+=write.c

libmultty.so: $(SOURCES)
	gcc -shared -I ../include -o $@ $(SOURCES)

