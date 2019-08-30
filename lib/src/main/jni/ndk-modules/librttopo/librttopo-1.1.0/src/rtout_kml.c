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
 * Copyright 2006 Corporacion Autonoma Regional de Santander
 * Copyright 2010 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "stringbuffer.h"

static int rtgeom_to_kml2_sb(const RTCTX *ctx, const RTGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb);
static int rtpoint_to_kml2_sb(const RTCTX *ctx, const RTPOINT *point, int precision, const char *prefix, stringbuffer_t *sb);
static int rtline_to_kml2_sb(const RTCTX *ctx, const RTLINE *line, int precision, const char *prefix, stringbuffer_t *sb);
static int rtpoly_to_kml2_sb(const RTCTX *ctx, const RTPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb);
static int rtcollection_to_kml2_sb(const RTCTX *ctx, const RTCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb);
static int ptarray_to_kml2_sb(const RTCTX *ctx, const RTPOINTARRAY *pa, int precision, stringbuffer_t *sb);

/*
* KML 2.2.0
*/

/* takes a GEOMETRY and returns a KML representation */
char*
rtgeom_to_kml2(const RTCTX *ctx, const RTGEOM *geom, int precision, const char *prefix)
{
  stringbuffer_t *sb;
  int rv;
  char *kml;

  /* Can't do anything with empty */
  if( rtgeom_is_empty(ctx, geom) )
    return NULL;

  sb = stringbuffer_create(ctx);
  rv = rtgeom_to_kml2_sb(ctx, geom, precision, prefix, sb);

  if ( rv == RT_FAILURE )
  {
    stringbuffer_destroy(ctx, sb);
    return NULL;
  }

  kml = stringbuffer_getstringcopy(ctx, sb);
  stringbuffer_destroy(ctx, sb);

  return kml;
}

static int
rtgeom_to_kml2_sb(const RTCTX *ctx, const RTGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb)
{
  switch (geom->type)
  {
  case RTPOINTTYPE:
    return rtpoint_to_kml2_sb(ctx, (RTPOINT*)geom, precision, prefix, sb);

  case RTLINETYPE:
    return rtline_to_kml2_sb(ctx, (RTLINE*)geom, precision, prefix, sb);

  case RTPOLYGONTYPE:
    return rtpoly_to_kml2_sb(ctx, (RTPOLY*)geom, precision, prefix, sb);

  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
    return rtcollection_to_kml2_sb(ctx, (RTCOLLECTION*)geom, precision, prefix, sb);

  default:
    rterror(ctx, "rtgeom_to_kml2: '%s' geometry type not supported", rttype_name(ctx, geom->type));
    return RT_FAILURE;
  }
}

static int
ptarray_to_kml2_sb(const RTCTX *ctx, const RTPOINTARRAY *pa, int precision, stringbuffer_t *sb)
{
  int i, j;
  int dims = RTFLAGS_GET_Z(pa->flags) ? 3 : 2;
  RTPOINT4D pt;
  double *d;

  for ( i = 0; i < pa->npoints; i++ )
  {
    rt_getPoint4d_p(ctx, pa, i, &pt);
    d = (double*)(&pt);
    if ( i ) stringbuffer_append(ctx, sb," ");
    for (j = 0; j < dims; j++)
    {
      if ( j ) stringbuffer_append(ctx, sb,",");
      if( fabs(d[j]) < OUT_MAX_DOUBLE )
      {
        if ( stringbuffer_aprintf(ctx, sb, "%.*f", precision, d[j]) < 0 ) return RT_FAILURE;
      }
      else
      {
        if ( stringbuffer_aprintf(ctx, sb, "%g", d[j]) < 0 ) return RT_FAILURE;
      }
      stringbuffer_trim_trailing_zeroes(ctx, sb);
    }
  }
  return RT_SUCCESS;
}


static int
rtpoint_to_kml2_sb(const RTCTX *ctx, const RTPOINT *point, int precision, const char *prefix, stringbuffer_t *sb)
{
  /* Open point */
  if ( stringbuffer_aprintf(ctx, sb, "<%sPoint><%scoordinates>", prefix, prefix) < 0 ) return RT_FAILURE;
  /* Coordinate array */
  if ( ptarray_to_kml2_sb(ctx, point->point, precision, sb) == RT_FAILURE ) return RT_FAILURE;
  /* Close point */
  if ( stringbuffer_aprintf(ctx, sb, "</%scoordinates></%sPoint>", prefix, prefix) < 0 ) return RT_FAILURE;
  return RT_SUCCESS;
}

static int
rtline_to_kml2_sb(const RTCTX *ctx, const RTLINE *line, int precision, const char *prefix, stringbuffer_t *sb)
{
  /* Open linestring */
  if ( stringbuffer_aprintf(ctx, sb, "<%sLineString><%scoordinates>", prefix, prefix) < 0 ) return RT_FAILURE;
  /* Coordinate array */
  if ( ptarray_to_kml2_sb(ctx, line->points, precision, sb) == RT_FAILURE ) return RT_FAILURE;
  /* Close linestring */
  if ( stringbuffer_aprintf(ctx, sb, "</%scoordinates></%sLineString>", prefix, prefix) < 0 ) return RT_FAILURE;

  return RT_SUCCESS;
}

static int
rtpoly_to_kml2_sb(const RTCTX *ctx, const RTPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb)
{
  int i, rv;

  /* Open polygon */
  if ( stringbuffer_aprintf(ctx, sb, "<%sPolygon>", prefix) < 0 ) return RT_FAILURE;
  for ( i = 0; i < poly->nrings; i++ )
  {
    /* Inner or outer ring opening tags */
    if( i )
      rv = stringbuffer_aprintf(ctx, sb, "<%sinnerBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
    else
      rv = stringbuffer_aprintf(ctx, sb, "<%souterBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
    if ( rv < 0 ) return RT_FAILURE;

    /* Coordinate array */
    if ( ptarray_to_kml2_sb(ctx, poly->rings[i], precision, sb) == RT_FAILURE ) return RT_FAILURE;

    /* Inner or outer ring closing tags */
    if( i )
      rv = stringbuffer_aprintf(ctx, sb, "</%scoordinates></%sLinearRing></%sinnerBoundaryIs>", prefix, prefix, prefix);
    else
      rv = stringbuffer_aprintf(ctx, sb, "</%scoordinates></%sLinearRing></%souterBoundaryIs>", prefix, prefix, prefix);
    if ( rv < 0 ) return RT_FAILURE;
  }
  /* Close polygon */
  if ( stringbuffer_aprintf(ctx, sb, "</%sPolygon>", prefix) < 0 ) return RT_FAILURE;

  return RT_SUCCESS;
}

static int
rtcollection_to_kml2_sb(const RTCTX *ctx, const RTCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb)
{
  int i, rv;

  /* Open geometry */
  if ( stringbuffer_aprintf(ctx, sb, "<%sMultiGeometry>", prefix) < 0 ) return RT_FAILURE;
  for ( i = 0; i < col->ngeoms; i++ )
  {
    rv = rtgeom_to_kml2_sb(ctx, col->geoms[i], precision, prefix, sb);
    if ( rv == RT_FAILURE ) return RT_FAILURE;
  }
  /* Close geometry */
  if ( stringbuffer_aprintf(ctx, sb, "</%sMultiGeometry>", prefix) < 0 ) return RT_FAILURE;

  return RT_SUCCESS;
}
