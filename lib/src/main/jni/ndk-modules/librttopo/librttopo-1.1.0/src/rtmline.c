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
 * Copyright 2001-2006 Refractions Research Inc.
 *
 **********************************************************************/



#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"

void
rtmline_release(const RTCTX *ctx, RTMLINE *rtmline)
{
  rtgeom_release(ctx, rtmline_as_rtgeom(ctx, rtmline));
}

RTMLINE *
rtmline_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTMLINE *ret = (RTMLINE*)rtcollection_construct_empty(ctx, RTMULTILINETYPE, srid, hasz, hasm);
  return ret;
}



RTMLINE* rtmline_add_rtline(const RTCTX *ctx, RTMLINE *mobj, const RTLINE *obj)
{
  return (RTMLINE*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
RTMLINE*
rtmline_measured_from_rtmline(const RTCTX *ctx, const RTMLINE *rtmline, double m_start, double m_end)
{
  int i = 0;
  int hasm = 0, hasz = 0;
  double length = 0.0, length_so_far = 0.0;
  double m_range = m_end - m_start;
  RTGEOM **geoms = NULL;

  if ( rtmline->type != RTMULTILINETYPE )
  {
    rterror(ctx, "rtmline_measured_from_lmwline: only multiline types supported");
    return NULL;
  }

  hasz = RTFLAGS_GET_Z(rtmline->flags);
  hasm = 1;

  /* Calculate the total length of the mline */
  for ( i = 0; i < rtmline->ngeoms; i++ )
  {
    RTLINE *rtline = (RTLINE*)rtmline->geoms[i];
    if ( rtline->points && rtline->points->npoints > 1 )
    {
      length += ptarray_length_2d(ctx, rtline->points);
    }
  }

  if ( rtgeom_is_empty(ctx, (RTGEOM*)rtmline) )
  {
    return (RTMLINE*)rtcollection_construct_empty(ctx, RTMULTILINETYPE, rtmline->srid, hasz, hasm);
  }

  geoms = rtalloc(ctx, sizeof(RTGEOM*) * rtmline->ngeoms);

  for ( i = 0; i < rtmline->ngeoms; i++ )
  {
    double sub_m_start, sub_m_end;
    double sub_length = 0.0;
    RTLINE *rtline = (RTLINE*)rtmline->geoms[i];

    if ( rtline->points && rtline->points->npoints > 1 )
    {
      sub_length = ptarray_length_2d(ctx, rtline->points);
    }

    sub_m_start = (m_start + m_range * length_so_far / length);
    sub_m_end = (m_start + m_range * (length_so_far + sub_length) / length);

    geoms[i] = (RTGEOM*)rtline_measured_from_rtline(ctx, rtline, sub_m_start, sub_m_end);

    length_so_far += sub_length;
  }

  return (RTMLINE*)rtcollection_construct(ctx, rtmline->type, rtmline->srid, NULL, rtmline->ngeoms, geoms);
}

void rtmline_free(const RTCTX *ctx, RTMLINE *mline)
{
  int i;
  if ( ! mline ) return;

  if ( mline->bbox )
    rtfree(ctx, mline->bbox);

  for ( i = 0; i < mline->ngeoms; i++ )
    if ( mline->geoms && mline->geoms[i] )
      rtline_free(ctx, mline->geoms[i]);

  if ( mline->geoms )
    rtfree(ctx, mline->geoms);

  rtfree(ctx, mline);
}
