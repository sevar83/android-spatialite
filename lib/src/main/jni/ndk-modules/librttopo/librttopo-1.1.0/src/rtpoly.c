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



/* basic RTPOLY manipulation */

#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"


#define CHECK_POLY_RINGS_ZM 1

/* construct a new RTPOLY.  arrays (points/points per ring) will NOT be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTPOLY*
rtpoly_construct(const RTCTX *ctx, int srid, RTGBOX *bbox, uint32_t nrings, RTPOINTARRAY **points)
{
  RTPOLY *result;
  int hasz, hasm;
#ifdef CHECK_POLY_RINGS_ZM
  char zm;
  uint32_t i;
#endif

  if ( nrings < 1 ) rterror(ctx, "rtpoly_construct: need at least 1 ring");

  hasz = RTFLAGS_GET_Z(points[0]->flags);
  hasm = RTFLAGS_GET_M(points[0]->flags);

#ifdef CHECK_POLY_RINGS_ZM
  zm = RTFLAGS_GET_ZM(points[0]->flags);
  for (i=1; i<nrings; i++)
  {
    if ( zm != RTFLAGS_GET_ZM(points[i]->flags) )
      rterror(ctx, "rtpoly_construct: mixed dimensioned rings");
  }
#endif

  result = (RTPOLY*) rtalloc(ctx, sizeof(RTPOLY));
  result->type = RTPOLYGONTYPE;
  result->flags = gflags(ctx, hasz, hasm, 0);
  RTFLAGS_SET_BBOX(result->flags, bbox?1:0);
  result->srid = srid;
  result->nrings = nrings;
  result->maxrings = nrings;
  result->rings = points;
  result->bbox = bbox;

  return result;
}

RTPOLY*
rtpoly_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTPOLY *result = rtalloc(ctx, sizeof(RTPOLY));
  result->type = RTPOLYGONTYPE;
  result->flags = gflags(ctx, hasz,hasm,0);
  result->srid = srid;
  result->nrings = 0;
  result->maxrings = 1; /* Allocate room for ring, just in case. */
  result->rings = rtalloc(ctx, result->maxrings * sizeof(RTPOINTARRAY*));
  result->bbox = NULL;
  return result;
}

void rtpoly_free(const RTCTX *ctx, RTPOLY  *poly)
{
  int t;

  if( ! poly ) return;

  if ( poly->bbox )
    rtfree(ctx, poly->bbox);

  for (t=0; t<poly->nrings; t++)
  {
    if ( poly->rings[t] )
      ptarray_free(ctx, poly->rings[t]);
  }

  if ( poly->rings )
    rtfree(ctx, poly->rings);

  rtfree(ctx, poly);
}

void printRTPOLY(const RTCTX *ctx, RTPOLY *poly)
{
  int t;
  rtnotice(ctx, "RTPOLY {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(poly->flags));
  rtnotice(ctx, "    SRID = %i", (int)poly->srid);
  rtnotice(ctx, "    nrings = %i", (int)poly->nrings);
  for (t=0; t<poly->nrings; t++)
  {
    rtnotice(ctx, "    RING # %i :",t);
    printPA(ctx, poly->rings[t]);
  }
  rtnotice(ctx, "}");
}

/* @brief Clone RTLINE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
RTPOLY *
rtpoly_clone(const RTCTX *ctx, const RTPOLY *g)
{
  int i;
  RTPOLY *ret = rtalloc(ctx, sizeof(RTPOLY));
  memcpy(ret, g, sizeof(RTPOLY));
  ret->rings = rtalloc(ctx, sizeof(RTPOINTARRAY *)*g->nrings);
  for ( i = 0; i < g->nrings; i++ ) {
    ret->rings[i] = ptarray_clone(ctx, g->rings[i]);
  }
  if ( g->bbox ) ret->bbox = gbox_copy(ctx, g->bbox);
  return ret;
}

/* Deep clone RTPOLY object. RTPOINTARRAY are copied, as is ring array */
RTPOLY *
rtpoly_clone_deep(const RTCTX *ctx, const RTPOLY *g)
{
  int i;
  RTPOLY *ret = rtalloc(ctx, sizeof(RTPOLY));
  memcpy(ret, g, sizeof(RTPOLY));
  if ( g->bbox ) ret->bbox = gbox_copy(ctx, g->bbox);
  ret->rings = rtalloc(ctx, sizeof(RTPOINTARRAY *)*g->nrings);
  for ( i = 0; i < ret->nrings; i++ )
  {
    ret->rings[i] = ptarray_clone_deep(ctx, g->rings[i]);
  }
  RTFLAGS_SET_READONLY(ret->flags,0);
  return ret;
}

/**
* Add a ring to a polygon. Point array will be referenced, not copied.
*/
int
rtpoly_add_ring(const RTCTX *ctx, RTPOLY *poly, RTPOINTARRAY *pa)
{
  if( ! poly || ! pa )
    return RT_FAILURE;

  /* We have used up our storage, add some more. */
  if( poly->nrings >= poly->maxrings )
  {
    int new_maxrings = 2 * (poly->nrings + 1);
    poly->rings = rtrealloc(ctx, poly->rings, new_maxrings * sizeof(RTPOINTARRAY*));
    poly->maxrings = new_maxrings;
  }

  /* Add the new ring entry. */
  poly->rings[poly->nrings] = pa;
  poly->nrings++;

  return RT_SUCCESS;
}

void
rtpoly_force_clockwise(const RTCTX *ctx, RTPOLY *poly)
{
  int i;

  /* No-op empties */
  if ( rtpoly_is_empty(ctx, poly) )
    return;

  /* External ring */
  if ( ptarray_isccw(ctx, poly->rings[0]) )
    ptarray_reverse(ctx, poly->rings[0]);

  /* Internal rings */
  for (i=1; i<poly->nrings; i++)
    if ( ! ptarray_isccw(ctx, poly->rings[i]) )
      ptarray_reverse(ctx, poly->rings[i]);

}

void
rtpoly_release(const RTCTX *ctx, RTPOLY *rtpoly)
{
  rtgeom_release(ctx, rtpoly_as_rtgeom(ctx, rtpoly));
}

void
rtpoly_reverse(const RTCTX *ctx, RTPOLY *poly)
{
  int i;
  if ( rtpoly_is_empty(ctx, poly) ) return;
  for (i=0; i<poly->nrings; i++)
    ptarray_reverse(ctx, poly->rings[i]);
}

RTPOLY *
rtpoly_segmentize2d(const RTCTX *ctx, RTPOLY *poly, double dist)
{
  RTPOINTARRAY **newrings;
  uint32_t i;

  newrings = rtalloc(ctx, sizeof(RTPOINTARRAY *)*poly->nrings);
  for (i=0; i<poly->nrings; i++)
  {
    newrings[i] = ptarray_segmentize2d(ctx, poly->rings[i], dist);
    if ( ! newrings[i] ) {
      while (i--) ptarray_free(ctx, newrings[i]);
      rtfree(ctx, newrings);
      return NULL;
    }
  }
  return rtpoly_construct(ctx, poly->srid, NULL,
                          poly->nrings, newrings);
}

/*
 * check coordinate equality
 * ring and coordinate order is considered
 */
char
rtpoly_same(const RTCTX *ctx, const RTPOLY *p1, const RTPOLY *p2)
{
  uint32_t i;

  if ( p1->nrings != p2->nrings ) return 0;
  for (i=0; i<p1->nrings; i++)
  {
    if ( ! ptarray_same(ctx, p1->rings[i], p2->rings[i]) )
      return 0;
  }
  return 1;
}

/*
 * Construct a polygon from a RTLINE being
 * the shell and an array of RTLINE (possibly NULL) being holes.
 * Pointarrays from intput geoms are cloned.
 * SRID must be the same for each input line.
 * Input lines must have at least 4 points, and be closed.
 */
RTPOLY *
rtpoly_from_rtlines(const RTCTX *ctx, const RTLINE *shell,
                    uint32_t nholes, const RTLINE **holes)
{
  uint32_t nrings;
  RTPOINTARRAY **rings = rtalloc(ctx, (nholes+1)*sizeof(RTPOINTARRAY *));
  int srid = shell->srid;
  RTPOLY *ret;

  if ( shell->points->npoints < 4 )
    rterror(ctx, "rtpoly_from_rtlines: shell must have at least 4 points");
  if ( ! ptarray_is_closed_2d(ctx, shell->points) )
    rterror(ctx, "rtpoly_from_rtlines: shell must be closed");
  rings[0] = ptarray_clone_deep(ctx, shell->points);

  for (nrings=1; nrings<=nholes; nrings++)
  {
    const RTLINE *hole = holes[nrings-1];

    if ( hole->srid != srid )
      rterror(ctx, "rtpoly_from_rtlines: mixed SRIDs in input lines");

    if ( hole->points->npoints < 4 )
      rterror(ctx, "rtpoly_from_rtlines: holes must have at least 4 points");
    if ( ! ptarray_is_closed_2d(ctx, hole->points) )
      rterror(ctx, "rtpoly_from_rtlines: holes must be closed");

    rings[nrings] = ptarray_clone_deep(ctx, hole->points);
  }

  ret = rtpoly_construct(ctx, srid, NULL, nrings, rings);
  return ret;
}

RTGEOM*
rtpoly_remove_repeated_points(const RTCTX *ctx, const RTPOLY *poly, double tolerance)
{
  uint32_t i;
  RTPOINTARRAY **newrings;

  newrings = rtalloc(ctx, sizeof(RTPOINTARRAY *)*poly->nrings);
  for (i=0; i<poly->nrings; i++)
  {
    newrings[i] = ptarray_remove_repeated_points_minpoints(ctx, poly->rings[i], tolerance, 4);
  }

  return (RTGEOM*)rtpoly_construct(ctx, poly->srid,
                                   poly->bbox ? gbox_copy(ctx, poly->bbox) : NULL,
                                   poly->nrings, newrings);

}


RTPOLY*
rtpoly_force_dims(const RTCTX *ctx, const RTPOLY *poly, int hasz, int hasm)
{
  RTPOLY *polyout;

  /* Return 2D empty */
  if( rtpoly_is_empty(ctx, poly) )
  {
    polyout = rtpoly_construct_empty(ctx, poly->srid, hasz, hasm);
  }
  else
  {
    RTPOINTARRAY **rings = NULL;
    int i;
    rings = rtalloc(ctx, sizeof(RTPOINTARRAY*) * poly->nrings);
    for( i = 0; i < poly->nrings; i++ )
    {
      rings[i] = ptarray_force_dims(ctx, poly->rings[i], hasz, hasm);
    }
    polyout = rtpoly_construct(ctx, poly->srid, NULL, poly->nrings, rings);
  }
  polyout->type = poly->type;
  return polyout;
}

int rtpoly_is_empty(const RTCTX *ctx, const RTPOLY *poly)
{
  if ( (poly->nrings < 1) || (!poly->rings) || (!poly->rings[0]) || (poly->rings[0]->npoints < 1) )
    return RT_TRUE;
  return RT_FALSE;
}

int rtpoly_count_vertices(const RTCTX *ctx, RTPOLY *poly)
{
  int i = 0;
  int v = 0; /* vertices */
  assert(poly);
  for ( i = 0; i < poly->nrings; i ++ )
  {
    v += poly->rings[i]->npoints;
  }
  return v;
}

RTPOLY* rtpoly_simplify(const RTCTX *ctx, const RTPOLY *ipoly, double dist, int preserve_collapsed)
{
  int i;
  RTPOLY *opoly = rtpoly_construct_empty(ctx, ipoly->srid, RTFLAGS_GET_Z(ipoly->flags), RTFLAGS_GET_M(ipoly->flags));

  RTDEBUGF(ctx, 2, "%s: simplifying polygon with %d rings", __func__, ipoly->nrings);

  if ( rtpoly_is_empty(ctx, ipoly) )
  {
    rtpoly_free(ctx, opoly);
    return NULL;
  }

  for ( i = 0; i < ipoly->nrings; i++ )
  {
    RTPOINTARRAY *opts;
    int minvertices = 0;

    /* We'll still let holes collapse, but if we're preserving */
    /* and this is a shell, we ensure it is kept */
    if ( preserve_collapsed && i == 0 )
      minvertices = 4;

    opts = ptarray_simplify(ctx, ipoly->rings[i], dist, minvertices);

    RTDEBUGF(ctx, 3, "ring%d simplified from %d to %d points", i, ipoly->rings[i]->npoints, opts->npoints);

    /* Less points than are needed to form a closed ring, we can't use this */
    if ( opts->npoints < 4 )
    {
      RTDEBUGF(ctx, 3, "ring%d skipped (% pts)", i, opts->npoints);
      ptarray_free(ctx, opts);
      if ( i ) continue;
      else break; /* Don't scan holes if shell is collapsed */
    }

    /* Add ring to simplified polygon */
    if( rtpoly_add_ring(ctx, opoly, opts) == RT_FAILURE )
    {
      rtpoly_free(ctx, opoly);
      return NULL;
    }
  }

  RTDEBUGF(ctx, 3, "simplified polygon with %d rings", ipoly->nrings);
  opoly->type = ipoly->type;

  if( rtpoly_is_empty(ctx, opoly) )
  {
    rtpoly_free(ctx, opoly);
    return NULL;
  }

  return opoly;
}

/**
* Find the area of the outer ring - sum (area of inner rings).
*/
double
rtpoly_area(const RTCTX *ctx, const RTPOLY *poly)
{
  double poly_area = 0.0;
  int i;

  if ( ! poly )
    rterror(ctx, "rtpoly_area called with null polygon pointer!");

  for ( i=0; i < poly->nrings; i++ )
  {
    RTPOINTARRAY *ring = poly->rings[i];
    double ringarea = 0.0;

    /* Empty or messed-up ring. */
    if ( ring->npoints < 3 )
      continue;

    ringarea = fabs(ptarray_signed_area(ctx, ring));
    if ( i == 0 ) /* Outer ring, positive area! */
      poly_area += ringarea;
    else /* Inner ring, negative area! */
      poly_area -= ringarea;
  }

  return poly_area;
}


/**
 * Compute the sum of polygon rings length.
 * Could use a more numerically stable calculator...
 */
double
rtpoly_perimeter(const RTCTX *ctx, const RTPOLY *poly)
{
  double result=0.0;
  int i;

  RTDEBUGF(ctx, 2, "in rtgeom_polygon_perimeter (%d rings)", poly->nrings);

  for (i=0; i<poly->nrings; i++)
    result += ptarray_length(ctx, poly->rings[i]);

  return result;
}

/**
 * Compute the sum of polygon rings length (forcing 2d computation).
 * Could use a more numerically stable calculator...
 */
double
rtpoly_perimeter_2d(const RTCTX *ctx, const RTPOLY *poly)
{
  double result=0.0;
  int i;

  RTDEBUGF(ctx, 2, "in rtgeom_polygon_perimeter (%d rings)", poly->nrings);

  for (i=0; i<poly->nrings; i++)
    result += ptarray_length_2d(ctx, poly->rings[i]);

  return result;
}

int
rtpoly_is_closed(const RTCTX *ctx, const RTPOLY *poly)
{
  int i = 0;

  if ( poly->nrings == 0 )
    return RT_TRUE;

  for ( i = 0; i < poly->nrings; i++ )
  {
    if (RTFLAGS_GET_Z(poly->flags))
    {
      if ( ! ptarray_is_closed_3d(ctx, poly->rings[i]) )
        return RT_FALSE;
    }
    else
    {
      if ( ! ptarray_is_closed_2d(ctx, poly->rings[i]) )
        return RT_FALSE;
    }
  }

  return RT_TRUE;
}

int
rtpoly_startpoint(const RTCTX *ctx, const RTPOLY* poly, RTPOINT4D* pt)
{
  if ( poly->nrings < 1 )
    return RT_FAILURE;
  return ptarray_startpoint(ctx, poly->rings[0], pt);
}

int
rtpoly_contains_point(const RTCTX *ctx, const RTPOLY *poly, const RTPOINT2D *pt)
{
  int i;

  if ( rtpoly_is_empty(ctx, poly) )
    return RT_FALSE;

  if ( ptarray_contains_point(ctx, poly->rings[0], pt) == RT_OUTSIDE )
    return RT_FALSE;

  for ( i = 1; i < poly->nrings; i++ )
  {
    if ( ptarray_contains_point(ctx, poly->rings[i], pt) == RT_INSIDE )
      return RT_FALSE;
  }
  return RT_TRUE;
}



RTPOLY* rtpoly_grid(const RTCTX *ctx, const RTPOLY *poly, const gridspec *grid)
{
  RTPOLY *opoly;
  int ri;

#if 0
  /*
   * TODO: control this assertion
   * it is assumed that, since the grid size will be a pixel,
   * a visible ring should show at least a white pixel inside,
   * thus, for a square, that would be grid_xsize*grid_ysize
   */
  double minvisiblearea = grid->xsize * grid->ysize;
#endif

  RTDEBUGF(ctx, 3, "rtpoly_grid: applying grid to polygon with %d rings", poly->nrings);

  opoly = rtpoly_construct_empty(ctx, poly->srid, rtgeom_has_z(ctx, (RTGEOM*)poly), rtgeom_has_m(ctx, (RTGEOM*)poly));

  for (ri=0; ri<poly->nrings; ri++)
  {
    RTPOINTARRAY *ring = poly->rings[ri];
    RTPOINTARRAY *newring;

    newring = ptarray_grid(ctx, ring, grid);

    /* Skip ring if not composed by at least 4 pts (3 segments) */
    if ( newring->npoints < 4 )
    {
      ptarray_free(ctx, newring);

      RTDEBUGF(ctx, 3, "grid_polygon3d: ring%d skipped ( <4 pts )", ri);

      if ( ri ) continue;
      else break; /* this is the external ring, no need to work on holes */
    }

    if ( ! rtpoly_add_ring(ctx, opoly, newring) )
    {
      rterror(ctx, "rtpoly_grid, memory error");
      return NULL;
    }
  }

  RTDEBUGF(ctx, 3, "rtpoly_grid: simplified polygon with %d rings", opoly->nrings);

  if ( ! opoly->nrings )
  {
    rtpoly_free(ctx, opoly);
    return NULL;
  }

  return opoly;
}
