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
 * Copyright 2011-2015 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************
 *
 * Last port: lwgeom_geos_split.c r14594
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "rtgeom_geos.h"
#include "librttopo_geom_internal.h"

#include <string.h>
#include <assert.h>

static RTGEOM* rtline_split_by_line(const RTCTX *ctx, const RTLINE* rtgeom_in, const RTGEOM* blade_in);
static RTGEOM* rtline_split_by_point(const RTCTX *ctx, const RTLINE* rtgeom_in, const RTPOINT* blade_in);
static RTGEOM* rtline_split_by_mpoint(const RTCTX *ctx, const RTLINE* rtgeom_in, const RTMPOINT* blade_in);
static RTGEOM* rtline_split(const RTCTX *ctx, const RTLINE* rtgeom_in, const RTGEOM* blade_in);
static RTGEOM* rtpoly_split_by_line(const RTCTX *ctx, const RTPOLY* rtgeom_in, const RTLINE* blade_in);
static RTGEOM* rtcollection_split(const RTCTX *ctx, const RTCOLLECTION* rtcoll_in, const RTGEOM* blade_in);
static RTGEOM* rtpoly_split(const RTCTX *ctx, const RTPOLY* rtpoly_in, const RTGEOM* blade_in);

/* Initializes and uses GEOS internally */
static RTGEOM*
rtline_split_by_line(const RTCTX *ctx, const RTLINE* rtline_in, const RTGEOM* blade_in)
{
  RTGEOM** components;
  RTGEOM* diff;
  RTCOLLECTION* out;
  GEOSGeometry* gdiff; /* difference */
  GEOSGeometry* g1;
  GEOSGeometry* g2;
  int ret;

  /* ASSERT blade_in is LINE or MULTILINE */
  assert (blade_in->type == RTLINETYPE ||
          blade_in->type == RTMULTILINETYPE ||
          blade_in->type == RTPOLYGONTYPE ||
          blade_in->type == RTMULTIPOLYGONTYPE );

  /* Possible outcomes:
   *
   *  1. The lines do not cross or overlap
   *      -> Return a collection with single element
   *  2. The lines cross
   *      -> Return a collection of all elements resulting from the split
   */

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, (RTGEOM*)rtline_in, 0);
  if ( ! g1 )
  {
    rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  g2 = RTGEOM2GEOS(ctx, blade_in, 0);
  if ( ! g2 )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* If blade is a polygon, pick its boundary */
  if ( blade_in->type == RTPOLYGONTYPE || blade_in->type == RTMULTIPOLYGONTYPE )
  {
    gdiff = GEOSBoundary_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    if ( ! gdiff )
    {
      GEOSGeom_destroy_r(ctx->gctx, g1);
      rterror(ctx, "GEOSBoundary: %s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    g2 = gdiff; gdiff = NULL;
  }

  /* If interior intersecton is linear we can't split */
  ret = GEOSRelatePattern_r(ctx->gctx, g1, g2, "1********");
  if ( 2 == ret )
  {
    rterror(ctx, "GEOSRelatePattern: %s", rtgeom_get_last_geos_error(ctx));
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    return NULL;
  }
  if ( ret )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    rterror(ctx, "Splitter line has linear intersection with input");
    return NULL;
  }


  gdiff = GEOSDifference_r(ctx->gctx, g1,g2);
  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);
  if (gdiff == NULL)
  {
    rterror(ctx, "GEOSDifference: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  diff = GEOS2RTGEOM(ctx, gdiff, RTFLAGS_GET_Z(rtline_in->flags));
  GEOSGeom_destroy_r(ctx->gctx, gdiff);
  if (NULL == diff)
  {
    rterror(ctx, "GEOS2RTGEOM: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  out = rtgeom_as_rtcollection(ctx, diff);
  if ( ! out )
  {
    components = rtalloc(ctx, sizeof(RTGEOM*)*1);
    components[0] = diff;
    out = rtcollection_construct(ctx, RTCOLLECTIONTYPE, rtline_in->srid,
                                 NULL, 1, components);
  }
  else
  {
    /* Set SRID */
    rtgeom_set_srid(ctx, (RTGEOM*)out, rtline_in->srid);
    /* Force collection type */
    out->type = RTCOLLECTIONTYPE;
  }


  return (RTGEOM*)out;
}

static RTGEOM*
rtline_split_by_point(const RTCTX *ctx, const RTLINE* rtline_in, const RTPOINT* blade_in)
{
  RTMLINE* out;

  out = rtmline_construct_empty(ctx, rtline_in->srid,
    RTFLAGS_GET_Z(rtline_in->flags),
    RTFLAGS_GET_M(rtline_in->flags));
  if ( rtline_split_by_point_to(ctx, rtline_in, blade_in, out) < 2 )
  {
    rtmline_add_rtline(ctx, out, rtline_clone_deep(ctx, rtline_in));
  }

  /* Turn multiline into collection */
  out->type = RTCOLLECTIONTYPE;

  return (RTGEOM*)out;
}

static RTGEOM*
rtline_split_by_mpoint(const RTCTX *ctx, const RTLINE* rtline_in, const RTMPOINT* mp)
{
  RTMLINE* out;
  int i, j;

  out = rtmline_construct_empty(ctx, rtline_in->srid,
          RTFLAGS_GET_Z(rtline_in->flags),
          RTFLAGS_GET_M(rtline_in->flags));
  rtmline_add_rtline(ctx, out, rtline_clone_deep(ctx, rtline_in));

  for (i=0; i<mp->ngeoms; ++i)
  {
    for (j=0; j<out->ngeoms; ++j)
    {
      rtline_in = out->geoms[j];
      RTPOINT *blade_in = mp->geoms[i];
      int ret = rtline_split_by_point_to(ctx, rtline_in, blade_in, out);
      if ( 2 == ret )
      {
        /* the point splits this line,
         * 2 splits were added to collection.
         * We'll move the latest added into
         * the slot of the current one.
         */
        rtline_free(ctx, out->geoms[j]);
        out->geoms[j] = out->geoms[--out->ngeoms];
      }
    }
  }

  /* Turn multiline into collection */
  out->type = RTCOLLECTIONTYPE;

  return (RTGEOM*)out;
}

int
rtline_split_by_point_to(const RTCTX *ctx, const RTLINE* rtline_in, const RTPOINT* blade_in,
                         RTMLINE* v)
{
  double mindist = -1;
  RTPOINT4D pt, pt_projected;
  RTPOINT4D p1, p2;
  RTPOINTARRAY *ipa = rtline_in->points;
  RTPOINTARRAY* pa1;
  RTPOINTARRAY* pa2;
  int i, nsegs, seg = -1;

  /* Possible outcomes:
   *
   *  1. The point is not on the line or on the boundary
   *      -> Leave collection untouched, return 0
   *  2. The point is on the boundary
   *      -> Leave collection untouched, return 1
   *  3. The point is in the line
   *      -> Push 2 elements on the collection:
   *         o start_point - cut_point
   *         o cut_point - last_point
   *      -> Return 2
   */

  rt_getPoint4d_p(ctx, blade_in->point, 0, &pt);

  /* Find closest segment */
  rt_getPoint4d_p(ctx, ipa, 0, &p1);
  nsegs = ipa->npoints - 1;
  for ( i = 0; i < nsegs; i++ )
  {
    rt_getPoint4d_p(ctx, ipa, i+1, &p2);
    double dist;
    dist = distance2d_pt_seg(ctx, (RTPOINT2D*)&pt, (RTPOINT2D*)&p1, (RTPOINT2D*)&p2);
    RTDEBUGF(ctx, 4, " Distance of point %g %g to segment %g %g, %g %g: %g", pt.x, pt.y, p1.x, p1.y, p2.x, p2.y, dist);
    if (i==0 || dist < mindist )
    {
      mindist = dist;
      seg=i;
      if ( mindist == 0.0 ) break; /* can't be closer than ON line */
    }
    p1 = p2;
  }

  RTDEBUGF(ctx, 3, "Closest segment: %d", seg);
  RTDEBUGF(ctx, 3, "mindist: %g", mindist);

  /* No intersection */
  if ( mindist > 0 ) return 0;

  /* empty or single-point line, intersection on boundary */
  if ( seg < 0 ) return 1;

  /*
   * We need to project the
   * point on the closest segment,
   * to interpolate Z and M if needed
   */
  rt_getPoint4d_p(ctx, ipa, seg, &p1);
  rt_getPoint4d_p(ctx, ipa, seg+1, &p2);
  closest_point_on_segment(ctx, &pt, &p1, &p2, &pt_projected);
  /* But X and Y we want the ones of the input point,
   * as on some architectures the interpolation math moves the
   * coordinates (see postgis#3422)
   */
  pt_projected.x = pt.x;
  pt_projected.y = pt.y;

  RTDEBUGF(ctx, 3, "Projected point:(%g %g), seg:%d, p1:(%g %g), p2:(%g %g)", pt_projected.x, pt_projected.y, seg, p1.x, p1.y, p2.x, p2.y);

  /* When closest point == an endpoint, this is a boundary intersection */
  if ( ( (seg == nsegs-1) && p4d_same(ctx, &pt_projected, &p2) ) ||
       ( (seg == 0)       && p4d_same(ctx, &pt_projected, &p1) ) )
  {
    return 1;
  }

  /* This is an internal intersection, let's build the two new pointarrays */

  pa1 = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(ipa->flags), RTFLAGS_GET_M(ipa->flags), seg+2);
  /* TODO: replace with a memcpy ? */
  for (i=0; i<=seg; ++i)
  {
    rt_getPoint4d_p(ctx, ipa, i, &p1);
    ptarray_append_point(ctx, pa1, &p1, RT_FALSE);
  }
  ptarray_append_point(ctx, pa1, &pt_projected, RT_FALSE);

  pa2 = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(ipa->flags), RTFLAGS_GET_M(ipa->flags), ipa->npoints-seg);
  ptarray_append_point(ctx, pa2, &pt_projected, RT_FALSE);
  /* TODO: replace with a memcpy (if so need to check for duplicated point) ? */
  for (i=seg+1; i<ipa->npoints; ++i)
  {
    rt_getPoint4d_p(ctx, ipa, i, &p1);
    ptarray_append_point(ctx, pa2, &p1, RT_FALSE);
  }

  /* NOTE: I've seen empty pointarrays with loc != 0 and loc != 1 */
  if ( pa1->npoints == 0 || pa2->npoints == 0 ) {
    ptarray_free(ctx, pa1);
    ptarray_free(ctx, pa2);
    /* Intersection is on the boundary */
    return 1;
  }

  rtmline_add_rtline(ctx, v, rtline_construct(ctx, SRID_UNKNOWN, NULL, pa1));
  rtmline_add_rtline(ctx, v, rtline_construct(ctx, SRID_UNKNOWN, NULL, pa2));
  return 2;
}

static RTGEOM*
rtline_split(const RTCTX *ctx, const RTLINE* rtline_in, const RTGEOM* blade_in)
{
  switch (blade_in->type)
  {
  case RTPOINTTYPE:
    return rtline_split_by_point(ctx, rtline_in, (RTPOINT*)blade_in);
  case RTMULTIPOINTTYPE:
    return rtline_split_by_mpoint(ctx, rtline_in, (RTMPOINT*)blade_in);

  case RTLINETYPE:
  case RTMULTILINETYPE:
  case RTPOLYGONTYPE:
  case RTMULTIPOLYGONTYPE:
    return rtline_split_by_line(ctx, rtline_in, blade_in);

  default:
    rterror(ctx, "Splitting a Line by a %s is unsupported",
            rttype_name(ctx, blade_in->type));
    return NULL;
  }
  return NULL;
}

/* Initializes and uses GEOS internally */
static RTGEOM*
rtpoly_split_by_line(const RTCTX *ctx, const RTPOLY* rtpoly_in, const RTLINE* blade_in)
{
  RTCOLLECTION* out;
  GEOSGeometry* g1;
  GEOSGeometry* g2;
  GEOSGeometry* g1_bounds;
  GEOSGeometry* polygons;
  const GEOSGeometry *vgeoms[1];
  int i,n;
  int hasZ = RTFLAGS_GET_Z(rtpoly_in->flags);


  /* Possible outcomes:
   *
   *  1. The line does not split the polygon
   *      -> Return a collection with single element
   *  2. The line does split the polygon
   *      -> Return a collection of all elements resulting from the split
   */

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, (RTGEOM*)rtpoly_in, 0);
  if ( NULL == g1 )
  {
    rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  g1_bounds = GEOSBoundary_r(ctx->gctx, g1);
  if ( NULL == g1_bounds )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "GEOSBoundary: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = RTGEOM2GEOS(ctx, (RTGEOM*)blade_in, 0);
  if ( NULL == g2 )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
    rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  vgeoms[0] = GEOSUnion_r(ctx->gctx, g1_bounds, g2);
  if ( NULL == vgeoms[0] )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
    rterror(ctx, "GEOSUnion: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* debugging..
    rtnotice(ctx, "Bounds poly: %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, g1_bounds, hasZ)));
    rtnotice(ctx, "Line: %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, g2, hasZ)));

    rtnotice(ctx, "Noded bounds: %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, vgeoms[0], hasZ)));
  */

  polygons = GEOSPolygonize_r(ctx->gctx, vgeoms, 1);
  if ( NULL == polygons )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
    GEOSGeom_destroy_r(ctx->gctx, (GEOSGeometry*)vgeoms[0]);
    rterror(ctx, "GEOSPolygonize: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

#if PARANOIA_LEVEL > 0
  if ( GEOSGeometryTypeId_r(ctx->gctx, polygons) != RTCOLLECTIONTYPE )
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
    GEOSGeom_destroy_r(ctx->gctx, (GEOSGeometry*)vgeoms[0]);
    GEOSGeom_destroy_r(ctx->gctx, polygons);
    rterror(ctx, "Unexpected return from GEOSpolygonize");
    return 0;
  }
#endif

  /* We should now have all polygons, just skip
   * the ones which are in holes of the original
   * geometries and return the rest in a collection
   */
  n = GEOSGetNumGeometries_r(ctx->gctx, polygons);
  out = rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, rtpoly_in->srid,
             hasZ, 0);
  /* Allocate space for all polys */
  out->geoms = rtrealloc(ctx, out->geoms, sizeof(RTGEOM*)*n);
  assert(0 == out->ngeoms);
  for (i=0; i<n; ++i)
  {
    GEOSGeometry* pos; /* point on surface */
    const GEOSGeometry* p = GEOSGetGeometryN_r(ctx->gctx, polygons, i);
    int contains;

    pos = GEOSPointOnSurface_r(ctx->gctx, p);
    if ( ! pos )
    {
      GEOSGeom_destroy_r(ctx->gctx, g1);
      GEOSGeom_destroy_r(ctx->gctx, g2);
      GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
      GEOSGeom_destroy_r(ctx->gctx, (GEOSGeometry*)vgeoms[0]);
      GEOSGeom_destroy_r(ctx->gctx, polygons);
      rterror(ctx, "GEOSPointOnSurface: %s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    contains = GEOSContains_r(ctx->gctx, g1, pos);
    if ( 2 == contains )
    {
      GEOSGeom_destroy_r(ctx->gctx, g1);
      GEOSGeom_destroy_r(ctx->gctx, g2);
      GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
      GEOSGeom_destroy_r(ctx->gctx, (GEOSGeometry*)vgeoms[0]);
      GEOSGeom_destroy_r(ctx->gctx, polygons);
      GEOSGeom_destroy_r(ctx->gctx, pos);
      rterror(ctx, "GEOSContains: %s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    GEOSGeom_destroy_r(ctx->gctx, pos);

    if ( 0 == contains )
    {
      /* Original geometry doesn't contain
       * a point in this ring, must be an hole
       */
      continue;
    }

    out->geoms[out->ngeoms++] = GEOS2RTGEOM(ctx, p, hasZ);
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);
  GEOSGeom_destroy_r(ctx->gctx, g1_bounds);
  GEOSGeom_destroy_r(ctx->gctx, (GEOSGeometry*)vgeoms[0]);
  GEOSGeom_destroy_r(ctx->gctx, polygons);

  return (RTGEOM*)out;
}

static RTGEOM*
rtcollection_split(const RTCTX *ctx, const RTCOLLECTION* rtcoll_in, const RTGEOM* blade_in)
{
  RTGEOM** split_vector=NULL;
  RTCOLLECTION* out;
  size_t split_vector_capacity;
  size_t split_vector_size=0;
  size_t i,j;

  split_vector_capacity=8;
  split_vector = rtalloc(ctx, split_vector_capacity * sizeof(RTGEOM*));
  if ( ! split_vector )
  {
    rterror(ctx, "Out of virtual memory");
    return NULL;
  }

  for (i=0; i<rtcoll_in->ngeoms; ++i)
  {
    RTCOLLECTION* col;
    RTGEOM* split = rtgeom_split(ctx, rtcoll_in->geoms[i], blade_in);
    /* an exception should prevent this from ever returning NULL */
    if ( ! split ) return NULL;

    col = rtgeom_as_rtcollection(ctx, split);
    /* Output, if any, will artays be a collection */
    assert(col);

    /* Reallocate split_vector if needed */
    if ( split_vector_size + col->ngeoms > split_vector_capacity )
    {
      /* NOTE: we could be smarter on reallocations here */
      split_vector_capacity += col->ngeoms;
      split_vector = rtrealloc(ctx, split_vector,
                               split_vector_capacity * sizeof(RTGEOM*));
      if ( ! split_vector )
      {
        rterror(ctx, "Out of virtual memory");
        return NULL;
      }
    }

    for (j=0; j<col->ngeoms; ++j)
    {
      col->geoms[j]->srid = SRID_UNKNOWN; /* strip srid */
      split_vector[split_vector_size++] = col->geoms[j];
    }
    rtfree(ctx, col->geoms);
    rtfree(ctx, col);
  }

  /* Now split_vector has split_vector_size geometries */
  out = rtcollection_construct(ctx, RTCOLLECTIONTYPE, rtcoll_in->srid,
                               NULL, split_vector_size, split_vector);

  return (RTGEOM*)out;
}

static RTGEOM*
rtpoly_split(const RTCTX *ctx, const RTPOLY* rtpoly_in, const RTGEOM* blade_in)
{
  switch (blade_in->type)
  {
  case RTLINETYPE:
    return rtpoly_split_by_line(ctx, rtpoly_in, (RTLINE*)blade_in);
  default:
    rterror(ctx, "Splitting a Polygon by a %s is unsupported",
            rttype_name(ctx, blade_in->type));
    return NULL;
  }
  return NULL;
}

/* exported */
RTGEOM*
rtgeom_split(const RTCTX *ctx, const RTGEOM* rtgeom_in, const RTGEOM* blade_in)
{
  switch (rtgeom_in->type)
  {
  case RTLINETYPE:
    return rtline_split(ctx, (const RTLINE*)rtgeom_in, blade_in);

  case RTPOLYGONTYPE:
    return rtpoly_split(ctx, (const RTPOLY*)rtgeom_in, blade_in);

  case RTMULTIPOLYGONTYPE:
  case RTMULTILINETYPE:
  case RTCOLLECTIONTYPE:
    return rtcollection_split(ctx, (const RTCOLLECTION*)rtgeom_in, blade_in);

  default:
    rterror(ctx, "Splitting of %s geometries is unsupported",
            rttype_name(ctx, rtgeom_in->type));
    return NULL;
  }

}

