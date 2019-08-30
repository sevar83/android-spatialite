/*

 gg_relations_ext.c -- Gaia spatial relations [advanced]
    
 version 4.3, 2015 June 29

 Author: Sandro Furieri a.furieri@lqt.it

 ------------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the SpatiaLite library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2008-2015
the Initial Developer. All Rights Reserved.

Contributor(s):

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifndef OMIT_GEOS		/* including GEOS */
#ifdef GEOS_REENTRANT
#ifdef GEOS_ONLY_REENTRANT
#define GEOS_USE_ONLY_R_API	/* only fully thread-safe GEOS API */
#endif
#endif
#include <geos_c.h>
#endif

#include <spatialite_private.h>
#include <spatialite/sqlite.h>

#include <spatialite/gaiageo.h>

#ifndef OMIT_GEOS		/* including GEOS */

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaOffsetCurve (gaiaGeomCollPtr geom, double radius, int points,
		 int left_right)
{
/*
// builds a geometry that is the OffsetCurve of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;

    if (left_right < 0)
	left_right = 0;		/* silencing stupid compiler warnings */

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos (geom);
    g2 = GEOSOffsetCurve (g1, radius, points, GEOSBUF_JOIN_ROUND, 5.0);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
#else
    if (geom == NULL || radius == 0.0 || points == 0 || left_right == 0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaOffsetCurve_r (const void *p_cache, gaiaGeomCollPtr geom, double radius,
		   int points, int left_right)
{
/*
// builds a geometry that is the OffsetCurve of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;

    if (left_right < 0)
	left_right = 0;		/* silencing stupid compiler warnings */

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos_r (cache, geom);
    g2 = GEOSOffsetCurve_r (handle, g1, radius, points,
			    GEOSBUF_JOIN_ROUND, 5.0);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM_r (cache, g2);
    else
	geo = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSingleSidedBuffer (gaiaGeomCollPtr geom, double radius, int points,
		       int left_right)
{
/*
// builds a geometry that is the SingleSided BUFFER of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSBufferParams *params = NULL;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos (geom);
/* setting up Buffer params */
    params = GEOSBufferParams_create ();
    GEOSBufferParams_setJoinStyle (params, GEOSBUF_JOIN_ROUND);
    GEOSBufferParams_setMitreLimit (params, 5.0);
    GEOSBufferParams_setQuadrantSegments (params, points);
    GEOSBufferParams_setSingleSided (params, 1);

/* creating the SingleSided Buffer */
    if (left_right == 0)
      {
	  /* right-sided requires NEGATIVE radius */
	  radius *= -1.0;
      }
    g2 = GEOSBufferWithParams (g1, params, radius);
    GEOSGeom_destroy (g1);
    GEOSBufferParams_destroy (params);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g2);
    else
	geo = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
#else
    if (geom == NULL || radius == 0.0 || points == 0 || left_right == 0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return geo;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSingleSidedBuffer_r (const void *p_cache, gaiaGeomCollPtr geom,
			 double radius, int points, int left_right)
{
/*
// builds a geometry that is the SingleSided BUFFER of GEOM 
// (which is expected to be of the LINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSBufferParams *params = NULL;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int closed = 0;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;

/* checking the input geometry for validity */
    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  if (gaiaIsClosed (ln))
	      closed++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGON are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts > 0 || pgs > 0 || lns > 1 || closed > 0)
	return NULL;

/* all right: this one simply is a LINESTRING */
    geom->DeclaredType = GAIA_LINESTRING;

    g1 = gaiaToGeos_r (cache, geom);
/* setting up Buffer params */
    params = GEOSBufferParams_create_r (handle);
    GEOSBufferParams_setJoinStyle_r (handle, params, GEOSBUF_JOIN_ROUND);
    GEOSBufferParams_setMitreLimit_r (handle, params, 5.0);
    GEOSBufferParams_setQuadrantSegments_r (handle, params, points);
    GEOSBufferParams_setSingleSided_r (handle, params, 1);

/* creating the SingleSided Buffer */
    if (left_right == 0)
      {
	  /* right-sided requires NEGATIVE radius */
	  radius *= -1.0;
      }
    g2 = GEOSBufferWithParams_r (handle, g1, params, radius);
    GEOSGeom_destroy_r (handle, g1);
    GEOSBufferParams_destroy_r (handle, params);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM_r (cache, g2);
    else
	geo = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom->Srid;
    return geo;
}

GAIAGEO_DECLARE int
gaiaHausdorffDistance (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2,
		       double *xdist)
{
/* 
/ computes the (discrete) Hausdorff distance intercurring 
/ between GEOM-1 and GEOM-2 
*/
    int ret = 0;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    double dist;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaResetGeosMsg ();
    if (!geom1 || !geom2)
	return 0;
    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    ret = GEOSHausdorffDistance (g1, g2, &dist);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (ret)
	*xdist = dist;
#else
    if (geom1 == NULL || geom2 == NULL || xdist == NULL)
	geom1 = NULL;		/* silencing stupid compiler warnings */
#endif
    return ret;
}

GAIAGEO_DECLARE int
gaiaHausdorffDistance_r (const void *p_cache, gaiaGeomCollPtr geom1,
			 gaiaGeomCollPtr geom2, double *xdist)
{
/* 
/ computes the (discrete) Hausdorff distance intercurring 
/ between GEOM-1 and GEOM-2 
*/
    double dist;
    int ret;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return 0;
    gaiaResetGeosMsg_r (cache);
    if (!geom1 || !geom2)
	return 0;
    g1 = gaiaToGeos_r (cache, geom1);
    g2 = gaiaToGeos_r (cache, geom2);
    ret = GEOSHausdorffDistance_r (handle, g1, g2, &dist);
    GEOSGeom_destroy_r (handle, g1);
    GEOSGeom_destroy_r (handle, g2);
    if (ret)
	*xdist = dist;
    return ret;
}

static gaiaGeomCollPtr
geom_as_lines (gaiaGeomCollPtr geom)
{
/* transforms a Geometry into a LINESTRING/MULTILINESTRING (if possible) */
    gaiaGeomCollPtr result;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr new_ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;

    if (!geom)
	return NULL;
    if (geom->FirstPoint != NULL)
      {
	  /* invalid: GEOM contains at least one POINT */
	  return NULL;
      }

    switch (geom->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* copying any Linestring */
	  new_ln = gaiaAddLinestringToGeomColl (result, ln->Points);
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* copying any Polygon Ring (as Linestring) */
	  rng = pg->Exterior;
	  new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior Ring */
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* any interior Ring */
		      if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    gaiaSetPoint (new_ln->Coords, iv, x, y);
			}
		  }
	    }
	  pg = pg->Next;
      }
    return result;
}

static void
add_shared_linestring (gaiaGeomCollPtr geom, gaiaDynamicLinePtr dyn)
{
/* adding a LINESTRING from Dynamic Line */
    int count = 0;
    gaiaLinestringPtr ln;
    gaiaPointPtr pt;
    int iv;

    if (!geom)
	return;
    if (!dyn)
	return;
    pt = dyn->First;
    while (pt)
      {
	  /* counting how many Points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count == 0)
	return;
    ln = gaiaAddLinestringToGeomColl (geom, count);
    iv = 0;
    pt = dyn->First;
    while (pt)
      {
	  /* copying points into the LINESTRING */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
}

static void
append_shared_path (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int order)
{
/* appends a Shared Path item to Dynamic Line */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (order)
      {
	  /* reversed order */
	  for (iv = ln->Points - 1; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z)
			  ;
		      else
			  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->Last->X && y == dyn->Last->Y)
			  ;
		      else
			  gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  /* conformant order */
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z)
			  ;
		      else
			  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->Last->X && y == dyn->Last->Y
			  && z == dyn->Last->Z && m == dyn->Last->M)
			  ;
		      else
			  gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->Last->X && y == dyn->Last->Y)
			  ;
		      else
			  gaiaAppendPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

static void
prepend_shared_path (gaiaDynamicLinePtr dyn, gaiaLinestringPtr ln, int order)
{
/* prepends a Shared Path item to Dynamic Line */
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (order)
      {
	  /* reversed order */
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z)
			  ;
		      else
			  gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->First->X && y == dyn->First->Y)
			  ;
		      else
			  gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
    else
      {
	  /* conformant order */
	  for (iv = ln->Points - 1; iv >= 0; iv--)
	    {
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z)
			  ;
		      else
			  gaiaPrependPointZToDynamicLine (dyn, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointMToDynamicLine (dyn, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      if (x == dyn->First->X && y == dyn->First->Y
			  && z == dyn->First->Z && m == dyn->First->M)
			  ;
		      else
			  gaiaPrependPointZMToDynamicLine (dyn, x, y, z, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      if (x == dyn->First->X && y == dyn->First->Y)
			  ;
		      else
			  gaiaPrependPointToDynamicLine (dyn, x, y);
		  }
	    }
      }
}

static gaiaGeomCollPtr
arrange_shared_paths (gaiaGeomCollPtr geom)
{
/* final aggregation step for shared paths */
    gaiaLinestringPtr ln;
    gaiaLinestringPtr *ln_array;
    gaiaGeomCollPtr result;
    gaiaDynamicLinePtr dyn;
    int count;
    int i;
    int i2;
    int iv;
    double x;
    double y;
    double z;
    double m;
    int ok;
    int ok2;

    if (!geom)
	return NULL;
    count = 0;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  count++;
	  ln = ln->Next;
      }
    if (count == 0)
	return NULL;

    ln_array = malloc (sizeof (gaiaLinestringPtr) * count);
    i = 0;
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* populating the Linestring references array */
	  ln_array[i++] = ln;
	  ln = ln->Next;
      }

/* allocating a new Geometry [MULTILINESTRING] */
    switch (geom->DimensionModel)
      {
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    result->DeclaredType = GAIA_MULTILINESTRING;

    ok = 1;
    while (ok)
      {
	  /* looping until we have processed any input item */
	  ok = 0;
	  for (i = 0; i < count; i++)
	    {
		if (ln_array[i] != NULL)
		  {
		      /* starting a new LINESTRING */
		      dyn = gaiaAllocDynamicLine ();
		      ln = ln_array[i];
		      ln_array[i] = NULL;
		      for (iv = 0; iv < ln->Points; iv++)
			{
			    /* inserting the 'seed' path */
			    if (ln->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
				  gaiaAppendPointZToDynamicLine (dyn, x, y, z);
			      }
			    else if (ln->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
				  gaiaAppendPointMToDynamicLine (dyn, x, y, m);
			      }
			    else if (ln->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z,
						    &m);
				  gaiaAppendPointZMToDynamicLine (dyn, x, y, z,
								  m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, iv, &x, &y);
				  gaiaAppendPointToDynamicLine (dyn, x, y);
			      }
			}
		      ok2 = 1;
		      while (ok2)
			{
			    /* looping until we have checked any other item */
			    ok2 = 0;
			    for (i2 = 0; i2 < count; i2++)
			      {
				  /* expanding the 'seed' path */
				  if (ln_array[i2] == NULL)
				      continue;
				  ln = ln_array[i2];
				  /* checking the first vertex */
				  iv = 0;
				  if (ln->DimensionModel == GAIA_XY_Z)
				    {
					gaiaGetPointXYZ (ln->Coords, iv, &x, &y,
							 &z);
				    }
				  else if (ln->DimensionModel == GAIA_XY_M)
				    {
					gaiaGetPointXYM (ln->Coords, iv, &x, &y,
							 &m);
				    }
				  else if (ln->DimensionModel == GAIA_XY_Z_M)
				    {
					gaiaGetPointXYZM (ln->Coords, iv, &x,
							  &y, &z, &m);
				    }
				  else
				    {
					gaiaGetPoint (ln->Coords, iv, &x, &y);
				    }
				  if (x == dyn->Last->X && y == dyn->Last->Y)
				    {
					/* appending this item to the 'seed' (conformant order) */
					append_shared_path (dyn, ln, 0);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  if (x == dyn->First->X && y == dyn->First->Y)
				    {
					/* prepending this item to the 'seed' (reversed order) */
					prepend_shared_path (dyn, ln, 1);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  /* checking the last vertex */
				  iv = ln->Points - 1;
				  if (ln->DimensionModel == GAIA_XY_Z)
				    {
					gaiaGetPointXYZ (ln->Coords, iv, &x, &y,
							 &z);
				    }
				  else if (ln->DimensionModel == GAIA_XY_M)
				    {
					gaiaGetPointXYM (ln->Coords, iv, &x, &y,
							 &m);
				    }
				  else if (ln->DimensionModel == GAIA_XY_Z_M)
				    {
					gaiaGetPointXYZM (ln->Coords, iv, &x,
							  &y, &z, &m);
				    }
				  else
				    {
					gaiaGetPoint (ln->Coords, iv, &x, &y);
				    }
				  if (x == dyn->Last->X && y == dyn->Last->Y)
				    {
					/* appending this item to the 'seed' (reversed order) */
					append_shared_path (dyn, ln, 1);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
				  if (x == dyn->First->X && y == dyn->First->Y)
				    {
					/* prepending this item to the 'seed' (conformant order) */
					prepend_shared_path (dyn, ln, 0);
					ln_array[i2] = NULL;
					ok2 = 1;
					continue;
				    }
			      }
			}
		      add_shared_linestring (result, dyn);
		      gaiaFreeDynamicLine (dyn);
		      ok = 1;
		      break;
		  }
	    }
      }
    free (ln_array);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSharedPaths (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/*
// builds a geometry containing Shared Paths commons to GEOM1 & GEOM2 
// (which are expected to be of the LINESTRING/MULTILINESTRING type)
//
*/
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    gaiaGeomCollPtr geo;
    gaiaGeomCollPtr line1;
    gaiaGeomCollPtr line2;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    gaiaResetGeosMsg ();
    if (!geom1)
	return NULL;
    if (!geom2)
	return NULL;
/* transforming input geoms as Lines */
    line1 = geom_as_lines (geom1);
    line2 = geom_as_lines (geom2);
    if (line1 == NULL || line2 == NULL)
      {
	  if (line1)
	      gaiaFreeGeomColl (line1);
	  if (line2)
	      gaiaFreeGeomColl (line2);
	  return NULL;
      }

    g1 = gaiaToGeos (line1);
    g2 = gaiaToGeos (line2);
    gaiaFreeGeomColl (line1);
    gaiaFreeGeomColl (line2);
    g3 = GEOSSharedPaths (g1, g2);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM (g3);
    else
	geo = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    result = arrange_shared_paths (geo);
    gaiaFreeGeomColl (geo);
#else
    if (geom1 == NULL || geom2 == NULL)
	geom1 = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSharedPaths_r (const void *p_cache, gaiaGeomCollPtr geom1,
		   gaiaGeomCollPtr geom2)
{
/*
// builds a geometry containing Shared Paths commons to GEOM1 & GEOM2 
// (which are expected to be of the LINESTRING/MULTILINESTRING type)
//
*/
    gaiaGeomCollPtr geo;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr line1;
    gaiaGeomCollPtr line2;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom1)
	return NULL;
    if (!geom2)
	return NULL;
/* transforming input geoms as Lines */
    line1 = geom_as_lines (geom1);
    line2 = geom_as_lines (geom2);
    if (line1 == NULL || line2 == NULL)
      {
	  if (line1)
	      gaiaFreeGeomColl (line1);
	  if (line2)
	      gaiaFreeGeomColl (line2);
	  return NULL;
      }

    g1 = gaiaToGeos_r (cache, line1);
    g2 = gaiaToGeos_r (cache, line2);
    gaiaFreeGeomColl (line1);
    gaiaFreeGeomColl (line2);
    g3 = GEOSSharedPaths_r (handle, g1, g2);
    GEOSGeom_destroy_r (handle, g1);
    GEOSGeom_destroy_r (handle, g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	geo = gaiaFromGeos_XYZ_r (cache, g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	geo = gaiaFromGeos_XYM_r (cache, g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	geo = gaiaFromGeos_XYZM_r (cache, g3);
    else
	geo = gaiaFromGeos_XY_r (cache, g3);
    GEOSGeom_destroy_r (handle, g3);
    if (geo == NULL)
	return NULL;
    geo->Srid = geom1->Srid;
    result = arrange_shared_paths (geo);
    gaiaFreeGeomColl (geo);
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineInterpolatePoint (gaiaGeomCollPtr geom, double fraction)
{
/*
 * attempts to intepolate a point on line at dist "fraction" 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_pt;
    double length;
    double projection;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    g = gaiaToGeos (geom);
    if (GEOSLength (g, &length))
      {
	  /* transforming fraction to length */
	  if (fraction < 0.0)
	      fraction = 0.0;
	  if (fraction > 1.0)
	      fraction = 1.0;
	  projection = length * fraction;
      }
    else
      {
	  GEOSGeom_destroy (g);
	  return NULL;
      }
    g_pt = GEOSInterpolate (g, projection);
    GEOSGeom_destroy (g);
    if (!g_pt)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g_pt);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g_pt);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g_pt);
    else
	result = gaiaFromGeos_XY (g_pt);
    GEOSGeom_destroy (g_pt);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
#else
    if (geom == NULL || fraction == 0.0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineInterpolatePoint_r (const void *p_cache, gaiaGeomCollPtr geom,
			    double fraction)
{
/*
 * attempts to intepolate a point on line at dist "fraction" 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_pt;
    double length;
    double projection;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    g = gaiaToGeos_r (cache, geom);
    if (GEOSLength_r (handle, g, &length))
      {
	  /* transforming fraction to length */
	  if (fraction < 0.0)
	      fraction = 0.0;
	  if (fraction > 1.0)
	      fraction = 1.0;
	  projection = length * fraction;
      }
    else
      {
	  GEOSGeom_destroy_r (handle, g);
	  return NULL;
      }
    g_pt = GEOSInterpolate_r (handle, g, projection);
    GEOSGeom_destroy_r (handle, g);
    if (!g_pt)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g_pt);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g_pt);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g_pt);
    else
	result = gaiaFromGeos_XY_r (cache, g_pt);
    GEOSGeom_destroy_r (handle, g_pt);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

static gaiaGeomCollPtr
gaiaLineInterpolateEquidistantPointsCommon (struct splite_internal_cache *cache,
					    gaiaGeomCollPtr geom,
					    double distance)
{
/*
 * attempts to intepolate a set of points on line at regular distances 
 */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr xpt;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_pt;
    double length;
    double current_length = 0.0;
    GEOSContextHandle_t handle = NULL;

    if (cache != NULL)
      {
	  if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	      || cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	      return NULL;
	  handle = cache->GEOS_handle;
	  if (handle == NULL)
	      return NULL;
      }
#ifdef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    if (handle == NULL)
	return NULL;
#endif
    if (!geom)
	return NULL;
    if (distance <= 0.0)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    if (cache != NULL)
      {
	  g = gaiaToGeos_r (cache, geom);
	  if (GEOSLength_r (handle, g, &length))
	    {
		if (length <= distance)
		  {
		      /* the line is too short to apply interpolation */
		      GEOSGeom_destroy_r (handle, g);
		      return NULL;
		  }
	    }
	  else
	    {
		GEOSGeom_destroy_r (handle, g);
		return NULL;
	    }
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  g = gaiaToGeos (geom);
	  if (GEOSLength (g, &length))
	    {
		if (length <= distance)
		  {
		      /* the line is too short to apply interpolation */
		      GEOSGeom_destroy (g);
		      return NULL;
		  }
	    }
	  else
	    {
		GEOSGeom_destroy (g);
		return NULL;
	    }
#endif
      }

/* creating the MultiPoint [always supporting M] */
    if (geom->DimensionModel == GAIA_XY_Z
	|| geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomCollXYM ();
    if (result == NULL)
      {
	  if (cache != NULL)
	    {
		GEOSGeom_destroy_r (handle, g);
		return NULL;
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		GEOSGeom_destroy (g);
		return NULL;
#endif
	    }
      }

    while (1)
      {
	  /* increasing the current distance */
	  current_length += distance;
	  if (current_length >= length)
	      break;
	  /* interpolating a point */
	  if (handle != NULL)
	      g_pt = GEOSInterpolate_r (handle, g, current_length);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  else
	      g_pt = GEOSInterpolate (g, current_length);
#endif
	  if (!g_pt)
	      goto error;
	  if (geom->DimensionModel == GAIA_XY_Z)
	    {
		if (cache != NULL)
		    xpt = gaiaFromGeos_XYZ_r (cache, g_pt);
		else
		    xpt = gaiaFromGeos_XYZ (g_pt);
		if (!xpt)
		    goto error;
		pt = xpt->FirstPoint;
		if (!pt)
		    goto error;
		gaiaAddPointToGeomCollXYZM (result, pt->X, pt->Y, pt->Z,
					    current_length);
	    }
	  else if (geom->DimensionModel == GAIA_XY_M)
	    {
		if (cache != NULL)
		    xpt = gaiaFromGeos_XYM_r (cache, g_pt);
		else
		    xpt = gaiaFromGeos_XYM (g_pt);
		if (!xpt)
		    goto error;
		pt = xpt->FirstPoint;
		if (!pt)
		    goto error;
		gaiaAddPointToGeomCollXYM (result, pt->X, pt->Y,
					   current_length);
	    }
	  else if (geom->DimensionModel == GAIA_XY_Z_M)
	    {
		if (cache != NULL)
		    xpt = gaiaFromGeos_XYZM_r (cache, g_pt);
		else
		    xpt = gaiaFromGeos_XYZM (g_pt);
		if (!xpt)
		    goto error;
		pt = xpt->FirstPoint;
		if (!pt)
		    goto error;
		gaiaAddPointToGeomCollXYZM (result, pt->X, pt->Y, pt->Z,
					    current_length);
	    }
	  else
	    {
		if (cache != NULL)
		    xpt = gaiaFromGeos_XY_r (cache, g_pt);
		else
		    xpt = gaiaFromGeos_XY (g_pt);
		if (!xpt)
		    goto error;
		pt = xpt->FirstPoint;
		if (!pt)
		    goto error;
		gaiaAddPointToGeomCollXYM (result, pt->X, pt->Y,
					   current_length);
	    }
	  if (handle != NULL)
	      GEOSGeom_destroy_r (handle, g_pt);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  else
	      GEOSGeom_destroy (g_pt);
#endif
	  gaiaFreeGeomColl (xpt);
      }
    if (handle != NULL)
	GEOSGeom_destroy_r (handle, g);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    else
	GEOSGeom_destroy (g);
#endif
    result->Srid = geom->Srid;
    result->DeclaredType = GAIA_MULTIPOINT;
    return result;

  error:
    if (handle != NULL)
      {
	  if (g_pt)
	      GEOSGeom_destroy_r (handle, g_pt);
	  GEOSGeom_destroy_r (handle, g);
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  if (g_pt)
	      GEOSGeom_destroy (g_pt);
	  GEOSGeom_destroy (g);
#endif
      }
    gaiaFreeGeomColl (result);
    return NULL;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineInterpolateEquidistantPoints (gaiaGeomCollPtr geom, double distance)
{
    gaiaResetGeosMsg ();
    return gaiaLineInterpolateEquidistantPointsCommon (NULL, geom, distance);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineInterpolateEquidistantPoints_r (const void *p_cache,
					gaiaGeomCollPtr geom, double distance)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    return gaiaLineInterpolateEquidistantPointsCommon (cache, geom, distance);
}

GAIAGEO_DECLARE double
gaiaLineLocatePoint (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* 
 * attempts to compute the location of the closest point on LineString 
 * to the given Point, as a fraction of total 2d line length 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    double result = -1.0;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    int pts1 = 0;
    int lns1 = 0;
    int pgs1 = 0;
    int pts2 = 0;
    int lns2 = 0;
    int pgs2 = 0;
    double length;
    double projection;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaResetGeosMsg ();
    if (!geom1 || !geom2)
	return -1.0;

/* checking if a single Linestring has been passed */
    pt = geom1->FirstPoint;
    while (pt)
      {
	  pts1++;
	  pt = pt->Next;
      }
    ln = geom1->FirstLinestring;
    while (ln)
      {
	  lns1++;
	  ln = ln->Next;
      }
    pg = geom1->FirstPolygon;
    while (pg)
      {
	  pgs1++;
	  pg = pg->Next;
      }
    if (pts1 == 0 && lns1 >= 1 && pgs1 == 0)
	;
    else
	return -1.0;

/* checking if a single Point has been passed */
    pt = geom2->FirstPoint;
    while (pt)
      {
	  pts2++;
	  pt = pt->Next;
      }
    ln = geom2->FirstLinestring;
    while (ln)
      {
	  lns2++;
	  ln = ln->Next;
      }
    pg = geom2->FirstPolygon;
    while (pg)
      {
	  pgs2++;
	  pg = pg->Next;
      }
    if (pts2 == 1 && lns2 == 0 && pgs2 == 0)
	;
    else
	return -1.0;

    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    projection = GEOSProject (g1, g2);
    if (GEOSLength (g1, &length))
      {
	  /* normalizing as a fraction between 0.0 and 1.0 */
	  result = projection / length;
      }
    else
	result = -1.0;
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
#else
    if (geom1 == NULL || geom2 == NULL)
	geom1 = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE double
gaiaLineLocatePoint_r (const void *p_cache, gaiaGeomCollPtr geom1,
		       gaiaGeomCollPtr geom2)
{
/* 
 * attempts to compute the location of the closest point on LineString 
 * to the given Point, as a fraction of total 2d line length 
 *
 * the fraction is expressed into the range from 0.0 to 1.0
 */
    int pts1 = 0;
    int lns1 = 0;
    int pgs1 = 0;
    int pts2 = 0;
    int lns2 = 0;
    int pgs2 = 0;
    double length;
    double projection;
    double result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return -1.0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return -1.0;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return -1.0;
    gaiaResetGeosMsg_r (cache);
    if (!geom1 || !geom2)
	return -1.0;

/* checking if a single Linestring has been passed */
    pt = geom1->FirstPoint;
    while (pt)
      {
	  pts1++;
	  pt = pt->Next;
      }
    ln = geom1->FirstLinestring;
    while (ln)
      {
	  lns1++;
	  ln = ln->Next;
      }
    pg = geom1->FirstPolygon;
    while (pg)
      {
	  pgs1++;
	  pg = pg->Next;
      }
    if (pts1 == 0 && lns1 >= 1 && pgs1 == 0)
	;
    else
	return -1.0;

/* checking if a single Point has been passed */
    pt = geom2->FirstPoint;
    while (pt)
      {
	  pts2++;
	  pt = pt->Next;
      }
    ln = geom2->FirstLinestring;
    while (ln)
      {
	  lns2++;
	  ln = ln->Next;
      }
    pg = geom2->FirstPolygon;
    while (pg)
      {
	  pgs2++;
	  pg = pg->Next;
      }
    if (pts2 == 1 && lns2 == 0 && pgs2 == 0)
	;
    else
	return -1.0;

    g1 = gaiaToGeos_r (cache, geom1);
    g2 = gaiaToGeos_r (cache, geom2);
    projection = GEOSProject_r (handle, g1, g2);
    if (GEOSLength_r (handle, g1, &length))
      {
	  /* normalizing as a fraction between 0.0 and 1.0 */
	  result = projection / length;
      }
    else
	result = -1.0;
    GEOSGeom_destroy_r (handle, g1);
    GEOSGeom_destroy_r (handle, g2);
    return result;
}

static gaiaGeomCollPtr
gaiaLineSubstringCommon (struct splite_internal_cache *cache,
			 gaiaGeomCollPtr geom, double start_fraction,
			 double end_fraction)
{
/* 
 * attempts to build a new Linestring being a substring of the input one starting 
 * and ending at the given fractions of total 2d length 
 */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr out;
    gaiaPolygonPtr pg;
    GEOSGeometry *g;
    GEOSGeometry *g_start;
    GEOSGeometry *g_end;
    GEOSCoordSequence *cs;
    const GEOSCoordSequence *in_cs;
    GEOSGeometry *segm;
    double length;
    double total = 0.0;
    double start;
    double end;
    int iv;
    int i_start = -1;
    int i_end = -1;
    int points;
    double x;
    double y;
    double z;
    double m;
    double x0;
    double y0;
    unsigned int dims;
    GEOSContextHandle_t handle = NULL;

    if (cache != NULL)
      {
	  if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	      || cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	      return NULL;
	  handle = cache->GEOS_handle;
	  if (handle == NULL)
	      return NULL;
      }
#ifdef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    if (handle == NULL)
	return NULL;
#endif
    if (!geom)
	return NULL;

/* checking if a single Linestring has been passed */
    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	;
    else
	return NULL;

    if (start_fraction < 0.0)
	start_fraction = 0.0;
    if (start_fraction > 1.0)
	start_fraction = 1.0;
    if (end_fraction < 0.0)
	end_fraction = 0.0;
    if (end_fraction > 1.0)
	end_fraction = 1.0;
    if (start_fraction >= end_fraction)
	return NULL;
    if (cache != NULL)
      {
	  g = gaiaToGeos_r (cache, geom);
	  if (GEOSLength_r (handle, g, &length))
	    {
		start = length * start_fraction;
		end = length * end_fraction;
	    }
	  else
	    {
		GEOSGeom_destroy_r (handle, g);
		return NULL;
	    }
	  g_start = GEOSInterpolate_r (handle, g, start);
	  g_end = GEOSInterpolate_r (handle, g, end);
	  GEOSGeom_destroy_r (handle, g);
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  g = gaiaToGeos (geom);
	  if (GEOSLength (g, &length))
	    {
		start = length * start_fraction;
		end = length * end_fraction;
	    }
	  else
	    {
		GEOSGeom_destroy (g);
		return NULL;
	    }
	  g_start = GEOSInterpolate (g, start);
	  g_end = GEOSInterpolate (g, end);
	  GEOSGeom_destroy (g);
#endif
      }
    if (!g_start || !g_end)
	return NULL;

/* identifying first and last valid vertex */
    x0 = 0.0;
    y0 = 0.0;
    ln = geom->FirstLinestring;
    for (iv = 0; iv < ln->Points; iv++)
      {
	  switch (ln->DimensionModel)
	    {
	    case GAIA_XY_Z:
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		break;
	    case GAIA_XY_M:
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		break;
	    case GAIA_XY_Z_M:
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		break;
	    default:
		gaiaGetPoint (ln->Coords, iv, &x, &y);
		break;
	    };

	  if (iv > 0)
	    {
		if (handle != NULL)
		  {
		      cs = GEOSCoordSeq_create_r (handle, 2, 2);
		      GEOSCoordSeq_setX_r (handle, cs, 0, x0);
		      GEOSCoordSeq_setY_r (handle, cs, 0, y0);
		      GEOSCoordSeq_setX_r (handle, cs, 1, x);
		      GEOSCoordSeq_setY_r (handle, cs, 1, y);
		      segm = GEOSGeom_createLineString_r (handle, cs);
		      GEOSLength_r (handle, segm, &length);
		      total += length;
		      GEOSGeom_destroy_r (handle, segm);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      cs = GEOSCoordSeq_create (2, 2);
		      GEOSCoordSeq_setX (cs, 0, x0);
		      GEOSCoordSeq_setY (cs, 0, y0);
		      GEOSCoordSeq_setX (cs, 1, x);
		      GEOSCoordSeq_setY (cs, 1, y);
		      segm = GEOSGeom_createLineString (cs);
		      GEOSLength (segm, &length);
		      total += length;
		      GEOSGeom_destroy (segm);
#endif
		  }
		if (total > start && i_start < 0)
		    i_start = iv;
		if (total < end)
		    i_end = iv;
	    }
	  x0 = x;
	  y0 = y;
      }
    if (i_start < 0 || i_end < 0)
      {
	  i_start = -1;
	  i_end = -1;
	  points = 2;
      }
    else
	points = i_end - i_start + 3;

/* creating the output geometry */
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom->Srid;
    out = gaiaAddLinestringToGeomColl (result, points);

/* start vertex */
    points = 0;
    if (handle)
      {
	  in_cs = GEOSGeom_getCoordSeq_r (handle, g_start);
	  GEOSCoordSeq_getDimensions_r (handle, in_cs, &dims);
	  if (dims == 3)
	    {
		GEOSCoordSeq_getX_r (handle, in_cs, 0, &x);
		GEOSCoordSeq_getY_r (handle, in_cs, 0, &y);
		GEOSCoordSeq_getZ_r (handle, in_cs, 0, &z);
		m = 0.0;
	    }
	  else
	    {
		GEOSCoordSeq_getX_r (handle, in_cs, 0, &x);
		GEOSCoordSeq_getY_r (handle, in_cs, 0, &y);
		z = 0.0;
		m = 0.0;
	    }
	  GEOSGeom_destroy_r (handle, g_start);
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  in_cs = GEOSGeom_getCoordSeq (g_start);
	  GEOSCoordSeq_getDimensions (in_cs, &dims);
	  if (dims == 3)
	    {
		GEOSCoordSeq_getX (in_cs, 0, &x);
		GEOSCoordSeq_getY (in_cs, 0, &y);
		GEOSCoordSeq_getZ (in_cs, 0, &z);
		m = 0.0;
	    }
	  else
	    {
		GEOSCoordSeq_getX (in_cs, 0, &x);
		GEOSCoordSeq_getY (in_cs, 0, &y);
		z = 0.0;
		m = 0.0;
	    }
	  GEOSGeom_destroy (g_start);
#endif
      }
    switch (out->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (out->Coords, points, x, y, z);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
	  break;
      default:
	  gaiaSetPoint (out->Coords, points, x, y);
	  break;
      };
    points++;

    if (i_start < 0 || i_end < 0)
	;
    else
      {
	  for (iv = i_start; iv <= i_end; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		switch (out->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaSetPointXYZ (out->Coords, points, x, y, z);
		      break;
		  case GAIA_XY_M:
		      gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
		      break;
		  default:
		      gaiaSetPoint (out->Coords, points, x, y);
		      break;
		  };
		points++;
	    }
      }

/* end vertex */
    if (handle != NULL)
      {
	  in_cs = GEOSGeom_getCoordSeq_r (handle, g_end);
	  GEOSCoordSeq_getDimensions_r (handle, in_cs, &dims);
	  if (dims == 3)
	    {
		GEOSCoordSeq_getX_r (handle, in_cs, 0, &x);
		GEOSCoordSeq_getY_r (handle, in_cs, 0, &y);
		GEOSCoordSeq_getZ_r (handle, in_cs, 0, &z);
		m = 0.0;
	    }
	  else
	    {
		GEOSCoordSeq_getX_r (handle, in_cs, 0, &x);
		GEOSCoordSeq_getY_r (handle, in_cs, 0, &y);
		z = 0.0;
		m = 0.0;
	    }
	  GEOSGeom_destroy_r (handle, g_end);
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  in_cs = GEOSGeom_getCoordSeq (g_end);
	  GEOSCoordSeq_getDimensions (in_cs, &dims);
	  if (dims == 3)
	    {
		GEOSCoordSeq_getX (in_cs, 0, &x);
		GEOSCoordSeq_getY (in_cs, 0, &y);
		GEOSCoordSeq_getZ (in_cs, 0, &z);
		m = 0.0;
	    }
	  else
	    {
		GEOSCoordSeq_getX (in_cs, 0, &x);
		GEOSCoordSeq_getY (in_cs, 0, &y);
		z = 0.0;
		m = 0.0;
	    }
	  GEOSGeom_destroy (g_end);
#endif
      }
    switch (out->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (out->Coords, points, x, y, z);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (out->Coords, points, x, y, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (out->Coords, points, x, y, z, 0.0);
	  break;
      default:
	  gaiaSetPoint (out->Coords, points, x, y);
	  break;
      };
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineSubstring (gaiaGeomCollPtr geom, double start_fraction,
		   double end_fraction)
{
    gaiaResetGeosMsg ();
    return gaiaLineSubstringCommon (NULL, geom, start_fraction, end_fraction);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineSubstring_r (const void *p_cache, gaiaGeomCollPtr geom,
		     double start_fraction, double end_fraction)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    return gaiaLineSubstringCommon (cache, geom, start_fraction, end_fraction);
}

static GEOSGeometry *
buildGeosPoints (GEOSContextHandle_t handle, const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry of POINTS */
    int pts = 0;
    unsigned int dims;
    int iv;
    int ib;
    int nItem;
    double x;
    double y;
    double z;
    double m;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    GEOSGeometry *geos = NULL;
    GEOSGeometry *geos_item;
    GEOSGeometry **geos_coll;
    GEOSCoordSequence *cs;

#ifdef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    if (handle == NULL)
	return NULL;
#endif
    if (!gaia)
	return NULL;

    pt = gaia->FirstPoint;
    while (pt)
      {
	  /* counting how many POINTs are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  /* counting how many POINTs are there */
	  pts += ln->Points;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many POINTs are there */
	  rng = pg->Exterior;
	  pts += rng->Points - 1;	/* exterior ring */
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		pts += rng->Points - 1;
	    }
	  pg = pg->Next;
      }
    if (pts == 0)
	return NULL;
    switch (gaia->DimensionModel)
      {
      case GAIA_XY_Z:
      case GAIA_XY_Z_M:
	  dims = 3;
	  break;
      default:
	  dims = 2;
	  break;
      };
    nItem = 0;
    geos_coll = malloc (sizeof (GEOSGeometry *) * (pts));
    pt = gaia->FirstPoint;
    while (pt)
      {
	  if (handle != NULL)
	    {
		cs = GEOSCoordSeq_create_r (handle, 1, dims);
		switch (pt->DimensionModel)
		  {
		  case GAIA_XY_Z:
		  case GAIA_XY_Z_M:
		      GEOSCoordSeq_setX_r (handle, cs, 0, pt->X);
		      GEOSCoordSeq_setY_r (handle, cs, 0, pt->Y);
		      GEOSCoordSeq_setZ_r (handle, cs, 0, pt->Z);
		      break;
		  default:
		      GEOSCoordSeq_setX_r (handle, cs, 0, pt->X);
		      GEOSCoordSeq_setY_r (handle, cs, 0, pt->Y);
		      break;
		  };
		geos_item = GEOSGeom_createPoint_r (handle, cs);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		cs = GEOSCoordSeq_create (1, dims);
		switch (pt->DimensionModel)
		  {
		  case GAIA_XY_Z:
		  case GAIA_XY_Z_M:
		      GEOSCoordSeq_setX (cs, 0, pt->X);
		      GEOSCoordSeq_setY (cs, 0, pt->Y);
		      GEOSCoordSeq_setZ (cs, 0, pt->Z);
		      break;
		  default:
		      GEOSCoordSeq_setX (cs, 0, pt->X);
		      GEOSCoordSeq_setY (cs, 0, pt->Y);
		      break;
		  };
		geos_item = GEOSGeom_createPoint (cs);
#endif
	    }
	  *(geos_coll + nItem++) = geos_item;
	  pt = pt->Next;
      }
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		if (handle != NULL)
		  {
		      cs = GEOSCoordSeq_create_r (handle, 1, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX_r (handle, cs, 0, x);
			    GEOSCoordSeq_setY_r (handle, cs, 0, y);
			    GEOSCoordSeq_setZ_r (handle, cs, 0, z);
			}
		      else
			{
			    GEOSCoordSeq_setX_r (handle, cs, 0, x);
			    GEOSCoordSeq_setY_r (handle, cs, 0, y);
			}
		      geos_item = GEOSGeom_createPoint_r (handle, cs);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      cs = GEOSCoordSeq_create (1, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			    GEOSCoordSeq_setZ (cs, 0, z);
			}
		      else
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			}
		      geos_item = GEOSGeom_createPoint (cs);
#endif
		  }
		*(geos_coll + nItem++) = geos_item;
	    }
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 1; iv < rng->Points; iv++)
	    {
		/* exterior ring */
		z = 0.0;
		m = 0.0;
		switch (rng->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      break;
		  };
		if (handle != NULL)
		  {
		      cs = GEOSCoordSeq_create_r (handle, 1, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX_r (handle, cs, 0, x);
			    GEOSCoordSeq_setY_r (handle, cs, 0, y);
			    GEOSCoordSeq_setZ_r (handle, cs, 0, z);
			}
		      else
			{
			    GEOSCoordSeq_setX_r (handle, cs, 0, x);
			    GEOSCoordSeq_setY_r (handle, cs, 0, y);
			}
		      geos_item = GEOSGeom_createPoint_r (handle, cs);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      cs = GEOSCoordSeq_create (1, dims);
		      if (dims == 3)
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			    GEOSCoordSeq_setZ (cs, 0, z);
			}
		      else
			{
			    GEOSCoordSeq_setX (cs, 0, x);
			    GEOSCoordSeq_setY (cs, 0, y);
			}
		      geos_item = GEOSGeom_createPoint (cs);
#endif
		  }
		*(geos_coll + nItem++) = geos_item;
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		for (iv = 1; iv < rng->Points; iv++)
		  {
		      /* exterior ring */
		      z = 0.0;
		      m = 0.0;
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    break;
			default:
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    break;
			};
		      if (handle != NULL)
			{
			    cs = GEOSCoordSeq_create_r (handle, 1, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y);
				  GEOSCoordSeq_setZ_r (handle, cs, 0, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y);
			      }
			    geos_item = GEOSGeom_createPoint_r (handle, cs);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    cs = GEOSCoordSeq_create (1, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX (cs, 0, x);
				  GEOSCoordSeq_setY (cs, 0, y);
				  GEOSCoordSeq_setZ (cs, 0, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX (cs, 0, x);
				  GEOSCoordSeq_setY (cs, 0, y);
			      }
			    geos_item = GEOSGeom_createPoint (cs);
#endif
			}
		      *(geos_coll + nItem++) = geos_item;
		  }
	    }
	  pg = pg->Next;
      }
    if (handle != NULL)
      {
	  geos =
	      GEOSGeom_createCollection_r (handle, GEOS_MULTIPOINT, geos_coll,
					   pts);
	  free (geos_coll);
	  GEOSSetSRID_r (handle, geos, gaia->Srid);
      }
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    else
      {
	  geos = GEOSGeom_createCollection (GEOS_MULTIPOINT, geos_coll, pts);
	  free (geos_coll);
	  GEOSSetSRID (geos, gaia->Srid);
      }
#endif
    return geos;
}

static GEOSGeometry *
buildGeosSegments (GEOSContextHandle_t handle, const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry of SEGMENTS */
    int segms = 0;
    unsigned int dims;
    int iv;
    int ib;
    int nItem;
    double x;
    double y;
    double z;
    double m;
    double x0 = 0.0;
    double y0 = 0.0;
    double z0 = 0.0;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    GEOSGeometry *geos = NULL;
    GEOSGeometry *geos_item;
    GEOSGeometry **geos_coll;
    GEOSCoordSequence *cs;

#ifdef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    if (handle == NULL)
	return NULL;
#endif
    if (!gaia)
	return NULL;

    ln = gaia->FirstLinestring;
    while (ln)
      {
	  /* counting how many SEGMENTs are there */
	  segms += ln->Points - 1;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many SEGMENTs are there */
	  rng = pg->Exterior;
	  segms += rng->Points - 1;	/* exterior ring */
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		segms += rng->Points - 1;
	    }
	  pg = pg->Next;
      }
    if (segms == 0)
	return NULL;
    switch (gaia->DimensionModel)
      {
      case GAIA_XY_Z:
      case GAIA_XY_Z_M:
	  dims = 3;
	  break;
      default:
	  dims = 2;
	  break;
      };
    nItem = 0;
    geos_coll = malloc (sizeof (GEOSGeometry *) * (segms));
    ln = gaia->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		z = 0.0;
		m = 0.0;
		switch (ln->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      break;
		  };
		if (iv > 0)
		  {
		      if (handle != NULL)
			{
			    cs = GEOSCoordSeq_create_r (handle, 2, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x0);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y0);
				  GEOSCoordSeq_setZ_r (handle, cs, 0, z0);
				  GEOSCoordSeq_setX_r (handle, cs, 1, x);
				  GEOSCoordSeq_setY_r (handle, cs, 1, y);
				  GEOSCoordSeq_setZ_r (handle, cs, 1, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x0);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y0);
				  GEOSCoordSeq_setX_r (handle, cs, 1, x);
				  GEOSCoordSeq_setY_r (handle, cs, 1, y);
			      }
			    geos_item =
				GEOSGeom_createLineString_r (handle, cs);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    cs = GEOSCoordSeq_create (2, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setZ (cs, 0, z0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
				  GEOSCoordSeq_setZ (cs, 1, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
			      }
			    geos_item = GEOSGeom_createLineString (cs);
#endif
			}
		      *(geos_coll + nItem++) = geos_item;
		  }
		x0 = x;
		y0 = y;
		z0 = z;
	    }
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior ring */
		z = 0.0;
		m = 0.0;
		switch (rng->DimensionModel)
		  {
		  case GAIA_XY_Z:
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      break;
		  case GAIA_XY_Z_M:
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      break;
		  default:
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      break;
		  };
		if (iv > 0)
		  {
		      if (handle != NULL)
			{
			    cs = GEOSCoordSeq_create_r (handle, 2, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x0);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y0);
				  GEOSCoordSeq_setZ_r (handle, cs, 0, z0);
				  GEOSCoordSeq_setX_r (handle, cs, 1, x);
				  GEOSCoordSeq_setY_r (handle, cs, 1, y);
				  GEOSCoordSeq_setZ_r (handle, cs, 1, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, x0);
				  GEOSCoordSeq_setY_r (handle, cs, 0, y0);
				  GEOSCoordSeq_setX_r (handle, cs, 1, x);
				  GEOSCoordSeq_setY_r (handle, cs, 1, y);
			      }
			    geos_item =
				GEOSGeom_createLineString_r (handle, cs);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    cs = GEOSCoordSeq_create (2, dims);
			    if (dims == 3)
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setZ (cs, 0, z0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
				  GEOSCoordSeq_setZ (cs, 1, z);
			      }
			    else
			      {
				  GEOSCoordSeq_setX (cs, 0, x0);
				  GEOSCoordSeq_setY (cs, 0, y0);
				  GEOSCoordSeq_setX (cs, 1, x);
				  GEOSCoordSeq_setY (cs, 1, y);
			      }
			    geos_item = GEOSGeom_createLineString (cs);
#endif
			}
		      *(geos_coll + nItem++) = geos_item;
		  }
		x0 = x;
		y0 = y;
		z0 = z;
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* interior ring */
		rng = pg->Interiors + ib;
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* exterior ring */
		      z = 0.0;
		      m = 0.0;
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    break;
			default:
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    break;
			};
		      if (iv > 0)
			{
			    if (handle != NULL)
			      {
				  cs = GEOSCoordSeq_create_r (handle, 2, dims);
				  if (dims == 3)
				    {
					GEOSCoordSeq_setX_r (handle, cs, 0, x0);
					GEOSCoordSeq_setY_r (handle, cs, 0, y0);
					GEOSCoordSeq_setZ_r (handle, cs, 0, z0);
					GEOSCoordSeq_setX_r (handle, cs, 1, x);
					GEOSCoordSeq_setY_r (handle, cs, 1, y);
					GEOSCoordSeq_setZ_r (handle, cs, 1, z);
				    }
				  else
				    {
					GEOSCoordSeq_setX_r (handle, cs, 0, x0);
					GEOSCoordSeq_setY_r (handle, cs, 0, y0);
					GEOSCoordSeq_setX_r (handle, cs, 1, x);
					GEOSCoordSeq_setY_r (handle, cs, 1, y);
				    }
				  geos_item =
				      GEOSGeom_createLineString_r (handle, cs);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  cs = GEOSCoordSeq_create (2, dims);
				  if (dims == 3)
				    {
					GEOSCoordSeq_setX (cs, 0, x0);
					GEOSCoordSeq_setY (cs, 0, y0);
					GEOSCoordSeq_setZ (cs, 0, z0);
					GEOSCoordSeq_setX (cs, 1, x);
					GEOSCoordSeq_setY (cs, 1, y);
					GEOSCoordSeq_setZ (cs, 1, z);
				    }
				  else
				    {
					GEOSCoordSeq_setX (cs, 0, x0);
					GEOSCoordSeq_setY (cs, 0, y0);
					GEOSCoordSeq_setX (cs, 1, x);
					GEOSCoordSeq_setY (cs, 1, y);
				    }
				  geos_item = GEOSGeom_createLineString (cs);
#endif
			      }
			    *(geos_coll + nItem++) = geos_item;
			}
		      x0 = x;
		      y0 = y;
		      z0 = z;
		  }
	    }
	  pg = pg->Next;
      }
    if (handle != NULL)
      {
	  geos =
	      GEOSGeom_createCollection_r (handle, GEOS_MULTILINESTRING,
					   geos_coll, segms);
	  free (geos_coll);
	  GEOSSetSRID_r (handle, geos, gaia->Srid);
      }
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    else
      {
	  geos =
	      GEOSGeom_createCollection (GEOS_MULTILINESTRING, geos_coll,
					 segms);
	  free (geos_coll);
	  GEOSSetSRID (geos, gaia->Srid);
      }
#endif
    return geos;
}

static gaiaGeomCollPtr
gaiaShortestLineCommon (struct splite_internal_cache *cache,
			gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* attempts to compute the shortest line between two geometries */
    GEOSGeometry *g1_points;
    GEOSGeometry *g1_segments;
    const GEOSGeometry *g1_item;
    GEOSGeometry *g2_points;
    GEOSGeometry *g2_segments;
    const GEOSGeometry *g2_item;
    const GEOSCoordSequence *cs;
    GEOSGeometry *g_pt;
    gaiaGeomCollPtr result = NULL;
    gaiaLinestringPtr ln;
    int nItems1;
    int nItems2;
    int it1;
    int it2;
    unsigned int dims;
    double x_ini = 0.0;
    double y_ini = 0.0;
    double z_ini = 0.0;
    double x_fin = 0.0;
    double y_fin = 0.0;
    double z_fin = 0.0;
    double dist;
    double min_dist = DBL_MAX;
    double projection;
    GEOSContextHandle_t handle = NULL;

    if (cache != NULL)
      {
	  if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	      || cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	      return NULL;
	  handle = cache->GEOS_handle;
	  if (handle == NULL)
	      return NULL;
      }
#ifdef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    if (handle == NULL)
	return NULL;
#endif
    if (!geom1 || !geom2)
	return NULL;

    g1_points = buildGeosPoints (handle, geom1);
    g1_segments = buildGeosSegments (handle, geom1);
    g2_points = buildGeosPoints (handle, geom2);
    g2_segments = buildGeosSegments (handle, geom2);

    if (g1_points && g2_points)
      {
	  /* computing distances between POINTs */
	  if (handle != NULL)
	    {
		nItems1 = GEOSGetNumGeometries_r (handle, g1_points);
		nItems2 = GEOSGetNumGeometries_r (handle, g2_points);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		nItems1 = GEOSGetNumGeometries (g1_points);
		nItems2 = GEOSGetNumGeometries (g2_points);
#endif
	    }
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		if (handle != NULL)
		    g1_item = GEOSGetGeometryN_r (handle, g1_points, it1);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    g1_item = GEOSGetGeometryN (g1_points, it1);
#endif
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      int distret;
		      if (handle != NULL)
			{
			    g2_item =
				GEOSGetGeometryN_r (handle, g2_points, it2);
			    distret =
				GEOSDistance_r (handle, g1_item, g2_item,
						&dist);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    g2_item = GEOSGetGeometryN (g2_points, it2);
			    distret = GEOSDistance (g1_item, g2_item, &dist);
#endif
			}
		      if (distret)
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  min_dist = dist;
				  if (handle != NULL)
				    {
					cs = GEOSGeom_getCoordSeq_r (handle,
								     g1_item);
					GEOSCoordSeq_getDimensions_r (handle,
								      cs,
								      &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   0, &x_ini);
					      GEOSCoordSeq_getY_r (handle, cs,
								   0, &y_ini);
					      GEOSCoordSeq_getZ_r (handle, cs,
								   0, &z_ini);
					  }
					else
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   0, &x_ini);
					      GEOSCoordSeq_getY_r (handle, cs,
								   0, &y_ini);
					      z_ini = 0.0;
					  }
					cs = GEOSGeom_getCoordSeq_r (handle,
								     g2_item);
					GEOSCoordSeq_getDimensions_r (handle,
								      cs,
								      &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   0, &x_fin);
					      GEOSCoordSeq_getY_r (handle, cs,
								   0, &y_fin);
					      GEOSCoordSeq_getZ_r (handle, cs,
								   0, &z_fin);
					  }
					else
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   0, &x_fin);
					      GEOSCoordSeq_getY_r (handle, cs,
								   0, &y_fin);
					      z_fin = 0.0;
					  }
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					cs = GEOSGeom_getCoordSeq (g1_item);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      GEOSCoordSeq_getZ (cs, 0, &z_ini);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_ini);
					      GEOSCoordSeq_getY (cs, 0, &y_ini);
					      z_ini = 0.0;
					  }
					cs = GEOSGeom_getCoordSeq (g2_item);
					GEOSCoordSeq_getDimensions (cs, &dims);
					if (dims == 3)
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      GEOSCoordSeq_getZ (cs, 0, &z_fin);
					  }
					else
					  {
					      GEOSCoordSeq_getX (cs, 0, &x_fin);
					      GEOSCoordSeq_getY (cs, 0, &y_fin);
					      z_fin = 0.0;
					  }
#endif
				    }
			      }
			}
		  }
	    }
      }

    if (g1_points && g2_segments)
      {
	  /* computing distances between POINTs (g1) and SEGMENTs (g2) */
	  if (handle != NULL)
	    {
		nItems1 = GEOSGetNumGeometries_r (handle, g1_points);
		nItems2 = GEOSGetNumGeometries_r (handle, g2_segments);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		nItems1 = GEOSGetNumGeometries (g1_points);
		nItems2 = GEOSGetNumGeometries (g2_segments);
#endif
	    }
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		if (handle != NULL)
		    g1_item = GEOSGetGeometryN_r (handle, g1_points, it1);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    g1_item = GEOSGetGeometryN (g1_points, it1);
#endif
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      int distret;
		      if (handle != NULL)
			{
			    g2_item =
				GEOSGetGeometryN_r (handle, g2_segments, it2);
			    distret =
				GEOSDistance_r (handle, g1_item, g2_item,
						&dist);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    g2_item = GEOSGetGeometryN (g2_segments, it2);
			    distret = GEOSDistance (g1_item, g2_item, &dist);
#endif
			}
		      if (distret)
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  if (handle != NULL)
				    {
					projection =
					    GEOSProject_r (handle, g2_item,
							   g1_item);
					g_pt =
					    GEOSInterpolate_r (handle, g2_item,
							       projection);
					if (g_pt)
					  {
					      min_dist = dist;
					      cs = GEOSGeom_getCoordSeq_r
						  (handle, g1_item);
					      GEOSCoordSeq_getDimensions_r
						  (handle, cs, &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_ini);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_ini);
						    GEOSCoordSeq_getZ_r (handle,
									 cs, 0,
									 &z_ini);
						}
					      else
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_ini);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_ini);
						    z_ini = 0.0;
						}
					      cs = GEOSGeom_getCoordSeq_r
						  (handle, g_pt);
					      GEOSCoordSeq_getDimensions_r
						  (handle, cs, &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_fin);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_fin);
						    GEOSCoordSeq_getZ_r (handle,
									 cs, 0,
									 &z_fin);
						}
					      else
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_fin);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_fin);
						    z_fin = 0.0;
						}
					      GEOSGeom_destroy_r (handle, g_pt);
					  }
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					projection =
					    GEOSProject (g2_item, g1_item);
					g_pt =
					    GEOSInterpolate (g2_item,
							     projection);
					if (g_pt)
					  {
					      min_dist = dist;
					      cs = GEOSGeom_getCoordSeq
						  (g1_item);
					      GEOSCoordSeq_getDimensions (cs,
									  &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_ini);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_ini);
						    GEOSCoordSeq_getZ (cs, 0,
								       &z_ini);
						}
					      else
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_ini);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_ini);
						    z_ini = 0.0;
						}
					      cs = GEOSGeom_getCoordSeq (g_pt);
					      GEOSCoordSeq_getDimensions (cs,
									  &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_fin);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_fin);
						    GEOSCoordSeq_getZ (cs, 0,
								       &z_fin);
						}
					      else
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_fin);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_fin);
						    z_fin = 0.0;
						}
					      GEOSGeom_destroy (g_pt);
					  }
#endif
				    }
			      }
			}
		  }
	    }
      }

    if (g1_segments && g2_points)
      {
	  /* computing distances between SEGMENTs (g1) and POINTs (g2) */
	  if (handle != NULL)
	    {
		nItems1 = GEOSGetNumGeometries_r (handle, g1_segments);
		nItems2 = GEOSGetNumGeometries_r (handle, g2_points);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		nItems1 = GEOSGetNumGeometries (g1_segments);
		nItems2 = GEOSGetNumGeometries (g2_points);
#endif
	    }
	  for (it1 = 0; it1 < nItems1; it1++)
	    {
		if (handle != NULL)
		    g1_item = GEOSGetGeometryN_r (handle, g1_segments, it1);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    g1_item = GEOSGetGeometryN (g1_segments, it1);
#endif
		for (it2 = 0; it2 < nItems2; it2++)
		  {
		      int distret;
		      if (handle != NULL)
			{
			    g2_item =
				GEOSGetGeometryN_r (handle, g2_points, it2);
			    distret =
				GEOSDistance_r (handle, g1_item, g2_item,
						&dist);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    g2_item = GEOSGetGeometryN (g2_points, it2);
			    distret = GEOSDistance (g1_item, g2_item, &dist);
#endif
			}
		      if (distret)
			{
			    if (dist < min_dist)
			      {
				  /* saving min-dist points */
				  if (handle != NULL)
				    {
					projection =
					    GEOSProject_r (handle, g1_item,
							   g2_item);
					g_pt =
					    GEOSInterpolate_r (handle, g1_item,
							       projection);
					if (g_pt)
					  {
					      min_dist = dist;
					      cs = GEOSGeom_getCoordSeq_r
						  (handle, g_pt);
					      GEOSCoordSeq_getDimensions_r
						  (handle, cs, &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_ini);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_ini);
						    GEOSCoordSeq_getZ_r (handle,
									 cs, 0,
									 &z_ini);
						}
					      else
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_ini);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_ini);
						    z_ini = 0.0;
						}
					      cs = GEOSGeom_getCoordSeq_r
						  (handle, g2_item);
					      GEOSCoordSeq_getDimensions_r
						  (handle, cs, &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_fin);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_fin);
						    GEOSCoordSeq_getZ_r (handle,
									 cs, 0,
									 &z_fin);
						}
					      else
						{
						    GEOSCoordSeq_getX_r (handle,
									 cs, 0,
									 &x_fin);
						    GEOSCoordSeq_getY_r (handle,
									 cs, 0,
									 &y_fin);
						    z_fin = 0.0;
						}
					      GEOSGeom_destroy_r (handle, g_pt);
					  }
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					projection =
					    GEOSProject (g1_item, g2_item);
					g_pt =
					    GEOSInterpolate (g1_item,
							     projection);
					if (g_pt)
					  {
					      min_dist = dist;
					      cs = GEOSGeom_getCoordSeq (g_pt);
					      GEOSCoordSeq_getDimensions (cs,
									  &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_ini);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_ini);
						    GEOSCoordSeq_getZ (cs, 0,
								       &z_ini);
						}
					      else
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_ini);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_ini);
						    z_ini = 0.0;
						}
					      cs = GEOSGeom_getCoordSeq
						  (g2_item);
					      GEOSCoordSeq_getDimensions (cs,
									  &dims);
					      if (dims == 3)
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_fin);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_fin);
						    GEOSCoordSeq_getZ (cs, 0,
								       &z_fin);
						}
					      else
						{
						    GEOSCoordSeq_getX (cs, 0,
								       &x_fin);
						    GEOSCoordSeq_getY (cs, 0,
								       &y_fin);
						    z_fin = 0.0;
						}
					      GEOSGeom_destroy (g_pt);
					  }
#endif
				    }
			      }
			}
		  }
	    }
      }
    if (handle != NULL)
      {
	  if (g1_points)
	      GEOSGeom_destroy_r (handle, g1_points);
	  if (g1_segments)
	      GEOSGeom_destroy_r (handle, g1_segments);
	  if (g2_points)
	      GEOSGeom_destroy_r (handle, g2_points);
	  if (g2_segments)
	      GEOSGeom_destroy_r (handle, g2_segments);
      }
    else
      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  if (g1_points)
	      GEOSGeom_destroy (g1_points);
	  if (g1_segments)
	      GEOSGeom_destroy (g1_segments);
	  if (g2_points)
	      GEOSGeom_destroy (g2_points);
	  if (g2_segments)
	      GEOSGeom_destroy (g2_segments);
#endif
      }
    if (min_dist == DBL_MAX || min_dist <= 0.0)
	return NULL;

/* building the shortest line */
    switch (geom1->DimensionModel)
      {
      case GAIA_XY_Z:
	  result = gaiaAllocGeomCollXYZ ();
	  break;
      case GAIA_XY_M:
	  result = gaiaAllocGeomCollXYM ();
	  break;
      case GAIA_XY_Z_M:
	  result = gaiaAllocGeomCollXYZM ();
	  break;
      default:
	  result = gaiaAllocGeomColl ();
	  break;
      };
    result->Srid = geom1->Srid;
    ln = gaiaAddLinestringToGeomColl (result, 2);
    switch (ln->DimensionModel)
      {
      case GAIA_XY_Z:
	  gaiaSetPointXYZ (ln->Coords, 0, x_ini, y_ini, z_ini);
	  gaiaSetPointXYZ (ln->Coords, 1, x_fin, y_fin, z_fin);
	  break;
      case GAIA_XY_M:
	  gaiaSetPointXYM (ln->Coords, 0, x_ini, y_ini, 0.0);
	  gaiaSetPointXYM (ln->Coords, 1, x_fin, y_fin, 0.0);
	  break;
      case GAIA_XY_Z_M:
	  gaiaSetPointXYZM (ln->Coords, 0, x_ini, y_ini, z_ini, 0.0);
	  gaiaSetPointXYZM (ln->Coords, 1, x_fin, y_fin, z_fin, 0.0);
	  break;
      default:
	  gaiaSetPoint (ln->Coords, 0, x_ini, y_ini);
	  gaiaSetPoint (ln->Coords, 1, x_fin, y_fin);
	  break;
      };
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaShortestLine (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
    gaiaResetGeosMsg ();
    return gaiaShortestLineCommon (NULL, geom1, geom2);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaShortestLine_r (const void *p_cache, gaiaGeomCollPtr geom1,
		    gaiaGeomCollPtr geom2)
{
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    return gaiaShortestLineCommon (cache, geom1, geom2);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSnap (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2, double tolerance)
{
/* attempts to "snap" geom1 on geom2 using the given tolerance */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    gaiaResetGeosMsg ();
    if (!geom1 || !geom2)
	return NULL;

    g1 = gaiaToGeos (geom1);
    g2 = gaiaToGeos (geom2);
    g3 = GEOSSnap (g1, g2, tolerance);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g3);
    else
	result = gaiaFromGeos_XY (g3);
    GEOSGeom_destroy (g3);
    if (result == NULL)
	return NULL;
    result->Srid = geom1->Srid;
#else
    if (geom1 == NULL || geom2 == NULL || tolerance == 0.0)
	geom1 = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSnap_r (const void *p_cache, gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2,
	    double tolerance)
{
/* attempts to "snap" geom1 on geom2 using the given tolerance */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    GEOSGeometry *g3;
    gaiaGeomCollPtr result;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom1 || !geom2)
	return NULL;

    g1 = gaiaToGeos_r (cache, geom1);
    g2 = gaiaToGeos_r (cache, geom2);
    g3 = GEOSSnap_r (handle, g1, g2, tolerance);
    GEOSGeom_destroy_r (handle, g1);
    GEOSGeom_destroy_r (handle, g2);
    if (!g3)
	return NULL;
    if (geom1->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g3);
    else if (geom1->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g3);
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g3);
    else
	result = gaiaFromGeos_XY_r (cache, g3);
    GEOSGeom_destroy_r (handle, g3);
    if (result == NULL)
	return NULL;
    result->Srid = geom1->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineMerge (gaiaGeomCollPtr geom)
{
/* attempts to reassemble lines from a collection of sparse fragments */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;
    if (gaiaIsToxic (geom))
	return NULL;

    g1 = gaiaToGeos (geom);
    g2 = GEOSLineMerge (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
#else
    if (geom == NULL)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineMerge_r (const void *p_cache, gaiaGeomCollPtr geom)
{
/* attempts to reassemble lines from a collection of sparse fragments */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;
    if (gaiaIsToxic_r (cache, geom))
	return NULL;

    g1 = gaiaToGeos_r (cache, geom);
    g2 = GEOSLineMerge_r (handle, g1);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaUnaryUnion (gaiaGeomCollPtr geom)
{
/* Unary Union (single Collection) */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;
    if (gaiaIsToxic (geom))
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSUnaryUnion (g1);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
#else
    if (geom == NULL)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaUnaryUnion_r (const void *p_cache, gaiaGeomCollPtr geom)
{
/* Unary Union (single Collection) */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;
    if (gaiaIsToxic_r (cache, geom))
	return NULL;
    g1 = gaiaToGeos_r (cache, geom);
    g2 = GEOSUnaryUnion_r (handle, g1);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    return result;
}

static void
rotateRingBeforeCut (gaiaLinestringPtr ln, gaiaPointPtr node)
{
/* rotating a Ring, so to ensure that Start/End points match the node */
    int io = 0;
    int iv;
    int copy = 0;
    int base_idx = -1;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    gaiaLinestringPtr new_ln = NULL;

    if (ln->DimensionModel == GAIA_XY_Z)
	new_ln = gaiaAllocLinestringXYZ (ln->Points);
    else if (ln->DimensionModel == GAIA_XY_M)
	new_ln = gaiaAllocLinestringXYM (ln->Points);
    else if (ln->DimensionModel == GAIA_XY_Z_M)
	new_ln = gaiaAllocLinestringXYZM (ln->Points);
    else
	new_ln = gaiaAllocLinestring (ln->Points);

/* first pass */
    for (iv = 0; iv < ln->Points; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  if (!copy)
	    {
		if (ln->DimensionModel == GAIA_XY_Z
		    || ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (node->X == x && node->Y == y && node->Z == z)
			{
			    base_idx = iv;
			    copy = 1;
			}
		  }
		else if (node->X == x && node->Y == y)
		  {
		      base_idx = iv;
		      copy = 1;
		  }
	    }
	  if (copy)
	    {
		/* copying points */
		if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (new_ln->Coords, io, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (new_ln->Coords, io, x, y, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (new_ln->Coords, io, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (new_ln->Coords, io, x, y);
		  }
		io++;
	    }
      }
    if (base_idx <= 0)
      {
	  gaiaFreeLinestring (new_ln);
	  return;
      }

/* second pass */
    for (iv = 1; iv <= base_idx; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (new_ln->Coords, io, x, y, z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (new_ln->Coords, io, x, y, m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (new_ln->Coords, io, x, y, z, m);
	    }
	  else
	    {
		gaiaSetPoint (new_ln->Coords, io, x, y);
	    }
	  io++;
      }

/* copying back */
    for (iv = 0; iv < new_ln->Points; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (new_ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (new_ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (new_ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (new_ln->Coords, iv, &x, &y);
	    }
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, iv, x, y, m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, iv, x, y, z, m);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, x, y);
	    }
      }
    gaiaFreeLinestring (new_ln);
}

static void
extractSubLine (gaiaGeomCollPtr result, gaiaLinestringPtr ln, int i_start,
		int i_end)
{
/* extracting s SubLine */
    int iv;
    int io = 0;
    int pts = i_end - i_start + 1;
    gaiaLinestringPtr new_ln = NULL;
    double x;
    double y;
    double z;
    double m;

    new_ln = gaiaAddLinestringToGeomColl (result, pts);

    for (iv = i_start; iv <= i_end; iv++)
      {
	  m = 0.0;
	  z = 0.0;
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (new_ln->Coords, io, x, y, z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (new_ln->Coords, io, x, y, m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (new_ln->Coords, io, x, y, z, m);
	    }
	  else
	    {
		gaiaSetPoint (new_ln->Coords, io, x, y);
	    }
	  io++;
      }
}

static void
cutLineAtNodes (gaiaLinestringPtr ln, gaiaPointPtr pt_base,
		gaiaGeomCollPtr result)
{
/* attempts to cut a single Line accordingly to given nodes */
    int closed = 0;
    int match = 0;
    int iv;
    int i_start;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    gaiaPointPtr pt;
    gaiaPointPtr node = NULL;

    if (gaiaIsClosed (ln))
	closed = 1;
/* pre-check */
    for (iv = 0; iv < ln->Points; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  pt = pt_base;
	  while (pt)
	    {
		if (ln->DimensionModel == GAIA_XY_Z
		    || ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z)
			{
			    node = pt;
			    match++;
			}
		  }
		else if (pt->X == x && pt->Y == y)
		  {
		      node = pt;
		      match++;
		  }
		pt = pt->Next;
	    }
      }

    if (closed && node)
	rotateRingBeforeCut (ln, node);

    i_start = 0;
    for (iv = 1; iv < ln->Points - 1; iv++)
      {
	  /* identifying sub-linestrings */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  match = 0;
	  pt = pt_base;
	  while (pt)
	    {
		if (ln->DimensionModel == GAIA_XY_Z
		    || ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      if (pt->X == x && pt->Y == y && pt->Z == z)
			{
			    match = 1;
			    break;
			}
		  }
		else if (pt->X == x && pt->Y == y)
		  {
		      match = 1;
		      break;
		  }
		pt = pt->Next;
	    }
	  if (match)
	    {
		/* cutting the line */
		extractSubLine (result, ln, i_start, iv);
		i_start = iv;
	    }
      }
    if (i_start != 0 && i_start != ln->Points - 1)
      {
	  /* extracting the last SubLine */
	  extractSubLine (result, ln, i_start, ln->Points - 1);
      }
    else
      {
	  /* cloning the untouched Line */
	  extractSubLine (result, ln, 0, ln->Points - 1);
      }
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLinesCutAtNodes (gaiaGeomCollPtr geom1, gaiaGeomCollPtr geom2)
{
/* attempts to cut lines accordingly to nodes */
    int pts1 = 0;
    int lns1 = 0;
    int pgs1 = 0;
    int pts2 = 0;
    int lns2 = 0;
    int pgs2 = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaGeomCollPtr result = NULL;

    if (!geom1)
	return NULL;
    if (!geom2)
	return NULL;

/* both Geometryes should have identical Dimensions */
    if (geom1->DimensionModel != geom2->DimensionModel)
	return NULL;

    pt = geom1->FirstPoint;
    while (pt)
      {
	  pts1++;
	  pt = pt->Next;
      }
    ln = geom1->FirstLinestring;
    while (ln)
      {
	  lns1++;
	  ln = ln->Next;
      }
    pg = geom1->FirstPolygon;
    while (pg)
      {
	  pgs1++;
	  pg = pg->Next;
      }
    pt = geom2->FirstPoint;
    while (pt)
      {
	  pts2++;
	  pt = pt->Next;
      }
    ln = geom2->FirstLinestring;
    while (ln)
      {
	  lns2++;
	  ln = ln->Next;
      }
    pg = geom2->FirstPolygon;
    while (pg)
      {
	  pgs2++;
	  pg = pg->Next;
      }

/* the first Geometry is expected to contain one or more Linestring(s) */
    if (pts1 == 0 && lns1 > 0 && pgs1 == 0)
	;
    else
	return NULL;
/* the second Geometry is expected to contain one or more Point(s) */
    if (pts2 > 0 && lns2 == 0 && pgs2 == 0)
	;
    else
	return NULL;

/* attempting to cut Lines accordingly to Nodes */
    if (geom1->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom1->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom1->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    ln = geom1->FirstLinestring;
    while (ln)
      {
	  cutLineAtNodes (ln, geom2->FirstPoint, result);
	  ln = ln->Next;
      }
    if (result->FirstLinestring == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    result->Srid = geom1->Srid;
    return result;
}

#ifdef GEOS_ADVANCED		/* GEOS advanced features - 3.4.0 */

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDelaunayTriangulation (gaiaGeomCollPtr geom, double tolerance,
			   int only_edges)
{
/* Delaunay Triangulation */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSDelaunayTriangulation (g1, tolerance, only_edges);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    if (only_edges)
	result->DeclaredType = GAIA_MULTILINESTRING;
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
#else
    if (geom == NULL || tolerance == 0.0 || only_edges == 0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDelaunayTriangulation_r (const void *p_cache, gaiaGeomCollPtr geom,
			     double tolerance, int only_edges)
{
/* Delaunay Triangulation */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;
    g1 = gaiaToGeos_r (cache, geom);
    g2 = GEOSDelaunayTriangulation_r (handle, g1, tolerance, only_edges);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (result == NULL)
	return NULL;
    result->Srid = geom->Srid;
    if (only_edges)
	result->DeclaredType = GAIA_MULTILINESTRING;
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
    return result;
}

#ifdef GEOS_REENTRANT		/* only if GEOS >= 3.5.0 directly supporting Voronoj */
static gaiaGeomCollPtr
voronoj_envelope (gaiaGeomCollPtr geom, double extra_frame_size)
{
/* building the extended envelope for Voronoj */
    gaiaGeomCollPtr bbox;
    gaiaPolygonPtr pg;
    gaiaRingPtr rect;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double ext_x;
    double ext_y;
    double delta;
    double delta2;

    gaiaMbrGeometry (geom);
/* setting the frame extent */
    if (extra_frame_size < 0.0)
	extra_frame_size = 5.0;
    ext_x = geom->MaxX - geom->MinX;
    ext_y = geom->MaxY - geom->MinY;
    delta = (ext_x * extra_frame_size) / 100.0;
    delta2 = (ext_y * extra_frame_size) / 100.0;
    if (delta2 > delta)
	delta = delta2;
    minx = geom->MinX - delta;
    miny = geom->MinY - delta;
    maxx = geom->MaxX + delta;
    maxy = geom->MaxY + delta;

/* building the frame */
    if (geom->DimensionModel == GAIA_XY_Z)
	bbox = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	bbox = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	bbox = gaiaAllocGeomCollXYZM ();
    else
	bbox = gaiaAllocGeomColl ();
    bbox->Srid = geom->Srid;
    bbox->DeclaredType = GAIA_POLYGON;
    pg = gaiaAddPolygonToGeomColl (bbox, 5, 0);
    rect = pg->Exterior;
    if (geom->DimensionModel == GAIA_XY_Z)
      {
	  gaiaSetPointXYZ (rect->Coords, 0, minx, miny, 0.0);
	  gaiaSetPointXYZ (rect->Coords, 1, maxx, miny, 0.0);
	  gaiaSetPointXYZ (rect->Coords, 2, maxx, maxy, 0.0);
	  gaiaSetPointXYZ (rect->Coords, 3, minx, maxy, 0.0);
	  gaiaSetPointXYZ (rect->Coords, 4, minx, miny, 0.0);
      }
    else if (geom->DimensionModel == GAIA_XY_M)
      {
	  gaiaSetPointXYM (rect->Coords, 0, minx, miny, 0.0);
	  gaiaSetPointXYM (rect->Coords, 1, maxx, miny, 0.0);
	  gaiaSetPointXYM (rect->Coords, 2, maxx, maxy, 0.0);
	  gaiaSetPointXYM (rect->Coords, 3, minx, maxy, 0.0);
	  gaiaSetPointXYM (rect->Coords, 4, minx, miny, 0.0);
      }
    else if (geom->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaSetPointXYZM (rect->Coords, 0, minx, miny, 0.0, 0.0);
	  gaiaSetPointXYZM (rect->Coords, 1, maxx, miny, 0.0, 0.0);
	  gaiaSetPointXYZM (rect->Coords, 2, maxx, maxy, 0.0, 0.0);
	  gaiaSetPointXYZM (rect->Coords, 3, minx, maxy, 0.0, 0.0);
	  gaiaSetPointXYZM (rect->Coords, 4, minx, miny, 0.0, 0.0);
      }
    else
      {
	  gaiaSetPoint (rect->Coords, 0, minx, miny);	/* vertex # 1 */
	  gaiaSetPoint (rect->Coords, 1, maxx, miny);	/* vertex # 2 */
	  gaiaSetPoint (rect->Coords, 2, maxx, maxy);	/* vertex # 3 */
	  gaiaSetPoint (rect->Coords, 3, minx, maxy);	/* vertex # 4 */
	  gaiaSetPoint (rect->Coords, 4, minx, miny);	/* vertex # 5 [same as vertex # 1 to close the polygon] */
      }

    return bbox;
}

static int
voronoj_mbr_contains (gaiaGeomCollPtr g1, gaiaGeomCollPtr g2)
{
/* checks if MBR#1 fully contains MBR#2 */
    if (g2->MinX < g1->MinX)
	return 0;
    if (g2->MaxX > g1->MaxX)
	return 0;
    if (g2->MinY < g1->MinY)
	return 0;
    if (g2->MaxY > g1->MaxY)
	return 0;
    return 1;
}

static int
voronoj_mbr_overlaps (gaiaGeomCollPtr g1, gaiaGeomCollPtr g2)
{
/* checks if two MBRs do overlap */
    if (g1->MaxX < g2->MinX)
	return 0;
    if (g1->MinX > g2->MaxX)
	return 0;
    if (g1->MaxY < g2->MinY)
	return 0;
    if (g1->MinY > g2->MaxY)
	return 0;
    return 1;
}

static gaiaGeomCollPtr
voronoj_postprocess (struct splite_internal_cache *cache, gaiaGeomCollPtr raw,
		     gaiaGeomCollPtr envelope, int only_edges)
{
/* postprocessing the result returned by GEOS Voronoj */
    gaiaGeomCollPtr candidate;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr framed;
    gaiaPolygonPtr pg;
    gaiaPolygonPtr new_pg;

    if (raw->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (raw->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (raw->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = raw->Srid;
    result->DeclaredType = GAIA_MULTIPOLYGON;

    if (raw->DimensionModel == GAIA_XY_Z)
	candidate = gaiaAllocGeomCollXYZ ();
    else if (raw->DimensionModel == GAIA_XY_M)
	candidate = gaiaAllocGeomCollXYM ();
    else if (raw->DimensionModel == GAIA_XY_Z_M)
	candidate = gaiaAllocGeomCollXYZM ();
    else
	candidate = gaiaAllocGeomColl ();
    candidate->Srid = raw->Srid;
    candidate->DeclaredType = GAIA_POLYGON;

    gaiaMbrGeometry (raw);
    gaiaMbrGeometry (envelope);
    pg = raw->FirstPolygon;
    while (pg != NULL)
      {
	  candidate->FirstPolygon = pg;
	  candidate->LastPolygon = pg;
	  candidate->MinX = pg->MinX;
	  candidate->MinY = pg->MinY;
	  candidate->MaxX = pg->MaxX;
	  candidate->MaxY = pg->MaxY;
	  if (voronoj_mbr_contains (envelope, candidate))
	    {
		/* copying a Polygon fully contained within the frame */
		new_pg = gaiaClonePolygon (pg);
		if (result->FirstPolygon == NULL)
		    result->FirstPolygon = new_pg;
		if (result->LastPolygon != NULL)
		    result->LastPolygon->Next = new_pg;
		result->LastPolygon = new_pg;
	    }
	  else if (voronoj_mbr_overlaps (envelope, candidate))
	    {
		/* found a polygon only partially contained within the frame */
		new_pg = gaiaClonePolygon (pg);
		candidate->FirstPolygon = new_pg;
		candidate->LastPolygon = new_pg;
		if (cache == NULL)
		    framed = gaiaGeometryIntersection (envelope, candidate);
		else
		    framed =
			gaiaGeometryIntersection_r (cache, envelope, candidate);
		candidate->FirstPolygon = NULL;
		candidate->LastPolygon = NULL;
		gaiaFreePolygon (new_pg);
		if (framed)
		  {
		      /* copying all framed polygons into the result */
		      gaiaPolygonPtr pg2 = framed->FirstPolygon;
		      while (pg2 != NULL)
			{
			    if (result->FirstPolygon == NULL)
				result->FirstPolygon = pg2;
			    if (result->LastPolygon != NULL)
				result->LastPolygon->Next = pg2;
			    result->LastPolygon = pg2;
			    pg2 = pg2->Next;
			}
		      framed->FirstPolygon = NULL;
		      framed->LastPolygon = NULL;
		      gaiaFreeGeomColl (framed);
		  }
	    }
	  pg = pg->Next;
      }

    candidate->FirstPolygon = NULL;
    candidate->LastPolygon = NULL;
    gaiaFreeGeomColl (candidate);
    gaiaFreeGeomColl (raw);
    if (only_edges)
      {
	  gaiaGeomCollPtr lines = gaiaLinearize (result, 1);
	  gaiaFreeGeomColl (result);
	  return lines;
      }
    return result;
}
#endif

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaVoronojDiagram (gaiaGeomCollPtr geom, double extra_frame_size,
		    double tolerance, int only_edges)
{
/* Voronoj Diagram */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
#ifdef GEOS_REENTRANT
    GEOSGeometry *env;
    gaiaGeomCollPtr envelope;
#else
    void *voronoj;
    gaiaPolygonPtr pg;
    int pgs = 0;
    int errs = 0;
#endif
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
#ifdef GEOS_REENTRANT		/* GEOS >= 3.5.0 directly supports Voronoj */
    envelope = voronoj_envelope (geom, extra_frame_size);
    env = gaiaToGeos (envelope);
    g2 = GEOSVoronoiDiagram (g1, env, tolerance, 0);
    GEOSGeom_destroy (g1);
    GEOSGeom_destroy (env);
    if (!g2)
      {
	  gaiaFreeGeomColl (envelope);
	  return NULL;
      }
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    result = voronoj_postprocess (NULL, result, envelope, only_edges);
    gaiaFreeGeomColl (envelope);
    if (result == NULL)
	return NULL;
#else
    g2 = GEOSDelaunayTriangulation (g1, tolerance, 0);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* counting how many triangles are in Delaunay */
	  if (delaunay_triangle_check (pg))
	      pgs++;
	  else
	      errs++;
	  pg = pg->Next;
      }
    if (pgs == 0 || errs)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

/* building the Voronoj Diagram from Delaunay */
    voronoj = voronoj_build (pgs, result->FirstPolygon, extra_frame_size);
    gaiaFreeGeomColl (result);

/* creating the Geometry representing Voronoj */
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    result = voronoj_export (voronoj, result, only_edges);
    voronoj_free (voronoj);

    result->Srid = geom->Srid;
    if (only_edges)
	result->DeclaredType = GAIA_MULTILINESTRING;
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
#endif /* end GEOS_REENTRANT */
#else
    if (geom == NULL || extra_frame_size == 0.0 || tolerance == 0.0
	|| only_edges == 0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif /* end GEOS_USE_ONLY_R_API */
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaVoronojDiagram_r (const void *p_cache, gaiaGeomCollPtr geom,
		      double extra_frame_size, double tolerance, int only_edges)
{
/* Voronoj Diagram */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
#ifdef GEOS_REENTRANT
    GEOSGeometry *env;
    gaiaGeomCollPtr envelope;
#else
    gaiaPolygonPtr pg;
    int pgs = 0;
    int errs = 0;
    void *voronoj;
#endif
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;
    g1 = gaiaToGeos_r (cache, geom);
#ifdef GEOS_REENTRANT		/* GEOS >= 3.5.0 directly supports Voronoj */
    envelope = voronoj_envelope (geom, extra_frame_size);
    env = gaiaToGeos_r (cache, envelope);
    g2 = GEOSVoronoiDiagram_r (handle, g1, env, tolerance, 0);
    GEOSGeom_destroy_r (handle, g1);
    GEOSGeom_destroy_r (handle, env);
    if (!g2)
      {
	  gaiaFreeGeomColl (envelope);
	  return NULL;
      }
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    result = voronoj_postprocess (cache, result, envelope, only_edges);
    gaiaFreeGeomColl (envelope);
    if (result == NULL)
	return NULL;
#else
    g2 = GEOSDelaunayTriangulation_r (handle, g1, tolerance, 0);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (result == NULL)
	return NULL;
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* counting how many triangles are in Delaunay */
	  if (delaunay_triangle_check (pg))
	      pgs++;
	  else
	      errs++;
	  pg = pg->Next;
      }
    if (pgs == 0 || errs)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

/* building the Voronoj Diagram from Delaunay */
    voronoj =
	voronoj_build_r (cache, pgs, result->FirstPolygon, extra_frame_size);
    gaiaFreeGeomColl (result);

/* creating the Geometry representing Voronoj */
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    result = voronoj_export_r (cache, voronoj, result, only_edges);
    voronoj_free (voronoj);

    result->Srid = geom->Srid;
    if (only_edges)
	result->DeclaredType = GAIA_MULTILINESTRING;
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
#endif /* end GEOS_REENTRANT */
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaConcaveHull (gaiaGeomCollPtr geom, double factor, double tolerance,
		 int allow_holes)
{
/* Concave Hull */
    gaiaGeomCollPtr result = NULL;
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr concave_hull;
    gaiaPolygonPtr pg;
    int pgs = 0;
    int errs = 0;
    gaiaResetGeosMsg ();
    if (!geom)
	return NULL;
    g1 = gaiaToGeos (geom);
    g2 = GEOSDelaunayTriangulation (g1, tolerance, 0);
    GEOSGeom_destroy (g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ (g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM (g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM (g2);
    else
	result = gaiaFromGeos_XY (g2);
    GEOSGeom_destroy (g2);
    if (result == NULL)
	return NULL;
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* counting how many triangles are in Delaunay */
	  if (delaunay_triangle_check (pg))
	      pgs++;
	  else
	      errs++;
	  pg = pg->Next;
      }
    if (pgs == 0 || errs)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

/* building the Concave Hull from Delaunay */
    concave_hull =
	concave_hull_build (result->FirstPolygon, geom->DimensionModel, factor,
			    allow_holes);
    gaiaFreeGeomColl (result);
    if (!concave_hull)
	return NULL;
    result = concave_hull;

    result->Srid = geom->Srid;
#else
    if (geom == NULL || factor == 0.0 || tolerance == 0.0 || allow_holes == 0)
	geom = NULL;		/* silencing stupid compiler warnings */
#endif
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaConcaveHull_r (const void *p_cache, gaiaGeomCollPtr geom, double factor,
		   double tolerance, int allow_holes)
{
/* Concave Hull */
    GEOSGeometry *g1;
    GEOSGeometry *g2;
    gaiaGeomCollPtr result;
    gaiaGeomCollPtr concave_hull;
    gaiaPolygonPtr pg;
    int pgs = 0;
    int errs = 0;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    GEOSContextHandle_t handle = NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    handle = cache->GEOS_handle;
    if (handle == NULL)
	return NULL;
    gaiaResetGeosMsg_r (cache);
    if (!geom)
	return NULL;
    g1 = gaiaToGeos_r (cache, geom);
    g2 = GEOSDelaunayTriangulation_r (handle, g1, tolerance, 0);
    GEOSGeom_destroy_r (handle, g1);
    if (!g2)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaFromGeos_XYZ_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaFromGeos_XYM_r (cache, g2);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaFromGeos_XYZM_r (cache, g2);
    else
	result = gaiaFromGeos_XY_r (cache, g2);
    GEOSGeom_destroy_r (handle, g2);
    if (result == NULL)
	return NULL;
    pg = result->FirstPolygon;
    while (pg)
      {
	  /* counting how many triangles are in Delaunay */
	  if (delaunay_triangle_check (pg))
	      pgs++;
	  else
	      errs++;
	  pg = pg->Next;
      }
    if (pgs == 0 || errs)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }

/* building the Concave Hull from Delaunay */
    concave_hull =
	concave_hull_build_r (p_cache, result->FirstPolygon,
			      geom->DimensionModel, factor, allow_holes);
    gaiaFreeGeomColl (result);
    if (!concave_hull)
	return NULL;
    result = concave_hull;

    result->Srid = geom->Srid;
    return result;
}

#endif /* end GEOS advanced features */

#endif /* end including GEOS */
