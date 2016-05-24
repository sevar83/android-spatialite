# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# SV: ICU 51
# https://github.com/illarionov/android-sqlcipher-spatialite/blob/spatialite/ndk-modules/icu4c/Android.mk

LOCAL_PATH := $(call my-dir)

ICU_PATH := platform_external_icu4c

icu_packaging_cflags := \
	-DU_STATIC_IMPLEMENTATION \
	-DUCONFIG_NO_LEGACY_CONVERSION \
	-DUCONFIG_NO_FORMATTING \
	-DUCONFIG_NO_TRANSLITERATION

#-------------------------
# start icu project import
#-------------------------

ICU_COMMON_PATH := $(ICU_PATH)/common
ICU_COMMON_FULL_PATH := $(LOCAL_PATH)/$(ICU_PATH)/common

# new icu common build begin

icu_common_src_files := \
    cmemory.c          cstring.c          \
    cwchar.c           locmap.c           \
    punycode.cpp       putil.cpp          \
    uarrsort.c         ubidi.c            \
    ubidiln.c          ubidi_props.c      \
    ubidiwrt.c         ucase.cpp          \
    ucasemap.cpp       ucat.c             \
    uchar.c            ucln_cmn.c         \
    ucmndata.c                            \
    ucnv2022.cpp       ucnv_bld.cpp       \
    ucnvbocu.cpp       ucnv.c             \
    ucnv_cb.c          ucnv_cnv.c         \
    ucnvdisp.c         ucnv_err.c         \
    ucnv_ext.cpp       ucnvhz.c           \
    ucnv_io.cpp        ucnvisci.c         \
    ucnvlat1.c         ucnv_lmb.c         \
    ucnvmbcs.c         ucnvscsu.c         \
    ucnv_set.c         ucnv_u16.c         \
    ucnv_u32.c         ucnv_u7.c          \
    ucnv_u8.c                             \
    udatamem.c         \
    udataswp.c         uenum.c            \
    uhash.c            uinit.c            \
    uinvchar.c         uloc.cpp           \
    umapfile.c         umath.c            \
    umutex.cpp         unames.cpp         \
    unorm_it.c         uresbund.cpp       \
    ures_cnv.c         uresdata.c         \
    usc_impl.c         uscript.c          \
    uscript_props.cpp  \
    ushape.cpp         ustrcase.cpp       \
    ustr_cnv.c         ustrfmt.c          \
    ustring.cpp        ustrtrns.cpp       \
    ustr_wcs.cpp       utf_impl.c         \
    utrace.c           utrie.cpp          \
    utypes.c           wintz.c            \
    utrie2_builder.cpp icuplug.c          \
    propsvec.c         ulist.c            \
    uloc_tag.c         ucnv_ct.c

icu_common_src_files += \
        bmpset.cpp      unisetspan.cpp   \
    brkeng.cpp      brkiter.cpp      \
    caniter.cpp     chariter.cpp     \
    dictbe.cpp  locbased.cpp     \
    locid.cpp       locutil.cpp      \
    normlzr.cpp     parsepos.cpp     \
    propname.cpp    rbbi.cpp         \
    rbbidata.cpp    rbbinode.cpp     \
    rbbirb.cpp      rbbiscan.cpp     \
    rbbisetb.cpp    rbbistbl.cpp     \
    rbbitblb.cpp    resbund_cnv.cpp  \
    resbund.cpp     ruleiter.cpp     \
    schriter.cpp    serv.cpp         \
    servlk.cpp      servlkf.cpp      \
    servls.cpp      servnotf.cpp     \
    servrbf.cpp     servslkf.cpp     \
    ubrk.cpp         \
    uchriter.cpp    uhash_us.cpp     \
    uidna.cpp       uiter.cpp        \
    unifilt.cpp     unifunct.cpp     \
    uniset.cpp      uniset_props.cpp \
    unistr_case.cpp unistr_cnv.cpp   \
    unistr.cpp      unistr_props.cpp \
    unormcmp.cpp    unorm.cpp        \
    uobject.cpp     uset.cpp         \
    usetiter.cpp    uset_props.cpp   \
    usprep.cpp      ustack.cpp       \
    ustrenum.cpp    utext.cpp        \
    util.cpp        util_props.cpp   \
    uvector.cpp     uvectr32.cpp     \
    errorcode.cpp                    \
    bytestream.cpp  stringpiece.cpp  \
    mutex.cpp       dtintrv.cpp      \
    ucnvsel.cpp     uvectr64.cpp     \
    locavailable.cpp         locdispnames.cpp   \
    loclikely.cpp            locresdata.cpp     \
    normalizer2impl.cpp      normalizer2.cpp    \
    filterednormalizer2.cpp  ucol_swp.cpp       \
    uprops.cpp      utrie2.cpp \
        charstr.cpp     uts46.cpp \
        udata.cpp   appendable.cpp  bytestrie.cpp \
        bytestriebuilder.cpp  bytestrieiterator.cpp \
        messagepattern.cpp patternprops.cpp stringtriebuilder.cpp \
        ucharstrie.cpp ucharstriebuilder.cpp ucharstrieiterator.cpp \
    dictionarydata.cpp \
    ustrcase_locale.cpp unistr_titlecase_brkiter.cpp \
    uniset_closure.cpp ucasemap_titlecase_brkiter.cpp \
    ustr_titlecase_brkiter.cpp unistr_case_locale.cpp

# This is the empty compiled-in icu data structure
# that we need to satisfy the linker.
icu_common_src_files += ../stubdata/stubdata.c

# new icu common build end

icu_common_c_includes := \
        $(ICU_COMMON_FULL_PATH) \
        $(ICU_COMMON_FULL_PATH)/../i18n

# We make the ICU data directory relative to $ANDROID_ROOT on Android, so both
# device and sim builds can use the same codepath, and it's hard to break one
# without noticing because the other still works.
icu_common_local_cflags := '-DICU_DATA_DIR_PREFIX_ENV_VAR="ICU_PREFIX"'
icu_common_local_cflags += '-DICU_DATA_DIR="/icu"'

# bionic doesn't have <langinfo.h>.
icu_common_local_cflags += -DU_HAVE_NL_LANGINFO_CODESET=0
# bionic has timezone instead of __timezone.
icu_common_local_cflags += -DU_TIMEZONE=timezone

icu_common_local_cflags += -D_REENTRANT
icu_common_local_cflags += -DU_COMMON_IMPLEMENTATION

icu_common_local_cflags += -O3 -fvisibility=hidden

#
# Build  as a static library
#

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(icu_common_c_includes)
LOCAL_EXPORT_C_INCLUDES := $(ICU_COMMON_FULL_PATH)
LOCAL_CPP_FEATURES := rtti
LOCAL_CFLAGS += $(icu_common_local_cflags) $(icu_packaging_cflags) -DPIC -fPIC
# Using -Os over -O3 actually cuts down the final executable size by a few dozen kilobytes
LOCAL_CFLAGS += -Os
LOCAL_MODULE := libicuuc_static
LOCAL_SRC_FILES += $(addprefix $(ICU_COMMON_PATH)/, $(icu_common_src_files))
include $(BUILD_STATIC_LIBRARY)


#----------
# end icuuc
#----------


#--------------
# start icui18n
#--------------
include $(CLEAR_VARS)
ICU_I18N_PATH := $(ICU_PATH)/i18n
ICU_I18N_FULL_PATH := $(LOCAL_PATH)/$(ICU_PATH)/i18n

# start new icu18n

icu_i18n_src_files := \
    ucln_in.c  decContext.c \
    ulocdata.c  utmscale.c decNumber.c

icu_i18n_src_files += \
        indiancal.cpp   dtptngen.cpp dtrule.cpp   \
        persncal.cpp    rbtz.cpp     reldtfmt.cpp \
        taiwncal.cpp    tzrule.cpp   tztrans.cpp  \
        udatpg.cpp      vtzone.cpp                \
    anytrans.cpp    astro.cpp    buddhcal.cpp \
    basictz.cpp     calendar.cpp casetrn.cpp  \
    choicfmt.cpp    coleitr.cpp  coll.cpp     \
    compactdecimalformat.cpp \
    cpdtrans.cpp    csdetect.cpp csmatch.cpp  \
    csr2022.cpp     csrecog.cpp  csrmbcs.cpp  \
    csrsbcs.cpp     csrucode.cpp csrutf8.cpp  \
    curramt.cpp     currfmt.cpp  currunit.cpp \
    dangical.cpp \
    datefmt.cpp     dcfmtsym.cpp decimfmt.cpp \
    digitlst.cpp    dtfmtsym.cpp esctrn.cpp   \
    fmtable_cnv.cpp fmtable.cpp  format.cpp   \
    funcrepl.cpp    gender.cpp \
    gregocal.cpp gregoimp.cpp \
    hebrwcal.cpp    identifier_info.cpp \
    inputext.cpp islamcal.cpp \
    japancal.cpp    measfmt.cpp  measure.cpp  \
    msgfmt.cpp      name2uni.cpp nfrs.cpp     \
    nfrule.cpp      nfsubs.cpp   nortrans.cpp \
    nultrans.cpp    numfmt.cpp   olsontz.cpp  \
    quant.cpp       rbnf.cpp     rbt.cpp      \
    rbt_data.cpp    rbt_pars.cpp rbt_rule.cpp \
    rbt_set.cpp     regexcmp.cpp regexst.cpp  \
    regeximp.cpp    region.cpp \
    rematch.cpp     remtrans.cpp repattrn.cpp \
    scriptset.cpp \
    search.cpp      simpletz.cpp smpdtfmt.cpp \
    sortkey.cpp     strmatch.cpp strrepl.cpp  \
    stsearch.cpp    tblcoll.cpp  timezone.cpp \
    titletrn.cpp    tolowtrn.cpp toupptrn.cpp \
    translit.cpp    transreg.cpp tridpars.cpp \
    ucal.cpp        ucol_bld.cpp ucol_cnt.cpp \
    ucol.cpp        ucoleitr.cpp ucol_elm.cpp \
    ucol_res.cpp    ucol_sit.cpp ucol_tok.cpp \
    ucsdet.cpp      ucurr.cpp    udat.cpp     \
    umsg.cpp        unesctrn.cpp uni2name.cpp \
    unum.cpp        uregexc.cpp  uregex.cpp   \
    usearch.cpp     utrans.cpp   windtfmt.cpp \
    winnmfmt.cpp    zonemeta.cpp \
    numsys.cpp      chnsecal.cpp \
    cecal.cpp       coptccal.cpp ethpccal.cpp \
    brktrans.cpp    wintzimpl.cpp plurrule.cpp \
    plurfmt.cpp     dtitvfmt.cpp dtitvinf.cpp \
    tmunit.cpp      tmutamt.cpp  tmutfmt.cpp  \
        currpinf.cpp    uspoof.cpp   uspoof_impl.cpp \
        uspoof_build.cpp     \
        regextxt.cpp    selfmt.cpp   uspoof_conf.cpp \
        uspoof_wsconf.cpp ztrans.cpp zrule.cpp  \
        vzone.cpp       fphdlimp.cpp fpositer.cpp\
        locdspnm.cpp    ucol_wgt.cpp \
        alphaindex.cpp  bocsu.cpp    decfmtst.cpp \
        smpdtfst.cpp    smpdtfst.h   tzfmt.cpp \
        tzgnames.cpp    tznames.cpp  tznames_impl.cpp \
        udateintervalformat.cpp  upluralrules.cpp

# end new icu18n

icu_i18n_c_includes = \
        $(ICU_I18N_FULL_PATH) \
        $(ICU_I18N_FULL_PATH)/../common

icu_i18n_local_cflags := -D_REENTRANT
icu_i18n_local_cflags += -DU_I18N_IMPLEMENTATION
icu_i18n_local_cflags += -O3 -fvisibility=hidden

#
# Build as a static library
#

include $(CLEAR_VARS)
LOCAL_SRC_FILES += $(addprefix $(ICU_I18N_PATH)/, $(icu_i18n_src_files))
LOCAL_C_INCLUDES += $(icu_i18n_c_includes)
LOCAL_STATIC_LIBRARIES += libicuuc_static
LOCAL_EXPORT_C_INCLUDES := $(ICU_I18N_FULL_PATH)
LOCAL_CPP_FEATURES := rtti
LOCAL_CFLAGS += $(icu_i18n_local_cflags) $(icu_packaging_cflags) -DPIC -fPIC
# Using -Os over -O3 actually cuts down the final executable size by a few dozen kilobytes
LOCAL_CFLAGS += -Os
LOCAL_MODULE := libicui18n_static
include $(BUILD_STATIC_LIBRARY)
