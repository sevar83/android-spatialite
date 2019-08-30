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


RTPSURFACE* rtpsurface_add_rtpoly(const RTCTX *ctx, RTPSURFACE *mobj, const RTPOLY *obj)
{
  return (RTPSURFACE*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}


void rtpsurface_free(const RTCTX *ctx, RTPSURFACE *psurf)
{
  int i;
  if ( ! psurf ) return;
  if ( psurf->bbox )
    rtfree(ctx, psurf->bbox);

  for ( i = 0; i < psurf->ngeoms; i++ )
    if ( psurf->geoms && psurf->geoms[i] )
      rtpoly_free(ctx, psurf->geoms[i]);

  if ( psurf->geoms )
    rtfree(ctx, psurf->geoms);

  rtfree(ctx, psurf);
}


void printRTPSURFACE(const RTCTX *ctx, RTPSURFACE *psurf)
{
  int i, j;
  RTPOLY *patch;

  if (psurf->type != RTPOLYHEDRALSURFACETYPE)
    rterror(ctx, "printRTPSURFACE called with something else than a POLYHEDRALSURFACE");

  rtnotice(ctx, "RTPSURFACE {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(psurf->flags));
  rtnotice(ctx, "    SRID = %i", (int)psurf->srid);
  rtnotice(ctx, "    ngeoms = %i", (int)psurf->ngeoms);

  for (i=0; i<psurf->ngeoms; i++)
  {
    patch = (RTPOLY *) psurf->geoms[i];
    for (j=0; j<patch->nrings; j++)
    {
      rtnotice(ctx, "    RING # %i :",j);
      printPA(ctx, patch->rings[j]);
    }
  }
  rtnotice(ctx, "}");
}




/*
 * TODO rewrite all this stuff to be based on a truly topological model
 */

struct struct_psurface_arcs
{
  double ax, ay, az;
  double bx, by, bz;
  int cnt, face;
};
typedef struct struct_psurface_arcs *psurface_arcs;

/* We supposed that the geometry is valid
   we could have wrong result if not */
int rtpsurface_is_closed(const RTCTX *ctx, const RTPSURFACE *psurface)
{
  int i, j, k;
  int narcs, carc;
  int found;
  psurface_arcs arcs;
  RTPOINT4D pa, pb;
  RTPOLY *patch;

  /* If surface is not 3D, it's can't be closed */
  if (!RTFLAGS_GET_Z(psurface->flags)) return 0;

  /* If surface is less than 4 faces hard to be closed too */
  if (psurface->ngeoms < 4) return 0;

  /* Max theorical arcs number if no one is shared ... */
  for (i=0, narcs=0 ; i < psurface->ngeoms ; i++)
  {
    patch = (RTPOLY *) psurface->geoms[i];
    narcs += patch->rings[0]->npoints - 1;
  }

  arcs = rtalloc(ctx, sizeof(struct struct_psurface_arcs) * narcs);
  for (i=0, carc=0; i < psurface->ngeoms ; i++)
  {

    patch = (RTPOLY *) psurface->geoms[i];
    for (j=0; j < patch->rings[0]->npoints - 1; j++)
    {

      rt_getPoint4d_p(ctx, patch->rings[0], j,   &pa);
      rt_getPoint4d_p(ctx, patch->rings[0], j+1, &pb);

      /* remove redundant points if any */
      if (pa.x == pb.x && pa.y == pb.y && pa.z == pb.z) continue;

      /* Make sure to order the 'lower' point first */
      if ( (pa.x > pb.x) ||
              (pa.x == pb.x && pa.y > pb.y) ||
              (pa.x == pb.x && pa.y == pb.y && pa.z > pb.z) )
      {
        pa = pb;
        rt_getPoint4d_p(ctx, patch->rings[0], j, &pb);
      }

      for (found=0, k=0; k < carc ; k++)
      {

        if (  ( arcs[k].ax == pa.x && arcs[k].ay == pa.y &&
                arcs[k].az == pa.z && arcs[k].bx == pb.x &&
                arcs[k].by == pb.y && arcs[k].bz == pb.z &&
                arcs[k].face != i) )
        {
          arcs[k].cnt++;
          found = 1;

          /* Look like an invalid PolyhedralSurface
                anyway not a closed one */
          if (arcs[k].cnt > 2)
          {
            rtfree(ctx, arcs);
            return 0;
          }
        }
      }

      if (!found)
      {
        arcs[carc].cnt=1;
        arcs[carc].face=i;
        arcs[carc].ax = pa.x;
        arcs[carc].ay = pa.y;
        arcs[carc].az = pa.z;
        arcs[carc].bx = pb.x;
        arcs[carc].by = pb.y;
        arcs[carc].bz = pb.z;
        carc++;

        /* Look like an invalid PolyhedralSurface
              anyway not a closed one */
        if (carc > narcs)
        {
          rtfree(ctx, arcs);
          return 0;
        }
      }
    }
  }

  /* A polyhedron is closed if each edge
         is shared by exactly 2 faces */
  for (k=0; k < carc ; k++)
  {
    if (arcs[k].cnt != 2)
    {
      rtfree(ctx, arcs);
      return 0;
    }
  }
  rtfree(ctx, arcs);

  /* Invalid Polyhedral case */
  if (carc < psurface->ngeoms) return 0;

  return 1;
}
