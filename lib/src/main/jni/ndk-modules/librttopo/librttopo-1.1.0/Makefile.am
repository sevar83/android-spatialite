ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src headers

EXTRA_DIST = \
  CREDITS \
  doc/BUILDING-ON-WINDOWS.md \
  doc/DATASTORES.md \
  doc/g_serialized.txt \
  makefile.vc \
  nmake.opt \
	README.md

AUTOMAKE_OPTIONS = dist-zip foreign

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = rttopo.pc

dist-hook: gen-ChangeLog

gen-ChangeLog:
	cd $(distdir) && owd=`pwd` && \
	cd ../ && cd $(srcdir) && \
  if test -d .git; then \
    git2cl > $${owd}/ChangeLog; \
  elif test -f ChangeLog; then \
    cp ChangeLog $${owd}/ChangeLog; \
	fi

