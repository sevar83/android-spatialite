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



#include <stdio.h>
#include <string.h>

#include "rttopo_config.h"
/*#define RTGEOM_DEBUG_LEVEL 4*/
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

int
ptarray_has_z(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  if ( ! pa ) return RT_FALSE;
  return RTFLAGS_GET_Z(pa->flags);
}

int
ptarray_has_m(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  if ( ! pa ) return RT_FALSE;
  return RTFLAGS_GET_M(pa->flags);
}

/*
 * Size of point represeneted in the RTPOINTARRAY
 * 16 for 2d, 24 for 3d, 32 for 4d
 */
int inline
ptarray_point_size(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  RTDEBUGF(ctx, 5, "ptarray_point_size: RTFLAGS_NDIMS(pa->flags)=%x",RTFLAGS_NDIMS(pa->flags));

  return sizeof(double)*RTFLAGS_NDIMS(pa->flags);
}

RTPOINTARRAY*
ptarray_construct(const RTCTX *ctx, char hasz, char hasm, uint32_t npoints)
{
  RTPOINTARRAY *pa = ptarray_construct_empty(ctx, hasz, hasm, npoints);
  pa->npoints = npoints;
  return pa;
}

RTPOINTARRAY*
ptarray_construct_empty(const RTCTX *ctx, char hasz, char hasm, uint32_t maxpoints)
{
  RTPOINTARRAY *pa = rtalloc(ctx, sizeof(RTPOINTARRAY));
  pa->serialized_pointlist = NULL;

  /* Set our dimsionality info on the bitmap */
  pa->flags = gflags(ctx, hasz, hasm, 0);

  /* We will be allocating a bit of room */
  pa->npoints = 0;
  pa->maxpoints = maxpoints;

  /* Allocate the coordinate array */
  if ( maxpoints > 0 )
    pa->serialized_pointlist = rtalloc(ctx, maxpoints * ptarray_point_size(ctx, pa));
  else
    pa->serialized_pointlist = NULL;

  return pa;
}

/*
* Add a point into a pointarray. Only adds as many dimensions as the
* pointarray supports.
*/
int
ptarray_insert_point(const RTCTX *ctx, RTPOINTARRAY *pa, const RTPOINT4D *p, int where)
{
  size_t point_size = ptarray_point_size(ctx, pa);
  RTDEBUGF(ctx, 5,"pa = %p; p = %p; where = %d", pa, p, where);
  RTDEBUGF(ctx, 5,"pa->npoints = %d; pa->maxpoints = %d", pa->npoints, pa->maxpoints);

  if ( RTFLAGS_GET_READONLY(pa->flags) )
  {
    rterror(ctx, "ptarray_insert_point: called on read-only point array");
    return RT_FAILURE;
  }

  /* Error on invalid offset value */
  if ( where > pa->npoints || where < 0)
  {
    rterror(ctx, "ptarray_insert_point: offset out of range (%d)", where);
    return RT_FAILURE;
  }

  /* If we have no storage, let's allocate some */
  if( pa->maxpoints == 0 || ! pa->serialized_pointlist )
  {
    pa->maxpoints = 32;
    pa->npoints = 0;
    pa->serialized_pointlist = rtalloc(ctx, ptarray_point_size(ctx, pa) * pa->maxpoints);
  }

  /* Error out if we have a bad situation */
  if ( pa->npoints > pa->maxpoints )
  {
    rterror(ctx, "npoints (%d) is greated than maxpoints (%d)", pa->npoints, pa->maxpoints);
    return RT_FAILURE;
  }

  /* Check if we have enough storage, add more if necessary */
  if( pa->npoints == pa->maxpoints )
  {
    pa->maxpoints *= 2;
    pa->serialized_pointlist = rtrealloc(ctx, pa->serialized_pointlist, ptarray_point_size(ctx, pa) * pa->maxpoints);
  }

  /* Make space to insert the new point */
  if( where < pa->npoints )
  {
    size_t copy_size = point_size * (pa->npoints - where);
    memmove(rt_getPoint_internal(ctx, pa, where+1), rt_getPoint_internal(ctx, pa, where), copy_size);
    RTDEBUGF(ctx, 5,"copying %d bytes to start vertex %d from start vertex %d", copy_size, where+1, where);
  }

  /* We have one more point */
  ++pa->npoints;

  /* Copy the new point into the gap */
  ptarray_set_point4d(ctx, pa, where, p);
  RTDEBUGF(ctx, 5,"copying new point to start vertex %d", point_size, where);

  return RT_SUCCESS;
}

int
ptarray_append_point(const RTCTX *ctx, RTPOINTARRAY *pa, const RTPOINT4D *pt, int repeated_points)
{

  /* Check for pathology */
  if( ! pa || ! pt )
  {
    rterror(ctx, "ptarray_append_point: null input");
    return RT_FAILURE;
  }

  /* Check for duplicate end point */
  if ( repeated_points == RT_FALSE && pa->npoints > 0 )
  {
    RTPOINT4D tmp;
    rt_getPoint4d_p(ctx, pa, pa->npoints-1, &tmp);
    RTDEBUGF(ctx, 4,"checking for duplicate end point (pt = POINT(%g %g) pa->npoints-q = POINT(%g %g))",pt->x,pt->y,tmp.x,tmp.y);

    /* Return RT_SUCCESS and do nothing else if previous point in list is equal to this one */
    if ( (pt->x == tmp.x) && (pt->y == tmp.y) &&
         (RTFLAGS_GET_Z(pa->flags) ? pt->z == tmp.z : 1) &&
         (RTFLAGS_GET_M(pa->flags) ? pt->m == tmp.m : 1) )
    {
      return RT_SUCCESS;
    }
  }

  /* Append is just a special case of insert */
  return ptarray_insert_point(ctx, pa, pt, pa->npoints);
}

int
ptarray_append_ptarray(const RTCTX *ctx, RTPOINTARRAY *pa1, RTPOINTARRAY *pa2, double gap_tolerance)
{
  unsigned int poff = 0;
  unsigned int npoints;
  unsigned int ncap;
  unsigned int ptsize;

  /* Check for pathology */
  if( ! pa1 || ! pa2 )
  {
    rterror(ctx, "ptarray_append_ptarray: null input");
    return RT_FAILURE;
  }

  npoints = pa2->npoints;

  if ( ! npoints ) return RT_SUCCESS; /* nothing more to do */

  if( RTFLAGS_GET_READONLY(pa1->flags) )
  {
    rterror(ctx, "ptarray_append_ptarray: target pointarray is read-only");
    return RT_FAILURE;
  }

  if( RTFLAGS_GET_ZM(pa1->flags) != RTFLAGS_GET_ZM(pa2->flags) )
  {
    rterror(ctx, "ptarray_append_ptarray: appending mixed dimensionality is not allowed");
    return RT_FAILURE;
  }

  ptsize = ptarray_point_size(ctx, pa1);

  /* Check for duplicate end point */
  if ( pa1->npoints )
  {
    RTPOINT2D tmp1, tmp2;
    rt_getPoint2d_p(ctx, pa1, pa1->npoints-1, &tmp1);
    rt_getPoint2d_p(ctx, pa2, 0, &tmp2);

    /* If the end point and start point are the same, then don't copy start point */
    if (p2d_same(ctx, &tmp1, &tmp2)) {
      poff = 1;
      --npoints;
    }
    else if ( gap_tolerance == 0 || ( gap_tolerance > 0 &&
               distance2d_pt_pt(ctx, &tmp1, &tmp2) > gap_tolerance ) )
    {
      rterror(ctx, "Second line start point too far from first line end point");
      return RT_FAILURE;
    }
  }

  /* Check if we need extra space */
  ncap = pa1->npoints + npoints;
  if ( pa1->maxpoints < ncap )
  {
    pa1->maxpoints = ncap > pa1->maxpoints*2 ?
                     ncap : pa1->maxpoints*2;
    pa1->serialized_pointlist = rtrealloc(ctx, pa1->serialized_pointlist, ptsize * pa1->maxpoints);
  }

  memcpy(rt_getPoint_internal(ctx, pa1, pa1->npoints),
         rt_getPoint_internal(ctx, pa2, poff), ptsize * npoints);

  pa1->npoints = ncap;

  return RT_SUCCESS;
}

/*
* Add a point into a pointarray. Only adds as many dimensions as the
* pointarray supports.
*/
int
ptarray_remove_point(const RTCTX *ctx, RTPOINTARRAY *pa, int where)
{
  size_t ptsize = ptarray_point_size(ctx, pa);

  /* Check for pathology */
  if( ! pa )
  {
    rterror(ctx, "ptarray_remove_point: null input");
    return RT_FAILURE;
  }

  /* Error on invalid offset value */
  if ( where >= pa->npoints || where < 0)
  {
    rterror(ctx, "ptarray_remove_point: offset out of range (%d)", where);
    return RT_FAILURE;
  }

  /* If the point is any but the last, we need to copy the data back one point */
  if( where < pa->npoints - 1 )
  {
    memmove(rt_getPoint_internal(ctx, pa, where), rt_getPoint_internal(ctx, pa, where+1), ptsize * (pa->npoints - where - 1));
  }

  /* We have one less point */
  pa->npoints--;

  return RT_SUCCESS;
}

/**
* Build a new #RTPOINTARRAY, but on top of someone else's ordinate array.
* Flag as read-only, so that ptarray_free(ctx) does not free the serialized_ptlist
*/
RTPOINTARRAY* ptarray_construct_reference_data(const RTCTX *ctx, char hasz, char hasm, uint32_t npoints, uint8_t *ptlist)
{
  RTPOINTARRAY *pa = rtalloc(ctx, sizeof(RTPOINTARRAY));
  RTDEBUGF(ctx, 5, "hasz = %d, hasm = %d, npoints = %d, ptlist = %p", hasz, hasm, npoints, ptlist);
  pa->flags = gflags(ctx, hasz, hasm, 0);
  RTFLAGS_SET_READONLY(pa->flags, 1); /* We don't own this memory, so we can't alter or free it. */
  pa->npoints = npoints;
  pa->maxpoints = npoints;
  pa->serialized_pointlist = ptlist;
  return pa;
}


RTPOINTARRAY*
ptarray_construct_copy_data(const RTCTX *ctx, char hasz, char hasm, uint32_t npoints, const uint8_t *ptlist)
{
  RTPOINTARRAY *pa = rtalloc(ctx, sizeof(RTPOINTARRAY));

  pa->flags = gflags(ctx, hasz, hasm, 0);
  pa->npoints = npoints;
  pa->maxpoints = npoints;

  if ( npoints > 0 )
  {
    pa->serialized_pointlist = rtalloc(ctx, ptarray_point_size(ctx, pa) * npoints);
    memcpy(pa->serialized_pointlist, ptlist, ptarray_point_size(ctx, pa) * npoints);
  }
  else
  {
    pa->serialized_pointlist = NULL;
  }

  return pa;
}

void ptarray_free(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  if(pa)
  {
    if(pa->serialized_pointlist && ( ! RTFLAGS_GET_READONLY(pa->flags) ) )
      rtfree(ctx, pa->serialized_pointlist);
    rtfree(ctx, pa);
    RTDEBUG(ctx, 5,"Freeing a PointArray");
  }
}


void
ptarray_reverse(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  /* TODO change this to double array operations once point array is double aligned */
  RTPOINT4D pbuf;
  uint32_t i;
  int ptsize = ptarray_point_size(ctx, pa);
  int last = pa->npoints-1;
  int mid = pa->npoints/2;

  for (i=0; i<mid; i++)
  {
    uint8_t *from, *to;
    from = rt_getPoint_internal(ctx, pa, i);
    to = rt_getPoint_internal(ctx, pa, (last-i));
    memcpy((uint8_t *)&pbuf, to, ptsize);
    memcpy(to, from, ptsize);
    memcpy(from, (uint8_t *)&pbuf, ptsize);
  }

}


/**
 * Reverse X and Y axis on a given RTPOINTARRAY
 */
RTPOINTARRAY*
ptarray_flip_coordinates(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  int i;
  double d;
  RTPOINT4D p;

  for (i=0 ; i < pa->npoints ; i++)
  {
    rt_getPoint4d_p(ctx, pa, i, &p);
    d = p.y;
    p.y = p.x;
    p.x = d;
    ptarray_set_point4d(ctx, pa, i, &p);
  }

  return pa;
}

void
ptarray_swap_ordinates(const RTCTX *ctx, RTPOINTARRAY *pa, RTORD o1, RTORD o2)
{
  int i;
  double d, *dp1, *dp2;
  RTPOINT4D p;

#if PARANOIA_LEVEL > 0
  assert(o1 < 4);
  assert(o2 < 4);
#endif

  dp1 = ((double*)&p)+(unsigned)o1;
  dp2 = ((double*)&p)+(unsigned)o2;
  for (i=0 ; i < pa->npoints ; i++)
  {
    rt_getPoint4d_p(ctx, pa, i, &p);
    d = *dp2;
    *dp2 = *dp1;
    *dp1 = d;
    ptarray_set_point4d(ctx, pa, i, &p);
  }
}


/**
 * @brief Returns a modified #RTPOINTARRAY so that no segment is
 *     longer than the given distance (computed using 2d).
 *
 * Every input point is kept.
 * Z and M values for added points (if needed) are set to 0.
 */
RTPOINTARRAY *
ptarray_segmentize2d(const RTCTX *ctx, const RTPOINTARRAY *ipa, double dist)
{
  double  segdist;
  RTPOINT4D  p1, p2;
  RTPOINT4D pbuf;
  RTPOINTARRAY *opa;
  int ipoff=0; /* input point offset */
  int hasz = RTFLAGS_GET_Z(ipa->flags);
  int hasm = RTFLAGS_GET_M(ipa->flags);

  pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0;

  /* Initial storage */
  opa = ptarray_construct_empty(ctx, hasz, hasm, ipa->npoints);

  /* Add first point */
  rt_getPoint4d_p(ctx, ipa, ipoff, &p1);
  ptarray_append_point(ctx, opa, &p1, RT_FALSE);

  ipoff++;

  while (ipoff<ipa->npoints)
  {
    /*
     * We use these pointers to avoid
     * "strict-aliasing rules break" warning raised
     * by gcc (3.3 and up).
     *
     * It looks that casting a variable address (also
     * referred to as "type-punned pointer")
     * breaks those "strict" rules.
     *
     */
    RTPOINT4D *p1ptr=&p1, *p2ptr=&p2;

    rt_getPoint4d_p(ctx, ipa, ipoff, &p2);

    segdist = distance2d_pt_pt(ctx, (RTPOINT2D *)p1ptr, (RTPOINT2D *)p2ptr);

    if (segdist > dist) /* add an intermediate point */
    {
      pbuf.x = p1.x + (p2.x-p1.x)/segdist * dist;
      pbuf.y = p1.y + (p2.y-p1.y)/segdist * dist;
      if( hasz )
        pbuf.z = p1.z + (p2.z-p1.z)/segdist * dist;
      if( hasm )
        pbuf.m = p1.m + (p2.m-p1.m)/segdist * dist;
      ptarray_append_point(ctx, opa, &pbuf, RT_FALSE);
      p1 = pbuf;
    }
    else /* copy second point */
    {
      ptarray_append_point(ctx, opa, &p2, (ipa->npoints==2)?RT_TRUE:RT_FALSE);
      p1 = p2;
      ipoff++;
    }

    RT_ON_INTERRUPT(ptarray_free(ctx, opa); return NULL);
  }

  return opa;
}

char
ptarray_same(const RTCTX *ctx, const RTPOINTARRAY *pa1, const RTPOINTARRAY *pa2)
{
  uint32_t i;
  size_t ptsize;

  if ( RTFLAGS_GET_ZM(pa1->flags) != RTFLAGS_GET_ZM(pa2->flags) ) return RT_FALSE;
  RTDEBUG(ctx, 5,"dimensions are the same");

  if ( pa1->npoints != pa2->npoints ) return RT_FALSE;
  RTDEBUG(ctx, 5,"npoints are the same");

  ptsize = ptarray_point_size(ctx, pa1);
  RTDEBUGF(ctx, 5, "ptsize = %d", ptsize);

  for (i=0; i<pa1->npoints; i++)
  {
    if ( memcmp(rt_getPoint_internal(ctx, pa1, i), rt_getPoint_internal(ctx, pa2, i), ptsize) )
      return RT_FALSE;
    RTDEBUGF(ctx, 5,"point #%d is the same",i);
  }

  return RT_TRUE;
}

RTPOINTARRAY *
ptarray_addPoint(const RTCTX *ctx, const RTPOINTARRAY *pa, uint8_t *p, size_t pdims, uint32_t where)
{
  RTPOINTARRAY *ret;
  RTPOINT4D pbuf;
  size_t ptsize = ptarray_point_size(ctx, pa);

  RTDEBUGF(ctx, 3, "pa %x p %x size %d where %d",
           pa, p, pdims, where);

  if ( pdims < 2 || pdims > 4 )
  {
    rterror(ctx, "ptarray_addPoint: point dimension out of range (%d)",
            pdims);
    return NULL;
  }

  if ( where > pa->npoints )
  {
    rterror(ctx, "ptarray_addPoint: offset out of range (%d)",
            where);
    return NULL;
  }

  RTDEBUG(ctx, 3, "called with a %dD point");

  pbuf.x = pbuf.y = pbuf.z = pbuf.m = 0.0;
  memcpy((uint8_t *)&pbuf, p, pdims*sizeof(double));

  RTDEBUG(ctx, 3, "initialized point buffer");

  ret = ptarray_construct(ctx, RTFLAGS_GET_Z(pa->flags),
                          RTFLAGS_GET_M(pa->flags), pa->npoints+1);

  if ( where == -1 ) where = pa->npoints;

  if ( where )
  {
    memcpy(rt_getPoint_internal(ctx, ret, 0), rt_getPoint_internal(ctx, pa, 0), ptsize*where);
  }

  memcpy(rt_getPoint_internal(ctx, ret, where), (uint8_t *)&pbuf, ptsize);

  if ( where+1 != ret->npoints )
  {
    memcpy(rt_getPoint_internal(ctx, ret, where+1),
           rt_getPoint_internal(ctx, pa, where),
           ptsize*(pa->npoints-where));
  }

  return ret;
}

RTPOINTARRAY *
ptarray_removePoint(const RTCTX *ctx, RTPOINTARRAY *pa, uint32_t which)
{
  RTPOINTARRAY *ret;
  size_t ptsize = ptarray_point_size(ctx, pa);

  RTDEBUGF(ctx, 3, "pa %x which %d", pa, which);

#if PARANOIA_LEVEL > 0
  if ( which > pa->npoints-1 )
  {
    rterror(ctx, "ptarray_removePoint: offset (%d) out of range (%d..%d)",
            which, 0, pa->npoints-1);
    return NULL;
  }

  if ( pa->npoints < 3 )
  {
    rterror(ctx, "ptarray_removePointe: can't remove a point from a 2-vertex RTPOINTARRAY");
  }
#endif

  ret = ptarray_construct(ctx, RTFLAGS_GET_Z(pa->flags),
                          RTFLAGS_GET_M(pa->flags), pa->npoints-1);

  /* copy initial part */
  if ( which )
  {
    memcpy(rt_getPoint_internal(ctx, ret, 0), rt_getPoint_internal(ctx, pa, 0), ptsize*which);
  }

  /* copy final part */
  if ( which < pa->npoints-1 )
  {
    memcpy(rt_getPoint_internal(ctx, ret, which), rt_getPoint_internal(ctx, pa, which+1),
           ptsize*(pa->npoints-which-1));
  }

  return ret;
}

RTPOINTARRAY *
ptarray_merge(const RTCTX *ctx, RTPOINTARRAY *pa1, RTPOINTARRAY *pa2)
{
  RTPOINTARRAY *pa;
  size_t ptsize = ptarray_point_size(ctx, pa1);

  if (RTFLAGS_GET_ZM(pa1->flags) != RTFLAGS_GET_ZM(pa2->flags))
    rterror(ctx, "ptarray_cat: Mixed dimension");

  pa = ptarray_construct(ctx,  RTFLAGS_GET_Z(pa1->flags),
                          RTFLAGS_GET_M(pa1->flags),
                          pa1->npoints + pa2->npoints);

  memcpy(         rt_getPoint_internal(ctx, pa, 0),
                  rt_getPoint_internal(ctx, pa1, 0),
                  ptsize*(pa1->npoints));

  memcpy(         rt_getPoint_internal(ctx, pa, pa1->npoints),
                  rt_getPoint_internal(ctx, pa2, 0),
                  ptsize*(pa2->npoints));

  ptarray_free(ctx, pa1);
  ptarray_free(ctx, pa2);

  return pa;
}


/**
 * @brief Deep clone a pointarray (also clones serialized pointlist)
 */
RTPOINTARRAY *
ptarray_clone_deep(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  RTPOINTARRAY *out = rtalloc(ctx, sizeof(RTPOINTARRAY));
  size_t size;

  RTDEBUG(ctx, 3, "ptarray_clone_deep called.");

  out->flags = in->flags;
  out->npoints = in->npoints;
  out->maxpoints = in->maxpoints;

  RTFLAGS_SET_READONLY(out->flags, 0);

  size = in->npoints * ptarray_point_size(ctx, in);
  out->serialized_pointlist = rtalloc(ctx, size);
  memcpy(out->serialized_pointlist, in->serialized_pointlist, size);

  return out;
}

/**
 * @brief Clone a RTPOINTARRAY object. Serialized pointlist is not copied.
 */
RTPOINTARRAY *
ptarray_clone(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  RTPOINTARRAY *out = rtalloc(ctx, sizeof(RTPOINTARRAY));

  RTDEBUG(ctx, 3, "ptarray_clone_deep called.");

  out->flags = in->flags;
  out->npoints = in->npoints;
  out->maxpoints = in->maxpoints;

  RTFLAGS_SET_READONLY(out->flags, 1);

  out->serialized_pointlist = in->serialized_pointlist;

  return out;
}

/**
* Check for ring closure using whatever dimensionality is declared on the
* pointarray.
*/
int
ptarray_is_closed(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  return 0 == memcmp(rt_getPoint_internal(ctx, in, 0), rt_getPoint_internal(ctx, in, in->npoints-1), ptarray_point_size(ctx, in));
}


int
ptarray_is_closed_2d(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  return 0 == memcmp(rt_getPoint_internal(ctx, in, 0), rt_getPoint_internal(ctx, in, in->npoints-1), sizeof(RTPOINT2D));
}

int
ptarray_is_closed_3d(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  return 0 == memcmp(rt_getPoint_internal(ctx, in, 0), rt_getPoint_internal(ctx, in, in->npoints-1), sizeof(POINT3D));
}

int
ptarray_is_closed_z(const RTCTX *ctx, const RTPOINTARRAY *in)
{
  if ( RTFLAGS_GET_Z(in->flags) )
    return ptarray_is_closed_3d(ctx, in);
  else
    return ptarray_is_closed_2d(ctx, in);
}

/**
* Return 1 if the point is inside the RTPOINTARRAY, -1 if it is outside,
* and 0 if it is on the boundary.
*/
int
ptarray_contains_point(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt)
{
  return ptarray_contains_point_partial(ctx, pa, pt, RT_TRUE, NULL);
}

int
ptarray_contains_point_partial(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number)
{
  int wn = 0;
  int i;
  double side;
  const RTPOINT2D *seg1;
  const RTPOINT2D *seg2;
  double ymin, ymax;

  seg1 = rt_getPoint2d_cp(ctx, pa, 0);
  seg2 = rt_getPoint2d_cp(ctx, pa, pa->npoints-1);
  if ( check_closed && ! p2d_same(ctx, seg1, seg2) )
    rterror(ctx, "ptarray_contains_point called on unclosed ring");

  for ( i=1; i < pa->npoints; i++ )
  {
    seg2 = rt_getPoint2d_cp(ctx, pa, i);

    /* Zero length segments are ignored. */
    if ( seg1->x == seg2->x && seg1->y == seg2->y )
    {
      seg1 = seg2;
      continue;
    }

    ymin = FP_MIN(seg1->y, seg2->y);
    ymax = FP_MAX(seg1->y, seg2->y);

    /* Only test segments in our vertical range */
    if ( pt->y > ymax || pt->y < ymin )
    {
      seg1 = seg2;
      continue;
    }

    side = rt_segment_side(ctx, seg1, seg2, pt);

    /*
    * A point on the boundary of a ring is not contained.
    * WAS: if (fabs(side) < 1e-12), see #852
    */
    if ( (side == 0) && rt_pt_in_seg(ctx, pt, seg1, seg2) )
    {
      return RT_BOUNDARY;
    }

    /*
    * If the point is to the left of the line, and it's rising,
    * then the line is to the right of the point and
    * circling counter-clockwise, so incremement.
    */
    if ( (side < 0) && (seg1->y <= pt->y) && (pt->y < seg2->y) )
    {
      wn++;
    }

    /*
    * If the point is to the right of the line, and it's falling,
    * then the line is to the right of the point and circling
    * clockwise, so decrement.
    */
    else if ( (side > 0) && (seg2->y <= pt->y) && (pt->y < seg1->y) )
    {
      wn--;
    }

    seg1 = seg2;
  }

  /* Sent out the winding number for calls that are building on this as a primitive */
  if ( winding_number )
    *winding_number = wn;

  /* Outside */
  if (wn == 0)
  {
    return RT_OUTSIDE;
  }

  /* Inside */
  return RT_INSIDE;
}

/**
* For RTPOINTARRAYs representing CIRCULARSTRINGS. That is, linked triples
* with each triple being control points of a circular arc. Such
* RTPOINTARRAYs have an odd number of vertices.
*
* Return 1 if the point is inside the RTPOINTARRAY, -1 if it is outside,
* and 0 if it is on the boundary.
*/

int
ptarrayarc_contains_point(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt)
{
  return ptarrayarc_contains_point_partial(ctx, pa, pt, RT_TRUE /* Check closed*/, NULL);
}

int
ptarrayarc_contains_point_partial(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number)
{
  int wn = 0;
  int i, side;
  const RTPOINT2D *seg1;
  const RTPOINT2D *seg2;
  const RTPOINT2D *seg3;
  RTGBOX gbox;

  /* Check for not an arc ring (artays have odd # of points) */
  if ( (pa->npoints % 2) == 0 )
  {
    rterror(ctx, "ptarrayarc_contains_point called with even number of points");
    return RT_OUTSIDE;
  }

  /* Check for not an arc ring (artays have >= 3 points) */
  if ( pa->npoints < 3 )
  {
    rterror(ctx, "ptarrayarc_contains_point called too-short pointarray");
    return RT_OUTSIDE;
  }

  /* Check for unclosed case */
  seg1 = rt_getPoint2d_cp(ctx, pa, 0);
  seg3 = rt_getPoint2d_cp(ctx, pa, pa->npoints-1);
  if ( check_closed && ! p2d_same(ctx, seg1, seg3) )
  {
    rterror(ctx, "ptarrayarc_contains_point called on unclosed ring");
    return RT_OUTSIDE;
  }
  /* OK, it's closed. Is it just one circle? */
  else if ( p2d_same(ctx, seg1, seg3) && pa->npoints == 3 )
  {
    double radius, d;
    RTPOINT2D c;
    seg2 = rt_getPoint2d_cp(ctx, pa, 1);

    /* Wait, it's just a point, so it can't contain anything */
    if ( rt_arc_is_pt(ctx, seg1, seg2, seg3) )
      return RT_OUTSIDE;

    /* See if the point is within the circle radius */
    radius = rt_arc_center(ctx, seg1, seg2, seg3, &c);
    d = distance2d_pt_pt(ctx, pt, &c);
    if ( FP_EQUALS(d, radius) )
      return RT_BOUNDARY; /* Boundary of circle */
    else if ( d < radius )
      return RT_INSIDE; /* Inside circle */
    else
      return RT_OUTSIDE; /* Outside circle */
  }
  else if ( p2d_same(ctx, seg1, pt) || p2d_same(ctx, seg3, pt) )
  {
    return RT_BOUNDARY; /* Boundary case */
  }

  /* Start on the ring */
  seg1 = rt_getPoint2d_cp(ctx, pa, 0);
  for ( i=1; i < pa->npoints; i += 2 )
  {
    seg2 = rt_getPoint2d_cp(ctx, pa, i);
    seg3 = rt_getPoint2d_cp(ctx, pa, i+1);

    /* Catch an easy boundary case */
    if( p2d_same(ctx, seg3, pt) )
      return RT_BOUNDARY;

    /* Skip arcs that have no size */
    if ( rt_arc_is_pt(ctx, seg1, seg2, seg3) )
    {
      seg1 = seg3;
      continue;
    }

    /* Only test segments in our vertical range */
    rt_arc_calculate_gbox_cartesian_2d(ctx, seg1, seg2, seg3, &gbox);
    if ( pt->y > gbox.ymax || pt->y < gbox.ymin )
    {
      seg1 = seg3;
      continue;
    }

    /* Outside of horizontal range, and not between end points we also skip */
    if ( (pt->x > gbox.xmax || pt->x < gbox.xmin) &&
       (pt->y > FP_MAX(seg1->y, seg3->y) || pt->y < FP_MIN(seg1->y, seg3->y)) )
    {
      seg1 = seg3;
      continue;
    }

    side = rt_arc_side(ctx, seg1, seg2, seg3, pt);

    /* On the boundary */
    if ( (side == 0) && rt_pt_in_arc(ctx, pt, seg1, seg2, seg3) )
    {
      return RT_BOUNDARY;
    }

    /* Going "up"! Point to left of arc. */
    if ( side < 0 && (seg1->y <= pt->y) && (pt->y < seg3->y) )
    {
      wn++;
    }

    /* Going "down"! */
    if ( side > 0 && (seg2->y <= pt->y) && (pt->y < seg1->y) )
    {
      wn--;
    }

    /* Inside the arc! */
    if ( pt->x <= gbox.xmax && pt->x >= gbox.xmin )
    {
      RTPOINT2D C;
      double radius = rt_arc_center(ctx, seg1, seg2, seg3, &C);
      double d = distance2d_pt_pt(ctx, pt, &C);

      /* On the boundary! */
      if ( d == radius )
        return RT_BOUNDARY;

      /* Within the arc! */
      if ( d  < radius )
      {
        /* Left side, increment winding number */
        if ( side < 0 )
          wn++;
        /* Right side, decrement winding number */
        if ( side > 0 )
          wn--;
      }
    }

    seg1 = seg3;
  }

  /* Sent out the winding number for calls that are building on this as a primitive */
  if ( winding_number )
    *winding_number = wn;

  /* Outside */
  if (wn == 0)
  {
    return RT_OUTSIDE;
  }

  /* Inside */
  return RT_INSIDE;
}

/**
* Returns the area in cartesian units. Area is negative if ring is oriented CCW,
* positive if it is oriented CW and zero if the ring is degenerate or flat.
* http://en.wikipedia.org/wiki/Shoelace_formula
*/
double
ptarray_signed_area(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  const RTPOINT2D *P1;
  const RTPOINT2D *P2;
  const RTPOINT2D *P3;
  double sum = 0.0;
  double x0, x, y1, y2;
  int i;

  if (! pa || pa->npoints < 3 )
    return 0.0;

  P1 = rt_getPoint2d_cp(ctx, pa, 0);
  P2 = rt_getPoint2d_cp(ctx, pa, 1);
  x0 = P1->x;
  for ( i = 1; i < pa->npoints - 1; i++ )
  {
    P3 = rt_getPoint2d_cp(ctx, pa, i+1);
    x = P2->x - x0;
    y1 = P3->y;
    y2 = P1->y;
    sum += x * (y2-y1);

    /* Move forwards! */
    P1 = P2;
    P2 = P3;
  }
  return sum / 2.0;
}

int
ptarray_isccw(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  double area = 0;
  area = ptarray_signed_area(ctx, pa);
  if ( area > 0 ) return RT_FALSE;
  else return RT_TRUE;
}

RTPOINTARRAY*
ptarray_force_dims(const RTCTX *ctx, const RTPOINTARRAY *pa, int hasz, int hasm)
{
  /* TODO handle zero-length point arrays */
  int i;
  int in_hasz = RTFLAGS_GET_Z(pa->flags);
  int in_hasm = RTFLAGS_GET_M(pa->flags);
  RTPOINT4D pt;
  RTPOINTARRAY *pa_out = ptarray_construct_empty(ctx, hasz, hasm, pa->npoints);

  for( i = 0; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &pt);
    if( hasz && ! in_hasz )
      pt.z = 0.0;
    if( hasm && ! in_hasm )
      pt.m = 0.0;
    ptarray_append_point(ctx, pa_out, &pt, RT_TRUE);
  }

  return pa_out;
}

RTPOINTARRAY *
ptarray_substring(const RTCTX *ctx, RTPOINTARRAY *ipa, double from, double to, double tolerance)
{
  RTPOINTARRAY *dpa;
  RTPOINT4D pt;
  RTPOINT4D p1, p2;
  RTPOINT4D *p1ptr=&p1; /* don't break strict-aliasing rule */
  RTPOINT4D *p2ptr=&p2;
  int nsegs, i;
  double length, slength, tlength;
  int state = 0; /* 0=before, 1=inside */

  /*
   * Create a dynamic pointarray with an initial capacity
   * equal to full copy of input points
   */
  dpa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(ipa->flags), RTFLAGS_GET_M(ipa->flags), ipa->npoints);

  /* Compute total line length */
  length = ptarray_length_2d(ctx, ipa);


  RTDEBUGF(ctx, 3, "Total length: %g", length);


  /* Get 'from' and 'to' lengths */
  from = length*from;
  to = length*to;


  RTDEBUGF(ctx, 3, "From/To: %g/%g", from, to);


  tlength = 0;
  rt_getPoint4d_p(ctx, ipa, 0, &p1);
  nsegs = ipa->npoints - 1;
  for ( i = 0; i < nsegs; i++ )
  {
    double dseg;

    rt_getPoint4d_p(ctx, ipa, i+1, &p2);


    RTDEBUGF(ctx, 3 ,"Segment %d: (%g,%g,%g,%g)-(%g,%g,%g,%g)",
             i, p1.x, p1.y, p1.z, p1.m, p2.x, p2.y, p2.z, p2.m);


    /* Find the length of this segment */
    slength = distance2d_pt_pt(ctx, (RTPOINT2D *)p1ptr, (RTPOINT2D *)p2ptr);

    /*
     * We are before requested start.
     */
    if ( state == 0 ) /* before */
    {

      RTDEBUG(ctx, 3, " Before start");

      if ( fabs ( from - ( tlength + slength ) ) <= tolerance )
      {

        RTDEBUG(ctx, 3, "  Second point is our start");

        /*
         * Second point is our start
         */
        ptarray_append_point(ctx, dpa, &p2, RT_FALSE);
        state=1; /* we're inside now */
        goto END;
      }

      else if ( fabs(from - tlength) <= tolerance )
      {

        RTDEBUG(ctx, 3, "  First point is our start");

        /*
         * First point is our start
         */
        ptarray_append_point(ctx, dpa, &p1, RT_FALSE);

        /*
         * We're inside now, but will check
         * 'to' point as well
         */
        state=1;
      }

      /*
       * Didn't reach the 'from' point,
       * nothing to do
       */
      else if ( from > tlength + slength ) goto END;

      else  /* tlength < from < tlength+slength */
      {

        RTDEBUG(ctx, 3, "  Seg contains first point");

        /*
         * Our start is between first and
         * second point
         */
        dseg = (from - tlength) / slength;

        interpolate_point4d(ctx, &p1, &p2, &pt, dseg);

        ptarray_append_point(ctx, dpa, &pt, RT_FALSE);

        /*
         * We're inside now, but will check
         * 'to' point as well
         */
        state=1;
      }
    }

    if ( state == 1 ) /* inside */
    {

      RTDEBUG(ctx, 3, " Inside");

      /*
       * 'to' point is our second point.
       */
      if ( fabs(to - ( tlength + slength ) ) <= tolerance )
      {

        RTDEBUG(ctx, 3, " Second point is our end");

        ptarray_append_point(ctx, dpa, &p2, RT_FALSE);
        break; /* substring complete */
      }

      /*
       * 'to' point is our first point.
       * (should only happen if 'to' is 0)
       */
      else if ( fabs(to - tlength) <= tolerance )
      {

        RTDEBUG(ctx, 3, " First point is our end");

        ptarray_append_point(ctx, dpa, &p1, RT_FALSE);

        break; /* substring complete */
      }

      /*
       * Didn't reach the 'end' point,
       * just copy second point
       */
      else if ( to > tlength + slength )
      {
        ptarray_append_point(ctx, dpa, &p2, RT_FALSE);
        goto END;
      }

      /*
       * 'to' point falls on this segment
       * Interpolate and break.
       */
      else if ( to < tlength + slength )
      {

        RTDEBUG(ctx, 3, " Seg contains our end");

        dseg = (to - tlength) / slength;
        interpolate_point4d(ctx, &p1, &p2, &pt, dseg);

        ptarray_append_point(ctx, dpa, &pt, RT_FALSE);

        break;
      }

      else
      {
        RTDEBUG(ctx, 3, "Unhandled case");
      }
    }


END:

    tlength += slength;
    memcpy(&p1, &p2, sizeof(RTPOINT4D));
  }

  RTDEBUGF(ctx, 3, "Out of loop, ptarray has %d points", dpa->npoints);

  return dpa;
}

/*
 * Write into the *ret argument coordinates of the closes point on
 * the given segment to the reference input point.
 */
void
closest_point_on_segment(const RTCTX *ctx, const RTPOINT4D *p, const RTPOINT4D *A, const RTPOINT4D *B, RTPOINT4D *ret)
{
  double r;

  if (  FP_EQUALS(A->x, B->x) && FP_EQUALS(A->y, B->y) )
  {
    *ret = *A;
    return;
  }

  /*
   * We use comp.graphics.algorithms Frequently Asked Questions method
   *
   * (1)           AC dot AB
   *           r = ----------
   *                ||AB||^2
   *  r has the following meaning:
   *  r=0 P = A
   *  r=1 P = B
   *  r<0 P is on the backward extension of AB
   *  r>1 P is on the forward extension of AB
   *  0<r<1 P is interior to AB
   *
   */
  r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

  if (r<0)
  {
    *ret = *A;
    return;
  }
  if (r>1)
  {
    *ret = *B;
    return;
  }

  ret->x = A->x + ( (B->x - A->x) * r );
  ret->y = A->y + ( (B->y - A->y) * r );
  ret->z = A->z + ( (B->z - A->z) * r );
  ret->m = A->m + ( (B->m - A->m) * r );
}

/*
 * Given a point, returns the location of closest point on pointarray
 * and, optionally, it's actual distance from the point array.
 */
double
ptarray_locate_point(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT4D *p4d, double *mindistout, RTPOINT4D *proj4d)
{
  double mindist=-1;
  double tlen, plen;
  int t, seg=-1;
  RTPOINT4D  start4d, end4d, projtmp;
  RTPOINT2D proj, p;
  const RTPOINT2D *start = NULL, *end = NULL;

  /* Initialize our 2D copy of the input parameter */
  p.x = p4d->x;
  p.y = p4d->y;

  if ( ! proj4d ) proj4d = &projtmp;

  start = rt_getPoint2d_cp(ctx, pa, 0);

  /* If the pointarray has only one point, the nearest point is */
  /* just that point */
  if ( pa->npoints == 1 )
  {
    rt_getPoint4d_p(ctx, pa, 0, proj4d);
    if ( mindistout )
      *mindistout = distance2d_pt_pt(ctx, &p, start);
    return 0.0;
  }

  /* Loop through pointarray looking for nearest segment */
  for (t=1; t<pa->npoints; t++)
  {
    double dist;
    end = rt_getPoint2d_cp(ctx, pa, t);
    dist = distance2d_pt_seg(ctx, &p, start, end);

    if (t==1 || dist < mindist )
    {
      mindist = dist;
      seg=t-1;
    }

    if ( mindist == 0 )
    {
      RTDEBUG(ctx, 3, "Breaking on mindist=0");
      break;
    }

    start = end;
  }

  if ( mindistout ) *mindistout = mindist;

  RTDEBUGF(ctx, 3, "Closest segment: %d", seg);
  RTDEBUGF(ctx, 3, "mindist: %g", mindist);

  /*
   * We need to project the
   * point on the closest segment.
   */
  rt_getPoint4d_p(ctx, pa, seg, &start4d);
  rt_getPoint4d_p(ctx, pa, seg+1, &end4d);
  closest_point_on_segment(ctx, p4d, &start4d, &end4d, proj4d);

  /* Copy 4D values into 2D holder */
  proj.x = proj4d->x;
  proj.y = proj4d->y;

  RTDEBUGF(ctx, 3, "Closest segment:%d, npoints:%d", seg, pa->npoints);

  /* For robustness, force 1 when closest point == endpoint */
  if ( (seg >= (pa->npoints-2)) && p2d_same(ctx, &proj, end) )
  {
    return 1.0;
  }

  RTDEBUGF(ctx, 3, "Closest point on segment: %g,%g", proj.x, proj.y);

  tlen = ptarray_length_2d(ctx, pa);

  RTDEBUGF(ctx, 3, "tlen %g", tlen);

  /* Location of any point on a zero-length line is 0 */
  /* See http://trac.osgeo.org/postgis/ticket/1772#comment:2 */
  if ( tlen == 0 ) return 0;

  plen=0;
  start = rt_getPoint2d_cp(ctx, pa, 0);
  for (t=0; t<seg; t++, start=end)
  {
    end = rt_getPoint2d_cp(ctx, pa, t+1);
    plen += distance2d_pt_pt(ctx, start, end);

    RTDEBUGF(ctx, 4, "Segment %d made plen %g", t, plen);
  }

  plen+=distance2d_pt_pt(ctx, &proj, start);

  RTDEBUGF(ctx, 3, "plen %g, tlen %g", plen, tlen);

  return plen/tlen;
}

/**
 * @brief Longitude shift for a pointarray.
 *    Y remains the same
 *    X is converted:
 *       from -180..180 to 0..360
 *       from 0..360 to -180..180
 *    X < 0 becomes X + 360
 *    X > 180 becomes X - 360
 */
void
ptarray_longitude_shift(const RTCTX *ctx, RTPOINTARRAY *pa)
{
  int i;
  double x;

  for (i=0; i<pa->npoints; i++)
  {
    memcpy(&x, rt_getPoint_internal(ctx, pa, i), sizeof(double));
    if ( x < 0 ) x+= 360;
    else if ( x > 180 ) x -= 360;
    memcpy(rt_getPoint_internal(ctx, pa, i), &x, sizeof(double));
  }
}


/*
 * Returns a RTPOINTARRAY with consecutive equal points
 * removed. Equality test on all dimensions of input.
 *
 * Artays returns a newly allocated object.
 *
 */
RTPOINTARRAY *
ptarray_remove_repeated_points_minpoints(const RTCTX *ctx, const RTPOINTARRAY *in, double tolerance, int minpoints)
{
  RTPOINTARRAY* out;
  size_t ptsize;
  size_t ipn, opn;
  const RTPOINT2D *last_point, *this_point;
  double tolsq = tolerance * tolerance;

  if ( minpoints < 1 ) minpoints = 1;

  RTDEBUGF(ctx, 3, "%s called", __func__);

  /* Single or zero point arrays can't have duplicates */
  if ( in->npoints < 3 ) return ptarray_clone_deep(ctx, in);

  ptsize = ptarray_point_size(ctx, in);

  RTDEBUGF(ctx, 3, " ptsize: %d", ptsize);

  /* Allocate enough space for all points */
  out = ptarray_construct(ctx, RTFLAGS_GET_Z(in->flags),
                          RTFLAGS_GET_M(in->flags), in->npoints);

  /* Now fill up the actual points (NOTE: could be optimized) */

  opn=1;
  memcpy(rt_getPoint_internal(ctx, out, 0), rt_getPoint_internal(ctx, in, 0), ptsize);
  last_point = rt_getPoint2d_cp(ctx, in, 0);
  RTDEBUGF(ctx, 3, " first point copied, out points: %d", opn);
  for ( ipn = 1; ipn < in->npoints; ++ipn)
  {
    this_point = rt_getPoint2d_cp(ctx, in, ipn);
    if ( (ipn >= in->npoints-minpoints+1 && opn < minpoints) ||
         (tolerance == 0 && memcmp(rt_getPoint_internal(ctx, in, ipn-1), rt_getPoint_internal(ctx, in, ipn), ptsize) != 0) ||
         (tolerance > 0.0 && distance2d_sqr_pt_pt(ctx, last_point, this_point) > tolsq) )
    {
      /* The point is different from the previous,
       * we add it to output */
      memcpy(rt_getPoint_internal(ctx, out, opn++), rt_getPoint_internal(ctx, in, ipn), ptsize);
      last_point = this_point;
      RTDEBUGF(ctx, 3, " Point %d differs from point %d. Out points: %d", ipn, ipn-1, opn);
    }
  }

  RTDEBUGF(ctx, 3, " in:%d out:%d", out->npoints, opn);
  out->npoints = opn;

  return out;
}

RTPOINTARRAY *
ptarray_remove_repeated_points(const RTCTX *ctx, const RTPOINTARRAY *in, double tolerance)
{
  return ptarray_remove_repeated_points_minpoints(ctx, in, tolerance, 2);
}

static void
ptarray_dp_findsplit(const RTCTX *ctx, RTPOINTARRAY *pts, int p1, int p2, int *split, double *dist)
{
  int k;
  const RTPOINT2D *pk, *pa, *pb;
  double tmp, d;

  RTDEBUG(ctx, 4, "function called");

  *split = p1;
  d = -1;

  if (p1 + 1 < p2)
  {

    pa = rt_getPoint2d_cp(ctx, pts, p1);
    pb = rt_getPoint2d_cp(ctx, pts, p2);

    RTDEBUGF(ctx, 4, "P%d(%f,%f) to P%d(%f,%f)",
             p1, pa->x, pa->y, p2, pb->x, pb->y);

    for (k=p1+1; k<p2; k++)
    {
      pk = rt_getPoint2d_cp(ctx, pts, k);

      RTDEBUGF(ctx, 4, "P%d(%f,%f)", k, pk->x, pk->y);

      /* distance computation */
      tmp = distance2d_sqr_pt_seg(ctx, pk, pa, pb);

      if (tmp > d)
      {
        d = tmp;  /* record the maximum */
        *split = k;

        RTDEBUGF(ctx, 4, "P%d is farthest (%g)", k, d);
      }
    }
    *dist = d;

  } /* length---should be redone if can == 0 */
  else
  {
    RTDEBUG(ctx, 3, "segment too short, no split/no dist");
    *dist = -1;
  }

}

RTPOINTARRAY *
ptarray_simplify(const RTCTX *ctx, RTPOINTARRAY *inpts, double epsilon, unsigned int minpts)
{
  int *stack;      /* recursion stack */
  int sp=-1;      /* recursion stack pointer */
  int p1, split;
  double dist;
  RTPOINTARRAY *outpts;
  RTPOINT4D pt;

  double eps_sqr = epsilon * epsilon;

  /* Allocate recursion stack */
  stack = rtalloc(ctx, sizeof(int)*inpts->npoints);

  p1 = 0;
  stack[++sp] = inpts->npoints-1;

  RTDEBUGF(ctx, 2, "Input has %d pts and %d dims", inpts->npoints,
                                              RTFLAGS_NDIMS(inpts->flags));

  /* Allocate output RTPOINTARRAY, and add first point. */
  outpts = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(inpts->flags), RTFLAGS_GET_M(inpts->flags), inpts->npoints);
  rt_getPoint4d_p(ctx, inpts, 0, &pt);
  ptarray_append_point(ctx, outpts, &pt, RT_FALSE);

  RTDEBUG(ctx, 3, "Added P0 to simplified point array (size 1)");

  do
  {

    ptarray_dp_findsplit(ctx, inpts, p1, stack[sp], &split, &dist);

    RTDEBUGF(ctx, 3, "Farthest point from P%d-P%d is P%d (dist. %g)", p1, stack[sp], split, dist);

    if (dist > eps_sqr || ( outpts->npoints+sp+1 < minpts && dist >= 0 ) )
    {
      RTDEBUGF(ctx, 4, "Added P%d to stack (outpts:%d)", split, sp);
      stack[++sp] = split;
    }
    else
    {
      rt_getPoint4d_p(ctx, inpts, stack[sp], &pt);
      RTDEBUGF(ctx, 4, "npoints , minpoints %d %d", outpts->npoints, minpts);
      ptarray_append_point(ctx, outpts, &pt, RT_FALSE);

      RTDEBUGF(ctx, 4, "Added P%d to simplified point array (size: %d)", stack[sp], outpts->npoints);

      p1 = stack[sp--];
    }

    RTDEBUGF(ctx, 4, "stack pointer = %d", sp);
  }
  while (! (sp<0) );

  rtfree(ctx, stack);
  return outpts;
}

/**
* Find the 2d length of the given #RTPOINTARRAY, using circular
* arc interpolation between each coordinate triple.
* Length(A1, A2, A3, A4, A5) = Length(A1, A2, A3)+Length(A3, A4, A5)
*/
double
ptarray_arc_length_2d(const RTCTX *ctx, const RTPOINTARRAY *pts)
{
  double dist = 0.0;
  int i;
  const RTPOINT2D *a1;
  const RTPOINT2D *a2;
  const RTPOINT2D *a3;

  if ( pts->npoints % 2 != 1 )
        rterror(ctx, "arc point array with even number of points");

  a1 = rt_getPoint2d_cp(ctx, pts, 0);

  for ( i=2; i < pts->npoints; i += 2 )
  {
      a2 = rt_getPoint2d_cp(ctx, pts, i-1);
    a3 = rt_getPoint2d_cp(ctx, pts, i);
    dist += rt_arc_length(ctx, a1, a2, a3);
    a1 = a3;
  }
  return dist;
}

/**
* Find the 2d length of the given #RTPOINTARRAY (even if it's 3d)
*/
double
ptarray_length_2d(const RTCTX *ctx, const RTPOINTARRAY *pts)
{
  double dist = 0.0;
  int i;
  const RTPOINT2D *frm;
  const RTPOINT2D *to;

  if ( pts->npoints < 2 ) return 0.0;

  frm = rt_getPoint2d_cp(ctx, pts, 0);

  for ( i=1; i < pts->npoints; i++ )
  {
    to = rt_getPoint2d_cp(ctx, pts, i);

    dist += sqrt( ((frm->x - to->x)*(frm->x - to->x))  +
                  ((frm->y - to->y)*(frm->y - to->y)) );

    frm = to;
  }
  return dist;
}

/**
* Find the 3d/2d length of the given #RTPOINTARRAY
* (depending on its dimensionality)
*/
double
ptarray_length(const RTCTX *ctx, const RTPOINTARRAY *pts)
{
  double dist = 0.0;
  int i;
  RTPOINT3DZ frm;
  RTPOINT3DZ to;

  if ( pts->npoints < 2 ) return 0.0;

  /* compute 2d length if 3d is not available */
  if ( ! RTFLAGS_GET_Z(pts->flags) ) return ptarray_length_2d(ctx, pts);

  rt_getPoint3dz_p(ctx, pts, 0, &frm);
  for ( i=1; i < pts->npoints; i++ )
  {
    rt_getPoint3dz_p(ctx, pts, i, &to);
    dist += sqrt( ((frm.x - to.x)*(frm.x - to.x)) +
                  ((frm.y - to.y)*(frm.y - to.y)) +
                  ((frm.z - to.z)*(frm.z - to.z)) );
    frm = to;
  }
  return dist;
}


/*
 * Get a pointer to nth point of a RTPOINTARRAY.
 *
 * Casting to returned pointer to RTPOINT2D* should be safe,
 * as gserialized format artays keeps the RTPOINTARRAY pointer
 * aligned to double boundary.
 */
uint8_t *
rt_getPoint_internal(const RTCTX *ctx, const RTPOINTARRAY *pa, int n)
{
  size_t size;
  uint8_t *ptr;

#if PARANOIA_LEVEL > 0
  if ( pa == NULL )
  {
    rterror(ctx, "rt_getPoint got NULL pointarray");
    return NULL;
  }

  RTDEBUGF(ctx, 5, "(n=%d, pa.npoints=%d, pa.maxpoints=%d)",n,pa->npoints,pa->maxpoints);

  if ( ( n < 0 ) ||
       ( n > pa->npoints ) ||
       ( n >= pa->maxpoints ) )
  {
    rterror(ctx, "rt_getPoint_internal called outside of ptarray range (n=%d, pa.npoints=%d, pa.maxpoints=%d)",n,pa->npoints,pa->maxpoints);
    return NULL; /*error */
  }
#endif

  size = ptarray_point_size(ctx, pa);

  ptr = pa->serialized_pointlist + size * n;
  if ( RTFLAGS_NDIMS(pa->flags) == 2)
  {
    RTDEBUGF(ctx, 5, "point = %g %g", *((double*)(ptr)), *((double*)(ptr+8)));
  }
  else if ( RTFLAGS_NDIMS(pa->flags) == 3)
  {
    RTDEBUGF(ctx, 5, "point = %g %g %g", *((double*)(ptr)), *((double*)(ptr+8)), *((double*)(ptr+16)));
  }
  else if ( RTFLAGS_NDIMS(pa->flags) == 4)
  {
    RTDEBUGF(ctx, 5, "point = %g %g %g %g", *((double*)(ptr)), *((double*)(ptr+8)), *((double*)(ptr+16)), *((double*)(ptr+24)));
  }

  return ptr;
}


/**
 * Affine transform a pointarray.
 */
void
ptarray_affine(const RTCTX *ctx, RTPOINTARRAY *pa, const RTAFFINE *a)
{
  int i;
  double x,y,z;
  RTPOINT4D p4d;

  RTDEBUG(ctx, 2, "rtgeom_affine_ptarray start");

  if ( RTFLAGS_GET_Z(pa->flags) )
  {
    RTDEBUG(ctx, 3, " has z");

    for (i=0; i<pa->npoints; i++)
    {
      rt_getPoint4d_p(ctx, pa, i, &p4d);
      x = p4d.x;
      y = p4d.y;
      z = p4d.z;
      p4d.x = a->afac * x + a->bfac * y + a->cfac * z + a->xoff;
      p4d.y = a->dfac * x + a->efac * y + a->ffac * z + a->yoff;
      p4d.z = a->gfac * x + a->hfac * y + a->ifac * z + a->zoff;
      ptarray_set_point4d(ctx, pa, i, &p4d);

      RTDEBUGF(ctx, 3, " POINT %g %g %g => %g %g %g", x, y, x, p4d.x, p4d.y, p4d.z);
    }
  }
  else
  {
    RTDEBUG(ctx, 3, " doesn't have z");

    for (i=0; i<pa->npoints; i++)
    {
      rt_getPoint4d_p(ctx, pa, i, &p4d);
      x = p4d.x;
      y = p4d.y;
      p4d.x = a->afac * x + a->bfac * y + a->xoff;
      p4d.y = a->dfac * x + a->efac * y + a->yoff;
      ptarray_set_point4d(ctx, pa, i, &p4d);

      RTDEBUGF(ctx, 3, " POINT %g %g %g => %g %g %g", x, y, x, p4d.x, p4d.y, p4d.z);
    }
  }

  RTDEBUG(ctx, 3, "rtgeom_affine_ptarray end");

}

/**
 * Scale a pointarray.
 */
void
ptarray_scale(const RTCTX *ctx, RTPOINTARRAY *pa, const RTPOINT4D *fact)
{
  int i;
  RTPOINT4D p4d;

  RTDEBUG(ctx, 3, "ptarray_scale start");

  for (i=0; i<pa->npoints; ++i)
  {
    rt_getPoint4d_p(ctx, pa, i, &p4d);
    p4d.x *= fact->x;
    p4d.y *= fact->y;
    p4d.z *= fact->z;
    p4d.m *= fact->m;
    ptarray_set_point4d(ctx, pa, i, &p4d);
  }

  RTDEBUG(ctx, 3, "ptarray_scale end");

}

int
ptarray_startpoint(const RTCTX *ctx, const RTPOINTARRAY* pa, RTPOINT4D* pt)
{
  return rt_getPoint4d_p(ctx, pa, 0, pt);
}




/*
 * Stick an array of points to the given gridspec.
 * Return "gridded" points in *outpts and their number in *outptsn.
 *
 * Two consecutive points falling on the same grid cell are collapsed
 * into one single point.
 *
 */
RTPOINTARRAY *
ptarray_grid(const RTCTX *ctx, const RTPOINTARRAY *pa, const gridspec *grid)
{
  RTPOINT4D pt;
  int ipn; /* input point numbers */
  RTPOINTARRAY *dpa;

  RTDEBUGF(ctx, 2, "ptarray_grid called on %p", pa);

  dpa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(pa->flags),RTFLAGS_GET_M(pa->flags), pa->npoints);

  for (ipn=0; ipn<pa->npoints; ++ipn)
  {

    rt_getPoint4d_p(ctx, pa, ipn, &pt);

    if ( grid->xsize )
      pt.x = rint((pt.x - grid->ipx)/grid->xsize) *
               grid->xsize + grid->ipx;

    if ( grid->ysize )
      pt.y = rint((pt.y - grid->ipy)/grid->ysize) *
               grid->ysize + grid->ipy;

    if ( RTFLAGS_GET_Z(pa->flags) && grid->zsize )
      pt.z = rint((pt.z - grid->ipz)/grid->zsize) *
               grid->zsize + grid->ipz;

    if ( RTFLAGS_GET_M(pa->flags) && grid->msize )
      pt.m = rint((pt.m - grid->ipm)/grid->msize) *
               grid->msize + grid->ipm;

    ptarray_append_point(ctx, dpa, &pt, RT_FALSE);

  }

  return dpa;
}

int
ptarray_npoints_in_rect(const RTCTX *ctx, const RTPOINTARRAY *pa, const RTGBOX *gbox)
{
  const RTPOINT2D *pt;
  int n = 0;
  int i;
  for ( i = 0; i < pa->npoints; i++ )
  {
    pt = rt_getPoint2d_cp(ctx, pa, i);
    if ( gbox_contains_point2d(ctx, gbox, pt) )
      n++;
  }
  return n;
}


