LOCAL_PATH := $(call my-dir)
include ${CLEAR_VARS}

SPATIALITE_PATH := libspatialite-4.3.0a

# copy sqlite3.h and sqlite3ext.h from "android-sqlite" module in "config" folder
# ./configure
# find $(SPATIALITE_PATH)/ -name "*.c" | grep -Ev "test|examples" | sort | awk '{ print "\t"$1" \\" }'
# comment out epsg_inlined_prussian.c
# comment initialize_epsg_prussian and and initialize_epsg_extra calls in epsg_inlined_extra.c

LOCAL_SRC_FILES := \
 	 $(SPATIALITE_PATH)/src/connection_cache/alloc_cache.c \
     $(SPATIALITE_PATH)/src/connection_cache/generator/code_generator.c \
     $(SPATIALITE_PATH)/src/control_points/gaia_control_points.c \
     $(SPATIALITE_PATH)/src/control_points/grass_crs3d.c \
     $(SPATIALITE_PATH)/src/control_points/grass_georef.c \
     $(SPATIALITE_PATH)/src/control_points/grass_georef_tps.c \
     $(SPATIALITE_PATH)/src/dxf/dxf_load_distinct.c \
     $(SPATIALITE_PATH)/src/dxf/dxf_loader.c \
     $(SPATIALITE_PATH)/src/dxf/dxf_load_mixed.c \
     $(SPATIALITE_PATH)/src/dxf/dxf_parser.c \
     $(SPATIALITE_PATH)/src/dxf/dxf_writer.c \
     $(SPATIALITE_PATH)/src/gaiaaux/gg_sqlaux.c \
     $(SPATIALITE_PATH)/src/gaiaaux/gg_utf8.c \
     $(SPATIALITE_PATH)/src/gaiaexif/gaia_exif.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_advanced.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_endian.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_ewkt.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_extras.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_geodesic.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_geoJSON.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_geometries.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_geoscvt.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_gml.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_kml.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_lwgeom.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_matrix.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_relations.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_relations_ext.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_shape.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_transform.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_vanuatu.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_voronoj.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_wkb.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_wkt.c \
     $(SPATIALITE_PATH)/src/gaiageo/gg_xml.c \
     $(SPATIALITE_PATH)/src/geopackage/gaia_cvt_gpkg.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgAddGeometryColumn.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkg_add_geometry_triggers.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkg_add_spatial_index.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkg_add_tile_triggers.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgBinary.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgCreateBaseTables.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgCreateTilesTable.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgCreateTilesZoomLevel.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgGetImageType.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkg_get_normal_row.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkg_get_normal_zoom.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgInsertEpsgSRID.c \
     $(SPATIALITE_PATH)/src/geopackage/gpkgMakePoint.c \
     $(SPATIALITE_PATH)/src/md5/gaia_md5.c \
     $(SPATIALITE_PATH)/src/md5/md5.c \
     $(SPATIALITE_PATH)/src/shapefiles/shapefiles.c \
     $(SPATIALITE_PATH)/src/shapefiles/validator.c \
     $(SPATIALITE_PATH)/src/spatialite/extra_tables.c \
     $(SPATIALITE_PATH)/src/spatialite/mbrcache.c \
     $(SPATIALITE_PATH)/src/spatialite/metatables.c \
     $(SPATIALITE_PATH)/src/spatialite/se_helpers.c \
     $(SPATIALITE_PATH)/src/spatialite/spatialite.c \
     $(SPATIALITE_PATH)/src/spatialite/spatialite_init.c \
     $(SPATIALITE_PATH)/src/spatialite/srid_aux.c \
     $(SPATIALITE_PATH)/src/spatialite/statistics.c \
     $(SPATIALITE_PATH)/src/spatialite/table_cloner.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualbbox.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualdbf.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualelementary.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualfdo.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualgpkg.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualnetwork.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualshape.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualspatialindex.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualXL.c \
     $(SPATIALITE_PATH)/src/spatialite/virtualxpath.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_00.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_01.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_02.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_03.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_04.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_05.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_06.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_07.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_08.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_09.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_10.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_11.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_12.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_13.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_14.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_15.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_16.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_17.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_18.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_19.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_20.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_21.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_22.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_23.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_24.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_25.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_26.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_27.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_28.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_29.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_30.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_31.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_32.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_33.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_34.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_35.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_36.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_37.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_38.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_39.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_40.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_41.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_42.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_43.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_44.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_45.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_46.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_extra.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_prussian.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_wgs84_00.c \
     $(SPATIALITE_PATH)/src/srsinit/epsg_inlined_wgs84_01.c \
     $(SPATIALITE_PATH)/src/srsinit/srs_init.c \
     $(SPATIALITE_PATH)/src/versioninfo/version.c \
     $(SPATIALITE_PATH)/src/virtualtext/virtualtext.c \
     $(SPATIALITE_PATH)/src/wfs/wfs_in.c
#		$(SPATIALITE_PATH)/src/srsinit/epsg_inlined_prussian.c
# 	$(SPATIALITE_PATH)/src/gaiageo/Ewkt.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lemon/lemon_src/lemon.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lemon/lemon_src/lempar.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lex.Ewkt.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lex.GeoJson.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lex.Gml.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lex.Kml.c
# 	$(SPATIALITE_PATH)/src/gaiageo/lex.VanuatuWkt.c
# 	$(SPATIALITE_PATH)/src/gaiageo/vanuatuWkt.c
# 	$(SPATIALITE_PATH)/src/gaiageo/geoJSON.c
#	 	$(SPATIALITE_PATH)/src/gaiageo/Gml.c
# 	$(SPATIALITE_PATH)/src/gaiageo/Kml.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(SPATIALITE_PATH)/src/headers \
    $(LOCAL_PATH)/config

LOCAL_MODULE := spatialite

# spatialite flags
# comment out TARGET_CPU in config.h - will be replaced with TARGET_ARCH_ABI
spatialite_flags := \
	-fvisibility=hidden \
	-DTARGET_CPU=\"$(TARGET_ARCH_ABI)\" \
	-DVERSION="\"4.3.0a\"" \
	-DSQLITE_ENABLE_RTREE=1 \
	-DSQLITE_OMIT_BUILTIN_TEST=1 \
	-DOMIT_FREEXL \
	-DOMIT_GEOCALLBACKS \
	-DENABLE_GCP=1 \
    -DENABLE_GEOPACKAGE=1 \
    -DENABLE_LIBXML2=1 \

LOCAL_CFLAGS    := \
   $(common_sqlite_flags) \
   $(spatialite_flags)

LOCAL_STATIC_LIBRARIES := proj4 geos sqlite iconv libxml2

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(SPATIALITE_PATH)/src/headers
LOCAL_EXPORT_LDLIBS := -llog -lz

include $(BUILD_STATIC_LIBRARY)
