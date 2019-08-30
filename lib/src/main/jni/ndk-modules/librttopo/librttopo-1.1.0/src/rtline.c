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
 * Copyright (C) 2012 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



/* basic RTLINE functions */

#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"



/*
 * Construct a new RTLINE.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTLINE *
rtline_construct(const RTCTX *ctx, int srid, RTGBOX *bbox, RTPOINTARRAY *points)
{
  RTLINE *result;
  result = (RTLINE*) rtalloc(ctx, sizeof(RTLINE));

  RTDEBUG(ctx, 2, "rtline_construct called.");

  result->type = RTLINETYPE;

  result->flags = points->flags;
  RTFLAGS_SET_BBOX(result->flags, bbox?1:0);

  RTDEBUGF(ctx, 3, "rtline_construct type=%d", result->type);

  result->srid = srid;
  result->points = points;
  result->bbox = bbox;

  return result;
}

RTLINE *
rtline_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTLINE *result = rtalloc(ctx, sizeof(RTLINE));
  result->type = RTLINETYPE;
  result->flags = gflags(ctx, hasz,hasm,0);
  result->srid = srid;
  result->points = ptarray_construct_empty(ctx, hasz, hasm, 1);
  result->bbox = NULL;
  return result;
}


void rtline_free(const RTCTX *ctx, RTLINE  *line)
{
  if ( ! line ) return;

  if ( line->bbox )
    rtfree(ctx, line->bbox);
  if ( line->points )
    ptarray_free(ctx, line->points);
  rtfree(ctx, line);
}


void printRTLINE(const RTCTX *ctx, RTLINE *line)
{
  rtnotice(ctx, "RTLINE {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(line->flags));
  rtnotice(ctx, "    srid = %i", (int)line->srid);
  printPA(ctx, line->points);
  rtnotice(ctx, "}");
}

/* @brief Clone RTLINE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
RTLINE *
rtline_clone(const RTCTX *ctx, const RTLINE *g)
{
  RTLINE *ret = rtalloc(ctx, sizeof(RTLINE));

  RTDEBUGF(ctx, 2, "rtline_clone called with %p", g);

  memcpy(ret, g, sizeof(RTLINE));

  ret->points = ptarray_clone(ctx, g->points);

  if ( g->bbox ) ret->bbox = gbox_copy(ctx, g->bbox);
  return ret;
}

/* Deep clone RTLINE object. RTPOINTARRAY *is* copied. */
RTLINE *
rtline_clone_deep(const RTCTX *ctx, const RTLINE *g)
{
  RTLINE *ret = rtalloc(ctx, sizeof(RTLINE));

  RTDEBUGF(ctx, 2, "rtline_clone_deep called with %p", g);
  memcpy(ret, g, sizeof(RTLINE));

  if ( g->bbox ) ret->bbox = gbox_copy(ctx, g->bbox);
  if ( g->points ) ret->points = ptarray_clone_deep(ctx, g->points);
  RTFLAGS_SET_READONLY(ret->flags,0);

  return ret;
}


void
rtline_release(const RTCTX *ctx, RTLINE *rtline)
{
  rtgeom_release(ctx, rtline_as_rtgeom(ctx, rtline));
}

void
rtline_reverse(const RTCTX *ctx, RTLINE *line)
{
  if ( rtline_is_empty(ctx, line) ) return;
  ptarray_reverse(ctx, line->points);
}

RTLINE *
rtline_segmentize2d(const RTCTX *ctx, RTLINE *line, double dist)
{
  RTPOINTARRAY *segmentized = ptarray_segmentize2d(ctx, line->points, dist);
  if ( ! segmentized ) return NULL;
  return rtline_construct(ctx, line->srid, NULL, segmentized);
}

/* check coordinate equality  */
char
rtline_same(const RTCTX *ctx, const RTLINE *l1, const RTLINE *l2)
{
  return ptarray_same(ctx, l1->points, l2->points);
}

/*
 * Construct a RTLINE from an array of point and line geometries
 * RTLINE dimensions are large enough to host all input dimensions.
 */
RTLINE *
rtline_from_rtgeom_array(const RTCTX *ctx, int srid, uint32_t ngeoms, RTGEOM **geoms)
{
   int i;
  int hasz = RT_FALSE;
  int hasm = RT_FALSE;
  RTPOINTARRAY *pa;
  RTLINE *line;
  RTPOINT4D pt;

  /*
   * Find output dimensions, check integrity
   */
  for (i=0; i<ngeoms; i++)
  {
    if ( RTFLAGS_GET_Z(geoms[i]->flags) ) hasz = RT_TRUE;
    if ( RTFLAGS_GET_M(geoms[i]->flags) ) hasm = RT_TRUE;
    if ( hasz && hasm ) break; /* Nothing more to learn! */
  }

  /* ngeoms should be a guess about how many points we have in input */
  pa = ptarray_construct_empty(ctx, hasz, hasm, ngeoms);

  for ( i=0; i < ngeoms; i++ )
  {
    RTGEOM *g = geoms[i];

    if ( rtgeom_is_empty(ctx, g) ) continue;

    if ( g->type == RTPOINTTYPE )
    {
      rtpoint_getPoint4d_p(ctx, (RTPOINT*)g, &pt);
      ptarray_append_point(ctx, pa, &pt, RT_TRUE);
    }
    else if ( g->type == RTLINETYPE )
    {
      ptarray_append_ptarray(ctx, pa, ((RTLINE*)g)->points, -1);
    }
    else
    {
      ptarray_free(ctx, pa);
      rterror(ctx, "rtline_from_ptarray: invalid input type: %s", rttype_name(ctx, g->type));
      return NULL;
    }
  }

  if ( pa->npoints > 0 )
    line = rtline_construct(ctx, srid, NULL, pa);
  else  {
    /* Is this really any different from the above ? */
    ptarray_free(ctx, pa);
    line = rtline_construct_empty(ctx, srid, hasz, hasm);
  }

  return line;
}

/*
 * Construct a RTLINE from an array of RTPOINTs
 * RTLINE dimensions are large enough to host all input dimensions.
 */
RTLINE *
rtline_from_ptarray(const RTCTX *ctx, int srid, uint32_t npoints, RTPOINT **points)
{
   int i;
  int hasz = RT_FALSE;
  int hasm = RT_FALSE;
  RTPOINTARRAY *pa;
  RTLINE *line;
  RTPOINT4D pt;

  /*
   * Find output dimensions, check integrity
   */
  for (i=0; i<npoints; i++)
  {
    if ( points[i]->type != RTPOINTTYPE )
    {
      rterror(ctx, "rtline_from_ptarray: invalid input type: %s", rttype_name(ctx, points[i]->type));
      return NULL;
    }
    if ( RTFLAGS_GET_Z(points[i]->flags) ) hasz = RT_TRUE;
    if ( RTFLAGS_GET_M(points[i]->flags) ) hasm = RT_TRUE;
    if ( hasz && hasm ) break; /* Nothing more to learn! */
  }

  pa = ptarray_construct_empty(ctx, hasz, hasm, npoints);

  for ( i=0; i < npoints; i++ )
  {
    if ( ! rtpoint_is_empty(ctx, points[i]) )
    {
      rtpoint_getPoint4d_p(ctx, points[i], &pt);
      ptarray_append_point(ctx, pa, &pt, RT_TRUE);
    }
  }

  if ( pa->npoints > 0 )
    line = rtline_construct(ctx, srid, NULL, pa);
  else
    line = rtline_construct_empty(ctx, srid, hasz, hasm);

  return line;
}

/*
 * Construct a RTLINE from a RTMPOINT
 */
RTLINE *
rtline_from_rtmpoint(const RTCTX *ctx, int srid, const RTMPOINT *mpoint)
{
  uint32_t i;
  RTPOINTARRAY *pa = NULL;
  RTGEOM *rtgeom = (RTGEOM*)mpoint;
  RTPOINT4D pt;

  char hasz = rtgeom_has_z(ctx, rtgeom);
  char hasm = rtgeom_has_m(ctx, rtgeom);
  uint32_t npoints = mpoint->ngeoms;

  if ( rtgeom_is_empty(ctx, rtgeom) )
  {
    return rtline_construct_empty(ctx, srid, hasz, hasm);
  }

  pa = ptarray_construct(ctx, hasz, hasm, npoints);

  for (i=0; i < npoints; i++)
  {
    rt_getPoint4d_p(ctx, mpoint->geoms[i]->point, 0, &pt);
    ptarray_set_point4d(ctx, pa, i, &pt);
  }

  RTDEBUGF(ctx, 3, "rtline_from_rtmpoint: constructed pointarray for %d points", mpoint->ngeoms);

  return rtline_construct(ctx, srid, NULL, pa);
}

/**
* Returns freshly allocated #RTPOINT that corresponds to the index where.
* Returns NULL if the geometry is empty or the index invalid.
*/
RTPOINT*
rtline_get_rtpoint(const RTCTX *ctx, const RTLINE *line, int where)
{
  RTPOINT4D pt;
  RTPOINT *rtpoint;
  RTPOINTARRAY *pa;

  if ( rtline_is_empty(ctx, line) || where < 0 || where >= line->points->npoints )
    return NULL;

  pa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(line->flags), RTFLAGS_GET_M(line->flags), 1);
  pt = rt_getPoint4d(ctx, line->points, where);
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);
  rtpoint = rtpoint_construct(ctx, line->srid, NULL, pa);
  return rtpoint;
}


int
rtline_add_rtpoint(const RTCTX *ctx, RTLINE *line, RTPOINT *point, int where)
{
  RTPOINT4D pt;
  rt_getPoint4d_p(ctx, point->point, 0, &pt);

  if ( ptarray_insert_point(ctx, line->points, &pt, where) != RT_SUCCESS )
    return RT_FAILURE;

  /* Update the bounding box */
  if ( line->bbox )
  {
    rtgeom_drop_bbox(ctx, rtline_as_rtgeom(ctx, line));
    rtgeom_add_bbox(ctx, rtline_as_rtgeom(ctx, line));
  }

  return RT_SUCCESS;
}



RTLINE *
rtline_removepoint(const RTCTX *ctx, RTLINE *line, uint32_t index)
{
  RTPOINTARRAY *newpa;
  RTLINE *ret;

  newpa = ptarray_removePoint(ctx, line->points, index);

  ret = rtline_construct(ctx, line->srid, NULL, newpa);
  rtgeom_add_bbox(ctx, (RTGEOM *) ret);

  return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 */
void
rtline_setPoint4d(const RTCTX *ctx, RTLINE *line, uint32_t index, RTPOINT4D *newpoint)
{
  ptarray_set_point4d(ctx, line->points, index, newpoint);
  /* Update the box, if there is one to update */
  if ( line->bbox )
  {
    rtgeom_drop_bbox(ctx, (RTGEOM*)line);
    rtgeom_add_bbox(ctx, (RTGEOM*)line);
  }
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
RTLINE*
rtline_measured_from_rtline(const RTCTX *ctx, const RTLINE *rtline, double m_start, double m_end)
{
  int i = 0;
  int hasm = 0, hasz = 0;
  int npoints = 0;
  double length = 0.0;
  double length_so_far = 0.0;
  double m_range = m_end - m_start;
  double m;
  RTPOINTARRAY *pa = NULL;
  RTPOINT3DZ p1, p2;

  if ( rtline->type != RTLINETYPE )
  {
    rterror(ctx, "rtline_construct_from_rtline: only line types supported");
    return NULL;
  }

  hasz = RTFLAGS_GET_Z(rtline->flags);
  hasm = 1;

  /* Null points or npoints == 0 will result in empty return geometry */
  if ( rtline->points )
  {
    npoints = rtline->points->npoints;
    length = ptarray_length_2d(ctx, rtline->points);
    rt_getPoint3dz_p(ctx, rtline->points, 0, &p1);
  }

  pa = ptarray_construct(ctx, hasz, hasm, npoints);

  for ( i = 0; i < npoints; i++ )
  {
    RTPOINT4D q;
    RTPOINT2D a, b;
    rt_getPoint3dz_p(ctx, rtline->points, i, &p2);
    a.x = p1.x;
    a.y = p1.y;
    b.x = p2.x;
    b.y = p2.y;
    length_so_far += distance2d_pt_pt(ctx, &a, &b);
    if ( length > 0.0 )
      m = m_start + m_range * length_so_far / length;
    /* #3172, support (valid) zero-length inputs */
    else if ( length == 0.0 && npoints > 1 )
      m = m_start + m_range * i / (npoints-1);
    else
      m = 0.0;
    q.x = p2.x;
    q.y = p2.y;
    q.z = p2.z;
    q.m = m;
    ptarray_set_point4d(ctx, pa, i, &q);
    p1 = p2;
  }

  return rtline_construct(ctx, rtline->srid, NULL, pa);
}

RTGEOM*
rtline_remove_repeated_points(const RTCTX *ctx, const RTLINE *rtline, double tolerance)
{
  RTPOINTARRAY* npts = ptarray_remove_repeated_points_minpoints(ctx, rtline->points, tolerance, 2);

  RTDEBUGF(ctx, 3, "%s: npts %p", __func__, npts);

  return (RTGEOM*)rtline_construct(ctx, rtline->srid,
                                   rtline->bbox ? gbox_copy(ctx, rtline->bbox) : 0,
                                   npts);
}

int
rtline_is_closed(const RTCTX *ctx, const RTLINE *line)
{
  if (RTFLAGS_GET_Z(line->flags))
    return ptarray_is_closed_3d(ctx, line->points);

  return ptarray_is_closed_2d(ctx, line->points);
}

int
rtline_is_trajectory(const RTCTX *ctx, const RTLINE *line)
{
  RTPOINT3DM p;
  int i, n;
  double m = -1 * FLT_MAX;

  if ( ! RTFLAGS_GET_M(line->flags) ) {
    rtnotice(ctx, "Line does not have M dimension");
    return RT_FALSE;
  }

  n = line->points->npoints;
  if ( n < 2 ) return RT_TRUE; /* empty or single-point are "good" */

  for (i=0; i<n; ++i) {
    rt_getPoint3dm_p(ctx, line->points, i, &p);
    if ( p.m <= m ) {
      rtnotice(ctx, "Measure of vertex %d (%g) not bigger than measure of vertex %d (%g)",
        i, p.m, i-1, m);
      return RT_FALSE;
    }
    m = p.m;
  }

  return RT_TRUE;
}


RTLINE*
rtline_force_dims(const RTCTX *ctx, const RTLINE *line, int hasz, int hasm)
{
  RTPOINTARRAY *pdims = NULL;
  RTLINE *lineout;

  /* Return 2D empty */
  if( rtline_is_empty(ctx, line) )
  {
    lineout = rtline_construct_empty(ctx, line->srid, hasz, hasm);
  }
  else
  {
    pdims = ptarray_force_dims(ctx, line->points, hasz, hasm);
    lineout = rtline_construct(ctx, line->srid, NULL, pdims);
  }
  lineout->type = line->type;
  return lineout;
}

int rtline_is_empty(const RTCTX *ctx, const RTLINE *line)
{
  if ( !line->points || line->points->npoints < 1 )
    return RT_TRUE;
  return RT_FALSE;
}


int rtline_count_vertices(const RTCTX *ctx, RTLINE *line)
{
  assert(line);
  if ( ! line->points )
    return 0;
  return line->points->npoints;
}

RTLINE* rtline_simplify(const RTCTX *ctx, const RTLINE *iline, double dist, int preserve_collapsed)
{
  static const int minvertices = 2; /* TODO: allow setting this */
  RTLINE *oline;
  RTPOINTARRAY *pa;

  RTDEBUG(ctx, 2, "function called");

  /* Skip empty case */
  if( rtline_is_empty(ctx, iline) )
    return NULL;

  pa = ptarray_simplify(ctx, iline->points, dist, minvertices);
  if ( ! pa ) return NULL;

  /* Make sure single-point collapses have two points */
  if ( pa->npoints == 1 )
  {
    /* Make sure single-point collapses have two points */
    if ( preserve_collapsed )
    {
      RTPOINT4D pt;
      rt_getPoint4d_p(ctx, pa, 0, &pt);
      ptarray_append_point(ctx, pa, &pt, RT_TRUE);
    }
    /* Return null for collapse */
    else
    {
      ptarray_free(ctx, pa);
      return NULL;
    }
  }

  oline = rtline_construct(ctx, iline->srid, NULL, pa);
  oline->type = iline->type;
  return oline;
}

double rtline_length(const RTCTX *ctx, const RTLINE *line)
{
  if ( rtline_is_empty(ctx, line) )
    return 0.0;
  return ptarray_length(ctx, line->points);
}

double rtline_length_2d(const RTCTX *ctx, const RTLINE *line)
{
  if ( rtline_is_empty(ctx, line) )
    return 0.0;
  return ptarray_length_2d(ctx, line->points);
}



RTLINE* rtline_grid(const RTCTX *ctx, const RTLINE *line, const gridspec *grid)
{
  RTLINE *oline;
  RTPOINTARRAY *opa;

  opa = ptarray_grid(ctx, line->points, grid);

  /* Skip line3d with less then 2 points */
  if ( opa->npoints < 2 ) return NULL;

  /* TODO: grid bounding box... */
  oline = rtline_construct(ctx, line->srid, NULL, opa);

  return oline;
}

