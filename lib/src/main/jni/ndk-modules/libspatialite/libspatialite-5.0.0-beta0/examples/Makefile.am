
if MODULE_ONLY
noinst_PROGRAMS = 
else
noinst_PROGRAMS = demo1 demo2 demo3 demo4 demo5
endif

AM_CFLAGS = -I@srcdir@/../src/headers 
AM_LDFLAGS = -L../src -lspatialite -lm $(GCOV_FLAGS)

LDADD = @LIBXML2_LIBS@

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = examples.doxy
