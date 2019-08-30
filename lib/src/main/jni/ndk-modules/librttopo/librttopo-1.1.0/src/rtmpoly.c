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


void
rtmpoly_release(const RTCTX *ctx, RTMPOLY *rtmpoly)
{
  rtgeom_release(ctx, rtmpoly_as_rtgeom(ctx, rtmpoly));
}

RTMPOLY *
rtmpoly_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTMPOLY *ret = (RTMPOLY*)rtcollection_construct_empty(ctx, RTMULTIPOLYGONTYPE, srid, hasz, hasm);
  return ret;
}


RTMPOLY* rtmpoly_add_rtpoly(const RTCTX *ctx, RTMPOLY *mobj, const RTPOLY *obj)
{
  return (RTMPOLY*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}


void rtmpoly_free(const RTCTX *ctx, RTMPOLY *mpoly)
{
  int i;
  if ( ! mpoly ) return;
  if ( mpoly->bbox )
    rtfree(ctx, mpoly->bbox);

  for ( i = 0; i < mpoly->ngeoms; i++ )
    if ( mpoly->geoms && mpoly->geoms[i] )
      rtpoly_free(ctx, mpoly->geoms[i]);

  if ( mpoly->geoms )
    rtfree(ctx, mpoly->geoms);

  rtfree(ctx, mpoly);
}

