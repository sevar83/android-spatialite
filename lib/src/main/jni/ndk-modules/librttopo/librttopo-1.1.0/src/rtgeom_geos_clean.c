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
 * Copyright 2009-2010 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/

#include "rttopo_config.h"

#include "librttopo_geom.h"
#include "rtgeom_geos.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* #define RTGEOM_DEBUG_LEVEL 4 */
/* #define PARANOIA_LEVEL 2 */
#undef RTGEOM_PROFILE_MAKEVALID


/*
 * Return Nth vertex in GEOSGeometry as a POINT.
 * May return NULL if the geometry has NO vertexex.
 */
static GEOSGeometry*
RTGEOM_GEOS_getPointN(const RTCTX *ctx, const GEOSGeometry* g_in, uint32_t n)
{
  uint32_t dims;
  const GEOSCoordSequence* seq_in;
  GEOSCoordSeq seq_out;
  double val;
  uint32_t sz;
  int gn;
  GEOSGeometry* ret;

  switch ( GEOSGeomTypeId_r(ctx->gctx, g_in) )
  {
  case GEOS_MULTIPOINT:
  case GEOS_MULTILINESTRING:
  case GEOS_MULTIPOLYGON:
  case GEOS_GEOMETRYCOLLECTION:
  {
    for (gn=0; gn<GEOSGetNumGeometries_r(ctx->gctx, g_in); ++gn)
    {
      const GEOSGeometry* g = GEOSGetGeometryN_r(ctx->gctx, g_in, gn);
      ret = RTGEOM_GEOS_getPointN(ctx, g,n);
      if ( ret ) return ret;
    }
    break;
  }

  case GEOS_POLYGON:
  {
    ret = RTGEOM_GEOS_getPointN(ctx, GEOSGetExteriorRing_r(ctx->gctx, g_in), n);
    if ( ret ) return ret;
    for (gn=0; gn<GEOSGetNumInteriorRings_r(ctx->gctx, g_in); ++gn)
    {
      const GEOSGeometry* g = GEOSGetInteriorRingN_r(ctx->gctx, g_in, gn);
      ret = RTGEOM_GEOS_getPointN(ctx, g, n);
      if ( ret ) return ret;
    }
    break;
  }

  case GEOS_POINT:
  case GEOS_LINESTRING:
  case GEOS_LINEARRING:
    break;

  }

  seq_in = GEOSGeom_getCoordSeq_r(ctx->gctx, g_in);
  if ( ! seq_in ) return NULL;
  if ( ! GEOSCoordSeq_getSize_r(ctx->gctx, seq_in, &sz) ) return NULL;
  if ( ! sz ) return NULL;

  if ( ! GEOSCoordSeq_getDimensions_r(ctx->gctx, seq_in, &dims) ) return NULL;

  seq_out = GEOSCoordSeq_create_r(ctx->gctx, 1, dims);
  if ( ! seq_out ) return NULL;

  if ( ! GEOSCoordSeq_getX_r(ctx->gctx, seq_in, n, &val) ) return NULL;
  if ( ! GEOSCoordSeq_setX_r(ctx->gctx, seq_out, n, val) ) return NULL;
  if ( ! GEOSCoordSeq_getY_r(ctx->gctx, seq_in, n, &val) ) return NULL;
  if ( ! GEOSCoordSeq_setY_r(ctx->gctx, seq_out, n, val) ) return NULL;
  if ( dims > 2 )
  {
    if ( ! GEOSCoordSeq_getZ_r(ctx->gctx, seq_in, n, &val) ) return NULL;
    if ( ! GEOSCoordSeq_setZ_r(ctx->gctx, seq_out, n, val) ) return NULL;
  }

  return GEOSGeom_createPoint_r(ctx->gctx, seq_out);
}



RTGEOM * rtcollection_make_geos_friendly(const RTCTX *ctx, RTCOLLECTION *g);
RTGEOM * rtline_make_geos_friendly(const RTCTX *ctx, RTLINE *line);
RTGEOM * rtpoly_make_geos_friendly(const RTCTX *ctx, RTPOLY *poly);
RTPOINTARRAY* ring_make_geos_friendly(const RTCTX *ctx, RTPOINTARRAY* ring);

/*
 * Ensure the geometry is "structurally" valid
 * (enough for GEOS to accept it)
 * May return the input untouched (if already valid).
 * May return geometries of lower dimension (on collapses)
 */
static RTGEOM *
rtgeom_make_geos_friendly(const RTCTX *ctx, RTGEOM *geom)
{
  RTDEBUGF(ctx, 2, "rtgeom_make_geos_friendly enter (type %d)", geom->type);
  switch (geom->type)
  {
  case RTPOINTTYPE:
  case RTMULTIPOINTTYPE:
    /* a point is artays valid */
    return geom;
    break;

  case RTLINETYPE:
    /* lines need at least 2 points */
    return rtline_make_geos_friendly(ctx, (RTLINE *)geom);
    break;

  case RTPOLYGONTYPE:
    /* polygons need all rings closed and with npoints > 3 */
    return rtpoly_make_geos_friendly(ctx, (RTPOLY *)geom);
    break;

  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOLLECTIONTYPE:
    return rtcollection_make_geos_friendly(ctx, (RTCOLLECTION *)geom);
    break;

  case RTCIRCSTRINGTYPE:
  case RTCOMPOUNDTYPE:
  case RTCURVEPOLYTYPE:
  case RTMULTISURFACETYPE:
  case RTMULTICURVETYPE:
  default:
    rterror(ctx, "rtgeom_make_geos_friendly: unsupported input geometry type: %s (%d)", rttype_name(ctx, geom->type), geom->type);
    break;
  }
  return 0;
}

/*
 * Close the point array, if not already closed in 2d.
 * Returns the input if already closed in 2d, or a newly
 * constructed RTPOINTARRAY.
 * TODO: move in ptarray.c
 */
RTPOINTARRAY* ptarray_close2d(const RTCTX *ctx, RTPOINTARRAY* ring);
RTPOINTARRAY*
ptarray_close2d(const RTCTX *ctx, RTPOINTARRAY* ring)
{
  RTPOINTARRAY* newring;

  /* close the ring if not already closed (2d only) */
  if ( ! ptarray_is_closed_2d(ctx, ring) )
  {
    /* close it up */
    newring = ptarray_addPoint(ctx, ring,
                               rt_getPoint_internal(ctx, ring, 0),
                               RTFLAGS_NDIMS(ring->flags),
                               ring->npoints);
    ring = newring;
  }
  return ring;
}

/* May return the same input or a new one (never zero) */
RTPOINTARRAY*
ring_make_geos_friendly(const RTCTX *ctx, RTPOINTARRAY* ring)
{
  RTPOINTARRAY* closedring;
  RTPOINTARRAY* ring_in = ring;

  /* close the ring if not already closed (2d only) */
  closedring = ptarray_close2d(ctx, ring);
  if (closedring != ring )
  {
    ring = closedring;
  }

  /* return 0 for collapsed ring (after closeup) */

  while ( ring->npoints < 4 )
  {
    RTPOINTARRAY *oring = ring;
    RTDEBUGF(ctx, 4, "ring has %d points, adding another", ring->npoints);
    /* let's add another... */
    ring = ptarray_addPoint(ctx, ring,
                            rt_getPoint_internal(ctx, ring, 0),
                            RTFLAGS_NDIMS(ring->flags),
                            ring->npoints);
    if ( oring != ring_in ) ptarray_free(ctx, oring);
  }


  return ring;
}

/* Make sure all rings are closed and have > 3 points.
 * May return the input untouched.
 */
RTGEOM *
rtpoly_make_geos_friendly(const RTCTX *ctx, RTPOLY *poly)
{
  RTGEOM* ret;
  RTPOINTARRAY **new_rings;
  int i;

  /* If the polygon has no rings there's nothing to do */
  if ( ! poly->nrings ) return (RTGEOM*)poly;

  /* Allocate enough pointers for all rings */
  new_rings = rtalloc(ctx, sizeof(RTPOINTARRAY*)*poly->nrings);

  /* All rings must be closed and have > 3 points */
  for (i=0; i<poly->nrings; i++)
  {
    RTPOINTARRAY* ring_in = poly->rings[i];
    RTPOINTARRAY* ring_out = ring_make_geos_friendly(ctx, ring_in);

    if ( ring_in != ring_out )
    {
      RTDEBUGF(ctx, 3, "rtpoly_make_geos_friendly: ring %d cleaned, now has %d points", i, ring_out->npoints);
      ptarray_free(ctx, ring_in);
    }
    else
    {
      RTDEBUGF(ctx, 3, "rtpoly_make_geos_friendly: ring %d untouched", i);
    }

    assert ( ring_out );
    new_rings[i] = ring_out;
  }

  rtfree(ctx, poly->rings);
  poly->rings = new_rings;
  ret = (RTGEOM*)poly;

  return ret;
}

/* Need NO or >1 points. Duplicate first if only one. */
RTGEOM *
rtline_make_geos_friendly(const RTCTX *ctx, RTLINE *line)
{
  RTGEOM *ret;

  if (line->points->npoints == 1) /* 0 is fine, 2 is fine */
  {
#if 1
    /* Duplicate point */
    line->points = ptarray_addPoint(ctx, line->points,
                                    rt_getPoint_internal(ctx, line->points, 0),
                                    RTFLAGS_NDIMS(line->points->flags),
                                    line->points->npoints);
    ret = (RTGEOM*)line;
#else
    /* Turn into a point */
    ret = (RTGEOM*)rtpoint_construct(ctx, line->srid, 0, line->points);
#endif
    return ret;
  }
  else
  {
    return (RTGEOM*)line;
    /* return rtline_clone(ctx, line); */
  }
}

RTGEOM *
rtcollection_make_geos_friendly(const RTCTX *ctx, RTCOLLECTION *g)
{
  RTGEOM **new_geoms;
  uint32_t i, new_ngeoms=0;
  RTCOLLECTION *ret;

  /* enough space for all components */
  new_geoms = rtalloc(ctx, sizeof(RTGEOM *)*g->ngeoms);

  ret = rtalloc(ctx, sizeof(RTCOLLECTION));
  memcpy(ret, g, sizeof(RTCOLLECTION));
    ret->maxgeoms = g->ngeoms;

  for (i=0; i<g->ngeoms; i++)
  {
    RTGEOM* newg = rtgeom_make_geos_friendly(ctx, g->geoms[i]);
    if ( newg ) new_geoms[new_ngeoms++] = newg;
  }

  ret->bbox = NULL; /* recompute later... */

  ret->ngeoms = new_ngeoms;
  if ( new_ngeoms )
  {
    ret->geoms = new_geoms;
  }
  else
  {
    free(new_geoms);
    ret->geoms = NULL;
        ret->maxgeoms = 0;
  }

  return (RTGEOM*)ret;
}

/*
 * Fully node given linework
 */
static GEOSGeometry*
RTGEOM_GEOS_nodeLines(const RTCTX *ctx, const GEOSGeometry* lines)
{
  GEOSGeometry* noded;
  GEOSGeometry* point;

  /*
   * Union with first geometry point, obtaining full noding
   * and dissolving of duplicated repeated points
   *
   * TODO: substitute this with UnaryUnion?
   */

  point = RTGEOM_GEOS_getPointN(ctx, lines, 0);
  if ( ! point ) return NULL;

  RTDEBUGF(ctx, 3,
                 "Boundary point: %s",
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, point, 0)));

  noded = GEOSUnion_r(ctx->gctx, lines, point);
  if ( NULL == noded )
  {
    GEOSGeom_destroy_r(ctx->gctx, point);
    return NULL;
  }

  GEOSGeom_destroy_r(ctx->gctx, point);

  RTDEBUGF(ctx, 3,
                 "RTGEOM_GEOS_nodeLines: in[%s] out[%s]",
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, lines, 0)),
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, noded, 0)));

  return noded;
}

#if RTGEOM_GEOS_VERSION >= 33
/*
 * We expect rtgeom_geos_ensure_init(ctx);
 * Will return NULL on error (expect error handler being called by then)
 *
 */
static GEOSGeometry*
RTGEOM_GEOS_makeValidPolygon(const RTCTX *ctx, const GEOSGeometry* gin)
{
  GEOSGeom gout;
  GEOSGeom geos_bound;
  GEOSGeom geos_cut_edges, geos_area, collapse_points;
  GEOSGeometry *vgeoms[3]; /* One for area, one for cut-edges */
  unsigned int nvgeoms=0;

  assert (GEOSGeomTypeId_r(ctx->gctx, gin) == GEOS_POLYGON ||
          GEOSGeomTypeId_r(ctx->gctx, gin) == GEOS_MULTIPOLYGON);

  geos_bound = GEOSBoundary_r(ctx->gctx, gin);
  if ( NULL == geos_bound )
  {
    return NULL;
  }

  RTDEBUGF(ctx, 3,
                 "Boundaries: %s",
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, geos_bound, 0)));

  /* Use noded boundaries as initial "cut" edges */

#ifdef RTGEOM_PROFILE_MAKEVALID
  rtnotice(ctx, "ST_MakeValid: noding lines");
#endif


  geos_cut_edges = RTGEOM_GEOS_nodeLines(ctx, geos_bound);
  if ( NULL == geos_cut_edges )
  {
    GEOSGeom_destroy_r(ctx->gctx, geos_bound);
    rtnotice(ctx, "RTGEOM_GEOS_nodeLines(ctx): %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }

  /* NOTE: the noding process may drop lines collapsing to points.
   *       We want to retrive any of those */
  {
    GEOSGeometry* pi;
    GEOSGeometry* po;

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: extracting unique points from bounds");
#endif

    pi = GEOSGeom_extractUniquePoints_r(ctx->gctx, geos_bound);
    if ( NULL == pi )
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_bound);
      rtnotice(ctx, "GEOSGeom_extractUniquePoints_r(ctx->gctx): %s",
               rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    RTDEBUGF(ctx, 3,
                   "Boundaries input points %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, pi, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: extracting unique points from cut_edges");
#endif

    po = GEOSGeom_extractUniquePoints_r(ctx->gctx, geos_cut_edges);
    if ( NULL == po )
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_bound);
      GEOSGeom_destroy_r(ctx->gctx, pi);
      rtnotice(ctx, "GEOSGeom_extractUniquePoints_r(ctx->gctx): %s",
               rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    RTDEBUGF(ctx, 3,
                   "Boundaries output points %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, po, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: find collapse points");
#endif

    collapse_points = GEOSDifference_r(ctx->gctx, pi, po);
    if ( NULL == collapse_points )
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_bound);
      GEOSGeom_destroy_r(ctx->gctx, pi);
      GEOSGeom_destroy_r(ctx->gctx, po);
      rtnotice(ctx, "GEOSDifference_r(ctx->gctx): %s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    RTDEBUGF(ctx, 3,
                   "Collapse points: %s",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, collapse_points, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: cleanup(1)");
#endif

    GEOSGeom_destroy_r(ctx->gctx, pi);
    GEOSGeom_destroy_r(ctx->gctx, po);
  }
  GEOSGeom_destroy_r(ctx->gctx, geos_bound);

  RTDEBUGF(ctx, 3,
                 "Noded Boundaries: %s",
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, geos_cut_edges, 0)));

  /* And use an empty geometry as initial "area" */
  geos_area = GEOSGeom_createEmptyPolygon_r(ctx->gctx);
  if ( ! geos_area )
  {
    rtnotice(ctx, "GEOSGeom_createEmptyPolygon_r(ctx->gctx): %s", rtgeom_get_last_geos_error(ctx));
    GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
    return NULL;
  }

  /*
   * See if an area can be build with the remaining edges
   * and if it can, symdifference with the original area.
   * Iterate this until no more polygons can be created
   * with left-over edges.
   */
  while (GEOSGetNumGeometries_r(ctx->gctx, geos_cut_edges))
  {
    GEOSGeometry* new_area=0;
    GEOSGeometry* new_area_bound=0;
    GEOSGeometry* symdif=0;
    GEOSGeometry* new_cut_edges=0;

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: building area from %d edges", GEOSGetNumGeometries_r(ctx->gctx, geos_cut_edges));
#endif

    /*
     * ASSUMPTION: cut_edges should already be fully noded
     */

    new_area = RTGEOM_GEOS_buildArea(ctx, geos_cut_edges);
    if ( ! new_area )   /* must be an exception */
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
      GEOSGeom_destroy_r(ctx->gctx, geos_area);
      rtnotice(ctx, "RTGEOM_GEOS_buildArea(ctx) threw an error: %s",
               rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    if ( GEOSisEmpty_r(ctx->gctx, new_area) )
    {
      /* no more rings can be build with thes edges */
      GEOSGeom_destroy_r(ctx->gctx, new_area);
      break;
    }

    /*
     * We succeeded in building a ring !
     */

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: ring built with %d cut edges, saving boundaries", GEOSGetNumGeometries_r(ctx->gctx, geos_cut_edges));
#endif

    /*
     * Save the new ring boundaries first (to compute
     * further cut edges later)
     */
    new_area_bound = GEOSBoundary_r(ctx->gctx, new_area);
    if ( ! new_area_bound )
    {
      /* We did check for empty area already so
       * this must be some other error */
      rtnotice(ctx, "GEOSBoundary_r(ctx->gctx, '%s') threw an error: %s",
               rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, new_area, 0)),
               rtgeom_get_last_geos_error(ctx));
      GEOSGeom_destroy_r(ctx->gctx, new_area);
      GEOSGeom_destroy_r(ctx->gctx, geos_area);
      return NULL;
    }

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: running SymDifference with new area");
#endif

    /*
     * Now symdif new and old area
     */
    symdif = GEOSSymDifference_r(ctx->gctx, geos_area, new_area);
    if ( ! symdif )   /* must be an exception */
    {
      GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
      GEOSGeom_destroy_r(ctx->gctx, new_area);
      GEOSGeom_destroy_r(ctx->gctx, new_area_bound);
      GEOSGeom_destroy_r(ctx->gctx, geos_area);
      rtnotice(ctx, "GEOSSymDifference_r(ctx->gctx) threw an error: %s",
               rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

    GEOSGeom_destroy_r(ctx->gctx, geos_area);
    GEOSGeom_destroy_r(ctx->gctx, new_area);
    geos_area = symdif;
    symdif = 0;

    /*
     * Now let's re-set geos_cut_edges with what's left
     * from the original boundary.
     * ASSUMPTION: only the previous cut-edges can be
     *             left, so we don't need to reconsider
     *             the whole original boundaries
     *
     * NOTE: this is an expensive operation.
     *
     */

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice(ctx, "ST_MakeValid: computing new cut_edges (GEOSDifference)");
#endif

    new_cut_edges = GEOSDifference_r(ctx->gctx, geos_cut_edges, new_area_bound);
    GEOSGeom_destroy_r(ctx->gctx, new_area_bound);
    if ( ! new_cut_edges )   /* an exception ? */
    {
      /* cleanup and throw */
      GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
      GEOSGeom_destroy_r(ctx->gctx, geos_area);
      /* TODO: Shouldn't this be an rterror ? */
      rtnotice(ctx, "GEOSDifference_r(ctx->gctx) threw an error: %s",
               rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
    geos_cut_edges = new_cut_edges;
  }

#ifdef RTGEOM_PROFILE_MAKEVALID
  rtnotice(ctx, "ST_MakeValid: final checks");
#endif

  if ( ! GEOSisEmpty_r(ctx->gctx, geos_area) )
  {
    vgeoms[nvgeoms++] = geos_area;
  }
  else
  {
    GEOSGeom_destroy_r(ctx->gctx, geos_area);
  }

  if ( ! GEOSisEmpty_r(ctx->gctx, geos_cut_edges) )
  {
    vgeoms[nvgeoms++] = geos_cut_edges;
  }
  else
  {
    GEOSGeom_destroy_r(ctx->gctx, geos_cut_edges);
  }

  if ( ! GEOSisEmpty_r(ctx->gctx, collapse_points) )
  {
    vgeoms[nvgeoms++] = collapse_points;
  }
  else
  {
    GEOSGeom_destroy_r(ctx->gctx, collapse_points);
  }

  if ( 1 == nvgeoms )
  {
    /* Return cut edges */
    gout = vgeoms[0];
  }
  else
  {
    /* Collect areas and lines (if any line) */
    gout = GEOSGeom_createCollection_r(ctx->gctx, GEOS_GEOMETRYCOLLECTION, vgeoms, nvgeoms);
    if ( ! gout )   /* an exception again */
    {
      /* cleanup and throw */
      /* TODO: Shouldn't this be an rterror ? */
      rtnotice(ctx, "GEOSGeom_createCollection_r(ctx->gctx) threw an error: %s",
               rtgeom_get_last_geos_error(ctx));
      /* TODO: cleanup! */
      return NULL;
    }
  }

  return gout;

}

static GEOSGeometry*
RTGEOM_GEOS_makeValidLine(const RTCTX *ctx, const GEOSGeometry* gin)
{
  GEOSGeometry* noded;
  noded = RTGEOM_GEOS_nodeLines(ctx, gin);
  return noded;
}

static GEOSGeometry*
RTGEOM_GEOS_makeValidMultiLine(const RTCTX *ctx, const GEOSGeometry* gin)
{
  GEOSGeometry** lines;
  GEOSGeometry** points;
  GEOSGeometry* mline_out=0;
  GEOSGeometry* mpoint_out=0;
  GEOSGeometry* gout=0;
  uint32_t nlines=0, nlines_alloc;
  uint32_t npoints=0;
  uint32_t ngeoms=0, nsubgeoms;
  uint32_t i, j;

  ngeoms = GEOSGetNumGeometries_r(ctx->gctx, gin);

  nlines_alloc = ngeoms;
  lines = rtalloc(ctx, sizeof(GEOSGeometry*)*nlines_alloc);
  points = rtalloc(ctx, sizeof(GEOSGeometry*)*ngeoms);

  for (i=0; i<ngeoms; ++i)
  {
    const GEOSGeometry* g = GEOSGetGeometryN_r(ctx->gctx, gin, i);
    GEOSGeometry* vg;
    vg = RTGEOM_GEOS_makeValidLine(ctx, g);
    if ( GEOSisEmpty_r(ctx->gctx, vg) )
    {
      /* we don't care about this one */
      GEOSGeom_destroy_r(ctx->gctx, vg);
    }
    if ( GEOSGeomTypeId_r(ctx->gctx, vg) == GEOS_POINT )
    {
      points[npoints++] = vg;
    }
    else if ( GEOSGeomTypeId_r(ctx->gctx, vg) == GEOS_LINESTRING )
    {
      lines[nlines++] = vg;
    }
    else if ( GEOSGeomTypeId_r(ctx->gctx, vg) == GEOS_MULTILINESTRING )
    {
      nsubgeoms=GEOSGetNumGeometries_r(ctx->gctx, vg);
      nlines_alloc += nsubgeoms;
      lines = rtrealloc(ctx, lines, sizeof(GEOSGeometry*)*nlines_alloc);
      for (j=0; j<nsubgeoms; ++j)
      {
        const GEOSGeometry* gc = GEOSGetGeometryN_r(ctx->gctx, vg, j);
        /* NOTE: ownership of the cloned geoms will be
         *       taken by final collection */
        lines[nlines++] = GEOSGeom_clone_r(ctx->gctx, gc);
      }
    }
    else
    {
      /* NOTE: return from GEOSGeomType will leak
       * but we really don't expect this to happen */
      rterror(ctx, "unexpected geom type returned "
              "by RTGEOM_GEOS_makeValid: %s",
              GEOSGeomType_r(ctx->gctx, vg));
    }
  }

  if ( npoints )
  {
    if ( npoints > 1 )
    {
      mpoint_out = GEOSGeom_createCollection_r(ctx->gctx, GEOS_MULTIPOINT,
                                             points, npoints);
    }
    else
    {
      mpoint_out = points[0];
    }
  }

  if ( nlines )
  {
    if ( nlines > 1 )
    {
      mline_out = GEOSGeom_createCollection_r(ctx->gctx,
                      GEOS_MULTILINESTRING, lines, nlines);
    }
    else
    {
      mline_out = lines[0];
    }
  }

  rtfree(ctx, lines);

  if ( mline_out && mpoint_out )
  {
    points[0] = mline_out;
    points[1] = mpoint_out;
    gout = GEOSGeom_createCollection_r(ctx->gctx, GEOS_GEOMETRYCOLLECTION,
                                     points, 2);
  }
  else if ( mline_out )
  {
    gout = mline_out;
  }
  else if ( mpoint_out )
  {
    gout = mpoint_out;
  }

  rtfree(ctx, points);

  return gout;
}

static GEOSGeometry* RTGEOM_GEOS_makeValid(const RTCTX *ctx, const GEOSGeometry*);

/*
 * We expect rtgeom_geos_ensure_init(ctx);
 * Will return NULL on error (expect error handler being called by then)
 */
static GEOSGeometry*
RTGEOM_GEOS_makeValidCollection(const RTCTX *ctx, const GEOSGeometry* gin)
{
  int nvgeoms;
  GEOSGeometry **vgeoms;
  GEOSGeom gout;
  unsigned int i;

  nvgeoms = GEOSGetNumGeometries_r(ctx->gctx, gin);
  if ( nvgeoms == -1 ) {
    rterror(ctx, "GEOSGetNumGeometries: %s", rtgeom_get_last_geos_error(ctx));
    return 0;
  }

  vgeoms = rtalloc(ctx,  sizeof(GEOSGeometry*) * nvgeoms );
  if ( ! vgeoms ) {
    rterror(ctx, "RTGEOM_GEOS_makeValidCollection: out of memory");
    return 0;
  }

  for ( i=0; i<nvgeoms; ++i ) {
    vgeoms[i] = RTGEOM_GEOS_makeValid(ctx,  GEOSGetGeometryN_r(ctx->gctx, gin, i) );
    if ( ! vgeoms[i] ) {
      while (i--) GEOSGeom_destroy_r(ctx->gctx, vgeoms[i]);
      rtfree(ctx, vgeoms);
      /* we expect rterror being called already by makeValid */
      return NULL;
    }
  }

  /* Collect areas and lines (if any line) */
  gout = GEOSGeom_createCollection_r(ctx->gctx, GEOS_GEOMETRYCOLLECTION, vgeoms, nvgeoms);
  if ( ! gout )   /* an exception again */
  {
    /* cleanup and throw */
    for ( i=0; i<nvgeoms; ++i ) GEOSGeom_destroy_r(ctx->gctx, vgeoms[i]);
    rtfree(ctx, vgeoms);
    rterror(ctx, "GEOSGeom_createCollection_r(ctx->gctx) threw an error: %s",
             rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  rtfree(ctx, vgeoms);

  return gout;

}


static GEOSGeometry*
RTGEOM_GEOS_makeValid(const RTCTX *ctx, const GEOSGeometry* gin)
{
  GEOSGeometry* gout;
  char ret_char;

  /*
   * Step 2: return what we got so far if already valid
   */

  ret_char = GEOSisValid_r(ctx->gctx, gin);
  if ( ret_char == 2 )
  {
    /* I don't think should ever happen */
    rterror(ctx, "GEOSisValid_r(ctx->gctx): %s", rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  else if ( ret_char )
  {
    RTDEBUGF(ctx, 3,
                   "Geometry [%s] is valid. ",
                   rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, gin, 0)));

    /* It's valid at this step, return what we have */
    return GEOSGeom_clone_r(ctx->gctx, gin);
  }

  RTDEBUGF(ctx, 3,
                 "Geometry [%s] is still not valid: %s. "
                 "Will try to clean up further.",
                 rtgeom_to_ewkt(ctx, GEOS2RTGEOM(ctx, gin, 0)), rtgeom_get_last_geos_error(ctx));



  /*
   * Step 3 : make what we got valid
   */

  switch (GEOSGeomTypeId_r(ctx->gctx, gin))
  {
  case GEOS_MULTIPOINT:
  case GEOS_POINT:
    /* points are artays valid, but we might have invalid ordinate values */
    rtnotice(ctx, "PUNTUAL geometry resulted invalid to GEOS -- dunno how to clean that up");
    return NULL;
    break;

  case GEOS_LINESTRING:
    gout = RTGEOM_GEOS_makeValidLine(ctx, gin);
    if ( ! gout )  /* an exception or something */
    {
      /* cleanup and throw */
      rterror(ctx, "%s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    break; /* we've done */

  case GEOS_MULTILINESTRING:
    gout = RTGEOM_GEOS_makeValidMultiLine(ctx, gin);
    if ( ! gout )  /* an exception or something */
    {
      /* cleanup and throw */
      rterror(ctx, "%s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    break; /* we've done */

  case GEOS_POLYGON:
  case GEOS_MULTIPOLYGON:
  {
    gout = RTGEOM_GEOS_makeValidPolygon(ctx, gin);
    if ( ! gout )  /* an exception or something */
    {
      /* cleanup and throw */
      rterror(ctx, "%s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    break; /* we've done */
  }

  case GEOS_GEOMETRYCOLLECTION:
  {
    gout = RTGEOM_GEOS_makeValidCollection(ctx, gin);
    if ( ! gout )  /* an exception or something */
    {
      /* cleanup and throw */
      rterror(ctx, "%s", rtgeom_get_last_geos_error(ctx));
      return NULL;
    }
    break; /* we've done */
  }

  default:
  {
    char* typname = GEOSGeomType_r(ctx->gctx, gin);
    rtnotice(ctx, "ST_MakeValid: doesn't support geometry type: %s",
             typname);
    GEOSFree_r(ctx->gctx, typname);
    return NULL;
    break;
  }
  }

#if PARANOIA_LEVEL > 1
  /*
   * Now check if every point of input is also found
   * in output, or abort by returning NULL
   *
   * Input geometry was rtgeom_in
   */
  {
      int loss;
      GEOSGeometry *pi, *po, *pd;

      /* TODO: handle some errors here...
       * Lack of exceptions is annoying indeed,
       * I'm getting old --strk;
       */
      pi = GEOSGeom_extractUniquePoints_r(ctx->gctx, gin);
      po = GEOSGeom_extractUniquePoints_r(ctx->gctx, gout);
      pd = GEOSDifference_r(ctx->gctx, pi, po); /* input points - output points */
      GEOSGeom_destroy_r(ctx->gctx, pi);
      GEOSGeom_destroy_r(ctx->gctx, po);
      loss = !GEOSisEmpty_r(ctx->gctx, pd);
      GEOSGeom_destroy_r(ctx->gctx, pd);
      if ( loss )
      {
        rtnotice(ctx, "Vertices lost in RTGEOM_GEOS_makeValid");
        /* return NULL */
      }
  }
#endif /* PARANOIA_LEVEL > 1 */


  return gout;
}

/* Exported. Uses GEOS internally */
RTGEOM*
rtgeom_make_valid(const RTCTX *ctx, RTGEOM* rtgeom_in)
{
  int is3d;
  GEOSGeom geosgeom;
  GEOSGeometry* geosout;
  RTGEOM *rtgeom_out;

  is3d = RTFLAGS_GET_Z(rtgeom_in->flags);

  /*
   * Step 1 : try to convert to GEOS, if impossible, clean that up first
   *          otherwise (adding only duplicates of existing points)
   */

  rtgeom_geos_ensure_init(ctx);

  rtgeom_out = rtgeom_in;
  geosgeom = RTGEOM2GEOS(ctx, rtgeom_out, 0);
  if ( ! geosgeom )
  {
    RTDEBUGF(ctx, 4,
                   "Original geom can't be converted to GEOS _r(ctx->gctx, %s)"
                   " - will try cleaning that up first",
                   rtgeom_get_last_geos_error(ctx));


    rtgeom_out = rtgeom_make_geos_friendly(ctx, rtgeom_out);
    if ( ! rtgeom_out )
    {
      rterror(ctx, "Could not make a valid geometry out of input");
    }

    /* try again as we did cleanup now */
    /* TODO: invoke RTGEOM2GEOS directly with autoclean ? */
    geosgeom = RTGEOM2GEOS(ctx, rtgeom_out, 0);
    if ( ! geosgeom )
    {
      rterror(ctx, "Couldn't convert RTGEOM geom to GEOS: %s",
              rtgeom_get_last_geos_error(ctx));
      return NULL;
    }

  }
  else
  {
    RTDEBUG(ctx, 4, "original geom converted to GEOS");
    rtgeom_out = rtgeom_in;
  }

  geosout = RTGEOM_GEOS_makeValid(ctx, geosgeom);
  GEOSGeom_destroy_r(ctx->gctx, geosgeom);
  if ( ! geosout )
  {
    return NULL;
  }

  rtgeom_out = GEOS2RTGEOM(ctx, geosout, is3d);
  GEOSGeom_destroy_r(ctx->gctx, geosout);

  if ( rtgeom_is_collection(ctx, rtgeom_in) && ! rtgeom_is_collection(ctx, rtgeom_out) )
  {{
    RTGEOM **ogeoms = rtalloc(ctx, sizeof(RTGEOM*));
    RTGEOM *ogeom;
    RTDEBUG(ctx, 3, "rtgeom_make_valid: forcing multi");
    /* NOTE: this is safe because rtgeom_out is surely not rtgeom_in or
     * otherwise we couldn't have a collection and a non-collection */
    assert(rtgeom_in != rtgeom_out);
    ogeoms[0] = rtgeom_out;
    ogeom = (RTGEOM *)rtcollection_construct(ctx, RTMULTITYPE[rtgeom_out->type],
                              rtgeom_out->srid, rtgeom_out->bbox, 1, ogeoms);
    rtgeom_out->bbox = NULL;
    rtgeom_out = ogeom;
  }}

  rtgeom_out->srid = rtgeom_in->srid;
  return rtgeom_out;
}

#endif /* RTGEOM_GEOS_VERSION >= 33 */

