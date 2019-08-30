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
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"



int
rtcompound_is_closed(const RTCTX *ctx, const RTCOMPOUND *compound)
{
  size_t size;
  int npoints=0;

  if ( rtgeom_has_z(ctx, (RTGEOM*)compound) )
  {
    size = sizeof(POINT3D);
  }
  else
  {
    size = sizeof(RTPOINT2D);
  }

  if ( compound->geoms[compound->ngeoms - 1]->type == RTCIRCSTRINGTYPE )
  {
    npoints = ((RTCIRCSTRING *)compound->geoms[compound->ngeoms - 1])->points->npoints;
  }
  else if (compound->geoms[compound->ngeoms - 1]->type == RTLINETYPE)
  {
    npoints = ((RTLINE *)compound->geoms[compound->ngeoms - 1])->points->npoints;
  }

  if ( memcmp(rt_getPoint_internal(ctx,  (RTPOINTARRAY *)compound->geoms[0]->data, 0),
              rt_getPoint_internal(ctx,  (RTPOINTARRAY *)compound->geoms[compound->ngeoms - 1]->data,
                                 npoints - 1),
              size) )
  {
    return RT_FALSE;
  }

  return RT_TRUE;
}

double rtcompound_length(const RTCTX *ctx, const RTCOMPOUND *comp)
{
  double length = 0.0;
  RTLINE *line;
  if ( rtgeom_is_empty(ctx, (RTGEOM*)comp) )
    return 0.0;
  line = rtcompound_stroke(ctx, comp, 32);
  length = rtline_length(ctx, line);
  rtline_free(ctx, line);
  return length;
}

double rtcompound_length_2d(const RTCTX *ctx, const RTCOMPOUND *comp)
{
  double length = 0.0;
  RTLINE *line;
  if ( rtgeom_is_empty(ctx, (RTGEOM*)comp) )
    return 0.0;
  line = rtcompound_stroke(ctx, comp, 32);
  length = rtline_length_2d(ctx, line);
  rtline_free(ctx, line);
  return length;
}

int rtcompound_add_rtgeom(const RTCTX *ctx, RTCOMPOUND *comp, RTGEOM *geom)
{
  RTCOLLECTION *col = (RTCOLLECTION*)comp;

  /* Empty things can't continuously join up with other things */
  if ( rtgeom_is_empty(ctx, geom) )
  {
    RTDEBUG(ctx, 4, "Got an empty component for a compound curve!");
    return RT_FAILURE;
  }

  if( col->ngeoms > 0 )
  {
    RTPOINT4D last, first;
    /* First point of the component we are adding */
    RTLINE *newline = (RTLINE*)geom;
    /* Last point of the previous component */
    RTLINE *prevline = (RTLINE*)(col->geoms[col->ngeoms-1]);

    rt_getPoint4d_p(ctx, newline->points, 0, &first);
    rt_getPoint4d_p(ctx, prevline->points, prevline->points->npoints-1, &last);

    if ( !(FP_EQUALS(first.x,last.x) && FP_EQUALS(first.y,last.y)) )
    {
      RTDEBUG(ctx, 4, "Components don't join up end-to-end!");
      RTDEBUGF(ctx, 4, "first pt (%g %g %g %g) last pt (%g %g %g %g)", first.x, first.y, first.z, first.m, last.x, last.y, last.z, last.m);
      return RT_FAILURE;
    }
  }

  col = rtcollection_add_rtgeom(ctx, col, geom);
  return RT_SUCCESS;
}

RTCOMPOUND *
rtcompound_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTCOMPOUND *ret = (RTCOMPOUND*)rtcollection_construct_empty(ctx, RTCOMPOUNDTYPE, srid, hasz, hasm);
  return ret;
}

int rtgeom_contains_point(const RTCTX *ctx, const RTGEOM *geom, const RTPOINT2D *pt)
{
  switch( geom->type )
  {
    case RTLINETYPE:
      return ptarray_contains_point(ctx, ((RTLINE*)geom)->points, pt);
    case RTCIRCSTRINGTYPE:
      return ptarrayarc_contains_point(ctx, ((RTCIRCSTRING*)geom)->points, pt);
    case RTCOMPOUNDTYPE:
      return rtcompound_contains_point(ctx, (RTCOMPOUND*)geom, pt);
  }
  rterror(ctx, "rtgeom_contains_point failed");
  return RT_FAILURE;
}

int
rtcompound_contains_point(const RTCTX *ctx, const RTCOMPOUND *comp, const RTPOINT2D *pt)
{
  int i;
  RTLINE *rtline;
  RTCIRCSTRING *rtcirc;
  int wn = 0;
  int winding_number = 0;
  int result;

  for ( i = 0; i < comp->ngeoms; i++ )
  {
    RTGEOM *rtgeom = comp->geoms[i];
    if ( rtgeom->type == RTLINETYPE )
    {
      rtline = rtgeom_as_rtline(ctx, rtgeom);
      if ( comp->ngeoms == 1 )
      {
        return ptarray_contains_point(ctx, rtline->points, pt);
      }
      else
      {
        /* Don't check closure while doing p-i-p test */
        result = ptarray_contains_point_partial(ctx, rtline->points, pt, RT_FALSE, &winding_number);
      }
    }
    else
    {
      rtcirc = rtgeom_as_rtcircstring(ctx, rtgeom);
      if ( ! rtcirc ) {
        rterror(ctx, "Unexpected component of type %s in compound curve", rttype_name(ctx, rtgeom->type));
        return 0;
      }
      if ( comp->ngeoms == 1 )
      {
        return ptarrayarc_contains_point(ctx, rtcirc->points, pt);
      }
      else
      {
        /* Don't check closure while doing p-i-p test */
        result = ptarrayarc_contains_point_partial(ctx, rtcirc->points, pt, RT_FALSE, &winding_number);
      }
    }

    /* Propogate boundary condition */
    if ( result == RT_BOUNDARY )
      return RT_BOUNDARY;

    wn += winding_number;
  }

  /* Outside */
  if (wn == 0)
    return RT_OUTSIDE;

  /* Inside */
  return RT_INSIDE;
}

RTCOMPOUND *
rtcompound_construct_from_rtline(const RTCTX *ctx, const RTLINE *rtline)
{
  RTCOMPOUND* ogeom = rtcompound_construct_empty(ctx, rtline->srid, RTFLAGS_GET_Z(rtline->flags), RTFLAGS_GET_M(rtline->flags));
  rtcompound_add_rtgeom(ctx, ogeom, rtgeom_clone(ctx, (RTGEOM*)rtline));
  /* ogeom->bbox = rtline->bbox; */
  return ogeom;
}

RTPOINT*
rtcompound_get_rtpoint(const RTCTX *ctx, const RTCOMPOUND *rtcmp, int where)
{
  int i;
  int count = 0;
  int npoints = 0;
  if ( rtgeom_is_empty(ctx, (RTGEOM*)rtcmp) )
    return NULL;

  npoints = rtgeom_count_vertices(ctx, (RTGEOM*)rtcmp);
  if ( where < 0 || where >= npoints )
  {
    rterror(ctx, "%s: index %d is not in range of number of vertices (%d) in input", __func__, where, npoints);
    return NULL;
  }

  for ( i = 0; i < rtcmp->ngeoms; i++ )
  {
    RTGEOM* part = rtcmp->geoms[i];
    int npoints_part = rtgeom_count_vertices(ctx, part);
    if ( where >= count && where < count + npoints_part )
    {
      return rtline_get_rtpoint(ctx, (RTLINE*)part, where - count);
    }
    else
    {
      count += npoints_part;
    }
  }

  return NULL;
}



RTPOINT *
rtcompound_get_startpoint(const RTCTX *ctx, const RTCOMPOUND *rtcmp)
{
  return rtcompound_get_rtpoint(ctx, rtcmp, 0);
}

RTPOINT *
rtcompound_get_endpoint(const RTCTX *ctx, const RTCOMPOUND *rtcmp)
{
  RTLINE *rtline;
  if ( rtcmp->ngeoms < 1 )
  {
    return NULL;
  }

  rtline = (RTLINE*)(rtcmp->geoms[rtcmp->ngeoms-1]);

  if ( (!rtline) || (!rtline->points) || (rtline->points->npoints < 1) )
  {
    return NULL;
  }

  return rtline_get_rtpoint(ctx, rtline, rtline->points->npoints-1);
}

