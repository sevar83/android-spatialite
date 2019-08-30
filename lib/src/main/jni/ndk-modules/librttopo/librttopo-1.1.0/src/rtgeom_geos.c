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
 * Copyright 2011-2014 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "rttopo_config.h"
#include "rtgeom_geos.h"
#include "librttopo_geom.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

#include <stdlib.h>

RTTIN * rttin_from_geos(const RTCTX *ctx, const GEOSGeometry *geom, int want3d);

#undef RTGEOM_PROFILE_BUILDAREA

const char *
rtgeom_get_last_geos_error(const RTCTX *ctx)
{
  return ctx->rtgeom_geos_errmsg;
}

static void
rtgeom_geos_notice(const char *msg, void *ctx)
{
  rtnotice(ctx, "%s\n", msg);
}

extern void
rtgeom_geos_error(const char *msg, void *ptr)
{
  RTCTX *ctx = (RTCTX *)ptr;

  /* TODO:  write in the context, not in the global ! */

  /* Call the supplied function */
  if ( RTGEOM_GEOS_ERRMSG_MAXSIZE-1 < snprintf(ctx->rtgeom_geos_errmsg, RTGEOM_GEOS_ERRMSG_MAXSIZE-1, "%s", msg) )
  {
    ctx->rtgeom_geos_errmsg[RTGEOM_GEOS_ERRMSG_MAXSIZE-1] = '\0';
  }
}

void
rtgeom_geos_ensure_init(const RTCTX *ctx)
{
  if ( ctx->gctx != NULL ) return;
  GEOSContextHandle_t h = GEOS_init_r();
  ((RTCTX*)ctx)->gctx = h;
  GEOSContext_setNoticeMessageHandler_r(h, rtgeom_geos_notice, (void*)ctx);
  GEOSContext_setErrorMessageHandler_r(h, rtgeom_geos_error, (void*)ctx);
}


/*
**  GEOS <==> PostGIS conversion functions
**
** Default conversion creates a GEOS point array, then iterates through the
** PostGIS points, setting each value in the GEOS array one at a time.
**
*/

/* Return a RTPOINTARRAY from a GEOSCoordSeq */
RTPOINTARRAY *
ptarray_from_GEOSCoordSeq(const RTCTX *ctx, const GEOSCoordSequence *cs, char want3d)
{
  uint32_t dims=2;
  uint32_t size, i;
  RTPOINTARRAY *pa;
  RTPOINT4D point;

  RTDEBUG(ctx, 2, "ptarray_fromGEOSCoordSeq called");

  if ( ! GEOSCoordSeq_getSize_r(ctx->gctx, cs, &size) )
    rterror(ctx, "Exception thrown");

  RTDEBUGF(ctx, 4, " GEOSCoordSeq size: %d", size);

  if ( want3d )
  {
    if ( ! GEOSCoordSeq_getDimensions_r(ctx->gctx, cs, &dims) )
      rterror(ctx, "Exception thrown");

    RTDEBUGF(ctx, 4, " GEOSCoordSeq dimensions: %d", dims);

    /* forget higher dimensions (if any) */
    if ( dims > 3 ) dims = 3;
  }

  RTDEBUGF(ctx, 4, " output dimensions: %d", dims);

  pa = ptarray_construct(ctx, (dims==3), 0, size);

  for (i=0; i<size; i++)
  {
    GEOSCoordSeq_getX_r(ctx->gctx, cs, i, &(point.x));
    GEOSCoordSeq_getY_r(ctx->gctx, cs, i, &(point.y));
    if ( dims >= 3 ) GEOSCoordSeq_getZ_r(ctx->gctx, cs, i, &(point.z));
    ptarray_set_point4d(ctx, pa,i,&point);
  }

  return pa;
}

/* Return an RTGEOM from a Geometry */
RTGEOM *
GEOS2RTGEOM(const RTCTX *ctx, const GEOSGeometry *geom, char want3d)
{
  int type = GEOSGeomTypeId_r(ctx->gctx, geom) ;
  int hasZ;
  int SRID = GEOSGetSRID_r(ctx->gctx, geom);

  /* GEOS's 0 is equivalent to our unknown as for SRID values */
  if ( SRID == 0 ) SRID = SRID_UNKNOWN;

  if ( want3d )
  {
    hasZ = GEOSHasZ_r(ctx->gctx, geom);
    if ( ! hasZ )
    {
      RTDEBUG(ctx, 3, "Geometry has no Z, won't provide one");

      want3d = 0;
    }
  }

/*
  if ( GEOSisEmpty_r(ctx->gctx, geom) )
  {
    return (RTGEOM*)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, SRID, want3d, 0);
  }
*/

  switch (type)
  {
    const GEOSCoordSequence *cs;
    RTPOINTARRAY *pa, **ppaa;
    const GEOSGeometry *g;
    RTGEOM **geoms;
    uint32_t i, ngeoms;

  case GEOS_POINT:
    RTDEBUG(ctx, 4, "rtgeom_from_geometry: it's a Point");
    cs = GEOSGeom_getCoordSeq_r(ctx->gctx, geom);
    if ( GEOSisEmpty_r(ctx->gctx, geom) )
      return (RTGEOM*)rtpoint_construct_empty(ctx, SRID, want3d, 0);
    pa = ptarray_from_GEOSCoordSeq(ctx, cs, want3d);
    return (RTGEOM *)rtpoint_construct(ctx, SRID, NULL, pa);

  case GEOS_LINESTRING:
  case GEOS_LINEARRING:
    RTDEBUG(ctx, 4, "rtgeom_from_geometry: it's a LineString or LinearRing");
    if ( GEOSisEmpty_r(ctx->gctx, geom) )
      return (RTGEOM*)rtline_construct_empty(ctx, SRID, want3d, 0);

    cs = GEOSGeom_getCoordSeq_r(ctx->gctx, geom);
    pa = ptarray_from_GEOSCoordSeq(ctx, cs, want3d);
    return (RTGEOM *)rtline_construct(ctx, SRID, NULL, pa);

  case GEOS_POLYGON:
    RTDEBUG(ctx, 4, "rtgeom_from_geometry: it's a Polygon");
    if ( GEOSisEmpty_r(ctx->gctx, geom) )
      return (RTGEOM*)rtpoly_construct_empty(ctx, SRID, want3d, 0);
    ngeoms = GEOSGetNumInteriorRings_r(ctx->gctx, geom);
    ppaa = rtalloc(ctx, sizeof(RTPOINTARRAY *)*(ngeoms+1));
    g = GEOSGetExteriorRing_r(ctx->gctx, geom);
    cs = GEOSGeom_getCoordSeq_r(ctx->gctx, g);
    ppaa[0] = ptarray_from_GEOSCoordSeq(ctx, cs, want3d);
    for (i=0; i<ngeoms; i++)
    {
      g = GEOSGetInteriorRingN_r(ctx->gctx, geom, i);
      cs = GEOSGeom_getCoordSeq_r(ctx->gctx, g);
      ppaa[i+1] = ptarray_from_GEOSCoordSeq(ctx, cs,
                                            want3d);
    }
    return (RTGEOM *)rtpoly_construct(ctx, SRID, NULL,
                                      ngeoms+1, ppaa);

  case GEOS_MULTIPOINT:
  case GEOS_MULTILINESTRING:
  case GEOS_MULTIPOLYGON:
  case GEOS_GEOMETRYCOLLECTION:
    RTDEBUG(ctx, 4, "rtgeom_from_geometry: it's a Collection or Multi");

    ngeoms = GEOSGetNumGeometries_r(ctx->gctx, geom);
    geoms = NULL;
    if ( ngeoms )
    {
      geoms = rtalloc(ctx, sizeof(RTGEOM *)*ngeoms);
      for (i=0; i<ngeoms; i++)
      {
        g = GEOSGetGeometryN_r(ctx->gctx, geom, i);
        geoms[i] = GEOS2RTGEOM(ctx, g, want3d);
      }
    }
    return (RTGEOM *)rtcollection_construct(ctx, type,
                                            SRID, NULL, ngeoms, geoms);

  default:
    rterror(ctx, "GEOS2RTGEOM: unknown geometry type: %d", type);
    return NULL;

  }

}

static GEOSCoordSeq
ptarray_to_GEOSCoordSeq(const RTCTX *ctx, const RTPOINTARRAY *pa)
{
  uint32_t dims = 2;
  uint32_t i;
  const RTPOINT3DZ *p3d;
  const RTPOINT2D *p2d;
  GEOSCoordSeq sq;

  if ( RTFLAGS_GET_Z(pa->flags) )
    dims = 3;

  if ( ! (sq = GEOSCoordSeq_create_r(ctx->gctx, pa->npoints, dims)) )
  {
    rterror(ctx, "Error creating GEOS Coordinate Sequence");
    return NULL;
  }

  for ( i=0; i < pa->npoints; i++ )
  {
    if ( dims == 3 )
    {
      p3d = rt_getPoint3dz_cp(ctx, pa, i);
      p2d = (const RTPOINT2D *)p3d;
      RTDEBUGF(ctx, 4, "Point: %g,%g,%g", p3d->x, p3d->y, p3d->z);
    }
    else
    {
      p2d = rt_getPoint2d_cp(ctx, pa, i);
      RTDEBUGF(ctx, 4, "Point: %g,%g", p2d->x, p2d->y);
    }

    GEOSCoordSeq_setX_r(ctx->gctx, sq, i, p2d->x);
    GEOSCoordSeq_setY_r(ctx->gctx, sq, i, p2d->y);

    if ( dims == 3 )
      GEOSCoordSeq_setZ_r(ctx->gctx, sq, i, p3d->z);
  }
  return sq;
}

static GEOSGeometry *
ptarray_to_GEOSLinearRing(const RTCTX *ctx, const RTPOINTARRAY *pa, int autofix)
{
  GEOSCoordSeq sq;
  GEOSGeom g;
  RTPOINTARRAY *npa = 0;

  if ( autofix )
  {
    /* check ring for being closed and fix if not */
    if ( ! ptarray_is_closed_2d(ctx, pa) )
    {
      npa = ptarray_addPoint(ctx, pa, rt_getPoint_internal(ctx, pa, 0), RTFLAGS_NDIMS(pa->flags), pa->npoints);
      pa = npa;
    }
    /* TODO: check ring for having at least 4 vertices */
#if 0
    while ( pa->npoints < 4 )
    {
      npa = ptarray_addPoint(ctx, npa, rt_getPoint_internal(ctx, pa, 0), RTFLAGS_NDIMS(pa->flags), pa->npoints);
    }
#endif
  }

  sq = ptarray_to_GEOSCoordSeq(ctx, pa);
  if ( npa ) ptarray_free(ctx, npa);
  g = GEOSGeom_createLinearRing_r(ctx->gctx, sq);
  return g;
}

GEOSGeometry *
GBOX2GEOS(const RTCTX *ctx, const RTGBOX *box)
{
  GEOSGeometry* envelope;
  GEOSGeometry* ring;
  GEOSCoordSequence* seq = GEOSCoordSeq_create_r(ctx->gctx, 5, 2);
  if (!seq)
  {
    return NULL;
  }

  GEOSCoordSeq_setX_r(ctx->gctx, seq, 0, box->xmin);
  GEOSCoordSeq_setY_r(ctx->gctx, seq, 0, box->ymin);

  GEOSCoordSeq_setX_r(ctx->gctx, seq, 1, box->xmax);
  GEOSCoordSeq_setY_r(ctx->gctx, seq, 1, box->ymin);

  GEOSCoordSeq_setX_r(ctx->gctx, seq, 2, box->xmax);
  GEOSCoordSeq_setY_r(ctx->gctx, seq, 2, box->ymax);

  GEOSCoordSeq_setX_r(ctx->gctx, seq, 3, box->xmin);
  GEOSCoordSeq_setY_r(ctx->gctx, seq, 3, box->ymax);

  GEOSCoordSeq_setX_r(ctx->gctx, seq, 4, box->xmin);
  GEOSCoordSeq_setY_r(ctx->gctx, seq, 4, box->ymin);

  ring = GEOSGeom_createLinearRing_r(ctx->gctx, seq);
  if (!ring)
  {
    GEOSCoordSeq_destroy_r(ctx->gctx, seq);
    return NULL;
  }

  envelope = GEOSGeom_createPolygon_r(ctx->gctx, ring, NULL, 0);
  if (!envelope)
  {
    GEOSGeom_destroy_r(ctx->gctx, ring);
    return NULL;
  }

  return envelope;
}

GEOSGeometry *
RTGEOM2GEOS(const RTCTX *ctx, const RTGEOM *rtgeom, int autofix)
{
  GEOSCoordSeq sq;
  GEOSGeom g, shell;
  GEOSGeom *geoms = NULL;
  /*
  RTGEOM *tmp;
  */
  uint32_t ngeoms, i;
  int geostype;
#if RTDEBUG_LEVEL >= 4
  char *wkt;
#endif

  RTDEBUGF(ctx, 4, "RTGEOM2GEOS got a %s", rttype_name(ctx, rtgeom->type));

  if (rtgeom_has_arc(ctx, rtgeom))
  {
    RTGEOM *rtgeom_stroked = rtgeom_stroke(ctx, rtgeom, 32);
    GEOSGeometry *g = RTGEOM2GEOS(ctx, rtgeom_stroked, autofix);
    rtgeom_free(ctx, rtgeom_stroked);
    return g;
  }

  switch (rtgeom->type)
  {
    RTPOINT *rtp = NULL;
    RTPOLY *rtpoly = NULL;
    RTLINE *rtl = NULL;
    RTCOLLECTION *rtc = NULL;

  case RTPOINTTYPE:
    rtp = (RTPOINT *)rtgeom;

    if ( rtgeom_is_empty(ctx, rtgeom) )
    {
      g = GEOSGeom_createEmptyPolygon_r(ctx->gctx);
    }
    else
    {
      sq = ptarray_to_GEOSCoordSeq(ctx, rtp->point);
      g = GEOSGeom_createPoint_r(ctx->gctx, sq);
    }
    if ( ! g )
    {
      /* rtnotice(ctx, "Exception in RTGEOM2GEOS"); */
      return NULL;
    }
    break;
  case RTLINETYPE:
    rtl = (RTLINE *)rtgeom;
    /* TODO: if (autofix) */
    if ( rtl->points->npoints == 1 ) {
      /* Duplicate point, to make geos-friendly */
      rtl->points = ptarray_addPoint(ctx, rtl->points,
                               rt_getPoint_internal(ctx, rtl->points, 0),
                               RTFLAGS_NDIMS(rtl->points->flags),
                               rtl->points->npoints);
    }
    sq = ptarray_to_GEOSCoordSeq(ctx, rtl->points);
    g = GEOSGeom_createLineString_r(ctx->gctx, sq);
    if ( ! g )
    {
      /* rtnotice(ctx, "Exception in RTGEOM2GEOS"); */
      return NULL;
    }
    break;

  case RTPOLYGONTYPE:
    rtpoly = (RTPOLY *)rtgeom;
    if ( rtgeom_is_empty(ctx, rtgeom) )
    {
      g = GEOSGeom_createEmptyPolygon_r(ctx->gctx);
    }
    else
    {
      shell = ptarray_to_GEOSLinearRing(ctx, rtpoly->rings[0], autofix);
      if ( ! shell ) return NULL;
      /*rterror(ctx, "RTGEOM2GEOS: exception during polygon shell conversion"); */
      ngeoms = rtpoly->nrings-1;
      if ( ngeoms > 0 )
        geoms = malloc(sizeof(GEOSGeom)*ngeoms);

      for (i=1; i<rtpoly->nrings; ++i)
      {
        geoms[i-1] = ptarray_to_GEOSLinearRing(ctx, rtpoly->rings[i], autofix);
        if ( ! geoms[i-1] )
        {
          --i;
          while (i) GEOSGeom_destroy_r(ctx->gctx, geoms[--i]);
          free(geoms);
          GEOSGeom_destroy_r(ctx->gctx, shell);
          return NULL;
        }
        /*rterror(ctx, "RTGEOM2GEOS: exception during polygon hole conversion"); */
      }
      g = GEOSGeom_createPolygon_r(ctx->gctx, shell, geoms, ngeoms);
      if (geoms) free(geoms);
    }
    if ( ! g ) return NULL;
    break;
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOLLECTIONTYPE:
    if ( rtgeom->type == RTMULTIPOINTTYPE )
      geostype = GEOS_MULTIPOINT;
    else if ( rtgeom->type == RTMULTILINETYPE )
      geostype = GEOS_MULTILINESTRING;
    else if ( rtgeom->type == RTMULTIPOLYGONTYPE )
      geostype = GEOS_MULTIPOLYGON;
    else
      geostype = GEOS_GEOMETRYCOLLECTION;

    rtc = (RTCOLLECTION *)rtgeom;

    ngeoms = rtc->ngeoms;
    if ( ngeoms > 0 )
      geoms = malloc(sizeof(GEOSGeom)*ngeoms);

    for (i=0; i<ngeoms; ++i)
    {
      GEOSGeometry* g = RTGEOM2GEOS(ctx, rtc->geoms[i], 0);
      if ( ! g )
      {
        while (i) GEOSGeom_destroy_r(ctx->gctx, geoms[--i]);
        free(geoms);
        return NULL;
      }
      geoms[i] = g;
    }
    g = GEOSGeom_createCollection_r(ctx->gctx, geostype, geoms, ngeoms);
    if ( geoms ) free(geoms);
    if ( ! g ) return NULL;
    break;

  default:
    rterror(ctx, "Unknown geometry type: %d - %s", rtgeom->type, rttype_name(ctx, rtgeom->type));
    return NULL;
  }

  GEOSSetSRID_r(ctx->gctx, g, rtgeom->srid);

#if RTDEBUG_LEVEL >= 4
  wkt = GEOSGeomToWKT_r(ctx->gctx, g);
  RTDEBUGF(ctx, 4, "RTGEOM2GEOS: GEOSGeom: %s", wkt);
  free(wkt);
#endif

  return g;
}

const char*
rtgeom_geos_version()
{
  const char *ver = GEOSversion();
  return ver;
}

RTGEOM *
rtgeom_normalize(const RTCTX *ctx, const RTGEOM *geom1)
{
  RTGEOM *result ;
  GEOSGeometry *g1;
  int is3d ;
  int srid ;

  srid = (int)(geom1->srid);
  is3d = RTFLAGS_GET_Z(geom1->flags);

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ;
  }

  if ( -1 == GEOSNormalize_r(ctx->gctx, g1) )
  {
    rterror(ctx, "Error in GEOSNormalize: %s", rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }

  GEOSSetSRID_r(ctx->gctx, g1, srid); /* needed ? */
  result = GEOS2RTGEOM(ctx, g1, is3d);
  GEOSGeom_destroy_r(ctx->gctx, g1);

  if (result == NULL)
  {
    rterror(ctx, "Error performing intersection: GEOS2RTGEOM: %s",
                  rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  return result ;
}

RTGEOM *
rtgeom_intersection(const RTCTX *ctx, const RTGEOM *geom1, const RTGEOM *geom2)
{
  RTGEOM *result ;
  GEOSGeometry *g1, *g2, *g3 ;
  int is3d ;
  int srid ;

  /* A.Intersection(Empty) == Empty */
  if ( rtgeom_is_empty(ctx, geom2) )
    return rtgeom_clone_deep(ctx, geom2);

  /* Empty.Intersection(A) == Empty */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom1);

  /* ensure srids are identical */
  srid = (int)(geom1->srid);
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  RTDEBUG(ctx, 3, "intersection() START");

  g1 = RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ;
  }

  g2 = RTGEOM2GEOS(ctx, geom2, 0);
  if ( 0 == g2 )   /* exception thrown at construction */
  {
    rterror(ctx, "Second argument geometry could not be converted to GEOS.");
    GEOSGeom_destroy_r(ctx->gctx, g1);
    return NULL ;
  }

  RTDEBUG(ctx, 3, " constructed geometrys - calling geos");
  RTDEBUGF(ctx, 3, " g1 = %s", GEOSGeomToWKT_r(ctx->gctx, g1));
  RTDEBUGF(ctx, 3, " g2 = %s", GEOSGeomToWKT_r(ctx->gctx, g2));
  /*RTDEBUGF(ctx, 3, "g2 is valid = %i",GEOSisvalid_r(ctx->gctx, g2)); */
  /*RTDEBUGF(ctx, 3, "g1 is valid = %i",GEOSisvalid_r(ctx->gctx, g1)); */

  g3 = GEOSIntersection_r(ctx->gctx, g1,g2);

  RTDEBUG(ctx, 3, " intersection finished");

  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    rterror(ctx, "Error performing intersection: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3) ) ;

  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  if (result == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "Error performing intersection: GEOS2RTGEOM: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  return result ;
}

RTGEOM *
rtgeom_linemerge(const RTCTX *ctx, const RTGEOM *geom1)
{
  RTGEOM *result ;
  GEOSGeometry *g1, *g3 ;
  int is3d = RTFLAGS_GET_Z(geom1->flags);
  int srid = geom1->srid;

  /* Empty.Linemerge() == Empty */
  if ( rtgeom_is_empty(ctx, geom1) )
    return (RTGEOM*)rtcollection_construct_empty(ctx,  RTCOLLECTIONTYPE, srid, is3d,
                                         rtgeom_has_m(ctx, geom1) );

  rtgeom_geos_ensure_init(ctx);

  RTDEBUG(ctx, 3, "linemerge() START");

  g1 = RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ;
  }

  RTDEBUG(ctx, 3, " constructed geometrys - calling geos");
  RTDEBUGF(ctx, 3, " g1 = %s", GEOSGeomToWKT_r(ctx->gctx, g1));
  /*RTDEBUGF(ctx, 3, "g1 is valid = %i",GEOSisvalid_r(ctx->gctx, g1)); */

  g3 = GEOSLineMerge_r(ctx->gctx, g1);

  RTDEBUG(ctx, 3, " linemerge finished");

  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "Error performing linemerge: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3) ) ;

  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  if (result == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "Error performing linemerge: GEOS2RTGEOM: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  return result ;
}

RTGEOM *
rtgeom_unaryunion(const RTCTX *ctx, const RTGEOM *geom1)
{
  RTGEOM *result ;
  GEOSGeometry *g1, *g3 ;
  int is3d = RTFLAGS_GET_Z(geom1->flags);
  int srid = geom1->srid;

  /* Empty.UnaryUnion() == Empty */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom1);

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ;
  }

  g3 = GEOSUnaryUnion_r(ctx->gctx, g1);

  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "Error performing unaryunion: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3) ) ;

  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  if (result == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "Error performing unaryunion: GEOS2RTGEOM: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  return result ;
}

RTGEOM *
rtgeom_difference(const RTCTX *ctx, const RTGEOM *geom1, const RTGEOM *geom2)
{
  GEOSGeometry *g1, *g2, *g3;
  RTGEOM *result;
  int is3d;
  int srid;

  /* A.Difference(Empty) == A */
  if ( rtgeom_is_empty(ctx, geom2) )
    return rtgeom_clone_deep(ctx, geom1);

  /* Empty.Intersection(A) == Empty */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom1);

  /* ensure srids are identical */
  srid = (int)(geom1->srid);
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = RTGEOM2GEOS(ctx, geom2, 0);
  if ( 0 == g2 )   /* exception thrown at construction */
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "Second argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g3 = GEOSDifference_r(ctx->gctx, g1,g2);

  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    rterror(ctx, "GEOSDifference: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3) ) ;

  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  if (result == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "Error performing difference: GEOS2RTGEOM: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  /* compressType(result); */

  return result;
}

RTGEOM *
rtgeom_symdifference(const RTCTX *ctx, const RTGEOM* geom1, const RTGEOM* geom2)
{
  GEOSGeometry *g1, *g2, *g3;
  RTGEOM *result;
  int is3d;
  int srid;

  /* A.SymDifference(Empty) == A */
  if ( rtgeom_is_empty(ctx, geom2) )
    return rtgeom_clone_deep(ctx, geom1);

  /* Empty.DymDifference(B) == B */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom2);

  /* ensure srids are identical */
  srid = (int)(geom1->srid);
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, geom1, 0);

  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = RTGEOM2GEOS(ctx, geom2, 0);

  if ( 0 == g2 )   /* exception thrown at construction */
  {
    rterror(ctx, "Second argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    GEOSGeom_destroy_r(ctx->gctx, g1);
    return NULL;
  }

  g3 = GEOSSymDifference_r(ctx->gctx, g1,g2);

  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    rterror(ctx, "GEOSSymDifference: %s", rtgeom_get_last_geos_error(ctx));
    return NULL; /*never get here */
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3));

  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  if (result == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "GEOS symdifference_r(ctx->gctx) threw an error (result postgis geometry formation)!");
    return NULL ; /*never get here */
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  return result;
}

RTGEOM*
rtgeom_union(const RTCTX *ctx, const RTGEOM *geom1, const RTGEOM *geom2)
{
  int is3d;
  int srid;
  GEOSGeometry *g1, *g2, *g3;
  RTGEOM *result;

  RTDEBUG(ctx, 2, "in geomunion");

  /* A.Union(empty) == A */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom2);

  /* B.Union(empty) == B */
  if ( rtgeom_is_empty(ctx, geom2) )
    return rtgeom_clone_deep(ctx, geom1);


  /* ensure srids are identical */
  srid = (int)(geom1->srid);
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  g1 = RTGEOM2GEOS(ctx, geom1, 0);

  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = RTGEOM2GEOS(ctx, geom2, 0);

  if ( 0 == g2 )   /* exception thrown at construction */
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    rterror(ctx, "Second argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  RTDEBUGF(ctx, 3, "g1=%s", GEOSGeomToWKT_r(ctx->gctx, g1));
  RTDEBUGF(ctx, 3, "g2=%s", GEOSGeomToWKT_r(ctx->gctx, g2));

  g3 = GEOSUnion_r(ctx->gctx, g1,g2);

  RTDEBUGF(ctx, 3, "g3=%s", GEOSGeomToWKT_r(ctx->gctx, g3));

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);

  if (g3 == NULL)
  {
    rterror(ctx, "GEOSUnion: %s", rtgeom_get_last_geos_error(ctx));
    return NULL; /* never get here */
  }


  GEOSSetSRID_r(ctx->gctx, g3, srid);

  result = GEOS2RTGEOM(ctx, g3, is3d);

  GEOSGeom_destroy_r(ctx->gctx, g3);

  if (result == NULL)
  {
    rterror(ctx, "Error performing union: GEOS2RTGEOM: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL; /*never get here */
  }

  return result;
}

RTGEOM *
rtgeom_clip_by_rect(const RTCTX *ctx, const RTGEOM *geom1, double x0, double y0, double x1, double y1)
{
#if RTGEOM_GEOS_VERSION < 35
  rterror(ctx, "The GEOS version this postgis binary "
          "was compiled against (%d) doesn't support "
          "'GEOSClipByRect' function _r(ctx->gctx, 3.3.5+ required)",
          RTGEOM_GEOS_VERSION);
  return NULL;
#else /* RTGEOM_GEOS_VERSION >= 35 */
  RTGEOM *result ;
  GEOSGeometry *g1, *g3 ;
  int is3d ;

  /* A.Intersection(Empty) == Empty */
  if ( rtgeom_is_empty(ctx, geom1) )
    return rtgeom_clone_deep(ctx, geom1);

  is3d = RTFLAGS_GET_Z(geom1->flags);

  rtgeom_geos_ensure_init(ctx);

  RTDEBUG(ctx, 3, "clip_by_rect() START");

  g1 = RTGEOM2GEOS(ctx, geom1, 1); /* auto-fix structure */
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ;
  }

  RTDEBUG(ctx, 3, " constructed geometrys - calling geos");
  RTDEBUGF(ctx, 3, " g1 = %s", GEOSGeomToWKT_r(ctx->gctx, g1));
  /*RTDEBUGF(ctx, 3, "g1 is valid = %i",GEOSisvalid_r(ctx->gctx, g1)); */

  g3 = GEOSClipByRect_r(ctx->gctx, g1,x0,y0,x1,y1);
  GEOSGeom_destroy_r(ctx->gctx, g1);

  RTDEBUG(ctx, 3, " clip_by_rect finished");

  if (g3 == NULL)
  {
    rtnotice(ctx, "Error performing rectangular clipping: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3) ) ;

  result = GEOS2RTGEOM(ctx, g3, is3d);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  if (result == NULL)
  {
    rterror(ctx, "Error performing intersection: GEOS2RTGEOM: %s", rtgeom_get_last_geos_error(ctx));
    return NULL ; /* never get here */
  }

  result->srid = geom1->srid;

  return result ;
#endif /* RTGEOM_GEOS_VERSION >= 35 */
}


/* ------------ BuildArea stuff ---------------------------------------------------------------------{ */

typedef struct Face_t {
  const GEOSGeometry* geom;
  GEOSGeometry* env;
  double envarea;
  struct Face_t* parent; /* if this face is an hole of another one, or NULL */
} Face;

static Face* newFace(const RTCTX *ctx, const GEOSGeometry* g);
static void delFace(const RTCTX *ctx, Face* f);
static unsigned int countParens(const RTCTX *ctx, const Face* f);
static void findFaceHoles(const RTCTX *ctx, Face** faces, int nfaces);

static Face*
newFace(const RTCTX *ctx, const GEOSGeometry* g)
{
  Face* f = rtalloc(ctx, sizeof(Face));
  f->geom = g;
  f->env = GEOSEnvelope_r(ctx->gctx, f->geom);
  GEOSArea_r(ctx->gctx, f->env, &f->envarea);
  f->parent = NULL;
  /* rtnotice(ctx, "Built Face with area %g and %d holes", f->envarea, GEOSGetNumInteriorRings_r(ctx->gctx, f->geom)); */
  return f;
}

static unsigned int
countParens(const RTCTX *ctx, const Face* f)
{
  unsigned int pcount = 0;
  while ( f->parent ) {
    ++pcount;
    f = f->parent;
  }
  return pcount;
}

/* Destroy the face and release memory associated with it */
static void
delFace(const RTCTX *ctx, Face* f)
{
  GEOSGeom_destroy_r(ctx->gctx, f->env);
  rtfree(ctx, f);
}


static int
compare_by_envarea(const void* g1, const void* g2)
{
  Face* f1 = *(Face**)g1;
  Face* f2 = *(Face**)g2;
  double n1 = f1->envarea;
  double n2 = f2->envarea;

  if ( n1 < n2 ) return 1;
  if ( n1 > n2 ) return -1;
  return 0;
}

/* Find holes of each face */
static void
findFaceHoles(const RTCTX *ctx, Face** faces, int nfaces)
{
  int i, j, h;

  /* We sort by envelope area so that we know holes are only
   * after their shells */
  qsort(faces, nfaces, sizeof(Face*), compare_by_envarea);
  for (i=0; i<nfaces; ++i) {
    Face* f = faces[i];
    int nholes = GEOSGetNumInteriorRings_r(ctx->gctx, f->geom);
    RTDEBUGF(ctx, 2, "Scanning face %d with env area %g and %d holes", i, f->envarea, nholes);
    for (h=0; h<nholes; ++h) {
      const GEOSGeometry *hole = GEOSGetInteriorRingN_r(ctx->gctx, f->geom, h);
      RTDEBUGF(ctx, 2, "Looking for hole %d/%d of face %d among %d other faces", h+1, nholes, i, nfaces-i-1);
      for (j=i+1; j<nfaces; ++j) {
    const GEOSGeometry *f2er;
        Face* f2 = faces[j];
        if ( f2->parent ) continue; /* hole already assigned */
        f2er = GEOSGetExteriorRing_r(ctx->gctx, f2->geom);
        /* TODO: can be optimized as the ring would have the
         *       same vertices, possibly in different order.
         *       maybe comparing number of points could already be
         *       useful.
         */
        if ( GEOSEquals_r(ctx->gctx, f2er, hole) ) {
          RTDEBUGF(ctx, 2, "Hole %d/%d of face %d is face %d", h+1, nholes, i, j);
          f2->parent = f;
          break;
        }
      }
    }
  }
}

static GEOSGeometry*
collectFacesWithEvenAncestors(const RTCTX *ctx, Face** faces, int nfaces)
{
  GEOSGeometry **geoms = rtalloc(ctx, sizeof(GEOSGeometry*)*nfaces);
  GEOSGeometry *ret;
  unsigned int ngeoms = 0;
  int i;

  for (i=0; i<nfaces; ++i) {
    Face *f = faces[i];
    if ( countParens(ctx, f) % 2 ) continue; /* we skip odd parents geoms */
    geoms[ngeoms++] = GEOSGeom_clone_r(ctx->gctx, f->geom);
  }

  ret = GEOSGeom_createCollection_r(ctx->gctx, GEOS_MULTIPOLYGON, geoms, ngeoms);
  rtfree(ctx, geoms);
  return ret;
}

GEOSGeometry*
RTGEOM_GEOS_buildArea(const RTCTX *ctx, const GEOSGeometry* geom_in)
{
  GEOSGeometry *tmp;
  GEOSGeometry *geos_result, *shp;
  GEOSGeometry const *vgeoms[1];
  uint32_t i, ngeoms;
  int srid = GEOSGetSRID_r(ctx->gctx, geom_in);
  Face ** geoms;

  vgeoms[0] = geom_in;
#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Polygonizing");
#endif
  geos_result = GEOSPolygonize_r(ctx->gctx, vgeoms, 1);

  RTDEBUGF(ctx, 3, "GEOSpolygonize returned @ %p", geos_result);

  /* Null return from GEOSpolygonize _r(ctx->gctx, an exception) */
  if ( ! geos_result ) return 0;

  /*
   * We should now have a collection
   */
#if PARANOIA_LEVEL > 0
  if ( GEOSGeometryTypeId_r(ctx->gctx, geos_result) != RTCOLLECTIONTYPE )
  {
    GEOSGeom_destroy_r(ctx->gctx, geos_result);
    rterror(ctx, "Unexpected return from GEOSpolygonize");
    return 0;
  }
#endif

  ngeoms = GEOSGetNumGeometries_r(ctx->gctx, geos_result);
#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Num geometries from polygonizer: %d", ngeoms);
#endif


  RTDEBUGF(ctx, 3, "GEOSpolygonize: ngeoms in polygonize output: %d", ngeoms);
  RTDEBUGF(ctx, 3, "GEOSpolygonize: polygonized:%s",
              rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, geos_result, 0)));

  /*
   * No geometries in collection, early out
   */
  if ( ngeoms == 0 )
  {
    GEOSSetSRID_r(ctx->gctx, geos_result, srid);
    return geos_result;
  }

  /*
   * Return first geometry if we only have one in collection,
   * to avoid the unnecessary Geometry clone below.
   */
  if ( ngeoms == 1 )
  {
    tmp = (GEOSGeometry *)GEOSGetGeometryN_r(ctx->gctx, geos_result, 0);
    if ( ! tmp )
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_result);
      return 0; /* exception */
    }
    shp = GEOSGeom_clone_r(ctx->gctx, tmp);
    GEOSGeom_destroy_r(ctx->gctx, geos_result); /* only safe after the clone above */
    GEOSSetSRID_r(ctx->gctx, shp, srid);
    return shp;
  }

  RTDEBUGF(ctx, 2, "Polygonize returned %d geoms", ngeoms);

  /*
   * Polygonizer returns a polygon for each face in the built topology.
   *
   * This means that for any face with holes we'll have other faces
   * representing each hole. We can imagine a parent-child relationship
   * between these faces.
   *
   * In order to maximize the number of visible rings in output we
   * only use those faces which have an even number of parents.
   *
   * Example:
   *
   *   +---------------+
   *   |     L0        |  L0 has no parents
   *   |  +---------+  |
   *   |  |   L1    |  |  L1 is an hole of L0
   *   |  |  +---+  |  |
   *   |  |  |L2 |  |  |  L2 is an hole of L1 (which is an hole of L0)
   *   |  |  |   |  |  |
   *   |  |  +---+  |  |
   *   |  +---------+  |
   *   |               |
   *   +---------------+
   *
   * See http://trac.osgeo.org/postgis/ticket/1806
   *
   */

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Preparing face structures");
#endif

  /* Prepare face structures for later analysis */
  geoms = rtalloc(ctx, sizeof(Face**)*ngeoms);
  for (i=0; i<ngeoms; ++i)
    geoms[i] = newFace(ctx, GEOSGetGeometryN_r(ctx->gctx, geos_result, i));

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Finding face holes");
#endif

  /* Find faces representing other faces holes */
  findFaceHoles(ctx, geoms, ngeoms);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Colletting even ancestor faces");
#endif

  /* Build a MultiPolygon composed only by faces with an
   * even number of ancestors */
  tmp = collectFacesWithEvenAncestors(ctx, geoms, ngeoms);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Cleaning up");
#endif

  /* Cleanup face structures */
  for (i=0; i<ngeoms; ++i) delFace(ctx, geoms[i]);
  rtfree(ctx, geoms);

  /* Faces referenced memory owned by geos_result.
   * It is safe to destroy geos_result after deleting them. */
  GEOSGeom_destroy_r(ctx->gctx, geos_result);

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Self-unioning");
#endif

  /* Run a single overlay operation to dissolve shared edges */
  shp = GEOSUnionCascaded_r(ctx->gctx, tmp);
  if ( ! shp )
  {
    GEOSGeom_destroy_r(ctx->gctx, tmp);
    return 0; /* exception */
  }

#ifdef RTGEOM_PROFILE_BUILDAREA
  rtnotice(ctx, "Final cleanup");
#endif

  GEOSGeom_destroy_r(ctx->gctx, tmp);

  GEOSSetSRID_r(ctx->gctx, shp, srid);

  return shp;
}

RTGEOM*
rtgeom_buildarea(const RTCTX *ctx, const RTGEOM *geom)
{
  GEOSGeometry* geos_in;
  GEOSGeometry* geos_out;
  RTGEOM* geom_out;
  int SRID = (int)(geom->srid);
  int is3d = RTFLAGS_GET_Z(geom->flags);

  /* Can't build an area from an empty! */
  if ( rtgeom_is_empty(ctx, geom) )
  {
    return (RTGEOM*)rtpoly_construct_empty(ctx, SRID, is3d, 0);
  }

  RTDEBUG(ctx, 3, "buildarea called");

  RTDEBUGF(ctx, 3, "ST_BuildArea got geom @ %p", geom);

  rtgeom_geos_ensure_init(ctx);

  geos_in = RTGEOM2GEOS(ctx, geom, 0);

  if ( 0 == geos_in )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  geos_out = RTGEOM_GEOS_buildArea(ctx, geos_in);
  GEOSGeom_destroy_r(ctx->gctx, geos_in);

  if ( ! geos_out ) /* exception thrown.. */
  {
    rterror(ctx, "RTGEOM_GEOS_buildArea: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* If no geometries are in result collection, return NULL */
  if ( GEOSGetNumGeometries_r(ctx->gctx, geos_out) == 0 )
  {
    GEOSGeom_destroy_r(ctx->gctx, geos_out);
    return NULL;
  }

  geom_out = GEOS2RTGEOM(ctx, geos_out, is3d);
  GEOSGeom_destroy_r(ctx->gctx, geos_out);

#if PARANOIA_LEVEL > 0
  if ( geom_out == NULL )
  {
    rterror(ctx, "serialization error");
    return NULL;
  }

#endif

  return geom_out;
}

int
rtgeom_is_simple(const RTCTX *ctx, const RTGEOM *geom)
{
  GEOSGeometry* geos_in;
  int simple;

  /* Empty is artays simple */
  if ( rtgeom_is_empty(ctx, geom) )
  {
    return 1;
  }

  rtgeom_geos_ensure_init(ctx);

  geos_in = RTGEOM2GEOS(ctx, geom, 0);
  if ( 0 == geos_in )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return -1;
  }
  simple = GEOSisSimple_r(ctx->gctx, geos_in);
  GEOSGeom_destroy_r(ctx->gctx, geos_in);

  if ( simple == 2 ) /* exception thrown */
  {
    rterror(ctx, "rtgeom_is_simple: %s", rtgeom_get_last_geos_error(ctx));
    return -1;
  }

  return simple ? 1 : 0;
}

/* ------------ end of BuildArea stuff ---------------------------------------------------------------------} */

RTGEOM*
rtgeom_geos_noop(const RTCTX *ctx, const RTGEOM* geom_in)
{
  GEOSGeometry *geosgeom;
  RTGEOM* geom_out;

  int is3d = RTFLAGS_GET_Z(geom_in->flags);

  rtgeom_geos_ensure_init(ctx);
  geosgeom = RTGEOM2GEOS(ctx, geom_in, 0);
  if ( ! geosgeom ) {
    rterror(ctx, "Geometry could not be converted to GEOS: %s",
      rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  geom_out = GEOS2RTGEOM(ctx, geosgeom, is3d);
  GEOSGeom_destroy_r(ctx->gctx, geosgeom);
  if ( ! geom_out ) {
    rterror(ctx, "GEOS Geometry could not be converted to RTGEOM: %s",
      rtgeom_get_last_geos_error(ctx));
  }
  return geom_out;

}

RTGEOM*
rtgeom_snap(const RTCTX *ctx, const RTGEOM* geom1, const RTGEOM* geom2, double tolerance)
{
  int srid, is3d;
  GEOSGeometry *g1, *g2, *g3;
  RTGEOM* out;

  srid = geom1->srid;
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  g1 = (GEOSGeometry *)RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = (GEOSGeometry *)RTGEOM2GEOS(ctx, geom2, 0);
  if ( 0 == g2 )   /* exception thrown at construction */
  {
    rterror(ctx, "Second argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    GEOSGeom_destroy_r(ctx->gctx, g1);
    return NULL;
  }

  g3 = GEOSSnap_r(ctx->gctx, g1, g2, tolerance);
  if (g3 == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g1);
    GEOSGeom_destroy_r(ctx->gctx, g2);
    rterror(ctx, "GEOSSnap: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);

  GEOSSetSRID_r(ctx->gctx, g3, srid);
  out = GEOS2RTGEOM(ctx, g3, is3d);
  if (out == NULL)
  {
    GEOSGeom_destroy_r(ctx->gctx, g3);
    rterror(ctx, "GEOSSnap_r(ctx->gctx) threw an error (result RTGEOM geometry formation)!");
    return NULL;
  }
  GEOSGeom_destroy_r(ctx->gctx, g3);

  return out;
}

RTGEOM*
rtgeom_sharedpaths(const RTCTX *ctx, const RTGEOM* geom1, const RTGEOM* geom2)
{
  GEOSGeometry *g1, *g2, *g3;
  RTGEOM *out;
  int is3d, srid;

  srid = geom1->srid;
  error_if_srid_mismatch(ctx, srid, (int)(geom2->srid));

  is3d = (RTFLAGS_GET_Z(geom1->flags) || RTFLAGS_GET_Z(geom2->flags)) ;

  rtgeom_geos_ensure_init(ctx);

  g1 = (GEOSGeometry *)RTGEOM2GEOS(ctx, geom1, 0);
  if ( 0 == g1 )   /* exception thrown at construction */
  {
    rterror(ctx, "First argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g2 = (GEOSGeometry *)RTGEOM2GEOS(ctx, geom2, 0);
  if ( 0 == g2 )   /* exception thrown at construction */
  {
    rterror(ctx, "Second argument geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    GEOSGeom_destroy_r(ctx->gctx, g1);
    return NULL;
  }

  g3 = GEOSSharedPaths_r(ctx->gctx, g1,g2);

  GEOSGeom_destroy_r(ctx->gctx, g1);
  GEOSGeom_destroy_r(ctx->gctx, g2);

  if (g3 == NULL)
  {
    rterror(ctx, "GEOSSharedPaths: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  GEOSSetSRID_r(ctx->gctx, g3, srid);
  out = GEOS2RTGEOM(ctx, g3, is3d);
  GEOSGeom_destroy_r(ctx->gctx, g3);

  if (out == NULL)
  {
    rterror(ctx, "GEOS2RTGEOM threw an error");
    return NULL;
  }

  return out;
}

RTGEOM*
rtgeom_offsetcurve(const RTCTX *ctx, const RTLINE *rtline, double size, int quadsegs, int joinStyle, double mitreLimit)
{
  GEOSGeometry *g1, *g3;
  RTGEOM *rtgeom_result;
  RTGEOM *rtgeom_in = rtline_as_rtgeom(ctx, rtline);

  rtgeom_geos_ensure_init(ctx);

  g1 = (GEOSGeometry *)RTGEOM2GEOS(ctx, rtgeom_in, 0);
  if ( ! g1 )
  {
    rterror(ctx, "rtgeom_offsetcurve: Geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  g3 = GEOSOffsetCurve_r(ctx->gctx, g1, size, quadsegs, joinStyle, mitreLimit);
  /* Don't need input geometry anymore */
  GEOSGeom_destroy_r(ctx->gctx, g1);

  if (g3 == NULL)
  {
    rterror(ctx, "GEOSOffsetCurve: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3));

  GEOSSetSRID_r(ctx->gctx, g3, rtgeom_get_srid(ctx, rtgeom_in));
  rtgeom_result = GEOS2RTGEOM(ctx, g3, rtgeom_has_z(ctx, rtgeom_in));
  GEOSGeom_destroy_r(ctx->gctx, g3);

  if (rtgeom_result == NULL)
  {
    rterror(ctx, "rtgeom_offsetcurve: GEOS2RTGEOM returned null");
    return NULL;
  }

  return rtgeom_result;
}

RTTIN * rttin_from_geos(const RTCTX *ctx, const GEOSGeometry *geom, int want3d) {
  int type = GEOSGeomTypeId_r(ctx->gctx, geom);
  int hasZ;
  int SRID = GEOSGetSRID_r(ctx->gctx, geom);

  /* GEOS's 0 is equivalent to our unknown as for SRID values */
  if ( SRID == 0 ) SRID = SRID_UNKNOWN;

  if ( want3d ) {
    hasZ = GEOSHasZ_r(ctx->gctx, geom);
    if ( ! hasZ ) {
      RTDEBUG(ctx, 3, "Geometry has no Z, won't provide one");
      want3d = 0;
    }
  }

  switch (type) {
    RTTRIANGLE **geoms;
    uint32_t i, ngeoms;
  case GEOS_GEOMETRYCOLLECTION:
    RTDEBUG(ctx, 4, "rtgeom_from_geometry: it's a Collection or Multi");

    ngeoms = GEOSGetNumGeometries_r(ctx->gctx, geom);
    geoms = NULL;
    if ( ngeoms ) {
      geoms = rtalloc(ctx, ngeoms * sizeof *geoms);
      if (!geoms) {
        rterror(ctx, "rttin_from_geos: can't allocate geoms");
        return NULL;
      }
      for (i=0; i<ngeoms; i++) {
        const GEOSGeometry *poly, *ring;
        const GEOSCoordSequence *cs;
        RTPOINTARRAY *pa;

        poly = GEOSGetGeometryN_r(ctx->gctx, geom, i);
        ring = GEOSGetExteriorRing_r(ctx->gctx, poly);
        cs = GEOSGeom_getCoordSeq_r(ctx->gctx, ring);
        pa = ptarray_from_GEOSCoordSeq(ctx, cs, want3d);

        geoms[i] = rttriangle_construct(ctx, SRID, NULL, pa);
      }
    }
    return (RTTIN *)rtcollection_construct(ctx, RTTINTYPE, SRID, NULL, ngeoms, (RTGEOM **)geoms);
  case GEOS_POLYGON:
  case GEOS_MULTIPOINT:
  case GEOS_MULTILINESTRING:
  case GEOS_MULTIPOLYGON:
  case GEOS_LINESTRING:
  case GEOS_LINEARRING:
  case GEOS_POINT:
    rterror(ctx, "rttin_from_geos: invalid geometry type for tin: %d", type);
    break;

  default:
    rterror(ctx, "GEOS2RTGEOM: unknown geometry type: %d", type);
    return NULL;
  }

  /* shouldn't get here */
  return NULL;
}
/*
 * output = 1 for edges, 2 for TIN, 0 for polygons
 */
RTGEOM*
rtgeom_delaunay_triangulation(const RTCTX *ctx, const RTGEOM *rtgeom_in, double tolerance, int output)
{
  GEOSGeometry *g1, *g3;
  RTGEOM *rtgeom_result;

  if (output < 0 || output > 2) {
    rterror(ctx, "rtgeom_delaunay_triangulation: invalid output type specified %d", output);
    return NULL;
  }

  rtgeom_geos_ensure_init(ctx);

  g1 = (GEOSGeometry *)RTGEOM2GEOS(ctx, rtgeom_in, 0);
  if ( ! g1 )
  {
    rterror(ctx, "rtgeom_delaunay_triangulation: Geometry could not be converted to GEOS: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* if output != 1 we want polys */
  g3 = GEOSDelaunayTriangulation_r(ctx->gctx, g1, tolerance, output == 1);

  /* Don't need input geometry anymore */
  GEOSGeom_destroy_r(ctx->gctx, g1);

  if (g3 == NULL)
  {
    rterror(ctx, "GEOSDelaunayTriangulation: %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* RTDEBUGF(ctx, 3, "result: %s", GEOSGeomToWKT_r(ctx->gctx, g3)); */

  GEOSSetSRID_r(ctx->gctx, g3, rtgeom_get_srid(ctx, rtgeom_in));

  if (output == 2) {
    rtgeom_result = (RTGEOM *)rttin_from_geos(ctx, g3, rtgeom_has_z(ctx, rtgeom_in));
  } else {
    rtgeom_result = GEOS2RTGEOM(ctx, g3, rtgeom_has_z(ctx, rtgeom_in));
  }

  GEOSGeom_destroy_r(ctx->gctx, g3);

  if (rtgeom_result == NULL) {
    if (output != 2) {
      rterror(ctx, "rtgeom_delaunay_triangulation: GEOS2RTGEOM returned null");
    } else {
      rterror(ctx, "rtgeom_delaunay_triangulation: rttin_from_geos returned null");
    }
    return NULL;
  }

  return rtgeom_result;
}
