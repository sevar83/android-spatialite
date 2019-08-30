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
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#if !HAVE_ISFINITE
#endif

#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"
#include <stdlib.h>
#include <math.h>

RTGBOX* gbox_new(const RTCTX *ctx, uint8_t flags)
{
  RTGBOX *g = (RTGBOX*)rtalloc(ctx, sizeof(RTGBOX));
  gbox_init(ctx, g);
  g->flags = flags;
  return g;
}

void gbox_init(const RTCTX *ctx, RTGBOX *gbox)
{
  memset(gbox, 0, sizeof(RTGBOX));
}

RTGBOX* gbox_clone(const RTCTX *ctx, const RTGBOX *gbox)
{
  RTGBOX *g = rtalloc(ctx, sizeof(RTGBOX));
  memcpy(g, gbox, sizeof(RTGBOX));
  return g;
}

/* TODO to be removed */
BOX3D* box3d_from_gbox(const RTCTX *ctx, const RTGBOX *gbox)
{
  BOX3D *b;
  assert(gbox);

  b = rtalloc(ctx, sizeof(BOX3D));

  b->xmin = gbox->xmin;
  b->xmax = gbox->xmax;
  b->ymin = gbox->ymin;
  b->ymax = gbox->ymax;

  if ( RTFLAGS_GET_Z(gbox->flags) )
  {
    b->zmin = gbox->zmin;
    b->zmax = gbox->zmax;
  }
  else
  {
    b->zmin = b->zmax = 0.0;
  }

  b->srid = SRID_UNKNOWN;
   return b;
}

/* TODO to be removed */
RTGBOX* box3d_to_gbox(const RTCTX *ctx, const BOX3D *b3d)
{
  RTGBOX *b;
  assert(b3d);

  b = rtalloc(ctx, sizeof(RTGBOX));

  b->xmin = b3d->xmin;
  b->xmax = b3d->xmax;
  b->ymin = b3d->ymin;
  b->ymax = b3d->ymax;
  b->zmin = b3d->zmin;
  b->zmax = b3d->zmax;

   return b;
}

void gbox_expand(const RTCTX *ctx, RTGBOX *g, double d)
{
  g->xmin -= d;
  g->xmax += d;
  g->ymin -= d;
  g->ymax += d;
  if ( RTFLAGS_GET_Z(g->flags) )
  {
    g->zmin -= d;
    g->zmax += d;
  }
  if ( RTFLAGS_GET_M(g->flags) )
  {
    g->mmin -= d;
    g->mmax += d;
  }
}

int gbox_union(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2, RTGBOX *gout)
{
  if ( ( ! g1 ) && ( ! g2 ) )
    return RT_FALSE;

  if  ( ! g1 )
  {
    memcpy(gout, g2, sizeof(RTGBOX));
    return RT_TRUE;
  }
  if ( ! g2 )
  {
    memcpy(gout, g1, sizeof(RTGBOX));
    return RT_TRUE;
  }

  gout->flags = g1->flags;

  gout->xmin = FP_MIN(g1->xmin, g2->xmin);
  gout->xmax = FP_MAX(g1->xmax, g2->xmax);

  gout->ymin = FP_MIN(g1->ymin, g2->ymin);
  gout->ymax = FP_MAX(g1->ymax, g2->ymax);

  gout->zmin = FP_MIN(g1->zmin, g2->zmin);
  gout->zmax = FP_MAX(g1->zmax, g2->zmax);

  return RT_TRUE;
}

int gbox_same(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{
  if (RTFLAGS_GET_ZM(g1->flags) != RTFLAGS_GET_ZM(g2->flags))
    return RT_FALSE;

  if (!gbox_same_2d(ctx, g1, g2)) return RT_FALSE;

  if (RTFLAGS_GET_Z(g1->flags) && (g1->zmin != g2->zmin || g1->zmax != g2->zmax))
    return RT_FALSE;
  if (RTFLAGS_GET_M(g1->flags) && (g1->mmin != g2->mmin || g1->mmax != g2->mmax))
    return RT_FALSE;

  return RT_TRUE;
}

int gbox_same_2d(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{
    if (g1->xmin == g2->xmin && g1->ymin == g2->ymin &&
        g1->xmax == g2->xmax && g1->ymax == g2->ymax)
    return RT_TRUE;
  return RT_FALSE;
}

int gbox_same_2d_float(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{
  if  ((g1->xmax == g2->xmax || next_float_up(ctx, g1->xmax)   == next_float_up(ctx, g2->xmax))   &&
       (g1->ymax == g2->ymax || next_float_up(ctx, g1->ymax)   == next_float_up(ctx, g2->ymax))   &&
       (g1->xmin == g2->xmin || next_float_down(ctx, g1->xmin) == next_float_down(ctx, g1->xmin)) &&
       (g1->ymin == g2->ymin || next_float_down(ctx, g2->ymin) == next_float_down(ctx, g2->ymin)))
      return RT_TRUE;
  return RT_FALSE;
}

int gbox_is_valid(const RTCTX *ctx, const RTGBOX *gbox)
{
  /* X */
  if ( ! isfinite(gbox->xmin) || isnan(gbox->xmin) ||
       ! isfinite(gbox->xmax) || isnan(gbox->xmax) )
    return RT_FALSE;

  /* Y */
  if ( ! isfinite(gbox->ymin) || isnan(gbox->ymin) ||
       ! isfinite(gbox->ymax) || isnan(gbox->ymax) )
    return RT_FALSE;

  /* Z */
  if ( RTFLAGS_GET_GEODETIC(gbox->flags) || RTFLAGS_GET_Z(gbox->flags) )
  {
    if ( ! isfinite(gbox->zmin) || isnan(gbox->zmin) ||
         ! isfinite(gbox->zmax) || isnan(gbox->zmax) )
      return RT_FALSE;
  }

  /* M */
  if ( RTFLAGS_GET_M(gbox->flags) )
  {
    if ( ! isfinite(gbox->mmin) || isnan(gbox->mmin) ||
         ! isfinite(gbox->mmax) || isnan(gbox->mmax) )
      return RT_FALSE;
  }

  return RT_TRUE;
}

int gbox_merge_point3d(const RTCTX *ctx, const POINT3D *p, RTGBOX *gbox)
{
  if ( gbox->xmin > p->x ) gbox->xmin = p->x;
  if ( gbox->ymin > p->y ) gbox->ymin = p->y;
  if ( gbox->zmin > p->z ) gbox->zmin = p->z;
  if ( gbox->xmax < p->x ) gbox->xmax = p->x;
  if ( gbox->ymax < p->y ) gbox->ymax = p->y;
  if ( gbox->zmax < p->z ) gbox->zmax = p->z;
  return RT_SUCCESS;
}

int gbox_init_point3d(const RTCTX *ctx, const POINT3D *p, RTGBOX *gbox)
{
  gbox->xmin = gbox->xmax = p->x;
  gbox->ymin = gbox->ymax = p->y;
  gbox->zmin = gbox->zmax = p->z;
  return RT_SUCCESS;
}

int gbox_contains_point3d(const RTCTX *ctx, const RTGBOX *gbox, const POINT3D *pt)
{
  if ( gbox->xmin > pt->x || gbox->ymin > pt->y || gbox->zmin > pt->z ||
          gbox->xmax < pt->x || gbox->ymax < pt->y || gbox->zmax < pt->z )
  {
    return RT_FALSE;
  }
  return RT_TRUE;
}

int gbox_merge(const RTCTX *ctx, const RTGBOX *new_box, RTGBOX *merge_box)
{
  assert(merge_box);

  if ( RTFLAGS_GET_ZM(merge_box->flags) != RTFLAGS_GET_ZM(new_box->flags) )
    return RT_FAILURE;

  if ( new_box->xmin < merge_box->xmin) merge_box->xmin = new_box->xmin;
  if ( new_box->ymin < merge_box->ymin) merge_box->ymin = new_box->ymin;
  if ( new_box->xmax > merge_box->xmax) merge_box->xmax = new_box->xmax;
  if ( new_box->ymax > merge_box->ymax) merge_box->ymax = new_box->ymax;

  if ( RTFLAGS_GET_Z(merge_box->flags) || RTFLAGS_GET_GEODETIC(merge_box->flags) )
  {
    if ( new_box->zmin < merge_box->zmin) merge_box->zmin = new_box->zmin;
    if ( new_box->zmax > merge_box->zmax) merge_box->zmax = new_box->zmax;
  }
  if ( RTFLAGS_GET_M(merge_box->flags) )
  {
    if ( new_box->mmin < merge_box->mmin) merge_box->mmin = new_box->mmin;
    if ( new_box->mmax > merge_box->mmax) merge_box->mmax = new_box->mmax;
  }

  return RT_SUCCESS;
}

int gbox_overlaps(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{

  /* Make sure our boxes are consistent */
  if ( RTFLAGS_GET_GEODETIC(g1->flags) != RTFLAGS_GET_GEODETIC(g2->flags) )
    rterror(ctx, "gbox_overlaps: cannot compare geodetic and non-geodetic boxes");

  /* Check X/Y first */
  if ( g1->xmax < g2->xmin || g1->ymax < g2->ymin ||
       g1->xmin > g2->xmax || g1->ymin > g2->ymax )
    return RT_FALSE;

  /* Deal with the geodetic case special: we only compare the geodetic boxes (x/y/z) */
  /* Never the M dimension */
  if ( RTFLAGS_GET_GEODETIC(g1->flags) && RTFLAGS_GET_GEODETIC(g2->flags) )
  {
    if ( g1->zmax < g2->zmin || g1->zmin > g2->zmax )
      return RT_FALSE;
    else
      return RT_TRUE;
  }

  /* If both geodetic or both have Z, check Z */
  if ( RTFLAGS_GET_Z(g1->flags) && RTFLAGS_GET_Z(g2->flags) )
  {
    if ( g1->zmax < g2->zmin || g1->zmin > g2->zmax )
      return RT_FALSE;
  }

  /* If both have M, check M */
  if ( RTFLAGS_GET_M(g1->flags) && RTFLAGS_GET_M(g2->flags) )
  {
    if ( g1->mmax < g2->mmin || g1->mmin > g2->mmax )
      return RT_FALSE;
  }

  return RT_TRUE;
}

int
gbox_overlaps_2d(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{

  /* Make sure our boxes are consistent */
  if ( RTFLAGS_GET_GEODETIC(g1->flags) != RTFLAGS_GET_GEODETIC(g2->flags) )
    rterror(ctx, "gbox_overlaps: cannot compare geodetic and non-geodetic boxes");

  /* Check X/Y first */
  if ( g1->xmax < g2->xmin || g1->ymax < g2->ymin ||
       g1->xmin > g2->xmax || g1->ymin > g2->ymax )
    return RT_FALSE;

  return RT_TRUE;
}

int
gbox_contains_2d(const RTCTX *ctx, const RTGBOX *g1, const RTGBOX *g2)
{
  if ( ( g2->xmin < g1->xmin ) || ( g2->xmax > g1->xmax ) ||
       ( g2->ymin < g1->ymin ) || ( g2->ymax > g1->ymax ) )
  {
    return RT_FALSE;
  }
  return RT_TRUE;
}

int
gbox_contains_point2d(const RTCTX *ctx, const RTGBOX *g, const RTPOINT2D *p)
{
  if ( ( g->xmin <= p->x ) && ( g->xmax >= p->x ) &&
       ( g->ymin <= p->y ) && ( g->ymax >= p->y ) )
  {
    return RT_TRUE;
  }
  return RT_FALSE;
}

/**
* Warning, this function is only good for x/y/z boxes, used
* in unit testing of geodetic box generation.
*/
RTGBOX* gbox_from_string(const RTCTX *ctx, const char *str)
{
  const char *ptr = str;
  char *nextptr;
  char *gbox_start = strstr(str, "RTGBOX((");
  RTGBOX *gbox = gbox_new(ctx, gflags(ctx, 0,0,1));
  if ( ! gbox_start ) return NULL; /* No header found */
  ptr += 6;
  gbox->xmin = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  ptr = nextptr + 1;
  gbox->ymin = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  ptr = nextptr + 1;
  gbox->zmin = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  ptr = nextptr + 3;
  gbox->xmax = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  ptr = nextptr + 1;
  gbox->ymax = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  ptr = nextptr + 1;
  gbox->zmax = strtod(ptr, &nextptr);
  if ( ptr == nextptr ) return NULL; /* No double found */
  return gbox;
}

char* gbox_to_string(const RTCTX *ctx, const RTGBOX *gbox)
{
  static int sz = 128;
  char *str = NULL;

  if ( ! gbox )
    return strdup("NULL POINTER");

  str = (char*)rtalloc(ctx, sz);

  if ( RTFLAGS_GET_GEODETIC(gbox->flags) )
  {
    snprintf(str, sz, "RTGBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->xmax, gbox->ymax, gbox->zmax);
    return str;
  }
  if ( RTFLAGS_GET_Z(gbox->flags) && RTFLAGS_GET_M(gbox->flags) )
  {
    snprintf(str, sz, "RTGBOX((%.8g,%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->mmin, gbox->xmax, gbox->ymax, gbox->zmax, gbox->mmax);
    return str;
  }
  if ( RTFLAGS_GET_Z(gbox->flags) )
  {
    snprintf(str, sz, "RTGBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->zmin, gbox->xmax, gbox->ymax, gbox->zmax);
    return str;
  }
  if ( RTFLAGS_GET_M(gbox->flags) )
  {
    snprintf(str, sz, "RTGBOX((%.8g,%.8g,%.8g),(%.8g,%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->mmin, gbox->xmax, gbox->ymax, gbox->mmax);
    return str;
  }
  snprintf(str, sz, "RTGBOX((%.8g,%.8g),(%.8g,%.8g))", gbox->xmin, gbox->ymin, gbox->xmax, gbox->ymax);
  return str;
}

RTGBOX* gbox_copy(const RTCTX *ctx, const RTGBOX *box)
{
  RTGBOX *copy = (RTGBOX*)rtalloc(ctx, sizeof(RTGBOX));
  memcpy(copy, box, sizeof(RTGBOX));
  return copy;
}

void gbox_duplicate(const RTCTX *ctx, const RTGBOX *original, RTGBOX *duplicate)
{
  assert(duplicate);
  memcpy(duplicate, original, sizeof(RTGBOX));
}

size_t gbox_serialized_size(const RTCTX *ctx, uint8_t flags)
{
  if ( RTFLAGS_GET_GEODETIC(flags) )
    return 6 * sizeof(float);
  else
    return 2 * RTFLAGS_NDIMS(flags) * sizeof(float);
}


/* ********************************************************************************
** Compute cartesian bounding RTGBOX boxes from RTGEOM.
*/

int rt_arc_calculate_gbox_cartesian_2d(const RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, RTGBOX *gbox)
{
  RTPOINT2D xmin, ymin, xmax, ymax;
  RTPOINT2D C;
  int A2_side;
  double radius_A;

  RTDEBUG(ctx, 2, "rt_arc_calculate_gbox_cartesian_2d called.");

  radius_A = rt_arc_center(ctx, A1, A2, A3, &C);

  /* Negative radius signals straight line, p1/p2/p3 are colinear */
  if (radius_A < 0.0)
  {
        gbox->xmin = FP_MIN(A1->x, A3->x);
        gbox->ymin = FP_MIN(A1->y, A3->y);
        gbox->xmax = FP_MAX(A1->x, A3->x);
        gbox->ymax = FP_MAX(A1->y, A3->y);
      return RT_SUCCESS;
  }

  /* Matched start/end points imply circle */
  if ( A1->x == A3->x && A1->y == A3->y )
  {
    gbox->xmin = C.x - radius_A;
    gbox->ymin = C.y - radius_A;
    gbox->xmax = C.x + radius_A;
    gbox->ymax = C.y + radius_A;
    return RT_SUCCESS;
  }

  /* First approximation, bounds of start/end points */
    gbox->xmin = FP_MIN(A1->x, A3->x);
    gbox->ymin = FP_MIN(A1->y, A3->y);
    gbox->xmax = FP_MAX(A1->x, A3->x);
    gbox->ymax = FP_MAX(A1->y, A3->y);

  /* Create points for the possible extrema */
  xmin.x = C.x - radius_A;
  xmin.y = C.y;
  ymin.x = C.x;
  ymin.y = C.y - radius_A;
  xmax.x = C.x + radius_A;
  xmax.y = C.y;
  ymax.x = C.x;
  ymax.y = C.y + radius_A;

  /* Divide the circle into two parts, one on each side of a line
     joining p1 and p3. The circle extrema on the same side of that line
     as p2 is on, are also the extrema of the bbox. */

  A2_side = rt_segment_side(ctx, A1, A3, A2);

  if ( A2_side == rt_segment_side(ctx, A1, A3, &xmin) )
    gbox->xmin = xmin.x;

  if ( A2_side == rt_segment_side(ctx, A1, A3, &ymin) )
    gbox->ymin = ymin.y;

  if ( A2_side == rt_segment_side(ctx, A1, A3, &xmax) )
    gbox->xmax = xmax.x;

  if ( A2_side == rt_segment_side(ctx, A1, A3, &ymax) )
    gbox->ymax = ymax.y;

  return RT_SUCCESS;
}


static int rt_arc_calculate_gbox_cartesian(const RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, const RTPOINT4D *p3, RTGBOX *gbox)
{
  int rv;

  RTDEBUG(ctx, 2, "rt_arc_calculate_gbox_cartesian called.");

  rv = rt_arc_calculate_gbox_cartesian_2d(ctx, (RTPOINT2D*)p1, (RTPOINT2D*)p2, (RTPOINT2D*)p3, gbox);
    gbox->zmin = FP_MIN(p1->z, p3->z);
    gbox->mmin = FP_MIN(p1->m, p3->m);
    gbox->zmax = FP_MAX(p1->z, p3->z);
    gbox->mmax = FP_MAX(p1->m, p3->m);
  return rv;
}

int ptarray_calculate_gbox_cartesian(const RTCTX *ctx, const RTPOINTARRAY *pa, RTGBOX *gbox )
{
  int i;
  RTPOINT4D p;
  int has_z, has_m;

  if ( ! pa ) return RT_FAILURE;
  if ( ! gbox ) return RT_FAILURE;
  if ( pa->npoints < 1 ) return RT_FAILURE;

  has_z = RTFLAGS_GET_Z(pa->flags);
  has_m = RTFLAGS_GET_M(pa->flags);
  gbox->flags = gflags(ctx, has_z, has_m, 0);
  RTDEBUGF(ctx, 4, "ptarray_calculate_gbox Z: %d M: %d", has_z, has_m);

  rt_getPoint4d_p(ctx, pa, 0, &p);
  gbox->xmin = gbox->xmax = p.x;
  gbox->ymin = gbox->ymax = p.y;
  if ( has_z )
    gbox->zmin = gbox->zmax = p.z;
  if ( has_m )
    gbox->mmin = gbox->mmax = p.m;

  for ( i = 1 ; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &p);
    gbox->xmin = FP_MIN(gbox->xmin, p.x);
    gbox->xmax = FP_MAX(gbox->xmax, p.x);
    gbox->ymin = FP_MIN(gbox->ymin, p.y);
    gbox->ymax = FP_MAX(gbox->ymax, p.y);
    if ( has_z )
    {
      gbox->zmin = FP_MIN(gbox->zmin, p.z);
      gbox->zmax = FP_MAX(gbox->zmax, p.z);
    }
    if ( has_m )
    {
      gbox->mmin = FP_MIN(gbox->mmin, p.m);
      gbox->mmax = FP_MAX(gbox->mmax, p.m);
    }
  }
  return RT_SUCCESS;
}

static int rtcircstring_calculate_gbox_cartesian(const RTCTX *ctx, RTCIRCSTRING *curve, RTGBOX *gbox)
{
  uint8_t flags = gflags(ctx, RTFLAGS_GET_Z(curve->flags), RTFLAGS_GET_M(curve->flags), 0);
  RTGBOX tmp;
  RTPOINT4D p1, p2, p3;
  int i;

  if ( ! curve ) return RT_FAILURE;
  if ( curve->points->npoints < 3 ) return RT_FAILURE;

  tmp.flags = flags;

  /* Initialize */
  gbox->xmin = gbox->ymin = gbox->zmin = gbox->mmin = FLT_MAX;
  gbox->xmax = gbox->ymax = gbox->zmax = gbox->mmax = -1*FLT_MAX;

  for ( i = 2; i < curve->points->npoints; i += 2 )
  {
    rt_getPoint4d_p(ctx, curve->points, i-2, &p1);
    rt_getPoint4d_p(ctx, curve->points, i-1, &p2);
    rt_getPoint4d_p(ctx, curve->points, i, &p3);

    if (rt_arc_calculate_gbox_cartesian(ctx, &p1, &p2, &p3, &tmp) == RT_FAILURE)
      continue;

    gbox_merge(ctx, &tmp, gbox);
  }

  return RT_SUCCESS;
}

static int rtpoint_calculate_gbox_cartesian(const RTCTX *ctx, RTPOINT *point, RTGBOX *gbox)
{
  if ( ! point ) return RT_FAILURE;
  return ptarray_calculate_gbox_cartesian(ctx,  point->point, gbox );
}

static int rtline_calculate_gbox_cartesian(const RTCTX *ctx, RTLINE *line, RTGBOX *gbox)
{
  if ( ! line ) return RT_FAILURE;
  return ptarray_calculate_gbox_cartesian(ctx,  line->points, gbox );
}

static int rttriangle_calculate_gbox_cartesian(const RTCTX *ctx, RTTRIANGLE *triangle, RTGBOX *gbox)
{
  if ( ! triangle ) return RT_FAILURE;
  return ptarray_calculate_gbox_cartesian(ctx,  triangle->points, gbox );
}

static int rtpoly_calculate_gbox_cartesian(const RTCTX *ctx, RTPOLY *poly, RTGBOX *gbox)
{
  if ( ! poly ) return RT_FAILURE;
  if ( poly->nrings == 0 ) return RT_FAILURE;
  /* Just need to check outer ring */
  return ptarray_calculate_gbox_cartesian(ctx,  poly->rings[0], gbox );
}

static int rtcollection_calculate_gbox_cartesian(const RTCTX *ctx, RTCOLLECTION *coll, RTGBOX *gbox)
{
  RTGBOX subbox;
  int i;
  int result = RT_FAILURE;
  int first = RT_TRUE;
  assert(coll);
  if ( (coll->ngeoms == 0) || !gbox)
    return RT_FAILURE;

  subbox.flags = coll->flags;

  for ( i = 0; i < coll->ngeoms; i++ )
  {
    if ( rtgeom_calculate_gbox_cartesian(ctx, (RTGEOM*)(coll->geoms[i]), &subbox) == RT_SUCCESS )
    {
      /* Keep a copy of the sub-bounding box for later
      if ( coll->geoms[i]->bbox )
        rtfree(ctx, coll->geoms[i]->bbox);
      coll->geoms[i]->bbox = gbox_copy(ctx, &subbox); */
      if ( first )
      {
        gbox_duplicate(ctx, &subbox, gbox);
        first = RT_FALSE;
      }
      else
      {
        gbox_merge(ctx, &subbox, gbox);
      }
      result = RT_SUCCESS;
    }
  }
  return result;
}

int rtgeom_calculate_gbox_cartesian(const RTCTX *ctx, const RTGEOM *rtgeom, RTGBOX *gbox)
{
  if ( ! rtgeom ) return RT_FAILURE;
  RTDEBUGF(ctx, 4, "rtgeom_calculate_gbox got type (%d) - %s", rtgeom->type, rttype_name(ctx, rtgeom->type));

  switch (rtgeom->type)
  {
  case RTPOINTTYPE:
    return rtpoint_calculate_gbox_cartesian(ctx, (RTPOINT *)rtgeom, gbox);
  case RTLINETYPE:
    return rtline_calculate_gbox_cartesian(ctx, (RTLINE *)rtgeom, gbox);
  case RTCIRCSTRINGTYPE:
    return rtcircstring_calculate_gbox_cartesian(ctx, (RTCIRCSTRING *)rtgeom, gbox);
  case RTPOLYGONTYPE:
    return rtpoly_calculate_gbox_cartesian(ctx, (RTPOLY *)rtgeom, gbox);
  case RTTRIANGLETYPE:
    return rttriangle_calculate_gbox_cartesian(ctx, (RTTRIANGLE *)rtgeom, gbox);
  case RTCOMPOUNDTYPE:
  case RTCURVEPOLYTYPE:
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTICURVETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTMULTISURFACETYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    return rtcollection_calculate_gbox_cartesian(ctx, (RTCOLLECTION *)rtgeom, gbox);
  }
  /* Never get here, please. */
  rterror(ctx, "unsupported type (%d) - %s", rtgeom->type, rttype_name(ctx, rtgeom->type));
  return RT_FAILURE;
}

void gbox_float_round(const RTCTX *ctx, RTGBOX *gbox)
{
  gbox->xmin = next_float_down(ctx, gbox->xmin);
  gbox->xmax = next_float_up(ctx, gbox->xmax);

  gbox->ymin = next_float_down(ctx, gbox->ymin);
  gbox->ymax = next_float_up(ctx, gbox->ymax);

  if ( RTFLAGS_GET_M(gbox->flags) )
  {
    gbox->mmin = next_float_down(ctx, gbox->mmin);
    gbox->mmax = next_float_up(ctx, gbox->mmax);
  }

  if ( RTFLAGS_GET_Z(gbox->flags) )
  {
    gbox->zmin = next_float_down(ctx, gbox->zmin);
    gbox->zmax = next_float_up(ctx, gbox->zmax);
  }
}

