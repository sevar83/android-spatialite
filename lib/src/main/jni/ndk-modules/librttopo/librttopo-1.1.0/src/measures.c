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
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2010 Nicklas Avén
 * Copyright 2012 Paul Ramsey
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <string.h>
#include <stdlib.h>

#include "measures.h"
#include "rtgeom_log.h"



/*------------------------------------------------------------------------------------------------------------
Initializing functions
The functions starting the distance-calculation processses
--------------------------------------------------------------------------------------------------------------*/

RTGEOM *
rtgeom_closest_line(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distanceline(ctx, rt1, rt2, rt1->srid, DIST_MIN);
}

RTGEOM *
rtgeom_furthest_line(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distanceline(ctx, rt1, rt2, rt1->srid, DIST_MAX);
}

RTGEOM *
rtgeom_closest_point(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distancepoint(ctx, rt1, rt2, rt1->srid, DIST_MIN);
}

RTGEOM *
rtgeom_furthest_point(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distancepoint(ctx, rt1, rt2, rt1->srid, DIST_MAX);
}


void
rt_dist2d_distpts_init(const RTCTX *ctx, DISTPTS *dl, int mode)
{
  dl->twisted = -1;
  dl->p1.x = dl->p1.y = 0.0;
  dl->p2.x = dl->p2.y = 0.0;
  dl->mode = mode;
  dl->tolerance = 0.0;
  if ( mode == DIST_MIN )
    dl->distance = FLT_MAX;
  else
    dl->distance = -1 * FLT_MAX;
}

/**
Function initializing shortestline and longestline calculations.
*/
RTGEOM *
rt_dist2d_distanceline(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode)
{
  double x1,x2,y1,y2;

  double initdistance = ( mode == DIST_MIN ? FLT_MAX : -1.0);
  DISTPTS thedl;
  RTPOINT *rtpoints[2];
  RTGEOM *result;

  thedl.mode = mode;
  thedl.distance = initdistance;
  thedl.tolerance = 0.0;

  RTDEBUG(ctx, 2, "rt_dist2d_distanceline is called");

  if (!rt_dist2d_comp(ctx,  rt1,rt2,&thedl))
  {
    /*should never get here. all cases ought to be error handled earlier*/
    rterror(ctx, "Some unspecified error.");
    result = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, srid, 0, 0);
  }

  /*if thedl.distance is unchanged there where only empty geometries input*/
  if (thedl.distance == initdistance)
  {
    RTDEBUG(ctx, 3, "didn't find geometries to measure between, returning null");
    result = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, srid, 0, 0);
  }
  else
  {
    x1=thedl.p1.x;
    y1=thedl.p1.y;
    x2=thedl.p2.x;
    y2=thedl.p2.y;

    rtpoints[0] = rtpoint_make2d(ctx, srid, x1, y1);
    rtpoints[1] = rtpoint_make2d(ctx, srid, x2, y2);

    result = (RTGEOM *)rtline_from_ptarray(ctx, srid, 2, rtpoints);
  }
  return result;
}

/**
Function initializing closestpoint calculations.
*/
RTGEOM *
rt_dist2d_distancepoint(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2,int srid,int mode)
{
  double x,y;
  DISTPTS thedl;
  double initdistance = FLT_MAX;
  RTGEOM *result;

  thedl.mode = mode;
  thedl.distance= initdistance;
  thedl.tolerance = 0;

  RTDEBUG(ctx, 2, "rt_dist2d_distancepoint is called");

  if (!rt_dist2d_comp(ctx,  rt1,rt2,&thedl))
  {
    /*should never get here. all cases ought to be error handled earlier*/
    rterror(ctx, "Some unspecified error.");
    result = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, srid, 0, 0);
  }
  if (thedl.distance == initdistance)
  {
    RTDEBUG(ctx, 3, "didn't find geometries to measure between, returning null");
    result = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, srid, 0, 0);
  }
  else
  {
    x=thedl.p1.x;
    y=thedl.p1.y;
    result = (RTGEOM *)rtpoint_make2d(ctx, srid, x, y);
  }
  return result;
}


/**
Function initialazing max distance calculation
*/
double
rtgeom_maxdistance2d(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  RTDEBUG(ctx, 2, "rtgeom_maxdistance2d is called");

  return rtgeom_maxdistance2d_tolerance(ctx,  rt1, rt2, 0.0 );
}

/**
Function handling max distance calculations and dfyllywithin calculations.
The difference is just the tolerance.
*/
double
rtgeom_maxdistance2d_tolerance(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
  /*double thedist;*/
  DISTPTS thedl;
  RTDEBUG(ctx, 2, "rtgeom_maxdistance2d_tolerance is called");
  thedl.mode = DIST_MAX;
  thedl.distance= -1;
  thedl.tolerance = tolerance;
  if (rt_dist2d_comp(ctx,  rt1,rt2,&thedl))
  {
    return thedl.distance;
  }
  /*should never get here. all cases ought to be error handled earlier*/
  rterror(ctx, "Some unspecified error.");
  return -1;
}

/**
  Function initialazing min distance calculation
*/
double
rtgeom_mindistance2d(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2)
{
  RTDEBUG(ctx, 2, "rtgeom_mindistance2d is called");
  return rtgeom_mindistance2d_tolerance(ctx,  rt1, rt2, 0.0 );
}

/**
  Function handling min distance calculations and dwithin calculations.
  The difference is just the tolerance.
*/
double
rtgeom_mindistance2d_tolerance(const RTCTX *ctx, const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
  DISTPTS thedl;
  RTDEBUG(ctx, 2, "rtgeom_mindistance2d_tolerance is called");
  thedl.mode = DIST_MIN;
  thedl.distance= FLT_MAX;
  thedl.tolerance = tolerance;
  if (rt_dist2d_comp(ctx,  rt1,rt2,&thedl))
  {
    return thedl.distance;
  }
  /*should never get here. all cases ought to be error handled earlier*/
  rterror(ctx, "Some unspecified error.");
  return FLT_MAX;
}


/*------------------------------------------------------------------------------------------------------------
End of Initializing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Preprocessing functions
Functions preparing geometries for distance-calculations
--------------------------------------------------------------------------------------------------------------*/

/**
  This function just deserializes geometries
  Bboxes is not checked here since it is the subgeometries
  bboxes we will use anyway.
*/
int
rt_dist2d_comp(const RTCTX *ctx, const RTGEOM *rt1,const RTGEOM *rt2, DISTPTS *dl)
{
  RTDEBUG(ctx, 2, "rt_dist2d_comp is called");

  return rt_dist2d_recursive(ctx, rt1, rt2, dl);
}

static int
rt_dist2d_is_collection(const RTCTX *ctx, const RTGEOM *g)
{

  switch (g->type)
  {
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOLLECTIONTYPE:
  case RTMULTICURVETYPE:
  case RTMULTISURFACETYPE:
  case RTCOMPOUNDTYPE:
  case RTPOLYHEDRALSURFACETYPE:
    return RT_TRUE;
    break;

  default:
    return RT_FALSE;
  }
}

/**
This is a recursive function delivering every possible combinatin of subgeometries
*/
int rt_dist2d_recursive(const RTCTX *ctx, const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl)
{
  int i, j;
  int n1=1;
  int n2=1;
  RTGEOM *g1 = NULL;
  RTGEOM *g2 = NULL;
  RTCOLLECTION *c1 = NULL;
  RTCOLLECTION *c2 = NULL;

  RTDEBUGF(ctx, 2, "rt_dist2d_comp is called with type1=%d, type2=%d", rtg1->type, rtg2->type);

  if (rt_dist2d_is_collection(ctx, rtg1))
  {
    RTDEBUG(ctx, 3, "First geometry is collection");
    c1 = rtgeom_as_rtcollection(ctx, rtg1);
    n1 = c1->ngeoms;
  }
  if (rt_dist2d_is_collection(ctx, rtg2))
  {
    RTDEBUG(ctx, 3, "Second geometry is collection");
    c2 = rtgeom_as_rtcollection(ctx, rtg2);
    n2 = c2->ngeoms;
  }

  for ( i = 0; i < n1; i++ )
  {

    if (rt_dist2d_is_collection(ctx, rtg1))
    {
      g1 = c1->geoms[i];
    }
    else
    {
      g1 = (RTGEOM*)rtg1;
    }

    if (rtgeom_is_empty(ctx, g1)) return RT_TRUE;

    if (rt_dist2d_is_collection(ctx, g1))
    {
      RTDEBUG(ctx, 3, "Found collection inside first geometry collection, recursing");
      if (!rt_dist2d_recursive(ctx, g1, rtg2, dl)) return RT_FALSE;
      continue;
    }
    for ( j = 0; j < n2; j++ )
    {
      if (rt_dist2d_is_collection(ctx, rtg2))
      {
        g2 = c2->geoms[j];
      }
      else
      {
        g2 = (RTGEOM*)rtg2;
      }
      if (rt_dist2d_is_collection(ctx, g2))
      {
        RTDEBUG(ctx, 3, "Found collection inside second geometry collection, recursing");
        if (!rt_dist2d_recursive(ctx, g1, g2, dl)) return RT_FALSE;
        continue;
      }

      if ( ! g1->bbox )
      {
        rtgeom_add_bbox(ctx, g1);
      }
      if ( ! g2->bbox )
      {
        rtgeom_add_bbox(ctx, g2);
      }

      /*If one of geometries is empty, return. True here only means continue searching. False would have stoped the process*/
      if (rtgeom_is_empty(ctx, g1)||rtgeom_is_empty(ctx, g2)) return RT_TRUE;

      if ( (dl->mode != DIST_MAX) &&
         (! rt_dist2d_check_overlap(ctx, g1, g2)) &&
           (g1->type == RTLINETYPE || g1->type == RTPOLYGONTYPE) &&
           (g2->type == RTLINETYPE || g2->type == RTPOLYGONTYPE) )
      {
        if (!rt_dist2d_distribute_fast(ctx, g1, g2, dl)) return RT_FALSE;
      }
      else
      {
        if (!rt_dist2d_distribute_bruteforce(ctx, g1, g2, dl)) return RT_FALSE;
        if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
      }
    }
  }
  return RT_TRUE;
}


int
rt_dist2d_distribute_bruteforce(const RTCTX *ctx, const RTGEOM *rtg1,const RTGEOM *rtg2, DISTPTS *dl)
{

  int  t1 = rtg1->type;
  int  t2 = rtg2->type;

  switch ( t1 )
  {
    case RTPOINTTYPE:
    {
      dl->twisted = 1;
      switch ( t2 )
      {
        case RTPOINTTYPE:
          return rt_dist2d_point_point(ctx, (RTPOINT *)rtg1, (RTPOINT *)rtg2, dl);
        case RTLINETYPE:
          return rt_dist2d_point_line(ctx, (RTPOINT *)rtg1, (RTLINE *)rtg2, dl);
        case RTPOLYGONTYPE:
          return rt_dist2d_point_poly(ctx, (RTPOINT *)rtg1, (RTPOLY *)rtg2, dl);
        case RTCIRCSTRINGTYPE:
          return rt_dist2d_point_circstring(ctx, (RTPOINT *)rtg1, (RTCIRCSTRING *)rtg2, dl);
        case RTCURVEPOLYTYPE:
          return rt_dist2d_point_curvepoly(ctx, (RTPOINT *)rtg1, (RTCURVEPOLY *)rtg2, dl);
        default:
          rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t2));
      }
    }
    case RTLINETYPE:
    {
      dl->twisted = 1;
      switch ( t2 )
      {
        case RTPOINTTYPE:
          dl->twisted=(-1);
          return rt_dist2d_point_line(ctx, (RTPOINT *)rtg2, (RTLINE *)rtg1, dl);
        case RTLINETYPE:
          return rt_dist2d_line_line(ctx, (RTLINE *)rtg1, (RTLINE *)rtg2, dl);
        case RTPOLYGONTYPE:
          return rt_dist2d_line_poly(ctx, (RTLINE *)rtg1, (RTPOLY *)rtg2, dl);
        case RTCIRCSTRINGTYPE:
          return rt_dist2d_line_circstring(ctx, (RTLINE *)rtg1, (RTCIRCSTRING *)rtg2, dl);
        case RTCURVEPOLYTYPE:
          return rt_dist2d_line_curvepoly(ctx, (RTLINE *)rtg1, (RTCURVEPOLY *)rtg2, dl);
        default:
          rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t2));
      }
    }
    case RTCIRCSTRINGTYPE:
    {
      dl->twisted = 1;
      switch ( t2 )
      {
        case RTPOINTTYPE:
          dl->twisted = -1;
          return rt_dist2d_point_circstring(ctx, (RTPOINT *)rtg2, (RTCIRCSTRING *)rtg1, dl);
        case RTLINETYPE:
          dl->twisted = -1;
          return rt_dist2d_line_circstring(ctx, (RTLINE *)rtg2, (RTCIRCSTRING *)rtg1, dl);
        case RTPOLYGONTYPE:
          return rt_dist2d_circstring_poly(ctx, (RTCIRCSTRING *)rtg1, (RTPOLY *)rtg2, dl);
        case RTCIRCSTRINGTYPE:
          return rt_dist2d_circstring_circstring(ctx, (RTCIRCSTRING *)rtg1, (RTCIRCSTRING *)rtg2, dl);
        case RTCURVEPOLYTYPE:
          return rt_dist2d_circstring_curvepoly(ctx, (RTCIRCSTRING *)rtg1, (RTCURVEPOLY *)rtg2, dl);
        default:
          rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t2));
      }
    }
    case RTPOLYGONTYPE:
    {
      dl->twisted = -1;
      switch ( t2 )
      {
        case RTPOINTTYPE:
          return rt_dist2d_point_poly(ctx, (RTPOINT *)rtg2, (RTPOLY *)rtg1, dl);
        case RTLINETYPE:
          return rt_dist2d_line_poly(ctx, (RTLINE *)rtg2, (RTPOLY *)rtg1, dl);
        case RTCIRCSTRINGTYPE:
          return rt_dist2d_circstring_poly(ctx, (RTCIRCSTRING *)rtg2, (RTPOLY *)rtg1, dl);
        case RTPOLYGONTYPE:
          dl->twisted = 1;
          return rt_dist2d_poly_poly(ctx, (RTPOLY *)rtg1, (RTPOLY *)rtg2, dl);
        case RTCURVEPOLYTYPE:
          dl->twisted = 1;
          return rt_dist2d_poly_curvepoly(ctx, (RTPOLY *)rtg1, (RTCURVEPOLY *)rtg2, dl);
        default:
          rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t2));
      }
    }
    case RTCURVEPOLYTYPE:
    {
      dl->twisted = (-1);
      switch ( t2 )
      {
        case RTPOINTTYPE:
          return rt_dist2d_point_curvepoly(ctx, (RTPOINT *)rtg2, (RTCURVEPOLY *)rtg1, dl);
        case RTLINETYPE:
          return rt_dist2d_line_curvepoly(ctx, (RTLINE *)rtg2, (RTCURVEPOLY *)rtg1, dl);
        case RTPOLYGONTYPE:
          return rt_dist2d_poly_curvepoly(ctx, (RTPOLY *)rtg2, (RTCURVEPOLY *)rtg1, dl);
        case RTCIRCSTRINGTYPE:
          return rt_dist2d_circstring_curvepoly(ctx, (RTCIRCSTRING *)rtg2, (RTCURVEPOLY *)rtg1, dl);
        case RTCURVEPOLYTYPE:
          dl->twisted = 1;
          return rt_dist2d_curvepoly_curvepoly(ctx, (RTCURVEPOLY *)rtg1, (RTCURVEPOLY *)rtg2, dl);
        default:
          rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t2));
      }
    }
    default:
    {
      rterror(ctx, "Unsupported geometry type: %s", rttype_name(ctx, t1));
    }
  }

  /*You shouldn't being able to get here*/
  rterror(ctx, "unspecified error in function rt_dist2d_distribute_bruteforce");
  return RT_FALSE;
}




/**

We have to check for overlapping bboxes
*/
int
rt_dist2d_check_overlap(const RTCTX *ctx, RTGEOM *rtg1,RTGEOM *rtg2)
{
  RTDEBUG(ctx, 2, "rt_dist2d_check_overlap is called");
  if ( ! rtg1->bbox )
    rtgeom_calculate_gbox(ctx, rtg1, rtg1->bbox);
  if ( ! rtg2->bbox )
    rtgeom_calculate_gbox(ctx, rtg2, rtg2->bbox);

  /*Check if the geometries intersect.
  */
  if ((rtg1->bbox->xmax<rtg2->bbox->xmin||rtg1->bbox->xmin>rtg2->bbox->xmax||rtg1->bbox->ymax<rtg2->bbox->ymin||rtg1->bbox->ymin>rtg2->bbox->ymax))
  {
    RTDEBUG(ctx, 3, "geometries bboxes did not overlap");
    return RT_FALSE;
  }
  RTDEBUG(ctx, 3, "geometries bboxes overlap");
  return RT_TRUE;
}

/**

Here the geometries are distributed for the new faster distance-calculations
*/
int
rt_dist2d_distribute_fast(const RTCTX *ctx, RTGEOM *rtg1, RTGEOM *rtg2, DISTPTS *dl)
{
  RTPOINTARRAY *pa1, *pa2;
  int  type1 = rtg1->type;
  int  type2 = rtg2->type;

  RTDEBUGF(ctx, 2, "rt_dist2d_distribute_fast is called with typ1=%d, type2=%d", rtg1->type, rtg2->type);

  switch (type1)
  {
  case RTLINETYPE:
    pa1 = ((RTLINE *)rtg1)->points;
    break;
  case RTPOLYGONTYPE:
    pa1 = ((RTPOLY *)rtg1)->rings[0];
    break;
  default:
    rterror(ctx, "Unsupported geometry1 type: %s", rttype_name(ctx, type1));
    return RT_FALSE;
  }
  switch (type2)
  {
  case RTLINETYPE:
    pa2 = ((RTLINE *)rtg2)->points;
    break;
  case RTPOLYGONTYPE:
    pa2 = ((RTPOLY *)rtg2)->rings[0];
    break;
  default:
    rterror(ctx, "Unsupported geometry2 type: %s", rttype_name(ctx, type1));
    return RT_FALSE;
  }
  dl->twisted=1;
  return rt_dist2d_fast_ptarray_ptarray(ctx, pa1, pa2, dl, rtg1->bbox, rtg2->bbox);
}

/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Brute force functions
The old way of calculating distances, now used for:
1)  distances to points (because there shouldn't be anything to gain by the new way of doing it)
2)  distances when subgeometries geometries bboxes overlaps
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
rt_dist2d_point_point(const RTCTX *ctx, RTPOINT *point1, RTPOINT *point2, DISTPTS *dl)
{
  const RTPOINT2D *p1, *p2;

  p1 = rt_getPoint2d_cp(ctx, point1->point, 0);
  p2 = rt_getPoint2d_cp(ctx, point2->point, 0);

  return rt_dist2d_pt_pt(ctx, p1, p2, dl);
}
/**

point to line calculation
*/
int
rt_dist2d_point_line(const RTCTX *ctx, RTPOINT *point, RTLINE *line, DISTPTS *dl)
{
  const RTPOINT2D *p;
  RTDEBUG(ctx, 2, "rt_dist2d_point_line is called");
  p = rt_getPoint2d_cp(ctx, point->point, 0);
  return rt_dist2d_pt_ptarray(ctx, p, line->points, dl);
}

int
rt_dist2d_point_circstring(const RTCTX *ctx, RTPOINT *point, RTCIRCSTRING *circ, DISTPTS *dl)
{
  const RTPOINT2D *p;
  p = rt_getPoint2d_cp(ctx, point->point, 0);
  return rt_dist2d_pt_ptarrayarc(ctx, p, circ->points, dl);
}

/**
 * 1. see if pt in outer boundary. if no, then treat the outer ring like a line
 * 2. if in the boundary, test to see if its in a hole.
 *    if so, then return dist to hole, else return 0 (point in polygon)
 */
int
rt_dist2d_point_poly(const RTCTX *ctx, RTPOINT *point, RTPOLY *poly, DISTPTS *dl)
{
  const RTPOINT2D *p;
  int i;

  RTDEBUG(ctx, 2, "rt_dist2d_point_poly called");

  p = rt_getPoint2d_cp(ctx, point->point, 0);

  if (dl->mode == DIST_MAX)
  {
    RTDEBUG(ctx, 3, "looking for maxdistance");
    return rt_dist2d_pt_ptarray(ctx, p, poly->rings[0], dl);
  }
  /* Return distance to outer ring if not inside it */
  if ( ptarray_contains_point(ctx, poly->rings[0], p) == RT_OUTSIDE )
  {
    RTDEBUG(ctx, 3, "first point not inside outer-ring");
    return rt_dist2d_pt_ptarray(ctx, p, poly->rings[0], dl);
  }

  /*
   * Inside the outer ring.
   * Scan though each of the inner rings looking to
   * see if its inside.  If not, distance==0.
   * Otherwise, distance = pt to ring distance
   */
  for ( i = 1;  i < poly->nrings; i++)
  {
    /* Inside a hole. Distance = pt -> ring */
    if ( ptarray_contains_point(ctx, poly->rings[i], p) != RT_OUTSIDE )
    {
      RTDEBUG(ctx, 3, " inside an hole");
      return rt_dist2d_pt_ptarray(ctx, p, poly->rings[i], dl);
    }
  }

  RTDEBUG(ctx, 3, " inside the polygon");
  if (dl->mode == DIST_MIN)
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = p->x;
    dl->p1.y = dl->p2.y = p->y;
  }
  return RT_TRUE; /* Is inside the polygon */
}

int
rt_dist2d_point_curvepoly(const RTCTX *ctx, RTPOINT *point, RTCURVEPOLY *poly, DISTPTS *dl)
{
  const RTPOINT2D *p;
  int i;

  p = rt_getPoint2d_cp(ctx, point->point, 0);

  if (dl->mode == DIST_MAX)
    rterror(ctx, "rt_dist2d_point_curvepoly cannot calculate max distance");

  /* Return distance to outer ring if not inside it */
  if ( rtgeom_contains_point(ctx, poly->rings[0], p) == RT_OUTSIDE )
  {
    return rt_dist2d_recursive(ctx, (RTGEOM*)point, poly->rings[0], dl);
  }

  /*
   * Inside the outer ring.
   * Scan though each of the inner rings looking to
   * see if its inside.  If not, distance==0.
   * Otherwise, distance = pt to ring distance
   */
  for ( i = 1;  i < poly->nrings; i++)
  {
    /* Inside a hole. Distance = pt -> ring */
    if ( rtgeom_contains_point(ctx, poly->rings[i], p) != RT_OUTSIDE )
    {
      RTDEBUG(ctx, 3, " inside a hole");
      return rt_dist2d_recursive(ctx, (RTGEOM*)point, poly->rings[i], dl);
    }
  }

  RTDEBUG(ctx, 3, " inside the polygon");
  if (dl->mode == DIST_MIN)
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = p->x;
    dl->p1.y = dl->p2.y = p->y;
  }

  return RT_TRUE; /* Is inside the polygon */
}

/**

line to line calculation
*/
int
rt_dist2d_line_line(const RTCTX *ctx, RTLINE *line1, RTLINE *line2, DISTPTS *dl)
{
  RTPOINTARRAY *pa1 = line1->points;
  RTPOINTARRAY *pa2 = line2->points;
  RTDEBUG(ctx, 2, "rt_dist2d_line_line is called");
  return rt_dist2d_ptarray_ptarray(ctx, pa1, pa2, dl);
}

int
rt_dist2d_line_circstring(const RTCTX *ctx, RTLINE *line1, RTCIRCSTRING *line2, DISTPTS *dl)
{
  return rt_dist2d_ptarray_ptarrayarc(ctx, line1->points, line2->points, dl);
}

/**
 * line to polygon calculation
 * Brute force.
 * Test line-ring distance against each ring.
 * If there's an intersection (distance==0) then return 0 (crosses boundary).
 * Otherwise, test to see if any point is inside outer rings of polygon,
 * but not in inner rings.
 * If so, return 0  (line inside polygon),
 * otherwise return min distance to a ring (could be outside
 * polygon or inside a hole)
 */
int
rt_dist2d_line_poly(const RTCTX *ctx, RTLINE *line, RTPOLY *poly, DISTPTS *dl)
{
  const RTPOINT2D *pt;
  int i;

  RTDEBUGF(ctx, 2, "rt_dist2d_line_poly called (%d rings)", poly->nrings);

  pt = rt_getPoint2d_cp(ctx, line->points, 0);
  if ( ptarray_contains_point(ctx, poly->rings[0], pt) == RT_OUTSIDE )
  {
    return rt_dist2d_ptarray_ptarray(ctx, line->points, poly->rings[0], dl);
  }

  for (i=1; i<poly->nrings; i++)
  {
    if (!rt_dist2d_ptarray_ptarray(ctx, line->points, poly->rings[i], dl)) return RT_FALSE;

    RTDEBUGF(ctx, 3, " distance from ring %d: %f, mindist: %f",
             i, dl->distance, dl->tolerance);
    /* just a check if  the answer is already given */
    if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE;
  }

  /*
   * No intersection, have to check if a point is
   * inside polygon
   */
  pt = rt_getPoint2d_cp(ctx, line->points, 0);

  /*
   * Outside outer ring, so min distance to a ring
   * is the actual min distance

  if ( ! pt_in_ring_2d(ctx, &pt, poly->rings[0]) )
  {
    return ;
  } */

  /*
   * Its in the outer ring.
   * Have to check if its inside a hole
   */
  for (i=1; i<poly->nrings; i++)
  {
    if ( ptarray_contains_point(ctx, poly->rings[i], pt) != RT_OUTSIDE )
    {
      /*
       * Its inside a hole, then the actual
       * distance is the min ring distance
       */
      return RT_TRUE;
    }
  }
  if (dl->mode == DIST_MIN)
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
  }
  return RT_TRUE; /* Not in hole, so inside polygon */
}

int
rt_dist2d_line_curvepoly(const RTCTX *ctx, RTLINE *line, RTCURVEPOLY *poly, DISTPTS *dl)
{
  const RTPOINT2D *pt = rt_getPoint2d_cp(ctx, line->points, 0);
  int i;

  if ( rtgeom_contains_point(ctx, poly->rings[0], pt) == RT_OUTSIDE )
  {
    return rt_dist2d_recursive(ctx, (RTGEOM*)line, poly->rings[0], dl);
  }

  for ( i = 1; i < poly->nrings; i++ )
  {
    if ( ! rt_dist2d_recursive(ctx, (RTGEOM*)line, poly->rings[i], dl) )
      return RT_FALSE;

    if ( dl->distance<=dl->tolerance && dl->mode == DIST_MIN )
      return RT_TRUE;
  }

  for ( i=1; i < poly->nrings; i++ )
  {
    if ( rtgeom_contains_point(ctx, poly->rings[i],pt) != RT_OUTSIDE )
    {
      /* Its inside a hole, then the actual */
      return RT_TRUE;
    }
  }

  if (dl->mode == DIST_MIN)
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
  }

  return RT_TRUE; /* Not in hole, so inside polygon */
}

/**
Function handling polygon to polygon calculation
1  if we are looking for maxdistance, just check the outer rings.
2  check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
3  check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1
4  check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2
5  If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.
 */
int
rt_dist2d_poly_poly(const RTCTX *ctx, RTPOLY *poly1, RTPOLY *poly2, DISTPTS *dl)
{

  const RTPOINT2D *pt;
  int i;

  RTDEBUG(ctx, 2, "rt_dist2d_poly_poly called");

  /*1  if we are looking for maxdistance, just check the outer rings.*/
  if (dl->mode == DIST_MAX)
  {
    return rt_dist2d_ptarray_ptarray(ctx, poly1->rings[0], poly2->rings[0], dl);
  }


  /* 2  check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
  here it would be possible to handle the information about wich one is inside wich one and only search for the smaller ones in the bigger ones holes.*/
  pt = rt_getPoint2d_cp(ctx, poly1->rings[0], 0);
  if ( ptarray_contains_point(ctx, poly2->rings[0], pt) == RT_OUTSIDE )
  {
    pt = rt_getPoint2d_cp(ctx, poly2->rings[0], 0);
    if ( ptarray_contains_point(ctx, poly1->rings[0], pt) == RT_OUTSIDE )
    {
      return rt_dist2d_ptarray_ptarray(ctx, poly1->rings[0], poly2->rings[0], dl);
    }
  }

  /*3  check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1*/
  pt = rt_getPoint2d_cp(ctx, poly2->rings[0], 0);
  for (i=1; i<poly1->nrings; i++)
  {
    /* Inside a hole */
    if ( ptarray_contains_point(ctx, poly1->rings[i], pt) != RT_OUTSIDE )
    {
      return rt_dist2d_ptarray_ptarray(ctx, poly1->rings[i], poly2->rings[0], dl);
    }
  }

  /*4  check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2*/
  pt = rt_getPoint2d_cp(ctx, poly1->rings[0], 0);
  for (i=1; i<poly2->nrings; i++)
  {
    /* Inside a hole */
    if ( ptarray_contains_point(ctx, poly2->rings[i], pt) != RT_OUTSIDE )
    {
      return rt_dist2d_ptarray_ptarray(ctx, poly1->rings[0], poly2->rings[i], dl);
    }
  }


  /*5  If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.*/
  pt = rt_getPoint2d_cp(ctx, poly1->rings[0], 0);
  if ( ptarray_contains_point(ctx, poly2->rings[0], pt) != RT_OUTSIDE )
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
    return RT_TRUE;
  }

  pt = rt_getPoint2d_cp(ctx, poly2->rings[0], 0);
  if ( ptarray_contains_point(ctx, poly1->rings[0], pt) != RT_OUTSIDE )
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
    return RT_TRUE;
  }


  rterror(ctx, "Unspecified error in function rt_dist2d_poly_poly");
  return RT_FALSE;
}

int
rt_dist2d_poly_curvepoly(const RTCTX *ctx, RTPOLY *poly1, RTCURVEPOLY *curvepoly2, DISTPTS *dl)
{
  RTCURVEPOLY *curvepoly1 = rtcurvepoly_construct_from_rtpoly(ctx, poly1);
  int rv = rt_dist2d_curvepoly_curvepoly(ctx, curvepoly1, curvepoly2, dl);
  rtgeom_free(ctx, (RTGEOM*)curvepoly1);
  return rv;
}

int
rt_dist2d_circstring_poly(const RTCTX *ctx, RTCIRCSTRING *circ, RTPOLY *poly, DISTPTS *dl)
{
  RTCURVEPOLY *curvepoly = rtcurvepoly_construct_from_rtpoly(ctx, poly);
  int rv = rt_dist2d_line_curvepoly(ctx, (RTLINE*)circ, curvepoly, dl);
  rtgeom_free(ctx, (RTGEOM*)curvepoly);
  return rv;
}


int
rt_dist2d_circstring_curvepoly(const RTCTX *ctx, RTCIRCSTRING *circ, RTCURVEPOLY *poly, DISTPTS *dl)
{
  return rt_dist2d_line_curvepoly(ctx, (RTLINE*)circ, poly, dl);
}

int
rt_dist2d_circstring_circstring(const RTCTX *ctx, RTCIRCSTRING *line1, RTCIRCSTRING *line2, DISTPTS *dl)
{
  return rt_dist2d_ptarrayarc_ptarrayarc(ctx, line1->points, line2->points, dl);
}

static const RTPOINT2D *
rt_curvering_getfirstpoint2d_cp(const RTCTX *ctx, RTGEOM *geom)
{
  switch( geom->type )
  {
    case RTLINETYPE:
      return rt_getPoint2d_cp(ctx, ((RTLINE*)geom)->points, 0);
    case RTCIRCSTRINGTYPE:
      return rt_getPoint2d_cp(ctx, ((RTCIRCSTRING*)geom)->points, 0);
    case RTCOMPOUNDTYPE:
    {
      RTCOMPOUND *comp = (RTCOMPOUND*)geom;
      RTLINE *line = (RTLINE*)(comp->geoms[0]);
      return rt_getPoint2d_cp(ctx, line->points, 0);
    }
    default:
      rterror(ctx, "rt_curvering_getfirstpoint2d_cp: unknown type");
  }
  return NULL;
}

int
rt_dist2d_curvepoly_curvepoly(const RTCTX *ctx, RTCURVEPOLY *poly1, RTCURVEPOLY *poly2, DISTPTS *dl)
{
  const RTPOINT2D *pt;
  int i;

  RTDEBUG(ctx, 2, "rt_dist2d_curvepoly_curvepoly called");

  /*1  if we are looking for maxdistance, just check the outer rings.*/
  if (dl->mode == DIST_MAX)
  {
    return rt_dist2d_recursive(ctx, poly1->rings[0],  poly2->rings[0], dl);
  }


  /* 2  check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
  here it would be possible to handle the information about wich one is inside wich one and only search for the smaller ones in the bigger ones holes.*/
  pt = rt_curvering_getfirstpoint2d_cp(ctx, poly1->rings[0]);
  if ( rtgeom_contains_point(ctx, poly2->rings[0], pt) == RT_OUTSIDE )
  {
    pt = rt_curvering_getfirstpoint2d_cp(ctx, poly2->rings[0]);
    if ( rtgeom_contains_point(ctx, poly1->rings[0], pt) == RT_OUTSIDE )
    {
      return rt_dist2d_recursive(ctx, poly1->rings[0], poly2->rings[0], dl);
    }
  }

  /*3  check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1*/
  pt = rt_curvering_getfirstpoint2d_cp(ctx, poly2->rings[0]);
  for (i = 1; i < poly1->nrings; i++)
  {
    /* Inside a hole */
    if ( rtgeom_contains_point(ctx, poly1->rings[i], pt) != RT_OUTSIDE )
    {
      return rt_dist2d_recursive(ctx, poly1->rings[i], poly2->rings[0], dl);
    }
  }

  /*4  check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2*/
  pt = rt_curvering_getfirstpoint2d_cp(ctx, poly1->rings[0]);
  for (i = 1; i < poly2->nrings; i++)
  {
    /* Inside a hole */
    if ( rtgeom_contains_point(ctx, poly2->rings[i], pt) != RT_OUTSIDE )
    {
      return rt_dist2d_recursive(ctx, poly1->rings[0],  poly2->rings[i], dl);
    }
  }


  /*5  If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.*/
  pt = rt_curvering_getfirstpoint2d_cp(ctx, poly1->rings[0]);
  if ( rtgeom_contains_point(ctx, poly2->rings[0], pt) != RT_OUTSIDE )
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
    return RT_TRUE;
  }

  pt = rt_curvering_getfirstpoint2d_cp(ctx, poly2->rings[0]);
  if ( rtgeom_contains_point(ctx, poly1->rings[0], pt) != RT_OUTSIDE )
  {
    dl->distance = 0.0;
    dl->p1.x = dl->p2.x = pt->x;
    dl->p1.y = dl->p2.y = pt->y;
    return RT_TRUE;
  }

  rterror(ctx, "Unspecified error in function rt_dist2d_curvepoly_curvepoly");
  return RT_FALSE;
}



/**
 * search all the segments of pointarray to see which one is closest to p1
 * Returns minimum distance between point and pointarray
 */
int
rt_dist2d_pt_ptarray(const RTCTX *ctx, const RTPOINT2D *p, RTPOINTARRAY *pa,DISTPTS *dl)
{
  int t;
  const RTPOINT2D *start, *end;
  int twist = dl->twisted;

  RTDEBUG(ctx, 2, "rt_dist2d_pt_ptarray is called");

  start = rt_getPoint2d_cp(ctx, pa, 0);

  if ( !rt_dist2d_pt_pt(ctx, p, start, dl) ) return RT_FALSE;

  for (t=1; t<pa->npoints; t++)
  {
    dl->twisted=twist;
    end = rt_getPoint2d_cp(ctx, pa, t);
    if (!rt_dist2d_pt_seg(ctx, p, start, end, dl)) return RT_FALSE;

    if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
    start = end;
  }

  return RT_TRUE;
}

/**
* Search all the arcs of pointarray to see which one is closest to p1
* Returns minimum distance between point and arc pointarray.
*/
int
rt_dist2d_pt_ptarrayarc(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINTARRAY *pa, DISTPTS *dl)
{
  int t;
  const RTPOINT2D *A1;
  const RTPOINT2D *A2;
  const RTPOINT2D *A3;
  int twist = dl->twisted;

  RTDEBUG(ctx, 2, "rt_dist2d_pt_ptarrayarc is called");

  if ( pa->npoints % 2 == 0 || pa->npoints < 3 )
  {
    rterror(ctx, "rt_dist2d_pt_ptarrayarc called with non-arc input");
    return RT_FALSE;
  }

  if (dl->mode == DIST_MAX)
  {
    rterror(ctx, "rt_dist2d_pt_ptarrayarc does not currently support DIST_MAX mode");
    return RT_FALSE;
  }

  A1 = rt_getPoint2d_cp(ctx, pa, 0);

  if ( ! rt_dist2d_pt_pt(ctx, p, A1, dl) )
    return RT_FALSE;

  for ( t=1; t<pa->npoints; t += 2 )
  {
    dl->twisted = twist;
    A2 = rt_getPoint2d_cp(ctx, pa, t);
    A3 = rt_getPoint2d_cp(ctx, pa, t+1);

    if ( rt_dist2d_pt_arc(ctx, p, A1, A2, A3, dl) == RT_FALSE )
      return RT_FALSE;

    if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN )
      return RT_TRUE; /*just a check if  the answer is already given*/

    A1 = A3;
  }

  return RT_TRUE;
}




/**
* test each segment of l1 against each segment of l2.
*/
int
rt_dist2d_ptarray_ptarray(const RTCTX *ctx, RTPOINTARRAY *l1, RTPOINTARRAY *l2,DISTPTS *dl)
{
  int t,u;
  const RTPOINT2D  *start, *end;
  const RTPOINT2D  *start2, *end2;
  int twist = dl->twisted;

  RTDEBUGF(ctx, 2, "rt_dist2d_ptarray_ptarray called (points: %d-%d)",l1->npoints, l2->npoints);

  if (dl->mode == DIST_MAX)/*If we are searching for maxdistance we go straight to point-point calculation since the maxdistance have to be between two vertexes*/
  {
    for (t=0; t<l1->npoints; t++) /*for each segment in L1 */
    {
      start = rt_getPoint2d_cp(ctx, l1, t);
      for (u=0; u<l2->npoints; u++) /*for each segment in L2 */
      {
        start2 = rt_getPoint2d_cp(ctx, l2, u);
        rt_dist2d_pt_pt(ctx, start, start2, dl);
        RTDEBUGF(ctx, 4, "maxdist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
        RTDEBUGF(ctx, 3, " seg%d-seg%d dist: %f, mindist: %f",
                 t, u, dl->distance, dl->tolerance);
      }
    }
  }
  else
  {
    start = rt_getPoint2d_cp(ctx, l1, 0);
    for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
    {
      end = rt_getPoint2d_cp(ctx, l1, t);
      start2 = rt_getPoint2d_cp(ctx, l2, 0);
      for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
      {
        end2 = rt_getPoint2d_cp(ctx, l2, u);
        dl->twisted=twist;
        rt_dist2d_seg_seg(ctx, start, end, start2, end2, dl);
        RTDEBUGF(ctx, 4, "mindist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
        RTDEBUGF(ctx, 3, " seg%d-seg%d dist: %f, mindist: %f",
                 t, u, dl->distance, dl->tolerance);
        if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
        start2 = end2;
      }
      start = end;
    }
  }
  return RT_TRUE;
}

/**
* Test each segment of pa against each arc of pb for distance.
*/
int
rt_dist2d_ptarray_ptarrayarc(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINTARRAY *pb, DISTPTS *dl)
{
  int t, u;
  const RTPOINT2D *A1;
  const RTPOINT2D *A2;
  const RTPOINT2D *B1;
  const RTPOINT2D *B2;
  const RTPOINT2D *B3;
  int twist = dl->twisted;

  RTDEBUGF(ctx, 2, "rt_dist2d_ptarray_ptarrayarc called (points: %d-%d)",pa->npoints, pb->npoints);

  if ( pb->npoints % 2 == 0 || pb->npoints < 3 )
  {
    rterror(ctx, "rt_dist2d_ptarray_ptarrayarc called with non-arc input");
    return RT_FALSE;
  }

  if ( dl->mode == DIST_MAX )
  {
    rterror(ctx, "rt_dist2d_ptarray_ptarrayarc does not currently support DIST_MAX mode");
    return RT_FALSE;
  }
  else
  {
    A1 = rt_getPoint2d_cp(ctx, pa, 0);
    for ( t=1; t < pa->npoints; t++ ) /* For each segment in pa */
    {
      A2 = rt_getPoint2d_cp(ctx, pa, t);
      B1 = rt_getPoint2d_cp(ctx, pb, 0);
      for ( u=1; u < pb->npoints; u += 2 ) /* For each arc in pb */
      {
        B2 = rt_getPoint2d_cp(ctx, pb, u);
        B3 = rt_getPoint2d_cp(ctx, pb, u+1);
        dl->twisted = twist;

        rt_dist2d_seg_arc(ctx, A1, A2, B1, B2, B3, dl);

        /* If we've found a distance within tolerance, we're done */
        if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN )
          return RT_TRUE;

        B1 = B3;
      }
      A1 = A2;
    }
  }
  return RT_TRUE;
}

/**
* Test each arc of pa against each arc of pb for distance.
*/
int
rt_dist2d_ptarrayarc_ptarrayarc(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINTARRAY *pb, DISTPTS *dl)
{
  int t, u;
  const RTPOINT2D *A1;
  const RTPOINT2D *A2;
  const RTPOINT2D *A3;
  const RTPOINT2D *B1;
  const RTPOINT2D *B2;
  const RTPOINT2D *B3;
  int twist = dl->twisted;

  RTDEBUGF(ctx, 2, "rt_dist2d_ptarrayarc_ptarrayarc called (points: %d-%d)",pa->npoints, pb->npoints);

  if (dl->mode == DIST_MAX)
  {
    rterror(ctx, "rt_dist2d_ptarrayarc_ptarrayarc does not currently support DIST_MAX mode");
    return RT_FALSE;
  }
  else
  {
    A1 = rt_getPoint2d_cp(ctx, pa, 0);
    for ( t=1; t < pa->npoints; t += 2 ) /* For each segment in pa */
    {
      A2 = rt_getPoint2d_cp(ctx, pa, t);
      A3 = rt_getPoint2d_cp(ctx, pa, t+1);
      B1 = rt_getPoint2d_cp(ctx, pb, 0);
      for ( u=1; u < pb->npoints; u += 2 ) /* For each arc in pb */
      {
        B2 = rt_getPoint2d_cp(ctx, pb, u);
        B3 = rt_getPoint2d_cp(ctx, pb, u+1);
        dl->twisted = twist;

        rt_dist2d_arc_arc(ctx, A1, A2, A3, B1, B2, B3, dl);

        /* If we've found a distance within tolerance, we're done */
        if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN )
          return RT_TRUE;

        B1 = B3;
      }
      A1 = A3;
    }
  }
  return RT_TRUE;
}

/**
* Calculate the shortest distance between an arc and an edge.
* Line/circle approach from http://stackoverflow.com/questions/1073336/circle-line-collision-detection
*/
int
rt_dist2d_seg_arc(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *B1, const RTPOINT2D *B2, const RTPOINT2D *B3, DISTPTS *dl)
{
  RTPOINT2D C; /* center of arc circle */
  double radius_C; /* radius of arc circle */
  RTPOINT2D D; /* point on A closest to C */
  double dist_C_D; /* distance from C to D */
  int pt_in_arc, pt_in_seg;
  DISTPTS dltmp;

  /* Bail out on crazy modes */
  if ( dl->mode < 0 )
    rterror(ctx, "rt_dist2d_seg_arc does not support maxdistance mode");

  /* What if the "arc" is a point? */
  if ( rt_arc_is_pt(ctx, B1, B2, B3) )
    return rt_dist2d_pt_seg(ctx, B1, A1, A2, dl);

  /* Calculate center and radius of the circle. */
  radius_C = rt_arc_center(ctx, B1, B2, B3, &C);

  /* This "arc" is actually a line (B2 is colinear with B1,B3) */
  if ( radius_C < 0.0 )
    return rt_dist2d_seg_seg(ctx, A1, A2, B1, B3, dl);

  /* Calculate distance between the line and circle center */
  rt_dist2d_distpts_init(ctx, &dltmp, DIST_MIN);
  if ( rt_dist2d_pt_seg(ctx, &C, A1, A2, &dltmp) == RT_FALSE )
    rterror(ctx, "rt_dist2d_pt_seg failed in rt_dist2d_seg_arc");

  D = dltmp.p1;
  dist_C_D = dltmp.distance;

  /* Line intersects circle, maybe arc intersects edge? */
  /* If so, that's the closest point. */
  /* If not, the closest point is one of the end points of A */
  if ( dist_C_D < radius_C )
  {
    double length_A; /* length of the segment A */
    RTPOINT2D E, F; /* points of interection of edge A and circle(B) */
    double dist_D_EF; /* distance from D to E or F (same distance both ways) */

    dist_D_EF = sqrt(radius_C*radius_C - dist_C_D*dist_C_D);
    length_A = sqrt((A2->x-A1->x)*(A2->x-A1->x)+(A2->y-A1->y)*(A2->y-A1->y));

    /* Point of intersection E */
    E.x = D.x - (A2->x-A1->x) * dist_D_EF / length_A;
    E.y = D.y - (A2->y-A1->y) * dist_D_EF / length_A;
    /* Point of intersection F */
    F.x = D.x + (A2->x-A1->x) * dist_D_EF / length_A;
    F.y = D.y + (A2->y-A1->y) * dist_D_EF / length_A;


    /* If E is within A and within B then it's an interesction point */
    pt_in_arc = rt_pt_in_arc(ctx, &E, B1, B2, B3);
    pt_in_seg = rt_pt_in_seg(ctx, &E, A1, A2);

    if ( pt_in_arc && pt_in_seg )
    {
      dl->distance = 0.0;
      dl->p1 = E;
      dl->p2 = E;
      return RT_TRUE;
    }

    /* If F is within A and within B then it's an interesction point */
    pt_in_arc = rt_pt_in_arc(ctx, &F, B1, B2, B3);
    pt_in_seg = rt_pt_in_seg(ctx, &F, A1, A2);

    if ( pt_in_arc && pt_in_seg )
    {
      dl->distance = 0.0;
      dl->p1 = F;
      dl->p2 = F;
      return RT_TRUE;
    }
  }

  /* Line grazes circle, maybe arc intersects edge? */
  /* If so, grazing point is the closest point. */
  /* If not, the closest point is one of the end points of A */
  else if ( dist_C_D == radius_C )
  {
    /* Closest point D is also the point of grazing */
    pt_in_arc = rt_pt_in_arc(ctx, &D, B1, B2, B3);
    pt_in_seg = rt_pt_in_seg(ctx, &D, A1, A2);

    /* Is D contained in both A and B? */
    if ( pt_in_arc && pt_in_seg )
    {
      dl->distance = 0.0;
      dl->p1 = D;
      dl->p2 = D;
      return RT_TRUE;
    }
  }
  /* Line misses circle. */
  /* If closest point to A on circle is within B, then that's the closest */
  /* Otherwise, the closest point will be an end point of A */
  else
  {
    RTPOINT2D G; /* Point on circle closest to A */
    G.x = C.x + (D.x-C.x) * radius_C / dist_C_D;
    G.y = C.y + (D.y-C.y) * radius_C / dist_C_D;

    pt_in_arc = rt_pt_in_arc(ctx, &G, B1, B2, B3);
    pt_in_seg = rt_pt_in_seg(ctx, &D, A1, A2);

    /* Closest point is on the interior of A and B */
    if ( pt_in_arc && pt_in_seg )
      return rt_dist2d_pt_pt(ctx, &D, &G, dl);

  }

  /* Now we test the many combinations of end points with either */
  /* arcs or edges. Each previous check determined if the closest */
  /* potential point was within the arc/segment inscribed on the */
  /* line/circle holding the arc/segment. */

  /* Closest point is in the arc, but not in the segment, so */
  /* one of the segment end points must be the closest. */
  if ( pt_in_arc & ! pt_in_seg )
  {
    rt_dist2d_pt_arc(ctx, A1, B1, B2, B3, dl);
    rt_dist2d_pt_arc(ctx, A2, B1, B2, B3, dl);
    return RT_TRUE;
  }
  /* or, one of the arc end points is the closest */
  else if  ( pt_in_seg && ! pt_in_arc )
  {
    rt_dist2d_pt_seg(ctx, B1, A1, A2, dl);
    rt_dist2d_pt_seg(ctx, B3, A1, A2, dl);
    return RT_TRUE;
  }
  /* Finally, one of the end-point to end-point combos is the closest. */
  else
  {
    rt_dist2d_pt_pt(ctx, A1, B1, dl);
    rt_dist2d_pt_pt(ctx, A1, B3, dl);
    rt_dist2d_pt_pt(ctx, A2, B1, dl);
    rt_dist2d_pt_pt(ctx, A2, B3, dl);
    return RT_TRUE;
  }

  return RT_FALSE;
}

int
rt_dist2d_pt_arc(const RTCTX *ctx, const RTPOINT2D* P, const RTPOINT2D* A1, const RTPOINT2D* A2, const RTPOINT2D* A3, DISTPTS* dl)
{
  double radius_A, d;
  RTPOINT2D C; /* center of circle defined by arc A */
  RTPOINT2D X; /* point circle(A) where line from C to P crosses */

  if ( dl->mode < 0 )
    rterror(ctx, "rt_dist2d_pt_arc does not support maxdistance mode");

  /* What if the arc is a point? */
  if ( rt_arc_is_pt(ctx, A1, A2, A3) )
    return rt_dist2d_pt_pt(ctx, P, A1, dl);

  /* Calculate centers and radii of circles. */
  radius_A = rt_arc_center(ctx, A1, A2, A3, &C);

  /* This "arc" is actually a line (A2 is colinear with A1,A3) */
  if ( radius_A < 0.0 )
    return rt_dist2d_pt_seg(ctx, P, A1, A3, dl);

  /* Distance from point to center */
  d = distance2d_pt_pt(ctx, &C, P);

  /* X is the point on the circle where the line from P to C crosses */
  X.x = C.x + (P->x - C.x) * radius_A / d;
  X.y = C.y + (P->y - C.y) * radius_A / d;

  /* Is crossing point inside the arc? Or arc is actually circle? */
  if ( p2d_same(ctx, A1, A3) || rt_pt_in_arc(ctx, &X, A1, A2, A3) )
  {
    rt_dist2d_pt_pt(ctx, P, &X, dl);
  }
  else
  {
    /* Distance is the minimum of the distances to the arc end points */
    rt_dist2d_pt_pt(ctx, A1, P, dl);
    rt_dist2d_pt_pt(ctx, A3, P, dl);
  }
  return RT_TRUE;
}


int
rt_dist2d_arc_arc(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3,
                  const RTPOINT2D *B1, const RTPOINT2D *B2, const RTPOINT2D *B3,
                  DISTPTS *dl)
{
  RTPOINT2D CA, CB; /* Center points of arcs A and B */
  double radius_A, radius_B, d; /* Radii of arcs A and B */
  RTPOINT2D P; /* Temporary point P */
  RTPOINT2D D; /* Mid-point between the centers CA and CB */
  int pt_in_arc_A, pt_in_arc_B; /* Test whether potential intersection point is within the arc */

  if ( dl->mode != DIST_MIN )
    rterror(ctx, "rt_dist2d_arc_arc only supports mindistance");

  /* TODO: Handle case where arc is closed circle (A1 = A3) */

  /* What if one or both of our "arcs" is actually a point? */
  if ( rt_arc_is_pt(ctx, B1, B2, B3) && rt_arc_is_pt(ctx, A1, A2, A3) )
    return rt_dist2d_pt_pt(ctx, B1, A1, dl);
  else if ( rt_arc_is_pt(ctx, B1, B2, B3) )
    return rt_dist2d_pt_arc(ctx, B1, A1, A2, A3, dl);
  else if ( rt_arc_is_pt(ctx, A1, A2, A3) )
    return rt_dist2d_pt_arc(ctx, A1, B1, B2, B3, dl);

  /* Calculate centers and radii of circles. */
  radius_A = rt_arc_center(ctx, A1, A2, A3, &CA);
  radius_B = rt_arc_center(ctx, B1, B2, B3, &CB);

  /* Two co-linear arcs?!? That's two segments. */
  if ( radius_A < 0 && radius_B < 0 )
    return rt_dist2d_seg_seg(ctx, A1, A3, B1, B3, dl);

  /* A is co-linear, delegate to rt_dist_seg_arc here. */
  if ( radius_A < 0 )
    return rt_dist2d_seg_arc(ctx, A1, A3, B1, B2, B3, dl);

  /* B is co-linear, delegate to rt_dist_seg_arc here. */
  if ( radius_B < 0 )
    return rt_dist2d_seg_arc(ctx, B1, B3, A1, A2, A3, dl);

  /* Make sure that arc "A" has the bigger radius */
  if ( radius_B > radius_A )
  {
    const RTPOINT2D *tmp;
    tmp = B1; B1 = A1; A1 = tmp;
    tmp = B2; B2 = A2; A2 = tmp;
    tmp = B3; B3 = A3; A3 = tmp;
    P = CB; CB = CA; CA = P;
    d = radius_B; radius_B = radius_A; radius_A = d;
  }

  /* Center-center distance */
  d = distance2d_pt_pt(ctx, &CA, &CB);

  /* Equal circles. Arcs may intersect at multiple points, or at none! */
  if ( FP_EQUALS(d, 0.0) && FP_EQUALS(radius_A, radius_B) )
  {
    rterror(ctx, "rt_dist2d_arc_arc can't handle cojoint circles, uh oh");
  }

  /* Circles touch at a point. Is that point within the arcs? */
  if ( d == (radius_A + radius_B) )
  {
    D.x = CA.x + (CB.x - CA.x) * radius_A / d;
    D.y = CA.y + (CB.y - CA.y) * radius_A / d;

    pt_in_arc_A = rt_pt_in_arc(ctx, &D, A1, A2, A3);
    pt_in_arc_B = rt_pt_in_arc(ctx, &D, B1, B2, B3);

    /* Arcs do touch at D, return it */
    if ( pt_in_arc_A && pt_in_arc_B )
    {
      dl->distance = 0.0;
      dl->p1 = D;
      dl->p2 = D;
      return RT_TRUE;
    }
  }
  /* Disjoint or contained circles don't intersect. Closest point may be on */
  /* the line joining CA to CB. */
  else if ( d > (radius_A + radius_B) /* Disjoint */ || d < (radius_A - radius_B) /* Contained */ )
  {
    RTPOINT2D XA, XB; /* Points where the line from CA to CB cross their circle bounds */

    /* Calculate hypothetical nearest points, the places on the */
    /* two circles where the center-center line crosses. If both */
    /* arcs contain their hypothetical points, that's the crossing distance */
    XA.x = CA.x + (CB.x - CA.x) * radius_A / d;
    XA.y = CA.y + (CB.y - CA.y) * radius_A / d;
    XB.x = CB.x + (CA.x - CB.x) * radius_B / d;
    XB.y = CB.y + (CA.y - CB.y) * radius_B / d;

    pt_in_arc_A = rt_pt_in_arc(ctx, &XA, A1, A2, A3);
    pt_in_arc_B = rt_pt_in_arc(ctx, &XB, B1, B2, B3);

    /* If the nearest points are both within the arcs, that's our answer */
    /* the shortest distance is at the nearest points */
    if ( pt_in_arc_A && pt_in_arc_B )
    {
      return rt_dist2d_pt_pt(ctx, &XA, &XB, dl);
    }
  }
  /* Circles cross at two points, are either of those points in both arcs? */
  /* http://paulbourke.net/geometry/2circle/ */
  else if ( d < (radius_A + radius_B) )
  {
    RTPOINT2D E, F; /* Points where circle(A) and circle(B) cross */
    /* Distance from CA to D */
    double a = (radius_A*radius_A - radius_B*radius_B + d*d) / (2*d);
    /* Distance from D to E or F */
    double h = sqrt(radius_A*radius_A - a*a);

    /* Location of D */
    D.x = CA.x + (CB.x - CA.x) * a / d;
    D.y = CA.y + (CB.y - CA.y) * a / d;

    /* Start from D and project h units perpendicular to CA-D to get E */
    E.x = D.x + (D.y - CA.y) * h / a;
    E.y = D.y + (D.x - CA.x) * h / a;

    /* Crossing point E contained in arcs? */
    pt_in_arc_A = rt_pt_in_arc(ctx, &E, A1, A2, A3);
    pt_in_arc_B = rt_pt_in_arc(ctx, &E, B1, B2, B3);

    if ( pt_in_arc_A && pt_in_arc_B )
    {
      dl->p1 = dl->p2 = E;
      dl->distance = 0.0;
      return RT_TRUE;
    }

    /* Start from D and project h units perpendicular to CA-D to get F */
    F.x = D.x - (D.y - CA.y) * h / a;
    F.y = D.y - (D.x - CA.x) * h / a;

    /* Crossing point F contained in arcs? */
    pt_in_arc_A = rt_pt_in_arc(ctx, &F, A1, A2, A3);
    pt_in_arc_B = rt_pt_in_arc(ctx, &F, B1, B2, B3);

    if ( pt_in_arc_A && pt_in_arc_B )
    {
      dl->p1 = dl->p2 = F;
      dl->distance = 0.0;
      return RT_TRUE;
    }
  }
  else
  {
    rterror(ctx, "rt_dist2d_arc_arc: arcs neither touch, intersect nor are disjoint! INCONCEIVABLE!");
    return RT_FALSE;
  }

  /* Closest point is in the arc A, but not in the arc B, so */
  /* one of the B end points must be the closest. */
  if ( pt_in_arc_A & ! pt_in_arc_B )
  {
    rt_dist2d_pt_arc(ctx, B1, A1, A2, A3, dl);
    rt_dist2d_pt_arc(ctx, B3, A1, A2, A3, dl);
    return RT_TRUE;
  }
  /* Closest point is in the arc B, but not in the arc A, so */
  /* one of the A end points must be the closest. */
  else if  ( pt_in_arc_B && ! pt_in_arc_A )
  {
    rt_dist2d_pt_arc(ctx, A1, B1, B2, B3, dl);
    rt_dist2d_pt_arc(ctx, A3, B1, B2, B3, dl);
    return RT_TRUE;
  }
  /* Finally, one of the end-point to end-point combos is the closest. */
  else
  {
    rt_dist2d_pt_pt(ctx, A1, B1, dl);
    rt_dist2d_pt_pt(ctx, A1, B3, dl);
    rt_dist2d_pt_pt(ctx, A2, B1, dl);
    rt_dist2d_pt_pt(ctx, A2, B3, dl);
    return RT_TRUE;
  }

  return RT_TRUE;
}

/**
Finds the shortest distance between two segments.
This function is changed so it is not doing any comparasion of distance
but just sending every possible combination further to rt_dist2d_pt_seg
*/
int
rt_dist2d_seg_seg(const RTCTX *ctx, const RTPOINT2D *A, const RTPOINT2D *B, const RTPOINT2D *C, const RTPOINT2D *D, DISTPTS *dl)
{
  double  s_top, s_bot,s;
  double  r_top, r_bot,r;

  RTDEBUGF(ctx, 2, "rt_dist2d_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
           A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

  /*A and B are the same point */
  if (  ( A->x == B->x) && (A->y == B->y) )
  {
    return rt_dist2d_pt_seg(ctx, A,C,D,dl);
  }
  /*U and V are the same point */

  if (  ( C->x == D->x) && (C->y == D->y) )
  {
    dl->twisted= ((dl->twisted) * (-1));
    return rt_dist2d_pt_seg(ctx, D,A,B,dl);
  }
  /* AB and CD are line segments */
  /* from comp.graphics.algo

  Solving the above for r and s yields
        (Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy)
             r = ----------------------------- (eqn 1)
        (Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)

       (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
    s = ----------------------------- (eqn 2)
      (Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)
  Let P be the position vector of the intersection point, then
    P=A+r(B-A) or
    Px=Ax+r(Bx-Ax)
    Py=Ay+r(By-Ay)
  By examining the values of r & s, you can also determine some other limiting conditions:
    If 0<=r<=1 & 0<=s<=1, intersection exists
    r<0 or r>1 or s<0 or s>1 line segments do not intersect
    If the denominator in eqn 1 is zero, AB & CD are parallel
    If the numerator in eqn 1 is also zero, AB & CD are collinear.

  */
  r_top = (A->y-C->y)*(D->x-C->x) - (A->x-C->x)*(D->y-C->y);
  r_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

  s_top = (A->y-C->y)*(B->x-A->x) - (A->x-C->x)*(B->y-A->y);
  s_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

  if  ( (r_bot==0) || (s_bot == 0) )
  {
    if ((rt_dist2d_pt_seg(ctx, A,C,D,dl)) && (rt_dist2d_pt_seg(ctx, B,C,D,dl)))
    {
      dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
      return ((rt_dist2d_pt_seg(ctx, C,A,B,dl)) && (rt_dist2d_pt_seg(ctx, D,A,B,dl))); /*if all is successful we return true*/
    }
    else
    {
      return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
    }
  }

  s = s_top/s_bot;
  r=  r_top/r_bot;

  if (((r<0) || (r>1) || (s<0) || (s>1)) || (dl->mode == DIST_MAX))
  {
    if ((rt_dist2d_pt_seg(ctx, A,C,D,dl)) && (rt_dist2d_pt_seg(ctx, B,C,D,dl)))
    {
      dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
      return ((rt_dist2d_pt_seg(ctx, C,A,B,dl)) && (rt_dist2d_pt_seg(ctx, D,A,B,dl))); /*if all is successful we return true*/
    }
    else
    {
      return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
    }
  }
  else
  {
    if (dl->mode == DIST_MIN)  /*If there is intersection we identify the intersection point and return it but only if we are looking for mindistance*/
    {
      RTPOINT2D theP;

      if (((A->x==C->x)&&(A->y==C->y))||((A->x==D->x)&&(A->y==D->y)))
      {
        theP.x = A->x;
        theP.y = A->y;
      }
      else if (((B->x==C->x)&&(B->y==C->y))||((B->x==D->x)&&(B->y==D->y)))
      {
        theP.x = B->x;
        theP.y = B->y;
      }
      else
      {
        theP.x = A->x+r*(B->x-A->x);
        theP.y = A->y+r*(B->y-A->y);
      }
      dl->distance=0.0;
      dl->p1=theP;
      dl->p2=theP;
    }
    return RT_TRUE;

  }
  rterror(ctx, "unspecified error in function rt_dist2d_seg_seg");
  return RT_FALSE; /*If we have come here something is wrong*/
}


/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/

/**

The new faster calculation comparing pointarray to another pointarray
the arrays can come from both polygons and linestrings.
The naming is not good but comes from that it compares a
chosen selection of the points not all of them
*/
int
rt_dist2d_fast_ptarray_ptarray(const RTCTX *ctx, RTPOINTARRAY *l1, RTPOINTARRAY *l2,DISTPTS *dl, RTGBOX *box1, RTGBOX *box2)
{
  /*here we define two lists to hold our calculated "z"-values and the order number in the geometry*/

  double k, thevalue;
  float  deltaX, deltaY, c1m, c2m;
  RTPOINT2D  c1, c2;
  const RTPOINT2D *theP;
  float min1X, max1X, max1Y, min1Y,min2X, max2X, max2Y, min2Y;
  int t;
  int n1 = l1->npoints;
  int n2 = l2->npoints;

  LISTSTRUCT *list1, *list2;
  list1 = (LISTSTRUCT*)rtalloc(ctx, sizeof(LISTSTRUCT)*n1);
  list2 = (LISTSTRUCT*)rtalloc(ctx, sizeof(LISTSTRUCT)*n2);

  RTDEBUG(ctx, 2, "rt_dist2d_fast_ptarray_ptarray is called");

  max1X = box1->xmax;
  min1X = box1->xmin;
  max1Y = box1->ymax;
  min1Y = box1->ymin;
  max2X = box2->xmax;
  min2X = box2->xmin;
  max2Y = box2->ymax;
  min2Y = box2->ymin;
  /*we want the center of the bboxes, and calculate the slope between the centerpoints*/
  c1.x = min1X + (max1X-min1X)/2;
  c1.y = min1Y + (max1Y-min1Y)/2;
  c2.x = min2X + (max2X-min2X)/2;
  c2.y = min2Y + (max2Y-min2Y)/2;

  deltaX=(c2.x-c1.x);
  deltaY=(c2.y-c1.y);


  /*Here we calculate where the line perpendicular to the center-center line crosses the axes for each vertex
  if the center-center line is vertical the perpendicular line will be horizontal and we find it's crossing the Y-axes with z = y-kx */
  if ((deltaX*deltaX)<(deltaY*deltaY))        /*North or South*/
  {
    k = -deltaX/deltaY;
    for (t=0; t<n1; t++) /*for each segment in L1 */
    {
      theP = rt_getPoint2d_cp(ctx, l1, t);
      thevalue = theP->y - (k * theP->x);
      list1[t].themeasure=thevalue;
      list1[t].pnr=t;

    }
    for (t=0; t<n2; t++) /*for each segment in L2*/
    {
      theP = rt_getPoint2d_cp(ctx, l2, t);
      thevalue = theP->y - (k * theP->x);
      list2[t].themeasure=thevalue;
      list2[t].pnr=t;

    }
    c1m = c1.y-(k*c1.x);
    c2m = c2.y-(k*c2.x);
  }


  /*if the center-center line is horizontal the perpendicular line will be vertical. To eliminate problems with deviding by zero we are here mirroring the coordinate-system
   and we find it's crossing the X-axes with z = x-(1/k)y */
  else        /*West or East*/
  {
    k = -deltaY/deltaX;
    for (t=0; t<n1; t++) /*for each segment in L1 */
    {
      theP = rt_getPoint2d_cp(ctx, l1, t);
      thevalue = theP->x - (k * theP->y);
      list1[t].themeasure=thevalue;
      list1[t].pnr=t;
      /* rtnotice(ctx, "l1 %d, measure=%f",t,thevalue ); */
    }
    for (t=0; t<n2; t++) /*for each segment in L2*/
    {
      theP = rt_getPoint2d_cp(ctx, l2, t);
      thevalue = theP->x - (k * theP->y);
      list2[t].themeasure=thevalue;
      list2[t].pnr=t;
      /* rtnotice(ctx, "l2 %d, measure=%f",t,thevalue ); */
    }
    c1m = c1.x-(k*c1.y);
    c2m = c2.x-(k*c2.y);
  }

  /*we sort our lists by the calculated values*/
  qsort(list1, n1, sizeof(LISTSTRUCT), struct_cmp_by_measure);
  qsort(list2, n2, sizeof(LISTSTRUCT), struct_cmp_by_measure);

  if (c1m < c2m)
  {
    if (!rt_dist2d_pre_seg_seg(ctx, l1,l2,list1,list2,k,dl))
    {
      rtfree(ctx, list1);
      rtfree(ctx, list2);
      return RT_FALSE;
    }
  }
  else
  {
    dl->twisted= ((dl->twisted) * (-1));
    if (!rt_dist2d_pre_seg_seg(ctx, l2,l1,list2,list1,k,dl))
    {
      rtfree(ctx, list1);
      rtfree(ctx, list2);
      return RT_FALSE;
    }
  }
  rtfree(ctx, list1);
  rtfree(ctx, list2);
  return RT_TRUE;
}

int
struct_cmp_by_measure(const void *a, const void *b)
{
  LISTSTRUCT *ia = (LISTSTRUCT*)a;
  LISTSTRUCT *ib = (LISTSTRUCT*)b;
  return ( ia->themeasure>ib->themeasure ) ? 1 : -1;
}

/**
  preparation before rt_dist2d_seg_seg.
*/
int
rt_dist2d_pre_seg_seg(const RTCTX *ctx, RTPOINTARRAY *l1, RTPOINTARRAY *l2,LISTSTRUCT *list1, LISTSTRUCT *list2,double k, DISTPTS *dl)
{
  const RTPOINT2D *p1, *p2, *p3, *p4, *p01, *p02;
  int pnr1,pnr2,pnr3,pnr4, n1, n2, i, u, r, twist;
  double maxmeasure;
  n1=  l1->npoints;
  n2 = l2->npoints;

  RTDEBUG(ctx, 2, "rt_dist2d_pre_seg_seg is called");

  p1 = rt_getPoint2d_cp(ctx, l1, list1[0].pnr);
  p3 = rt_getPoint2d_cp(ctx, l2, list2[0].pnr);
  rt_dist2d_pt_pt(ctx, p1, p3, dl);
  maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));
  twist = dl->twisted; /*to keep the incomming order between iterations*/
  for (i =(n1-1); i>=0; --i)
  {
    /*we break this iteration when we have checked every
    point closer to our perpendicular "checkline" than
    our shortest found distance*/
    if (((list2[0].themeasure-list1[i].themeasure)) > maxmeasure) break;
    for (r=-1; r<=1; r +=2) /*because we are not iterating in the original pointorder we have to check the segment before and after every point*/
    {
      pnr1 = list1[i].pnr;
      p1 = rt_getPoint2d_cp(ctx, l1, pnr1);
      if (pnr1+r<0)
      {
        p01 = rt_getPoint2d_cp(ctx, l1, (n1-1));
        if (( p1->x == p01->x) && (p1->y == p01->y)) pnr2 = (n1-1);
        else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
      }

      else if (pnr1+r>(n1-1))
      {
        p01 = rt_getPoint2d_cp(ctx, l1, 0);
        if (( p1->x == p01->x) && (p1->y == p01->y)) pnr2 = 0;
        else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
      }
      else pnr2 = pnr1+r;


      p2 = rt_getPoint2d_cp(ctx, l1, pnr2);
      for (u=0; u<n2; ++u)
      {
        if (((list2[u].themeasure-list1[i].themeasure)) >= maxmeasure) break;
        pnr3 = list2[u].pnr;
        p3 = rt_getPoint2d_cp(ctx, l2, pnr3);
        if (pnr3==0)
        {
          p02 = rt_getPoint2d_cp(ctx, l2, (n2-1));
          if (( p3->x == p02->x) && (p3->y == p02->y)) pnr4 = (n2-1);
          else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
        }
        else pnr4 = pnr3-1;

        p4 = rt_getPoint2d_cp(ctx, l2, pnr4);
        dl->twisted=twist;
        if (!rt_dist2d_selected_seg_seg(ctx, p1, p2, p3, p4, dl)) return RT_FALSE;

        if (pnr3>=(n2-1))
        {
          p02 = rt_getPoint2d_cp(ctx, l2, 0);
          if (( p3->x == p02->x) && (p3->y == p02->y)) pnr4 = 0;
          else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
        }

        else pnr4 = pnr3+1;

        p4 = rt_getPoint2d_cp(ctx, l2, pnr4);
        dl->twisted=twist; /*we reset the "twist" for each iteration*/
        if (!rt_dist2d_selected_seg_seg(ctx, p1, p2, p3, p4, dl)) return RT_FALSE;

        maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));/*here we "translate" the found mindistance so it can be compared to our "z"-values*/
      }
    }
  }

  return RT_TRUE;
}


/**
  This is the same function as rt_dist2d_seg_seg but
  without any calculations to determine intersection since we
  already know they do not intersect
*/
int
rt_dist2d_selected_seg_seg(const RTCTX *ctx, const RTPOINT2D *A, const RTPOINT2D *B, const RTPOINT2D *C, const RTPOINT2D *D, DISTPTS *dl)
{
  RTDEBUGF(ctx, 2, "rt_dist2d_selected_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
           A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

  /*A and B are the same point */
  if (  ( A->x == B->x) && (A->y == B->y) )
  {
    return rt_dist2d_pt_seg(ctx, A,C,D,dl);
  }
  /*U and V are the same point */

  if (  ( C->x == D->x) && (C->y == D->y) )
  {
    dl->twisted= ((dl->twisted) * (-1));
    return rt_dist2d_pt_seg(ctx, D,A,B,dl);
  }

  if ((rt_dist2d_pt_seg(ctx, A,C,D,dl)) && (rt_dist2d_pt_seg(ctx, B,C,D,dl)))
  {
    dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
    return ((rt_dist2d_pt_seg(ctx, C,A,B,dl)) && (rt_dist2d_pt_seg(ctx, D,A,B,dl))); /*if all is successful we return true*/
  }
  else
  {
    return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
  }
}

/*------------------------------------------------------------------------------------------------------------
End of New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/

/**
rt_dist2d_comp from p to line A->B
This one is now sending every occation to rt_dist2d_pt_pt
Before it was handling occations where r was between 0 and 1 internally
and just returning the distance without identifying the points.
To get this points it was nessecary to change and it also showed to be about 10%faster.
*/
int
rt_dist2d_pt_seg(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINT2D *A, const RTPOINT2D *B, DISTPTS *dl)
{
  RTPOINT2D c;
  double  r;
  /*if start==end, then use pt distance */
  if (  ( A->x == B->x) && (A->y == B->y) )
  {
    return rt_dist2d_pt_pt(ctx, p,A,dl);
  }
  /*
   * otherwise, we use comp.graphics.algorithms
   * Frequently Asked Questions method
   *
   *  (1)        AC dot AB
   *         r = ---------
   *              ||AB||^2
   *  r has the following meaning:
   *  r=0 P = A
   *  r=1 P = B
   *  r<0 P is on the backward extension of AB
   *  r>1 P is on the forward extension of AB
   *  0<r<1 P is interior to AB
   */

  r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  /*This is for finding the maxdistance.
  the maxdistance have to be between two vertexes,
  compared to mindistance which can be between
  tvo vertexes vertex.*/
  if (dl->mode == DIST_MAX)
  {
    if (r>=0.5)
    {
      return rt_dist2d_pt_pt(ctx, p,A,dl);
    }
    if (r<0.5)
    {
      return rt_dist2d_pt_pt(ctx, p,B,dl);
    }
  }

  if (r<0)  /*If p projected on the line is outside point A*/
  {
    return rt_dist2d_pt_pt(ctx, p,A,dl);
  }
  if (r>=1)  /*If p projected on the line is outside point B or on point B*/
  {
    return rt_dist2d_pt_pt(ctx, p,B,dl);
  }

  /*If the point p is on the segment this is a more robust way to find out that*/
  if (( ((A->y-p->y)*(B->x-A->x)==(A->x-p->x)*(B->y-A->y) ) ) && (dl->mode ==  DIST_MIN))
  {
    dl->distance = 0.0;
    dl->p1 = *p;
    dl->p2 = *p;
  }

  /*If the projection of point p on the segment is between A and B
  then we find that "point on segment" and send it to rt_dist2d_pt_pt*/
  c.x=A->x + r * (B->x-A->x);
  c.y=A->y + r * (B->y-A->y);

  return rt_dist2d_pt_pt(ctx, p,&c,dl);
}


/**

Compares incomming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
rt_dist2d_pt_pt(const RTCTX *ctx, const RTPOINT2D *thep1, const RTPOINT2D *thep2, DISTPTS *dl)
{
  double hside = thep2->x - thep1->x;
  double vside = thep2->y - thep1->y;
  double dist = sqrt ( hside*hside + vside*vside );

  if (((dl->distance - dist)*(dl->mode))>0) /*multiplication with mode to handle mindistance (mode=1)  and maxdistance (mode = (-1)*/
  {
    dl->distance = dist;

    if (dl->twisted>0)  /*To get the points in right order. twisted is updated between 1 and (-1) every time the order is changed earlier in the chain*/
    {
      dl->p1 = *thep1;
      dl->p2 = *thep2;
    }
    else
    {
      dl->p1 = *thep2;
      dl->p2 = *thep1;
    }
  }
  return RT_TRUE;
}




/*------------------------------------------------------------------------------------------------------------
End of Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/


/**
The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_pt(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2)
{
  double hside = p2->x - p1->x;
  double vside = p2->y - p1->y;

  return sqrt ( hside*hside + vside*vside );

}

double
distance2d_sqr_pt_pt(const RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2)
{
  double hside = p2->x - p1->x;
  double vside = p2->y - p1->y;

  return  hside*hside + vside*vside;

}


/**

The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_seg(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINT2D *A, const RTPOINT2D *B)
{
  double  r,s;

  /*if start==end, then use pt distance */
  if (  ( A->x == B->x) && (A->y == B->y) )
    return distance2d_pt_pt(ctx, p,A);

  /*
   * otherwise, we use comp.graphics.algorithms
   * Frequently Asked Questions method
   *
   *  (1)             AC dot AB
          *         r = ---------
          *               ||AB||^2
   *  r has the following meaning:
   *  r=0 P = A
   *  r=1 P = B
   *  r<0 P is on the backward extension of AB
   *  r>1 P is on the forward extension of AB
   *  0<r<1 P is interior to AB
   */

  r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  if (r<0) return distance2d_pt_pt(ctx, p,A);
  if (r>1) return distance2d_pt_pt(ctx, p,B);


  /*
   * (2)
   *       (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
   *  s = -----------------------------
   *                 L^2
   *
   *  Then the distance from C to P = |s|*L.
   *
   */

  s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
      ( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  return FP_ABS(s) * sqrt(
             (B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y)
         );
}

/* return distance squared, useful to avoid sqrt calculations */
double
distance2d_sqr_pt_seg(const RTCTX *ctx, const RTPOINT2D *p, const RTPOINT2D *A, const RTPOINT2D *B)
{
  double  r,s;

  if (  ( A->x == B->x) && (A->y == B->y) )
    return distance2d_sqr_pt_pt(ctx, p,A);

  r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  if (r<0) return distance2d_sqr_pt_pt(ctx, p,A);
  if (r>1) return distance2d_sqr_pt_pt(ctx, p,B);


  /*
   * (2)
   *       (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
   *  s = -----------------------------
   *                 L^2
   *
   *  Then the distance from C to P = |s|*L.
   *
   */

  s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
      ( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  return s * s * ( (B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y) );
}



/**
 * Compute the azimuth of segment AB in radians.
 * Return 0 on exception (same point), 1 otherwise.
 */
int
azimuth_pt_pt(const RTCTX *ctx, const RTPOINT2D *A, const RTPOINT2D *B, double *d)
{
  if ( A->x == B->x )
  {
    if ( A->y < B->y ) *d=0.0;
    else if ( A->y > B->y ) *d=M_PI;
    else return 0;
    return 1;
  }

  if ( A->y == B->y )
  {
    if ( A->x < B->x ) *d=M_PI/2;
    else if ( A->x > B->x ) *d=M_PI+(M_PI/2);
    else return 0;
    return 1;
  }

  if ( A->x < B->x )
  {
    if ( A->y < B->y )
    {
      *d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) );
    }
    else /* ( A->y > B->y )  - equality case handled above */
    {
      *d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
         + (M_PI/2);
    }
  }

  else /* ( A->x > B->x ) - equality case handled above */
  {
    if ( A->y > B->y )
    {
      *d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) )
         + M_PI;
    }
    else /* ( A->y < B->y )  - equality case handled above */
    {
      *d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
         + (M_PI+(M_PI/2));
    }
  }

  return 1;
}


