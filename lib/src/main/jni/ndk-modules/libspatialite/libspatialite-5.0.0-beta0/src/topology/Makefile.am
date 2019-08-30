
AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/headers -I.

noinst_HEADERS = topology_private.h \
	network_private.h \
	lwn_network.h \
	lwn_network_private.h
noinst_LTLIBRARIES = libtopology.la topology.la

libtopology_la_SOURCES = gaia_topology.c \
	gaia_auxtopo.c \
	gaia_auxtopo_table.c \
	gaia_topostmts.c \
	topo_callbacks.c \
	lwn_network.c \
	gaia_network.c \
	gaia_auxnet.c \
	gaia_netstmts.c \
	net_callbacks.c

topology_la_SOURCES = gaia_topology.c \
	gaia_auxtopo.c \
	gaia_auxtopo_table.c \
	gaia_topostmts.c \
	topo_callbacks.c \
	lwn_network.c \
	gaia_network.c \
	gaia_auxnet.c \
	gaia_netstmts.c \
	net_callbacks.c

topology_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
topology_la_CPPFLAGS += -I$(top_srcdir)/src/headers -I.
topology_la_CPPFLAGS += -DLOADABLE_EXTENSION
topology_la_LDFLAGS = -module
topology_la_LIBTOOLFLAGS = --tag=disable-static

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
