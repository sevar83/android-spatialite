LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := proj4

PROJ4_CONFIG := $(LOCAL_PATH)/config

LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(PROJ4_PATH)/proj/src $(PROJ4_CONFIG)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(PROJ4_PATH)/proj/src $(PROJ4_CONFIG)
LOCAL_SRC_FILES := \
	 $(PROJ4_PATH)/proj/src/PJ_aeqd.c \
	 $(PROJ4_PATH)/proj/src/PJ_gnom.c \
	 $(PROJ4_PATH)/proj/src/PJ_laea.c \
	 $(PROJ4_PATH)/proj/src/PJ_mod_ster.c \
	 $(PROJ4_PATH)/proj/src/PJ_nsper.c \
	 $(PROJ4_PATH)/proj/src/PJ_nzmg.c \
	 $(PROJ4_PATH)/proj/src/PJ_ortho.c \
	 $(PROJ4_PATH)/proj/src/PJ_stere.c \
	 $(PROJ4_PATH)/proj/src/PJ_sterea.c \
	 $(PROJ4_PATH)/proj/src/PJ_aea.c \
	 $(PROJ4_PATH)/proj/src/PJ_bipc.c \
	 $(PROJ4_PATH)/proj/src/PJ_bonne.c \
	 $(PROJ4_PATH)/proj/src/PJ_eqdc.c \
	 $(PROJ4_PATH)/proj/src/PJ_isea.c \
	 $(PROJ4_PATH)/proj/src/PJ_imw_p.c \
	 $(PROJ4_PATH)/proj/src/PJ_krovak.c \
	 $(PROJ4_PATH)/proj/src/PJ_lcc.c \
	 $(PROJ4_PATH)/proj/src/PJ_poly.c \
	 $(PROJ4_PATH)/proj/src/PJ_rpoly.c \
	 $(PROJ4_PATH)/proj/src/PJ_sconics.c \
	 $(PROJ4_PATH)/proj/src/proj_rouss.c \
	 $(PROJ4_PATH)/proj/src/PJ_cass.c \
	 $(PROJ4_PATH)/proj/src/PJ_cc.c \
	 $(PROJ4_PATH)/proj/src/PJ_cea.c \
	 $(PROJ4_PATH)/proj/src/PJ_eqc.c \
	 $(PROJ4_PATH)/proj/src/PJ_gall.c \
	 $(PROJ4_PATH)/proj/src/PJ_labrd.c \
	 $(PROJ4_PATH)/proj/src/PJ_lsat.c \
	 $(PROJ4_PATH)/proj/src/PJ_merc.c \
	 $(PROJ4_PATH)/proj/src/PJ_mill.c \
	 $(PROJ4_PATH)/proj/src/PJ_ocea.c \
	 $(PROJ4_PATH)/proj/src/PJ_omerc.c \
	 $(PROJ4_PATH)/proj/src/PJ_somerc.c \
	 $(PROJ4_PATH)/proj/src/PJ_tcc.c \
	 $(PROJ4_PATH)/proj/src/PJ_tcea.c \
	 $(PROJ4_PATH)/proj/src/PJ_tmerc.c \
	 $(PROJ4_PATH)/proj/src/PJ_airy.c \
	 $(PROJ4_PATH)/proj/src/PJ_aitoff.c \
	 $(PROJ4_PATH)/proj/src/PJ_august.c \
	 $(PROJ4_PATH)/proj/src/PJ_bacon.c \
	 $(PROJ4_PATH)/proj/src/PJ_chamb.c \
	 $(PROJ4_PATH)/proj/src/PJ_hammer.c \
	 $(PROJ4_PATH)/proj/src/PJ_lagrng.c \
	 $(PROJ4_PATH)/proj/src/PJ_larr.c \
	 $(PROJ4_PATH)/proj/src/PJ_lask.c \
	 $(PROJ4_PATH)/proj/src/PJ_nocol.c \
	 $(PROJ4_PATH)/proj/src/PJ_ob_tran.c \
	 $(PROJ4_PATH)/proj/src/PJ_oea.c \
	 $(PROJ4_PATH)/proj/src/PJ_tpeqd.c \
	 $(PROJ4_PATH)/proj/src/PJ_vandg.c \
	 $(PROJ4_PATH)/proj/src/PJ_vandg2.c \
	 $(PROJ4_PATH)/proj/src/PJ_vandg4.c \
	 $(PROJ4_PATH)/proj/src/PJ_wag7.c \
	 $(PROJ4_PATH)/proj/src/PJ_lcca.c \
	 $(PROJ4_PATH)/proj/src/PJ_geos.c \
	 $(PROJ4_PATH)/proj/src/proj_etmerc.c \
	 $(PROJ4_PATH)/proj/src/PJ_boggs.c \
	 $(PROJ4_PATH)/proj/src/PJ_collg.c \
	 $(PROJ4_PATH)/proj/src/PJ_crast.c \
	 $(PROJ4_PATH)/proj/src/PJ_denoy.c \
	 $(PROJ4_PATH)/proj/src/PJ_eck1.c \
	 $(PROJ4_PATH)/proj/src/PJ_eck2.c \
	 $(PROJ4_PATH)/proj/src/PJ_eck3.c \
	 $(PROJ4_PATH)/proj/src/PJ_eck4.c \
	 $(PROJ4_PATH)/proj/src/PJ_eck5.c \
	 $(PROJ4_PATH)/proj/src/PJ_fahey.c \
	 $(PROJ4_PATH)/proj/src/PJ_fouc_s.c \
	 $(PROJ4_PATH)/proj/src/PJ_gins8.c \
	 $(PROJ4_PATH)/proj/src/PJ_gstmerc.c \
	 $(PROJ4_PATH)/proj/src/PJ_gn_sinu.c \
	 $(PROJ4_PATH)/proj/src/PJ_goode.c \
	 $(PROJ4_PATH)/proj/src/PJ_igh.c \
	 $(PROJ4_PATH)/proj/src/PJ_hatano.c \
	 $(PROJ4_PATH)/proj/src/PJ_loxim.c \
	 $(PROJ4_PATH)/proj/src/PJ_mbt_fps.c \
	 $(PROJ4_PATH)/proj/src/PJ_mbtfpp.c \
	 $(PROJ4_PATH)/proj/src/PJ_mbtfpq.c \
	 $(PROJ4_PATH)/proj/src/PJ_moll.c \
	 $(PROJ4_PATH)/proj/src/PJ_nell.c \
	 $(PROJ4_PATH)/proj/src/PJ_nell_h.c \
	 $(PROJ4_PATH)/proj/src/PJ_putp2.c \
	 $(PROJ4_PATH)/proj/src/PJ_putp3.c \
	 $(PROJ4_PATH)/proj/src/PJ_putp4p.c \
	 $(PROJ4_PATH)/proj/src/PJ_putp5.c \
	 $(PROJ4_PATH)/proj/src/PJ_putp6.c \
	 $(PROJ4_PATH)/proj/src/PJ_robin.c \
	 $(PROJ4_PATH)/proj/src/PJ_sts.c \
	 $(PROJ4_PATH)/proj/src/PJ_urm5.c \
	 $(PROJ4_PATH)/proj/src/PJ_urmfps.c \
	 $(PROJ4_PATH)/proj/src/PJ_wag2.c \
	 $(PROJ4_PATH)/proj/src/PJ_wag3.c \
	 $(PROJ4_PATH)/proj/src/PJ_wink1.c \
	 $(PROJ4_PATH)/proj/src/PJ_wink2.c \
	 $(PROJ4_PATH)/proj/src/pj_latlong.c \
	 $(PROJ4_PATH)/proj/src/pj_geocent.c \
	 $(PROJ4_PATH)/proj/src/aasincos.c \
	 $(PROJ4_PATH)/proj/src/adjlon.c \
	 $(PROJ4_PATH)/proj/src/bch2bps.c \
	 $(PROJ4_PATH)/proj/src/bchgen.c \
	 $(PROJ4_PATH)/proj/src/biveval.c \
	 $(PROJ4_PATH)/proj/src/dmstor.c \
	 $(PROJ4_PATH)/proj/src/mk_cheby.c \
	 $(PROJ4_PATH)/proj/src/pj_auth.c \
	 $(PROJ4_PATH)/proj/src/pj_deriv.c \
	 $(PROJ4_PATH)/proj/src/pj_ell_set.c \
	 $(PROJ4_PATH)/proj/src/pj_ellps.c \
	 $(PROJ4_PATH)/proj/src/pj_errno.c \
	 $(PROJ4_PATH)/proj/src/pj_factors.c \
	 $(PROJ4_PATH)/proj/src/pj_fwd.c \
	 $(PROJ4_PATH)/proj/src/pj_init.c \
	 $(PROJ4_PATH)/proj/src/pj_inv.c \
	 $(PROJ4_PATH)/proj/src/pj_list.c \
	 $(PROJ4_PATH)/proj/src/pj_malloc.c \
	 $(PROJ4_PATH)/proj/src/pj_mlfn.c \
	 $(PROJ4_PATH)/proj/src/pj_msfn.c \
	 $(PROJ4_PATH)/proj/src/proj_mdist.c \
	 \
	 $(PROJ4_PATH)/proj/src/pj_param.c \
	 $(PROJ4_PATH)/proj/src/pj_phi2.c \
	 $(PROJ4_PATH)/proj/src/pj_pr_list.c \
	 $(PROJ4_PATH)/proj/src/pj_qsfn.c \
	 $(PROJ4_PATH)/proj/src/pj_strerrno.c \
	 $(PROJ4_PATH)/proj/src/pj_tsfn.c \
	 $(PROJ4_PATH)/proj/src/pj_units.c \
	 $(PROJ4_PATH)/proj/src/pj_ctx.c \
	 $(PROJ4_PATH)/proj/src/pj_log.c \
	 $(PROJ4_PATH)/proj/src/pj_zpoly1.c \
	 $(PROJ4_PATH)/proj/src/rtodms.c \
	 $(PROJ4_PATH)/proj/src/vector1.c \
	 $(PROJ4_PATH)/proj/src/pj_release.c \
	 $(PROJ4_PATH)/proj/src/pj_gauss.c \
	 $(PROJ4_PATH)/proj/src/PJ_healpix.c \
	 $(PROJ4_PATH)/proj/src/PJ_natearth.c \
	 \
	 $(PROJ4_PATH)/proj/src/pj_gc_reader.c \
	 $(PROJ4_PATH)/proj/src/pj_gridcatalog.c \
	 $(PROJ4_PATH)/proj/src/nad_cvt.c \
	 $(PROJ4_PATH)/proj/src/nad_init.c \
	 $(PROJ4_PATH)/proj/src/nad_intr.c \
	 $(PROJ4_PATH)/proj/src/emess.c \
	 $(PROJ4_PATH)/proj/src/pj_apply_gridshift.c \
	 $(PROJ4_PATH)/proj/src/pj_datums.c \
	 $(PROJ4_PATH)/proj/src/pj_datum_set.c \
	 $(PROJ4_PATH)/proj/src/pj_transform.c \
	 $(PROJ4_PATH)/proj/src/geocent.c \
	 $(PROJ4_PATH)/proj/src/pj_utils.c \
	 $(PROJ4_PATH)/proj/src/pj_gridinfo.c \
	 $(PROJ4_PATH)/proj/src/pj_gridlist.c \
	 $(PROJ4_PATH)/proj/src/jniproj.c \
	 $(PROJ4_PATH)/proj/src/pj_mutex.c \
	 $(PROJ4_PATH)/proj/src/pj_initcache.c \
	 $(PROJ4_PATH)/proj/src/pj_apply_vgridshift.c \
	 \
	 pj_open_lib.c \
	 jniproj.c
#	 $(PROJ4_PATH)/proj/src/pj_open_lib.c

TARGET-process-src-files-tags += $(call add-src-files-target-cflags, $(PROJ4_PATH)/proj/src/pj_gc_reader.c, -include ctype.h)

include $(BUILD_STATIC_LIBRARY)

