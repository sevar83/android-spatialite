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
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <math.h>

#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"

static uint8_t* rtgeom_to_wkb_buf(const RTCTX *ctx, const RTGEOM *geom, uint8_t *buf, uint8_t variant);
static size_t rtgeom_to_wkb_size(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant);

/*
* Look-up table for hex writer
*/
static char *hexchr = "0123456789ABCDEF";

char* hexbytes_from_bytes(const RTCTX *ctx, uint8_t *bytes, size_t size)
{
  char *hex;
  int i;
  if ( ! bytes || ! size )
  {
    rterror(ctx, "hexbutes_from_bytes: invalid input");
    return NULL;
  }
  hex = rtalloc(ctx, size * 2 + 1);
  hex[2*size] = '\0';
  for( i = 0; i < size; i++ )
  {
    /* Top four bits to 0-F */
    hex[2*i] = hexchr[bytes[i] >> 4];
    /* Bottom four bits to 0-F */
    hex[2*i+1] = hexchr[bytes[i] & 0x0F];
  }
  return hex;
}

/*
* Optional SRID
*/
static int rtgeom_wkb_needs_srid(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant)
{
  /* Sub-components of collections inherit their SRID from the parent.
     We force that behavior with the RTWKB_NO_SRID flag */
  if ( variant & RTWKB_NO_SRID )
    return RT_FALSE;

  /* We can only add an SRID if the geometry has one, and the
     RTWKB form is extended */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_has_srid(ctx, geom) )
    return RT_TRUE;

  /* Everything else doesn't get an SRID */
  return RT_FALSE;
}

/*
* GeometryType
*/
static uint32_t rtgeom_wkb_type(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant)
{
  uint32_t wkb_type = 0;

  switch ( geom->type )
  {
  case RTPOINTTYPE:
    wkb_type = RTWKB_POINT_TYPE;
    break;
  case RTLINETYPE:
    wkb_type = RTWKB_LINESTRING_TYPE;
    break;
  case RTPOLYGONTYPE:
    wkb_type = RTWKB_POLYGON_TYPE;
    break;
  case RTMULTIPOINTTYPE:
    wkb_type = RTWKB_MULTIPOINT_TYPE;
    break;
  case RTMULTILINETYPE:
    wkb_type = RTWKB_MULTILINESTRING_TYPE;
    break;
  case RTMULTIPOLYGONTYPE:
    wkb_type = RTWKB_MULTIPOLYGON_TYPE;
    break;
  case RTCOLLECTIONTYPE:
    wkb_type = RTWKB_GEOMETRYCOLLECTION_TYPE;
    break;
  case RTCIRCSTRINGTYPE:
    wkb_type = RTWKB_CIRCULARSTRING_TYPE;
    break;
  case RTCOMPOUNDTYPE:
    wkb_type = RTWKB_COMPOUNDCURVE_TYPE;
    break;
  case RTCURVEPOLYTYPE:
    wkb_type = RTWKB_CURVEPOLYGON_TYPE;
    break;
  case RTMULTICURVETYPE:
    wkb_type = RTWKB_MULTICURVE_TYPE;
    break;
  case RTMULTISURFACETYPE:
    wkb_type = RTWKB_MULTISURFACE_TYPE;
    break;
  case RTPOLYHEDRALSURFACETYPE:
    wkb_type = RTWKB_POLYHEDRALSURFACE_TYPE;
    break;
  case RTTINTYPE:
    wkb_type = RTWKB_TIN_TYPE;
    break;
  case RTTRIANGLETYPE:
    wkb_type = RTWKB_TRIANGLE_TYPE;
    break;
  default:
    rterror(ctx, "Unsupported geometry type: %s [%d]",
      rttype_name(ctx, geom->type), geom->type);
  }

  if ( variant & RTWKB_EXTENDED )
  {
    if ( RTFLAGS_GET_Z(geom->flags) )
      wkb_type |= RTWKBZOFFSET;
    if ( RTFLAGS_GET_M(geom->flags) )
      wkb_type |= RTWKBMOFFSET;
/*    if ( geom->srid != SRID_UNKNOWN && ! (variant & RTWKB_NO_SRID) ) */
    if ( rtgeom_wkb_needs_srid(ctx, geom, variant) )
      wkb_type |= RTWKBSRIDFLAG;
  }
  else if ( variant & RTWKB_ISO )
  {
    /* Z types are in the 1000 range */
    if ( RTFLAGS_GET_Z(geom->flags) )
      wkb_type += 1000;
    /* M types are in the 2000 range */
    if ( RTFLAGS_GET_M(geom->flags) )
      wkb_type += 2000;
    /* ZM types are in the 1000 + 2000 = 3000 range, see above */
  }
  return wkb_type;
}

/*
* Endian
*/
static uint8_t* endian_to_wkb_buf(const RTCTX *ctx, uint8_t *buf, uint8_t variant)
{
  if ( variant & RTWKB_HEX )
  {
    buf[0] = '0';
    buf[1] = ((variant & RTWKB_NDR) ? '1' : '0');
    return buf + 2;
  }
  else
  {
    buf[0] = ((variant & RTWKB_NDR) ? 1 : 0);
    return buf + 1;
  }
}

/*
* SwapBytes?
*/
static inline int wkb_swap_bytes(const RTCTX *ctx, uint8_t variant)
{
  /* If requested variant matches machine arch, we don't have to swap! */
  if ( ((variant & RTWKB_NDR) && (getMachineEndian(ctx) == NDR)) ||
       ((! (variant & RTWKB_NDR)) && (getMachineEndian(ctx) == XDR)) )
  {
    return RT_FALSE;
  }
  return RT_TRUE;
}

/*
* Integer32
*/
static uint8_t* integer_to_wkb_buf(const RTCTX *ctx, const int ival, uint8_t *buf, uint8_t variant)
{
  char *iptr = (char*)(&ival);
  int i = 0;

  if ( sizeof(int) != RTWKB_INT_SIZE )
  {
    rterror(ctx, "Machine int size is not %d bytes!", RTWKB_INT_SIZE);
  }
  RTDEBUGF(ctx, 4, "Writing value '%u'", ival);
  if ( variant & RTWKB_HEX )
  {
    int swap = wkb_swap_bytes(ctx, variant);
    /* Machine/request arch mismatch, so flip byte order */
    for ( i = 0; i < RTWKB_INT_SIZE; i++ )
    {
      int j = (swap ? RTWKB_INT_SIZE - 1 - i : i);
      uint8_t b = iptr[j];
      /* Top four bits to 0-F */
      buf[2*i] = hexchr[b >> 4];
      /* Bottom four bits to 0-F */
      buf[2*i+1] = hexchr[b & 0x0F];
    }
    return buf + (2 * RTWKB_INT_SIZE);
  }
  else
  {
    /* Machine/request arch mismatch, so flip byte order */
    if ( wkb_swap_bytes(ctx, variant) )
    {
      for ( i = 0; i < RTWKB_INT_SIZE; i++ )
      {
        buf[i] = iptr[RTWKB_INT_SIZE - 1 - i];
      }
    }
    /* If machine arch and requested arch match, don't flip byte order */
    else
    {
      memcpy(buf, iptr, RTWKB_INT_SIZE);
    }
    return buf + RTWKB_INT_SIZE;
  }
}

/*
* Float64
*/
static uint8_t* double_to_wkb_buf(const RTCTX *ctx, const double d, uint8_t *buf, uint8_t variant)
{
  char *dptr = (char*)(&d);
  int i = 0;

  if ( sizeof(double) != RTWKB_DOUBLE_SIZE )
  {
    rterror(ctx, "Machine double size is not %d bytes!", RTWKB_DOUBLE_SIZE);
  }

  if ( variant & RTWKB_HEX )
  {
    int swap =  wkb_swap_bytes(ctx, variant);
    /* Machine/request arch mismatch, so flip byte order */
    for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
    {
      int j = (swap ? RTWKB_DOUBLE_SIZE - 1 - i : i);
      uint8_t b = dptr[j];
      /* Top four bits to 0-F */
      buf[2*i] = hexchr[b >> 4];
      /* Bottom four bits to 0-F */
      buf[2*i+1] = hexchr[b & 0x0F];
    }
    return buf + (2 * RTWKB_DOUBLE_SIZE);
  }
  else
  {
    /* Machine/request arch mismatch, so flip byte order */
    if ( wkb_swap_bytes(ctx, variant) )
    {
      for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
      {
        buf[i] = dptr[RTWKB_DOUBLE_SIZE - 1 - i];
      }
    }
    /* If machine arch and requested arch match, don't flip byte order */
    else
    {
      memcpy(buf, dptr, RTWKB_DOUBLE_SIZE);
    }
    return buf + RTWKB_DOUBLE_SIZE;
  }
}


/*
* Empty
*/
static size_t empty_to_wkb_size(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant)
{
  /* endian byte + type integer */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

  /* optional srid integer */
  if ( rtgeom_wkb_needs_srid(ctx, geom, variant) )
    size += RTWKB_INT_SIZE;

  /* Represent POINT EMPTY as POINT(NaN NaN) */
  if ( geom->type == RTPOINTTYPE )
  {
    const RTPOINT *pt = (RTPOINT*)geom;
    size += RTWKB_DOUBLE_SIZE * RTFLAGS_NDIMS(pt->point->flags);
  }
  /* num-elements */
  else
  {
    size += RTWKB_INT_SIZE;
  }

  return size;
}

static uint8_t* empty_to_wkb_buf(const RTCTX *ctx, const RTGEOM *geom, uint8_t *buf, uint8_t variant)
{
  uint32_t wkb_type = rtgeom_wkb_type(ctx, geom, variant);

  /* Set the endian flag */
  buf = endian_to_wkb_buf(ctx, buf, variant);

  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, wkb_type, buf, variant);

  /* Set the SRID if necessary */
  if ( rtgeom_wkb_needs_srid(ctx, geom, variant) )
    buf = integer_to_wkb_buf(ctx, geom->srid, buf, variant);

  /* Represent POINT EMPTY as POINT(NaN NaN) */
  if ( geom->type == RTPOINTTYPE )
  {
    const RTPOINT *pt = (RTPOINT*)geom;
    static double nn = NAN;
    int i;
    for ( i = 0; i < RTFLAGS_NDIMS(pt->point->flags); i++ )
    {
      buf = double_to_wkb_buf(ctx, nn, buf, variant);
    }
  }
  /* Everything else is flagged as empty using num-elements == 0 */
  else
  {
    /* Set nrings/npoints/ngeoms to zero */
    buf = integer_to_wkb_buf(ctx, 0, buf, variant);
  }

  return buf;
}

/*
* RTPOINTARRAY
*/
static size_t ptarray_to_wkb_size(const RTCTX *ctx, const RTPOINTARRAY *pa, uint8_t variant)
{
  int dims = 2;
  size_t size = 0;

  if ( variant & (RTWKB_ISO | RTWKB_EXTENDED) )
    dims = RTFLAGS_NDIMS(pa->flags);

  /* Include the npoints if it's not a POINT type) */
  if ( ! ( variant & RTWKB_NO_NPOINTS ) )
    size += RTWKB_INT_SIZE;

  /* size of the double list */
  size += pa->npoints * dims * RTWKB_DOUBLE_SIZE;

  return size;
}

static uint8_t* ptarray_to_wkb_buf(const RTCTX *ctx, const RTPOINTARRAY *pa, uint8_t *buf, uint8_t variant)
{
  int dims = 2;
  int pa_dims = RTFLAGS_NDIMS(pa->flags);
  int i, j;
  double *dbl_ptr;

  /* SFSQL is artays 2-d. Extended and ISO use all available dimensions */
  if ( (variant & RTWKB_ISO) || (variant & RTWKB_EXTENDED) )
    dims = pa_dims;

  /* Set the number of points (if it's not a POINT type) */
  if ( ! ( variant & RTWKB_NO_NPOINTS ) )
    buf = integer_to_wkb_buf(ctx, pa->npoints, buf, variant);

  /* Bulk copy the coordinates when: dimensionality matches, output format */
  /* is not hex, and output endian matches internal endian. */
  if ( pa->npoints && (dims == pa_dims) && ! wkb_swap_bytes(ctx, variant) && ! (variant & RTWKB_HEX)  )
  {
    size_t size = pa->npoints * dims * RTWKB_DOUBLE_SIZE;
    memcpy(buf, rt_getPoint_internal(ctx, pa, 0), size);
    buf += size;
  }
  /* Copy coordinates one-by-one otherwise */
  else
  {
    for ( i = 0; i < pa->npoints; i++ )
    {
      RTDEBUGF(ctx, 4, "Writing point #%d", i);
      dbl_ptr = (double*)rt_getPoint_internal(ctx, pa, i);
      for ( j = 0; j < dims; j++ )
      {
        RTDEBUGF(ctx, 4, "Writing dimension #%d (buf = %p)", j, buf);
        buf = double_to_wkb_buf(ctx, dbl_ptr[j], buf, variant);
      }
    }
  }
  RTDEBUGF(ctx, 4, "Done (buf = %p)", buf);
  return buf;
}

/*
* POINT
*/
static size_t rtpoint_to_wkb_size(const RTCTX *ctx, const RTPOINT *pt, uint8_t variant)
{
  /* Endian flag + type number */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)pt) )
    return empty_to_wkb_size(ctx, (RTGEOM*)pt, variant);

  /* Extended RTWKB needs space for optional SRID integer */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)pt, variant) )
    size += RTWKB_INT_SIZE;

  /* Points */
  size += ptarray_to_wkb_size(ctx, pt->point, variant | RTWKB_NO_NPOINTS);
  return size;
}

static uint8_t* rtpoint_to_wkb_buf(const RTCTX *ctx, const RTPOINT *pt, uint8_t *buf, uint8_t variant)
{
  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)pt) )
    return empty_to_wkb_buf(ctx, (RTGEOM*)pt, buf, variant);

  /* Set the endian flag */
  RTDEBUGF(ctx, 4, "Entering function, buf = %p", buf);
  buf = endian_to_wkb_buf(ctx, buf, variant);
  RTDEBUGF(ctx, 4, "Endian set, buf = %p", buf);
  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, rtgeom_wkb_type(ctx, (RTGEOM*)pt, variant), buf, variant);
  RTDEBUGF(ctx, 4, "Type set, buf = %p", buf);
  /* Set the optional SRID for extended variant */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)pt, variant) )
  {
    buf = integer_to_wkb_buf(ctx, pt->srid, buf, variant);
    RTDEBUGF(ctx, 4, "SRID set, buf = %p", buf);
  }
  /* Set the coordinates */
  buf = ptarray_to_wkb_buf(ctx, pt->point, buf, variant | RTWKB_NO_NPOINTS);
  RTDEBUGF(ctx, 4, "Pointarray set, buf = %p", buf);
  return buf;
}

/*
* LINESTRING, CIRCULARSTRING
*/
static size_t rtline_to_wkb_size(const RTCTX *ctx, const RTLINE *line, uint8_t variant)
{
  /* Endian flag + type number */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)line) )
    return empty_to_wkb_size(ctx, (RTGEOM*)line, variant);

  /* Extended RTWKB needs space for optional SRID integer */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)line, variant) )
    size += RTWKB_INT_SIZE;

  /* Size of point array */
  size += ptarray_to_wkb_size(ctx, line->points, variant);
  return size;
}

static uint8_t* rtline_to_wkb_buf(const RTCTX *ctx, const RTLINE *line, uint8_t *buf, uint8_t variant)
{
  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)line) )
    return empty_to_wkb_buf(ctx, (RTGEOM*)line, buf, variant);

  /* Set the endian flag */
  buf = endian_to_wkb_buf(ctx, buf, variant);
  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, rtgeom_wkb_type(ctx, (RTGEOM*)line, variant), buf, variant);
  /* Set the optional SRID for extended variant */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)line, variant) )
    buf = integer_to_wkb_buf(ctx, line->srid, buf, variant);
  /* Set the coordinates */
  buf = ptarray_to_wkb_buf(ctx, line->points, buf, variant);
  return buf;
}

/*
* TRIANGLE
*/
static size_t rttriangle_to_wkb_size(const RTCTX *ctx, const RTTRIANGLE *tri, uint8_t variant)
{
  /* endian flag + type number + number of rings */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;

  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)tri) )
    return empty_to_wkb_size(ctx, (RTGEOM*)tri, variant);

  /* Extended RTWKB needs space for optional SRID integer */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)tri, variant) )
    size += RTWKB_INT_SIZE;

  /* How big is this point array? */
  size += ptarray_to_wkb_size(ctx, tri->points, variant);

  return size;
}

static uint8_t* rttriangle_to_wkb_buf(const RTCTX *ctx, const RTTRIANGLE *tri, uint8_t *buf, uint8_t variant)
{
  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)tri) )
    return empty_to_wkb_buf(ctx, (RTGEOM*)tri, buf, variant);

  /* Set the endian flag */
  buf = endian_to_wkb_buf(ctx, buf, variant);

  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, rtgeom_wkb_type(ctx, (RTGEOM*)tri, variant), buf, variant);

  /* Set the optional SRID for extended variant */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)tri, variant) )
    buf = integer_to_wkb_buf(ctx, tri->srid, buf, variant);

  /* Set the number of rings (only one, it's a triangle, buddy) */
  buf = integer_to_wkb_buf(ctx, 1, buf, variant);

  /* Write that ring */
  buf = ptarray_to_wkb_buf(ctx, tri->points, buf, variant);

  return buf;
}

/*
* POLYGON
*/
static size_t rtpoly_to_wkb_size(const RTCTX *ctx, const RTPOLY *poly, uint8_t variant)
{
  /* endian flag + type number + number of rings */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;
  int i = 0;

  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)poly) )
    return empty_to_wkb_size(ctx, (RTGEOM*)poly, variant);

  /* Extended RTWKB needs space for optional SRID integer */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)poly, variant) )
    size += RTWKB_INT_SIZE;

  for ( i = 0; i < poly->nrings; i++ )
  {
    /* Size of ring point array */
    size += ptarray_to_wkb_size(ctx, poly->rings[i], variant);
  }

  return size;
}

static uint8_t* rtpoly_to_wkb_buf(const RTCTX *ctx, const RTPOLY *poly, uint8_t *buf, uint8_t variant)
{
  int i;

  /* Only process empty at this level in the EXTENDED case */
  if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty(ctx, (RTGEOM*)poly) )
    return empty_to_wkb_buf(ctx, (RTGEOM*)poly, buf, variant);

  /* Set the endian flag */
  buf = endian_to_wkb_buf(ctx, buf, variant);
  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, rtgeom_wkb_type(ctx, (RTGEOM*)poly, variant), buf, variant);
  /* Set the optional SRID for extended variant */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)poly, variant) )
    buf = integer_to_wkb_buf(ctx, poly->srid, buf, variant);
  /* Set the number of rings */
  buf = integer_to_wkb_buf(ctx, poly->nrings, buf, variant);

  for ( i = 0; i < poly->nrings; i++ )
  {
    buf = ptarray_to_wkb_buf(ctx, poly->rings[i], buf, variant);
  }

  return buf;
}


/*
* MULTIPOINT, MULTILINESTRING, MULTIPOLYGON, GEOMETRYCOLLECTION
* MULTICURVE, COMPOUNDCURVE, MULTISURFACE, CURVEPOLYGON, TIN,
* POLYHEDRALSURFACE
*/
static size_t rtcollection_to_wkb_size(const RTCTX *ctx, const RTCOLLECTION *col, uint8_t variant)
{
  /* Endian flag + type number + number of subgeoms */
  size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;
  int i = 0;

  /* Extended RTWKB needs space for optional SRID integer */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)col, variant) )
    size += RTWKB_INT_SIZE;

  for ( i = 0; i < col->ngeoms; i++ )
  {
    /* size of subgeom */
    size += rtgeom_to_wkb_size(ctx, (RTGEOM*)col->geoms[i], variant | RTWKB_NO_SRID);
  }

  return size;
}

static uint8_t* rtcollection_to_wkb_buf(const RTCTX *ctx, const RTCOLLECTION *col, uint8_t *buf, uint8_t variant)
{
  int i;

  /* Set the endian flag */
  buf = endian_to_wkb_buf(ctx, buf, variant);
  /* Set the geometry type */
  buf = integer_to_wkb_buf(ctx, rtgeom_wkb_type(ctx, (RTGEOM*)col, variant), buf, variant);
  /* Set the optional SRID for extended variant */
  if ( rtgeom_wkb_needs_srid(ctx, (RTGEOM*)col, variant) )
    buf = integer_to_wkb_buf(ctx, col->srid, buf, variant);
  /* Set the number of sub-geometries */
  buf = integer_to_wkb_buf(ctx, col->ngeoms, buf, variant);

  /* Write the sub-geometries. Sub-geometries do not get SRIDs, they
     inherit from their parents. */
  for ( i = 0; i < col->ngeoms; i++ )
  {
    buf = rtgeom_to_wkb_buf(ctx, col->geoms[i], buf, variant | RTWKB_NO_SRID);
  }

  return buf;
}

/*
* GEOMETRY
*/
static size_t rtgeom_to_wkb_size(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant)
{
  size_t size = 0;

  if ( geom == NULL )
    return 0;

  /* Short circuit out empty geometries */
  if ( (!(variant & RTWKB_EXTENDED)) && rtgeom_is_empty(ctx, geom) )
  {
    return empty_to_wkb_size(ctx, geom, variant);
  }

  switch ( geom->type )
  {
    case RTPOINTTYPE:
      size += rtpoint_to_wkb_size(ctx, (RTPOINT*)geom, variant);
      break;

    /* LineString and CircularString both have points elements */
    case RTCIRCSTRINGTYPE:
    case RTLINETYPE:
      size += rtline_to_wkb_size(ctx, (RTLINE*)geom, variant);
      break;

    /* Polygon has nrings and rings elements */
    case RTPOLYGONTYPE:
      size += rtpoly_to_wkb_size(ctx, (RTPOLY*)geom, variant);
      break;

    /* Triangle has one ring of three points */
    case RTTRIANGLETYPE:
      size += rttriangle_to_wkb_size(ctx, (RTTRIANGLE*)geom, variant);
      break;

    /* All these Collection types have ngeoms and geoms elements */
    case RTMULTIPOINTTYPE:
    case RTMULTILINETYPE:
    case RTMULTIPOLYGONTYPE:
    case RTCOMPOUNDTYPE:
    case RTCURVEPOLYTYPE:
    case RTMULTICURVETYPE:
    case RTMULTISURFACETYPE:
    case RTCOLLECTIONTYPE:
    case RTPOLYHEDRALSURFACETYPE:
    case RTTINTYPE:
      size += rtcollection_to_wkb_size(ctx, (RTCOLLECTION*)geom, variant);
      break;

    /* Unknown type! */
    default:
      rterror(ctx, "Unsupported geometry type: %s [%d]", rttype_name(ctx, geom->type), geom->type);
  }

  return size;
}

/* TODO handle the TRIANGLE type properly */

static uint8_t* rtgeom_to_wkb_buf(const RTCTX *ctx, const RTGEOM *geom, uint8_t *buf, uint8_t variant)
{

  /* Do not simplify empties when outputting to canonical form */
  if ( rtgeom_is_empty(ctx, geom) & ! (variant & RTWKB_EXTENDED) )
    return empty_to_wkb_buf(ctx, geom, buf, variant);

  switch ( geom->type )
  {
    case RTPOINTTYPE:
      return rtpoint_to_wkb_buf(ctx, (RTPOINT*)geom, buf, variant);

    /* LineString and CircularString both have 'points' elements */
    case RTCIRCSTRINGTYPE:
    case RTLINETYPE:
      return rtline_to_wkb_buf(ctx, (RTLINE*)geom, buf, variant);

    /* Polygon has 'nrings' and 'rings' elements */
    case RTPOLYGONTYPE:
      return rtpoly_to_wkb_buf(ctx, (RTPOLY*)geom, buf, variant);

    /* Triangle has one ring of three points */
    case RTTRIANGLETYPE:
      return rttriangle_to_wkb_buf(ctx, (RTTRIANGLE*)geom, buf, variant);

    /* All these Collection types have 'ngeoms' and 'geoms' elements */
    case RTMULTIPOINTTYPE:
    case RTMULTILINETYPE:
    case RTMULTIPOLYGONTYPE:
    case RTCOMPOUNDTYPE:
    case RTCURVEPOLYTYPE:
    case RTMULTICURVETYPE:
    case RTMULTISURFACETYPE:
    case RTCOLLECTIONTYPE:
    case RTPOLYHEDRALSURFACETYPE:
    case RTTINTYPE:
      return rtcollection_to_wkb_buf(ctx, (RTCOLLECTION*)geom, buf, variant);

    /* Unknown type! */
    default:
      rterror(ctx, "Unsupported geometry type: %s [%d]", rttype_name(ctx, geom->type), geom->type);
  }
  /* Return value to keep compiler happy. */
  return 0;
}

/**
* Convert RTGEOM to a char* in RTWKB format. Caller is responsible for freeing
* the returned array.
*
* @param variant. Unsigned bitmask value. Accepts one of: RTWKB_ISO, RTWKB_EXTENDED, RTWKB_SFSQL.
* Accepts any of: RTWKB_NDR, RTWKB_HEX. For example: Variant = ( RTWKB_ISO | RTWKB_NDR ) would
* return the little-endian ISO form of RTWKB. For Example: Variant = ( RTWKB_EXTENDED | RTWKB_HEX )
* would return the big-endian extended form of RTWKB, as hex-encoded ASCII (the "canonical form").
* @param size_out If supplied, will return the size of the returned memory segment,
* including the null terminator in the case of ASCII.
*/
uint8_t* rtgeom_to_wkb(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant, size_t *size_out)
{
  size_t buf_size;
  uint8_t *buf = NULL;
  uint8_t *wkb_out = NULL;

  /* Initialize output size */
  if ( size_out ) *size_out = 0;

  if ( geom == NULL )
  {
    RTDEBUG(ctx, 4,"Cannot convert NULL into RTWKB.");
    rterror(ctx, "Cannot convert NULL into RTWKB.");
    return NULL;
  }

  /* Calculate the required size of the output buffer */
  buf_size = rtgeom_to_wkb_size(ctx, geom, variant);
  RTDEBUGF(ctx, 4, "RTWKB output size: %d", buf_size);

  if ( buf_size == 0 )
  {
    RTDEBUG(ctx, 4,"Error calculating output RTWKB buffer size.");
    rterror(ctx, "Error calculating output RTWKB buffer size.");
    return NULL;
  }

  /* Hex string takes twice as much space as binary + a null character */
  if ( variant & RTWKB_HEX )
  {
    buf_size = 2 * buf_size + 1;
    RTDEBUGF(ctx, 4, "Hex RTWKB output size: %d", buf_size);
  }

  /* If neither or both variants are specified, choose the native order */
  if ( ! (variant & RTWKB_NDR || variant & RTWKB_XDR) ||
         (variant & RTWKB_NDR && variant & RTWKB_XDR) )
  {
    if ( getMachineEndian(ctx) == NDR )
      variant = variant | RTWKB_NDR;
    else
      variant = variant | RTWKB_XDR;
  }

  /* Allocate the buffer */
  buf = rtalloc(ctx, buf_size);

  if ( buf == NULL )
  {
    RTDEBUGF(ctx, 4,"Unable to allocate %d bytes for RTWKB output buffer.", buf_size);
    rterror(ctx, "Unable to allocate %d bytes for RTWKB output buffer.", buf_size);
    return NULL;
  }

  /* Retain a pointer to the front of the buffer for later */
  wkb_out = buf;

  /* Write the RTWKB into the output buffer */
  buf = rtgeom_to_wkb_buf(ctx, geom, buf, variant);

  /* Null the last byte if this is a hex output */
  if ( variant & RTWKB_HEX )
  {
    *buf = '\0';
    buf++;
  }

  RTDEBUGF(ctx, 4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

  /* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
  if ( buf_size != (buf - wkb_out) )
  {
    RTDEBUG(ctx, 4,"Output RTWKB is not the same size as the allocated buffer.");
    rterror(ctx, "Output RTWKB is not the same size as the allocated buffer.");
    rtfree(ctx, wkb_out);
    return NULL;
  }

  /* Report output size */
  if ( size_out ) *size_out = buf_size;

  return wkb_out;
}

char* rtgeom_to_hexwkb(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant, size_t *size_out)
{
  return (char*)rtgeom_to_wkb(ctx, geom, variant | RTWKB_HEX, size_out);
}

