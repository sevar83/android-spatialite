
AM_CPPFLAGS = -I$(top_builddir)/src
AM_CPPFLAGS += -I$(top_builddir)/headers -I$(top_srcdir)/headers

lib_LTLIBRARIES = librttopo.la

librttopo_la_SOURCES = box2d.c bytebuffer.c g_box.c \
	g_serialized.c g_util.c measures3d.c measures.c \
	ptarray.c rtalgorithm.c rtcircstring.c rtcollection.c \
	rtcompound.c rtcurvepoly.c rtgeodetic.c \
	rtgeom_api.c rtgeom.c rtgeom_debug.c rtgeom_geos.c \
	rtgeom_geos_clean.c rtgeom_geos_node.c rtgeom_geos_split.c \
  rtgeom_topo.c rthomogenize.c rtin_geojson.c rtin_twkb.c \
	rtin_wkb.c rtiterator.c rtlinearreferencing.c rtline.c \
	rtmcurve.c rtmline.c rtmpoint.c rtmpoly.c rtmsurface.c \
	rtout_encoded_polyline.c rtout_geojson.c rtout_gml.c \
	rtout_kml.c rtout_svg.c rtout_twkb.c rtout_wkb.c \
	rtout_wkt.c rtout_x3d.c rtpoint.c rtpoly.c rtprint.c \
	rtpsurface.c rtspheroid.c rtstroke.c \
	rtt_tpsnap.c \
  rttin.c rttree.c \
	rttriangle.c rtutil.c stringbuffer.c varint.c


# Version info is current:revision:age
# TODO: have this set from configure.ac
librttopo_la_LDFLAGS = -version-info 2:0:1 -no-undefined

librttopo_la_LIBADD = -lm

noinst_HEADERS = bytebuffer.h librttopo_geom_internal.h \
	librttopo_internal.h measures3d.h measures.h \
	rtgeodetic.h rtgeom_geos.h \
	rtgeom_log.h rtout_twkb.h rttopo_config.h \
	rttree.h stringbuffer.h varint.h
