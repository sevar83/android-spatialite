AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = headers \
	gaiaaux \
	gaiaexif \
	gaiageo \
	geopackage \
	spatialite \
	shapefiles \
	srsinit \
	connection_cache \
	virtualtext \
	wfs \
	dxf \
	md5 \
	control_points \
	cutter \
	topology \
	stored_procedures

AM_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
AM_CPPFLAGS += -I$(top_srcdir)/src/headers
AM_CPPFLAGS += -I$(top_srcdir)

if MODULE_ONLY
lib_LTLIBRARIES = mod_spatialite.la
else
lib_LTLIBRARIES = libspatialite.la mod_spatialite.la
endif

libspatialite_la_SOURCES = versioninfo/version.c

libspatialite_la_LIBADD = ./gaiaaux/libgaiaaux.la \
	./gaiaexif/libgaiaexif.la \
	./gaiageo/libgaiageo.la \
	./geopackage/libgeopackage.la \
	./spatialite/libsplite.la \
	./shapefiles/libshapefiles.la \
	./dxf/libdxf.la \
	./md5/libmd5.la  \
	./control_points/libcontrol_points.la \
	./cutter/libcutter.la \
	./topology/libtopology.la \
	./srsinit/libsrsinit.la \
	./stored_procedures/libstored_procedures.la \
	./connection_cache/libconnection_cache.la \
	./virtualtext/libvirtualtext.la \
	./wfs/libwfs.la @LIBXML2_LIBS@

if MINGW
libspatialite_la_LDFLAGS = -version-info 4:4:0 -no-undefined
libspatialite_la_LIBADD += -lm
else 
if ANDROID
libspatialite_la_LDFLAGS = -version-info 8:1:1
libspatialite_la_LIBADD += -ldl -lm
else
libspatialite_la_LDFLAGS = -version-info 8:1:1
libspatialite_la_LIBADD += -lpthread -ldl -lm
endif
endif

mod_spatialite_la_SOURCES = versioninfo/version.c

mod_spatialite_la_LIBADD = ./gaiaaux/gaiaaux.la \
	./gaiaexif/gaiaexif.la \
	./gaiageo/gaiageo.la \
	./geopackage/geopackage.la \
	./spatialite/splite.la \
	./shapefiles/shapefiles.la \
	./dxf/dxf.la \
	./md5/md5.la \
	./control_points/control_points.la \
	./cutter/cutter.la \
	./topology/topology.la \
	./srsinit/srsinit.la \
	./stored_procedures/stored_procedures.la \
	./connection_cache/connection_cache.la \
	./virtualtext/virtualtext.la \
	./wfs/wfs.la @LIBXML2_LIBS@

mod_spatialite_la_CPPFLAGS = @CFLAGS@ @CPPFLAGS@
mod_spatialite_la_CPPFLAGS += -I$(top_srcdir)/src/headers -I.
mod_spatialite_la_CPPFLAGS += -DLOADABLE_EXTENSION
mod_spatialite_la_LIBTOOLFLAGS = --tag=disable-static

if MINGW
mod_spatialite_la_LDFLAGS = -module -avoid-version -no-undefined
mod_spatialite_la_LIBADD += -lm
else 
if ANDROID
mod_spatialite_la_LDFLAGS = -module -version-info 8:0:1
mod_spatialite_la_LIBADD += -ldl -lm
else
mod_spatialite_la_LDFLAGS = -module -version-info 8:0:1
mod_spatialite_la_LIBADD += -lpthread -ldl -lm
endif
endif

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda
