
AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/headers -I.

noinst_LTLIBRARIES = libcutter.la cutter.la

libcutter_la_SOURCES = gaia_cutter.c

cutter_la_SOURCES = gaia_cutter.c

cutter_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
cutter_la_CPPFLAGS += -I$(top_srcdir)/src/headers -I.
cutter_la_CPPFLAGS += -DLOADABLE_EXTENSION
cutter_la_LDFLAGS = -module
cutter_la_LIBTOOLFLAGS = --tag=disable-static

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
