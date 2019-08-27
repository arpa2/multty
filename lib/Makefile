all: libmultty.so libmulttyplex.so

SOURCES+=close.c
SOURCES+=escape.c
SOURCES+=flush.c
SOURCES+=open.c
SOURCES+=puts.c
SOURCES+=write.c

SOURCES_PLEX+=progfind.c
SOURCES_PLEX+=proghave.c
SOURCES_PLEX+=progdrop.c

libmultty.so: $(SOURCES)
	gcc -shared -I ../include -o $@ $(SOURCES)

libmulttyplex.so: $(SOURCES_PLEX)
	gcc -shared -I ../include -o $@ $(SOURCES_PLEX)

