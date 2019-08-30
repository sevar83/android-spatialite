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
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009-2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include <string.h>  /* strlen */
#include <assert.h>

static char * asgeojson_point(const RTCTX *ctx, const RTPOINT *point, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_line(const RTCTX *ctx, const RTLINE *line, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_poly(const RTCTX *ctx, const RTPOLY *poly, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_multipoint(const RTCTX *ctx, const RTMPOINT *mpoint, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_multiline(const RTCTX *ctx, const RTMLINE *mline, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_multipolygon(const RTCTX *ctx, const RTMPOLY *mpoly, char *srs, RTGBOX *bbox, int precision);
static char * asgeojson_collection(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, RTGBOX *bbox, int precision);
static size_t asgeojson_geom_size(const RTCTX *ctx, const RTGEOM *geom, RTGBOX *bbox, int precision);
static size_t asgeojson_geom_buf(const RTCTX *ctx, const RTGEOM *geom, char *output, RTGBOX *bbox, int precision);

static size_t pointArray_to_geojson(const RTCTX *ctx, RTPOINTARRAY *pa, char *buf, int precision);
static size_t pointArray_geojson_size(const RTCTX *ctx, RTPOINTARRAY *pa, int precision);

/**
 * Takes a GEOMETRY and returns a GeoJson representation
 */
char *
rtgeom_to_geojson(const RTCTX *ctx, const RTGEOM *geom, char *srs, int precision, int has_bbox)
{
  int type = geom->type;
  RTGBOX *bbox = NULL;
  RTGBOX tmp;

  if ( precision > OUT_MAX_DOUBLE_PRECISION ) precision = OUT_MAX_DOUBLE_PRECISION;

  if (has_bbox)
  {
    /* Whether these are geography or geometry,
       the GeoJSON expects a cartesian bounding box */
    rtgeom_calculate_gbox_cartesian(ctx, geom, &tmp);
    bbox = &tmp;
  }

  switch (type)
  {
  case RTPOINTTYPE:
    return asgeojson_point(ctx, (RTPOINT*)geom, srs, bbox, precision);
  case RTLINETYPE:
    return asgeojson_line(ctx, (RTLINE*)geom, srs, bbox, precision);
  case RTPOLYGONTYPE:
    return asgeojson_poly(ctx, (RTPOLY*)geom, srs, bbox, precision);
  case RTMULTIPOINTTYPE:
    return asgeojson_multipoint(ctx, (RTMPOINT*)geom, srs, bbox, precision);
  case RTMULTILINETYPE:
    return asgeojson_multiline(ctx, (RTMLINE*)geom, srs, bbox, precision);
  case RTMULTIPOLYGONTYPE:
    return asgeojson_multipolygon(ctx, (RTMPOLY*)geom, srs, bbox, precision);
  case RTCOLLECTIONTYPE:
    return asgeojson_collection(ctx, (RTCOLLECTION*)geom, srs, bbox, precision);
  default:
    rterror(ctx, "rtgeom_to_geojson: '%s' geometry type not supported",
            rttype_name(ctx, type));
  }

  /* Never get here */
  return NULL;
}



/**
 * Handle SRS
 */
static size_t
asgeojson_srs_size(const RTCTX *ctx, char *srs)
{
  int size;

  size = sizeof("'crs':{'type':'name',");
  size += sizeof("'properties':{'name':''}},");
  size += strlen(srs) * sizeof(char);

  return size;
}

static size_t
asgeojson_srs_buf(const RTCTX *ctx, char *output, char *srs)
{
  char *ptr = output;

  ptr += sprintf(ptr, "\"crs\":{\"type\":\"name\",");
  ptr += sprintf(ptr, "\"properties\":{\"name\":\"%s\"}},", srs);

  return (ptr-output);
}



/**
 * Handle Bbox
 */
static size_t
asgeojson_bbox_size(const RTCTX *ctx, int hasz, int precision)
{
  int size;

  if (!hasz)
  {
    size = sizeof("\"bbox\":[,,,],");
    size +=  2 * 2 * (OUT_MAX_DIGS_DOUBLE + precision);
  }
  else
  {
    size = sizeof("\"bbox\":[,,,,,],");
    size +=  2 * 3 * (OUT_MAX_DIGS_DOUBLE + precision);
  }

  return size;
}

static size_t
asgeojson_bbox_buf(const RTCTX *ctx, char *output, RTGBOX *bbox, int hasz, int precision)
{
  char *ptr = output;

  if (!hasz)
    ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f],",
                   precision, bbox->xmin, precision, bbox->ymin,
                   precision, bbox->xmax, precision, bbox->ymax);
  else
    ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f,%.*f,%.*f],",
                   precision, bbox->xmin, precision, bbox->ymin, precision, bbox->zmin,
                   precision, bbox->xmax, precision, bbox->ymax, precision, bbox->zmax);

  return (ptr-output);
}



/**
 * Point Geometry
 */

static size_t
asgeojson_point_size(const RTCTX *ctx, const RTPOINT *point, char *srs, RTGBOX *bbox, int precision)
{
  int size;

  size = pointArray_geojson_size(ctx, point->point, precision);
  size += sizeof("{'type':'Point',");
  size += sizeof("'coordinates':}");

  if ( rtpoint_is_empty(ctx, point) )
    size += 2; /* [] */

  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(point->flags), precision);

  return size;
}

static size_t
asgeojson_point_buf(const RTCTX *ctx, const RTPOINT *point, char *srs, char *output, RTGBOX *bbox, int precision)
{
  char *ptr = output;

  ptr += sprintf(ptr, "{\"type\":\"Point\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(point->flags), precision);

  ptr += sprintf(ptr, "\"coordinates\":");
  if ( rtpoint_is_empty(ctx, point) )
    ptr += sprintf(ptr, "[]");
  ptr += pointArray_to_geojson(ctx, point->point, ptr, precision);
  ptr += sprintf(ptr, "}");

  return (ptr-output);
}

static char *
asgeojson_point(const RTCTX *ctx, const RTPOINT *point, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_point_size(ctx, point, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_point_buf(ctx, point, srs, output, bbox, precision);
  return output;
}



/**
 * Line Geometry
 */

static size_t
asgeojson_line_size(const RTCTX *ctx, const RTLINE *line, char *srs, RTGBOX *bbox, int precision)
{
  int size;

  size = sizeof("{'type':'LineString',");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(line->flags), precision);
  size += sizeof("'coordinates':[]}");
  size += pointArray_geojson_size(ctx, line->points, precision);

  return size;
}

static size_t
asgeojson_line_buf(const RTCTX *ctx, const RTLINE *line, char *srs, char *output, RTGBOX *bbox, int precision)
{
  char *ptr=output;

  ptr += sprintf(ptr, "{\"type\":\"LineString\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(line->flags), precision);
  ptr += sprintf(ptr, "\"coordinates\":[");
  ptr += pointArray_to_geojson(ctx, line->points, ptr, precision);
  ptr += sprintf(ptr, "]}");

  return (ptr-output);
}

static char *
asgeojson_line(const RTCTX *ctx, const RTLINE *line, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_line_size(ctx, line, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_line_buf(ctx, line, srs, output, bbox, precision);

  return output;
}



/**
 * Polygon Geometry
 */

static size_t
asgeojson_poly_size(const RTCTX *ctx, const RTPOLY *poly, char *srs, RTGBOX *bbox, int precision)
{
  size_t size;
  int i;

  size = sizeof("{\"type\":\"Polygon\",");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(poly->flags), precision);
  size += sizeof("\"coordinates\":[");
  for (i=0, size=0; i<poly->nrings; i++)
  {
    size += pointArray_geojson_size(ctx, poly->rings[i], precision);
    size += sizeof("[]");
  }
  size += sizeof(",") * i;
  size += sizeof("]}");

  return size;
}

static size_t
asgeojson_poly_buf(const RTCTX *ctx, const RTPOLY *poly, char *srs, char *output, RTGBOX *bbox, int precision)
{
  int i;
  char *ptr=output;

  ptr += sprintf(ptr, "{\"type\":\"Polygon\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(poly->flags), precision);
  ptr += sprintf(ptr, "\"coordinates\":[");
  for (i=0; i<poly->nrings; i++)
  {
    if (i) ptr += sprintf(ptr, ",");
    ptr += sprintf(ptr, "[");
    ptr += pointArray_to_geojson(ctx, poly->rings[i], ptr, precision);
    ptr += sprintf(ptr, "]");
  }
  ptr += sprintf(ptr, "]}");

  return (ptr-output);
}

static char *
asgeojson_poly(const RTCTX *ctx, const RTPOLY *poly, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_poly_size(ctx, poly, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_poly_buf(ctx, poly, srs, output, bbox, precision);

  return output;
}



/**
 * Multipoint Geometry
 */

static size_t
asgeojson_multipoint_size(const RTCTX *ctx, const RTMPOINT *mpoint, char *srs, RTGBOX *bbox, int precision)
{
  RTPOINT * point;
  int size;
  int i;

  size = sizeof("{'type':'MultiPoint',");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(mpoint->flags), precision);
  size += sizeof("'coordinates':[]}");

  for (i=0; i<mpoint->ngeoms; i++)
  {
    point = mpoint->geoms[i];
    size += pointArray_geojson_size(ctx, point->point, precision);
  }
  size += sizeof(",") * i;

  return size;
}

static size_t
asgeojson_multipoint_buf(const RTCTX *ctx, const RTMPOINT *mpoint, char *srs, char *output, RTGBOX *bbox, int precision)
{
  RTPOINT *point;
  int i;
  char *ptr=output;

  ptr += sprintf(ptr, "{\"type\":\"MultiPoint\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(mpoint->flags), precision);
  ptr += sprintf(ptr, "\"coordinates\":[");

  for (i=0; i<mpoint->ngeoms; i++)
  {
    if (i) ptr += sprintf(ptr, ",");
    point = mpoint->geoms[i];
    ptr += pointArray_to_geojson(ctx, point->point, ptr, precision);
  }
  ptr += sprintf(ptr, "]}");

  return (ptr - output);
}

static char *
asgeojson_multipoint(const RTCTX *ctx, const RTMPOINT *mpoint, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_multipoint_size(ctx, mpoint, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_multipoint_buf(ctx, mpoint, srs, output, bbox, precision);

  return output;
}



/**
 * Multiline Geometry
 */

static size_t
asgeojson_multiline_size(const RTCTX *ctx, const RTMLINE *mline, char *srs, RTGBOX *bbox, int precision)
{
  RTLINE * line;
  int size;
  int i;

  size = sizeof("{'type':'MultiLineString',");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(mline->flags), precision);
  size += sizeof("'coordinates':[]}");

  for (i=0 ; i<mline->ngeoms; i++)
  {
    line = mline->geoms[i];
    size += pointArray_geojson_size(ctx, line->points, precision);
    size += sizeof("[]");
  }
  size += sizeof(",") * i;

  return size;
}

static size_t
asgeojson_multiline_buf(const RTCTX *ctx, const RTMLINE *mline, char *srs, char *output, RTGBOX *bbox, int precision)
{
  RTLINE *line;
  int i;
  char *ptr=output;

  ptr += sprintf(ptr, "{\"type\":\"MultiLineString\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(mline->flags), precision);
  ptr += sprintf(ptr, "\"coordinates\":[");

  for (i=0; i<mline->ngeoms; i++)
  {
    if (i) ptr += sprintf(ptr, ",");
    ptr += sprintf(ptr, "[");
    line = mline->geoms[i];
    ptr += pointArray_to_geojson(ctx, line->points, ptr, precision);
    ptr += sprintf(ptr, "]");
  }

  ptr += sprintf(ptr, "]}");

  return (ptr - output);
}

static char *
asgeojson_multiline(const RTCTX *ctx, const RTMLINE *mline, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_multiline_size(ctx, mline, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_multiline_buf(ctx, mline, srs, output, bbox, precision);

  return output;
}



/**
 * MultiPolygon Geometry
 */

static size_t
asgeojson_multipolygon_size(const RTCTX *ctx, const RTMPOLY *mpoly, char *srs, RTGBOX *bbox, int precision)
{
  RTPOLY *poly;
  int size;
  int i, j;

  size = sizeof("{'type':'MultiPolygon',");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(mpoly->flags), precision);
  size += sizeof("'coordinates':[]}");

  for (i=0; i < mpoly->ngeoms; i++)
  {
    poly = mpoly->geoms[i];
    for (j=0 ; j <poly->nrings ; j++)
    {
      size += pointArray_geojson_size(ctx, poly->rings[j], precision);
      size += sizeof("[]");
    }
    size += sizeof("[]");
  }
  size += sizeof(",") * i;
  size += sizeof("]}");

  return size;
}

static size_t
asgeojson_multipolygon_buf(const RTCTX *ctx, const RTMPOLY *mpoly, char *srs, char *output, RTGBOX *bbox, int precision)
{
  RTPOLY *poly;
  int i, j;
  char *ptr=output;

  ptr += sprintf(ptr, "{\"type\":\"MultiPolygon\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(mpoly->flags), precision);
  ptr += sprintf(ptr, "\"coordinates\":[");
  for (i=0; i<mpoly->ngeoms; i++)
  {
    if (i) ptr += sprintf(ptr, ",");
    ptr += sprintf(ptr, "[");
    poly = mpoly->geoms[i];
    for (j=0 ; j < poly->nrings ; j++)
    {
      if (j) ptr += sprintf(ptr, ",");
      ptr += sprintf(ptr, "[");
      ptr += pointArray_to_geojson(ctx, poly->rings[j], ptr, precision);
      ptr += sprintf(ptr, "]");
    }
    ptr += sprintf(ptr, "]");
  }
  ptr += sprintf(ptr, "]}");

  return (ptr - output);
}

static char *
asgeojson_multipolygon(const RTCTX *ctx, const RTMPOLY *mpoly, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_multipolygon_size(ctx, mpoly, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_multipolygon_buf(ctx, mpoly, srs, output, bbox, precision);

  return output;
}



/**
 * Collection Geometry
 */

static size_t
asgeojson_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, RTGBOX *bbox, int precision)
{
  int i;
  int size;
  RTGEOM *subgeom;

  size = sizeof("{'type':'GeometryCollection',");
  if (srs) size += asgeojson_srs_size(ctx, srs);
  if (bbox) size += asgeojson_bbox_size(ctx, RTFLAGS_GET_Z(col->flags), precision);
  size += sizeof("'geometries':");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    size += asgeojson_geom_size(ctx, subgeom, NULL, precision);
  }
  size += sizeof(",") * i;
  size += sizeof("]}");

  return size;
}

static size_t
asgeojson_collection_buf(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, char *output, RTGBOX *bbox, int precision)
{
  int i;
  char *ptr=output;
  RTGEOM *subgeom;

  ptr += sprintf(ptr, "{\"type\":\"GeometryCollection\",");
  if (srs) ptr += asgeojson_srs_buf(ctx, ptr, srs);
  if (col->ngeoms && bbox) ptr += asgeojson_bbox_buf(ctx, ptr, bbox, RTFLAGS_GET_Z(col->flags), precision);
  ptr += sprintf(ptr, "\"geometries\":[");

  for (i=0; i<col->ngeoms; i++)
  {
    if (i) ptr += sprintf(ptr, ",");
    subgeom = col->geoms[i];
    ptr += asgeojson_geom_buf(ctx, subgeom, ptr, NULL, precision);
  }

  ptr += sprintf(ptr, "]}");

  return (ptr - output);
}

static char *
asgeojson_collection(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, RTGBOX *bbox, int precision)
{
  char *output;
  int size;

  size = asgeojson_collection_size(ctx, col, srs, bbox, precision);
  output = rtalloc(ctx, size);
  asgeojson_collection_buf(ctx, col, srs, output, bbox, precision);

  return output;
}



static size_t
asgeojson_geom_size(const RTCTX *ctx, const RTGEOM *geom, RTGBOX *bbox, int precision)
{
  int type = geom->type;
  size_t size = 0;

  switch (type)
  {
  case RTPOINTTYPE:
    size = asgeojson_point_size(ctx, (RTPOINT*)geom, NULL, bbox, precision);
    break;

  case RTLINETYPE:
    size = asgeojson_line_size(ctx, (RTLINE*)geom, NULL, bbox, precision);
    break;

  case RTPOLYGONTYPE:
    size = asgeojson_poly_size(ctx, (RTPOLY*)geom, NULL, bbox, precision);
    break;

  case RTMULTIPOINTTYPE:
    size = asgeojson_multipoint_size(ctx, (RTMPOINT*)geom, NULL, bbox, precision);
    break;

  case RTMULTILINETYPE:
    size = asgeojson_multiline_size(ctx, (RTMLINE*)geom, NULL, bbox, precision);
    break;

  case RTMULTIPOLYGONTYPE:
    size = asgeojson_multipolygon_size(ctx, (RTMPOLY*)geom, NULL, bbox, precision);
    break;

  default:
    rterror(ctx, "GeoJson: geometry not supported.");
  }

  return size;
}


static size_t
asgeojson_geom_buf(const RTCTX *ctx, const RTGEOM *geom, char *output, RTGBOX *bbox, int precision)
{
  int type = geom->type;
  char *ptr=output;

  switch (type)
  {
  case RTPOINTTYPE:
    ptr += asgeojson_point_buf(ctx, (RTPOINT*)geom, NULL, ptr, bbox, precision);
    break;

  case RTLINETYPE:
    ptr += asgeojson_line_buf(ctx, (RTLINE*)geom, NULL, ptr, bbox, precision);
    break;

  case RTPOLYGONTYPE:
    ptr += asgeojson_poly_buf(ctx, (RTPOLY*)geom, NULL, ptr, bbox, precision);
    break;

  case RTMULTIPOINTTYPE:
    ptr += asgeojson_multipoint_buf(ctx, (RTMPOINT*)geom, NULL, ptr, bbox, precision);
    break;

  case RTMULTILINETYPE:
    ptr += asgeojson_multiline_buf(ctx, (RTMLINE*)geom, NULL, ptr, bbox, precision);
    break;

  case RTMULTIPOLYGONTYPE:
    ptr += asgeojson_multipolygon_buf(ctx, (RTMPOLY*)geom, NULL, ptr, bbox, precision);
    break;

  default:
    if (bbox) rtfree(ctx, bbox);
    rterror(ctx, "GeoJson: geometry not supported.");
  }

  return (ptr-output);
}

/*
 * Print an ordinate value using at most the given number of decimal digits
 *
 * The actual number of printed decimal digits may be less than the
 * requested ones if out of significant digits.
 *
 * The function will not write more than maxsize bytes, including the
 * terminating NULL. Returns the number of bytes that would have been
 * written if there was enough space (excluding terminating NULL).
 * So a return of ``bufsize'' or more means that the string was
 * truncated and misses a terminating NULL.
 *
 * TODO: export ?
 *
 */
static int
rtprint_double(const RTCTX *ctx, double d, int maxdd, char *buf, size_t bufsize)
{
  double ad = fabs(d);
  int ndd = ad < 1 ? 0 : floor(log10(ad))+1; /* non-decimal digits */
  if (fabs(d) < OUT_MAX_DOUBLE)
  {
    if ( maxdd > (OUT_MAX_DOUBLE_PRECISION - ndd) )  maxdd -= ndd;
    return snprintf(buf, bufsize, "%.*f", maxdd, d);
  }
  else
  {
    return snprintf(buf, bufsize, "%g", d);
  }
}



static size_t
pointArray_to_geojson(const RTCTX *ctx, RTPOINTARRAY *pa, char *output, int precision)
{
  int i;
  char *ptr;
#define BUFSIZE OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION
  char x[BUFSIZE+1];
  char y[BUFSIZE+1];
  char z[BUFSIZE+1];

  assert ( precision <= OUT_MAX_DOUBLE_PRECISION );

  /* Ensure a terminating NULL at the end of buffers
   * so that we don't need to check for truncation
   * inprint_double */
  x[BUFSIZE] = '\0';
  y[BUFSIZE] = '\0';
  z[BUFSIZE] = '\0';

  ptr = output;

  /* TODO: rewrite this loop to be simpler and possibly quicker */
  if (!RTFLAGS_GET_Z(pa->flags))
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT2D *pt;
      pt = rt_getPoint2d_cp(ctx, pa, i);

      rtprint_double(ctx, pt->x, precision, x, BUFSIZE);
      trim_trailing_zeros(ctx, x);
      rtprint_double(ctx, pt->y, precision, y, BUFSIZE);
      trim_trailing_zeros(ctx, y);

      if ( i ) ptr += sprintf(ptr, ",");
      ptr += sprintf(ptr, "[%s,%s]", x, y);
    }
  }
  else
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT3DZ *pt;
      pt = rt_getPoint3dz_cp(ctx, pa, i);

      rtprint_double(ctx, pt->x, precision, x, BUFSIZE);
      trim_trailing_zeros(ctx, x);
      rtprint_double(ctx, pt->y, precision, y, BUFSIZE);
      trim_trailing_zeros(ctx, y);
      rtprint_double(ctx, pt->z, precision, z, BUFSIZE);
      trim_trailing_zeros(ctx, z);

      if ( i ) ptr += sprintf(ptr, ",");
      ptr += sprintf(ptr, "[%s,%s,%s]", x, y, z);
    }
  }

  return (ptr-output);
}



/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_geojson_size(const RTCTX *ctx, RTPOINTARRAY *pa, int precision)
{
  assert ( precision <= OUT_MAX_DOUBLE_PRECISION );
  if (RTFLAGS_NDIMS(pa->flags) == 2)
    return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(","))
           * 2 * pa->npoints + sizeof(",[]");

  return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(",,"))
         * 3 * pa->npoints + sizeof(",[]");
}
