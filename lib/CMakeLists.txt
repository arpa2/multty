
#TODO# add_com_err_table (multty SOURCE multty-errno)

if (WIN32)
        set(WSOCK_LIBRARIES wsock32 ws2_32)
else()
        set(WSOCK_LIBRARIES)
endif() 


library_pair (multty
	OUTPUT_NAME multty
	SOURCES
		close.c
		unescape.c
		escape.c
		flush.c
		outstr.c
		puts.c
		write.c
		vout.c
		vin.c
		# dispstrm.c
		mtystdin.c
		mtystdout.c
		mtystderr.c
	EXPORT mulTTY
)

library_pair (multtyplex
	OUTPUT_NAME multtyplex
	SOURCES
		progmkid.c
		progfind.c
		proghave.c
		progdrop.c
		progdescr.c
		progvar.c
		prograw.c
		progswitch.c
	EXPORT mulTTYplex
)

