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



/* basic RTCURVEPOLY manipulation */

#include "rttopo_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"


RTCURVEPOLY *
rtcurvepoly_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
  RTCURVEPOLY *ret;

  ret = rtalloc(ctx, sizeof(RTCURVEPOLY));
  ret->type = RTCURVEPOLYTYPE;
  ret->flags = gflags(ctx, hasz, hasm, 0);
  ret->srid = srid;
  ret->nrings = 0;
  ret->maxrings = 1; /* Allocate room for sub-members, just in case. */
  ret->rings = rtalloc(ctx, ret->maxrings * sizeof(RTGEOM*));
  ret->bbox = NULL;

  return ret;
}

RTCURVEPOLY *
rtcurvepoly_construct_from_rtpoly(const RTCTX *ctx, RTPOLY *rtpoly)
{
  RTCURVEPOLY *ret;
  int i;
  ret = rtalloc(ctx, sizeof(RTCURVEPOLY));
  ret->type = RTCURVEPOLYTYPE;
  ret->flags = rtpoly->flags;
  ret->srid = rtpoly->srid;
  ret->nrings = rtpoly->nrings;
  ret->maxrings = rtpoly->nrings; /* Allocate room for sub-members, just in case. */
  ret->rings = rtalloc(ctx, ret->maxrings * sizeof(RTGEOM*));
  ret->bbox = rtpoly->bbox ? gbox_clone(ctx, rtpoly->bbox) : NULL;
  for ( i = 0; i < ret->nrings; i++ )
  {
    ret->rings[i] = rtline_as_rtgeom(ctx, rtline_construct(ctx, ret->srid, NULL, ptarray_clone_deep(ctx, rtpoly->rings[i])));
  }
  return ret;
}

int rtcurvepoly_add_ring(const RTCTX *ctx, RTCURVEPOLY *poly, RTGEOM *ring)
{
  int i;

  /* Can't do anything with NULLs */
  if( ! poly || ! ring )
  {
    RTDEBUG(ctx, 4,"NULL inputs!!! quitting");
    return RT_FAILURE;
  }

  /* Check that we're not working with garbage */
  if ( poly->rings == NULL && (poly->nrings || poly->maxrings) )
  {
    RTDEBUG(ctx, 4,"mismatched nrings/maxrings");
    rterror(ctx, "Curvepolygon is in inconsistent state. Null memory but non-zero collection counts.");
  }

  /* Check that we're adding an allowed ring type */
  if ( ! ( ring->type == RTLINETYPE || ring->type == RTCIRCSTRINGTYPE || ring->type == RTCOMPOUNDTYPE ) )
  {
    RTDEBUGF(ctx, 4,"got incorrect ring type: %s",rttype_name(ctx, ring->type));
    return RT_FAILURE;
  }


  /* In case this is a truly empty, make some initial space  */
  if ( poly->rings == NULL )
  {
    poly->maxrings = 2;
    poly->nrings = 0;
    poly->rings = rtalloc(ctx, poly->maxrings * sizeof(RTGEOM*));
  }

  /* Allocate more space if we need it */
  if ( poly->nrings == poly->maxrings )
  {
    poly->maxrings *= 2;
    poly->rings = rtrealloc(ctx, poly->rings, sizeof(RTGEOM*) * poly->maxrings);
  }

  /* Make sure we don't already have a reference to this geom */
  for ( i = 0; i < poly->nrings; i++ )
  {
    if ( poly->rings[i] == ring )
    {
      RTDEBUGF(ctx, 4, "Found duplicate geometry in collection %p == %p", poly->rings[i], ring);
      return RT_SUCCESS;
    }
  }

  /* Add the ring and increment the ring count */
  poly->rings[poly->nrings] = (RTGEOM*)ring;
  poly->nrings++;
  return RT_SUCCESS;
}

/**
 * This should be rewritten to make use of the curve itself.
 */
double
rtcurvepoly_area(const RTCTX *ctx, const RTCURVEPOLY *curvepoly)
{
  double area = 0.0;
  RTPOLY *poly;
  if( rtgeom_is_empty(ctx, (RTGEOM*)curvepoly) )
    return 0.0;
  poly = rtcurvepoly_stroke(ctx, curvepoly, 32);
  area = rtpoly_area(ctx, poly);
  rtpoly_free(ctx, poly);
  return area;
}


double
rtcurvepoly_perimeter(const RTCTX *ctx, const RTCURVEPOLY *poly)
{
  double result=0.0;
  int i;

  for (i=0; i<poly->nrings; i++)
    result += rtgeom_length(ctx, poly->rings[i]);

  return result;
}

double
rtcurvepoly_perimeter_2d(const RTCTX *ctx, const RTCURVEPOLY *poly)
{
  double result=0.0;
  int i;

  for (i=0; i<poly->nrings; i++)
    result += rtgeom_length_2d(ctx, poly->rings[i]);

  return result;
}
