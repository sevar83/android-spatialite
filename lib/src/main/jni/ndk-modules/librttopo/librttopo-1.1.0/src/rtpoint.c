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
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rttopo_config.h"
/*#define RTGEOM_DEBUG_LEVEL 4*/
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"


/*
 * Convenience functions to hide the RTPOINTARRAY
 * TODO: obsolete this
 */
int
rtpoint_getPoint2d_p(const RTCTX *ctx, const RTPOINT *point, RTPOINT2D *out)
{
  return rt_getPoint2d_p(ctx, point->point, 0, out);
}

/* convenience functions to hide the RTPOINTARRAY */
int
rtpoint_getPoint3dz_p(const RTCTX *ctx, const RTPOINT *point, RTPOINT3DZ *out)
{
  return rt_getPoint3dz_p(ctx, point->point,0,out);
}
int
rtpoint_getPoint3dm_p(const RTCTX *ctx, const RTPOINT *point, RTPOINT3DM *out)
{
  return rt_getPoint3dm_p(ctx, point->point,0,out);
}
int
rtpoint_getPoint4d_p(const RTCTX *ctx, const RTPOINT *point, RTPOINT4D *out)
{
  return rt_getPoint4d_p(ctx, point->point,0,out);
}

double
rtpoint_get_x(const RTCTX *ctx, const RTPOINT *point)
{
  RTPOINT4D pt;
  if ( rtpoint_is_empty(ctx, point) )
    rterror(ctx, "rtpoint_get_x called with empty geometry");
  rt_getPoint4d_p(ctx, point->point, 0, &pt);
  return pt.x;
}

double
rtpoint_get_y(const RTCTX *ctx, const RTPOINT *point)
{
  RTPOINT4D pt;
  if ( rtpoint_is_empty(ctx, point) )
    rterror(ctx, "rtpoint_get_y called with empty geometry");
  rt_getPoint4d_p(ctx, point->point, 0, &pt);
  return pt.y;
}

double
rtpoint_get_z(const RTCTX *ctx, const RTPOINT *point)
{
  RTPOINT4D pt;
  if ( rtpoint_is_empty(ctx, point) )
    rterror(ctx, "rtpoint_get_z called with empty geometry");
  if ( ! RTFLAGS_GET_Z(point->flags) )
    rterror(ctx, "rtpoint_get_z called without z dimension");
  rt_getPoint4d_p(ctx, point->point, 0, &pt);
  return pt.z;
}

double
rtpoint_get_m(const RTCTX *ctx, const RTPOINT *point)
{
  RTPOINT4D pt;
  if ( rtpoint_is_empty(ctx, point) )
    rterror(ctx, "rtpoint_get_m called with empty geometry");
  if ( ! RTFLAGS_GET_M(point->flags) )
    rterror(ctx, "rtpoint_get_m called without m dimension");
  rt_getPoint4d_p(ctx, point->point, 0, &pt);
  return pt.m;
}

/*
 * Construct a new point.  point will not be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTPOINT *
rtpoint_construct(const RTCTX *ctx, int srid, RTGBOX *bbox, RTPOINTARRAY *point)
{
  RTPOINT *result;
  uint8_t flags = 0;

  if (point == NULL)
    return NULL; /* error */

  result = rtalloc(ctx, sizeof(RTPOINT));
  result->type = RTPOINTTYPE;
  RTFLAGS_SET_Z(flags, RTFLAGS_GET_Z(point->flags));
  RTFLAGS_SET_M(flags, RTFLAGS_GET_M(point->flags));
  RTFLAGS_SET_BBOX(flags, bbox?1:0);
  result->flags = flags;
  result->srid = srid;
  result->point = point;
  result->bbox = bbox;

  return result;
}

RTPOINT *
rtpoint_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTPOINT *result = rtalloc(ctx, sizeof(RTPOINT));
  result->type = RTPOINTTYPE;
  result->flags = gflags(ctx, hasz, hasm, 0);
  result->srid = srid;
  result->point = ptarray_construct(ctx, hasz, hasm, 0);
  result->bbox = NULL;
  return result;
}

RTPOINT *
rtpoint_make2d(const RTCTX *ctx, int srid, double x, double y)
{
  RTPOINT4D p = {x, y, 0.0, 0.0};
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, 0, 0, 1);

  ptarray_append_point(ctx, pa, &p, RT_TRUE);
  return rtpoint_construct(ctx, srid, NULL, pa);
}

RTPOINT *
rtpoint_make3dz(const RTCTX *ctx, int srid, double x, double y, double z)
{
  RTPOINT4D p = {x, y, z, 0.0};
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, 1, 0, 1);

  ptarray_append_point(ctx, pa, &p, RT_TRUE);

  return rtpoint_construct(ctx, srid, NULL, pa);
}

RTPOINT *
rtpoint_make3dm(const RTCTX *ctx, int srid, double x, double y, double m)
{
  RTPOINT4D p = {x, y, 0.0, m};
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, 0, 1, 1);

  ptarray_append_point(ctx, pa, &p, RT_TRUE);

  return rtpoint_construct(ctx, srid, NULL, pa);
}

RTPOINT *
rtpoint_make4d(const RTCTX *ctx, int srid, double x, double y, double z, double m)
{
  RTPOINT4D p = {x, y, z, m};
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, 1, 1, 1);

  ptarray_append_point(ctx, pa, &p, RT_TRUE);

  return rtpoint_construct(ctx, srid, NULL, pa);
}

RTPOINT *
rtpoint_make(const RTCTX *ctx, int srid, int hasz, int hasm, const RTPOINT4D *p)
{
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, hasz, hasm, 1);
  ptarray_append_point(ctx, pa, p, RT_TRUE);
  return rtpoint_construct(ctx, srid, NULL, pa);
}

void rtpoint_free(const RTCTX *ctx, RTPOINT *pt)
{
  if ( ! pt ) return;

  if ( pt->bbox )
    rtfree(ctx, pt->bbox);
  if ( pt->point )
    ptarray_free(ctx, pt->point);
  rtfree(ctx, pt);
}

void printRTPOINT(const RTCTX *ctx, RTPOINT *point)
{
  rtnotice(ctx, "RTPOINT {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(point->flags));
  rtnotice(ctx, "    BBOX = %i", RTFLAGS_GET_BBOX(point->flags) ? 1 : 0 );
  rtnotice(ctx, "    SRID = %i", (int)point->srid);
  printPA(ctx, point->point);
  rtnotice(ctx, "}");
}

/* @brief Clone RTPOINT object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
RTPOINT *
rtpoint_clone(const RTCTX *ctx, const RTPOINT *g)
{
  RTPOINT *ret = rtalloc(ctx, sizeof(RTPOINT));

  RTDEBUG(ctx, 2, "rtpoint_clone called");

  memcpy(ret, g, sizeof(RTPOINT));

  ret->point = ptarray_clone(ctx, g->point);

  if ( g->bbox ) ret->bbox = gbox_copy(ctx, g->bbox);
  return ret;
}



void
rtpoint_release(const RTCTX *ctx, RTPOINT *rtpoint)
{
  rtgeom_release(ctx, rtpoint_as_rtgeom(ctx, rtpoint));
}


/* check coordinate equality  */
char
rtpoint_same(const RTCTX *ctx, const RTPOINT *p1, const RTPOINT *p2)
{
  return ptarray_same(ctx, p1->point, p2->point);
}


RTPOINT*
rtpoint_force_dims(const RTCTX *ctx, const RTPOINT *point, int hasz, int hasm)
{
  RTPOINTARRAY *pdims = NULL;
  RTPOINT *pointout;

  /* Return 2D empty */
  if( rtpoint_is_empty(ctx, point) )
  {
    pointout = rtpoint_construct_empty(ctx, point->srid, hasz, hasm);
  }
  else
  {
    /* Artays we duplicate the ptarray and return */
    pdims = ptarray_force_dims(ctx, point->point, hasz, hasm);
    pointout = rtpoint_construct(ctx, point->srid, NULL, pdims);
  }
  pointout->type = point->type;
  return pointout;
}

int rtpoint_is_empty(const RTCTX *ctx, const RTPOINT *point)
{
  if ( ! point->point || point->point->npoints < 1 )
    return RT_TRUE;
  return RT_FALSE;
}


RTPOINT *
rtpoint_grid(const RTCTX *ctx, const RTPOINT *point, const gridspec *grid)
{
  RTPOINTARRAY *opa = ptarray_grid(ctx, point->point, grid);
  return rtpoint_construct(ctx, point->srid, NULL, opa);
}

