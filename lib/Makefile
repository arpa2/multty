all: libmultty.so libmulttyplex.so

SOURCES+=close.c
SOURCES+=escape.c
SOURCES+=flush.c
SOURCES+=open.c
SOURCES+=puts.c
SOURCES+=write.c
SOURCES+=vout.c

SOURCES_PLEX+=progmkid.c
SOURCES_PLEX+=progfind.c
SOURCES_PLEX+=proghave.c
SOURCES_PLEX+=progdrop.c
SOURCES_PLEX+=progvar.c
SOURCES_PLEX+=prograw.c
SOURCES_PLEX+=progswitch.c

libmultty.so: $(SOURCES)
	gcc -shared -I ../include -o $@ $(SOURCES)

libmulttyplex.so: $(SOURCES_PLEX)
	gcc -shared -I ../include -o $@ $(SOURCES_PLEX)

