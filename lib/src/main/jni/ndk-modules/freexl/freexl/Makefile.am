ACLOCAL_AMFLAGS = -I m4

SUBDIRS = headers src tests examples

EXTRA_DIST = makefile.vc nmake.opt \
	images/piazza.jpg images/piazza.eps mainpage.doxy config-msvc.h

AUTOMAKE_OPTIONS = dist-zip

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = freexl.pc

coverage-init:
	lcov --directory . --capture --initial --output-file freexl_cov.info

coverage::
	lcov --directory . --output-file freexl_cov.info --capture
	genhtml -o covresults freexl_cov.info

MOSTLYCLEANFILES = freexl_cov.info 
