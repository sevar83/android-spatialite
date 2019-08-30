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
 * Copyright 2011-2015 Arrival 3D
 *
 **********************************************************************/


/**
* @file X3D output routines.
*
**********************************************************************/


#include "rttopo_config.h"
#include <string.h>
#include "librttopo_geom_internal.h"

/** defid is the id of the coordinate can be used to hold other elements DEF='abc' transform='' etc. **/
static size_t asx3d3_point_size(const RTCTX *ctx, const RTPOINT *point, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_point(const RTCTX *ctx, const RTPOINT *point, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_line_size(const RTCTX *ctx, const RTLINE *line, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_line(const RTCTX *ctx, const RTLINE *line, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_poly_size(const RTCTX *ctx, const RTPOLY *poly, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_triangle_size(const RTCTX *ctx, const RTTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_triangle(const RTCTX *ctx, const RTTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precisioSn, int opts, const char *defid);
static char * asx3d3_multi(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_psurface(const RTCTX *ctx, const RTPSURFACE *psur, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_tin(const RTCTX *ctx, const RTTIN *tin, char *srs, int precision, int opts, const char *defid);
static size_t asx3d3_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static char * asx3d3_collection(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid);
static size_t pointArray_toX3D3(const RTCTX *ctx, RTPOINTARRAY *pa, char *buf, int precision, int opts, int is_closed);

static size_t pointArray_X3Dsize(const RTCTX *ctx, RTPOINTARRAY *pa, int precision);


/*
 * VERSION X3D 3.0.2 http://www.web3d.org/specifications/x3d-3.0.dtd
 */


/* takes a GEOMETRY and returns an X3D representation */
extern char *
rtgeom_to_x3d3(const RTCTX *ctx, const RTGEOM *geom, char *srs, int precision, int opts, const char *defid)
{
  int type = geom->type;

  switch (type)
  {
  case RTPOINTTYPE:
    return asx3d3_point(ctx, (RTPOINT*)geom, srs, precision, opts, defid);

  case RTLINETYPE:
    return asx3d3_line(ctx, (RTLINE*)geom, srs, precision, opts, defid);

  case RTPOLYGONTYPE:
  {
    /** We might change this later, but putting a polygon in an indexed face set
    * seems like the simplest way to go so treat just like a mulitpolygon
    */
    RTCOLLECTION *tmp = (RTCOLLECTION*)rtgeom_as_multi(ctx, geom);
    char *ret = asx3d3_multi(ctx, tmp, srs, precision, opts, defid);
    rtcollection_free(ctx, tmp);
    return ret;
  }

  case RTTRIANGLETYPE:
    return asx3d3_triangle(ctx, (RTTRIANGLE*)geom, srs, precision, opts, defid);

  case RTMULTIPOINTTYPE:
  case RTMULTILINETYPE:
  case RTMULTIPOLYGONTYPE:
    return asx3d3_multi(ctx, (RTCOLLECTION*)geom, srs, precision, opts, defid);

  case RTPOLYHEDRALSURFACETYPE:
    return asx3d3_psurface(ctx, (RTPSURFACE*)geom, srs, precision, opts, defid);

  case RTTINTYPE:
    return asx3d3_tin(ctx, (RTTIN*)geom, srs, precision, opts, defid);

  case RTCOLLECTIONTYPE:
    return asx3d3_collection(ctx, (RTCOLLECTION*)geom, srs, precision, opts, defid);

  default:
    rterror(ctx, "rtgeom_to_x3d3: '%s' geometry type not supported", rttype_name(ctx, type));
    return NULL;
  }
}

static size_t
asx3d3_point_size(const RTCTX *ctx, const RTPOINT *point, char *srs, int precision, int opts, const char *defid)
{
  int size;
  /* size_t defidlen = strlen(defid); */

  size = pointArray_X3Dsize(ctx, point->point, precision);
  /* size += ( sizeof("<point><pos>/") + (defidlen*2) ) * 2; */
  /* if (srs)     size += strlen(srs) + sizeof(" srsName=.."); */
  return size;
}

static size_t
asx3d3_point_buf(const RTCTX *ctx, const RTPOINT *point, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr = output;
  /* int dimension=2; */

  /* if (RTFLAGS_GET_Z(point->flags)) dimension = 3; */
  /*  if ( srs )
    {
      ptr += sprintf(ptr, "<%sPoint srsName=\"%s\">", defid, srs);
    }
    else*/
  /* ptr += sprintf(ptr, "%s", defid); */

  /* ptr += sprintf(ptr, "<%spos>", defid); */
  ptr += pointArray_toX3D3(ctx, point->point, ptr, precision, opts, 0);
  /* ptr += sprintf(ptr, "</%spos></%sPoint>", defid, defid); */

  return (ptr-output);
}

static char *
asx3d3_point(const RTCTX *ctx, const RTPOINT *point, char *srs, int precision, int opts, const char *defid)
{
  char *output;
  int size;

  size = asx3d3_point_size(ctx, point, srs, precision, opts, defid);
  output = rtalloc(ctx, size);
  asx3d3_point_buf(ctx, point, srs, output, precision, opts, defid);
  return output;
}


static size_t
asx3d3_line_size(const RTCTX *ctx, const RTLINE *line, char *srs, int precision, int opts, const char *defid)
{
  int size;
  size_t defidlen = strlen(defid);

  size = pointArray_X3Dsize(ctx, line->points, precision)*2;

  if ( X3D_USE_GEOCOORDS(opts) ) {
      size += (
              sizeof("<LineSet vertexCount=''><GeoCoordinate geoSystem='\"GD\" \"WE\" \"longitude_first\"' point='' /></LineSet>")  + defidlen
          ) * 2;
  }
  else {
    size += (
                sizeof("<LineSet vertexCount=''><Coordinate point='' /></LineSet>")  + defidlen
            ) * 2;
  }

  /* if (srs)     size += strlen(srs) + sizeof(" srsName=.."); */
  return size;
}

static size_t
asx3d3_line_buf(const RTCTX *ctx, const RTLINE *line, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr=output;
  /* int dimension=2; */
  RTPOINTARRAY *pa;


  /* if (RTFLAGS_GET_Z(line->flags)) dimension = 3; */

  pa = line->points;
  ptr += sprintf(ptr, "<LineSet %s vertexCount='%d'>", defid, pa->npoints);

  if ( X3D_USE_GEOCOORDS(opts) ) ptr += sprintf(ptr, "<GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ( (opts & RT_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
  else
    ptr += sprintf(ptr, "<Coordinate point='");
  ptr += pointArray_toX3D3(ctx, line->points, ptr, precision, opts, rtline_is_closed(ctx, (RTLINE *) line));

  ptr += sprintf(ptr, "' />");

  ptr += sprintf(ptr, "</LineSet>");
  return (ptr-output);
}

static size_t
asx3d3_line_coords(const RTCTX *ctx, const RTLINE *line, char *output, int precision, int opts)
{
  char *ptr=output;
  /* ptr += sprintf(ptr, ""); */
  ptr += pointArray_toX3D3(ctx, line->points, ptr, precision, opts, rtline_is_closed(ctx, line));
  return (ptr-output);
}

/* Calculate the coordIndex property of the IndexedLineSet for the multilinestring */
static size_t
asx3d3_mline_coordindex(const RTCTX *ctx, const RTMLINE *mgeom, char *output)
{
  char *ptr=output;
  RTLINE *geom;
  int i, j, k, si;
  RTPOINTARRAY *pa;
  int np;

  j = 0;
  for (i=0; i < mgeom->ngeoms; i++)
  {
    geom = (RTLINE *) mgeom->geoms[i];
    pa = geom->points;
    np = pa->npoints;
    si = j;  /* start index of first point of linestring */
    for (k=0; k < np ; k++)
    {
      if (k)
      {
        ptr += sprintf(ptr, " ");
      }
      /** if the linestring is closed, we put the start point index
      *   for the last vertex to denote use first point
      *    and don't increment the index **/
      if (!rtline_is_closed(ctx, geom) || k < (np -1) )
      {
        ptr += sprintf(ptr, "%d", j);
        j += 1;
      }
      else
      {
        ptr += sprintf(ptr,"%d", si);
      }
    }
    if (i < (mgeom->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " -1 "); /* separator for each linestring */
    }
  }
  return (ptr-output);
}

/* Calculate the coordIndex property of the IndexedLineSet for a multipolygon
    This is not ideal -- would be really nice to just share this function with psurf,
    but I'm not smart enough to do that yet*/
static size_t
asx3d3_mpoly_coordindex(const RTCTX *ctx, const RTMPOLY *psur, char *output)
{
  char *ptr=output;
  RTPOLY *patch;
  int i, j, k, l;
  int np;
  j = 0;
  for (i=0; i<psur->ngeoms; i++)
  {
    patch = (RTPOLY *) psur->geoms[i];
    for (l=0; l < patch->nrings; l++)
    {
      np = patch->rings[l]->npoints - 1;
      for (k=0; k < np ; k++)
      {
        if (k)
        {
          ptr += sprintf(ptr, " ");
        }
        ptr += sprintf(ptr, "%d", (j + k));
      }
      j += k;
      if (l < (patch->nrings - 1) )
      {
        /** @todo TODO: Decide the best way to render holes
        *  Evidentally according to my X3D expert the X3D consortium doesn't really
        *  support holes and it's an issue of argument among many that feel it should. He thinks CAD x3d extensions to spec might.
        *  What he has done and others developing X3D exports to simulate a hole is to cut around it.
        *  So if you have a donut, you would cut it into half and have 2 solid polygons.  Not really sure the best way to handle this.
        *  For now will leave it as polygons stacked on top of each other -- which is what we are doing here and perhaps an option
        *  to color differently.  It's not ideal but the alternative sounds complicated.
        **/
        ptr += sprintf(ptr, " -1 "); /* separator for each inner ring. Ideally we should probably triangulate and cut around as others do */
      }
    }
    if (i < (psur->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " -1 "); /* separator for each subgeom */
    }
  }
  return (ptr-output);
}

/** Return the linestring as an X3D LineSet */
static char *
asx3d3_line(const RTCTX *ctx, const RTLINE *line, char *srs, int precision, int opts, const char *defid)
{
  char *output;
  int size;

  size = sizeof("<LineSet><CoordIndex ='' /></LineSet>") + asx3d3_line_size(ctx, line, srs, precision, opts, defid);
  output = rtalloc(ctx, size);
  asx3d3_line_buf(ctx, line, srs, output, precision, opts, defid);
  return output;
}

/** Compute the string space needed for the IndexedFaceSet representation of the polygon **/
static size_t
asx3d3_poly_size(const RTCTX *ctx, const RTPOLY *poly,  char *srs, int precision, int opts, const char *defid)
{
  size_t size;
  size_t defidlen = strlen(defid);
  int i;

  size = ( sizeof("<IndexedFaceSet></IndexedFaceSet>") + (defidlen*3) ) * 2 + 6 * (poly->nrings - 1);

  for (i=0; i<poly->nrings; i++)
    size += pointArray_X3Dsize(ctx, poly->rings[i], precision);

  return size;
}

/** Compute the X3D coordinates of the polygon **/
static size_t
asx3d3_poly_buf(const RTCTX *ctx, const RTPOLY *poly, char *srs, char *output, int precision, int opts, int is_patch, const char *defid)
{
  int i;
  char *ptr=output;

  ptr += pointArray_toX3D3(ctx, poly->rings[0], ptr, precision, opts, 1);
  for (i=1; i<poly->nrings; i++)
  {
    ptr += sprintf(ptr, " "); /* inner ring points start */
    ptr += pointArray_toX3D3(ctx, poly->rings[i], ptr, precision, opts,1);
  }
  return (ptr-output);
}

static size_t
asx3d3_triangle_size(const RTCTX *ctx, const RTTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid)
{
  size_t size;
  size_t defidlen = strlen(defid);

  /** 6 for the 3 sides and space to separate each side **/
  size = sizeof("<IndexedTriangleSet index=''></IndexedTriangleSet>") + defidlen + 6;
  size += pointArray_X3Dsize(ctx, triangle->points, precision);

  return size;
}

static size_t
asx3d3_triangle_buf(const RTCTX *ctx, const RTTRIANGLE *triangle, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr=output;
  ptr += pointArray_toX3D3(ctx, triangle->points, ptr, precision, opts, 1);

  return (ptr-output);
}

static char *
asx3d3_triangle(const RTCTX *ctx, const RTTRIANGLE *triangle, char *srs, int precision, int opts, const char *defid)
{
  char *output;
  int size;

  size = asx3d3_triangle_size(ctx, triangle, srs, precision, opts, defid);
  output = rtalloc(ctx, size);
  asx3d3_triangle_buf(ctx, triangle, srs, output, precision, opts, defid);
  return output;
}


/**
 * Compute max size required for X3D version of this
 * inspected geometry. Will recurse when needed.
 * Don't call this with single-geoms inspected.
 */
static size_t
asx3d3_multi_size(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
  int i;
  size_t size;
  size_t defidlen = strlen(defid);
  RTGEOM *subgeom;

  /* the longest possible multi version needs to hold DEF=defid and coordinate breakout */
  if ( X3D_USE_GEOCOORDS(opts) )
    size = sizeof("<PointSet><GeoCoordinate geoSystem='\"GD\" \"WE\" \"longitude_first\"' point='' /></PointSet>");
  else
    size = sizeof("<PointSet><Coordinate point='' /></PointSet>") + defidlen;


  /* if ( srs ) size += strlen(srs) + sizeof(" srsName=.."); */

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      /* size += ( sizeof("point=''") + defidlen ) * 2; */
      size += asx3d3_point_size(ctx, (RTPOINT*)subgeom, 0, precision, opts, defid);
    }
    else if (subgeom->type == RTLINETYPE)
    {
      /* size += ( sizeof("<curveMember>/") + defidlen ) * 2; */
      size += asx3d3_line_size(ctx, (RTLINE*)subgeom, 0, precision, opts, defid);
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      /* size += ( sizeof("<surfaceMember>/") + defidlen ) * 2; */
      size += asx3d3_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, opts, defid);
    }
  }

  return size;
}

/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_multi_buf(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr, *x3dtype;
  int i;
  int dimension=2;

  if (RTFLAGS_GET_Z(col->flags)) dimension = 3;
  RTGEOM *subgeom;
  ptr = output;
  x3dtype="";


  switch (col->type)
  {
        case RTMULTIPOINTTYPE:
            x3dtype = "PointSet";
            if ( dimension == 2 ){ /** Use Polypoint2D instead **/
                x3dtype = "Polypoint2D";
                ptr += sprintf(ptr, "<%s %s point='", x3dtype, defid);
            }
            else {
                ptr += sprintf(ptr, "<%s %s>", x3dtype, defid);
            }
            break;
        case RTMULTILINETYPE:
            x3dtype = "IndexedLineSet";
            ptr += sprintf(ptr, "<%s %s coordIndex='", x3dtype, defid);
            ptr += asx3d3_mline_coordindex(ctx, (const RTMLINE *)col, ptr);
            ptr += sprintf(ptr, "'>");
            break;
        case RTMULTIPOLYGONTYPE:
            x3dtype = "IndexedFaceSet";
            ptr += sprintf(ptr, "<%s %s coordIndex='", x3dtype, defid);
            ptr += asx3d3_mpoly_coordindex(ctx, (const RTMPOLY *)col, ptr);
            ptr += sprintf(ptr, "'>");
            break;
        default:
            rterror(ctx, "asx3d3_multi_buf: '%s' geometry type not supported", rttype_name(ctx, col->type));
            return 0;
    }
    if (dimension == 3){
    if ( X3D_USE_GEOCOORDS(opts) )
      ptr += sprintf(ptr, "<GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ((opts & RT_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
    else
          ptr += sprintf(ptr, "<Coordinate point='");
    }

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    if (subgeom->type == RTPOINTTYPE)
    {
      ptr += asx3d3_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, opts, defid);
      ptr += sprintf(ptr, " ");
    }
    else if (subgeom->type == RTLINETYPE)
    {
      ptr += asx3d3_line_coords(ctx, (RTLINE*)subgeom, ptr, precision, opts);
      ptr += sprintf(ptr, " ");
    }
    else if (subgeom->type == RTPOLYGONTYPE)
    {
      ptr += asx3d3_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, opts, 0, defid);
      ptr += sprintf(ptr, " ");
    }
  }

  /* Close outmost tag */
  if (dimension == 3){
      ptr += sprintf(ptr, "' /></%s>", x3dtype);
  }
  else { ptr += sprintf(ptr, "' />"); }
  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_multi(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
  char *x3d;
  size_t size;

  size = asx3d3_multi_size(ctx, col, srs, precision, opts, defid);
  x3d = rtalloc(ctx, size);
  asx3d3_multi_buf(ctx, col, srs, x3d, precision, opts, defid);
  return x3d;
}


static size_t
asx3d3_psurface_size(const RTCTX *ctx, const RTPSURFACE *psur, char *srs, int precision, int opts, const char *defid)
{
  int i;
  size_t size;
  size_t defidlen = strlen(defid);

  if ( X3D_USE_GEOCOORDS(opts) ) size = sizeof("<IndexedFaceSet coordIndex=''><GeoCoordinate geoSystem='\"GD\" \"WE\" \"longitude_first\"' point='' />") + defidlen;
  else size = sizeof("<IndexedFaceSet coordIndex=''><Coordinate point='' />") + defidlen;


  for (i=0; i<psur->ngeoms; i++)
  {
    size += asx3d3_poly_size(ctx, psur->geoms[i], 0, precision, opts, defid)*5; /** need to make space for coordIndex values too including -1 separating each poly**/
  }

  return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_psurface_buf(const RTCTX *ctx, const RTPSURFACE *psur, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr;
  int i;
  int j;
  int k;
  int np;
  RTPOLY *patch;

  ptr = output;

  /* Open outmost tag */
  ptr += sprintf(ptr, "<IndexedFaceSet %s coordIndex='",defid);

  j = 0;
  for (i=0; i<psur->ngeoms; i++)
  {
    patch = (RTPOLY *) psur->geoms[i];
    np = patch->rings[0]->npoints - 1;
    for (k=0; k < np ; k++)
    {
      if (k)
      {
        ptr += sprintf(ptr, " ");
      }
      ptr += sprintf(ptr, "%d", (j + k));
    }
    if (i < (psur->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " -1 "); /* separator for each subgeom */
    }
    j += k;
  }

  if ( X3D_USE_GEOCOORDS(opts) )
    ptr += sprintf(ptr, "'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ( (opts & RT_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
  else ptr += sprintf(ptr, "'><Coordinate point='");

  for (i=0; i<psur->ngeoms; i++)
  {
    ptr += asx3d3_poly_buf(ctx, psur->geoms[i], 0, ptr, precision, opts, 1, defid);
    if (i < (psur->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " "); /* only add a trailing space if its not the last polygon in the set */
    }
  }

  /* Close outmost tag */
  ptr += sprintf(ptr, "' /></IndexedFaceSet>");

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_psurface(const RTCTX *ctx, const RTPSURFACE *psur, char *srs, int precision, int opts, const char *defid)
{
  char *x3d;
  size_t size;

  size = asx3d3_psurface_size(ctx, psur, srs, precision, opts, defid);
  x3d = rtalloc(ctx, size);
  asx3d3_psurface_buf(ctx, psur, srs, x3d, precision, opts, defid);
  return x3d;
}


static size_t
asx3d3_tin_size(const RTCTX *ctx, const RTTIN *tin, char *srs, int precision, int opts, const char *defid)
{
  int i;
  size_t size;
  size_t defidlen = strlen(defid);
  /* int dimension=2; */

  /** Need to make space for size of additional attributes,
  ** the coordIndex has a value for each edge for each triangle plus a space to separate so we need at least that much extra room ***/
  size = sizeof("<IndexedTriangleSet coordIndex=''></IndexedTriangleSet>") + defidlen + tin->ngeoms*12;

  for (i=0; i<tin->ngeoms; i++)
  {
    size += (asx3d3_triangle_size(ctx, tin->geoms[i], 0, precision, opts, defid) * 20); /** 3 is to make space for coordIndex **/
  }

  return size;
}


/*
 * Don't call this with single-geoms inspected!
 */
static size_t
asx3d3_tin_buf(const RTCTX *ctx, const RTTIN *tin, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr;
  int i;
  int k;
  /* int dimension=2; */

  ptr = output;

  ptr += sprintf(ptr, "<IndexedTriangleSet %s index='",defid);
  k = 0;
  /** Fill in triangle index **/
  for (i=0; i<tin->ngeoms; i++)
  {
    ptr += sprintf(ptr, "%d %d %d", k, (k+1), (k+2));
    if (i < (tin->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " ");
    }
    k += 3;
  }

  if ( X3D_USE_GEOCOORDS(opts) ) ptr += sprintf(ptr, "'><GeoCoordinate geoSystem='\"GD\" \"WE\" \"%s\"' point='", ( (opts & RT_X3D_FLIP_XY) ? "latitude_first" : "longitude_first") );
  else ptr += sprintf(ptr, "'><Coordinate point='");

  for (i=0; i<tin->ngeoms; i++)
  {
    ptr += asx3d3_triangle_buf(ctx, tin->geoms[i], 0, ptr, precision,
                               opts, defid);
    if (i < (tin->ngeoms - 1) )
    {
      ptr += sprintf(ptr, " ");
    }
  }

  /* Close outmost tag */

  ptr += sprintf(ptr, "'/></IndexedTriangleSet>");

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_tin(const RTCTX *ctx, const RTTIN *tin, char *srs, int precision, int opts, const char *defid)
{
  char *x3d;
  size_t size;

  size = asx3d3_tin_size(ctx, tin, srs, precision, opts, defid);
  x3d = rtalloc(ctx, size);
  asx3d3_tin_buf(ctx, tin, srs, x3d, precision, opts, defid);
  return x3d;
}

static size_t
asx3d3_collection_size(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
  int i;
  size_t size;
  size_t defidlen = strlen(defid);
  RTGEOM *subgeom;

  /* size = sizeof("<MultiGeometry></MultiGeometry>") + defidlen*2; */
  size = defidlen*2;

  /** if ( srs )
    size += strlen(srs) + sizeof(" srsName=.."); **/

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    size += ( sizeof("<Shape />") + defidlen ) * 2; /** for collections we need to wrap each in a shape tag to make valid **/
    if ( subgeom->type == RTPOINTTYPE )
    {
      size += asx3d3_point_size(ctx, (RTPOINT*)subgeom, 0, precision, opts, defid);
    }
    else if ( subgeom->type == RTLINETYPE )
    {
      size += asx3d3_line_size(ctx, (RTLINE*)subgeom, 0, precision, opts, defid);
    }
    else if ( subgeom->type == RTPOLYGONTYPE )
    {
      size += asx3d3_poly_size(ctx, (RTPOLY*)subgeom, 0, precision, opts, defid);
    }
    else if ( subgeom->type == RTTINTYPE )
    {
      size += asx3d3_tin_size(ctx, (RTTIN*)subgeom, 0, precision, opts, defid);
    }
    else if ( subgeom->type == RTPOLYHEDRALSURFACETYPE )
    {
      size += asx3d3_psurface_size(ctx, (RTPSURFACE*)subgeom, 0, precision, opts, defid);
    }
    else if ( rtgeom_is_collection(ctx, subgeom) )
    {
      size += asx3d3_multi_size(ctx, (RTCOLLECTION*)subgeom, 0, precision, opts, defid);
    }
    else
      rterror(ctx, "asx3d3_collection_size: unknown geometry type");
  }

  return size;
}

static size_t
asx3d3_collection_buf(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, char *output, int precision, int opts, const char *defid)
{
  char *ptr;
  int i;
  RTGEOM *subgeom;

  ptr = output;

  /* Open outmost tag */
  /** @TODO: decide if we need outtermost tags, this one was just a copy from gml so is wrong **/
#ifdef PGIS_X3D_OUTERMOST_TAGS
  if ( srs )
  {
    ptr += sprintf(ptr, "<%sMultiGeometry srsName=\"%s\">", defid, srs);
  }
  else
  {
    ptr += sprintf(ptr, "<%sMultiGeometry>", defid);
  }
#endif

  for (i=0; i<col->ngeoms; i++)
  {
    subgeom = col->geoms[i];
    ptr += sprintf(ptr, "<Shape%s>", defid);
    if ( subgeom->type == RTPOINTTYPE )
    {
      ptr += asx3d3_point_buf(ctx, (RTPOINT*)subgeom, 0, ptr, precision, opts, defid);
    }
    else if ( subgeom->type == RTLINETYPE )
    {
      ptr += asx3d3_line_buf(ctx, (RTLINE*)subgeom, 0, ptr, precision, opts, defid);
    }
    else if ( subgeom->type == RTPOLYGONTYPE )
    {
      ptr += asx3d3_poly_buf(ctx, (RTPOLY*)subgeom, 0, ptr, precision, opts, 0, defid);
    }
    else if ( subgeom->type == RTTINTYPE )
    {
      ptr += asx3d3_tin_buf(ctx, (RTTIN*)subgeom, srs, ptr, precision, opts,  defid);

    }
    else if ( subgeom->type == RTPOLYHEDRALSURFACETYPE )
    {
      ptr += asx3d3_psurface_buf(ctx, (RTPSURFACE*)subgeom, srs, ptr, precision, opts,  defid);

    }
    else if ( rtgeom_is_collection(ctx, subgeom) )
    {
      if ( subgeom->type == RTCOLLECTIONTYPE )
        ptr += asx3d3_collection_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, opts, defid);
      else
        ptr += asx3d3_multi_buf(ctx, (RTCOLLECTION*)subgeom, 0, ptr, precision, opts, defid);
    }
    else
      rterror(ctx, "asx3d3_collection_buf: unknown geometry type");

    ptr += printf(ptr, "</Shape>");
  }

  /* Close outmost tag */
#ifdef PGIS_X3D_OUTERMOST_TAGS
  ptr += sprintf(ptr, "</%sMultiGeometry>", defid);
#endif

  return (ptr-output);
}

/*
 * Don't call this with single-geoms inspected!
 */
static char *
asx3d3_collection(const RTCTX *ctx, const RTCOLLECTION *col, char *srs, int precision, int opts, const char *defid)
{
  char *x3d;
  size_t size;

  size = asx3d3_collection_size(ctx, col, srs, precision, opts, defid);
  x3d = rtalloc(ctx, size);
  asx3d3_collection_buf(ctx, col, srs, x3d, precision, opts, defid);
  return x3d;
}


/** In X3D3, coordinates are separated by a space separator
 */
static size_t
pointArray_toX3D3(const RTCTX *ctx, RTPOINTARRAY *pa, char *output, int precision, int opts, int is_closed)
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
      /** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
      if ( !is_closed || i < (pa->npoints - 1) )
      {
        RTPOINT2D pt;
        rt_getPoint2d_p(ctx, pa, i, &pt);

        if (fabs(pt.x) < OUT_MAX_DOUBLE)
          sprintf(x, "%.*f", precision, pt.x);
        else
          sprintf(x, "%g", pt.x);
        trim_trailing_zeros(ctx, x);

        if (fabs(pt.y) < OUT_MAX_DOUBLE)
          sprintf(y, "%.*f", precision, pt.y);
        else
          sprintf(y, "%g", pt.y);
        trim_trailing_zeros(ctx, y);

        if ( i )
          ptr += sprintf(ptr, " ");

        if ( ( opts & RT_X3D_FLIP_XY) )
          ptr += sprintf(ptr, "%s %s", y, x);
        else
          ptr += sprintf(ptr, "%s %s", x, y);
      }
    }
  }
  else
  {
    for (i=0; i<pa->npoints; i++)
    {
      /** Only output the point if it is not the last point of a closed object or it is a non-closed type **/
      if ( !is_closed || i < (pa->npoints - 1) )
      {
        RTPOINT4D pt;
        rt_getPoint4d_p(ctx, pa, i, &pt);

        if (fabs(pt.x) < OUT_MAX_DOUBLE)
          sprintf(x, "%.*f", precision, pt.x);
        else
          sprintf(x, "%g", pt.x);
        trim_trailing_zeros(ctx, x);

        if (fabs(pt.y) < OUT_MAX_DOUBLE)
          sprintf(y, "%.*f", precision, pt.y);
        else
          sprintf(y, "%g", pt.y);
        trim_trailing_zeros(ctx, y);

        if (fabs(pt.z) < OUT_MAX_DOUBLE)
          sprintf(z, "%.*f", precision, pt.z);
        else
          sprintf(z, "%g", pt.z);
        trim_trailing_zeros(ctx, z);

        if ( i )
          ptr += sprintf(ptr, " ");

        if ( ( opts & RT_X3D_FLIP_XY) )
          ptr += sprintf(ptr, "%s %s %s", y, x, z);
        else
          ptr += sprintf(ptr, "%s %s %s", x, y, z);
      }
    }
  }

  return ptr-output;
}



/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_X3Dsize(const RTCTX *ctx, RTPOINTARRAY *pa, int precision)
{
  if (RTFLAGS_NDIMS(pa->flags) == 2)
    return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(" "))
           * 2 * pa->npoints;

  return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(" ")) * 3 * pa->npoints;
}
