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
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"
#include "stringbuffer.h"

static void rtgeom_to_wkt_sb(const RTCTX *ctx, const RTGEOM *geom, stringbuffer_t *sb, int precision, uint8_t variant);


/*
* ISO format uses both Z and M qualifiers.
* Extended format only uses an M qualifier for 3DM variants, where it is not
* clear what the third dimension represents.
* SFSQL format never has more than two dimensions, so no qualifiers.
*/
static void dimension_qualifiers_to_wkt_sb(const RTCTX *ctx, const RTGEOM *geom, stringbuffer_t *sb, uint8_t variant)
{

  /* Extended RTWKT: POINTM(0 0 0) */
#if 0
  if ( (variant & RTWKT_EXTENDED) && ! (variant & RTWKT_IS_CHILD) && RTFLAGS_GET_M(geom->flags) && (!RTFLAGS_GET_Z(geom->flags)) )
#else
  if ( (variant & RTWKT_EXTENDED) && RTFLAGS_GET_M(geom->flags) && (!RTFLAGS_GET_Z(geom->flags)) )
#endif
  {
    stringbuffer_append(ctx, sb, "M"); /* "M" */
    return;
  }

  /* ISO RTWKT: POINT ZM (0 0 0 0) */
  if ( (variant & RTWKT_ISO) && (RTFLAGS_NDIMS(geom->flags) > 2) )
  {
    stringbuffer_append(ctx, sb, " ");
    if ( RTFLAGS_GET_Z(geom->flags) )
      stringbuffer_append(ctx, sb, "Z");
    if ( RTFLAGS_GET_M(geom->flags) )
      stringbuffer_append(ctx, sb, "M");
    stringbuffer_append(ctx, sb, " ");
  }
}

/*
* Write an empty token out, padding with a space if
* necessary.
*/
static void empty_to_wkt_sb(const RTCTX *ctx, stringbuffer_t *sb)
{
  if ( ! strchr(" ,(", stringbuffer_lastchar(ctx, sb)) ) /* "EMPTY" */
  {
    stringbuffer_append(ctx, sb, " ");
  }
  stringbuffer_append(ctx, sb, "EMPTY");
}

/*
* Point array is a list of coordinates. Depending on output mode,
* we may suppress some dimensions. ISO and Extended formats include
* all dimensions. Standard OGC output only includes X/Y coordinates.
*/
static void ptarray_to_wkt_sb(const RTCTX *ctx, const RTPOINTARRAY *ptarray, stringbuffer_t *sb, int precision, uint8_t variant)
{
  /* OGC only includes X/Y */
  int dimensions = 2;
  int i, j;

  /* ISO and extended formats include all dimensions */
  if ( variant & ( RTWKT_ISO | RTWKT_EXTENDED ) )
    dimensions = RTFLAGS_NDIMS(ptarray->flags);

  /* Opening paren? */
  if ( ! (variant & RTWKT_NO_PARENS) )
    stringbuffer_append(ctx, sb, "(");

  /* Digits and commas */
  for (i = 0; i < ptarray->npoints; i++)
  {
    double *dbl_ptr = (double*)rt_getPoint_internal(ctx, ptarray, i);

    /* Commas before ever coord but the first */
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");

    for (j = 0; j < dimensions; j++)
    {
      /* Spaces before every ordinate but the first */
      if ( j > 0 )
        stringbuffer_append(ctx, sb, " ");
      stringbuffer_aprintf(ctx, sb, "%.*g", precision, dbl_ptr[j]);
    }
  }

  /* Closing paren? */
  if ( ! (variant & RTWKT_NO_PARENS) )
    stringbuffer_append(ctx, sb, ")");
}

/*
* A four-dimensional point will have different outputs depending on variant.
*   ISO: POINT ZM (0 0 0 0)
*   Extended: POINT(0 0 0 0)
*   OGC: POINT(0 0)
* A three-dimensional m-point will have different outputs too.
*   ISO: POINT M (0 0 0)
*   Extended: POINTM(0 0 0)
*   OGC: POINT(0 0)
*/
static void rtpoint_to_wkt_sb(const RTCTX *ctx, const RTPOINT *pt, stringbuffer_t *sb, int precision, uint8_t variant)
{
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "POINT"); /* "POINT" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)pt, sb, variant);
  }

  if ( rtpoint_is_empty(ctx, pt) )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  ptarray_to_wkt_sb(ctx, pt->point, sb, precision, variant);
}

/*
* LINESTRING(0 0 0, 1 1 1)
*/
static void rtline_to_wkt_sb(const RTCTX *ctx, const RTLINE *line, stringbuffer_t *sb, int precision, uint8_t variant)
{
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "LINESTRING"); /* "LINESTRING" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)line, sb, variant);
  }
  if ( rtline_is_empty(ctx, line) )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  ptarray_to_wkt_sb(ctx, line->points, sb, precision, variant);
}

/*
* POLYGON(0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)
*/
static void rtpoly_to_wkt_sb(const RTCTX *ctx, const RTPOLY *poly, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "POLYGON"); /* "POLYGON" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)poly, sb, variant);
  }
  if ( rtpoly_is_empty(ctx, poly) )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "(");
  for ( i = 0; i < poly->nrings; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    ptarray_to_wkt_sb(ctx, poly->rings[i], sb, precision, variant);
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* CIRCULARSTRING
*/
static void rtcircstring_to_wkt_sb(const RTCTX *ctx, const RTCIRCSTRING *circ, stringbuffer_t *sb, int precision, uint8_t variant)
{
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "CIRCULARSTRING"); /* "CIRCULARSTRING" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)circ, sb, variant);
  }
  if ( rtcircstring_is_empty(ctx, circ) )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  ptarray_to_wkt_sb(ctx, circ->points, sb, precision, variant);
}


/*
* Multi-points do not wrap their sub-members in parens, unlike other multi-geometries.
*   MULTPOINT(0 0, 1 1) instead of MULTIPOINT((0 0),(1 1))
*/
static void rtmpoint_to_wkt_sb(const RTCTX *ctx, const RTMPOINT *mpoint, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "MULTIPOINT"); /* "MULTIPOINT" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)mpoint, sb, variant);
  }
  if ( mpoint->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < mpoint->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* We don't want type strings or parens on our subgeoms */
    rtpoint_to_wkt_sb(ctx, mpoint->geoms[i], sb, precision, variant | RTWKT_NO_PARENS | RTWKT_NO_TYPE );
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* MULTILINESTRING
*/
static void rtmline_to_wkt_sb(const RTCTX *ctx, const RTMLINE *mline, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "MULTILINESTRING"); /* "MULTILINESTRING" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)mline, sb, variant);
  }
  if ( mline->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < mline->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* We don't want type strings on our subgeoms */
    rtline_to_wkt_sb(ctx, mline->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* MULTIPOLYGON
*/
static void rtmpoly_to_wkt_sb(const RTCTX *ctx, const RTMPOLY *mpoly, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "MULTIPOLYGON"); /* "MULTIPOLYGON" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)mpoly, sb, variant);
  }
  if ( mpoly->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < mpoly->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* We don't want type strings on our subgeoms */
    rtpoly_to_wkt_sb(ctx, mpoly->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* Compound curves provide type information for their curved sub-geometries
* but not their linestring sub-geometries.
*   COMPOUNDCURVE((0 0, 1 1), CURVESTRING(1 1, 2 2, 3 3))
*/
static void rtcompound_to_wkt_sb(const RTCTX *ctx, const RTCOMPOUND *comp, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "COMPOUNDCURVE"); /* "COMPOUNDCURVE" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)comp, sb, variant);
  }
  if ( comp->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < comp->ngeoms; i++ )
  {
    int type = comp->geoms[i]->type;
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* Linestring subgeoms don't get type identifiers */
    if ( type == RTLINETYPE )
    {
      rtline_to_wkt_sb(ctx, (RTLINE*)comp->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
    }
    /* But circstring subgeoms *do* get type identifiers */
    else if ( type == RTCIRCSTRINGTYPE )
    {
      rtcircstring_to_wkt_sb(ctx, (RTCIRCSTRING*)comp->geoms[i], sb, precision, variant );
    }
    else
    {
      rterror(ctx, "rtcompound_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(ctx, type));
    }
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* Curve polygons provide type information for their curved rings
* but not their linestring rings.
*   CURVEPOLYGON((0 0, 1 1, 0 1, 0 0), CURVESTRING(0 0, 1 1, 0 1, 0.5 1, 0 0))
*/
static void rtcurvepoly_to_wkt_sb(const RTCTX *ctx, const RTCURVEPOLY *cpoly, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "CURVEPOLYGON"); /* "CURVEPOLYGON" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)cpoly, sb, variant);
  }
  if ( cpoly->nrings < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < cpoly->nrings; i++ )
  {
    int type = cpoly->rings[i]->type;
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    switch (type)
    {
    case RTLINETYPE:
      /* Linestring subgeoms don't get type identifiers */
      rtline_to_wkt_sb(ctx, (RTLINE*)cpoly->rings[i], sb, precision, variant | RTWKT_NO_TYPE );
      break;
    case RTCIRCSTRINGTYPE:
      /* But circstring subgeoms *do* get type identifiers */
      rtcircstring_to_wkt_sb(ctx, (RTCIRCSTRING*)cpoly->rings[i], sb, precision, variant );
      break;
    case RTCOMPOUNDTYPE:
      /* And compoundcurve subgeoms *do* get type identifiers */
      rtcompound_to_wkt_sb(ctx, (RTCOMPOUND*)cpoly->rings[i], sb, precision, variant );
      break;
    default:
      rterror(ctx, "rtcurvepoly_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(ctx, type));
    }
  }
  stringbuffer_append(ctx, sb, ")");
}


/*
* Multi-curves provide type information for their curved sub-geometries
* but not their linear sub-geometries.
*   MULTICURVE((0 0, 1 1), CURVESTRING(0 0, 1 1, 2 2))
*/
static void rtmcurve_to_wkt_sb(const RTCTX *ctx, const RTMCURVE *mcurv, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "MULTICURVE"); /* "MULTICURVE" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)mcurv, sb, variant);
  }
  if ( mcurv->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < mcurv->ngeoms; i++ )
  {
    int type = mcurv->geoms[i]->type;
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    switch (type)
    {
    case RTLINETYPE:
      /* Linestring subgeoms don't get type identifiers */
      rtline_to_wkt_sb(ctx, (RTLINE*)mcurv->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
      break;
    case RTCIRCSTRINGTYPE:
      /* But circstring subgeoms *do* get type identifiers */
      rtcircstring_to_wkt_sb(ctx, (RTCIRCSTRING*)mcurv->geoms[i], sb, precision, variant );
      break;
    case RTCOMPOUNDTYPE:
      /* And compoundcurve subgeoms *do* get type identifiers */
      rtcompound_to_wkt_sb(ctx, (RTCOMPOUND*)mcurv->geoms[i], sb, precision, variant );
      break;
    default:
      rterror(ctx, "rtmcurve_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(ctx, type));
    }
  }
  stringbuffer_append(ctx, sb, ")");
}


/*
* Multi-surfaces provide type information for their curved sub-geometries
* but not their linear sub-geometries.
*   MULTISURFACE(((0 0, 1 1, 1 0, 0 0)), CURVEPOLYGON(CURVESTRING(0 0, 1 1, 2 2, 0 1, 0 0)))
*/
static void rtmsurface_to_wkt_sb(const RTCTX *ctx, const RTMSURFACE *msurf, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "MULTISURFACE"); /* "MULTISURFACE" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)msurf, sb, variant);
  }
  if ( msurf->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
  for ( i = 0; i < msurf->ngeoms; i++ )
  {
    int type = msurf->geoms[i]->type;
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    switch (type)
    {
    case RTPOLYGONTYPE:
      /* Linestring subgeoms don't get type identifiers */
      rtpoly_to_wkt_sb(ctx, (RTPOLY*)msurf->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
      break;
    case RTCURVEPOLYTYPE:
      /* But circstring subgeoms *do* get type identifiers */
      rtcurvepoly_to_wkt_sb(ctx, (RTCURVEPOLY*)msurf->geoms[i], sb, precision, variant);
      break;
    default:
      rterror(ctx, "rtmsurface_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(ctx, type));
    }
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* Geometry collections provide type information for all their curved sub-geometries
* but not their linear sub-geometries.
*   GEOMETRYCOLLECTION(POLYGON((0 0, 1 1, 1 0, 0 0)), CURVEPOLYGON(CURVESTRING(0 0, 1 1, 2 2, 0 1, 0 0)))
*/
static void rtcollection_to_wkt_sb(const RTCTX *ctx, const RTCOLLECTION *collection, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "GEOMETRYCOLLECTION"); /* "GEOMETRYCOLLECTION" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)collection, sb, variant);
  }
  if ( collection->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }
  stringbuffer_append(ctx, sb, "(");
  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are children */
  for ( i = 0; i < collection->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    rtgeom_to_wkt_sb(ctx, (RTGEOM*)collection->geoms[i], sb, precision, variant );
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* TRIANGLE
*/
static void rttriangle_to_wkt_sb(const RTCTX *ctx, const RTTRIANGLE *tri, stringbuffer_t *sb, int precision, uint8_t variant)
{
  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "TRIANGLE"); /* "TRIANGLE" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)tri, sb, variant);
  }
  if ( rttriangle_is_empty(ctx, tri) )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "("); /* Triangles have extraneous brackets */
  ptarray_to_wkt_sb(ctx, tri->points, sb, precision, variant);
  stringbuffer_append(ctx, sb, ")");
}

/*
* TIN
*/
static void rttin_to_wkt_sb(const RTCTX *ctx, const RTTIN *tin, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "TIN"); /* "TIN" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)tin, sb, variant);
  }
  if ( tin->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  stringbuffer_append(ctx, sb, "(");
  for ( i = 0; i < tin->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* We don't want type strings on our subgeoms */
    rttriangle_to_wkt_sb(ctx, tin->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
  }
  stringbuffer_append(ctx, sb, ")");
}

/*
* POLYHEDRALSURFACE
*/
static void rtpsurface_to_wkt_sb(const RTCTX *ctx, const RTPSURFACE *psurf, stringbuffer_t *sb, int precision, uint8_t variant)
{
  int i = 0;

  if ( ! (variant & RTWKT_NO_TYPE) )
  {
    stringbuffer_append(ctx, sb, "POLYHEDRALSURFACE"); /* "POLYHEDRALSURFACE" */
    dimension_qualifiers_to_wkt_sb(ctx, (RTGEOM*)psurf, sb, variant);
  }
  if ( psurf->ngeoms < 1 )
  {
    empty_to_wkt_sb(ctx, sb);
    return;
  }

  variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */

  stringbuffer_append(ctx, sb, "(");
  for ( i = 0; i < psurf->ngeoms; i++ )
  {
    if ( i > 0 )
      stringbuffer_append(ctx, sb, ",");
    /* We don't want type strings on our subgeoms */
    rtpoly_to_wkt_sb(ctx, psurf->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
  }
  stringbuffer_append(ctx, sb, ")");
}


/*
* Generic GEOMETRY
*/
static void rtgeom_to_wkt_sb(const RTCTX *ctx, const RTGEOM *geom, stringbuffer_t *sb, int precision, uint8_t variant)
{
  RTDEBUGF(ctx, 4, "rtgeom_to_wkt_sb: type %s, hasz %d, hasm %d",
    rttype_name(ctx, geom->type), (geom->type),
    RTFLAGS_GET_Z(geom->flags)?1:0, RTFLAGS_GET_M(geom->flags)?1:0);

  switch (geom->type)
  {
  case RTPOINTTYPE:
    rtpoint_to_wkt_sb(ctx, (RTPOINT*)geom, sb, precision, variant);
    break;
  case RTLINETYPE:
    rtline_to_wkt_sb(ctx, (RTLINE*)geom, sb, precision, variant);
    break;
  case RTPOLYGONTYPE:
    rtpoly_to_wkt_sb(ctx, (RTPOLY*)geom, sb, precision, variant);
    break;
  case RTMULTIPOINTTYPE:
    rtmpoint_to_wkt_sb(ctx, (RTMPOINT*)geom, sb, precision, variant);
    break;
  case RTMULTILINETYPE:
    rtmline_to_wkt_sb(ctx, (RTMLINE*)geom, sb, precision, variant);
    break;
  case RTMULTIPOLYGONTYPE:
    rtmpoly_to_wkt_sb(ctx, (RTMPOLY*)geom, sb, precision, variant);
    break;
  case RTCOLLECTIONTYPE:
    rtcollection_to_wkt_sb(ctx, (RTCOLLECTION*)geom, sb, precision, variant);
    break;
  case RTCIRCSTRINGTYPE:
    rtcircstring_to_wkt_sb(ctx, (RTCIRCSTRING*)geom, sb, precision, variant);
    break;
  case RTCOMPOUNDTYPE:
    rtcompound_to_wkt_sb(ctx, (RTCOMPOUND*)geom, sb, precision, variant);
    break;
  case RTCURVEPOLYTYPE:
    rtcurvepoly_to_wkt_sb(ctx, (RTCURVEPOLY*)geom, sb, precision, variant);
    break;
  case RTMULTICURVETYPE:
    rtmcurve_to_wkt_sb(ctx, (RTMCURVE*)geom, sb, precision, variant);
    break;
  case RTMULTISURFACETYPE:
    rtmsurface_to_wkt_sb(ctx, (RTMSURFACE*)geom, sb, precision, variant);
    break;
  case RTTRIANGLETYPE:
    rttriangle_to_wkt_sb(ctx, (RTTRIANGLE*)geom, sb, precision, variant);
    break;
  case RTTINTYPE:
    rttin_to_wkt_sb(ctx, (RTTIN*)geom, sb, precision, variant);
    break;
  case RTPOLYHEDRALSURFACETYPE:
    rtpsurface_to_wkt_sb(ctx, (RTPSURFACE*)geom, sb, precision, variant);
    break;
  default:
    rterror(ctx, "rtgeom_to_wkt_sb: Type %d - %s unsupported.",
            geom->type, rttype_name(ctx, geom->type));
  }
}

/**
* RTWKT emitter function. Allocates a new *char and fills it with the RTWKT
* representation. If size_out is not NULL, it will be set to the size of the
* allocated *char.
*
* @param variant Bitmasked value, accepts one of RTWKT_ISO, RTWKT_SFSQL, RTWKT_EXTENDED.
* @param precision Number of significant digits in the output doubles.
* @param size_out If supplied, will return the size of the returned string,
* including the null terminator.
*/
char* rtgeom_to_wkt(const RTCTX *ctx, const RTGEOM *geom, uint8_t variant, int precision, size_t *size_out)
{
  stringbuffer_t *sb;
  char *str = NULL;
  if ( geom == NULL )
    return NULL;
  sb = stringbuffer_create(ctx);
  /* Extended mode starts with an "SRID=" section for geoms that have one */
  if ( (variant & RTWKT_EXTENDED) && rtgeom_has_srid(ctx, geom) )
  {
    stringbuffer_aprintf(ctx, sb, "SRID=%d;", geom->srid);
  }
  rtgeom_to_wkt_sb(ctx, geom, sb, precision, variant);
  if ( stringbuffer_getstring(ctx, sb) == NULL )
  {
    rterror(ctx, "Uh oh");
    return NULL;
  }
  str = stringbuffer_getstringcopy(ctx, sb);
  if ( size_out )
    *size_out = stringbuffer_getlength(ctx, sb) + 1;
  stringbuffer_destroy(ctx, sb);
  return str;
}

