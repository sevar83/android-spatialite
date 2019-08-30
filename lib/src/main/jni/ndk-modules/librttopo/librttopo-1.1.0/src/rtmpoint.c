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
rtmpoint_release(const RTCTX *ctx, RTMPOINT *rtmpoint)
{
  rtgeom_release(ctx, rtmpoint_as_rtgeom(ctx, rtmpoint));
}

RTMPOINT *
rtmpoint_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTMPOINT *ret = (RTMPOINT*)rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, srid, hasz, hasm);
  return ret;
}

RTMPOINT* rtmpoint_add_rtpoint(const RTCTX *ctx, RTMPOINT *mobj, const RTPOINT *obj)
{
  RTDEBUG(ctx, 4, "Called");
  return (RTMPOINT*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}

RTMPOINT *
rtmpoint_construct(const RTCTX *ctx, int srid, const RTPOINTARRAY *pa)
{
  int i;
  int hasz = ptarray_has_z(ctx, pa);
  int hasm = ptarray_has_m(ctx, pa);
  RTMPOINT *ret = (RTMPOINT*)rtcollection_construct_empty(ctx, RTMULTIPOINTTYPE, srid, hasz, hasm);

  for ( i = 0; i < pa->npoints; i++ )
  {
    RTPOINT *rtp;
    RTPOINT4D p;
    rt_getPoint4d_p(ctx, pa, i, &p);
    rtp = rtpoint_make(ctx, srid, hasz, hasm, &p);
    rtmpoint_add_rtpoint(ctx, ret, rtp);
  }

  return ret;
}


void rtmpoint_free(const RTCTX *ctx, RTMPOINT *mpt)
{
  int i;

  if ( ! mpt ) return;

  if ( mpt->bbox )
    rtfree(ctx, mpt->bbox);

  for ( i = 0; i < mpt->ngeoms; i++ )
    if ( mpt->geoms && mpt->geoms[i] )
      rtpoint_free(ctx, mpt->geoms[i]);

  if ( mpt->geoms )
    rtfree(ctx, mpt->geoms);

  rtfree(ctx, mpt);
}

RTGEOM*
rtmpoint_remove_repeated_points(const RTCTX *ctx, const RTMPOINT *mpoint, double tolerance)
{
  uint32_t nnewgeoms;
  uint32_t i, j;
  RTGEOM **newgeoms;

  newgeoms = rtalloc(ctx, sizeof(RTGEOM *)*mpoint->ngeoms);
  nnewgeoms = 0;
  for (i=0; i<mpoint->ngeoms; ++i)
  {
    /* Brute force, may be optimized by building an index */
    int seen=0;
    for (j=0; j<nnewgeoms; ++j)
    {
      if ( rtpoint_same(ctx, (RTPOINT*)newgeoms[j],
                        (RTPOINT*)mpoint->geoms[i]) )
      {
        seen=1;
        break;
      }
    }
    if ( seen ) continue;
    newgeoms[nnewgeoms++] = (RTGEOM*)rtpoint_clone(ctx, mpoint->geoms[i]);
  }

  return (RTGEOM*)rtcollection_construct(ctx, mpoint->type,
                                         mpoint->srid, mpoint->bbox ? gbox_copy(ctx, mpoint->bbox) : NULL,
                                         nnewgeoms, newgeoms);

}

