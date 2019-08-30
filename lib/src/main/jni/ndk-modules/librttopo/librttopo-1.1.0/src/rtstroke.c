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
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "librttopo_geom_internal.h"

/* #define RTGEOM_DEBUG_LEVEL 4 */

#include "rtgeom_log.h"


RTMLINE* rtmcurve_stroke(const RTCTX *ctx, const RTMCURVE *mcurve, uint32_t perQuad);
RTMPOLY* rtmsurface_stroke(const RTCTX *ctx, const RTMSURFACE *msurface, uint32_t perQuad);
RTCOLLECTION* rtcollection_stroke(const RTCTX *ctx, const RTCOLLECTION *collection, uint32_t perQuad);

RTGEOM* pta_unstroke(const RTCTX *ctx, const RTPOINTARRAY *points, int type, int srid);
RTGEOM* rtline_unstroke(const RTCTX *ctx, const RTLINE *line);
RTGEOM* rtpolygon_unstroke(const RTCTX *ctx, const RTPOLY *poly);
RTGEOM* rtmline_unstroke(const RTCTX *ctx, const RTMLINE *mline);
RTGEOM* rtmpolygon_unstroke(const RTCTX *ctx, const RTMPOLY *mpoly);
RTGEOM* rtgeom_unstroke(const RTCTX *ctx, const RTGEOM *geom);


/*
 * Determines (recursively in the case of collections) whether the geometry
 * contains at least on arc geometry or segment.
 */
int
rtgeom_has_arc(const RTCTX *ctx, const RTGEOM *geom)
{
  RTCOLLECTION *col;
  int i;

  RTDEBUG(ctx, 2, "rtgeom_has_arc called.");

  switch (geom->type)
  {
  case RTPOINTTYPE:
  case RTLINETYPE:
  case RTPOLYGONTYPE:
  case RTTRIANGLETYPE:
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
    return RT_FALSE;
  case RTCIRCSTRINGTYPE:
    return RT_TRUE;
  /* It's a collection that MAY contain an arc */
  default:
    col = (RTCOLLECTION *)geom;
    for (i=0; i<col->ngeoms; i++)
    {
      if (rtgeom_has_arc(ctx, col->geoms[i]) == RT_TRUE)
        return RT_TRUE;
    }
    return RT_FALSE;
  }
}



/*******************************************************************************
 * Begin curve segmentize functions
 ******************************************************************************/

static double interpolate_arc(const RTCTX *ctx, double angle, double a1, double a2, double a3, double zm1, double zm2, double zm3)
{
  RTDEBUGF(ctx, 4,"angle %.05g a1 %.05g a2 %.05g a3 %.05g zm1 %.05g zm2 %.05g zm3 %.05g",angle,a1,a2,a3,zm1,zm2,zm3);
  /* Counter-clockwise sweep */
  if ( a1 < a2 )
  {
    if ( angle <= a2 )
      return zm1 + (zm2-zm1) * (angle-a1) / (a2-a1);
    else
      return zm2 + (zm3-zm2) * (angle-a2) / (a3-a2);
  }
  /* Clockwise sweep */
  else
  {
    if ( angle >= a2 )
      return zm1 + (zm2-zm1) * (a1-angle) / (a1-a2);
    else
      return zm2 + (zm3-zm2) * (a2-angle) / (a2-a3);
  }
}

static RTPOINTARRAY *
rtcircle_stroke(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, const RTPOINT4D *p3, uint32_t perQuad)
{
  RTPOINT2D center;
  RTPOINT2D *t1 = (RTPOINT2D*)p1;
  RTPOINT2D *t2 = (RTPOINT2D*)p2;
  RTPOINT2D *t3 = (RTPOINT2D*)p3;
  RTPOINT4D pt;
  int p2_side = 0;
  int clockwise = RT_TRUE;
  double radius; /* Arc radius */
  double increment; /* Angle per segment */
  double a1, a2, a3, angle;
  RTPOINTARRAY *pa;
  int is_circle = RT_FALSE;

  RTDEBUG(ctx, 2, "rtcircle_calculate_gbox called.");

  radius = rt_arc_center(ctx, t1, t2, t3, &center);
  p2_side = rt_segment_side(ctx, t1, t3, t2);

  /* Matched start/end points imply circle */
  if ( p1->x == p3->x && p1->y == p3->y )
    is_circle = RT_TRUE;

  /* Negative radius signals straight line, p1/p2/p3 are colinear */
  if ( (radius < 0.0 || p2_side == 0) && ! is_circle )
      return NULL;

  /* The side of the p1/p3 line that p2 falls on dictates the sweep
     direction from p1 to p3. */
  if ( p2_side == -1 )
    clockwise = RT_TRUE;
  else
    clockwise = RT_FALSE;

  increment = fabs(M_PI_2 / perQuad);

  /* Angles of each point that defines the arc section */
  a1 = atan2(p1->y - center.y, p1->x - center.x);
  a2 = atan2(p2->y - center.y, p2->x - center.x);
  a3 = atan2(p3->y - center.y, p3->x - center.x);

  /* p2 on left side => clockwise sweep */
  if ( clockwise )
  {
    increment *= -1;
    /* Adjust a3 down so we can decrement from a1 to a3 cleanly */
    if ( a3 > a1 )
      a3 -= 2.0 * M_PI;
    if ( a2 > a1 )
      a2 -= 2.0 * M_PI;
  }
  /* p2 on right side => counter-clockwise sweep */
  else
  {
    /* Adjust a3 up so we can increment from a1 to a3 cleanly */
    if ( a3 < a1 )
      a3 += 2.0 * M_PI;
    if ( a2 < a1 )
      a2 += 2.0 * M_PI;
  }

  /* Override angles for circle case */
  if( is_circle )
  {
    a3 = a1 + 2.0 * M_PI;
    a2 = a1 + M_PI;
    increment = fabs(increment);
    clockwise = RT_FALSE;
  }

  /* Initialize point array */
  pa = ptarray_construct_empty(ctx, 1, 1, 32);

  /* Sweep from a1 to a3 */
  ptarray_append_point(ctx, pa, p1, RT_FALSE);
  for ( angle = a1 + increment; clockwise ? angle > a3 : angle < a3; angle += increment )
  {
    pt.x = center.x + radius * cos(angle);
    pt.y = center.y + radius * sin(angle);
    pt.z = interpolate_arc(ctx, angle, a1, a2, a3, p1->z, p2->z, p3->z);
    pt.m = interpolate_arc(ctx, angle, a1, a2, a3, p1->m, p2->m, p3->m);
    ptarray_append_point(ctx, pa, &pt, RT_FALSE);
  }
  return pa;
}

RTLINE *
rtcircstring_stroke(const RTCTX *ctx, const RTCIRCSTRING *icurve, uint32_t perQuad)
{
  RTLINE *oline;
  RTPOINTARRAY *ptarray;
  RTPOINTARRAY *tmp;
  uint32_t i, j;
  RTPOINT4D p1, p2, p3, p4;

  RTDEBUGF(ctx, 2, "rtcircstring_stroke called., dim = %d", icurve->points->flags);

  ptarray = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(icurve->points->flags), RTFLAGS_GET_M(icurve->points->flags), 64);

  for (i = 2; i < icurve->points->npoints; i+=2)
  {
    RTDEBUGF(ctx, 3, "rtcircstring_stroke: arc ending at point %d", i);

    rt_getPoint4d_p(ctx, icurve->points, i - 2, &p1);
    rt_getPoint4d_p(ctx, icurve->points, i - 1, &p2);
    rt_getPoint4d_p(ctx, icurve->points, i, &p3);
    tmp = rtcircle_stroke(ctx, &p1, &p2, &p3, perQuad);

    if (tmp)
    {
      RTDEBUGF(ctx, 3, "rtcircstring_stroke: generated %d points", tmp->npoints);

      for (j = 0; j < tmp->npoints; j++)
      {
        rt_getPoint4d_p(ctx, tmp, j, &p4);
        ptarray_append_point(ctx, ptarray, &p4, RT_TRUE);
      }
      ptarray_free(ctx, tmp);
    }
    else
    {
      RTDEBUG(ctx, 3, "rtcircstring_stroke: points are colinear, returning curve points as line");

      for (j = i - 2 ; j < i ; j++)
      {
        rt_getPoint4d_p(ctx, icurve->points, j, &p4);
        ptarray_append_point(ctx, ptarray, &p4, RT_TRUE);
      }
    }

  }
  rt_getPoint4d_p(ctx, icurve->points, icurve->points->npoints-1, &p1);
  ptarray_append_point(ctx, ptarray, &p1, RT_TRUE);

  oline = rtline_construct(ctx, icurve->srid, NULL, ptarray);
  return oline;
}

RTLINE *
rtcompound_stroke(const RTCTX *ctx, const RTCOMPOUND *icompound, uint32_t perQuad)
{
  RTGEOM *geom;
  RTPOINTARRAY *ptarray = NULL, *ptarray_out = NULL;
  RTLINE *tmp = NULL;
  uint32_t i, j;
  RTPOINT4D p;

  RTDEBUG(ctx, 2, "rtcompound_stroke called.");

  ptarray = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(icompound->flags), RTFLAGS_GET_M(icompound->flags), 64);

  for (i = 0; i < icompound->ngeoms; i++)
  {
    geom = icompound->geoms[i];
    if (geom->type == RTCIRCSTRINGTYPE)
    {
      tmp = rtcircstring_stroke(ctx, (RTCIRCSTRING *)geom, perQuad);
      for (j = 0; j < tmp->points->npoints; j++)
      {
        rt_getPoint4d_p(ctx, tmp->points, j, &p);
        ptarray_append_point(ctx, ptarray, &p, RT_TRUE);
      }
      rtline_free(ctx, tmp);
    }
    else if (geom->type == RTLINETYPE)
    {
      tmp = (RTLINE *)geom;
      for (j = 0; j < tmp->points->npoints; j++)
      {
        rt_getPoint4d_p(ctx, tmp->points, j, &p);
        ptarray_append_point(ctx, ptarray, &p, RT_TRUE);
      }
    }
    else
    {
      rterror(ctx, "Unsupported geometry type %d found.",
              geom->type, rttype_name(ctx, geom->type));
      return NULL;
    }
  }
  ptarray_out = ptarray_remove_repeated_points(ctx, ptarray, 0.0);
  ptarray_free(ctx, ptarray);
  return rtline_construct(ctx, icompound->srid, NULL, ptarray_out);
}

RTPOLY *
rtcurvepoly_stroke(const RTCTX *ctx, const RTCURVEPOLY *curvepoly, uint32_t perQuad)
{
  RTPOLY *ogeom;
  RTGEOM *tmp;
  RTLINE *line;
  RTPOINTARRAY **ptarray;
  int i;

  RTDEBUG(ctx, 2, "rtcurvepoly_stroke called.");

  ptarray = rtalloc(ctx, sizeof(RTPOINTARRAY *)*curvepoly->nrings);

  for (i = 0; i < curvepoly->nrings; i++)
  {
    tmp = curvepoly->rings[i];
    if (tmp->type == RTCIRCSTRINGTYPE)
    {
      line = rtcircstring_stroke(ctx, (RTCIRCSTRING *)tmp, perQuad);
      ptarray[i] = ptarray_clone_deep(ctx, line->points);
      rtline_free(ctx, line);
    }
    else if (tmp->type == RTLINETYPE)
    {
      line = (RTLINE *)tmp;
      ptarray[i] = ptarray_clone_deep(ctx, line->points);
    }
    else if (tmp->type == RTCOMPOUNDTYPE)
    {
      line = rtcompound_stroke(ctx, (RTCOMPOUND *)tmp, perQuad);
      ptarray[i] = ptarray_clone_deep(ctx, line->points);
      rtline_free(ctx, line);
    }
    else
    {
      rterror(ctx, "Invalid ring type found in CurvePoly.");
      return NULL;
    }
  }

  ogeom = rtpoly_construct(ctx, curvepoly->srid, NULL, curvepoly->nrings, ptarray);
  return ogeom;
}

RTMLINE *
rtmcurve_stroke(const RTCTX *ctx, const RTMCURVE *mcurve, uint32_t perQuad)
{
  RTMLINE *ogeom;
  RTGEOM **lines;
  int i;

  RTDEBUGF(ctx, 2, "rtmcurve_stroke called, geoms=%d, dim=%d.", mcurve->ngeoms, RTFLAGS_NDIMS(mcurve->flags));

  lines = rtalloc(ctx, sizeof(RTGEOM *)*mcurve->ngeoms);

  for (i = 0; i < mcurve->ngeoms; i++)
  {
    const RTGEOM *tmp = mcurve->geoms[i];
    if (tmp->type == RTCIRCSTRINGTYPE)
    {
      lines[i] = (RTGEOM *)rtcircstring_stroke(ctx, (RTCIRCSTRING *)tmp, perQuad);
    }
    else if (tmp->type == RTLINETYPE)
    {
      lines[i] = (RTGEOM *)rtline_construct(ctx, mcurve->srid, NULL, ptarray_clone_deep(ctx, ((RTLINE *)tmp)->points));
    }
    else if (tmp->type == RTCOMPOUNDTYPE)
    {
      lines[i] = (RTGEOM *)rtcompound_stroke(ctx, (RTCOMPOUND *)tmp, perQuad);
    }
    else
    {
      rterror(ctx, "Unsupported geometry found in MultiCurve.");
      return NULL;
    }
  }

  ogeom = (RTMLINE *)rtcollection_construct(ctx, RTMULTILINETYPE, mcurve->srid, NULL, mcurve->ngeoms, lines);
  return ogeom;
}

RTMPOLY *
rtmsurface_stroke(const RTCTX *ctx, const RTMSURFACE *msurface, uint32_t perQuad)
{
  RTMPOLY *ogeom;
  RTGEOM *tmp;
  RTPOLY *poly;
  RTGEOM **polys;
  RTPOINTARRAY **ptarray;
  int i, j;

  RTDEBUG(ctx, 2, "rtmsurface_stroke called.");

  polys = rtalloc(ctx, sizeof(RTGEOM *)*msurface->ngeoms);

  for (i = 0; i < msurface->ngeoms; i++)
  {
    tmp = msurface->geoms[i];
    if (tmp->type == RTCURVEPOLYTYPE)
    {
      polys[i] = (RTGEOM *)rtcurvepoly_stroke(ctx, (RTCURVEPOLY *)tmp, perQuad);
    }
    else if (tmp->type == RTPOLYGONTYPE)
    {
      poly = (RTPOLY *)tmp;
      ptarray = rtalloc(ctx, sizeof(RTPOINTARRAY *)*poly->nrings);
      for (j = 0; j < poly->nrings; j++)
      {
        ptarray[j] = ptarray_clone_deep(ctx, poly->rings[j]);
      }
      polys[i] = (RTGEOM *)rtpoly_construct(ctx, msurface->srid, NULL, poly->nrings, ptarray);
    }
  }
  ogeom = (RTMPOLY *)rtcollection_construct(ctx, RTMULTIPOLYGONTYPE, msurface->srid, NULL, msurface->ngeoms, polys);
  return ogeom;
}

RTCOLLECTION *
rtcollection_stroke(const RTCTX *ctx, const RTCOLLECTION *collection, uint32_t perQuad)
{
  RTCOLLECTION *ocol;
  RTGEOM *tmp;
  RTGEOM **geoms;
  int i;

  RTDEBUG(ctx, 2, "rtcollection_stroke called.");

  geoms = rtalloc(ctx, sizeof(RTGEOM *)*collection->ngeoms);

  for (i=0; i<collection->ngeoms; i++)
  {
    tmp = collection->geoms[i];
    switch (tmp->type)
    {
    case RTCIRCSTRINGTYPE:
      geoms[i] = (RTGEOM *)rtcircstring_stroke(ctx, (RTCIRCSTRING *)tmp, perQuad);
      break;
    case RTCOMPOUNDTYPE:
      geoms[i] = (RTGEOM *)rtcompound_stroke(ctx, (RTCOMPOUND *)tmp, perQuad);
      break;
    case RTCURVEPOLYTYPE:
      geoms[i] = (RTGEOM *)rtcurvepoly_stroke(ctx, (RTCURVEPOLY *)tmp, perQuad);
      break;
    case RTCOLLECTIONTYPE:
      geoms[i] = (RTGEOM *)rtcollection_stroke(ctx, (RTCOLLECTION *)tmp, perQuad);
      break;
    default:
      geoms[i] = rtgeom_clone(ctx, tmp);
      break;
    }
  }
  ocol = rtcollection_construct(ctx, RTCOLLECTIONTYPE, collection->srid, NULL, collection->ngeoms, geoms);
  return ocol;
}

RTGEOM *
rtgeom_stroke(const RTCTX *ctx, const RTGEOM *geom, uint32_t perQuad)
{
  RTGEOM * ogeom = NULL;
  switch (geom->type)
  {
  case RTCIRCSTRINGTYPE:
    ogeom = (RTGEOM *)rtcircstring_stroke(ctx, (RTCIRCSTRING *)geom, perQuad);
    break;
  case RTCOMPOUNDTYPE:
    ogeom = (RTGEOM *)rtcompound_stroke(ctx, (RTCOMPOUND *)geom, perQuad);
    break;
  case RTCURVEPOLYTYPE:
    ogeom = (RTGEOM *)rtcurvepoly_stroke(ctx, (RTCURVEPOLY *)geom, perQuad);
    break;
  case RTMULTICURVETYPE:
    ogeom = (RTGEOM *)rtmcurve_stroke(ctx, (RTMCURVE *)geom, perQuad);
    break;
  case RTMULTISURFACETYPE:
    ogeom = (RTGEOM *)rtmsurface_stroke(ctx, (RTMSURFACE *)geom, perQuad);
    break;
  case RTCOLLECTIONTYPE:
    ogeom = (RTGEOM *)rtcollection_stroke(ctx, (RTCOLLECTION *)geom, perQuad);
    break;
  default:
    ogeom = rtgeom_clone(ctx, geom);
  }
  return ogeom;
}

/**
 * Return ABC angle in radians
 * TODO: move to rtalgorithm
 */
static double
rt_arc_angle(const RTCTX *ctx, const RTPOINT2D *a, const RTPOINT2D *b, const RTPOINT2D *c)
{
  RTPOINT2D ab, cb;

  ab.x = b->x - a->x;
  ab.y = b->y - a->y;

  cb.x = b->x - c->x;
  cb.y = b->y - c->y;

  double dot = (ab.x * cb.x + ab.y * cb.y); /* dot product */
  double cross = (ab.x * cb.y - ab.y * cb.x); /* cross product */

  double alpha = atan2(cross, dot);

  return alpha;
}

/**
* Returns RT_TRUE if b is on the arc formed by a1/a2/a3, but not within
* that portion already described by a1/a2/a3
*/
static int pt_continues_arc(const RTCTX *ctx, const RTPOINT4D *a1, const RTPOINT4D *a2, const RTPOINT4D *a3, const RTPOINT4D *b)
{
  RTPOINT2D center;
  RTPOINT2D *t1 = (RTPOINT2D*)a1;
  RTPOINT2D *t2 = (RTPOINT2D*)a2;
  RTPOINT2D *t3 = (RTPOINT2D*)a3;
  RTPOINT2D *tb = (RTPOINT2D*)b;
  double radius = rt_arc_center(ctx, t1, t2, t3, &center);
  double b_distance, diff;

  /* Co-linear a1/a2/a3 */
  if ( radius < 0.0 )
    return RT_FALSE;

  b_distance = distance2d_pt_pt(ctx, tb, &center);
  diff = fabs(radius - b_distance);
  RTDEBUGF(ctx, 4, "circle_radius=%g, b_distance=%g, diff=%g, percentage=%g", radius, b_distance, diff, diff/radius);

  /* Is the point b on the circle? */
  if ( diff < EPSILON_SQLMM )
  {
    int a2_side = rt_segment_side(ctx, t1, t3, t2);
    int b_side  = rt_segment_side(ctx, t1, t3, tb);
    double angle1 = rt_arc_angle(ctx, t1, t2, t3);
    double angle2 = rt_arc_angle(ctx, t2, t3, tb);

    /* Is the angle similar to the previous one ? */
    diff = fabs(angle1 - angle2);
    RTDEBUGF(ctx, 4, " angle1: %g, angle2: %g, diff:%g", angle1, angle2, diff);
    if ( diff > EPSILON_SQLMM )
    {
      return RT_FALSE;
    }

    /* Is the point b on the same side of a1/a3 as the mid-point a2 is? */
    /* If not, it's in the unbounded part of the circle, so it continues the arc, return true. */
    if ( b_side != a2_side )
      return RT_TRUE;
  }
  return RT_FALSE;
}

static RTGEOM*
linestring_from_pa(const RTCTX *ctx, const RTPOINTARRAY *pa, int srid, int start, int end)
{
  int i = 0, j = 0;
  RTPOINT4D p;
  RTPOINTARRAY *pao = ptarray_construct(ctx, ptarray_has_z(ctx, pa), ptarray_has_m(ctx, pa), end-start+2);
  RTDEBUGF(ctx, 4, "srid=%d, start=%d, end=%d", srid, start, end);
  for( i = start; i < end + 2; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &p);
    ptarray_set_point4d(ctx, pao, j++, &p);
  }
  return rtline_as_rtgeom(ctx, rtline_construct(ctx, srid, NULL, pao));
}

static RTGEOM*
circstring_from_pa(const RTCTX *ctx, const RTPOINTARRAY *pa, int srid, int start, int end)
{

  RTPOINT4D p0, p1, p2;
  RTPOINTARRAY *pao = ptarray_construct(ctx, ptarray_has_z(ctx, pa), ptarray_has_m(ctx, pa), 3);
  RTDEBUGF(ctx, 4, "srid=%d, start=%d, end=%d", srid, start, end);
  rt_getPoint4d_p(ctx, pa, start, &p0);
  ptarray_set_point4d(ctx, pao, 0, &p0);
  rt_getPoint4d_p(ctx, pa, (start+end+1)/2, &p1);
  ptarray_set_point4d(ctx, pao, 1, &p1);
  rt_getPoint4d_p(ctx, pa, end+1, &p2);
  ptarray_set_point4d(ctx, pao, 2, &p2);
  return rtcircstring_as_rtgeom(ctx, rtcircstring_construct(ctx, srid, NULL, pao));
}

static RTGEOM*
geom_from_pa(const RTCTX *ctx, const RTPOINTARRAY *pa, int srid, int is_arc, int start, int end)
{
  RTDEBUGF(ctx, 4, "srid=%d, is_arc=%d, start=%d, end=%d", srid, is_arc, start, end);
  if ( is_arc )
    return circstring_from_pa(ctx, pa, srid, start, end);
  else
    return linestring_from_pa(ctx, pa, srid, start, end);
}

RTGEOM*
pta_unstroke(const RTCTX *ctx, const RTPOINTARRAY *points, int type, int srid)
{
  int i = 0, j, k;
  RTPOINT4D a1, a2, a3, b;
  RTPOINT4D first, center;
  char *edges_in_arcs;
  int found_arc = RT_FALSE;
  int current_arc = 1;
  int num_edges;
  int edge_type; /* non-zero if edge is part of an arc */
  int start, end;
  RTCOLLECTION *outcol;
  /* Minimum number of edges, per quadrant, required to define an arc */
  const unsigned int min_quad_edges = 2;

  /* Die on null input */
  if ( ! points )
    rterror(ctx, "pta_unstroke called with null pointarray");

  /* Null on empty input? */
  if ( points->npoints == 0 )
    return NULL;

  /* We can't desegmentize anything shorter than four points */
  if ( points->npoints < 4 )
  {
    /* Return a linestring here*/
    rterror(ctx, "pta_unstroke needs implementation for npoints < 4");
  }

  /* Allocate our result array of vertices that are part of arcs */
  num_edges = points->npoints - 1;
  edges_in_arcs = rtalloc(ctx, num_edges + 1);
  memset(edges_in_arcs, 0, num_edges + 1);

  /* We make a candidate arc of the first two edges, */
  /* And then see if the next edge follows it */
  while( i < num_edges-2 )
  {
    unsigned int arc_edges;
    double num_quadrants;
    double angle;

    found_arc = RT_FALSE;
    /* Make candidate arc */
    rt_getPoint4d_p(ctx, points, i  , &a1);
    rt_getPoint4d_p(ctx, points, i+1, &a2);
    rt_getPoint4d_p(ctx, points, i+2, &a3);
    memcpy(&first, &a1, sizeof(RTPOINT4D));

    for( j = i+3; j < num_edges+1; j++ )
    {
      RTDEBUGF(ctx, 4, "i=%d, j=%d", i, j);
      rt_getPoint4d_p(ctx, points, j, &b);
      /* Does this point fall on our candidate arc? */
      if ( pt_continues_arc(ctx, &a1, &a2, &a3, &b) )
      {
        /* Yes. Mark this edge and the two preceding it as arc components */
        RTDEBUGF(ctx, 4, "pt_continues_arc #%d", current_arc);
        found_arc = RT_TRUE;
        for ( k = j-1; k > j-4; k-- )
          edges_in_arcs[k] = current_arc;
      }
      else
      {
        /* No. So we're done with this candidate arc */
        RTDEBUG(ctx, 4, "pt_continues_arc = false");
        current_arc++;
        break;
      }

      memcpy(&a1, &a2, sizeof(RTPOINT4D));
      memcpy(&a2, &a3, sizeof(RTPOINT4D));
      memcpy(&a3,  &b, sizeof(RTPOINT4D));
    }
    /* Jump past all the edges that were added to the arc */
    if ( found_arc )
    {
      /* Check if an arc was composed by enough edges to be
       * really considered an arc
       * See http://trac.osgeo.org/postgis/ticket/2420
       */
      arc_edges = j - 1 - i;
      RTDEBUGF(ctx, 4, "arc defined by %d edges found", arc_edges);
      if ( first.x == b.x && first.y == b.y ) {
        RTDEBUG(ctx, 4, "arc is a circle");
        num_quadrants = 4;
      }
      else {
        rt_arc_center(ctx, (RTPOINT2D*)&first, (RTPOINT2D*)&b, (RTPOINT2D*)&a1, (RTPOINT2D*)&center);
        angle = rt_arc_angle(ctx, (RTPOINT2D*)&first, (RTPOINT2D*)&center, (RTPOINT2D*)&b);
        int p2_side = rt_segment_side(ctx, (RTPOINT2D*)&first, (RTPOINT2D*)&a1, (RTPOINT2D*)&b);
        if ( p2_side >= 0 ) angle = -angle;

        if ( angle < 0 ) angle = 2 * M_PI + angle;
        num_quadrants = ( 4 * angle ) / ( 2 * M_PI );
        RTDEBUGF(ctx, 4, "arc angle (%g %g, %g %g, %g %g) is %g (side is %d), quandrants:%g", first.x, first.y, center.x, center.y, b.x, b.y, angle, p2_side, num_quadrants);
      }
      /* a1 is first point, b is last point */
      if ( arc_edges < min_quad_edges * num_quadrants ) {
        RTDEBUGF(ctx, 4, "Not enough edges for a %g quadrants arc, %g needed", num_quadrants, min_quad_edges * num_quadrants);
        for ( k = j-1; k >= i; k-- )
          edges_in_arcs[k] = 0;
      }

      i = j-1;
    }
    else
    {
      /* Mark this edge as a linear edge */
      edges_in_arcs[i] = 0;
      i = i+1;
    }
  }

#if RTGEOM_DEBUG_LEVEL > 3
  {
    char *edgestr = rtalloc(ctx, num_edges+1);
    for ( i = 0; i < num_edges; i++ )
    {
      if ( edges_in_arcs[i] )
        edgestr[i] = 48 + edges_in_arcs[i];
      else
        edgestr[i] = '.';
    }
    edgestr[num_edges] = 0;
    RTDEBUGF(ctx, 3, "edge pattern %s", edgestr);
    rtfree(ctx, edgestr);
  }
#endif

  start = 0;
  edge_type = edges_in_arcs[0];
  outcol = rtcollection_construct_empty(ctx, RTCOMPOUNDTYPE, srid, ptarray_has_z(ctx, points), ptarray_has_m(ctx, points));
  for( i = 1; i < num_edges; i++ )
  {
    if( edge_type != edges_in_arcs[i] )
    {
      end = i - 1;
      rtcollection_add_rtgeom(ctx, outcol, geom_from_pa(ctx, points, srid, edge_type, start, end));
      start = i;
      edge_type = edges_in_arcs[i];
    }
  }
  rtfree(ctx, edges_in_arcs); /* not needed anymore */

  /* Roll out last item */
  end = num_edges - 1;
  rtcollection_add_rtgeom(ctx, outcol, geom_from_pa(ctx, points, srid, edge_type, start, end));

  /* Strip down to singleton if only one entry */
  if ( outcol->ngeoms == 1 )
  {
    RTGEOM *outgeom = outcol->geoms[0];
    outcol->ngeoms = 0; rtcollection_free(ctx, outcol);
    return outgeom;
  }
  return rtcollection_as_rtgeom(ctx, outcol);
}


RTGEOM *
rtline_unstroke(const RTCTX *ctx, const RTLINE *line)
{
  RTDEBUG(ctx, 2, "rtline_unstroke called.");

  if ( line->points->npoints < 4 ) return rtline_as_rtgeom(ctx, rtline_clone(ctx, line));
  else return pta_unstroke(ctx, line->points, line->flags, line->srid);
}

RTGEOM *
rtpolygon_unstroke(const RTCTX *ctx, const RTPOLY *poly)
{
  RTGEOM **geoms;
  int i, hascurve = 0;

  RTDEBUG(ctx, 2, "rtpolygon_unstroke called.");

  geoms = rtalloc(ctx, sizeof(RTGEOM *)*poly->nrings);
  for (i=0; i<poly->nrings; i++)
  {
    geoms[i] = pta_unstroke(ctx, poly->rings[i], poly->flags, poly->srid);
    if (geoms[i]->type == RTCIRCSTRINGTYPE || geoms[i]->type == RTCOMPOUNDTYPE)
    {
      hascurve = 1;
    }
  }
  if (hascurve == 0)
  {
    for (i=0; i<poly->nrings; i++)
    {
      rtfree(ctx, geoms[i]); /* TODO: should this be rtgeom_free instead ? */
    }
    return rtgeom_clone(ctx, (RTGEOM *)poly);
  }

  return (RTGEOM *)rtcollection_construct(ctx, RTCURVEPOLYTYPE, poly->srid, NULL, poly->nrings, geoms);
}

RTGEOM *
rtmline_unstroke(const RTCTX *ctx, const RTMLINE *mline)
{
  RTGEOM **geoms;
  int i, hascurve = 0;

  RTDEBUG(ctx, 2, "rtmline_unstroke called.");

  geoms = rtalloc(ctx, sizeof(RTGEOM *)*mline->ngeoms);
  for (i=0; i<mline->ngeoms; i++)
  {
    geoms[i] = rtline_unstroke(ctx, (RTLINE *)mline->geoms[i]);
    if (geoms[i]->type == RTCIRCSTRINGTYPE || geoms[i]->type == RTCOMPOUNDTYPE)
    {
      hascurve = 1;
    }
  }
  if (hascurve == 0)
  {
    for (i=0; i<mline->ngeoms; i++)
    {
      rtfree(ctx, geoms[i]); /* TODO: should this be rtgeom_free instead ? */
    }
    return rtgeom_clone(ctx, (RTGEOM *)mline);
  }
  return (RTGEOM *)rtcollection_construct(ctx, RTMULTICURVETYPE, mline->srid, NULL, mline->ngeoms, geoms);
}

RTGEOM *
rtmpolygon_unstroke(const RTCTX *ctx, const RTMPOLY *mpoly)
{
  RTGEOM **geoms;
  int i, hascurve = 0;

  RTDEBUG(ctx, 2, "rtmpoly_unstroke called.");

  geoms = rtalloc(ctx, sizeof(RTGEOM *)*mpoly->ngeoms);
  for (i=0; i<mpoly->ngeoms; i++)
  {
    geoms[i] = rtpolygon_unstroke(ctx, (RTPOLY *)mpoly->geoms[i]);
    if (geoms[i]->type == RTCURVEPOLYTYPE)
    {
      hascurve = 1;
    }
  }
  if (hascurve == 0)
  {
    for (i=0; i<mpoly->ngeoms; i++)
    {
      rtfree(ctx, geoms[i]); /* TODO: should this be rtgeom_free instead ? */
    }
    return rtgeom_clone(ctx, (RTGEOM *)mpoly);
  }
  return (RTGEOM *)rtcollection_construct(ctx, RTMULTISURFACETYPE, mpoly->srid, NULL, mpoly->ngeoms, geoms);
}

RTGEOM *
rtgeom_unstroke(const RTCTX *ctx, const RTGEOM *geom)
{
  RTDEBUG(ctx, 2, "rtgeom_unstroke called.");

  switch (geom->type)
  {
  case RTLINETYPE:
    return rtline_unstroke(ctx, (RTLINE *)geom);
  case RTPOLYGONTYPE:
    return rtpolygon_unstroke(ctx, (RTPOLY *)geom);
  case RTMULTILINETYPE:
    return rtmline_unstroke(ctx, (RTMLINE *)geom);
  case RTMULTIPOLYGONTYPE:
    return rtmpolygon_unstroke(ctx, (RTMPOLY *)geom);
  default:
    return rtgeom_clone(ctx, geom);
  }
}

