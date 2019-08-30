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
/*#define RTGEOM_DEBUG_LEVEL 4*/
#include "librttopo_geom_internal.h" /* NOTE: includes rtgeom_log.h */
#include "rtgeom_log.h"
#include <math.h>

/**
* Used for passing the parse state between the parsing functions.
*/
typedef struct
{
  const uint8_t *wkb; /* Points to start of RTWKB */
  size_t wkb_size; /* Expected size of RTWKB */
  int swap_bytes; /* Do an endian flip? */
  int check; /* Simple validity checks on geometries */
  uint32_t rttype; /* Current type we are handling */
  uint32_t srid; /* Current SRID we are handling */
  int has_z; /* Z? */
  int has_m; /* M? */
  int has_srid; /* SRID? */
  const uint8_t *pos; /* Current parse position */
} wkb_parse_state;


/**
* Internal function declarations.
*/
RTGEOM* rtgeom_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s);



/**********************************************************************/

/* Our static character->number map. Anything > 15 is invalid */
static uint8_t hex2char[256] = {
    /* not Hex characters */
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    /* 0-9 */
    0,1,2,3,4,5,6,7,8,9,20,20,20,20,20,20,
    /* A-F */
    20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
    /* not Hex characters */
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
  /* a-f */
    20,10,11,12,13,14,15,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    /* not Hex characters (upper 128 characters) */
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
    20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20
    };


uint8_t* bytes_from_hexbytes(const RTCTX *ctx, const char *hexbuf, size_t hexsize)
{
  uint8_t *buf = NULL;
  register uint8_t h1, h2;
  int i;

  if( hexsize % 2 )
    rterror(ctx, "Invalid hex string, length (%d) has to be a multiple of two!", hexsize);

  buf = rtalloc(ctx, hexsize/2);

  if( ! buf )
    rterror(ctx, "Unable to allocate memory buffer.");

  for( i = 0; i < hexsize/2; i++ )
  {
    h1 = hex2char[(int)hexbuf[2*i]];
    h2 = hex2char[(int)hexbuf[2*i+1]];
    if( h1 > 15 )
      rterror(ctx, "Invalid hex character (%c) encountered", hexbuf[2*i]);
    if( h2 > 15 )
      rterror(ctx, "Invalid hex character (%c) encountered", hexbuf[2*i+1]);
    /* First character is high bits, second is low bits */
    buf[i] = ((h1 & 0x0F) << 4) | (h2 & 0x0F);
  }
  return buf;
}


/**********************************************************************/





/**
* Check that we are not about to read off the end of the RTWKB
* array.
*/
static inline void wkb_parse_state_check(const RTCTX *ctx, wkb_parse_state *s, size_t next)
{
  if( (s->pos + next) > (s->wkb + s->wkb_size) )
    rterror(ctx, "RTWKB structure does not match expected size!");
}

/**
* Take in an unknown kind of wkb type number and ensure it comes out
* as an extended RTWKB type number (with Z/M/SRID flags masked onto the
* high bits).
*/
static void rttype_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s, uint32_t wkb_type)
{
  uint32_t wkb_simple_type;

  RTDEBUG(ctx, 4, "Entered function");

  s->has_z = RT_FALSE;
  s->has_m = RT_FALSE;
  s->has_srid = RT_FALSE;

  /* If any of the higher bits are set, this is probably an extended type. */
  if( wkb_type & 0xF0000000 )
  {
    if( wkb_type & RTWKBZOFFSET ) s->has_z = RT_TRUE;
    if( wkb_type & RTWKBMOFFSET ) s->has_m = RT_TRUE;
    if( wkb_type & RTWKBSRIDFLAG ) s->has_srid = RT_TRUE;
    RTDEBUGF(ctx, 4, "Extended type: has_z=%d has_m=%d has_srid=%d", s->has_z, s->has_m, s->has_srid);
  }

  /* Mask off the flags */
  wkb_type = wkb_type & 0x0FFFFFFF;
  /* Strip out just the type number (1-12) from the ISO number (eg 3001-3012) */
  wkb_simple_type = wkb_type % 1000;

  /* Extract the Z/M information from ISO style numbers */
  if( wkb_type >= 3000 && wkb_type < 4000 )
  {
    s->has_z = RT_TRUE;
    s->has_m = RT_TRUE;
  }
  else if ( wkb_type >= 2000 && wkb_type < 3000 )
  {
    s->has_m = RT_TRUE;
  }
  else if ( wkb_type >= 1000 && wkb_type < 2000 )
  {
    s->has_z = RT_TRUE;
  }

  switch (wkb_simple_type)
  {
    case RTWKB_POINT_TYPE:
      s->rttype = RTPOINTTYPE;
      break;
    case RTWKB_LINESTRING_TYPE:
      s->rttype = RTLINETYPE;
      break;
    case RTWKB_POLYGON_TYPE:
      s->rttype = RTPOLYGONTYPE;
      break;
    case RTWKB_MULTIPOINT_TYPE:
      s->rttype = RTMULTIPOINTTYPE;
      break;
    case RTWKB_MULTILINESTRING_TYPE:
      s->rttype = RTMULTILINETYPE;
      break;
    case RTWKB_MULTIPOLYGON_TYPE:
      s->rttype = RTMULTIPOLYGONTYPE;
      break;
    case RTWKB_GEOMETRYCOLLECTION_TYPE:
      s->rttype = RTCOLLECTIONTYPE;
      break;
    case RTWKB_CIRCULARSTRING_TYPE:
      s->rttype = RTCIRCSTRINGTYPE;
      break;
    case RTWKB_COMPOUNDCURVE_TYPE:
      s->rttype = RTCOMPOUNDTYPE;
      break;
    case RTWKB_CURVEPOLYGON_TYPE:
      s->rttype = RTCURVEPOLYTYPE;
      break;
    case RTWKB_MULTICURVE_TYPE:
      s->rttype = RTMULTICURVETYPE;
      break;
    case RTWKB_MULTISURFACE_TYPE:
      s->rttype = RTMULTISURFACETYPE;
      break;
    case RTWKB_POLYHEDRALSURFACE_TYPE:
      s->rttype = RTPOLYHEDRALSURFACETYPE;
      break;
    case RTWKB_TIN_TYPE:
      s->rttype = RTTINTYPE;
      break;
    case RTWKB_TRIANGLE_TYPE:
      s->rttype = RTTRIANGLETYPE;
      break;

    /* PostGIS 1.5 emits 13, 14 for CurvePolygon, MultiCurve */
    /* These numbers aren't SQL/MM (numbers currently only */
    /* go up to 12. We can handle the old data here (for now??) */
    /* converting them into the rttypes that are intended. */
    case RTWKB_CURVE_TYPE:
      s->rttype = RTCURVEPOLYTYPE;
      break;
    case RTWKB_SURFACE_TYPE:
      s->rttype = RTMULTICURVETYPE;
      break;

    default: /* Error! */
      rterror(ctx, "Unknown RTWKB type (%d)! Full RTWKB type number was (%d).", wkb_simple_type, wkb_type);
      break;
  }

  RTDEBUGF(ctx, 4,"Got rttype %s (%u)", rttype_name(ctx, s->rttype), s->rttype);

  return;
}

/**
* Byte
* Read a byte and advance the parse state forward.
*/
static char byte_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  char char_value = 0;
  RTDEBUG(ctx, 4, "Entered function");

  wkb_parse_state_check(ctx, s, RTWKB_BYTE_SIZE);
  RTDEBUG(ctx, 4, "Passed state check");

  char_value = s->pos[0];
  RTDEBUGF(ctx, 4, "Read byte value: %x", char_value);
  s->pos += RTWKB_BYTE_SIZE;

  return char_value;
}

/**
* Int32
* Read 4-byte integer and advance the parse state forward.
*/
static uint32_t integer_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  uint32_t i = 0;

  wkb_parse_state_check(ctx, s, RTWKB_INT_SIZE);

  memcpy(&i, s->pos, RTWKB_INT_SIZE);

  /* Swap? Copy into a stack-allocated integer. */
  if( s->swap_bytes )
  {
    int j = 0;
    uint8_t tmp;

    for( j = 0; j < RTWKB_INT_SIZE/2; j++ )
    {
      tmp = ((uint8_t*)(&i))[j];
      ((uint8_t*)(&i))[j] = ((uint8_t*)(&i))[RTWKB_INT_SIZE - j - 1];
      ((uint8_t*)(&i))[RTWKB_INT_SIZE - j - 1] = tmp;
    }
  }

  s->pos += RTWKB_INT_SIZE;
  return i;
}

/**
* Double
* Read an 8-byte double and advance the parse state forward.
*/
static double double_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  double d = 0;

  wkb_parse_state_check(ctx, s, RTWKB_DOUBLE_SIZE);

  memcpy(&d, s->pos, RTWKB_DOUBLE_SIZE);

  /* Swap? Copy into a stack-allocated integer. */
  if( s->swap_bytes )
  {
    int i = 0;
    uint8_t tmp;

    for( i = 0; i < RTWKB_DOUBLE_SIZE/2; i++ )
    {
      tmp = ((uint8_t*)(&d))[i];
      ((uint8_t*)(&d))[i] = ((uint8_t*)(&d))[RTWKB_DOUBLE_SIZE - i - 1];
      ((uint8_t*)(&d))[RTWKB_DOUBLE_SIZE - i - 1] = tmp;
    }

  }

  s->pos += RTWKB_DOUBLE_SIZE;
  return d;
}

/**
* RTPOINTARRAY
* Read a dynamically sized point array and advance the parse state forward.
* First read the number of points, then read the points.
*/
static RTPOINTARRAY* ptarray_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  RTPOINTARRAY *pa = NULL;
  size_t pa_size;
  uint32_t ndims = 2;
  uint32_t npoints = 0;

  /* Calculate the size of this point array. */
  npoints = integer_from_wkb_state(ctx, s);

  RTDEBUGF(ctx, 4,"Pointarray has %d points", npoints);

  if( s->has_z ) ndims++;
  if( s->has_m ) ndims++;
  pa_size = npoints * ndims * RTWKB_DOUBLE_SIZE;

  /* Empty! */
  if( npoints == 0 )
    return ptarray_construct(ctx, s->has_z, s->has_m, npoints);

  /* Does the data we want to read exist? */
  wkb_parse_state_check(ctx, s, pa_size);

  /* If we're in a native endianness, we can just copy the data directly! */
  if( ! s->swap_bytes )
  {
    pa = ptarray_construct_copy_data(ctx, s->has_z, s->has_m, npoints, (uint8_t*)s->pos);
    s->pos += pa_size;
  }
  /* Otherwise we have to read each double, separately. */
  else
  {
    int i = 0;
    double *dlist;
    pa = ptarray_construct(ctx, s->has_z, s->has_m, npoints);
    dlist = (double*)(pa->serialized_pointlist);
    for( i = 0; i < npoints * ndims; i++ )
    {
      dlist[i] = double_from_wkb_state(ctx, s);
    }
  }

  return pa;
}

/**
* POINT
* Read a RTWKB point, starting just after the endian byte,
* type number and optional srid number.
* Advance the parse state forward appropriately.
* RTWKB point has just a set of doubles, with the quantity depending on the
* dimension of the point, so this looks like a special case of the above
* with only one point.
*/
static RTPOINT* rtpoint_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  static uint32_t npoints = 1;
  RTPOINTARRAY *pa = NULL;
  size_t pa_size;
  uint32_t ndims = 2;
  const RTPOINT2D *pt;

  /* Count the dimensions. */
  if( s->has_z ) ndims++;
  if( s->has_m ) ndims++;
  pa_size = ndims * RTWKB_DOUBLE_SIZE;

  /* Does the data we want to read exist? */
  wkb_parse_state_check(ctx, s, pa_size);

  /* If we're in a native endianness, we can just copy the data directly! */
  if( ! s->swap_bytes )
  {
    pa = ptarray_construct_copy_data(ctx, s->has_z, s->has_m, npoints, (uint8_t*)s->pos);
    s->pos += pa_size;
  }
  /* Otherwise we have to read each double, separately */
  else
  {
    int i = 0;
    double *dlist;
    pa = ptarray_construct(ctx, s->has_z, s->has_m, npoints);
    dlist = (double*)(pa->serialized_pointlist);
    for( i = 0; i < ndims; i++ )
    {
      dlist[i] = double_from_wkb_state(ctx, s);
    }
  }

  /* Check for POINT(NaN NaN) ==> POINT EMPTY */
  pt = rt_getPoint2d_cp(ctx, pa, 0);
  if ( isnan(pt->x) && isnan(pt->y) )
  {
    ptarray_free(ctx, pa);
    return rtpoint_construct_empty(ctx, s->srid, s->has_z, s->has_m);
  }
  else
  {
    return rtpoint_construct(ctx, s->srid, NULL, pa);
  }
}

/**
* LINESTRING
* Read a RTWKB linestring, starting just after the endian byte,
* type number and optional srid number. Advance the parse state
* forward appropriately.
* There is only one pointarray in a linestring. Optionally
* check for minimal following of rules (two point minimum).
*/
static RTLINE* rtline_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  RTPOINTARRAY *pa = ptarray_from_wkb_state(ctx, s);

  if( pa == NULL || pa->npoints == 0 )
    return rtline_construct_empty(ctx, s->srid, s->has_z, s->has_m);

  if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 2 )
  {
    rterror(ctx, "%s must have at least two points", rttype_name(ctx, s->rttype));
    return NULL;
  }

  return rtline_construct(ctx, s->srid, NULL, pa);
}

/**
* CIRCULARSTRING
* Read a RTWKB circularstring, starting just after the endian byte,
* type number and optional srid number. Advance the parse state
* forward appropriately.
* There is only one pointarray in a linestring. Optionally
* check for minimal following of rules (three point minimum,
* odd number of points).
*/
static RTCIRCSTRING* rtcircstring_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  RTPOINTARRAY *pa = ptarray_from_wkb_state(ctx, s);

  if( pa == NULL || pa->npoints == 0 )
    return rtcircstring_construct_empty(ctx, s->srid, s->has_z, s->has_m);

  if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 3 )
  {
    rterror(ctx, "%s must have at least three points", rttype_name(ctx, s->rttype));
    return NULL;
  }

  if( s->check & RT_PARSER_CHECK_ODD && ! (pa->npoints % 2) )
  {
    rterror(ctx, "%s must have an odd number of points", rttype_name(ctx, s->rttype));
    return NULL;
  }

  return rtcircstring_construct(ctx, s->srid, NULL, pa);
}

/**
* POLYGON
* Read a RTWKB polygon, starting just after the endian byte,
* type number and optional srid number. Advance the parse state
* forward appropriately.
* First read the number of rings, then read each ring
* (which are structured as point arrays)
*/
static RTPOLY* rtpoly_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  uint32_t nrings = integer_from_wkb_state(ctx, s);
  int i = 0;
  RTPOLY *poly = rtpoly_construct_empty(ctx, s->srid, s->has_z, s->has_m);

  RTDEBUGF(ctx, 4,"Polygon has %d rings", nrings);

  /* Empty polygon? */
  if( nrings == 0 )
    return poly;

  for( i = 0; i < nrings; i++ )
  {
    RTPOINTARRAY *pa = ptarray_from_wkb_state(ctx, s);
    if( pa == NULL )
      continue;

    /* Check for at least four points. */
    if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 4 )
    {
      RTDEBUGF(ctx, 2, "%s must have at least four points in each ring", rttype_name(ctx, s->rttype));
      rterror(ctx, "%s must have at least four points in each ring", rttype_name(ctx, s->rttype));
      return NULL;
    }

    /* Check that first and last points are the same. */
    if( s->check & RT_PARSER_CHECK_CLOSURE && ! ptarray_is_closed_2d(ctx, pa) )
    {
      RTDEBUGF(ctx, 2, "%s must have closed rings", rttype_name(ctx, s->rttype));
      rterror(ctx, "%s must have closed rings", rttype_name(ctx, s->rttype));
      return NULL;
    }

    /* Add ring to polygon */
    if ( rtpoly_add_ring(ctx, poly, pa) == RT_FAILURE )
    {
      RTDEBUG(ctx, 2, "Unable to add ring to polygon");
      rterror(ctx, "Unable to add ring to polygon");
    }

  }
  return poly;
}

/**
* TRIANGLE
* Read a RTWKB triangle, starting just after the endian byte,
* type number and optional srid number. Advance the parse state
* forward appropriately.
* Triangles are encoded like polygons in RTWKB, but more like linestrings
* as rtgeometries.
*/
static RTTRIANGLE* rttriangle_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  uint32_t nrings = integer_from_wkb_state(ctx, s);
  RTTRIANGLE *tri = rttriangle_construct_empty(ctx, s->srid, s->has_z, s->has_m);
  RTPOINTARRAY *pa = NULL;

  /* Empty triangle? */
  if( nrings == 0 )
    return tri;

  /* Should be only one ring. */
  if ( nrings != 1 )
    rterror(ctx, "Triangle has wrong number of rings: %d", nrings);

  /* There's only one ring, we hope? */
  pa = ptarray_from_wkb_state(ctx, s);

  /* If there's no points, return an empty triangle. */
  if( pa == NULL )
    return tri;

  /* Check for at least four points. */
  if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 4 )
  {
    RTDEBUGF(ctx, 2, "%s must have at least four points", rttype_name(ctx, s->rttype));
    rterror(ctx, "%s must have at least four points", rttype_name(ctx, s->rttype));
    return NULL;
  }

  if( s->check & RT_PARSER_CHECK_CLOSURE && ! ptarray_is_closed(ctx, pa) )
  {
    rterror(ctx, "%s must have closed rings", rttype_name(ctx, s->rttype));
    return NULL;
  }

  if( s->check & RT_PARSER_CHECK_ZCLOSURE && ! ptarray_is_closed_z(ctx, pa) )
  {
    rterror(ctx, "%s must have closed rings", rttype_name(ctx, s->rttype));
    return NULL;
  }

  /* Empty TRIANGLE starts w/ empty RTPOINTARRAY, free it first */
  if (tri->points)
    ptarray_free(ctx, tri->points);

  tri->points = pa;
  return tri;
}

/**
* RTCURVEPOLYTYPE
*/
static RTCURVEPOLY* rtcurvepoly_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  uint32_t ngeoms = integer_from_wkb_state(ctx, s);
  RTCURVEPOLY *cp = rtcurvepoly_construct_empty(ctx, s->srid, s->has_z, s->has_m);
  RTGEOM *geom = NULL;
  int i;

  /* Empty collection? */
  if ( ngeoms == 0 )
    return cp;

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtgeom_from_wkb_state(ctx, s);
    if ( rtcurvepoly_add_ring(ctx, cp, geom) == RT_FAILURE )
      rterror(ctx, "Unable to add geometry (%p) to curvepoly (%p)", geom, cp);
  }

  return cp;
}

/**
* RTPOLYHEDRALSURFACETYPE
*/

/**
* COLLECTION, RTMULTIPOINTTYPE, RTMULTILINETYPE, RTMULTIPOLYGONTYPE, RTCOMPOUNDTYPE,
* RTMULTICURVETYPE, RTMULTISURFACETYPE,
* RTTINTYPE
*/
static RTCOLLECTION* rtcollection_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  uint32_t ngeoms = integer_from_wkb_state(ctx, s);
  RTCOLLECTION *col = rtcollection_construct_empty(ctx, s->rttype, s->srid, s->has_z, s->has_m);
  RTGEOM *geom = NULL;
  int i;

  RTDEBUGF(ctx, 4,"Collection has %d components", ngeoms);

  /* Empty collection? */
  if ( ngeoms == 0 )
    return col;

  /* Be strict in polyhedral surface closures */
  if ( s->rttype == RTPOLYHEDRALSURFACETYPE )
    s->check |= RT_PARSER_CHECK_ZCLOSURE;

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtgeom_from_wkb_state(ctx, s);
    if ( rtcollection_add_rtgeom(ctx, col, geom) == NULL )
    {
      rterror(ctx, "Unable to add geometry (%p) to collection (%p)", geom, col);
      return NULL;
    }
  }

  return col;
}


/**
* GEOMETRY
* Generic handling for RTWKB geometries. The front of every RTWKB geometry
* (including those embedded in collections) is an endian byte, a type
* number and an optional srid number. We handle all those here, then pass
* to the appropriate handler for the specific type.
*/
RTGEOM* rtgeom_from_wkb_state(const RTCTX *ctx, wkb_parse_state *s)
{
  char wkb_little_endian;
  uint32_t wkb_type;

  RTDEBUG(ctx, 4,"Entered function");

  /* Fail when handed incorrect starting byte */
  wkb_little_endian = byte_from_wkb_state(ctx, s);
  if( wkb_little_endian != 1 && wkb_little_endian != 0 )
  {
    RTDEBUG(ctx, 4,"Leaving due to bad first byte!");
    rterror(ctx, "Invalid endian flag value encountered.");
    return NULL;
  }

  /* Check the endianness of our input  */
  s->swap_bytes = RT_FALSE;
  if( getMachineEndian(ctx) == NDR ) /* Machine arch is little */
  {
    if ( ! wkb_little_endian )    /* Data is big! */
      s->swap_bytes = RT_TRUE;
  }
  else                              /* Machine arch is big */
  {
    if ( wkb_little_endian )      /* Data is little! */
      s->swap_bytes = RT_TRUE;
  }

  /* Read the type number */
  wkb_type = integer_from_wkb_state(ctx, s);
  RTDEBUGF(ctx, 4,"Got RTWKB type number: 0x%X", wkb_type);
  rttype_from_wkb_state(ctx, s, wkb_type);

  /* Read the SRID, if necessary */
  if( s->has_srid )
  {
    s->srid = clamp_srid(ctx, integer_from_wkb_state(ctx, s));
    /* TODO: warn on explicit UNKNOWN srid ? */
    RTDEBUGF(ctx, 4,"Got SRID: %u", s->srid);
  }

  /* Do the right thing */
  switch( s->rttype )
  {
    case RTPOINTTYPE:
      return (RTGEOM*)rtpoint_from_wkb_state(ctx, s);
      break;
    case RTLINETYPE:
      return (RTGEOM*)rtline_from_wkb_state(ctx, s);
      break;
    case RTCIRCSTRINGTYPE:
      return (RTGEOM*)rtcircstring_from_wkb_state(ctx, s);
      break;
    case RTPOLYGONTYPE:
      return (RTGEOM*)rtpoly_from_wkb_state(ctx, s);
      break;
    case RTTRIANGLETYPE:
      return (RTGEOM*)rttriangle_from_wkb_state(ctx, s);
      break;
    case RTCURVEPOLYTYPE:
      return (RTGEOM*)rtcurvepoly_from_wkb_state(ctx, s);
      break;
    case RTMULTIPOINTTYPE:
    case RTMULTILINETYPE:
    case RTMULTIPOLYGONTYPE:
    case RTCOMPOUNDTYPE:
    case RTMULTICURVETYPE:
    case RTMULTISURFACETYPE:
    case RTPOLYHEDRALSURFACETYPE:
    case RTTINTYPE:
    case RTCOLLECTIONTYPE:
      return (RTGEOM*)rtcollection_from_wkb_state(ctx, s);
      break;

    /* Unknown type! */
    default:
      rterror(ctx, "Unsupported geometry type: %s [%d]", rttype_name(ctx, s->rttype), s->rttype);
  }

  /* Return value to keep compiler happy. */
  return NULL;

}

/* TODO add check for SRID consistency */

/**
* RTWKB inputs *must* have a declared size, to prevent malformed RTWKB from reading
* off the end of the memory segment (this stops a malevolent user from declaring
* a one-ring polygon to have 10 rings, causing the RTWKB reader to walk off the
* end of the memory).
*
* Check is a bitmask of: RT_PARSER_CHECK_MINPOINTS, RT_PARSER_CHECK_ODD,
* RT_PARSER_CHECK_CLOSURE, RT_PARSER_CHECK_NONE, RT_PARSER_CHECK_ALL
*/
RTGEOM* rtgeom_from_wkb(const RTCTX *ctx, const uint8_t *wkb, const size_t wkb_size, const char check)
{
  wkb_parse_state s;

  /* Initialize the state appropriately */
  s.wkb = wkb;
  s.wkb_size = wkb_size;
  s.swap_bytes = RT_FALSE;
  s.check = check;
  s.rttype = 0;
  s.srid = SRID_UNKNOWN;
  s.has_z = RT_FALSE;
  s.has_m = RT_FALSE;
  s.has_srid = RT_FALSE;
  s.pos = wkb;

  /* Hand the check catch-all values */
  if ( check & RT_PARSER_CHECK_NONE )
    s.check = 0;
  else
    s.check = check;

  return rtgeom_from_wkb_state(ctx, &s);
}

RTGEOM* rtgeom_from_hexwkb(const RTCTX *ctx, const char *hexwkb, const char check)
{
  int hexwkb_len;
  uint8_t *wkb;
  RTGEOM *rtgeom;

  if ( ! hexwkb )
  {
    rterror(ctx, "rtgeom_from_hexwkb: null input");
    return NULL;
  }

  hexwkb_len = strlen(hexwkb);
  wkb = bytes_from_hexbytes(ctx, hexwkb, hexwkb_len);
  rtgeom = rtgeom_from_wkb(ctx, wkb, hexwkb_len/2, check);
  rtfree(ctx, wkb);
  return rtgeom;
}
