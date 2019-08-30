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
 * Copyright 2011 Sandro Santilli <strk@kbt.io>
 * Copyright 2010-2012 Oslandia
 * Copyright 2001-2003 Refractions Research Inc.
 *
 **********************************************************************/



/**
* @file GML output routines.
*
**********************************************************************/


#include "rttopo_config.h"
#include <string.h>
#include "librttopo_geom_internal.h"


static size_t asgml2_point_size(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, const char *prefix);
static char * asgml2_point(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, const char *prefix);
static size_t asgml2_line_size(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, const char *prefix);
static char * asgml2_line(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, const char *prefix);
static size_t asgml2_poly_size(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, const char *prefix);
static char * asgml2_poly(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, const char *prefix);
static size_t asgml2_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, const char *prefix);
static char * asgml2_multi(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, const char *prefix);
static size_t asgml2_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, const char *prefix);
static char * asgml2_collection(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, const char *prefix);
static size_t pointArray_toGML2(const RTCTX *ctx, RTPOINTARRAY *pa, char *buf, int precision);

static size_t asgml3_point_size(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_point(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_line_size(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_line(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_circstring(const RTCTX *ctx,  const RTCIRCSTRING *circ, const char *srs, int precision, int opts, const char *prefix, const char *id );
static size_t asgml3_poly_size(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_poly(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, int opts, int is_patch, const char *prefix, const char *id);
static char * asgml3_curvepoly(const RTCTX *ctx, const RTCURVEPOLY* poly, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_triangle_size(const RTCTX *ctx, const RTTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_triangle(const RTCTX *ctx, const RTTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_multi(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_psurface(const RTCTX *ctx, const RTPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_tin(const RTCTX *ctx, const RTTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t asgml3_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_collection(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id);
static char * asgml3_compound(const RTCTX *ctx, const RTCOMPOUND *col, const char *srs, int precision, int opts, const char *prefix, const char *id );
static char * asgml3_multicurve(const RTCTX *ctx,  const RTMCURVE* cur, const char *srs, int precision, int opts, const char *prefix, const char *id );
static char * asgml3_multisurface(const RTCTX *ctx, const RTMSURFACE *sur, const char *srs, int precision, int opts, const char *prefix, const char *id);
static size_t pointArray_toGML3(const RTCTX *ctx, RTPOINTARRAY *pa, char *buf, int precision, int opts);


static size_t pointArray_GMLsize(const RTCTX *ctx, RTPOINTARRAY *pa, int precision);

static char *
gbox_to_gml2(const RTCTX *ctx, const RTGBOX *bbox, const char *srs, int precision, const char *prefix)
{
  int size;
  RTPOINT4D pt;
  RTPOINTARRAY *pa;
  char *ptr, *output;
  size_t prefixlen = strlen(prefix);

  if ( ! bbox )
  {
    size = ( sizeof("<Box>/") + (prefixlen*2) ) * 2;
    if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

    ptr = output = rtalloc(ctx, size);

    ptr += sprintf(ptr, "<%sBox", prefix);

    if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

    ptr += sprintf(ptr, "/>");

    return output;
  }

  pa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(bbox->flags), 0, 2);

  pt.x = bbox->xmin;
  pt.y = bbox->ymin;
  if (RTFLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmin;
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);

  pt.x = bbox->xmax;
  pt.y = bbox->ymax;
  if (RTFLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmax;
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);

  size = pointArray_GMLsize(ctx, pa, precision);
  size += ( sizeof("<Box><coordinates>/") + (prefixlen*2) ) * 2;
  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

  ptr = output = rtalloc(ctx, size);

  if ( srs ) ptr += sprintf(ptr, "<%sBox srsName=\"%s\">", prefix, srs);
  else       ptr += sprintf(ptr, "<%sBox>", prefix);

  ptr += sprintf(ptr, "<%scoordinates>", prefix);
  ptr += pointArray_toGML2(ctx, pa, ptr, precision);
  ptr += sprintf(ptr, "</%scoordinates></%sBox>", prefix, prefix);

  ptarray_free(ctx, pa);

  return output;
}

static char *
gbox_to_gml3(const RTCTX *ctx, const RTGBOX *bbox, const char *srs, int precision, int opts, const char *prefix)
{
  int size;
  RTPOINT4D pt;
  RTPOINTARRAY *pa;
  char *ptr, *output;
  size_t prefixlen = strlen(prefix);
  int dimension = 2;

  if ( ! bbox )
  {
    size = ( sizeof("<Envelope>/") + (prefixlen*2) ) * 2;
    if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

    ptr = output = rtalloc(ctx, size);

    ptr += sprintf(ptr, "<%sEnvelope", prefix);
    if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

    ptr += sprintf(ptr, "/>");

    return output;
  }

  if (RTFLAGS_GET_Z(bbox->flags)) dimension = 3;

  pa = ptarray_construct_empty(ctx, RTFLAGS_GET_Z(bbox->flags), 0, 1);

  pt.x = bbox->xmin;
  pt.y = bbox->ymin;
  if (RTFLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmin;
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);

  size = pointArray_GMLsize(ctx, pa, precision) * 2;
  size += ( sizeof("<Envelope><lowerCorner><upperCorner>//") + (prefixlen*3) ) * 2;
  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
  if ( IS_DIMS(opts) ) size += sizeof(" srsDimension=. .");

  ptr = output = rtalloc(ctx, size);

  ptr += sprintf(ptr, "<%sEnvelope", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if ( IS_DIMS(opts) ) ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
  ptr += sprintf(ptr, ">");

  ptr += sprintf(ptr, "<%slowerCorner>", prefix);
  ptr += pointArray_toGML3(ctx, pa, ptr, precision, opts);
  ptr += sprintf(ptr, "</%slowerCorner>", prefix);

  ptarray_remove_point(ctx, pa, 0);
  pt.x = bbox->xmax;
  pt.y = bbox->ymax;
  if (RTFLAGS_GET_Z(bbox->flags)) pt.z = bbox->zmax;
  ptarray_append_point(ctx, pa, &pt, RT_TRUE);

  ptr += sprintf(ptr, "<%supperCorner>", prefix);
  ptr += pointArray_toGML3(ctx, pa, ptr, precision, opts);
  ptr += sprintf(ptr, "</%supperCorner>", prefix);

  ptr += sprintf(ptr, "</%sEnvelope>", prefix);

  ptarray_free(ctx, pa);

  return output;
}


extern char *
rtgeom_extent_to_gml2(const RTCTX *ctx, const RTGEOM *geom, const char *srs, int precision, const char *prefix)
{
  const RTGBOX* bbox = rtgeom_get_bbox(ctx, geom);
  /*
    if ( ! bbox ) {
      rterror(ctx, "rtgeom_extent_to_gml2: empty geometry doesn't have a bounding box");
      return NULL;
    }
  */
  char *ret = gbox_to_gml2(ctx, bbox, srs, precision, prefix);
  return ret;
}


extern char *
rtgeom_extent_to_gml3(const RTCTX *ctx, const RTGEOM *geom, const char *srs, int precision, int opts, const char *prefix)
{
  const RTGBOX* bbox = rtgeom_get_bbox(ctx, geom);
  /*
    if ( ! bbox ) {
      rterror(ctx, "rtgeom_extent_to_gml3: empty geometry doesn't have a bounding box");
      return NULL;
    }
  */
  return gbox_to_gml3(ctx, bbox, srs, precision, opts, prefix);
}


/**
 *  @brief VERSION GML 2
 *    takes a GEOMETRY and returns a GML2 representation
 */
extern char *
rtgeom_to_gml2(const RTCTX *ctx, const RTGEOM *geom, const char *srs, int precision, const char* prefix)
{
  int type = geom->type;

  /* Return null for empty (#1377) */
  if ( rtgeom_is_empty(ctx, geom) )
    return NULL;

  switch (type)
  {
  case RTPOINTTYPE:
    return asgml2_point(ctx, (RTPOINT*)geom, srs, precision, prefix);

  case RTLINETYPE:
    return asgml2_line(ctx, (RTLINE*)geom, srs, precision, prefix);

  case RTPOLYGONTYPE:
    return asgml2_poly(ctx, (RTPOLY*)geom, srs, precision, prefix);

  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
    return asgml2_multi(ctx, (RTCOLLECTION*)geom, srs, precision, prefix);

  case RTCOLLECTIONTYPE:
    return asgml2_collection(ctx, (RTCOLLECTION*)geom, srs, precision, prefix);

  case RTTRIANGLETYPE:
  case RTPOLYHEDRALSURFACETYPE:
  case RTTINTYPE:
    rterror(ctx, "Cannot convert %s to GML2. Try ST_AsGML(3, <geometry>) to generate GML3.", rttype_name(ctx, type));
    return NULL;

  default:
    rterror(ctx, "rtgeom_to_gml2: '%s' geometry type not supported", rttype_name(ctx, type));
    return NULL;
  }
}

static size_t
asgml2_point_size(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, const char* prefix)
{
  int size;
  size_t prefixlen = strlen(prefix);

  size = pointArray_GMLsize(ctx, point->point, precision);
  size += ( sizeof("<point><coordinates>/") + (prefixlen*2) ) * 2;
  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
  return size;
}

static size_t
asgml2_point_buf(const RTCTX *ctx, const RTPOINT *point, const char *srs, char *output, int precision, const char* prefix)
{
  char *ptr = output;

  ptr += sprintf(ptr, "<%sPoint", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if ( rtpoint_is_empty(ctx, point) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");
  ptr += sprintf(ptr, "<%scoordinates>", prefix);
  ptr += pointArray_toGML2(ctx, point->point, ptr, precision);
  ptr += sprintf(ptr, "</%scoordinates></%sPoint>", prefix, prefix);

  return (ptr-output);
}

static char *
asgml2_point(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, const char *prefix)
{
  char *output;
  int size;

  size = asgml2_point_size(ctx, point, srs, precision, prefix);
  output = rtalloc(ctx, size);
  asgml2_point_buf(ctx, point, srs, output, precision, prefix);
  return output;
}

static size_t
asgml2_line_size(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, const char *prefix)
{
  int size;
  size_t prefixlen = strlen(prefix);

  size = pointArray_GMLsize(ctx, line->points, precision);
  size += ( sizeof("<linestring><coordinates>/") + (prefixlen*2) ) * 2;
  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
  return size;
}

static size_t
asgml2_line_buf(const RTCTX *ctx, const RTLINE *line, const char *srs, char *output, int precision,
                const char *prefix)
{
  char *ptr=output;

  ptr += sprintf(ptr, "<%sLineString", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

  if ( rtline_is_empty(ctx, line) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  ptr += sprintf(ptr, "<%scoordinates>", prefix);
  ptr += pointArray_toGML2(ctx, line->points, ptr, precision);
  ptr += sprintf(ptr, "</%scoordinates></%sLineString>", prefix, prefix);

  return (ptr-output);
}

static char *
asgml2_line(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, const char *prefix)
{
  char *output;
  int size;

  size = asgml2_line_size(ctx, line, srs, precision, prefix);
  output = rtalloc(ctx, size);
  asgml2_line_buf(ctx, line, srs, output, precision, prefix);
  return output;
}

static size_t
asgml2_poly_size(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, const char *prefix)
{
  size_t size;
  int i;
  size_t prefixlen = strlen(prefix);

  size = sizeof("<polygon></polygon>") + prefixlen*2;
  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");
  if ( rtpoly_is_empty(ctx, poly) )
    return size;
  size += ( sizeof("<outerboundaryis><linearring><coordinates>/") + ( prefixlen*3) ) * 2;
  size += ( sizeof("<innerboundaryis><linearring><coordinates>/") + ( prefixlen*2) ) * 2 * poly->nrings;

  for (i=0; i<poly->nrings; i++)
    size += pointArray_GMLsize(ctx, poly->rings[i], precision);

  return size;
}

static size_t
asgml2_poly_buf(const RTCTX *ctx, const RTPOLY *poly, const char *srs, char *output, int precision,
                const char *prefix)
{
  int i;
  char *ptr=output;

  ptr += sprintf(ptr, "<%sPolygon", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if ( rtpoly_is_empty(ctx, poly) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");
  ptr += sprintf(ptr, "<%souterBoundaryIs><%sLinearRing><%scoordinates>",
                 prefix, prefix, prefix);
  ptr += pointArray_toGML2(ctx, poly->rings[0], ptr, precision);
  ptr += sprintf(ptr, "</%scoordinates></%sLinearRing></%souterBoundaryIs>", prefix, prefix, prefix);
  for (i=1; i<poly->nrings; i++)
  {
    ptr += sprintf(ptr, "<%sinnerBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
    ptr += pointArray_toGML2(ctx, poly->rings[i], ptr, precision);
    ptr += sprintf(ptr, "</%scoordinates></%sLinearRing></%sinnerBoundaryIs>", prefix, prefix, prefix);
  }
  ptr += sprintf(ptr, "</%sPolygon>", prefix);

  return (ptr-output);
}

static char *
asgml2_poly(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, const char *prefix)
{
  char *output;
  int size;

  size = asgml2_poly_size(ctx, poly, srs, precision, prefix);
  output = rtalloc(ctx, size);
  asgml2_poly_buf(ctx, poly, srs, output, precision, prefix);
  return output;
}

/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml2_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision,
                  const char *prefix)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);
  RTGEOM *subgeom;

  /* the longest possible multi version */
  size = sizeof("<MultiLineString></MultiLineString>");
  size += 2*prefixlen;

  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
      size += asgml2_point_size(ctx, (RTPOINT*)subgeom, 0, precision, prefix);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      size += ( sizeof("<lineStringMember>/") + prefixlen ) * 2;
      size += asgml2_line_size(ctx, (RTLINE*)subgeom, 0, precision, prefix);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      size += ( sizeof("<polygonMember>/") + prefixlen ) * 2;
      size += asgml2_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, prefix);
    }
  }

  return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_multi_buf(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, char *output,
                 int precision, const char *prefix)
{
  int type = col->type;
  char *ptr, *gmltype;
  int i;
  RTGEOM *subgeom;

  ptr = output;
  gmltype="";

  if   (type == RTMULTIPOINTTYPE)   gmltype = "MultiPoint";
  else if (type == RTMULTILINETYPE)     gmltype = "MultiLineString";
  else if (type == RTMULTIPOLYGONTYPE) gmltype = "MultiPolygon";

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%s%s", prefix, gmltype);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

  if (!col->ngeoms)
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      ptr += sprintf(ptr, "<%spointMember>", prefix);
      ptr += asgml2_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, prefix);
      ptr += sprintf(ptr, "</%spointMember>", prefix);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      ptr += sprintf(ptr, "<%slineStringMember>", prefix);
      ptr += asgml2_line_buf(ctx, (RTLINE*)subgeom, 0, ptr, precision, prefix);
      ptr += sprintf(ptr, "</%slineStringMember>", prefix);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      ptr += sprintf(ptr, "<%spolygonMember>", prefix);
      ptr += asgml2_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, prefix);
      ptr += sprintf(ptr, "</%spolygonMember>", prefix);
    }
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%s%s>", prefix, gmltype);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_multi(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision,
             const char *prefix)
{
  char *gml;
  size_t size;

  size = asgml2_multi_size(ctx, col, srs, precision, prefix);
  gml = rtalloc(ctx, size);
  asgml2_multi_buf(ctx, col, srs, gml, precision, prefix);
  return gml;
}


/*
 * Don't call this with single-geoms!
 */
static size_t
asgml2_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision,
                       const char *prefix)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);
  RTGEOM *subgeom;

  size = sizeof("<MultiGeometry></MultiGeometry>");
  size += (prefixlen * 2);

  if ( srs ) size += strlen(srs) + sizeof(" srsName=..");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];

    size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
    if ( subgeom->type == RTPOINTTYPE)
    {
      size += asgml2_point_size(ctx, (RTPOINT*)subgeom, 0, precision, prefix);
    }
    else if ( subgeom->type == RTLINETYPE)
    {
      size += asgml2_line_size(ctx, (RTLINE*)subgeom, 0, precision, prefix);
    }
    else if ( subgeom->type == RTPOLYGONTYPE)
    {
      size += asgml2_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, prefix);
    }
    else if ( rtgeom_is_collection(ctx, subgeom) )
    {
      size += asgml2_collection_size(ctx, (RTCOLLECTION*)subgeom, 0, precision, prefix);
    }
    else
      rterror(ctx, "asgml2_collection_size: Unable to process geometry type!");
  }


  return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml2_collection_buf(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, char *output, int precision, const char *prefix)
{
  char *ptr;
  int i;
  RTGEOM *subgeom;

  ptr = output;

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%sMultiGeometry", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);

  if (!col->ngeoms)
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];

    ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
    if (subgeom->type == RTPOINTTYPE)
    {
      ptr += asgml2_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, prefix);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      ptr += asgml2_line_buf(ctx, (RTLINE*)subgeom, 0, ptr, precision, prefix);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      ptr += asgml2_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, prefix);
    }
    else if (rtgeom_is_collection(ctx, subgeom))
    {
      if (subgeom->type == RTCOLLECTIONTYPE)
        ptr += asgml2_collection_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, prefix);
      else
        ptr += asgml2_multi_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, prefix);
    }
    ptr += sprintf(ptr, "</%sgeometryMember>", prefix);
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%sMultiGeometry>", prefix);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml2_collection(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision,
                  const char *prefix)
{
  char *gml;
  size_t size;

  size = asgml2_collection_size(ctx, col, srs, precision, prefix);
  gml = rtalloc(ctx, size);
  asgml2_collection_buf(ctx, col, srs, gml, precision, prefix);
  return gml;
}


static size_t
pointArray_toGML2(const RTCTX *ctx, RTPOINTARRAY *pa, char *output, int precision)
{
  int i;
  char *ptr;
  char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
  char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
  char z[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];

  ptr = output;

  if ( ! RTFLAGS_GET_Z(pa->flags) )
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT2D *pt;
      pt = rt_getPoint2d_cp(ctx, pa, i);

      if (fabs(pt->x) < OUT_MAX_DOUBLE)
        sprintf(x, "%.*f", precision, pt->x);
      else
        sprintf(x, "%g", pt->x);
      trim_trailing_zeros(ctx, x);

      if (fabs(pt->y) < OUT_MAX_DOUBLE)
        sprintf(y, "%.*f", precision, pt->y);
      else
        sprintf(y, "%g", pt->y);
      trim_trailing_zeros(ctx, y);

      if ( i ) ptr += sprintf(ptr, " ");
      ptr += sprintf(ptr, "%s,%s", x, y);
    }
  }
  else
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT3DZ *pt;
      pt = rt_getPoint3dz_cp(ctx, pa, i);

      if (fabs(pt->x) < OUT_MAX_DOUBLE)
        sprintf(x, "%.*f", precision, pt->x);
      else
        sprintf(x, "%g", pt->x);
      trim_trailing_zeros(ctx, x);

      if (fabs(pt->y) < OUT_MAX_DOUBLE)
        sprintf(y, "%.*f", precision, pt->y);
      else
        sprintf(y, "%g", pt->y);
      trim_trailing_zeros(ctx, y);

      if (fabs(pt->z) < OUT_MAX_DOUBLE)
        sprintf(z, "%.*f", precision, pt->z);
      else
        sprintf(z, "%g", pt->z);
      trim_trailing_zeros(ctx, z);

      if ( i ) ptr += sprintf(ptr, " ");
      ptr += sprintf(ptr, "%s,%s,%s", x, y, z);
    }
  }

  return ptr-output;
}


/*
 * VERSION GML 3.1.1
 */


/* takes a GEOMETRY and returns a GML representation */
extern char *
rtgeom_to_gml3(const RTCTX *ctx, const RTGEOM *geom, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int type = geom->type;

  /* Return null for empty (#1377) */
  if ( rtgeom_is_empty(ctx, geom) )
    return NULL;

  switch (type)
  {
  case RTPOINTTYPE:
    return asgml3_point(ctx, (RTPOINT*)geom, srs, precision, opts, prefix, id);

  case RTLINETYPE:
    return asgml3_line(ctx, (RTLINE*)geom, srs, precision, opts, prefix, id);

  case RTCIRCSTRINGTYPE:
    return asgml3_circstring(ctx, (RTCIRCSTRING*)geom, srs, precision, opts, prefix, id );

  case RTPOLYGONTYPE:
    return asgml3_poly(ctx, (RTPOLY*)geom, srs, precision, opts, 0, prefix, id);

  case RTCURVEPOLYTYPE:
    return asgml3_curvepoly(ctx, (RTCURVEPOLY*)geom, srs, precision, opts, prefix, id);

  case RTTRIANGLETYPE:
    return asgml3_triangle(ctx, (RTTRIANGLE*)geom, srs, precision, opts, prefix, id);

  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
    return asgml3_multi(ctx, (RTCOLLECTION*)geom, srs, precision, opts, prefix, id);

  case RTPOLYHEDRALSURFACETYPE:
    return asgml3_psurface(ctx, (RTPSURFACE*)geom, srs, precision, opts, prefix, id);

  case RTTINTYPE:
    return asgml3_tin(ctx, (RTTIN*)geom, srs, precision, opts, prefix, id);

  case RTCOLLECTIONTYPE:
    return asgml3_collection(ctx, (RTCOLLECTION*)geom, srs, precision, opts, prefix, id);

  case RTCOMPOUNDTYPE:
    return asgml3_compound(ctx,  (RTCOMPOUND*)geom, srs, precision, opts, prefix, id );

  case RTMULTICURVETYPE:
    return asgml3_multicurve(ctx,  (RTMCURVE*)geom, srs, precision, opts, prefix, id );

  case RTMULTISURFACETYPE:
    return asgml3_multisurface(ctx,  (RTMSURFACE*)geom, srs, precision, opts, prefix, id );

  default:
    rterror(ctx, "rtgeom_to_gml3: '%s' geometry type not supported", rttype_name(ctx, type));
    return NULL;
  }
}

static size_t
asgml3_point_size(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int size;
  size_t prefixlen = strlen(prefix);

  size = pointArray_GMLsize(ctx, point->point, precision);
  size += ( sizeof("<point><pos>/") + (prefixlen*2) ) * 2;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");
  return size;
}

static size_t
asgml3_point_buf(const RTCTX *ctx, const RTPOINT *point, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr = output;
  int dimension=2;

  if (RTFLAGS_GET_Z(point->flags)) dimension = 3;

  ptr += sprintf(ptr, "<%sPoint", prefix);
  if ( srs ) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if ( id )  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
  if ( rtpoint_is_empty(ctx, point) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }

  ptr += sprintf(ptr, ">");
  if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%spos srsDimension=\"%d\">", prefix, dimension);
  else         ptr += sprintf(ptr, "<%spos>", prefix);
  ptr += pointArray_toGML3(ctx, point->point, ptr, precision, opts);
  ptr += sprintf(ptr, "</%spos></%sPoint>", prefix, prefix);

  return (ptr-output);
}

static char *
asgml3_point(const RTCTX *ctx, const RTPOINT *point, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *output;
  int size;

  size = asgml3_point_size(ctx, point, srs, precision, opts, prefix, id);
  output = rtalloc(ctx, size);
  asgml3_point_buf(ctx, point, srs, output, precision, opts, prefix, id);
  return output;
}


static size_t
asgml3_line_size(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int size;
  size_t prefixlen = strlen(prefix);

  size = pointArray_GMLsize(ctx, line->points, precision);
  if ( opts & RT_GML_SHORTLINE )
  {
    size += (
                sizeof("<LineString><posList>/") +
                ( prefixlen * 2 )
            ) * 2;
  }
  else
  {
    size += (
                sizeof("<Curve><segments><LineStringSegment><posList>/") +
                ( prefixlen * 4 )
            ) * 2;
  }
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");
  return size;
}

static size_t
asgml3_line_buf(const RTCTX *ctx, const RTLINE *line, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr=output;
  int dimension=2;
  int shortline = ( opts & RT_GML_SHORTLINE );

  if (RTFLAGS_GET_Z(line->flags)) dimension = 3;

  if ( shortline )
  {
    ptr += sprintf(ptr, "<%sLineString", prefix);
  }
  else
  {
    ptr += sprintf(ptr, "<%sCurve", prefix);
  }

  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

  if ( rtline_is_empty(ctx, line) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  if ( ! shortline )
  {
    ptr += sprintf(ptr, "<%ssegments>", prefix);
    ptr += sprintf(ptr, "<%sLineStringSegment>", prefix);
  }

  if (IS_DIMS(opts))
  {
    ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">",
                   prefix, dimension);
  }
  else
  {
    ptr += sprintf(ptr, "<%sposList>", prefix);
  }

  ptr += pointArray_toGML3(ctx, line->points, ptr, precision, opts);

  ptr += sprintf(ptr, "</%sposList>", prefix);

  if ( shortline )
  {
    ptr += sprintf(ptr, "</%sLineString>", prefix);
  }
  else
  {
    ptr += sprintf(ptr, "</%sLineStringSegment>", prefix);
    ptr += sprintf(ptr, "</%ssegments>", prefix);
    ptr += sprintf(ptr, "</%sCurve>", prefix);
  }

  return (ptr-output);
}

static char *
asgml3_line(const RTCTX *ctx, const RTLINE *line, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *output;
  int size;

  size = asgml3_line_size(ctx, line, srs, precision, opts, prefix, id);
  output = rtalloc(ctx, size);
  asgml3_line_buf(ctx, line, srs, output, precision, opts, prefix, id);
  return output;
}


static size_t
asgml3_circstring_size(const RTCTX *ctx, const RTCIRCSTRING *circ, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int size = pointArray_GMLsize(ctx,  circ->points, precision );
  size_t prefixlen = strlen(prefix);
  size += 2 * ( sizeof( "<Curve><segments>/" ) + 2 * prefixlen );
  size += 2 * ( sizeof( "<ArcString><posList>/" ) + 2 * prefixlen );
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");
  return size;
}

static size_t
asgml3_circstring_buf(const RTCTX *ctx, const RTCIRCSTRING *circ, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char* ptr = output;
  int dimension=2;

  if (RTFLAGS_GET_Z(circ->flags))
  {
    dimension = 3;
  }

  ptr += sprintf(ptr, "<%sCurve", prefix);
  if (srs)
  {
    ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  }
  if (id)
  {
    ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
  }
  ptr += sprintf(ptr, ">");
  ptr += sprintf(ptr, "<%ssegments>", prefix);
  ptr += sprintf(ptr, "<%sArcString>", prefix);
  ptr += sprintf(ptr, "<%sposList", prefix);

  if (IS_DIMS(opts))
  {
    ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
  }
  ptr += sprintf(ptr, ">");

  ptr += pointArray_toGML3(ctx, circ->points, ptr, precision, opts);
  ptr += sprintf(ptr, "</%sposList>", prefix);
  ptr += sprintf(ptr, "</%sArcString>", prefix);
  ptr += sprintf(ptr, "</%ssegments>", prefix);
  ptr += sprintf(ptr, "</%sCurve>", prefix);
  return (ptr-output);
}

static char *
asgml3_circstring(const RTCTX *ctx,  const RTCIRCSTRING *circ, const char *srs, int precision, int opts, const char *prefix, const char *id )
{
  char *output;
  int size;

  size = asgml3_circstring_size(ctx, circ, srs, precision, opts, prefix, id);
  output = rtalloc(ctx,  size );
  asgml3_circstring_buf(ctx, circ, srs, output, precision, opts, prefix, id);
  return output;
}


static size_t
asgml3_poly_size(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  size_t size;
  size_t prefixlen = strlen(prefix);
  int i;

  size = ( sizeof("<PolygonPatch><exterior><LinearRing>///") + (prefixlen*3) ) * 2;
  size += ( sizeof("<interior><LinearRing>//") + (prefixlen*2) ) * 2 * (poly->nrings - 1);
  size += ( sizeof("<posList></posList>") + (prefixlen*2) ) * poly->nrings;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'") * poly->nrings;

  for (i=0; i<poly->nrings; i++)
    size += pointArray_GMLsize(ctx, poly->rings[i], precision);

  return size;
}

static size_t
asgml3_poly_buf(const RTCTX *ctx, const RTPOLY *poly, const char *srs, char *output, int precision, int opts, int is_patch, const char *prefix, const char *id)
{
  int i;
  char *ptr=output;
  int dimension=2;

  if (RTFLAGS_GET_Z(poly->flags)) dimension = 3;
  if (is_patch)
  {
    ptr += sprintf(ptr, "<%sPolygonPatch", prefix);

  }
  else
  {
    ptr += sprintf(ptr, "<%sPolygon", prefix);
  }

  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

  if ( rtpoly_is_empty(ctx, poly) )
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  ptr += sprintf(ptr, "<%sexterior><%sLinearRing>", prefix, prefix);
  if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
  else         ptr += sprintf(ptr, "<%sposList>", prefix);

  ptr += pointArray_toGML3(ctx, poly->rings[0], ptr, precision, opts);
  ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sexterior>",
                 prefix, prefix, prefix);
  for (i=1; i<poly->nrings; i++)
  {
    ptr += sprintf(ptr, "<%sinterior><%sLinearRing>", prefix, prefix);
    if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
    else         ptr += sprintf(ptr, "<%sposList>", prefix);
    ptr += pointArray_toGML3(ctx, poly->rings[i], ptr, precision, opts);
    ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sinterior>",
                   prefix, prefix, prefix);
  }
  if (is_patch) ptr += sprintf(ptr, "</%sPolygonPatch>", prefix);
  else ptr += sprintf(ptr, "</%sPolygon>", prefix);

  return (ptr-output);
}

static char *
asgml3_poly(const RTCTX *ctx, const RTPOLY *poly, const char *srs, int precision, int opts, int is_patch, const char *prefix, const char *id)
{
  char *output;
  int size;

  size = asgml3_poly_size(ctx, poly, srs, precision, opts, prefix, id);
  output = rtalloc(ctx, size);
  asgml3_poly_buf(ctx, poly, srs, output, precision, opts, is_patch, prefix, id);
  return output;
}

static size_t
asgml3_compound_size(const RTCTX *ctx, const RTCOMPOUND *col, const char *srs, int precision, int opts, const char *prefix, const char *id )
{
  int i;
  size_t size;
  RTGEOM *subgeom;
  size_t prefixlen = strlen(prefix);

  size = ( sizeof( "<Curve></Curve>" ) + 2 * prefixlen );

  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

  size += ( sizeof("<segments></segments>") + 2 * prefixlen );

  for(i= 0; i < col->ngeoms; ++i )
  {
    subgeom = col->geoms[i];
    if ( subgeom->type == RTLINETYPE )
    {

      size += sizeof( "<LineStringSegment></LineStringSegment" ) + 2 * prefixlen;
      size += sizeof( "<posList></posList" ) + 2 * prefixlen;
      size += pointArray_GMLsize(ctx,  ((RTLINE*)subgeom)->points, precision );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      size += sizeof( "<ArcString><posList></ArcString></posList>") + 4 * prefixlen;
      size += pointArray_GMLsize(ctx,  ((RTCIRCSTRING*)subgeom)->points, precision );
    }
    else
    {
      continue;
    }
    if (IS_DIMS(opts))
    {
      size += sizeof(" srsDimension='x'");
    }
  }
  return size;
}

static size_t
asgml3_compound_buf(const RTCTX *ctx, const RTCOMPOUND *col, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  RTGEOM *subgeom;
  int i;
  char* ptr = output;
  int dimension=2;

  if (RTFLAGS_GET_Z(col->flags))
  {
    dimension = 3;
  }

  ptr += sprintf( ptr, "<%sCurve", prefix );
  if (srs)
  {
    ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  }
  if (id)
  {
    ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id );
  }
  ptr += sprintf( ptr, ">" );
  ptr += sprintf( ptr, "<%ssegments>", prefix );

  for( i = 0; i < col->ngeoms; ++i )
  {
    subgeom = col->geoms[i];
    if( subgeom->type != RTLINETYPE && subgeom->type != RTCIRCSTRINGTYPE )
    {
      continue;
    }

    if ( subgeom->type == RTLINETYPE )
    {
      ptr += sprintf( ptr, "<%sLineStringSegment><%sposList", prefix, prefix );
      if (IS_DIMS(opts))
      {
        ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
      }
      ptr += sprintf(ptr, ">");
      ptr += pointArray_toGML3(ctx, ((RTCIRCSTRING*)subgeom)->points, ptr, precision, opts);
      ptr += sprintf( ptr, "</%sposList></%sLineStringSegment>", prefix, prefix );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      ptr += sprintf( ptr, "<%sArcString><%sposList" , prefix, prefix );
      if (IS_DIMS(opts))
      {
        ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
      }
      ptr += sprintf(ptr, ">");
      ptr += pointArray_toGML3(ctx, ((RTLINE*)subgeom)->points, ptr, precision, opts);
      ptr += sprintf( ptr, "</%sposList></%sArcString>", prefix, prefix );
    }
  }

  ptr += sprintf( ptr, "</%ssegments>", prefix );
  ptr += sprintf( ptr, "</%sCurve>", prefix );
  return ( ptr - output );
}

static char *
asgml3_compound(const RTCTX *ctx, const RTCOMPOUND *col, const char *srs, int precision, int opts, const char *prefix, const char *id )
{
  char* gml;
  size_t size;

  size = asgml3_compound_size(ctx,  col, srs, precision, opts, prefix, id );
  gml = rtalloc(ctx,  size );
  asgml3_compound_buf(ctx,  col, srs, gml, precision, opts, prefix, id );
  return gml;
}

static size_t asgml3_curvepoly_size(const RTCTX *ctx, const RTCURVEPOLY* poly, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  size_t prefixlen = strlen(prefix);
  RTGEOM* subgeom;
  size_t size = sizeof( "<Polygon></Polygon" ) + 2 * prefixlen;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  int i;

  for( i = 0; i < poly->nrings; ++i )
  {
    if( i == 0 )
    {
      size += sizeof( "<exterior></exterior>" ) + 2 * prefixlen;
    }
    else
    {
      size += sizeof( "<interior></interior>" ) + 2 * prefixlen;
    }
    subgeom = poly->rings[i];

    if ( subgeom->type == RTLINETYPE )
    {
      size += sizeof("<LinearRing></LinearRing>") + 2 * prefixlen;
      size += sizeof("<posList></posList") + 2 * prefixlen;
      if (IS_DIMS(opts))
      {
        size += sizeof(" srsDimension='x'");
      }
      size += pointArray_GMLsize(ctx,  ((RTLINE*)subgeom)->points, precision );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      size += sizeof("<Ring></Ring>") + 2 * prefixlen;
      size += sizeof("<CurveMember></CurveMember>") + 2 * prefixlen;
      size += asgml3_circstring_size(ctx, (RTCIRCSTRING*)subgeom, srs, precision, opts, prefix, id);
    }
    else if( subgeom->type == RTCOMPOUNDTYPE )
    {
      size += sizeof("<Ring></Ring>") + 2 * prefixlen;
      size += sizeof("<curveMember></curveMember>") + 2 * prefixlen;
      size += asgml3_compound_size(ctx,  (RTCOMPOUND*)subgeom, srs, precision, opts, prefix, id );
    }
  }
  return size;
}

static size_t asgml3_curvepoly_buf(const RTCTX *ctx, const RTCURVEPOLY* poly, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  int i;
  RTGEOM* subgeom;
  char *ptr=output;
  int dimension=2;

  if (RTFLAGS_GET_Z(poly->flags))
  {
    dimension = 3;
  }

  ptr += sprintf( ptr, "<%sPolygon", prefix );
  if (srs)
  {
    ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  }
  if (id)
  {
    ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id );
  }
  ptr += sprintf(ptr, ">");

  for( i = 0; i < poly->nrings; ++i )
  {
    if( i == 0 )
    {
      ptr += sprintf( ptr, "<%sexterior>", prefix);
    }
    else
    {
      ptr += sprintf( ptr, "<%sinterior>", prefix);
    }

    subgeom = poly->rings[i];
    if ( subgeom->type == RTLINETYPE )
    {
      ptr += sprintf( ptr, "<%sLinearRing>", prefix );
      ptr += sprintf( ptr, "<%sposList", prefix );
      if (IS_DIMS(opts))
      {
        ptr += sprintf(ptr, " srsDimension=\"%d\"", dimension);
      }
      ptr += sprintf( ptr, ">" );
      ptr += pointArray_toGML3(ctx, ((RTLINE*)subgeom)->points, ptr, precision, opts);
      ptr += sprintf( ptr, "</%sposList>", prefix );
      ptr += sprintf( ptr, "</%sLinearRing>", prefix );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      ptr += sprintf( ptr, "<%sRing>", prefix );
      ptr += sprintf( ptr, "<%scurveMember>", prefix );
      ptr += asgml3_circstring_buf(ctx,  (RTCIRCSTRING*)subgeom, srs, ptr, precision, opts, prefix, id );
      ptr += sprintf( ptr, "</%scurveMember>", prefix );
      ptr += sprintf( ptr, "</%sRing>", prefix );
    }
    else if( subgeom->type == RTCOMPOUNDTYPE )
    {
      ptr += sprintf( ptr, "<%sRing>", prefix );
      ptr += sprintf( ptr, "<%scurveMember>", prefix );
      ptr += asgml3_compound_buf(ctx,  (RTCOMPOUND*)subgeom, srs, ptr, precision, opts, prefix, id );
      ptr += sprintf( ptr, "</%scurveMember>", prefix );
      ptr += sprintf( ptr, "</%sRing>", prefix );
    }

    if( i == 0 )
    {
      ptr += sprintf( ptr, "</%sexterior>", prefix);
    }
    else
    {
      ptr += sprintf( ptr, "</%sinterior>", prefix);
    }
  }

  ptr += sprintf( ptr, "</%sPolygon>", prefix );
  return (ptr - output);
}

static char* asgml3_curvepoly(const RTCTX *ctx, const RTCURVEPOLY* poly, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char* gml;
  size_t size;

  size = asgml3_curvepoly_size(ctx,  poly, srs, precision, opts, prefix, id );
  gml = rtalloc(ctx,  size );
  asgml3_curvepoly_buf(ctx,  poly, srs, gml, precision, opts, prefix, id );
  return gml;
}


static size_t
asgml3_triangle_size(const RTCTX *ctx, const RTTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  size_t size;
  size_t prefixlen = strlen(prefix);

  size =  ( sizeof("<Triangle><exterior><LinearRing>///") + (prefixlen*3) ) * 2;
  size +=   sizeof("<posList></posList>") + (prefixlen*2);
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(prefix) + strlen(id) + sizeof(" id=..");
  if (IS_DIMS(opts)) size += sizeof(" srsDimension='x'");

  size += pointArray_GMLsize(ctx, triangle->points, precision);

  return size;
}

static size_t
asgml3_triangle_buf(const RTCTX *ctx, const RTTRIANGLE *triangle, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr=output;
  int dimension=2;

  if (RTFLAGS_GET_Z(triangle->flags)) dimension = 3;
  ptr += sprintf(ptr, "<%sTriangle", prefix);
  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
  ptr += sprintf(ptr, ">");

  ptr += sprintf(ptr, "<%sexterior><%sLinearRing>", prefix, prefix);
  if (IS_DIMS(opts)) ptr += sprintf(ptr, "<%sposList srsDimension=\"%d\">", prefix, dimension);
  else         ptr += sprintf(ptr, "<%sposList>", prefix);

  ptr += pointArray_toGML3(ctx, triangle->points, ptr, precision, opts);
  ptr += sprintf(ptr, "</%sposList></%sLinearRing></%sexterior>",
                 prefix, prefix, prefix);

  ptr += sprintf(ptr, "</%sTriangle>", prefix);

  return (ptr-output);
}

static char *
asgml3_triangle(const RTCTX *ctx, const RTTRIANGLE *triangle, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *output;
  int size;

  size = asgml3_triangle_size(ctx, triangle, srs, precision, opts, prefix, id);
  output = rtalloc(ctx, size);
  asgml3_triangle_buf(ctx, triangle, srs, output, precision, opts, prefix, id);
  return output;
}


/*
 * Compute max size required for GML version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asgml3_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);
  RTGEOM *subgeom;

  /* the longest possible multi version */
  size = sizeof("<MultiLineString></MultiLineString>") + prefixlen*2;

  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      size += ( sizeof("<pointMember>/") + prefixlen ) * 2;
      size += asgml3_point_size(ctx, (RTPOINT*)subgeom, 0, precision, opts, prefix, id);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      size += ( sizeof("<curveMember>/") + prefixlen ) * 2;
      size += asgml3_line_size(ctx, (RTLINE*)subgeom, 0, precision, opts, prefix, id);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      size += ( sizeof("<surfaceMember>/") + prefixlen ) * 2;
      size += asgml3_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, opts, prefix, id);
    }
  }

  return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_multi_buf(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  int type = col->type;
  char *ptr, *gmltype;
  int i;
  RTGEOM *subgeom;

  ptr = output;
  gmltype="";

  if   (type == RTMULTIPOINTTYPE)   gmltype = "MultiPoint";
  else if (type == RTMULTILINETYPE)    gmltype = "MultiCurve";
  else if (type == RTMULTIPOLYGONTYPE) gmltype = "MultiSurface";

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%s%s", prefix, gmltype);
  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

  if (!col->ngeoms)
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      ptr += sprintf(ptr, "<%spointMember>", prefix);
      ptr += asgml3_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, opts, prefix, id);
      ptr += sprintf(ptr, "</%spointMember>", prefix);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      ptr += sprintf(ptr, "<%scurveMember>", prefix);
      ptr += asgml3_line_buf(ctx, (RTLINE*)subgeom, 0, ptr, precision, opts, prefix, id);
      ptr += sprintf(ptr, "</%scurveMember>", prefix);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      ptr += sprintf(ptr, "<%ssurfaceMember>", prefix);
      ptr += asgml3_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, opts, 0, prefix, id);
      ptr += sprintf(ptr, "</%ssurfaceMember>", prefix);
    }
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%s%s>", prefix, gmltype);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_multi(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *gml;
  size_t size;

  size = asgml3_multi_size(ctx, col, srs, precision, opts, prefix, id);
  gml = rtalloc(ctx, size);
  asgml3_multi_buf(ctx, col, srs, gml, precision, opts, prefix, id);
  return gml;
}


static size_t
asgml3_psurface_size(const RTCTX *ctx, const RTPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);

  size = (sizeof("<PolyhedralSurface><polygonPatches>/") + prefixlen*2) * 2;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

  for (i=0; i<psur->ngeoms; i++)
  {
    size += asgml3_poly_size(ctx, psur->geoms[i], 0, precision, opts, prefix, id);
  }

  return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_psurface_buf(const RTCTX *ctx, const RTPSURFACE *psur, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr;
  int i;

  ptr = output;

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%sPolyhedralSurface", prefix);
  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
  ptr += sprintf(ptr, "><%spolygonPatches>", prefix);

  for (i=0; i<psur->ngeoms; i++)
  {
    ptr += asgml3_poly_buf(ctx, psur->geoms[i], 0, ptr, precision, opts, 1, prefix, id);
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%spolygonPatches></%sPolyhedralSurface>",
                 prefix, prefix);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_psurface(const RTCTX *ctx, const RTPSURFACE *psur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *gml;
  size_t size;

  size = asgml3_psurface_size(ctx, psur, srs, precision, opts, prefix, id);
  gml = rtalloc(ctx, size);
  asgml3_psurface_buf(ctx, psur, srs, gml, precision, opts, prefix, id);
  return gml;
}


static size_t
asgml3_tin_size(const RTCTX *ctx, const RTTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);

  size = (sizeof("<Tin><trianglePatches>/") + prefixlen*2) * 2;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

  for (i=0; i<tin->ngeoms; i++)
  {
    size += asgml3_triangle_size(ctx, tin->geoms[i], 0, precision, opts, prefix, id);
  }

  return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asgml3_tin_buf(const RTCTX *ctx, const RTTIN *tin, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr;
  int i;

  ptr = output;

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%sTin", prefix);
  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);
  else   ptr += sprintf(ptr, "><%strianglePatches>", prefix);

  for (i=0; i<tin->ngeoms; i++)
  {
    ptr += asgml3_triangle_buf(ctx, tin->geoms[i], 0, ptr, precision,
                               opts, prefix, id);
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%strianglePatches></%sTin>", prefix, prefix);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_tin(const RTCTX *ctx, const RTTIN *tin, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *gml;
  size_t size;

  size = asgml3_tin_size(ctx, tin, srs, precision, opts, prefix, id);
  gml = rtalloc(ctx, size);
  asgml3_tin_buf(ctx, tin, srs, gml, precision, opts, prefix, id);
  return gml;
}

static size_t
asgml3_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  int i;
  size_t size;
  size_t prefixlen = strlen(prefix);
  RTGEOM *subgeom;

  size = sizeof("<MultiGeometry></MultiGeometry>") + prefixlen*2;

  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    size += ( sizeof("<geometryMember>/") + prefixlen ) * 2;
    if ( subgeom->type == RTPOINTTYPE )
    {
      size += asgml3_point_size(ctx, (RTPOINT*)subgeom, 0, precision, opts, prefix, id);
    }
    else if ( subgeom->type == RTLINETYPE )
    {
      size += asgml3_line_size(ctx, (RTLINE*)subgeom, 0, precision, opts, prefix, id);
    }
    else if ( subgeom->type == RTPOLYGONTYPE )
    {
      size += asgml3_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, opts, prefix, id);
    }
    else if ( rtgeom_is_collection(ctx, subgeom) )
    {
      size += asgml3_multi_size(ctx, (RTCOLLECTION*)subgeom, 0, precision, opts, prefix, id);
    }
    else
      rterror(ctx, "asgml3_collection_size: unknown geometry type");
  }

  return size;
}

static size_t
asgml3_collection_buf(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char *ptr;
  int i;
  RTGEOM *subgeom;

  ptr = output;

  /* Open outmost tag */
  ptr += sprintf(ptr, "<%sMultiGeometry", prefix);
  if (srs) ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  if (id)  ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id);

  if (!col->ngeoms)
  {
    ptr += sprintf(ptr, "/>");
    return (ptr-output);
  }
  ptr += sprintf(ptr, ">");

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    ptr += sprintf(ptr, "<%sgeometryMember>", prefix);
    if ( subgeom->type == RTPOINTTYPE )
    {
      ptr += asgml3_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, opts, prefix, id);
    }
    else if ( subgeom->type == RTLINETYPE )
    {
      ptr += asgml3_line_buf(ctx, (RTLINE*)subgeom, 0, ptr, precision, opts, prefix, id);
    }
    else if ( subgeom->type == RTPOLYGONTYPE )
    {
      ptr += asgml3_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, opts, 0, prefix, id);
    }
    else if ( rtgeom_is_collection(ctx, subgeom) )
    {
      if ( subgeom->type == RTCOLLECTIONTYPE )
        ptr += asgml3_collection_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, opts, prefix, id);
      else
        ptr += asgml3_multi_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, opts, prefix, id);
    }
    else
      rterror(ctx, "asgml3_collection_buf: unknown geometry type");

    ptr += sprintf(ptr, "</%sgeometryMember>", prefix);
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "</%sMultiGeometry>", prefix);

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asgml3_collection(const RTCTX *ctx, const RTCOLLECTION *col, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char *gml;
  size_t size;

  size = asgml3_collection_size(ctx, col, srs, precision, opts, prefix, id);
  gml = rtalloc(ctx, size);
  asgml3_collection_buf(ctx, col, srs, gml, precision, opts, prefix, id);
  return gml;
}

static size_t asgml3_multicurve_size(const RTCTX *ctx,  const RTMCURVE* cur, const char *srs, int precision, int opts, const char *prefix, const char *id )
{
  size_t prefixlen = strlen(prefix);
  size_t size = sizeof( "<MultiCurve></MultiCurve>" ) + 2 * prefixlen;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  RTGEOM* subgeom;
  int i;

  for( i = 0; i < cur->ngeoms; ++i )
  {
    size += sizeof( "<curveMember></curveMember>" ) + 2 * prefixlen;
    subgeom = cur->geoms[i];
    if ( subgeom->type == RTLINETYPE )
    {
      size += asgml3_line_size(ctx,  (RTLINE*)subgeom, srs, precision, opts, prefix, id );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      size += asgml3_circstring_size(ctx,  (RTCIRCSTRING*)subgeom, srs, precision, opts, prefix, id );
    }
    else if( subgeom->type == RTCOMPOUNDTYPE )
    {
      size += asgml3_compound_size(ctx,  (RTCOMPOUND*)subgeom, srs, precision, opts, prefix, id );
    }
  }
  return size;
}

static size_t asgml3_multicurve_buf(const RTCTX *ctx,  const RTMCURVE* cur, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id )
{
  char* ptr = output;
  RTGEOM* subgeom;
  int i;

  ptr += sprintf(ptr, "<%sMultiCurve", prefix );
  if (srs)
  {
    ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  }
  if (id)
  {
    ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id );
  }
  ptr += sprintf( ptr, ">");

  for( i = 0; i < cur->ngeoms; ++i )
  {
    ptr += sprintf(ptr, "<%scurveMember>", prefix );
    subgeom = cur->geoms[i];
    if ( subgeom->type == RTLINETYPE )
    {
      ptr += asgml3_line_buf(ctx,  (RTLINE*)subgeom, srs, ptr, precision, opts, prefix, id );
    }
    else if( subgeom->type == RTCIRCSTRINGTYPE )
    {
      ptr += asgml3_circstring_buf(ctx,  (RTCIRCSTRING*)subgeom, srs, ptr, precision, opts, prefix, id );
    }
    else if( subgeom->type == RTCOMPOUNDTYPE )
    {
      ptr += asgml3_compound_buf(ctx,  (RTCOMPOUND*)subgeom, srs, ptr, precision, opts, prefix, id );
    }
    ptr += sprintf(ptr, "</%scurveMember>", prefix );
  }
  ptr += sprintf(ptr, "</%sMultiCurve>", prefix );
  return (ptr - output);
}

static char * asgml3_multicurve(const RTCTX *ctx,  const RTMCURVE* cur, const char *srs, int precision, int opts, const char *prefix, const char *id )
{
  char* gml;
  size_t size =asgml3_multicurve_size(ctx,  cur, srs, precision, opts, prefix, id );
  gml = rtalloc(ctx,  size );
  asgml3_multicurve_buf(ctx,  cur, srs, gml, precision, opts, prefix, id );
  return gml;
}

static size_t asgml3_multisurface_size(const RTCTX *ctx, const RTMSURFACE *sur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  size_t prefixlen = strlen(prefix);
  size_t size = sizeof( "<MultiSurface></MultiSurface>" ) + 2 * prefixlen;
  if (srs) size += strlen(srs) + sizeof(" srsName=..");
  if (id)  size += strlen(id) + strlen(prefix) + sizeof(" id=..");
  RTGEOM* subgeom;
  int i;

  for( i = 0; i < sur->ngeoms; ++i )
  {
    subgeom = sur->geoms[i];
    if( subgeom->type == RTPOLYGONTYPE )
    {
      size += asgml3_poly_size(ctx,  (RTPOLY*)sur->geoms[i], srs, precision, opts, prefix, id );
    }
    else if( subgeom->type == RTCURVEPOLYTYPE )
    {
      size += asgml3_curvepoly_size(ctx,  (RTCURVEPOLY*)sur->geoms[i], srs, precision, opts, prefix, id );
    }
  }
  return size;
}

static size_t asgml3_multisurface_buf(const RTCTX *ctx, const RTMSURFACE *sur, const char *srs, char *output, int precision, int opts, const char *prefix, const char *id)
{
  char* ptr = output;
  int i;
  RTGEOM* subgeom;

  ptr += sprintf( ptr, "<%sMultiSurface", prefix );
  if (srs)
  {
    ptr += sprintf(ptr, " srsName=\"%s\"", srs);
  }
  if (id)
  {
    ptr += sprintf(ptr, " %sid=\"%s\"", prefix, id );
  }
  ptr += sprintf( ptr, ">" );

  for( i = 0; i < sur->ngeoms; ++i )
  {
    subgeom = sur->geoms[i];
    if( subgeom->type == RTPOLYGONTYPE )
    {
      ptr += asgml3_poly_buf(ctx,  (RTPOLY*)sur->geoms[i], srs, ptr, precision, opts, 0, prefix, id );
    }
    else if( subgeom->type == RTCURVEPOLYTYPE )
    {
      ptr += asgml3_curvepoly_buf(ctx,  (RTCURVEPOLY*)sur->geoms[i], srs, ptr, precision, opts, prefix, id );
    }
  }
  ptr += sprintf( ptr, "</%sMultiSurface>", prefix );
  return ptr - output;
}

static char * asgml3_multisurface(const RTCTX *ctx, const RTMSURFACE *sur, const char *srs, int precision, int opts, const char *prefix, const char *id)
{
  char* gml;
  size_t size = asgml3_multisurface_size(ctx,  sur, srs, precision, opts, prefix, id );
  gml = rtalloc(ctx,  size );
  asgml3_multisurface_buf(ctx,  sur, srs, gml, precision, opts, prefix, id );
  return gml;
}


/* In GML3, inside <posList> or <pos>, coordinates are separated by a space separator
 * In GML3 also, lat/lon are reversed for geocentric data
 */
static size_t
pointArray_toGML3(const RTCTX *ctx, RTPOINTARRAY *pa, char *output, int precision, int opts)
{
  int i;
  char *ptr;
  char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
  char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
  char z[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];

  ptr = output;

  if ( ! RTFLAGS_GET_Z(pa->flags) )
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT2D *pt;
      pt = rt_getPoint2d_cp(ctx, pa, i);

      if (fabs(pt->x) < OUT_MAX_DOUBLE)
        sprintf(x, "%.*f", precision, pt->x);
      else
        sprintf(x, "%g", pt->x);
      trim_trailing_zeros(ctx, x);

      if (fabs(pt->y) < OUT_MAX_DOUBLE)
        sprintf(y, "%.*f", precision, pt->y);
      else
        sprintf(y, "%g", pt->y);
      trim_trailing_zeros(ctx, y);

      if ( i ) ptr += sprintf(ptr, " ");
      if (IS_DEGREE(opts))
        ptr += sprintf(ptr, "%s %s", y, x);
      else
        ptr += sprintf(ptr, "%s %s", x, y);
    }
  }
  else
  {
    for (i=0; i<pa->npoints; i++)
    {
      const RTPOINT3DZ *pt;
      pt = rt_getPoint3dz_cp(ctx, pa, i);

      if (fabs(pt->x) < OUT_MAX_DOUBLE)
        sprintf(x, "%.*f", precision, pt->x);
      else
        sprintf(x, "%g", pt->x);
      trim_trailing_zeros(ctx, x);

      if (fabs(pt->y) < OUT_MAX_DOUBLE)
        sprintf(y, "%.*f", precision, pt->y);
      else
        sprintf(y, "%g", pt->y);
      trim_trailing_zeros(ctx, y);

      if (fabs(pt->z) < OUT_MAX_DOUBLE)
        sprintf(z, "%.*f", precision, pt->z);
      else
        sprintf(z, "%g", pt->z);
      trim_trailing_zeros(ctx, z);

      if ( i ) ptr += sprintf(ptr, " ");
      if (IS_DEGREE(opts))
        ptr += sprintf(ptr, "%s %s %s", y, x, z);
      else
        ptr += sprintf(ptr, "%s %s %s", x, y, z);
    }
  }

  return ptr-output;
}



/*
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_GMLsize(const RTCTX *ctx, RTPOINTARRAY *pa, int precision)
{
  if (RTFLAGS_NDIMS(pa->flags) == 2)
    return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 2 * pa->npoints;

  return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(", ")) * 3 * pa->npoints;
}
