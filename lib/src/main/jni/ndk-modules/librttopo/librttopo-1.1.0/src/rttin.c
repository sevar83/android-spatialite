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


RTTIN* rttin_add_rttriangle(const RTCTX *ctx, RTTIN *mobj, const RTTRIANGLE *obj)
{
  return (RTTIN*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}

void rttin_free(const RTCTX *ctx, RTTIN *tin)
{
  int i;
  if ( ! tin ) return;
  if ( tin->bbox )
    rtfree(ctx, tin->bbox);

  for ( i = 0; i < tin->ngeoms; i++ )
    if ( tin->geoms && tin->geoms[i] )
      rttriangle_free(ctx, tin->geoms[i]);

  if ( tin->geoms )
    rtfree(ctx, tin->geoms);

  rtfree(ctx, tin);
}


void printRTTIN(const RTCTX *ctx, RTTIN *tin)
{
  int i;
  RTTRIANGLE *triangle;

  if (tin->type != RTTINTYPE)
    rterror(ctx, "printRTTIN called with something else than a TIN");

  rtnotice(ctx, "RTTIN {");
  rtnotice(ctx, "    ndims = %i", (int)RTFLAGS_NDIMS(tin->flags));
  rtnotice(ctx, "    SRID = %i", (int)tin->srid);
  rtnotice(ctx, "    ngeoms = %i", (int)tin->ngeoms);

  for (i=0; i<tin->ngeoms; i++)
  {
    triangle = (RTTRIANGLE *) tin->geoms[i];
    printPA(ctx, triangle->points);
  }
  rtnotice(ctx, "}");
}


/*
 * TODO rewrite all this stuff to be based on a truly topological model
 */

struct struct_tin_arcs
{
  double ax, ay, az;
  double bx, by, bz;
  int cnt, face;
};
typedef struct struct_tin_arcs *tin_arcs;

/* We supposed that the geometry is valid
   we could have wrong result if not */
int rttin_is_closed(const RTCTX *ctx, const RTTIN *tin)
{
  int i, j, k;
  int narcs, carc;
  int found;
  tin_arcs arcs;
  RTPOINT4D pa, pb;
  RTTRIANGLE *patch;

  /* If surface is not 3D, it's can't be closed */
  if (!RTFLAGS_GET_Z(tin->flags)) return 0;

  /* Max theorical arcs number if no one is shared ... */
  narcs = 3 * tin->ngeoms;

  arcs = rtalloc(ctx, sizeof(struct struct_tin_arcs) * narcs);
  for (i=0, carc=0; i < tin->ngeoms ; i++)
  {

    patch = (RTTRIANGLE *) tin->geoms[i];
    for (j=0; j < 3 ; j++)
    {

      rt_getPoint4d_p(ctx, patch->points, j,   &pa);
      rt_getPoint4d_p(ctx, patch->points, j+1, &pb);

      /* Make sure to order the 'lower' point first */
      if ( (pa.x > pb.x) ||
              (pa.x == pb.x && pa.y > pb.y) ||
              (pa.x == pb.x && pa.y == pb.y && pa.z > pb.z) )
      {
        pa = pb;
        rt_getPoint4d_p(ctx, patch->points, j, &pb);
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

          /* Look like an invalid TIN
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

        /* Look like an invalid TIN
              anyway not a closed one */
        if (carc > narcs)
        {
          rtfree(ctx, arcs);
          return 0;
        }
      }
    }
  }

  /* A TIN is closed if each edge
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

  /* Invalid TIN case */
  if (carc < tin->ngeoms) return 0;

  return 1;
}
