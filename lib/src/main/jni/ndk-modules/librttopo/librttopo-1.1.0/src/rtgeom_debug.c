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
 * Copyright (C) 2004 Refractions Research Inc.
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "rtgeom_log.h"
#include "librttopo_geom.h"

#include <stdio.h>
#include <string.h>

/* Place to hold the ZM string used in other summaries */
static char tflags[6];

static char *
rtgeom_flagchars(const RTCTX *ctx, RTGEOM *rtg)
{
  int flagno = 0;
  if ( RTFLAGS_GET_Z(rtg->flags) ) tflags[flagno++] = 'Z';
  if ( RTFLAGS_GET_M(rtg->flags) ) tflags[flagno++] = 'M';
  if ( RTFLAGS_GET_BBOX(rtg->flags) ) tflags[flagno++] = 'B';
  if ( RTFLAGS_GET_GEODETIC(rtg->flags) ) tflags[flagno++] = 'G';
  if ( rtg->srid != SRID_UNKNOWN ) tflags[flagno++] = 'S';
  tflags[flagno] = '\0';

  RTDEBUGF(ctx, 4, "Flags: %s - returning %p", rtg->flags, tflags);

  return tflags;
}

/*
 * Returns an alloced string containing summary for the RTGEOM object
 */
static char *
rtpoint_summary(const RTCTX *ctx, RTPOINT *point, int offset)
{
  char *result;
  char *pad="";
  char *zmflags = rtgeom_flagchars(ctx, (RTGEOM*)point);

  result = (char *)rtalloc(ctx, 128+offset);

  sprintf(result, "%*.s%s[%s]",
          offset, pad, rttype_name(ctx, point->type),
          zmflags);
  return result;
}

static char *
rtline_summary(const RTCTX *ctx, RTLINE *line, int offset)
{
  char *result;
  char *pad="";
  char *zmflags = rtgeom_flagchars(ctx, (RTGEOM*)line);

  result = (char *)rtalloc(ctx, 128+offset);

  sprintf(result, "%*.s%s[%s] with %d points",
          offset, pad, rttype_name(ctx, line->type),
          zmflags,
          line->points->npoints);
  return result;
}


static char *
rtcollection_summary(const RTCTX *ctx, RTCOLLECTION *col, int offset)
{
  size_t size = 128;
  char *result;
  char *tmp;
  int i;
  static char *nl = "\n";
  char *pad="";
  char *zmflags = rtgeom_flagchars(ctx, (RTGEOM*)col);

  RTDEBUG(ctx, 2, "rtcollection_summary called");

  result = (char *)rtalloc(ctx, size);

  sprintf(result, "%*.s%s[%s] with %d elements\n",
          offset, pad, rttype_name(ctx, col->type),
          zmflags,
          col->ngeoms);

  for (i=0; i<col->ngeoms; i++)
  {
    tmp = rtgeom_summary(ctx, col->geoms[i], offset+2);
    size += strlen(tmp)+1;
    result = rtrealloc(ctx, result, size);

    RTDEBUGF(ctx, 4, "Reallocated %d bytes for result", size);
    if ( i > 0 ) strcat(result,nl);

    strcat(result, tmp);
    rtfree(ctx, tmp);
  }

  RTDEBUG(ctx, 3, "rtcollection_summary returning");

  return result;
}

static char *
rtpoly_summary(const RTCTX *ctx, RTPOLY *poly, int offset)
{
  char tmp[256];
  size_t size = 64*(poly->nrings+1)+128;
  char *result;
  int i;
  char *pad="";
  static char *nl = "\n";
  char *zmflags = rtgeom_flagchars(ctx, (RTGEOM*)poly);

  RTDEBUG(ctx, 2, "rtpoly_summary called");

  result = (char *)rtalloc(ctx, size);

  sprintf(result, "%*.s%s[%s] with %i rings\n",
          offset, pad, rttype_name(ctx, poly->type),
          zmflags,
          poly->nrings);

  for (i=0; i<poly->nrings; i++)
  {
    sprintf(tmp,"%s   ring %i has %i points",
            pad, i, poly->rings[i]->npoints);
    if ( i > 0 ) strcat(result,nl);
    strcat(result,tmp);
  }

  RTDEBUG(ctx, 3, "rtpoly_summary returning");

  return result;
}

char *
rtgeom_summary(const RTCTX *ctx, const RTGEOM *rtgeom, int offset)
{
  char *result;

  switch (rtgeom->type)
  {
  case RTPOINTTYPE:
    return rtpoint_summary(ctx, (RTPOINT *)rtgeom, offset);

  case RTCIRCSTRINGTYPE:
  case RTTRIANGLETYPE:
  case RTLINETYPE:
    return rtline_summary(ctx, (RTLINE *)rtgeom, offset);

  case RTPOLYGONTYPE:
    return rtpoly_summary(ctx, (RTPOLY *)rtgeom, offset);

  case RTTINTYPE:
  case RTMULTISURFACETYPE:
  case RTMULTICURVETYPE:
  case RTCURVEPOLYTYPE:
  case RTCOMPOUNDTYPE:
  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
  case RTCOLLECTIONTYPE:
    return rtcollection_summary(ctx, (RTCOLLECTION *)rtgeom, offset);
  default:
    result = (char *)rtalloc(ctx, 256);
    sprintf(result, "Object is of unknown type: %d",
            rtgeom->type);
    return result;
  }

  return NULL;
}
