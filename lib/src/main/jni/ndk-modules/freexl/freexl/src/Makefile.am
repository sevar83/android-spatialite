AM_CPPFLAGS = -I$(top_srcdir)/headers
AM_CPPFLAGS += -I$(top_srcdir)

lib_LTLIBRARIES = libfreexl.la 

libfreexl_la_SOURCES = freexl.c

libfreexl_la_LDFLAGS = -version-info 2:0:1 -no-undefined

libfreexl_la_LIBADD = -lm

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
