ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = dist-zip foreign

EXTRA_DIST = makefile.vc nmake.opt \
	makefile64.vc nmake64.opt \
	makefile_mod.vc nmake_mod.opt \
	makefile_mod64.vc nmake_mod64.opt \
	Android_3.0.2.mk Android_4.3.0.mk \
	spatialite-4.3.0.mk Android_4.4.0.mk spatialite-4.4.0.mk \
	mainpage.doxy README.coverage spatialite-sql-latest.html \
	config-msvc.h

if ENABLE_EXAMPLES
EXAMPLES = examples
endif

SUBDIRS = src test $(EXAMPLES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = spatialite.pc

coverage-init:
	lcov --directory src --capture --initial --output-file libspatialite_cov.info


coverage::
	lcov --rc lcov_branch_coverage=1 --directory src --output-file libspatialite_cov.info --capture
	lcov --rc lcov_branch_coverage=1 --remove libspatialite_cov.info "*/src/gaiageo/*Lexer.l" --output-file libspatialite_cov.info
	genhtml --rc lcov_branch_coverage=1 -o covresults libspatialite_cov.info

MOSTLYCLEANFILES = libspatialite_cov.info
