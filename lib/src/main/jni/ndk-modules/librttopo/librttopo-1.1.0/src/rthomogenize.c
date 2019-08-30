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
 * Copyright 2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <stdlib.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"


typedef struct {
  int cnt[RTNUMTYPES];
  RTCOLLECTION* buf[RTNUMTYPES];
} HomogenizeBuffer;

static void
init_homogenizebuffer(const RTCTX *ctx, HomogenizeBuffer *buffer)
{
  int i;
  for ( i = 0; i < RTNUMTYPES; i++ )
  {
    buffer->cnt[i] = 0;
    buffer->buf[i] = NULL;
  }
}

/*
static void
free_homogenizebuffer(HomogenizeBuffer *buffer)
{
  int i;
  for ( i = 0; i < RTNUMTYPES; i++ )
  {
    if ( buffer->buf[i] )
    {
      rtcollection_free(ctx, buffer->buf[i]);
    }
  }
}
*/

/*
** Given a generic collection, return the "simplest" form.
**
** eg: GEOMETRYCOLLECTION(MULTILINESTRING()) => MULTILINESTRING()
**
**     GEOMETRYCOLLECTION(MULTILINESTRING(), MULTILINESTRING(), POINT())
**      => GEOMETRYCOLLECTION(MULTILINESTRING(), POINT())
**
** In general, if the subcomponents are homogeneous, return a properly
** typed collection.
** Otherwise, return a generic collection, with the subtypes in minimal
** typed collections.
*/
static void
rtcollection_build_buffer(const RTCTX *ctx, const RTCOLLECTION *col, HomogenizeBuffer *buffer)
{
  int i;

  if ( ! col ) return;
  if ( rtgeom_is_empty(ctx, rtcollection_as_rtgeom(ctx, col)) ) return;
  for ( i = 0; i < col->ngeoms; i++ )
  {
    RTGEOM *geom = col->geoms[i];
    switch(geom->type)
    {
      case RTPOINTTYPE:
      case RTLINETYPE:
      case RTCIRCSTRINGTYPE:
      case RTCOMPOUNDTYPE:
      case RTTRIANGLETYPE:
      case RTCURVEPOLYTYPE:
      case RTPOLYGONTYPE:
      {
        /* Init if necessary */
        if ( ! buffer->buf[geom->type] )
        {
          RTCOLLECTION *bufcol = rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, col->srid, RTFLAGS_GET_Z(col->flags), RTFLAGS_GET_M(col->flags));
          bufcol->type = rttype_get_collectiontype(ctx, geom->type);
          buffer->buf[geom->type] = bufcol;
        }
        /* Add sub-geom to buffer */
        rtcollection_add_rtgeom(ctx, buffer->buf[geom->type], rtgeom_clone(ctx, geom));
        /* Increment count for this singleton type */
        buffer->cnt[geom->type] = buffer->cnt[geom->type] + 1;
      }
      default:
      {
        rtcollection_build_buffer(ctx, rtgeom_as_rtcollection(ctx, geom), buffer);
      }
    }
  }
  return;
}

static RTGEOM*
rtcollection_homogenize(const RTCTX *ctx, const RTCOLLECTION *col)
{
  int i;
  int ntypes = 0;
  int type = 0;
  RTGEOM *outgeom = NULL;

  HomogenizeBuffer buffer;

  /* Sort all the parts into a buffer */
  init_homogenizebuffer(ctx, &buffer);
  rtcollection_build_buffer(ctx, col, &buffer);

  /* Check for homogeneity */
  for ( i = 0; i < RTNUMTYPES; i++ )
  {
    if ( buffer.cnt[i] > 0 )
    {
      ntypes++;
      type = i;
    }
  }

  /* No types? Huh. Return empty. */
  if ( ntypes == 0 )
  {
    RTCOLLECTION *outcol;
    outcol = rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, col->srid, RTFLAGS_GET_Z(col->flags), RTFLAGS_GET_M(col->flags));
    outgeom = rtcollection_as_rtgeom(ctx, outcol);
  }
  /* One type, return homogeneous collection */
  else if ( ntypes == 1 )
  {
    RTCOLLECTION *outcol;
    outcol = buffer.buf[type];
    if ( outcol->ngeoms == 1 )
    {
      outgeom = outcol->geoms[0];
      outcol->ngeoms=0; rtcollection_free(ctx, outcol);
    }
    else
    {
      outgeom = rtcollection_as_rtgeom(ctx, outcol);
    }
    outgeom->srid = col->srid;
  }
  /* Bah, more than out type, return anonymous collection */
  else if ( ntypes > 1 )
  {
    int j;
    RTCOLLECTION *outcol;
    outcol = rtcollection_construct_empty(ctx, RTCOLLECTIONTYPE, col->srid, RTFLAGS_GET_Z(col->flags), RTFLAGS_GET_M(col->flags));
    for ( j = 0; j < RTNUMTYPES; j++ )
    {
      if ( buffer.buf[j] )
      {
        RTCOLLECTION *bcol = buffer.buf[j];
        if ( bcol->ngeoms == 1 )
        {
          rtcollection_add_rtgeom(ctx, outcol, bcol->geoms[0]);
          bcol->ngeoms=0; rtcollection_free(ctx, bcol);
        }
        else
        {
          rtcollection_add_rtgeom(ctx, outcol, rtcollection_as_rtgeom(ctx, bcol));
        }
      }
    }
    outgeom = rtcollection_as_rtgeom(ctx, outcol);
  }

  return outgeom;
}





/*
** Given a generic geometry, return the "simplest" form.
**
** eg:
**     LINESTRING() => LINESTRING()
**
**     MULTILINESTRING(with a single line) => LINESTRING()
**
**     GEOMETRYCOLLECTION(MULTILINESTRING()) => MULTILINESTRING()
**
**     GEOMETRYCOLLECTION(MULTILINESTRING(), MULTILINESTRING(), POINT())
**      => GEOMETRYCOLLECTION(MULTILINESTRING(), POINT())
*/
RTGEOM *
rtgeom_homogenize(const RTCTX *ctx, const RTGEOM *geom)
{
  RTGEOM *hgeom;

  /* EMPTY Geometry */
  if (rtgeom_is_empty(ctx, geom))
  {
    if( rtgeom_is_collection(ctx, geom) )
    {
      return rtcollection_as_rtgeom(ctx, rtcollection_construct_empty(ctx, geom->type, geom->srid, rtgeom_has_z(ctx, geom), rtgeom_has_m(ctx, geom)));
    }

    return rtgeom_clone(ctx, geom);
  }

  switch (geom->type)
  {

    /* Return simple geometries untouched */
    case RTPOINTTYPE:
    case RTLINETYPE:
    case RTCIRCSTRINGTYPE:
    case RTCOMPOUNDTYPE:
    case RTTRIANGLETYPE:
    case RTCURVEPOLYTYPE:
    case RTPOLYGONTYPE:
      return rtgeom_clone(ctx, geom);

    /* Process homogeneous geometries lightly */
    case RTMULTIPOINTTYPE:
    case RTMULTILINETYPE:
    case RTMULTIPOLYGONTYPE:
    case RTMULTICURVETYPE:
    case RTMULTISURFACETYPE:
    case RTPOLYHEDRALSURFACETYPE:
    case RTTINTYPE:
    {
      RTCOLLECTION *col = (RTCOLLECTION*)geom;

      /* Strip single-entry multi-geometries down to singletons */
      if ( col->ngeoms == 1 )
      {
        hgeom = rtgeom_clone(ctx, (RTGEOM*)(col->geoms[0]));
        hgeom->srid = geom->srid;
        if (geom->bbox)
          hgeom->bbox = gbox_copy(ctx, geom->bbox);
        return hgeom;
      }

      /* Return proper multigeometry untouched */
      return rtgeom_clone(ctx, geom);
    }

    /* Work on anonymous collections separately */
    case RTCOLLECTIONTYPE:
      return rtcollection_homogenize(ctx, (RTCOLLECTION *) geom);
  }

  /* Unknown type */
  rterror(ctx, "rtgeom_homogenize: Geometry Type not supported (%i)",
          rttype_name(ctx, geom->type));

  return NULL; /* Never get here! */
}
