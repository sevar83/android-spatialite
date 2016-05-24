LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

PROJ4_PATH := $(LOCAL_PATH)/proj.4/proj
PROJ4_CONFIG := $(LOCAL_PATH)/config

LOCAL_MODULE    := proj4
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_C_INCLUDES := $(PROJ4_PATH)/src $(PROJ4_CONFIG)
LOCAL_EXPORT_C_INCLUDES := $(PROJ4_PATH)/src $(PROJ4_CONFIG)
LOCAL_SRC_FILES := \
	 proj.4/proj/src/PJ_aeqd.c \
	 proj.4/proj/src/PJ_gnom.c \
	 proj.4/proj/src/PJ_laea.c \
	 proj.4/proj/src/PJ_mod_ster.c \
	 proj.4/proj/src/PJ_nsper.c \
	 proj.4/proj/src/PJ_nzmg.c \
	 proj.4/proj/src/PJ_ortho.c \
	 proj.4/proj/src/PJ_stere.c \
	 proj.4/proj/src/PJ_sterea.c \
	 proj.4/proj/src/PJ_aea.c \
	 proj.4/proj/src/PJ_bipc.c \
	 proj.4/proj/src/PJ_bonne.c \
	 proj.4/proj/src/PJ_eqdc.c \
	 proj.4/proj/src/PJ_isea.c \
	 proj.4/proj/src/PJ_imw_p.c \
	 proj.4/proj/src/PJ_krovak.c \
	 proj.4/proj/src/PJ_lcc.c \
	 proj.4/proj/src/PJ_poly.c \
	 proj.4/proj/src/PJ_rpoly.c \
	 proj.4/proj/src/PJ_sconics.c \
	 proj.4/proj/src/proj_rouss.c \
	 proj.4/proj/src/PJ_cass.c \
	 proj.4/proj/src/PJ_cc.c \
	 proj.4/proj/src/PJ_cea.c \
	 proj.4/proj/src/PJ_eqc.c \
	 proj.4/proj/src/PJ_gall.c \
	 proj.4/proj/src/PJ_labrd.c \
	 proj.4/proj/src/PJ_lsat.c \
	 proj.4/proj/src/PJ_merc.c \
	 proj.4/proj/src/PJ_mill.c \
	 proj.4/proj/src/PJ_ocea.c \
	 proj.4/proj/src/PJ_omerc.c \
	 proj.4/proj/src/PJ_somerc.c \
	 proj.4/proj/src/PJ_tcc.c \
	 proj.4/proj/src/PJ_tcea.c \
	 proj.4/proj/src/PJ_tmerc.c \
	 proj.4/proj/src/PJ_airy.c \
	 proj.4/proj/src/PJ_aitoff.c \
	 proj.4/proj/src/PJ_august.c \
	 proj.4/proj/src/PJ_bacon.c \
	 proj.4/proj/src/PJ_chamb.c \
	 proj.4/proj/src/PJ_hammer.c \
	 proj.4/proj/src/PJ_lagrng.c \
	 proj.4/proj/src/PJ_larr.c \
	 proj.4/proj/src/PJ_lask.c \
	 proj.4/proj/src/PJ_nocol.c \
	 proj.4/proj/src/PJ_ob_tran.c \
	 proj.4/proj/src/PJ_oea.c \
	 proj.4/proj/src/PJ_tpeqd.c \
	 proj.4/proj/src/PJ_vandg.c \
	 proj.4/proj/src/PJ_vandg2.c \
	 proj.4/proj/src/PJ_vandg4.c \
	 proj.4/proj/src/PJ_wag7.c \
	 proj.4/proj/src/PJ_lcca.c \
	 proj.4/proj/src/PJ_geos.c \
	 proj.4/proj/src/proj_etmerc.c \
	 proj.4/proj/src/PJ_boggs.c \
	 proj.4/proj/src/PJ_collg.c \
	 proj.4/proj/src/PJ_crast.c \
	 proj.4/proj/src/PJ_denoy.c \
	 proj.4/proj/src/PJ_eck1.c \
	 proj.4/proj/src/PJ_eck2.c \
	 proj.4/proj/src/PJ_eck3.c \
	 proj.4/proj/src/PJ_eck4.c \
	 proj.4/proj/src/PJ_eck5.c \
	 proj.4/proj/src/PJ_fahey.c \
	 proj.4/proj/src/PJ_fouc_s.c \
	 proj.4/proj/src/PJ_gins8.c \
	 proj.4/proj/src/PJ_gstmerc.c \
	 proj.4/proj/src/PJ_gn_sinu.c \
	 proj.4/proj/src/PJ_goode.c \
	 proj.4/proj/src/PJ_igh.c \
	 proj.4/proj/src/PJ_hatano.c \
	 proj.4/proj/src/PJ_loxim.c \
	 proj.4/proj/src/PJ_mbt_fps.c \
	 proj.4/proj/src/PJ_mbtfpp.c \
	 proj.4/proj/src/PJ_mbtfpq.c \
	 proj.4/proj/src/PJ_moll.c \
	 proj.4/proj/src/PJ_nell.c \
	 proj.4/proj/src/PJ_nell_h.c \
	 proj.4/proj/src/PJ_putp2.c \
	 proj.4/proj/src/PJ_putp3.c \
	 proj.4/proj/src/PJ_putp4p.c \
	 proj.4/proj/src/PJ_putp5.c \
	 proj.4/proj/src/PJ_putp6.c \
	 proj.4/proj/src/PJ_robin.c \
	 proj.4/proj/src/PJ_sts.c \
	 proj.4/proj/src/PJ_urm5.c \
	 proj.4/proj/src/PJ_urmfps.c \
	 proj.4/proj/src/PJ_wag2.c \
	 proj.4/proj/src/PJ_wag3.c \
	 proj.4/proj/src/PJ_wink1.c \
	 proj.4/proj/src/PJ_wink2.c \
	 proj.4/proj/src/pj_latlong.c \
	 proj.4/proj/src/pj_geocent.c \
	 proj.4/proj/src/aasincos.c \
	 proj.4/proj/src/adjlon.c \
	 proj.4/proj/src/bch2bps.c \
	 proj.4/proj/src/bchgen.c \
	 proj.4/proj/src/biveval.c \
	 proj.4/proj/src/dmstor.c \
	 proj.4/proj/src/mk_cheby.c \
	 proj.4/proj/src/pj_auth.c \
	 proj.4/proj/src/pj_deriv.c \
	 proj.4/proj/src/pj_ell_set.c \
	 proj.4/proj/src/pj_ellps.c \
	 proj.4/proj/src/pj_errno.c \
	 proj.4/proj/src/pj_factors.c \
	 proj.4/proj/src/pj_fwd.c \
	 proj.4/proj/src/pj_init.c \
	 proj.4/proj/src/pj_inv.c \
	 proj.4/proj/src/pj_list.c \
	 proj.4/proj/src/pj_malloc.c \
	 proj.4/proj/src/pj_mlfn.c \
	 proj.4/proj/src/pj_msfn.c \
	 proj.4/proj/src/proj_mdist.c \
	 \
	 proj.4/proj/src/pj_param.c \
	 proj.4/proj/src/pj_phi2.c \
	 proj.4/proj/src/pj_pr_list.c \
	 proj.4/proj/src/pj_qsfn.c \
	 proj.4/proj/src/pj_strerrno.c \
	 proj.4/proj/src/pj_tsfn.c \
	 proj.4/proj/src/pj_units.c \
	 proj.4/proj/src/pj_ctx.c \
	 proj.4/proj/src/pj_log.c \
	 proj.4/proj/src/pj_zpoly1.c \
	 proj.4/proj/src/rtodms.c \
	 proj.4/proj/src/vector1.c \
	 proj.4/proj/src/pj_release.c \
	 proj.4/proj/src/pj_gauss.c \
	 proj.4/proj/src/PJ_healpix.c \
	 proj.4/proj/src/PJ_natearth.c \
	 \
	 proj.4/proj/src/pj_gc_reader.c \
	 proj.4/proj/src/pj_gridcatalog.c \
	 proj.4/proj/src/nad_cvt.c \
	 proj.4/proj/src/nad_init.c \
	 proj.4/proj/src/nad_intr.c \
	 proj.4/proj/src/emess.c \
	 proj.4/proj/src/pj_apply_gridshift.c \
	 proj.4/proj/src/pj_datums.c \
	 proj.4/proj/src/pj_datum_set.c \
	 proj.4/proj/src/pj_transform.c \
	 proj.4/proj/src/geocent.c \
	 proj.4/proj/src/pj_utils.c \
	 proj.4/proj/src/pj_gridinfo.c \
	 proj.4/proj/src/pj_gridlist.c \
	 proj.4/proj/src/jniproj.c \
	 proj.4/proj/src/pj_mutex.c \
	 proj.4/proj/src/pj_initcache.c \
	 proj.4/proj/src/pj_apply_vgridshift.c \
	 \
	 pj_open_lib.c
#	 proj.4/proj/src/pj_open_lib.c

TARGET-process-src-files-tags += $(call add-src-files-target-cflags, proj.4/proj/src/pj_gc_reader.c, -include ctype.h)

include $(BUILD_STATIC_LIBRARY)

