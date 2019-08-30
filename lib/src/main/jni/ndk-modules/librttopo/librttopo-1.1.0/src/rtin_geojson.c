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
 * Copyright 2013 Sandro Santilli <strk@kbt.io>
 * Copyright 2011 Kashif Rasul <kashif.rasul@gmail.com>
 *
 **********************************************************************/



#include <assert.h>
#include "librttopo_geom.h"
#include "rtgeom_log.h"
#include "rttopo_config.h"

#if defined(HAVE_LIBJSON) || defined(HAVE_LIBJSON_C) /* --{ */

#ifdef HAVE_LIBJSON_C
#include <json-c/json.h>
#include <json-c/json_object_private.h>
#else
#include <json/json.h>
#include <json/json_object_private.h>
#endif

#ifndef JSON_C_VERSION
/* Adds support for libjson < 0.10 */
# define json_tokener_error_desc(x) json_tokener_errors[(x)]
#endif

#include <string.h>

static void geojson_rterror(char *msg, int error_code)
{
  RTDEBUGF(ctx, 3, "rtgeom_from_geojson ERROR %i", error_code);
  rterror(ctx, "%s", msg);
}

/* Prototype */
static RTGEOM* parse_geojson(json_object *geojson, int *hasz, int root_srid);

static json_object*
findMemberByName(json_object* poObj, const char* pszName )
{
  json_object* poTmp;
  json_object_iter it;

  poTmp = poObj;

  if( NULL == pszName || NULL == poObj)
    return NULL;

  it.key = NULL;
  it.val = NULL;
  it.entry = NULL;

  if( NULL != json_object_get_object(poTmp) )
  {
    if( NULL == json_object_get_object(poTmp)->head )
    {
      geojson_rterror("invalid GeoJSON representation", 2);
      return NULL;
    }

    for( it.entry = json_object_get_object(poTmp)->head;
            ( it.entry ?
              ( it.key = (char*)it.entry->k,
                it.val = (json_object*)it.entry->v, it.entry) : 0);
            it.entry = it.entry->next)
    {
      if( strcasecmp((char *)it.key, pszName )==0 )
        return it.val;
    }
  }

  return NULL;
}


static int
parse_geojson_coord(json_object *poObj, int *hasz, RTPOINTARRAY *pa)
{
  RTPOINT4D pt;

  RTDEBUGF(ctx, 3, "parse_geojson_coord called for object %s.", json_object_to_json_string( poObj ) );

  if( json_type_array == json_object_get_type( poObj ) )
  {

    json_object* poObjCoord = NULL;
    const int nSize = json_object_array_length( poObj );
    RTDEBUGF(ctx, 3, "parse_geojson_coord called for array size %d.", nSize );

    if ( nSize < 2 )
    {
      geojson_rterror("Too few ordinates in GeoJSON", 4);
      return RT_FAILURE;
    }

    /* Read X coordinate */
    poObjCoord = json_object_array_get_idx( poObj, 0 );
    pt.x = json_object_get_double( poObjCoord );
    RTDEBUGF(ctx, 3, "parse_geojson_coord pt.x = %f.", pt.x );

    /* Read Y coordinate */
    poObjCoord = json_object_array_get_idx( poObj, 1 );
    pt.y = json_object_get_double( poObjCoord );
    RTDEBUGF(ctx, 3, "parse_geojson_coord pt.y = %f.", pt.y );

    if( nSize > 2 ) /* should this be >= 3 ? */
    {
      /* Read Z coordinate */
      poObjCoord = json_object_array_get_idx( poObj, 2 );
      pt.z = json_object_get_double( poObjCoord );
      RTDEBUGF(ctx, 3, "parse_geojson_coord pt.z = %f.", pt.z );
      *hasz = RT_TRUE;
    }
    else if ( nSize == 2 )
    {
      *hasz = RT_FALSE;
      /* Initialize Z coordinate, if required */
      if ( RTFLAGS_GET_Z(pa->flags) ) pt.z = 0.0;
    }
    else
    {
      /* TODO: should we account for nSize > 3 ? */
      /* more than 3 coordinates, we're just dropping dimensions here... */
    }

    /* Initialize M coordinate, if required */
    if ( RTFLAGS_GET_M(pa->flags) ) pt.m = 0.0;

  }
  else
  {
    /* If it's not an array, just don't handle it */
    return RT_FAILURE;
  }

  return ptarray_append_point(ctx, pa, &pt, RT_TRUE);
}

static RTGEOM*
parse_geojson_point(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom;
  RTPOINTARRAY *pa;
  json_object* coords = NULL;

  RTDEBUGF(ctx, 3, "parse_geojson_point called with root_srid = %d.", root_srid );

  coords = findMemberByName( geojson, "coordinates" );
  if ( ! coords )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
    return NULL;
  }

  pa = ptarray_construct_empty(ctx, 1, 0, 1);
  parse_geojson_coord(coords, hasz, pa);

  geom = (RTGEOM *) rtpoint_construct(ctx, root_srid, NULL, pa);
  RTDEBUG(ctx, 2, "parse_geojson_point finished.");
  return geom;
}

static RTGEOM*
parse_geojson_linestring(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom;
  RTPOINTARRAY *pa;
  json_object* points = NULL;
  int i = 0;

  RTDEBUG(ctx, 2, "parse_geojson_linestring called.");

  points = findMemberByName( geojson, "coordinates" );
  if ( ! points )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
  return NULL;
  }

  pa = ptarray_construct_empty(ctx, 1, 0, 1);

  if( json_type_array == json_object_get_type( points ) )
  {
    const int nPoints = json_object_array_length( points );
    for(i = 0; i < nPoints; ++i)
    {
      json_object* coords = NULL;
      coords = json_object_array_get_idx( points, i );
      parse_geojson_coord(coords, hasz, pa);
    }
  }

  geom = (RTGEOM *) rtline_construct(ctx, root_srid, NULL, pa);

  RTDEBUG(ctx, 2, "parse_geojson_linestring finished.");
  return geom;
}

static RTGEOM*
parse_geojson_polygon(json_object *geojson, int *hasz, int root_srid)
{
  RTPOINTARRAY **ppa = NULL;
  json_object* rings = NULL;
  json_object* points = NULL;
  int i = 0, j = 0;
  int nRings = 0, nPoints = 0;

  rings = findMemberByName( geojson, "coordinates" );
  if ( ! rings )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
    return NULL;
  }

  if ( json_type_array != json_object_get_type(rings) )
  {
    geojson_rterror("The 'coordinates' in GeoJSON are not an array", 4);
    return NULL;
  }

  nRings = json_object_array_length( rings );

  /* No rings => POLYGON EMPTY */
  if ( ! nRings )
  {
    return (RTGEOM *)rtpoly_construct_empty(ctx, root_srid, 0, 0);
  }

  for ( i = 0; i < nRings; i++ )
  {
    points = json_object_array_get_idx(rings, i);
    if ( ! points || json_object_get_type(points) != json_type_array )
    {
      geojson_rterror("The 'coordinates' in GeoJSON ring are not an array", 4);
      return NULL;
    }
    nPoints = json_object_array_length(points);

    /* Skip empty rings */
    if ( nPoints == 0 ) continue;

    if ( ! ppa )
      ppa = (RTPOINTARRAY**)rtalloc(ctx, sizeof(RTPOINTARRAY*) * nRings);

    ppa[i] = ptarray_construct_empty(ctx, 1, 0, 1);
    for ( j = 0; j < nPoints; j++ )
    {
      json_object* coords = NULL;
      coords = json_object_array_get_idx( points, j );
      parse_geojson_coord(coords, hasz, ppa[i]);
    }
  }

  /* All the rings were empty! */
  if ( ! ppa )
    return (RTGEOM *)rtpoly_construct_empty(ctx, root_srid, 0, 0);

  return (RTGEOM *) rtpoly_construct(ctx, root_srid, NULL, nRings, ppa);
}

static RTGEOM*
parse_geojson_multipoint(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom;
  int i = 0;
  json_object* poObjPoints = NULL;

  if (!root_srid)
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, root_srid, 1, 0);
  }
  else
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, -1, 1, 0);
  }

  poObjPoints = findMemberByName( geojson, "coordinates" );
  if ( ! poObjPoints )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
    return NULL;
  }

  if( json_type_array == json_object_get_type( poObjPoints ) )
  {
    const int nPoints = json_object_array_length( poObjPoints );
    for( i = 0; i < nPoints; ++i)
    {
      RTPOINTARRAY *pa;
      json_object* poObjCoords = NULL;
      poObjCoords = json_object_array_get_idx( poObjPoints, i );

      pa = ptarray_construct_empty(ctx, 1, 0, 1);
      parse_geojson_coord(poObjCoords, hasz, pa);

      geom = (RTGEOM*)rtmpoint_add_rtpoint(ctx, (RTMPOINT*)geom,
                                           (RTPOINT*)rtpoint_construct(ctx, root_srid, NULL, pa));
    }
  }

  return geom;
}

static RTGEOM*
parse_geojson_multilinestring(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom = NULL;
  int i, j;
  json_object* poObjLines = NULL;

  if (!root_srid)
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTILINETYPE, root_srid, 1, 0);
  }
  else
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTILINETYPE, -1, 1, 0);
  }

  poObjLines = findMemberByName( geojson, "coordinates" );
  if ( ! poObjLines )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
    return NULL;
  }

  if( json_type_array == json_object_get_type( poObjLines ) )
  {
    const int nLines = json_object_array_length( poObjLines );
    for( i = 0; i < nLines; ++i)
    {
      RTPOINTARRAY *pa = NULL;
      json_object* poObjLine = NULL;
      poObjLine = json_object_array_get_idx( poObjLines, i );
      pa = ptarray_construct_empty(ctx, 1, 0, 1);

      if( json_type_array == json_object_get_type( poObjLine ) )
      {
        const int nPoints = json_object_array_length( poObjLine );
        for(j = 0; j < nPoints; ++j)
        {
          json_object* coords = NULL;
          coords = json_object_array_get_idx( poObjLine, j );
          parse_geojson_coord(coords, hasz, pa);
        }

        geom = (RTGEOM*)rtmline_add_rtline(ctx, (RTMLINE*)geom,
                                           (RTLINE*)rtline_construct(ctx, root_srid, NULL, pa));
      }
    }
  }

  return geom;
}

static RTGEOM*
parse_geojson_multipolygon(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom = NULL;
  int i, j, k;
  json_object* poObjPolys = NULL;

  if (!root_srid)
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTIPOLYGONTYPE, root_srid, 1, 0);
  }
  else
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTMULTIPOLYGONTYPE, -1, 1, 0);
  }

  poObjPolys = findMemberByName( geojson, "coordinates" );
  if ( ! poObjPolys )
  {
    geojson_rterror("Unable to find 'coordinates' in GeoJSON string", 4);
    return NULL;
  }

  if( json_type_array == json_object_get_type( poObjPolys ) )
  {
    const int nPolys = json_object_array_length( poObjPolys );

    for(i = 0; i < nPolys; ++i)
    {
      json_object* poObjPoly = json_object_array_get_idx( poObjPolys, i );

      if( json_type_array == json_object_get_type( poObjPoly ) )
      {
        RTPOLY *rtpoly = rtpoly_construct_empty(ctx, geom->srid, rtgeom_has_z(ctx, geom), rtgeom_has_m(ctx, geom));
        int nRings = json_object_array_length( poObjPoly );

        for(j = 0; j < nRings; ++j)
        {
          json_object* points = json_object_array_get_idx( poObjPoly, j );

          if( json_type_array == json_object_get_type( points ) )
          {

            RTPOINTARRAY *pa = ptarray_construct_empty(ctx, 1, 0, 1);

            int nPoints = json_object_array_length( points );
            for ( k=0; k < nPoints; k++ )
            {
              json_object* coords = json_object_array_get_idx( points, k );
              parse_geojson_coord(coords, hasz, pa);
            }

            rtpoly_add_ring(ctx, rtpoly, pa);
          }
        }
        geom = (RTGEOM*)rtmpoly_add_rtpoly(ctx, (RTMPOLY*)geom, rtpoly);
      }
    }
  }

  return geom;
}

static RTGEOM*
parse_geojson_geometrycollection(json_object *geojson, int *hasz, int root_srid)
{
  RTGEOM *geom = NULL;
  int i;
  json_object* poObjGeoms = NULL;

  if (!root_srid)
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, root_srid, 1, 0);
  }
  else
  {
    geom = (RTGEOM *)rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, -1, 1, 0);
  }

  poObjGeoms = findMemberByName( geojson, "geometries" );
  if ( ! poObjGeoms )
  {
    geojson_rterror("Unable to find 'geometries' in GeoJSON string", 4);
    return NULL;
  }

  if( json_type_array == json_object_get_type( poObjGeoms ) )
  {
    const int nGeoms = json_object_array_length( poObjGeoms );
    json_object* poObjGeom = NULL;
    for(i = 0; i < nGeoms; ++i )
    {
      poObjGeom = json_object_array_get_idx( poObjGeoms, i );
      geom = (RTGEOM*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION *)geom,
                                              parse_geojson(poObjGeom, hasz, root_srid));
    }
  }

  return geom;
}

static RTGEOM*
parse_geojson(json_object *geojson, int *hasz, int root_srid)
{
  json_object* type = NULL;
  const char* name;

  if( NULL == geojson )
  {
    geojson_rterror("invalid GeoJSON representation", 2);
    return NULL;
  }

  type = findMemberByName( geojson, "type" );
  if( NULL == type )
  {
    geojson_rterror("unknown GeoJSON type", 3);
    return NULL;
  }

  name = json_object_get_string( type );

  if( strcasecmp( name, "Point" )==0 )
    return parse_geojson_point(geojson, hasz, root_srid);

  if( strcasecmp( name, "LineString" )==0 )
    return parse_geojson_linestring(geojson, hasz, root_srid);

  if( strcasecmp( name, "Polygon" )==0 )
    return parse_geojson_polygon(geojson, hasz, root_srid);

  if( strcasecmp( name, "MultiPoint" )==0 )
    return parse_geojson_multipoint(geojson, hasz, root_srid);

  if( strcasecmp( name, "MultiLineString" )==0 )
    return parse_geojson_multilinestring(geojson, hasz, root_srid);

  if( strcasecmp( name, "MultiPolygon" )==0 )
    return parse_geojson_multipolygon(geojson, hasz, root_srid);

  if( strcasecmp( name, "GeometryCollection" )==0 )
    return parse_geojson_geometrycollection(geojson, hasz, root_srid);

  rterror(ctx, "invalid GeoJson representation");
  return NULL; /* Never reach */
}

#endif /* HAVE_LIBJSON or HAVE_LIBJSON_C --} */

RTGEOM*
rtgeom_from_geojson(const RTCTX *ctx, const char *geojson, char **srs)
{
#ifndef HAVE_LIBJSON
  *srs = NULL;
  rterror(ctx, "You need JSON-C for rtgeom_from_geojson");
  return NULL;
#else /* HAVE_LIBJSON */

  /* size_t geojson_size = strlen(geojson); */

  RTGEOM *rtgeom;
  int hasz=RT_TRUE;
  json_tokener* jstok = NULL;
  json_object* poObj = NULL;
  json_object* poObjSrs = NULL;
  *srs = NULL;

  /* Begin to Parse json */
  jstok = json_tokener_new();
  poObj = json_tokener_parse_ex(jstok, geojson, -1);
  if( jstok->err != json_tokener_success)
  {
    char err[256];
    snprintf(err, 256, "%s (at offset %d)", json_tokener_error_desc(jstok->err), jstok->char_offset);
    json_tokener_free(jstok);
    json_object_put(poObj);
    geojson_rterror(err, 1);
    return NULL;
  }
  json_tokener_free(jstok);

  poObjSrs = findMemberByName( poObj, "crs" );
  if (poObjSrs != NULL)
  {
    json_object* poObjSrsType = findMemberByName( poObjSrs, "type" );
    if (poObjSrsType != NULL)
    {
      json_object* poObjSrsProps = findMemberByName( poObjSrs, "properties" );
      if ( poObjSrsProps )
      {
        json_object* poNameURL = findMemberByName( poObjSrsProps, "name" );
        if ( poNameURL )
        {
          const char* pszName = json_object_get_string( poNameURL );
          if ( pszName )
          {
            *srs = rtalloc(ctx, strlen(pszName) + 1);
            strcpy(*srs, pszName);
          }
        }
      }
    }
  }

  rtgeom = parse_geojson(poObj, &hasz, 0);
  json_object_put(poObj);

  rtgeom_add_bbox(ctx, rtgeom);

  if (!hasz)
  {
    RTGEOM *tmp = rtgeom_force_2d(ctx, rtgeom);
    rtgeom_free(ctx, rtgeom);
    rtgeom = tmp;

    RTDEBUG(ctx, 2, "geom_from_geojson called.");
  }

  return rtgeom;
#endif /* HAVE_LIBJSON } */
}


