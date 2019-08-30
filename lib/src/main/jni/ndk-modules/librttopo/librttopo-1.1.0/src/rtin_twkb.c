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
 * Copyright (C) 2014 Nicklas Avén
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <math.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"
#include "varint.h"

#define TWKB_IN_MAXCOORDS 4

/**
* Used for passing the parse state between the parsing functions.
*/
typedef struct
{
  /* Pointers to the bytes */
  uint8_t *twkb; /* Points to start of TWKB */
  uint8_t *twkb_end; /* Points to end of TWKB */
  uint8_t *pos; /* Current read position */

  uint32_t check; /* Simple validity checks on geometries */
  uint32_t rttype; /* Current type we are handling */

  uint8_t has_bbox;
  uint8_t has_size;
  uint8_t has_idlist;
  uint8_t has_z;
  uint8_t has_m;
  uint8_t is_empty;

  /* Precision factors to convert ints to double */
  double factor;
  double factor_z;
  double factor_m;

  uint64_t size;

  /* Info about current geometry */
  uint8_t magic_byte; /* the magic byte contain info about if twkb contain id, size info, bboxes and precision */

  int ndims; /* Number of dimensions */

  int64_t *coords; /* An array to keep delta values from 4 dimensions */

} twkb_parse_state;


/**
* Internal function declarations.
*/
RTGEOM* rtgeom_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s);


/**********************************************************************/

/**
* Check that we are not about to read off the end of the RTWKB
* array.
*/
static inline void twkb_parse_state_advance(const RTCTX *ctx, twkb_parse_state *s, size_t next)
{
  if( (s->pos + next) > s->twkb_end)
  {
    rterror(ctx, "%s: TWKB structure does not match expected size!", __func__);
    // rtnotice(ctx, "TWKB structure does not match expected size!");
  }

  s->pos += next;
}

static inline int64_t twkb_parse_state_varint(const RTCTX *ctx, twkb_parse_state *s)
{
  size_t size;
  int64_t val = varint_s64_decode(ctx, s->pos, s->twkb_end, &size);
  twkb_parse_state_advance(ctx, s, size);
  return val;
}

static inline uint64_t twkb_parse_state_uvarint(const RTCTX *ctx, twkb_parse_state *s)
{
  size_t size;
  uint64_t val = varint_u64_decode(ctx, s->pos, s->twkb_end, &size);
  twkb_parse_state_advance(ctx, s, size);
  return val;
}

static inline double twkb_parse_state_double(const RTCTX *ctx, twkb_parse_state *s, double factor)
{
  size_t size;
  int64_t val = varint_s64_decode(ctx, s->pos, s->twkb_end, &size);
  twkb_parse_state_advance(ctx, s, size);
  return val / factor;
}

static inline void twkb_parse_state_varint_skip(const RTCTX *ctx, twkb_parse_state *s)
{
  size_t size = varint_size(ctx, s->pos, s->twkb_end);

  if ( ! size )
    rterror(ctx, "%s: no varint to skip", __func__);

  twkb_parse_state_advance(ctx, s, size);
  return;
}



static uint32_t rttype_from_twkb_type(const RTCTX *ctx, uint8_t twkb_type)
{
  switch (twkb_type)
  {
    case 1:
      return RTPOINTTYPE;
    case 2:
      return RTLINETYPE;
    case 3:
      return RTPOLYGONTYPE;
    case 4:
      return RTMULTIPOINTTYPE;
    case 5:
      return RTMULTILINETYPE;
    case 6:
      return RTMULTIPOLYGONTYPE;
    case 7:
      return RTCOLLECTIONTYPE;

    default: /* Error! */
      rterror(ctx, "Unknown RTWKB type");
      return 0;
  }
  return 0;
}

/**
* Byte
* Read a byte and advance the parse state forward.
*/
static uint8_t byte_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  uint8_t val = *(s->pos);
  twkb_parse_state_advance(ctx, s, RTWKB_BYTE_SIZE);
  return val;
}


/**
* RTPOINTARRAY
* Read a dynamically sized point array and advance the parse state forward.
*/
static RTPOINTARRAY* ptarray_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s, uint32_t npoints)
{
  RTPOINTARRAY *pa = NULL;
  uint32_t ndims = s->ndims;
  int i;
  double *dlist;

  RTDEBUG(ctx, 2,"Entering ptarray_from_twkb_state");
  RTDEBUGF(ctx, 4,"Pointarray has %d points", npoints);

  /* Empty! */
  if( npoints == 0 )
    return ptarray_construct_empty(ctx, s->has_z, s->has_m, 0);

  pa = ptarray_construct(ctx, s->has_z, s->has_m, npoints);
  dlist = (double*)(pa->serialized_pointlist);
  for( i = 0; i < npoints; i++ )
  {
    int j = 0;
    /* X */
    s->coords[j] += twkb_parse_state_varint(ctx, s);
    dlist[ndims*i + j] = s->coords[j] / s->factor;
    j++;
    /* Y */
    s->coords[j] += twkb_parse_state_varint(ctx, s);
    dlist[ndims*i + j] = s->coords[j] / s->factor;
    j++;
    /* Z */
    if ( s->has_z )
    {
      s->coords[j] += twkb_parse_state_varint(ctx, s);
      dlist[ndims*i + j] = s->coords[j] / s->factor_z;
      j++;
    }
    /* M */
    if ( s->has_m )
    {
      s->coords[j] += twkb_parse_state_varint(ctx, s);
      dlist[ndims*i + j] = s->coords[j] / s->factor_m;
      j++;
    }
  }

  return pa;
}

/**
* POINT
*/
static RTPOINT* rtpoint_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  static uint32_t npoints = 1;
  RTPOINTARRAY *pa;

  RTDEBUG(ctx, 2,"Entering rtpoint_from_twkb_state");

  if ( s->is_empty )
    return rtpoint_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  pa = ptarray_from_twkb_state(ctx, s, npoints);
  return rtpoint_construct(ctx, SRID_UNKNOWN, NULL, pa);
}

/**
* LINESTRING
*/
static RTLINE* rtline_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  uint32_t npoints;
  RTPOINTARRAY *pa;

  RTDEBUG(ctx, 2,"Entering rtline_from_twkb_state");

  if ( s->is_empty )
    return rtline_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  /* Read number of points */
  npoints = twkb_parse_state_uvarint(ctx, s);

  if ( npoints == 0 )
    return rtline_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  /* Read coordinates */
  pa = ptarray_from_twkb_state(ctx, s, npoints);

  if( pa == NULL )
    return rtline_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 2 )
  {
    rterror(ctx, "%s must have at least two points", rttype_name(ctx, s->rttype));
    return NULL;
  }

  return rtline_construct(ctx, SRID_UNKNOWN, NULL, pa);
}

/**
* POLYGON
*/
static RTPOLY* rtpoly_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  uint32_t nrings;
  int i;
  RTPOLY *poly;

  RTDEBUG(ctx, 2,"Entering rtpoly_from_twkb_state");

  if ( s->is_empty )
    return rtpoly_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  /* Read number of rings */
  nrings = twkb_parse_state_uvarint(ctx, s);

  /* Start w/ empty polygon */
  poly = rtpoly_construct_empty(ctx, SRID_UNKNOWN, s->has_z, s->has_m);

  RTDEBUGF(ctx, 4,"Polygon has %d rings", nrings);

  /* Empty polygon? */
  if( nrings == 0 )
    return poly;

  for( i = 0; i < nrings; i++ )
  {
    /* Ret number of points */
    uint32_t npoints = twkb_parse_state_uvarint(ctx, s);
    RTPOINTARRAY *pa = ptarray_from_twkb_state(ctx, s, npoints);

    /* Skip empty rings */
    if( pa == NULL )
      continue;

    /* Force first and last points to be the same. */
    if( ! ptarray_is_closed_2d(ctx, pa) )
    {
      RTPOINT4D pt;
      rt_getPoint4d_p(ctx, pa, 0, &pt);
      ptarray_append_point(ctx, pa, &pt, RT_FALSE);
    }

    /* Check for at least four points. */
    if( s->check & RT_PARSER_CHECK_MINPOINTS && pa->npoints < 4 )
    {
      RTDEBUGF(ctx, 2, "%s must have at least four points in each ring", rttype_name(ctx, s->rttype));
      rterror(ctx, "%s must have at least four points in each ring", rttype_name(ctx, s->rttype));
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
* MULTIPOINT
*/
static RTCOLLECTION* rtmultipoint_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  int ngeoms, i;
  RTGEOM *geom = NULL;
  RTCOLLECTION *col = rtcollection_construct_empty(ctx, s->rttype, SRID_UNKNOWN, s->has_z, s->has_m);

  RTDEBUG(ctx, 2,"Entering rtmultipoint_from_twkb_state");

  if ( s->is_empty )
    return col;

  /* Read number of geometries */
  ngeoms = twkb_parse_state_uvarint(ctx, s);
  RTDEBUGF(ctx, 4,"Number of geometries %d", ngeoms);

  /* It has an idlist, we need to skip that */
  if ( s->has_idlist )
  {
    for ( i = 0; i < ngeoms; i++ )
      twkb_parse_state_varint_skip(ctx, s);
  }

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtpoint_as_rtgeom(ctx, rtpoint_from_twkb_state(ctx, s));
    if ( rtcollection_add_rtgeom(ctx, col, geom) == NULL )
    {
      rterror(ctx, "Unable to add geometry (%p) to collection (%p)", geom, col);
      return NULL;
    }
  }

  return col;
}

/**
* MULTILINESTRING
*/
static RTCOLLECTION* rtmultiline_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  int ngeoms, i;
  RTGEOM *geom = NULL;
  RTCOLLECTION *col = rtcollection_construct_empty(ctx, s->rttype, SRID_UNKNOWN, s->has_z, s->has_m);

  RTDEBUG(ctx, 2,"Entering rtmultilinestring_from_twkb_state");

  if ( s->is_empty )
    return col;

  /* Read number of geometries */
  ngeoms = twkb_parse_state_uvarint(ctx, s);

  RTDEBUGF(ctx, 4,"Number of geometries %d",ngeoms);

  /* It has an idlist, we need to skip that */
  if ( s->has_idlist )
  {
    for ( i = 0; i < ngeoms; i++ )
      twkb_parse_state_varint_skip(ctx, s);
  }

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtline_as_rtgeom(ctx, rtline_from_twkb_state(ctx, s));
    if ( rtcollection_add_rtgeom(ctx, col, geom) == NULL )
    {
      rterror(ctx, "Unable to add geometry (%p) to collection (%p)", geom, col);
      return NULL;
    }
  }

  return col;
}

/**
* MULTIPOLYGON
*/
static RTCOLLECTION* rtmultipoly_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  int ngeoms, i;
  RTGEOM *geom = NULL;
  RTCOLLECTION *col = rtcollection_construct_empty(ctx, s->rttype, SRID_UNKNOWN, s->has_z, s->has_m);

  RTDEBUG(ctx, 2,"Entering rtmultipolygon_from_twkb_state");

  if ( s->is_empty )
    return col;

  /* Read number of geometries */
  ngeoms = twkb_parse_state_uvarint(ctx, s);
  RTDEBUGF(ctx, 4,"Number of geometries %d",ngeoms);

  /* It has an idlist, we need to skip that */
  if ( s->has_idlist )
  {
    for ( i = 0; i < ngeoms; i++ )
      twkb_parse_state_varint_skip(ctx, s);
  }

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtpoly_as_rtgeom(ctx, rtpoly_from_twkb_state(ctx, s));
    if ( rtcollection_add_rtgeom(ctx, col, geom) == NULL )
    {
      rterror(ctx, "Unable to add geometry (%p) to collection (%p)", geom, col);
      return NULL;
    }
  }

  return col;
}


/**
* COLLECTION, RTMULTIPOINTTYPE, RTMULTILINETYPE, RTMULTIPOLYGONTYPE
**/
static RTCOLLECTION* rtcollection_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  int ngeoms, i;
  RTGEOM *geom = NULL;
  RTCOLLECTION *col = rtcollection_construct_empty(ctx, s->rttype, SRID_UNKNOWN, s->has_z, s->has_m);

  RTDEBUG(ctx, 2,"Entering rtcollection_from_twkb_state");

  if ( s->is_empty )
    return col;

  /* Read number of geometries */
  ngeoms = twkb_parse_state_uvarint(ctx, s);

  RTDEBUGF(ctx, 4,"Number of geometries %d",ngeoms);

  /* It has an idlist, we need to skip that */
  if ( s->has_idlist )
  {
    for ( i = 0; i < ngeoms; i++ )
      twkb_parse_state_varint_skip(ctx, s);
  }

  for ( i = 0; i < ngeoms; i++ )
  {
    geom = rtgeom_from_twkb_state(ctx, s);
    if ( rtcollection_add_rtgeom(ctx, col, geom) == NULL )
    {
      rterror(ctx, "Unable to add geometry (%p) to collection (%p)", geom, col);
      return NULL;
    }
  }


  return col;
}


static void header_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  RTDEBUG(ctx, 2,"Entering magicbyte_from_twkb_state");

  uint8_t extended_dims;

  /* Read the first two bytes */
  uint8_t type_precision = byte_from_twkb_state(ctx, s);
  uint8_t metadata = byte_from_twkb_state(ctx, s);

  /* Strip type and precision out of first byte */
  uint8_t type = type_precision & 0x0F;
  int8_t precision = unzigzag8(ctx, (type_precision & 0xF0) >> 4);

  /* Convert TWKB type to internal type */
  s->rttype = rttype_from_twkb_type(ctx, type);

  /* Convert the precision into factor */
  s->factor = pow(10, (double)precision);

  /* Strip metadata flags out of second byte */
  s->has_bbox   =  metadata & 0x01;
  s->has_size   = (metadata & 0x02) >> 1;
  s->has_idlist = (metadata & 0x04) >> 2;
  extended_dims = (metadata & 0x08) >> 3;
  s->is_empty   = (metadata & 0x10) >> 4;

  /* Flag for higher dims means read a third byte */
  if ( extended_dims )
  {
    int8_t precision_z, precision_m;

    extended_dims = byte_from_twkb_state(ctx, s);

    /* Strip Z/M presence and precision from ext byte */
    s->has_z    = (extended_dims & 0x01);
    s->has_m    = (extended_dims & 0x02) >> 1;
    precision_z = (extended_dims & 0x1C) >> 2;
    precision_m = (extended_dims & 0xE0) >> 5;

    /* Convert the precision into factor */
    s->factor_z = pow(10, (double)precision_z);
    s->factor_m = pow(10, (double)precision_m);
  }
  else
  {
    s->has_z = 0;
    s->has_m = 0;
    s->factor_z = 0;
    s->factor_m = 0;
  }

  /* Read the size, if there is one */
  if ( s->has_size )
  {
    s->size = twkb_parse_state_uvarint(ctx, s);
  }

  /* Calculate the number of dimensions */
  s->ndims = 2 + s->has_z + s->has_m;

  return;
}



/**
* Generic handling for TWKB geometries. The front of every TWKB geometry
* (including those embedded in collections) is a type byte and metadata byte,
* then optional size, bbox, etc. Read those, then switch to particular type
* handling code.
*/
RTGEOM* rtgeom_from_twkb_state(const RTCTX *ctx, twkb_parse_state *s)
{
  RTGBOX bbox;
  RTGEOM *geom = NULL;
  uint32_t has_bbox = RT_FALSE;
  int i;

  /* Read the first two bytes, and optional */
  /* extended precision info and optional size info */
  header_from_twkb_state(ctx, s);

  /* Just experienced a geometry header, so now we */
  /* need to reset our coordinate deltas */
  for ( i = 0; i < TWKB_IN_MAXCOORDS; i++ )
  {
    s->coords[i] = 0.0;
  }

  /* Read the bounding box, is there is one */
  if ( s->has_bbox )
  {
    /* Initialize */
    has_bbox = s->has_bbox;
    memset(&bbox, 0, sizeof(RTGBOX));
    bbox.flags = gflags(ctx, s->has_z, s->has_m, 0);

    /* X */
    bbox.xmin = twkb_parse_state_double(ctx, s, s->factor);
    bbox.xmax = bbox.xmin + twkb_parse_state_double(ctx, s, s->factor);
    /* Y */
    bbox.ymin = twkb_parse_state_double(ctx, s, s->factor);
    bbox.ymax = bbox.ymin + twkb_parse_state_double(ctx, s, s->factor);
    /* Z */
    if ( s->has_z )
    {
      bbox.zmin = twkb_parse_state_double(ctx, s, s->factor_z);
      bbox.zmax = bbox.zmin + twkb_parse_state_double(ctx, s, s->factor_z);
    }
    /* M */
    if ( s->has_z )
    {
      bbox.mmin = twkb_parse_state_double(ctx, s, s->factor_m);
      bbox.mmax = bbox.mmin + twkb_parse_state_double(ctx, s, s->factor_m);
    }
  }

  /* Switch to code for the particular type we're dealing with */
  switch( s->rttype )
  {
    case RTPOINTTYPE:
      geom = rtpoint_as_rtgeom(ctx, rtpoint_from_twkb_state(ctx, s));
      break;
    case RTLINETYPE:
      geom = rtline_as_rtgeom(ctx, rtline_from_twkb_state(ctx, s));
      break;
    case RTPOLYGONTYPE:
      geom = rtpoly_as_rtgeom(ctx, rtpoly_from_twkb_state(ctx, s));
      break;
    case RTMULTIPOINTTYPE:
      geom = rtcollection_as_rtgeom(ctx, rtmultipoint_from_twkb_state(ctx, s));
      break;
    case RTMULTILINETYPE:
      geom = rtcollection_as_rtgeom(ctx, rtmultiline_from_twkb_state(ctx, s));
      break;
    case RTMULTIPOLYGONTYPE:
      geom = rtcollection_as_rtgeom(ctx, rtmultipoly_from_twkb_state(ctx, s));
      break;
    case RTCOLLECTIONTYPE:
      geom = rtcollection_as_rtgeom(ctx, rtcollection_from_twkb_state(ctx, s));
      break;
    /* Unknown type! */
    default:
      rterror(ctx, "Unsupported geometry type: %s [%d]", rttype_name(ctx, s->rttype), s->rttype);
      break;
  }

  if ( has_bbox )
  {
    geom->bbox = gbox_clone(ctx, &bbox);
  }

  return geom;
}


/**
* RTWKB inputs *must* have a declared size, to prevent malformed RTWKB from reading
* off the end of the memory segment (this stops a malevolent user from declaring
* a one-ring polygon to have 10 rings, causing the RTWKB reader to walk off the
* end of the memory).
*
* Check is a bitmask of: RT_PARSER_CHECK_MINPOINTS, RT_PARSER_CHECK_ODD,
* RT_PARSER_CHECK_CLOSURE, RT_PARSER_CHECK_NONE, RT_PARSER_CHECK_ALL
*/
RTGEOM* rtgeom_from_twkb(const RTCTX *ctx, uint8_t *twkb, size_t twkb_size, char check)
{
  int64_t coords[TWKB_IN_MAXCOORDS] = {0, 0, 0, 0};
  twkb_parse_state s;

  RTDEBUG(ctx, 2,"Entering rtgeom_from_twkb");
  RTDEBUGF(ctx, 4,"twkb_size: %d",(int) twkb_size);

  /* Zero out the state */
  memset(&s, 0, sizeof(twkb_parse_state));

  /* Initialize the state appropriately */
  s.twkb = s.pos = twkb;
  s.twkb_end = twkb + twkb_size;
  s.check = check;
  s.coords = coords;

  /* Handle the check catch-all values */
  if ( check & RT_PARSER_CHECK_NONE )
    s.check = 0;
  else
    s.check = check;


  /* Read the rest of the geometry */
  return rtgeom_from_twkb_state(ctx, &s);
}
