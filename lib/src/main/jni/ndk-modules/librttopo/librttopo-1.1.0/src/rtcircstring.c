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



/* basic RTCIRCSTRING functions */

#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

void printRTCIRCSTRING(const RTCTX *ctx, RTCIRCSTRING *curve);
void rtcircstring_reverse(const RTCTX *ctx, RTCIRCSTRING *curve);
void rtcircstring_release(const RTCTX *ctx, RTCIRCSTRING *rtcirc);
char rtcircstring_same(const RTCTX *ctx, const RTCIRCSTRING *me, const RTCIRCSTRING *you);
RTCIRCSTRING * rtcircstring_from_rtpointarray(const RTCTX *ctx, int srid, uint32_t npoints, RTPOINT **points);
RTCIRCSTRING * rtcircstring_from_rtmpoint(const RTCTX *ctx, int srid, RTMPOINT *mpoint);
RTCIRCSTRING * rtcircstring_addpoint(const RTCTX *ctx, RTCIRCSTRING *curve, RTPOINT *point, uint32_t where);
RTCIRCSTRING * rtcircstring_removepoint(const RTCTX *ctx, RTCIRCSTRING *curve, uint32_t index);
void rtcircstring_setPoint4d(const RTCTX *ctx, RTCIRCSTRING *curve, uint32_t index, RTPOINT4D *newpoint);



/*
 * Construct a new RTCIRCSTRING.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTCIRCSTRING *
rtcircstring_construct(const RTCTX *ctx, int srid, RTGBOX *bbox, RTPOINTARRAY *points)
{
  RTCIRCSTRING *result;

  /*
  * The first arc requires three points.  Each additional
  * arc requires two more points.  Thus the minimum point count
  * is three, and the count must be odd.
  */
  if (points->npoints % 2 != 1 || points->npoints < 3)
  {
    rtnotice(ctx, "rtcircstring_construct: invalid point count %d", points->npoints);
  }

  result = (RTCIRCSTRING*) rtalloc(ctx, sizeof(RTCIRCSTRING));

  result->type = RTCIRCSTRINGTYPE;

  result->flags = points->flags;
  RTFLAGS_SET_BBOX(result->flags, bbox?1:0);

  result->srid = srid;
  result->points = points;
  result->bbox = bbox;

  return result;
}

RTCIRCSTRING *
rtcircstring_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTCIRCSTRING *result = rtalloc(ctx, sizeof(RTCIRCSTRING));
  result->type = RTCIRCSTRINGTYPE;
  result->flags = gflags(ctx, hasz,hasm,0);
  result->srid = srid;
  result->points = ptarray_construct_empty(ctx, hasz, hasm, 1);
  result->bbox = NULL;
  return result;
}

void
rtcircstring_release(const RTCTX *ctx, RTCIRCSTRING *rtcirc)
{
  rtgeom_release(ctx, rtcircstring_as_rtgeom(ctx, rtcirc));
}


void rtcircstring_free(const RTCTX *ctx, RTCIRCSTRING *curve)
{
  if ( ! curve ) return;

  if ( curve->bbox )
    rtfree(ctx, curve->bbox);
  if ( curve->points )
    ptarray_free(ctx, curve->points);
  rtfree(ctx, curve);
}



void printRTCIRCSTRING(const RTCTX *ctx, RTCIRCSTRING *curve)
{
  rtnotice(ctx, "RTCIRCSTRING {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(curve->flags));
  rtnotice(ctx, "    srid = %i", (int)curve->srid);
  printPA(ctx, curve->points);
  rtnotice(ctx, "}");
}

/* @brief Clone RTCIRCSTRING object. Serialized point lists are not copied.
 *
 * @see ptarray_clone
 */
RTCIRCSTRING *
rtcircstring_clone(const RTCTX *ctx, const RTCIRCSTRING *g)
{
  return (RTCIRCSTRING *)rtline_clone(ctx, (RTLINE *)g);
}


void rtcircstring_reverse(const RTCTX *ctx, RTCIRCSTRING *curve)
{
  ptarray_reverse(ctx, curve->points);
}

/* check coordinate equality */
char
rtcircstring_same(const RTCTX *ctx, const RTCIRCSTRING *me, const RTCIRCSTRING *you)
{
  return ptarray_same(ctx, me->points, you->points);
}

/*
 * Construct a RTCIRCSTRING from an array of RTPOINTs
 * RTCIRCSTRING dimensions are large enough to host all input dimensions.
 */
RTCIRCSTRING *
rtcircstring_from_rtpointarray(const RTCTX *ctx, int srid, uint32_t npoints, RTPOINT **points)
{
  int zmflag=0;
  uint32_t i;
  RTPOINTARRAY *pa;
  uint8_t *newpoints, *ptr;
  size_t ptsize, size;

  /*
   * Find output dimensions, check integrity
   */
  for (i = 0; i < npoints; i++)
  {
    if (points[i]->type != RTPOINTTYPE)
    {
      rterror(ctx, "rtcurve_from_rtpointarray: invalid input type: %s",
              rttype_name(ctx, points[i]->type));
      return NULL;
    }
    if (RTFLAGS_GET_Z(points[i]->flags)) zmflag |= 2;
    if (RTFLAGS_GET_M(points[i]->flags)) zmflag |= 1;
    if (zmflag == 3) break;
  }

  if (zmflag == 0) ptsize = 2 * sizeof(double);
  else if (zmflag == 3) ptsize = 4 * sizeof(double);
  else ptsize = 3 * sizeof(double);

  /*
   * Allocate output points array
   */
  size = ptsize * npoints;
  newpoints = rtalloc(ctx, size);
  memset(newpoints, 0, size);

  ptr = newpoints;
  for (i = 0; i < npoints; i++)
  {
    size = ptarray_point_size(ctx, points[i]->point);
    memcpy(ptr, rt_getPoint_internal(ctx, points[i]->point, 0), size);
    ptr += ptsize;
  }
  pa = ptarray_construct_reference_data(ctx, zmflag&2, zmflag&1, npoints, newpoints);

  return rtcircstring_construct(ctx, srid, NULL, pa);
}

/*
 * Construct a RTCIRCSTRING from a RTMPOINT
 */
RTCIRCSTRING *
rtcircstring_from_rtmpoint(const RTCTX *ctx, int srid, RTMPOINT *mpoint)
{
  uint32_t i;
  RTPOINTARRAY *pa;
  char zmflag = RTFLAGS_GET_ZM(mpoint->flags);
  size_t ptsize, size;
  uint8_t *newpoints, *ptr;

  if (zmflag == 0) ptsize = 2 * sizeof(double);
  else if (zmflag == 3) ptsize = 4 * sizeof(double);
  else ptsize = 3 * sizeof(double);

  /* Allocate space for output points */
  size = ptsize * mpoint->ngeoms;
  newpoints = rtalloc(ctx, size);
  memset(newpoints, 0, size);

  ptr = newpoints;
  for (i = 0; i < mpoint->ngeoms; i++)
  {
    memcpy(ptr,
           rt_getPoint_internal(ctx, mpoint->geoms[i]->point, 0),
           ptsize);
    ptr += ptsize;
  }

  pa = ptarray_construct_reference_data(ctx, zmflag&2, zmflag&1, mpoint->ngeoms, newpoints);

  RTDEBUGF(ctx, 3, "rtcurve_from_rtmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);

  return rtcircstring_construct(ctx, srid, NULL, pa);
}

RTCIRCSTRING *
rtcircstring_addpoint(const RTCTX *ctx, RTCIRCSTRING *curve, RTPOINT *point, uint32_t where)
{
  RTPOINTARRAY *newpa;
  RTCIRCSTRING *ret;

  newpa = ptarray_addPoint(ctx, curve->points,
                           rt_getPoint_internal(ctx, point->point, 0),
                           RTFLAGS_NDIMS(point->flags), where);
  ret = rtcircstring_construct(ctx, curve->srid, NULL, newpa);

  return ret;
}

RTCIRCSTRING *
rtcircstring_removepoint(const RTCTX *ctx, RTCIRCSTRING *curve, uint32_t index)
{
  RTPOINTARRAY *newpa;
  RTCIRCSTRING *ret;

  newpa = ptarray_removePoint(ctx, curve->points, index);
  ret = rtcircstring_construct(ctx, curve->srid, NULL, newpa);

  return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 * */
void
rtcircstring_setPoint4d(const RTCTX *ctx, RTCIRCSTRING *curve, uint32_t index, RTPOINT4D *newpoint)
{
  ptarray_set_point4d(ctx, curve->points, index, newpoint);
}

int
rtcircstring_is_closed(const RTCTX *ctx, const RTCIRCSTRING *curve)
{
  if (RTFLAGS_GET_Z(curve->flags))
    return ptarray_is_closed_3d(ctx, curve->points);

  return ptarray_is_closed_2d(ctx, curve->points);
}

int rtcircstring_is_empty(const RTCTX *ctx, const RTCIRCSTRING *circ)
{
  if ( !circ->points || circ->points->npoints < 1 )
    return RT_TRUE;
  return RT_FALSE;
}

double rtcircstring_length(const RTCTX *ctx, const RTCIRCSTRING *circ)
{
  return rtcircstring_length_2d(ctx, circ);
}

double rtcircstring_length_2d(const RTCTX *ctx, const RTCIRCSTRING *circ)
{
  if ( rtcircstring_is_empty(ctx, circ) )
    return 0.0;

  return ptarray_arc_length_2d(ctx, circ->points);
}

/*
 * Returns freshly allocated #RTPOINT that corresponds to the index where.
 * Returns NULL if the geometry is empty or the index invalid.
 */
RTPOINT* rtcircstring_get_rtpoint(const RTCTX *ctx, const RTCIRCSTRING *circ, int where) {
  RTPOINT4D pt;
  RTPOINT *rtpoint;
  RTPOINTARRAY *pa;

  if ( rtcircstring_is_empty(ctx, circ) || where < 0 || where >= circ->points->npoints )
    return NULL;

  pa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(circ->flags), RTFLAGS_GET_M(circ->flags), 1);
  pt = rt_getPoint4d(ctx, circ->points, where);
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);
  rtpoint = rtpoint_construct(ctx, circ->srid, NULL, pa);
  return rtpoint;
}

/*
* Snap to grid
*/
RTCIRCSTRING* rtcircstring_grid(const RTCTX *ctx, const RTCIRCSTRING *line, const gridspec *grid)
{
  RTCIRCSTRING *oline;
  RTPOINTARRAY *opa;

  opa = ptarray_grid(ctx, line->points, grid);

  /* Skip line3d with less then 2 points */
  if ( opa->npoints < 2 ) return NULL;

  /* TODO: grid bounding box... */
  oline = rtcircstring_construct(ctx, line->srid, NULL, opa);

  return oline;
}

