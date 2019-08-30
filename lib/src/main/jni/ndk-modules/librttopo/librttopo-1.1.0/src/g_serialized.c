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



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

/***********************************************************************
* GSERIALIZED metadata utility functions.
*/

int gserialized_has_bbox(const RTCTX *ctx, const GSERIALIZED *gser)
{
  return RTFLAGS_GET_BBOX(gser->flags);
}

int gserialized_has_z(const RTCTX *ctx, const GSERIALIZED *gser)
{
  return RTFLAGS_GET_Z(gser->flags);
}

int gserialized_has_m(const RTCTX *ctx, const GSERIALIZED *gser)
{
  return RTFLAGS_GET_M(gser->flags);
}

int gserialized_get_zm(const RTCTX *ctx, const GSERIALIZED *gser)
{
  return 2 * RTFLAGS_GET_Z(gser->flags) + RTFLAGS_GET_M(gser->flags);
}

int gserialized_ndims(const RTCTX *ctx, const GSERIALIZED *gser)
{
  return RTFLAGS_NDIMS(gser->flags);
}

int gserialized_is_geodetic(const RTCTX *ctx, const GSERIALIZED *gser)
{
    return RTFLAGS_GET_GEODETIC(gser->flags);
}

uint32_t gserialized_max_header_size(const RTCTX *ctx)
{
  /* read GSERIALIZED size + max bbox according gbox_serialized_size(ctx, 2 + Z + M) + 1 int for type */
  return sizeof(GSERIALIZED) + 8 * sizeof(float) + sizeof(int);
}

uint32_t gserialized_get_type(const RTCTX *ctx, const GSERIALIZED *s)
{
  uint32_t *ptr;
  assert(s);
  ptr = (uint32_t*)(s->data);
  RTDEBUG(ctx, 4,"entered");
  if ( RTFLAGS_GET_BBOX(s->flags) )
  {
    RTDEBUGF(ctx, 4,"skipping forward past bbox (%d bytes)",gbox_serialized_size(ctx, s->flags));
    ptr += (gbox_serialized_size(ctx, s->flags) / sizeof(uint32_t));
  }
  return *ptr;
}

int32_t gserialized_get_srid(const RTCTX *ctx, const GSERIALIZED *s)
{
  int32_t srid = 0;
  srid = srid | (s->srid[0] << 16);
  srid = srid | (s->srid[1] << 8);
  srid = srid | s->srid[2];
  /* Only the first 21 bits are set. Slide up and back to pull
     the negative bits down, if we need them. */
  srid = (srid<<11)>>11;

  /* 0 is our internal unknown value. We'll map back and forth here for now */
  if ( srid == 0 )
    return SRID_UNKNOWN;
  else
    return clamp_srid(ctx, srid);
}

void gserialized_set_srid(const RTCTX *ctx, GSERIALIZED *s, int32_t srid)
{
  RTDEBUGF(ctx, 3, "Called with srid = %d", srid);

  srid = clamp_srid(ctx, srid);

  /* 0 is our internal unknown value.
   * We'll map back and forth here for now */
  if ( srid == SRID_UNKNOWN )
    srid = 0;

  s->srid[0] = (srid & 0x001F0000) >> 16;
  s->srid[1] = (srid & 0x0000FF00) >> 8;
  s->srid[2] = (srid & 0x000000FF);
}

GSERIALIZED* gserialized_copy(const RTCTX *ctx, const GSERIALIZED *g)
{
  GSERIALIZED *g_out = NULL;
  assert(g);
  g_out = (GSERIALIZED*)rtalloc(ctx, SIZE_GET(g->size));
  memcpy((uint8_t*)g_out,(uint8_t*)g,SIZE_GET(g->size));
  return g_out;
}

static size_t gserialized_is_empty_recurse(const RTCTX *ctx, const uint8_t *p, int *isempty);
static size_t gserialized_is_empty_recurse(const RTCTX *ctx, const uint8_t *p, int *isempty)
{
  int i;
  int32_t type, num;

  memcpy(&type, p, 4);
  memcpy(&num, p+4, 4);

  if ( rttype_is_collection(ctx, type) )
  {
    size_t lz = 8;
    for ( i = 0; i < num; i++ )
    {
      lz += gserialized_is_empty_recurse(ctx, p+lz, isempty);
      if ( ! *isempty )
        return lz;
    }
    *isempty = RT_TRUE;
    return lz;
  }
  else
  {
    *isempty = (num == 0 ? RT_TRUE : RT_FALSE);
    return 8;
  }
}

int gserialized_is_empty(const RTCTX *ctx, const GSERIALIZED *g)
{
  uint8_t *p = (uint8_t*)g;
  int isempty = 0;
  assert(g);

  p += 8; /* Skip varhdr and srid/flags */
  if( RTFLAGS_GET_BBOX(g->flags) )
    p += gbox_serialized_size(ctx, g->flags); /* Skip the box */

  gserialized_is_empty_recurse(ctx, p, &isempty);
  return isempty;
}

char* gserialized_to_string(const RTCTX *ctx, const GSERIALIZED *g)
{
  return rtgeom_to_wkt(ctx, rtgeom_from_gserialized(ctx, g), RTWKT_ISO, 12, 0);
}

int gserialized_read_gbox_p(const RTCTX *ctx, const GSERIALIZED *g, RTGBOX *gbox)
{

  /* Null input! */
  if ( ! ( g && gbox ) ) return RT_FAILURE;

  /* Initialize the flags on the box */
  gbox->flags = g->flags;

  /* Has pre-calculated box */
  if ( RTFLAGS_GET_BBOX(g->flags) )
  {
    int i = 0;
    float *fbox = (float*)(g->data);
    gbox->xmin = fbox[i++];
    gbox->xmax = fbox[i++];
    gbox->ymin = fbox[i++];
    gbox->ymax = fbox[i++];

    /* Geodetic? Read next dimension (geocentric Z) and return */
    if ( RTFLAGS_GET_GEODETIC(g->flags) )
    {
      gbox->zmin = fbox[i++];
      gbox->zmax = fbox[i++];
      return RT_SUCCESS;
    }
    /* Cartesian? Read extra dimensions (if there) and return */
    if ( RTFLAGS_GET_Z(g->flags) )
    {
      gbox->zmin = fbox[i++];
      gbox->zmax = fbox[i++];
    }
    if ( RTFLAGS_GET_M(g->flags) )
    {
      gbox->mmin = fbox[i++];
      gbox->mmax = fbox[i++];
    }
    return RT_SUCCESS;
  }

  return RT_FAILURE;
}

/*
* Populate a bounding box *without* allocating an RTGEOM. Useful
* for some performance purposes.
*/
static int gserialized_peek_gbox_p(const RTCTX *ctx, const GSERIALIZED *g, RTGBOX *gbox)
{
  uint32_t type = gserialized_get_type(ctx, g);

  /* Peeking doesn't help if you already have a box or are geodetic */
  if ( RTFLAGS_GET_GEODETIC(g->flags) || RTFLAGS_GET_BBOX(g->flags) )
  {
    return RT_FAILURE;
  }

  /* Boxes of points are easy peasy */
  if ( type == RTPOINTTYPE )
  {
    int i = 1; /* Start past <pointtype><padding> */
    double *dptr = (double*)(g->data);

    /* Read the empty flag */
    int *iptr = (int*)(g->data);
    int isempty = (iptr[1] == 0);

    /* EMPTY point has no box */
    if ( isempty ) return RT_FAILURE;

    gbox->xmin = gbox->xmax = dptr[i++];
    gbox->ymin = gbox->ymax = dptr[i++];
    if ( RTFLAGS_GET_Z(g->flags) )
    {
      gbox->zmin = gbox->zmax = dptr[i++];
    }
    if ( RTFLAGS_GET_M(g->flags) )
    {
      gbox->mmin = gbox->mmax = dptr[i++];
    }
    gbox_float_round(ctx, gbox);
    return RT_SUCCESS;
  }
  /* We can calculate the box of a two-point cartesian line trivially */
  else if ( type == RTLINETYPE )
  {
    int ndims = RTFLAGS_NDIMS(g->flags);
    int i = 0; /* Start at <linetype><npoints> */
    double *dptr = (double*)(g->data);
    int *iptr = (int*)(g->data);
    int npoints = iptr[1]; /* Read the npoints */

    /* This only works with 2-point lines */
    if ( npoints != 2 )
      return RT_FAILURE;

    /* Advance to X */
    /* Past <linetype><npoints> */
    i++;
    gbox->xmin = FP_MIN(dptr[i], dptr[i+ndims]);
    gbox->xmax = FP_MAX(dptr[i], dptr[i+ndims]);

    /* Advance to Y */
    i++;
    gbox->ymin = FP_MIN(dptr[i], dptr[i+ndims]);
    gbox->ymax = FP_MAX(dptr[i], dptr[i+ndims]);

    if ( RTFLAGS_GET_Z(g->flags) )
    {
      /* Advance to Z */
      i++;
      gbox->zmin = FP_MIN(dptr[i], dptr[i+ndims]);
      gbox->zmax = FP_MAX(dptr[i], dptr[i+ndims]);
    }
    if ( RTFLAGS_GET_M(g->flags) )
    {
      /* Advance to M */
      i++;
      gbox->mmin = FP_MIN(dptr[i], dptr[i+ndims]);
      gbox->mmax = FP_MAX(dptr[i], dptr[i+ndims]);
    }
    gbox_float_round(ctx, gbox);
    return RT_SUCCESS;
  }
  /* We can also do single-entry multi-points */
  else if ( type == RTMULTIPOINTTYPE )
  {
    int i = 0; /* Start at <multipointtype><ngeoms> */
    double *dptr = (double*)(g->data);
    int *iptr = (int*)(g->data);
    int ngeoms = iptr[1]; /* Read the ngeoms */

    /* This only works with single-entry multipoints */
    if ( ngeoms != 1 )
      return RT_FAILURE;

    /* Move forward two doubles (four ints) */
    /* Past <multipointtype><ngeoms> */
    /* Past <pointtype><emtpyflat> */
    i += 2;

    /* Read the doubles from the one point */
    gbox->xmin = gbox->xmax = dptr[i++];
    gbox->ymin = gbox->ymax = dptr[i++];
    if ( RTFLAGS_GET_Z(g->flags) )
    {
      gbox->zmin = gbox->zmax = dptr[i++];
    }
    if ( RTFLAGS_GET_M(g->flags) )
    {
      gbox->mmin = gbox->mmax = dptr[i++];
    }
    gbox_float_round(ctx, gbox);
    return RT_SUCCESS;
  }
  /* And we can do single-entry multi-lines with two vertices (!!!) */
  else if ( type == RTMULTILINETYPE )
  {
    int ndims = RTFLAGS_NDIMS(g->flags);
    int i = 0; /* Start at <multilinetype><ngeoms> */
    double *dptr = (double*)(g->data);
    int *iptr = (int*)(g->data);
    int ngeoms = iptr[1]; /* Read the ngeoms */
    int npoints;

    /* This only works with 1-line multilines */
    if ( ngeoms != 1 )
      return RT_FAILURE;

    /* Npoints is at <multilinetype><ngeoms><linetype><npoints> */
    npoints = iptr[3];

    if ( npoints != 2 )
      return RT_FAILURE;

    /* Advance to X */
    /* Move forward two doubles (four ints) */
    /* Past <multilinetype><ngeoms> */
    /* Past <linetype><npoints> */
    i += 2;
    gbox->xmin = FP_MIN(dptr[i], dptr[i+ndims]);
    gbox->xmax = FP_MAX(dptr[i], dptr[i+ndims]);

    /* Advance to Y */
    i++;
    gbox->ymin = FP_MIN(dptr[i], dptr[i+ndims]);
    gbox->ymax = FP_MAX(dptr[i], dptr[i+ndims]);

    if ( RTFLAGS_GET_Z(g->flags) )
    {
      /* Advance to Z */
      i++;
      gbox->zmin = FP_MIN(dptr[i], dptr[i+ndims]);
      gbox->zmax = FP_MAX(dptr[i], dptr[i+ndims]);
    }
    if ( RTFLAGS_GET_M(g->flags) )
    {
      /* Advance to M */
      i++;
      gbox->mmin = FP_MIN(dptr[i], dptr[i+ndims]);
      gbox->mmax = FP_MAX(dptr[i], dptr[i+ndims]);
    }
    gbox_float_round(ctx, gbox);
    return RT_SUCCESS;
  }

  return RT_FAILURE;
}

/**
* Read the bounding box off a serialization and calculate one if
* it is not already there.
*/
int gserialized_get_gbox_p(const RTCTX *ctx, const GSERIALIZED *g, RTGBOX *box)
{
  /* Try to just read the serialized box. */
  if ( gserialized_read_gbox_p(ctx, g, box) == RT_SUCCESS )
  {
    return RT_SUCCESS;
  }
  /* No box? Try to peek into simpler geometries and */
  /* derive a box without creating an rtgeom */
  else if ( gserialized_peek_gbox_p(ctx, g, box) == RT_SUCCESS )
  {
    return RT_SUCCESS;
  }
  /* Damn! Nothing for it but to create an rtgeom... */
  /* See http://trac.osgeo.org/postgis/ticket/1023 */
  else
  {
    RTGEOM *rtgeom = rtgeom_from_gserialized(ctx, g);
    int ret = rtgeom_calculate_gbox(ctx, rtgeom, box);
    gbox_float_round(ctx, box);
    rtgeom_free(ctx, rtgeom);
    return ret;
  }
}


/***********************************************************************
* Calculate the GSERIALIZED size for an RTGEOM.
*/

/* Private functions */

static size_t gserialized_from_any_size(const RTCTX *ctx, const RTGEOM *geom); /* Local prototype */

static size_t gserialized_from_rtpoint_size(const RTCTX *ctx, const RTPOINT *point)
{
  size_t size = 4; /* Type number. */

  assert(point);

  size += 4; /* Number of points (one or zero (empty)). */
  size += point->point->npoints * RTFLAGS_NDIMS(point->flags) * sizeof(double);

  RTDEBUGF(ctx, 3, "point size = %d", size);

  return size;
}

static size_t gserialized_from_rtline_size(const RTCTX *ctx, const RTLINE *line)
{
  size_t size = 4; /* Type number. */

  assert(line);

  size += 4; /* Number of points (zero => empty). */
  size += line->points->npoints * RTFLAGS_NDIMS(line->flags) * sizeof(double);

  RTDEBUGF(ctx, 3, "linestring size = %d", size);

  return size;
}

static size_t gserialized_from_rttriangle_size(const RTCTX *ctx, const RTTRIANGLE *triangle)
{
  size_t size = 4; /* Type number. */

  assert(triangle);

  size += 4; /* Number of points (zero => empty). */
  size += triangle->points->npoints * RTFLAGS_NDIMS(triangle->flags) * sizeof(double);

  RTDEBUGF(ctx, 3, "triangle size = %d", size);

  return size;
}

static size_t gserialized_from_rtpoly_size(const RTCTX *ctx, const RTPOLY *poly)
{
  size_t size = 4; /* Type number. */
  int i = 0;

  assert(poly);

  size += 4; /* Number of rings (zero => empty). */
  if ( poly->nrings % 2 )
    size += 4; /* Padding to double alignment. */

  for ( i = 0; i < poly->nrings; i++ )
  {
    size += 4; /* Number of points in ring. */
    size += poly->rings[i]->npoints * RTFLAGS_NDIMS(poly->flags) * sizeof(double);
  }

  RTDEBUGF(ctx, 3, "polygon size = %d", size);

  return size;
}

static size_t gserialized_from_rtcircstring_size(const RTCTX *ctx, const RTCIRCSTRING *curve)
{
  size_t size = 4; /* Type number. */

  assert(curve);

  size += 4; /* Number of points (zero => empty). */
  size += curve->points->npoints * RTFLAGS_NDIMS(curve->flags) * sizeof(double);

  RTDEBUGF(ctx, 3, "circstring size = %d", size);

  return size;
}

static size_t gserialized_from_rtcollection_size(const RTCTX *ctx, const RTCOLLECTION *col)
{
  size_t size = 4; /* Type number. */
  int i = 0;

  assert(col);

  size += 4; /* Number of sub-geometries (zero => empty). */

  for ( i = 0; i < col->ngeoms; i++ )
  {
    size_t subsize = gserialized_from_any_size(ctx, col->geoms[i]);
    size += subsize;
    RTDEBUGF(ctx, 3, "rtcollection subgeom(%d) size = %d", i, subsize);
  }

  RTDEBUGF(ctx, 3, "rtcollection size = %d", size);

  return size;
}

static size_t gserialized_from_any_size(const RTCTX *ctx, const RTGEOM *geom)
{
  RTDEBUGF(ctx, 2, "Input type: %s", rttype_name(ctx, geom->type));

  switch (geom->type)
  {
  case RTPOINTTYPE:
    return gserialized_from_rtpoint_size(ctx, (RTPOINT *)geom);
  case RTLINETYPE:
    return gserialized_from_rtline_size(ctx, (RTLINE *)geom);
  case RTPOLYGONTYPE:
    return gserialized_from_rtpoly_size(ctx, (RTPOLY *)geom);
  case RTTRIANGLETYPE:
    return gserialized_from_rttriangle_size(ctx, (RTTRIANGLE *)geom);
  case RTCIRCSTRINGTYPE:
    return gserialized_from_rtcircstring_size(ctx, (RTCIRCSTRING *)geom);
  case RTCURVEPOLYTYPE:
  case RTCOMPOUNDTYPE:
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTICURVETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTMULTISURFACETYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    return gserialized_from_rtcollection_size(ctx, (RTCOLLECTION *)geom);
  default:
    rterror(ctx, "Unknown geometry type: %d - %s", geom->type, rttype_name(ctx, geom->type));
    return 0;
  }
}

/* Public function */

size_t gserialized_from_rtgeom_size(const RTCTX *ctx, const RTGEOM *geom)
{
  size_t size = 8; /* Header overhead. */
  assert(geom);

  if ( geom->bbox )
    size += gbox_serialized_size(ctx, geom->flags);

  size += gserialized_from_any_size(ctx, geom);
  RTDEBUGF(ctx, 3, "g_serialize size = %d", size);

  return size;
}

/***********************************************************************
* Serialize an RTGEOM into GSERIALIZED.
*/

/* Private functions */

static size_t gserialized_from_rtgeom_any(const RTCTX *ctx, const RTGEOM *geom, uint8_t *buf);

static size_t gserialized_from_rtpoint(const RTCTX *ctx, const RTPOINT *point, uint8_t *buf)
{
  uint8_t *loc;
  int ptsize = ptarray_point_size(ctx, point->point);
  int type = RTPOINTTYPE;

  assert(point);
  assert(buf);

  if ( RTFLAGS_GET_ZM(point->flags) != RTFLAGS_GET_ZM(point->point->flags) )
    rterror(ctx, "Dimensions mismatch in rtpoint");

  RTDEBUGF(ctx, 2, "rtpoint_to_gserialized(%p, %p) called", point, buf);

  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);
  /* Write in the number of points (0 => empty). */
  memcpy(loc, &(point->point->npoints), sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Copy in the ordinates. */
  if ( point->point->npoints > 0 )
  {
    memcpy(loc, rt_getPoint_internal(ctx, point->point, 0), ptsize);
    loc += ptsize;
  }

  return (size_t)(loc - buf);
}

static size_t gserialized_from_rtline(const RTCTX *ctx, const RTLINE *line, uint8_t *buf)
{
  uint8_t *loc;
  int ptsize;
  size_t size;
  int type = RTLINETYPE;

  assert(line);
  assert(buf);

  RTDEBUGF(ctx, 2, "rtline_to_gserialized(%p, %p) called", line, buf);

  if ( RTFLAGS_GET_Z(line->flags) != RTFLAGS_GET_Z(line->points->flags) )
    rterror(ctx, "Dimensions mismatch in rtline");

  ptsize = ptarray_point_size(ctx, line->points);

  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the npoints. */
  memcpy(loc, &(line->points->npoints), sizeof(uint32_t));
  loc += sizeof(uint32_t);

  RTDEBUGF(ctx, 3, "rtline_to_gserialized added npoints (%d)", line->points->npoints);

  /* Copy in the ordinates. */
  if ( line->points->npoints > 0 )
  {
    size = line->points->npoints * ptsize;
    memcpy(loc, rt_getPoint_internal(ctx, line->points, 0), size);
    loc += size;
  }
  RTDEBUGF(ctx, 3, "rtline_to_gserialized copied serialized_pointlist (%d bytes)", ptsize * line->points->npoints);

  return (size_t)(loc - buf);
}

static size_t gserialized_from_rtpoly(const RTCTX *ctx, const RTPOLY *poly, uint8_t *buf)
{
  int i;
  uint8_t *loc;
  int ptsize;
  int type = RTPOLYGONTYPE;

  assert(poly);
  assert(buf);

  RTDEBUG(ctx, 2, "rtpoly_to_gserialized called");

  ptsize = sizeof(double) * RTFLAGS_NDIMS(poly->flags);
  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the nrings. */
  memcpy(loc, &(poly->nrings), sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the npoints per ring. */
  for ( i = 0; i < poly->nrings; i++ )
  {
    memcpy(loc, &(poly->rings[i]->npoints), sizeof(uint32_t));
    loc += sizeof(uint32_t);
  }

  /* Add in padding if necessary to remain double aligned. */
  if ( poly->nrings % 2 )
  {
    memset(loc, 0, sizeof(uint32_t));
    loc += sizeof(uint32_t);
  }

  /* Copy in the ordinates. */
  for ( i = 0; i < poly->nrings; i++ )
  {
    RTPOINTARRAY *pa = poly->rings[i];
    size_t pasize;

    if ( RTFLAGS_GET_ZM(poly->flags) != RTFLAGS_GET_ZM(pa->flags) )
      rterror(ctx, "Dimensions mismatch in rtpoly");

    pasize = pa->npoints * ptsize;
    memcpy(loc, rt_getPoint_internal(ctx, pa, 0), pasize);
    loc += pasize;
  }
  return (size_t)(loc - buf);
}

static size_t gserialized_from_rttriangle(const RTCTX *ctx, const RTTRIANGLE *triangle, uint8_t *buf)
{
  uint8_t *loc;
  int ptsize;
  size_t size;
  int type = RTTRIANGLETYPE;

  assert(triangle);
  assert(buf);

  RTDEBUGF(ctx, 2, "rttriangle_to_gserialized(%p, %p) called", triangle, buf);

  if ( RTFLAGS_GET_ZM(triangle->flags) != RTFLAGS_GET_ZM(triangle->points->flags) )
    rterror(ctx, "Dimensions mismatch in rttriangle");

  ptsize = ptarray_point_size(ctx, triangle->points);

  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the npoints. */
  memcpy(loc, &(triangle->points->npoints), sizeof(uint32_t));
  loc += sizeof(uint32_t);

  RTDEBUGF(ctx, 3, "rttriangle_to_gserialized added npoints (%d)", triangle->points->npoints);

  /* Copy in the ordinates. */
  if ( triangle->points->npoints > 0 )
  {
    size = triangle->points->npoints * ptsize;
    memcpy(loc, rt_getPoint_internal(ctx, triangle->points, 0), size);
    loc += size;
  }
  RTDEBUGF(ctx, 3, "rttriangle_to_gserialized copied serialized_pointlist (%d bytes)", ptsize * triangle->points->npoints);

  return (size_t)(loc - buf);
}

static size_t gserialized_from_rtcircstring(const RTCTX *ctx, const RTCIRCSTRING *curve, uint8_t *buf)
{
  uint8_t *loc;
  int ptsize;
  size_t size;
  int type = RTCIRCSTRINGTYPE;

  assert(curve);
  assert(buf);

  if (RTFLAGS_GET_ZM(curve->flags) != RTFLAGS_GET_ZM(curve->points->flags))
    rterror(ctx, "Dimensions mismatch in rtcircstring");


  ptsize = ptarray_point_size(ctx, curve->points);
  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the npoints. */
  memcpy(loc, &curve->points->npoints, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Copy in the ordinates. */
  if ( curve->points->npoints > 0 )
  {
    size = curve->points->npoints * ptsize;
    memcpy(loc, rt_getPoint_internal(ctx, curve->points, 0), size);
    loc += size;
  }

  return (size_t)(loc - buf);
}

static size_t gserialized_from_rtcollection(const RTCTX *ctx, const RTCOLLECTION *coll, uint8_t *buf)
{
  size_t subsize = 0;
  uint8_t *loc;
  int i;
  int type;

  assert(coll);
  assert(buf);

  type = coll->type;
  loc = buf;

  /* Write in the type. */
  memcpy(loc, &type, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Write in the number of subgeoms. */
  memcpy(loc, &coll->ngeoms, sizeof(uint32_t));
  loc += sizeof(uint32_t);

  /* Serialize subgeoms. */
  for ( i=0; i<coll->ngeoms; i++ )
  {
    if (RTFLAGS_GET_ZM(coll->flags) != RTFLAGS_GET_ZM(coll->geoms[i]->flags))
      rterror(ctx, "Dimensions mismatch in rtcollection");
    subsize = gserialized_from_rtgeom_any(ctx, coll->geoms[i], loc);
    loc += subsize;
  }

  return (size_t)(loc - buf);
}

static size_t gserialized_from_rtgeom_any(const RTCTX *ctx, const RTGEOM *geom, uint8_t *buf)
{
  assert(geom);
  assert(buf);

  RTDEBUGF(ctx, 2, "Input type (%d) %s, hasz: %d hasm: %d",
    geom->type, rttype_name(ctx, geom->type),
    RTFLAGS_GET_Z(geom->flags), RTFLAGS_GET_M(geom->flags));
  RTDEBUGF(ctx, 2, "RTGEOM(%p) uint8_t(%p)", geom, buf);

  switch (geom->type)
  {
  case RTPOINTTYPE:
    return gserialized_from_rtpoint(ctx, (RTPOINT *)geom, buf);
  case RTLINETYPE:
    return gserialized_from_rtline(ctx, (RTLINE *)geom, buf);
  case RTPOLYGONTYPE:
    return gserialized_from_rtpoly(ctx, (RTPOLY *)geom, buf);
  case RTTRIANGLETYPE:
    return gserialized_from_rttriangle(ctx, (RTTRIANGLE *)geom, buf);
  case RTCIRCSTRINGTYPE:
    return gserialized_from_rtcircstring(ctx, (RTCIRCSTRING *)geom, buf);
  case RTCURVEPOLYTYPE:
  case RTCOMPOUNDTYPE:
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTICURVETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTMULTISURFACETYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    return gserialized_from_rtcollection(ctx, (RTCOLLECTION *)geom, buf);
  default:
    rterror(ctx, "Unknown geometry type: %d - %s", geom->type, rttype_name(ctx, geom->type));
    return 0;
  }
  return 0;
}

static size_t gserialized_from_gbox(const RTCTX *ctx, const RTGBOX *gbox, uint8_t *buf)
{
  uint8_t *loc = buf;
  float f;
  size_t return_size;

  assert(buf);

  f = next_float_down(ctx, gbox->xmin);
  memcpy(loc, &f, sizeof(float));
  loc += sizeof(float);

  f = next_float_up(ctx, gbox->xmax);
  memcpy(loc, &f, sizeof(float));
  loc += sizeof(float);

  f = next_float_down(ctx, gbox->ymin);
  memcpy(loc, &f, sizeof(float));
  loc += sizeof(float);

  f = next_float_up(ctx, gbox->ymax);
  memcpy(loc, &f, sizeof(float));
  loc += sizeof(float);

  if ( RTFLAGS_GET_GEODETIC(gbox->flags) )
  {
    f = next_float_down(ctx, gbox->zmin);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);

    f = next_float_up(ctx, gbox->zmax);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);

    return_size = (size_t)(loc - buf);
    RTDEBUGF(ctx, 4, "returning size %d", return_size);
    return return_size;
  }

  if ( RTFLAGS_GET_Z(gbox->flags) )
  {
    f = next_float_down(ctx, gbox->zmin);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);

    f = next_float_up(ctx, gbox->zmax);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);

  }

  if ( RTFLAGS_GET_M(gbox->flags) )
  {
    f = next_float_down(ctx, gbox->mmin);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);

    f = next_float_up(ctx, gbox->mmax);
    memcpy(loc, &f, sizeof(float));
    loc += sizeof(float);
  }
  return_size = (size_t)(loc - buf);
  RTDEBUGF(ctx, 4, "returning size %d", return_size);
  return return_size;
}

/* Public function */

GSERIALIZED* gserialized_from_rtgeom(const RTCTX *ctx, RTGEOM *geom, int is_geodetic, size_t *size)
{
  size_t expected_size = 0;
  size_t return_size = 0;
  uint8_t *serialized = NULL;
  uint8_t *ptr = NULL;
  GSERIALIZED *g = NULL;
  assert(geom);

  /*
  ** See if we need a bounding box, add one if we don't have one.
  */
  if ( (! geom->bbox) && rtgeom_needs_bbox(ctx, geom) && (!rtgeom_is_empty(ctx, geom)) )
  {
    rtgeom_add_bbox(ctx, geom);
  }

  /*
  ** Harmonize the flags to the state of the rtgeom
  */
  if ( geom->bbox )
    RTFLAGS_SET_BBOX(geom->flags, 1);

  /* Set up the uint8_t buffer into which we are going to write the serialized geometry. */
  expected_size = gserialized_from_rtgeom_size(ctx, geom);
  serialized = rtalloc(ctx, expected_size);
  ptr = serialized;

  /* Move past size, srid and flags. */
  ptr += 8;

  /* Write in the serialized form of the gbox, if necessary. */
  if ( geom->bbox )
    ptr += gserialized_from_gbox(ctx, geom->bbox, ptr);

  /* Write in the serialized form of the geometry. */
  ptr += gserialized_from_rtgeom_any(ctx, geom, ptr);

  /* Calculate size as returned by data processing functions. */
  return_size = ptr - serialized;

  if ( expected_size != return_size ) /* Uh oh! */
  {
    rterror(ctx, "Return size (%d) not equal to expected size (%d)!", return_size, expected_size);
    return NULL;
  }

  if ( size ) /* Return the output size to the caller if necessary. */
    *size = return_size;

  g = (GSERIALIZED*)serialized;

  /*
  ** We are aping PgSQL code here, PostGIS code should use
  ** VARSIZE to set this for real.
  */
  g->size = return_size << 2;

  /* Set the SRID! */
  gserialized_set_srid(ctx, g, geom->srid);

  g->flags = geom->flags;

  return g;
}

/***********************************************************************
* De-serialize GSERIALIZED into an RTGEOM.
*/

static RTGEOM* rtgeom_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size);

static RTPOINT* rtpoint_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint8_t *start_ptr = data_ptr;
  RTPOINT *point;
  uint32_t npoints = 0;

  assert(data_ptr);

  point = (RTPOINT*)rtalloc(ctx, sizeof(RTPOINT));
  point->srid = SRID_UNKNOWN; /* Default */
  point->bbox = NULL;
  point->type = RTPOINTTYPE;
  point->flags = g_flags;

  data_ptr += 4; /* Skip past the type. */
  npoints = rt_get_uint32_t(ctx, data_ptr); /* Zero => empty geometry */
  data_ptr += 4; /* Skip past the npoints. */

  if ( npoints > 0 )
    point->point = ptarray_construct_reference_data(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), 1, data_ptr);
  else
    point->point = ptarray_construct(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), 0); /* Empty point */

  data_ptr += npoints * RTFLAGS_NDIMS(g_flags) * sizeof(double);

  if ( g_size )
    *g_size = data_ptr - start_ptr;

  return point;
}

static RTLINE* rtline_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint8_t *start_ptr = data_ptr;
  RTLINE *line;
  uint32_t npoints = 0;

  assert(data_ptr);

  line = (RTLINE*)rtalloc(ctx, sizeof(RTLINE));
  line->srid = SRID_UNKNOWN; /* Default */
  line->bbox = NULL;
  line->type = RTLINETYPE;
  line->flags = g_flags;

  data_ptr += 4; /* Skip past the type. */
  npoints = rt_get_uint32_t(ctx, data_ptr); /* Zero => empty geometry */
  data_ptr += 4; /* Skip past the npoints. */

  if ( npoints > 0 )
    line->points = ptarray_construct_reference_data(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), npoints, data_ptr);

  else
    line->points = ptarray_construct(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), 0); /* Empty linestring */

  data_ptr += RTFLAGS_NDIMS(g_flags) * npoints * sizeof(double);

  if ( g_size )
    *g_size = data_ptr - start_ptr;

  return line;
}

static RTPOLY* rtpoly_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint8_t *start_ptr = data_ptr;
  RTPOLY *poly;
  uint8_t *ordinate_ptr;
  uint32_t nrings = 0;
  int i = 0;

  assert(data_ptr);

  poly = (RTPOLY*)rtalloc(ctx, sizeof(RTPOLY));
  poly->srid = SRID_UNKNOWN; /* Default */
  poly->bbox = NULL;
  poly->type = RTPOLYGONTYPE;
  poly->flags = g_flags;

  data_ptr += 4; /* Skip past the polygontype. */
  nrings = rt_get_uint32_t(ctx, data_ptr); /* Zero => empty geometry */
  poly->nrings = nrings;
  RTDEBUGF(ctx, 4, "nrings = %d", nrings);
  data_ptr += 4; /* Skip past the nrings. */

  ordinate_ptr = data_ptr; /* Start the ordinate pointer. */
  if ( nrings > 0)
  {
    poly->rings = (RTPOINTARRAY**)rtalloc(ctx,  sizeof(RTPOINTARRAY*) * nrings );
    ordinate_ptr += nrings * 4; /* Move past all the npoints values. */
    if ( nrings % 2 ) /* If there is padding, move past that too. */
      ordinate_ptr += 4;
  }
  else /* Empty polygon */
  {
    poly->rings = NULL;
  }

  for ( i = 0; i < nrings; i++ )
  {
    uint32_t npoints = 0;

    /* Read in the number of points. */
    npoints = rt_get_uint32_t(ctx, data_ptr);
    data_ptr += 4;

    /* Make a point array for the ring, and move the ordinate pointer past the ring ordinates. */
    poly->rings[i] = ptarray_construct_reference_data(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), npoints, ordinate_ptr);

    ordinate_ptr += sizeof(double) * RTFLAGS_NDIMS(g_flags) * npoints;
  }

  if ( g_size )
    *g_size = ordinate_ptr - start_ptr;

  return poly;
}

static RTTRIANGLE* rttriangle_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint8_t *start_ptr = data_ptr;
  RTTRIANGLE *triangle;
  uint32_t npoints = 0;

  assert(data_ptr);

  triangle = (RTTRIANGLE*)rtalloc(ctx, sizeof(RTTRIANGLE));
  triangle->srid = SRID_UNKNOWN; /* Default */
  triangle->bbox = NULL;
  triangle->type = RTTRIANGLETYPE;
  triangle->flags = g_flags;

  data_ptr += 4; /* Skip past the type. */
  npoints = rt_get_uint32_t(ctx, data_ptr); /* Zero => empty geometry */
  data_ptr += 4; /* Skip past the npoints. */

  if ( npoints > 0 )
    triangle->points = ptarray_construct_reference_data(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), npoints, data_ptr);
  else
    triangle->points = ptarray_construct(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), 0); /* Empty triangle */

  data_ptr += RTFLAGS_NDIMS(g_flags) * npoints * sizeof(double);

  if ( g_size )
    *g_size = data_ptr - start_ptr;

  return triangle;
}

static RTCIRCSTRING* rtcircstring_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint8_t *start_ptr = data_ptr;
  RTCIRCSTRING *circstring;
  uint32_t npoints = 0;

  assert(data_ptr);

  circstring = (RTCIRCSTRING*)rtalloc(ctx, sizeof(RTCIRCSTRING));
  circstring->srid = SRID_UNKNOWN; /* Default */
  circstring->bbox = NULL;
  circstring->type = RTCIRCSTRINGTYPE;
  circstring->flags = g_flags;

  data_ptr += 4; /* Skip past the circstringtype. */
  npoints = rt_get_uint32_t(ctx, data_ptr); /* Zero => empty geometry */
  data_ptr += 4; /* Skip past the npoints. */

  if ( npoints > 0 )
    circstring->points = ptarray_construct_reference_data(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), npoints, data_ptr);
  else
    circstring->points = ptarray_construct(ctx, RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), 0); /* Empty circularstring */

  data_ptr += RTFLAGS_NDIMS(g_flags) * npoints * sizeof(double);

  if ( g_size )
    *g_size = data_ptr - start_ptr;

  return circstring;
}

static RTCOLLECTION* rtcollection_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint32_t type;
  uint8_t *start_ptr = data_ptr;
  RTCOLLECTION *collection;
  uint32_t ngeoms = 0;
  int i = 0;

  assert(data_ptr);

  type = rt_get_uint32_t(ctx, data_ptr);
  data_ptr += 4; /* Skip past the type. */

  collection = (RTCOLLECTION*)rtalloc(ctx, sizeof(RTCOLLECTION));
  collection->srid = SRID_UNKNOWN; /* Default */
  collection->bbox = NULL;
  collection->type = type;
  collection->flags = g_flags;

  ngeoms = rt_get_uint32_t(ctx, data_ptr);
  collection->ngeoms = ngeoms; /* Zero => empty geometry */
  data_ptr += 4; /* Skip past the ngeoms. */

  if ( ngeoms > 0 )
    collection->geoms = rtalloc(ctx, sizeof(RTGEOM*) * ngeoms);
  else
    collection->geoms = NULL;

  /* Sub-geometries are never de-serialized with boxes (#1254) */
  RTFLAGS_SET_BBOX(g_flags, 0);

  for ( i = 0; i < ngeoms; i++ )
  {
    uint32_t subtype = rt_get_uint32_t(ctx, data_ptr);
    size_t subsize = 0;

    if ( ! rtcollection_allows_subtype(ctx, type, subtype) )
    {
      rterror(ctx, "Invalid subtype (%s) for collection type (%s)", rttype_name(ctx, subtype), rttype_name(ctx, type));
      rtfree(ctx, collection);
      return NULL;
    }
    collection->geoms[i] = rtgeom_from_gserialized_buffer(ctx, data_ptr, g_flags, &subsize);
    data_ptr += subsize;
  }

  if ( g_size )
    *g_size = data_ptr - start_ptr;

  return collection;
}

RTGEOM* rtgeom_from_gserialized_buffer(const RTCTX *ctx, uint8_t *data_ptr, uint8_t g_flags, size_t *g_size)
{
  uint32_t type;

  assert(data_ptr);

  type = rt_get_uint32_t(ctx, data_ptr);

  RTDEBUGF(ctx, 2, "Got type %d (%s), hasz=%d hasm=%d geodetic=%d hasbox=%d", type, rttype_name(ctx, type),
    RTFLAGS_GET_Z(g_flags), RTFLAGS_GET_M(g_flags), RTFLAGS_GET_GEODETIC(g_flags), RTFLAGS_GET_BBOX(g_flags));

  switch (type)
  {
  case RTPOINTTYPE:
    return (RTGEOM *)rtpoint_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  case RTLINETYPE:
    return (RTGEOM *)rtline_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  case RTCIRCSTRINGTYPE:
    return (RTGEOM *)rtcircstring_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  case RTPOLYGONTYPE:
    return (RTGEOM *)rtpoly_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  case RTTRIANGLETYPE:
    return (RTGEOM *)rttriangle_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOMPOUNDTYPE:
  case RTCURVEPOLYTYPE:
  case RTMULTICURVETYPE:
  case RTMULTISURFACETYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
  case RTCOLLECTIONTYPE:
    return (RTGEOM *)rtcollection_from_gserialized_buffer(ctx, data_ptr, g_flags, g_size);
  default:
    rterror(ctx, "Unknown geometry type: %d - %s", type, rttype_name(ctx, type));
    return NULL;
  }
}

RTGEOM* rtgeom_from_gserialized(const RTCTX *ctx, const GSERIALIZED *g)
{
  uint8_t g_flags = 0;
  int32_t g_srid = 0;
  uint32_t g_type = 0;
  uint8_t *data_ptr = NULL;
  RTGEOM *rtgeom = NULL;
  RTGBOX bbox;
  size_t g_size = 0;

  assert(g);

  g_srid = gserialized_get_srid(ctx, g);
  g_flags = g->flags;
  g_type = gserialized_get_type(ctx, g);
  RTDEBUGF(ctx, 4, "Got type %d (%s), srid=%d", g_type, rttype_name(ctx, g_type), g_srid);

  data_ptr = (uint8_t*)g->data;
  if ( RTFLAGS_GET_BBOX(g_flags) )
    data_ptr += gbox_serialized_size(ctx, g_flags);

  rtgeom = rtgeom_from_gserialized_buffer(ctx, data_ptr, g_flags, &g_size);

  if ( ! rtgeom )
    rterror(ctx, "rtgeom_from_gserialized: unable create geometry"); /* Ooops! */

  rtgeom->type = g_type;
  rtgeom->flags = g_flags;

  if ( gserialized_read_gbox_p(ctx, g, &bbox) == RT_SUCCESS )
  {
    rtgeom->bbox = gbox_copy(ctx, &bbox);
  }
  else if ( rtgeom_needs_bbox(ctx, rtgeom) && (rtgeom_calculate_gbox(ctx, rtgeom, &bbox) == RT_SUCCESS) )
  {
    rtgeom->bbox = gbox_copy(ctx, &bbox);
  }
  else
  {
    rtgeom->bbox = NULL;
  }

  rtgeom_set_srid(ctx, rtgeom, g_srid);

  return rtgeom;
}

