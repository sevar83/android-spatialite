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
 * Copyright 2010 Nicklas Avén
 * Copyright 2010 Nicklas Avén
 *
 **********************************************************************/



/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 * Copyright 2010 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librttopo_geom_internal.h"

/* for the measure functions*/
#define DIST_MAX    -1
#define DIST_MIN    1

/**
* Structure used in distance-calculations
*/
typedef struct
{
  double distance;  /*the distance between p1 and p2*/
  RTPOINT2D p1;
  RTPOINT2D p2;
  int mode;  /*the direction of looking, if thedir = -1 then we look for maxdistance and if it is 1 then we look for mindistance*/
  int twisted; /*To preserve the order of incoming points to match the first and secon point in shortest and longest line*/
  double tolerance; /*the tolerance for dwithin and dfullywithin*/
} DISTPTS;

typedef struct
{
  double themeasure;  /*a value calculated to compare distances*/
  int pnr;  /*pointnumber. the ordernumber of the point*/
} LISTSTRUCT;


/*
* Preprocessing functions
*/
int rt_dist2d_comp(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, DISTPTS *dl);
int rt_dist2d_distribute_bruteforce(const RTCTX *ctx, const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl);
int rt_dist2d_recursive(const RTCTX *ctx, const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl);
int rt_dist2d_check_overlap(const RTCTX *ctx, RTGEOM *rtg1, RTGEOM *rtg2);
int rt_dist2d_distribute_fast(const RTCTX *ctx, RTGEOM *rtg1, RTGEOM *rtg2, DISTPTS *dl);

/*
* Brute force functions
*/
int rt_dist2d_pt_ptarray(const RTCTX *ctx, const RTPOINT2D *p, RTPOINTARRAY *pa, DISTPTS *dl);
int rt_dist2d_pt_ptarrayarc(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINTARRAY *pa, DISTPTS *dl);
int rt_dist2d_ptarray_ptarray(const RTCTX *ctx, RTPOINTARRAY *l1, RTPOINTARRAY *l2, DISTPTS *dl);
int rt_dist2d_ptarray_ptarrayarc(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINTARRAY *pb, DISTPTS *dl);
int rt_dist2d_ptarrayarc_ptarrayarc(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINTARRAY *pb, DISTPTS *dl);
int rt_dist2d_ptarray_poly(RTPOINTARRAY *pa, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_point_point(const RTCTX *ctx, RTPOINT *point1, RTPOINT *point2, DISTPTS *dl);
int rt_dist2d_point_line(const RTCTX *ctx, RTPOINT *point, RTLINE *line, DISTPTS *dl);
int rt_dist2d_point_circstring(const RTCTX *ctx, RTPOINT *point, RTCIRCSTRING *circ, DISTPTS *dl);
int rt_dist2d_point_poly(const RTCTX *ctx, RTPOINT *point, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_point_curvepoly(const RTCTX *ctx, RTPOINT *point, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_line_line(const RTCTX *ctx, RTLINE *line1, RTLINE *line2, DISTPTS *dl);
int rt_dist2d_line_circstring(const RTCTX *ctx, RTLINE *line1, RTCIRCSTRING *line2, DISTPTS *dl);
int rt_dist2d_line_poly(const RTCTX *ctx, RTLINE *line, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_line_curvepoly(const RTCTX *ctx, RTLINE *line, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_circstring_circstring(const RTCTX *ctx, RTCIRCSTRING *line1, RTCIRCSTRING *line2, DISTPTS *dl);
int rt_dist2d_circstring_poly(const RTCTX *ctx, RTCIRCSTRING *circ, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_circstring_curvepoly(const RTCTX *ctx, RTCIRCSTRING *circ, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_poly_poly(const RTCTX *ctx, RTPOLY *poly1, RTPOLY *poly2, DISTPTS *dl);
int rt_dist2d_poly_curvepoly(const RTCTX *ctx, RTPOLY *poly1, RTCURVEPOLY *curvepoly2, DISTPTS *dl);
int rt_dist2d_curvepoly_curvepoly(const RTCTX *ctx, RTCURVEPOLY *poly1, RTCURVEPOLY *poly2, DISTPTS *dl);

/*
* New faster distance calculations
*/
int rt_dist2d_pre_seg_seg(const RTCTX *ctx, RTPOINTARRAY *l1, RTPOINTARRAY *l2,LISTSTRUCT *list1, LISTSTRUCT *list2,double k, DISTPTS *dl);
int rt_dist2d_selected_seg_seg(const RTCTX *ctx, const RTPOINT2D *A, const RTPOINT2D *B, const RTPOINT2D *C, const RTPOINT2D *D, DISTPTS *dl);
int struct_cmp_by_measure(const void *a, const void *b);
int rt_dist2d_fast_ptarray_ptarray(const RTCTX *ctx, RTPOINTARRAY *l1,RTPOINTARRAY *l2, DISTPTS *dl,  RTGBOX *box1, RTGBOX *box2);

/*
* Distance calculation primitives.
*/
int rt_dist2d_pt_pt(const RTCTX *ctx, const RTPOINT2D *P,  const RTPOINT2D *Q,  DISTPTS *dl);
int rt_dist2d_pt_seg(const RTCTX *ctx, const RTPOINT2D *P,  const RTPOINT2D *A1, const RTPOINT2D *A2, DISTPTS *dl);
int rt_dist2d_pt_arc(const RTCTX *ctx, const RTPOINT2D *P,  const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, DISTPTS *dl);
int rt_dist2d_seg_seg(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *B1, const RTPOINT2D *B2, DISTPTS *dl);
int rt_dist2d_seg_arc(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *B1, const RTPOINT2D *B2, const RTPOINT2D *B3, DISTPTS *dl);
int rt_dist2d_arc_arc(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, const RTPOINT2D *B1, const RTPOINT2D *B2, const RTPOINT2D* B3, DISTPTS *dl);
void rt_dist2d_distpts_init(const RTCTX *ctx, DISTPTS *dl, int mode);

/*
* Length primitives
*/
double rt_arc_length(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);

/*
* Geometry returning functions
*/
RTGEOM* rt_dist2d_distancepoint(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode);
RTGEOM* rt_dist2d_distanceline(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode);


