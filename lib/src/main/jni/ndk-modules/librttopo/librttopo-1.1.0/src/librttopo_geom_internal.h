/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 *
 * rttopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rttopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rttopo.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2011-2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2007-2008 Mark Cave-Ayland
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



#ifndef _LIBRTGEOM_INTERNAL_H
#define _LIBRTGEOM_INTERNAL_H 1

#include "librttopo_geom.h"

#include "rtgeom_log.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#if defined(PJ_VERSION) && PJ_VERSION >= 490
/* Enable new geodesic functions */
#define PROJ_GEODESIC 1
#else
/* Use the old (pre-2.2) geodesic functions */
#define PROJ_GEODESIC 0
#endif


#include <float.h>

#include "librttopo_geom.h"

/**
* Floating point comparators.
*/
#define FP_TOLERANCE 1e-12
#define FP_IS_ZERO(A) (fabs(A) <= FP_TOLERANCE)
#define FP_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define FP_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define FP_ABS(a)   ((a) <  (0) ? -(a) : (a))
#define FP_EQUALS(A, B) (fabs((A)-(B)) <= FP_TOLERANCE)
#define FP_NEQUALS(A, B) (fabs((A)-(B)) > FP_TOLERANCE)
#define FP_LT(A, B) (((A) + FP_TOLERANCE) < (B))
#define FP_LTEQ(A, B) (((A) - FP_TOLERANCE) <= (B))
#define FP_GT(A, B) (((A) - FP_TOLERANCE) > (B))
#define FP_GTEQ(A, B) (((A) + FP_TOLERANCE) >= (B))
#define FP_CONTAINS_TOP(A, X, B) (FP_LT(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_BOTTOM(A, X, B) (FP_LTEQ(A, X) && FP_LT(X, B))
#define FP_CONTAINS_INCL(A, X, B) (FP_LTEQ(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_EXCL(A, X, B) (FP_LT(A, X) && FP_LT(X, B))
#define FP_CONTAINS(A, X, B) FP_CONTAINS_EXCL(A, X, B)


/*
* this will change to NaN when I figure out how to
* get NaN in a platform-independent way
*/
#define NO_VALUE 0.0
#define NO_Z_VALUE NO_VALUE
#define NO_M_VALUE NO_VALUE


/**
* Well-Known Text (RTWKT) Output Variant Types
*/
#define RTWKT_NO_TYPE 0x08 /* Internal use only */
#define RTWKT_NO_PARENS 0x10 /* Internal use only */
#define RTWKT_IS_CHILD 0x20 /* Internal use only */

/**
* Well-Known Binary (RTWKB) Output Variant Types
*/

#define RTWKB_DOUBLE_SIZE 8 /* Internal use only */
#define RTWKB_INT_SIZE 4 /* Internal use only */
#define RTWKB_BYTE_SIZE 1 /* Internal use only */

/**
* Well-Known Binary (RTWKB) Geometry Types
*/
#define RTWKB_POINT_TYPE 1
#define RTWKB_LINESTRING_TYPE 2
#define RTWKB_POLYGON_TYPE 3
#define RTWKB_MULTIPOINT_TYPE 4
#define RTWKB_MULTILINESTRING_TYPE 5
#define RTWKB_MULTIPOLYGON_TYPE 6
#define RTWKB_GEOMETRYCOLLECTION_TYPE 7
#define RTWKB_CIRCULARSTRING_TYPE 8
#define RTWKB_COMPOUNDCURVE_TYPE 9
#define RTWKB_CURVEPOLYGON_TYPE 10
#define RTWKB_MULTICURVE_TYPE 11
#define RTWKB_MULTISURFACE_TYPE 12
#define RTWKB_CURVE_TYPE 13 /* from ISO draft, not sure is real */
#define RTWKB_SURFACE_TYPE 14 /* from ISO draft, not sure is real */
#define RTWKB_POLYHEDRALSURFACE_TYPE 15
#define RTWKB_TIN_TYPE 16
#define RTWKB_TRIANGLE_TYPE 17

/**
* Macro for reading the size from the GSERIALIZED size attribute.
* Cribbed from PgSQL, top 30 bits are size. Use VARSIZE() when working
* internally with PgSQL.
*/
#define SIZE_GET(varsize) (((varsize) >> 2) & 0x3FFFFFFF)
#define SIZE_SET(varsize, size) (((varsize) & 0x00000003)|(((size) & 0x3FFFFFFF) << 2 ))

/**
* Tolerance used to determine equality.
*/
#define EPSILON_SQLMM 1e-8

/*
 * Export functions
 */
#define OUT_MAX_DOUBLE 1E15
#define OUT_SHOW_DIGS_DOUBLE 20
#define OUT_MAX_DOUBLE_PRECISION 15
#define OUT_MAX_DIGS_DOUBLE (OUT_SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */


/**
* Constants for point-in-polygon return values
*/
#define RT_INSIDE 1
#define RT_BOUNDARY 0
#define RT_OUTSIDE -1

#define RTGEOM_GEOS_ERRMSG_MAXSIZE 256

struct RTCTX_T {
  GEOSContextHandle_t gctx;
  char rtgeom_geos_errmsg[RTGEOM_GEOS_ERRMSG_MAXSIZE];
  rtallocator rtalloc_var;
  rtreallocator rtrealloc_var;
  rtfreeor rtfree_var;
  rtreporter error_logger;
  void * error_logger_arg;
  rtreporter notice_logger;
  void * notice_logger_arg;
  rtdebuglogger debug_logger;
  void * debug_logger_arg;
};

typedef struct
{
  double xmin, ymin, zmin;
  double xmax, ymax, zmax;
  int32_t srid;
}
BOX3D;

/*
* Internal prototypes
*/

/* Machine endianness */
#define XDR 0 /* big endian */
#define NDR 1 /* little endian */
extern char getMachineEndian(const RTCTX *ctx);


/*
* Force dims
*/
RTGEOM* rtgeom_force_dims(const RTCTX *ctx, const RTGEOM *rtgeom, int hasz, int hasm);
RTPOINT* rtpoint_force_dims(const RTCTX *ctx, const RTPOINT *rtpoint, int hasz, int hasm);
RTLINE* rtline_force_dims(const RTCTX *ctx, const RTLINE *rtline, int hasz, int hasm);
RTPOLY* rtpoly_force_dims(const RTCTX *ctx, const RTPOLY *rtpoly, int hasz, int hasm);
RTCOLLECTION* rtcollection_force_dims(const RTCTX *ctx, const RTCOLLECTION *rtcol, int hasz, int hasm);
RTPOINTARRAY* ptarray_force_dims(const RTCTX *ctx, const RTPOINTARRAY *pa, int hasz, int hasm);

/**
 * Swap ordinate values o1 and o2 on a given RTPOINTARRAY
 *
 * Ordinates semantic is: 0=x 1=y 2=z 3=m
 */
void ptarray_swap_ordinates(const RTCTX *ctx, RTPOINTARRAY *pa, RTORD o1, RTORD o2);

/*
* Is Empty?
*/
int rtpoly_is_empty(const RTCTX *ctx, const RTPOLY *poly);
int rtcollection_is_empty(const RTCTX *ctx, const RTCOLLECTION *col);
int rtcircstring_is_empty(const RTCTX *ctx, const RTCIRCSTRING *circ);
int rttriangle_is_empty(const RTCTX *ctx, const RTTRIANGLE *triangle);
int rtline_is_empty(const RTCTX *ctx, const RTLINE *line);
int rtpoint_is_empty(const RTCTX *ctx, const RTPOINT *point);

/*
* Number of vertices?
*/
int rtline_count_vertices(const RTCTX *ctx, RTLINE *line);
int rtpoly_count_vertices(const RTCTX *ctx, RTPOLY *poly);
int rtcollection_count_vertices(const RTCTX *ctx, RTCOLLECTION *col);

/*
* Read from byte buffer
*/
extern uint32_t rt_get_uint32_t(const RTCTX *ctx, const uint8_t *loc);
extern int32_t rt_get_int32_t(const RTCTX *ctx, const uint8_t *loc);

/*
* DP simplification
*/

/**
 * @param minpts minimun number of points to retain, if possible.
 */
RTPOINTARRAY* ptarray_simplify(const RTCTX *ctx, RTPOINTARRAY *inpts, double epsilon, unsigned int minpts);
RTLINE* rtline_simplify(const RTCTX *ctx, const RTLINE *iline, double dist, int preserve_collapsed);
RTPOLY* rtpoly_simplify(const RTCTX *ctx, const RTPOLY *ipoly, double dist, int preserve_collapsed);
RTCOLLECTION* rtcollection_simplify(const RTCTX *ctx, const RTCOLLECTION *igeom, double dist, int preserve_collapsed);

/*
* Computational geometry
*/
int signum(const RTCTX *ctx, double n);

/*
* The possible ways a pair of segments can interact. Returned by rt_segment_intersects
*/
enum RTCG_SEGMENT_INTERSECTION_TYPE {
    SEG_ERROR = -1,
    SEG_NO_INTERSECTION = 0,
    SEG_COLINEAR = 1,
    SEG_CROSS_LEFT = 2,
    SEG_CROSS_RIGHT = 3,
    SEG_TOUCH_LEFT = 4,
    SEG_TOUCH_RIGHT = 5
};

/*
* Do the segments intersect? How?
*/
int rt_segment_intersects(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *q1, const RTPOINT2D *q2);

/*
* Get/Set an enumeratoed ordinate. (x,y,z,m)
*/
double rtpoint_get_ordinate(const RTCTX *ctx, const RTPOINT4D *p, char ordinate);
void rtpoint_set_ordinate(const RTCTX *ctx, RTPOINT4D *p, char ordinate, double value);

/*
* Generate an interpolated coordinate p given an interpolation value and ordinate to apply it to
*/
int point_interpolate(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, RTPOINT4D *p, int hasz, int hasm, char ordinate, double interpolation_value);


/**
* Clip a line based on the from/to range of one of its ordinates. Use for m- and z- clipping
*/
RTCOLLECTION *rtline_clip_to_ordinate_range(const RTCTX *ctx, const RTLINE *line, char ordinate, double from, double to);

/**
* Clip a multi-line based on the from/to range of one of its ordinates. Use for m- and z- clipping
*/
RTCOLLECTION *rtmline_clip_to_ordinate_range(const RTCTX *ctx, const RTMLINE *mline, char ordinate, double from, double to);

/**
* Clip a multi-point based on the from/to range of one of its ordinates. Use for m- and z- clipping
*/
RTCOLLECTION *rtmpoint_clip_to_ordinate_range(const RTCTX *ctx, const RTMPOINT *mpoint, char ordinate, double from, double to);

/**
* Clip a point based on the from/to range of one of its ordinates. Use for m- and z- clipping
*/
RTCOLLECTION *rtpoint_clip_to_ordinate_range(const RTCTX *ctx, const RTPOINT *mpoint, char ordinate, double from, double to);

/*
* Geohash
*/
int rtgeom_geohash_precision(const RTCTX *ctx, RTGBOX bbox, RTGBOX *bounds);
char *geohash_point(const RTCTX *ctx, double longitude, double latitude, int precision);
void decode_geohash_bbox(const RTCTX *ctx, char *geohash, double *lat, double *lon, int precision);

/*
* Point comparisons
*/
int p4d_same(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2);
int p3d_same(const RTCTX *ctx, const POINT3D *p1, const POINT3D *p2);
int p2d_same(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2);

/*
* Area calculations
*/
double rtpoly_area(const RTCTX *ctx, const RTPOLY *poly);
double rtcurvepoly_area(const RTCTX *ctx, const RTCURVEPOLY *curvepoly);
double rttriangle_area(const RTCTX *ctx, const RTTRIANGLE *triangle);

/**
* Pull a #RTGBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, return RT_FAILURE.
*/
extern int gserialized_read_gbox_p(const RTCTX *ctx, const GSERIALIZED *g, RTGBOX *gbox);

/*
* Length calculations
*/
double rtcompound_length(const RTCTX *ctx, const RTCOMPOUND *comp);
double rtcompound_length_2d(const RTCTX *ctx, const RTCOMPOUND *comp);
double rtline_length(const RTCTX *ctx, const RTLINE *line);
double rtline_length_2d(const RTCTX *ctx, const RTLINE *line);
double rtcircstring_length(const RTCTX *ctx, const RTCIRCSTRING *circ);
double rtcircstring_length_2d(const RTCTX *ctx, const RTCIRCSTRING *circ);
double rtpoly_perimeter(const RTCTX *ctx, const RTPOLY *poly);
double rtpoly_perimeter_2d(const RTCTX *ctx, const RTPOLY *poly);
double rtcurvepoly_perimeter(const RTCTX *ctx, const RTCURVEPOLY *poly);
double rtcurvepoly_perimeter_2d(const RTCTX *ctx, const RTCURVEPOLY *poly);
double rttriangle_perimeter(const RTCTX *ctx, const RTTRIANGLE *triangle);
double rttriangle_perimeter_2d(const RTCTX *ctx, const RTTRIANGLE *triangle);

/*
* Segmentization
*/
RTLINE *rtcircstring_stroke(const RTCTX *ctx, const RTCIRCSTRING *icurve, uint32_t perQuad);
RTLINE *rtcompound_stroke(const RTCTX *ctx, const RTCOMPOUND *icompound, uint32_t perQuad);
RTPOLY *rtcurvepoly_stroke(const RTCTX *ctx, const RTCURVEPOLY *curvepoly, uint32_t perQuad);

/*
* Affine
*/
void ptarray_affine(const RTCTX *ctx, RTPOINTARRAY *pa, const RTAFFINE *affine);

/*
* Scale
*/
void ptarray_scale(const RTCTX *ctx, RTPOINTARRAY *pa, const RTPOINT4D *factor);

/*
* PointArray
*/
int ptarray_has_z(const RTCTX *ctx, const RTPOINTARRAY *pa);
int ptarray_has_m(const RTCTX *ctx, const RTPOINTARRAY *pa);
double ptarray_signed_area(const RTCTX *ctx, const RTPOINTARRAY *pa);

/*
* Clone support
*/
RTLINE *rtline_clone(const RTCTX *ctx, const RTLINE *rtgeom);
RTPOLY *rtpoly_clone(const RTCTX *ctx, const RTPOLY *rtgeom);
RTTRIANGLE *rttriangle_clone(const RTCTX *ctx, const RTTRIANGLE *rtgeom);
RTCOLLECTION *rtcollection_clone(const RTCTX *ctx, const RTCOLLECTION *rtgeom);
RTCIRCSTRING *rtcircstring_clone(const RTCTX *ctx, const RTCIRCSTRING *curve);
RTPOINTARRAY *ptarray_clone(const RTCTX *ctx, const RTPOINTARRAY *ptarray);
RTGBOX *box2d_clone(const RTCTX *ctx, const RTGBOX *rtgeom);
RTLINE *rtline_clone_deep(const RTCTX *ctx, const RTLINE *rtgeom);
RTPOLY *rtpoly_clone_deep(const RTCTX *ctx, const RTPOLY *rtgeom);
RTCOLLECTION *rtcollection_clone_deep(const RTCTX *ctx, const RTCOLLECTION *rtgeom);
RTGBOX *gbox_clone(const RTCTX *ctx, const RTGBOX *gbox);

/*
* Startpoint
*/
int rtpoly_startpoint(const RTCTX *ctx, const RTPOLY* rtpoly, RTPOINT4D* pt);
int ptarray_startpoint(const RTCTX *ctx, const RTPOINTARRAY* pa, RTPOINT4D* pt);
int rtcollection_startpoint(const RTCTX *ctx, const RTCOLLECTION* col, RTPOINT4D* pt);

/*
 * Write into *ret the coordinates of the closest point on
 * segment A-B to the reference input point R
 */
void closest_point_on_segment(const RTCTX *ctx, const RTPOINT4D *R, const RTPOINT4D *A, const RTPOINT4D *B, RTPOINT4D *ret);

/*
* Repeated points
*/
RTPOINTARRAY *ptarray_remove_repeated_points_minpoints(const RTCTX *ctx, const RTPOINTARRAY *in, double tolerance, int minpoints);
RTPOINTARRAY *ptarray_remove_repeated_points(const RTCTX *ctx, const RTPOINTARRAY *in, double tolerance);
RTGEOM* rtmpoint_remove_repeated_points(const RTCTX *ctx, const RTMPOINT *in, double tolerance);
RTGEOM* rtline_remove_repeated_points(const RTCTX *ctx, const RTLINE *in, double tolerance);
RTGEOM* rtcollection_remove_repeated_points(const RTCTX *ctx, const RTCOLLECTION *in, double tolerance);
RTGEOM* rtpoly_remove_repeated_points(const RTCTX *ctx, const RTPOLY *in, double tolerance);

/*
* Closure test
*/
int rtline_is_closed(const RTCTX *ctx, const RTLINE *line);
int rtpoly_is_closed(const RTCTX *ctx, const RTPOLY *poly);
int rtcircstring_is_closed(const RTCTX *ctx, const RTCIRCSTRING *curve);
int rtcompound_is_closed(const RTCTX *ctx, const RTCOMPOUND *curve);
int rtpsurface_is_closed(const RTCTX *ctx, const RTPSURFACE *psurface);
int rttin_is_closed(const RTCTX *ctx, const RTTIN *tin);

/**
* Snap to grid
*/

/**
* Snap-to-grid Support
*/
typedef struct gridspec_t
{
  double ipx;
  double ipy;
  double ipz;
  double ipm;
  double xsize;
  double ysize;
  double zsize;
  double msize;
}
gridspec;

RTGEOM* rtgeom_grid(const RTCTX *ctx, const RTGEOM *rtgeom, const gridspec *grid);
RTCOLLECTION* rtcollection_grid(const RTCTX *ctx, const RTCOLLECTION *coll, const gridspec *grid);
RTPOINT* rtpoint_grid(const RTCTX *ctx, const RTPOINT *point, const gridspec *grid);
RTPOLY* rtpoly_grid(const RTCTX *ctx, const RTPOLY *poly, const gridspec *grid);
RTLINE* rtline_grid(const RTCTX *ctx, const RTLINE *line, const gridspec *grid);
RTCIRCSTRING* rtcircstring_grid(const RTCTX *ctx, const RTCIRCSTRING *line, const gridspec *grid);
RTPOINTARRAY* ptarray_grid(const RTCTX *ctx, const RTPOINTARRAY *pa, const gridspec *grid);

/*
* What side of the line formed by p1 and p2 does q fall?
* Returns -1 for left and 1 for right and 0 for co-linearity
*/
int rt_segment_side(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *q);
int rt_arc_side(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, const RTPOINT2D *Q);
int rt_arc_calculate_gbox_cartesian_2d(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, RTGBOX *gbox);
double rt_arc_center(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *p3, RTPOINT2D *result);
int rt_pt_in_seg(const RTCTX *ctx, const RTPOINT2D *P, const RTPOINT2D *A1, const RTPOINT2D *A2);
int rt_pt_in_arc(const RTCTX *ctx, const RTPOINT2D *P, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
int rt_arc_is_pt(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
double rt_seg_length(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2);
double rt_arc_length(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
int pt_in_ring_2d(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINTARRAY *ring);
int ptarray_contains_point(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt);
int ptarrayarc_contains_point(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt);
int ptarray_contains_point_partial(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number);
int ptarrayarc_contains_point_partial(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number);
int rtcompound_contains_point(const RTCTX *ctx, const RTCOMPOUND *comp, const RTPOINT2D *pt);
int rtgeom_contains_point(const RTCTX *ctx, const RTGEOM *geom, const RTPOINT2D *pt);


/**
* Return a valid SRID from an arbitrary integer
* Raises a notice if what comes out is different from
* what went in.
* Raises an error if SRID value is out of bounds.
*/
extern int clamp_srid(const RTCTX *ctx, int srid);

/* Raise an rterror if srids do not match */
void error_if_srid_mismatch(const RTCTX *ctx, int srid1, int srid2);

/**
* Split a line by a point and push components to the provided multiline.
* If the point doesn't split the line, push nothing to the container.
* Returns 0 if the point is off the line.
* Returns 1 if the point is on the line boundary (endpoints).
* Return 2 if the point is on the interior of the line (only case in which
* a split happens).
*
* NOTE: the components pushed to the output vector have their SRID stripped
*/
int rtline_split_by_point_to(const RTCTX *ctx, const RTLINE* ln, const RTPOINT* pt, RTMLINE* to);

/** Ensure the collection can hold at least up to ngeoms geometries */
void rtcollection_reserve(const RTCTX *ctx, RTCOLLECTION *col, int ngeoms);

/** Check if subtype is allowed in collectiontype */
extern int rtcollection_allows_subtype(const RTCTX *ctx, int collectiontype, int subtype);

/** RTGBOX utility functions to figure out coverage/location on the globe */
double gbox_angular_height(const RTCTX *ctx, const RTGBOX* gbox);
double gbox_angular_width(const RTCTX *ctx, const RTGBOX* gbox);
int gbox_centroid(const RTCTX *ctx, const RTGBOX* gbox, RTPOINT2D* out);

/* Utilities */
extern void trim_trailing_zeros(const RTCTX *ctx, char *num);

extern uint8_t RTMULTITYPE[RTNUMTYPES];

extern rtinterrupt_callback *_rtgeom_interrupt_callback;
extern int _rtgeom_interrupt_requested;
#define RT_ON_INTERRUPT(x) { \
  if ( _rtgeom_interrupt_callback ) { \
    (*_rtgeom_interrupt_callback)(); \
  } \
  if ( _rtgeom_interrupt_requested ) { \
    _rtgeom_interrupt_requested = 0; \
    rtnotice(ctx, "librtgeom code interrupted"); \
    x; \
  } \
}

int ptarray_npoints_in_rect(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTGBOX *gbox);
int gbox_contains_point2d(const RTCTX *ctx, const RTGBOX *g, const RTPOINT2D *p);
int rtpoly_contains_point(const RTCTX *ctx, const RTPOLY *poly, const RTPOINT2D *pt);
double distance2d_pt_seg(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINT2D *A, const RTPOINT2D *B);

/*------------------------------------------------------
 * other stuff
 *
 * handle the double-to-float conversion.  The results of this
 * will usually be a slightly bigger box because of the difference
 * between float8 and float4 representations.
 */

extern BOX3D* box3d_from_gbox(const RTCTX *ctx, const RTGBOX *gbox);
extern RTGBOX* box3d_to_gbox(const RTCTX *ctx, const BOX3D *b3d);
void expand_box3d(BOX3D *box, double d);
extern void printBOX3D(const RTCTX *ctx, BOX3D *b);



#endif /* _LIBRTGEOM_INTERNAL_H */
