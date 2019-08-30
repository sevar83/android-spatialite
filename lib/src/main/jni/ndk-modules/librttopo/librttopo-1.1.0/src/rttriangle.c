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
 * Copyright (C) 2010 - Oslandia
 *
 **********************************************************************/



/* basic RTTRIANGLE manipulation */

#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"



/* construct a new RTTRIANGLE.
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTTRIANGLE*
rttriangle_construct(const RTCTX *ctx, int srid, RTGBOX *bbox, RTPOINTARRAY *points)
{
  RTTRIANGLE *result;

  result = (RTTRIANGLE*) rtalloc(ctx, sizeof(RTTRIANGLE));
  result->type = RTTRIANGLETYPE;

  result->flags = points->flags;
  RTFLAGS_SET_BBOX(result->flags, bbox?1:0);

  result->srid = srid;
  result->points = points;
  result->bbox = bbox;

  return result;
}

RTTRIANGLE*
rttriangle_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTTRIANGLE *result = rtalloc(ctx, sizeof(RTTRIANGLE));
  result->type = RTTRIANGLETYPE;
  result->flags = gflags(ctx, hasz,hasm,0);
  result->srid = srid;
  result->points = ptarray_construct_empty(ctx, hasz, hasm, 1);
  result->bbox = NULL;
  return result;
}

void rttriangle_free(const RTCTX *ctx, RTTRIANGLE  *triangle)
{
  if ( ! triangle ) return;

  if (triangle->bbox)
    rtfree(ctx, triangle->bbox);

  if (triangle->points)
    ptarray_free(ctx, triangle->points);

  rtfree(ctx, triangle);
}

void printRTTRIANGLE(const RTCTX *ctx, RTTRIANGLE *triangle)
{
  if (triangle->type != RTTRIANGLETYPE)
                rterror(ctx, "printRTTRIANGLE called with something else than a Triangle");

  rtnotice(ctx, "RTTRIANGLE {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(triangle->flags));
  rtnotice(ctx, "    SRID = %i", (int)triangle->srid);
  printPA(ctx, triangle->points);
  rtnotice(ctx, "}");
}

/* @brief Clone RTTRIANGLE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
RTTRIANGLE *
rttriangle_clone(const RTCTX *ctx, const RTTRIANGLE *g)
{
  RTDEBUGF(ctx, 2, "rttriangle_clone called with %p", g);
  return (RTTRIANGLE *)rtline_clone(ctx, (const RTLINE *)g);
}

void
rttriangle_force_clockwise(const RTCTX *ctx, RTTRIANGLE *triangle)
{
  if ( ptarray_isccw(ctx, triangle->points) )
    ptarray_reverse(ctx, triangle->points);
}

void
rttriangle_reverse(const RTCTX *ctx, RTTRIANGLE *triangle)
{
  if( rttriangle_is_empty(ctx, triangle) ) return;
  ptarray_reverse(ctx, triangle->points);
}

void
rttriangle_release(const RTCTX *ctx, RTTRIANGLE *rttriangle)
{
  rtgeom_release(ctx, rttriangle_as_rtgeom(ctx, rttriangle));
}

/* check coordinate equality  */
char
rttriangle_same(const RTCTX *ctx, const RTTRIANGLE *t1, const RTTRIANGLE *t2)
{
  char r = ptarray_same(ctx, t1->points, t2->points);
  RTDEBUGF(ctx, 5, "returning %d", r);
  return r;
}

/*
 * Construct a triangle from a RTLINE being
 * the shell
 * Pointarray from intput geom are cloned.
 * Input line must have 4 points, and be closed.
 */
RTTRIANGLE *
rttriangle_from_rtline(const RTCTX *ctx, const RTLINE *shell)
{
  RTTRIANGLE *ret;
  RTPOINTARRAY *pa;

  if ( shell->points->npoints != 4 )
    rterror(ctx, "rttriangle_from_rtline: shell must have exactly 4 points");

  if (   (!RTFLAGS_GET_Z(shell->flags) && !ptarray_is_closed_2d(ctx, shell->points)) ||
          (RTFLAGS_GET_Z(shell->flags) && !ptarray_is_closed_3d(ctx, shell->points)) )
    rterror(ctx, "rttriangle_from_rtline: shell must be closed");

  pa = ptarray_clone_deep(ctx, shell->points);
  ret = rttriangle_construct(ctx, shell->srid, NULL, pa);

  if (rttriangle_is_repeated_points(ctx, ret))
    rterror(ctx, "rttriangle_from_rtline: some points are repeated in triangle");

  return ret;
}

char
rttriangle_is_repeated_points(const RTCTX *ctx, RTTRIANGLE *triangle)
{
  char ret;
  RTPOINTARRAY *pa;

  pa = ptarray_remove_repeated_points(ctx, triangle->points, 0.0);
  ret = ptarray_same(ctx, pa, triangle->points);
  ptarray_free(ctx, pa);

  return ret;
}

int rttriangle_is_empty(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  if ( !triangle->points || triangle->points->npoints < 1 )
    return RT_TRUE;
  return RT_FALSE;
}

/**
 * Find the area of the outer ring
 */
double
rttriangle_area(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  double area=0.0;
  int i;
  RTPOINT2D p1;
  RTPOINT2D p2;

  if (! triangle->points->npoints) return area; /* empty triangle */

  for (i=0; i < triangle->points->npoints-1; i++)
  {
    rt_getPoint2d_p(ctx, triangle->points, i, &p1);
    rt_getPoint2d_p(ctx, triangle->points, i+1, &p2);
    area += ( p1.x * p2.y ) - ( p1.y * p2.x );
  }

  area  /= 2.0;

  return fabs(area);
}


double
rttriangle_perimeter(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  if( triangle->points )
    return ptarray_length(ctx, triangle->points);
  else
    return 0.0;
}

double
rttriangle_perimeter_2d(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  if( triangle->points )
    return ptarray_length_2d(ctx, triangle->points);
  else
    return 0.0;
}
