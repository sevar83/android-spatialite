
AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/headers

noinst_LTLIBRARIES = libvirtualtext.la virtualtext.la

libvirtualtext_la_SOURCES = virtualtext.c

virtualtext_la_SOURCES = virtualtext.c

virtualtext_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
virtualtext_la_CPPFLAGS += -I$(top_srcdir)/src/headers -I.
virtualtext_la_CPPFLAGS += -DLOADABLE_EXTENSION
virtualtext_la_LDFLAGS = -module
virtualtext_la_LIBTOOLFLAGS = --tag=disable-static

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
