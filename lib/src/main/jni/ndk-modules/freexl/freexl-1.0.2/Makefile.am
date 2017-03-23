ACLOCAL_AMFLAGS = -I m4

SUBDIRS = headers src tests examples

EXTRA_DIST = makefile.vc nmake.opt \
	images/piazza.jpg images/piazza.eps mainpage.doxy config-msvc.h

AUTOMAKE_OPTIONS = dist-zip foreign

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = freexl.pc

coverage-init:
	lcov --directory src --capture --initial --output-file freexl_cov.info

coverage::
	lcov --rc lcov_branch_coverage=1 --directory src --output-file freexl_cov.info --capture
	genhtml --rc lcov_branch_coverage=1 -o covresults freexl_cov.info

MOSTLYCLEANFILES = freexl_cov.info 
