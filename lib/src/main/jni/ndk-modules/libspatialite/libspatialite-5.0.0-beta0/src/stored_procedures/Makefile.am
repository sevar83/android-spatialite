
AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/headers

noinst_LTLIBRARIES = libstored_procedures.la stored_procedures.la

libstored_procedures_la_SOURCES = stored_procedures.c

stored_procedures_la_SOURCES = stored_procedures.c

stored_procedures_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
stored_procedures_la_CPPFLAGS += -I$(top_srcdir)/src/headers -I.
stored_procedures_la_CPPFLAGS += -DLOADABLE_EXTENSION
stored_procedures_la_LDFLAGS = -module
stored_procedures_la_LIBTOOLFLAGS = --tag=disable-static

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
