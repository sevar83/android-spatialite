/*

 gg_rttopo.c -- Gaia RTTOPO support
    
 version 4.5, 2016 April 18

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
 
Portions created by the Initial Developer are Copyright (C) 2012-2015
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

/*
 
CREDITS:

this module (wrapping liblwgeom APIs) has been entierely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale

HISTORY:
this module was previously name gg_lwgeom.c and was based on liblwgeom;
the current version depends on the newer RTTOPO support

*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite_private.h>
#include <spatialite/sqlite.h>
#include <spatialite.h>
#include <spatialite/debug.h>

#include <spatialite/gaiageo.h>

#ifdef ENABLE_RTTOPO		/* enabling RTTOPO support */

#include <librttopo_geom.h>

extern char *rtgeom_to_encoded_polyline (const RTCTX * ctx, const RTGEOM * geom,
					 int precision);
static RTGEOM *rtgeom_from_encoded_polyline (const RTCTX * ctx,
					     const char *encodedpolyline,
					     int precision);

SPATIALITE_PRIVATE const char *
splite_rttopo_version (void)
{
    return rtgeom_version ();
}

GAIAGEO_DECLARE void
gaiaResetRtTopoMsg (const void *p_cache)
{
/* Resets the RTTOPO error and warning messages to an empty state */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    if (cache->gaia_rttopo_error_msg)
	free (cache->gaia_rttopo_error_msg);
    if (cache->gaia_rttopo_warning_msg)
	free (cache->gaia_rttopo_warning_msg);
    cache->gaia_rttopo_error_msg = NULL;
    cache->gaia_rttopo_warning_msg = NULL;
}

GAIAGEO_DECLARE const char *
gaiaGetRtTopoErrorMsg (const void *p_cache)
{
/* Return the latest RTTOPO error message (if any) */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;

    return cache->gaia_rttopo_error_msg;
}

GAIAGEO_DECLARE void
gaiaSetRtTopoErrorMsg (const void *p_cache, const char *msg)
{
/* Sets the RTTOPO error message */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    if (cache->gaia_rttopo_error_msg)
	free (cache->gaia_rttopo_error_msg);
    cache->gaia_rttopo_error_msg = NULL;
    if (msg == NULL)
	return;

    len = strlen (msg);
    cache->gaia_rttopo_error_msg = malloc (len + 1);
    strcpy (cache->gaia_rttopo_error_msg, msg);
}

GAIAGEO_DECLARE const char *
gaiaGetRtTopoWarningMsg (const void *p_cache)
{
/* Return the latest RTTOPO warning message (if any) */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;

    return cache->gaia_rttopo_warning_msg;
}

GAIAGEO_DECLARE void
gaiaSetRtTopoWarningMsg (const void *p_cache, const char *msg)
{
/* Sets the RTTOPO warning message */
    int len;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    if (cache->gaia_rttopo_warning_msg)
	free (cache->gaia_rttopo_warning_msg);
    cache->gaia_rttopo_warning_msg = NULL;
    if (msg == NULL)
	return;

    len = strlen (msg);
    cache->gaia_rttopo_warning_msg = malloc (len + 1);
    strcpy (cache->gaia_rttopo_warning_msg, msg);
}

static int
check_unclosed_ring (gaiaRingPtr rng)
{
/* checks if a Ring is closed or not */
    double x0;
    double y0;
    double z0 = 0.0;
    double m0 = 0.0;
    double x1;
    double y1;
    double z1 = 0.0;
    double m1 = 0.0;
    int last = rng->Points - 1;
    if (rng->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (rng->Coords, 0, &x0, &y0, &z0);
      }
    else if (rng->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (rng->Coords, 0, &x0, &y0, &m0);
      }
    else if (rng->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (rng->Coords, 0, &x0, &y0, &z0, &m0);
      }
    else
      {
	  gaiaGetPoint (rng->Coords, 0, &x0, &y0);
      }
    if (rng->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (rng->Coords, last, &x1, &y1, &z1);
      }
    else if (rng->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (rng->Coords, last, &x1, &y1, &m1);
      }
    else if (rng->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (rng->Coords, last, &x1, &y1, &z1, &m1);
      }
    else
      {
	  gaiaGetPoint (rng->Coords, last, &x1, &y1);
      }
    if (x0 == x1 && y0 == y1 && z0 == z1 && m0 == m1)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE void *
toRTGeom (const void *pctx, const void *pgaia)
{
/* converting a GAIA Geometry into a RTGEOM Geometry */
    const RTCTX *ctx = (const RTCTX *) pctx;
    const gaiaGeomCollPtr gaia = (const gaiaGeomCollPtr) pgaia;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int has_z;
    int has_m;
    int ngeoms;
    int numg;
    int ib;
    int iv;
    int type;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    int close_ring;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    RTPOINTARRAY *pa;
    RTPOINTARRAY **ppaa;
    RTPOINT4D point;
    RTGEOM **geoms;

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
	  /* counting how many LINESTRINGs are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = gaia->FirstPolygon;
    while (pg)
      {
	  /* counting how many POLYGONs are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 0 && pgs == 0)
	return NULL;

    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  /* single Point */
	  pt = gaia->FirstPoint;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  pa = ptarray_construct (ctx, has_z, has_m, 1);
	  point.x = pt->X;
	  point.y = pt->Y;
	  if (has_z)
	      point.z = pt->Z;
	  if (has_m)
	      point.m = pt->M;
	  ptarray_set_point4d (ctx, pa, 0, &point);
	  return (RTGEOM *) rtpoint_construct (ctx, gaia->Srid, NULL, pa);
      }
    else if (pts == 0 && lns == 1 && pgs == 0)
      {
	  /* single Linestring */
	  ln = gaia->FirstLinestring;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  pa = ptarray_construct (ctx, has_z, has_m, ln->Points);
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		/* copying vertices */
		if (gaia->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		  }
		else if (gaia->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		  }
		else if (gaia->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ctx, pa, iv, &point);
	    }
	  return (RTGEOM *) rtline_construct (ctx, gaia->Srid, NULL, pa);
      }
    else if (pts == 0 && lns == 0 && pgs == 1)
      {
	  /* single Polygon */
	  pg = gaia->FirstPolygon;
	  has_z = 0;
	  has_m = 0;
	  if (gaia->DimensionModel == GAIA_XY_Z
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_z = 1;
	  if (gaia->DimensionModel == GAIA_XY_M
	      || gaia->DimensionModel == GAIA_XY_Z_M)
	      has_m = 1;
	  ngeoms = pg->NumInteriors;
	  ppaa = rtalloc (ctx, sizeof (RTPOINTARRAY *) * (ngeoms + 1));
	  rng = pg->Exterior;
	  close_ring = check_unclosed_ring (rng);
	  if (close_ring)
	      ppaa[0] = ptarray_construct (ctx, has_z, has_m, rng->Points + 1);
	  else
	      ppaa[0] = ptarray_construct (ctx, has_z, has_m, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying vertices - Exterior Ring */
		if (gaia->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (gaia->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (gaia->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ctx, ppaa[0], iv, &point);
	    }
	  if (close_ring)
	    {
		/* making an unclosed ring to be closed */
		if (gaia->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
		  }
		else if (gaia->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
		  }
		else if (gaia->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, 0, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ctx, ppaa[0], rng->Points, &point);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		/* copying vertices - Interior Rings */
		rng = pg->Interiors + ib;
		close_ring = check_unclosed_ring (rng);
		if (close_ring)
		    ppaa[1 + ib] =
			ptarray_construct (ctx, has_z, has_m, rng->Points + 1);
		else
		    ppaa[1 + ib] =
			ptarray_construct (ctx, has_z, has_m, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ctx, ppaa[1 + ib], iv, &point);
		  }
		if (close_ring)
		  {
		      /* making an unclosed ring to be closed */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, 0, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ctx, ppaa[1 + ib], rng->Points,
					   &point);
		  }
	    }
	  return (RTGEOM *) rtpoly_construct (ctx, gaia->Srid, NULL, ngeoms + 1,
					      ppaa);
      }
    else
      {
	  /* some Collection */
	  switch (gaia->DeclaredType)
	    {
	    case GAIA_POINT:
		type = RTPOINTTYPE;
		break;
	    case GAIA_LINESTRING:
		type = RTLINETYPE;
		break;
	    case GAIA_POLYGON:
		type = RTPOLYGONTYPE;
		break;
	    case GAIA_MULTIPOINT:
		type = RTMULTIPOINTTYPE;
		break;
	    case GAIA_MULTILINESTRING:
		type = RTMULTILINETYPE;
		break;
	    case GAIA_MULTIPOLYGON:
		type = RTMULTIPOLYGONTYPE;
		break;
	    case GAIA_GEOMETRYCOLLECTION:
		type = RTCOLLECTIONTYPE;
		break;
	    default:
		if (lns == 0 && pgs == 0)
		    type = RTMULTIPOINTTYPE;
		else if (pts == 0 && pgs == 0)
		    type = RTMULTILINETYPE;
		else if (pts == 0 && lns == 0)
		    type = RTMULTIPOLYGONTYPE;
		else
		    type = RTCOLLECTIONTYPE;
		break;
	    };
	  numg = pts + lns + pgs;
	  geoms = rtalloc (ctx, sizeof (RTGEOM *) * numg);

	  numg = 0;
	  pt = gaia->FirstPoint;
	  while (pt)
	    {
		/* copying POINTs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		pa = ptarray_construct (ctx, has_z, has_m, 1);
		point.x = pt->X;
		point.y = pt->Y;
		if (has_z)
		    point.z = pt->Z;
		if (has_m)
		    point.m = pt->M;
		ptarray_set_point4d (ctx, pa, 0, &point);
		geoms[numg++] =
		    (RTGEOM *) rtpoint_construct (ctx, gaia->Srid, NULL, pa);
		pt = pt->Next;
	    }
	  ln = gaia->FirstLinestring;
	  while (ln)
	    {
		/* copying LINESTRINGs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		pa = ptarray_construct (ctx, has_z, has_m, ln->Points);
		for (iv = 0; iv < ln->Points; iv++)
		  {
		      /* copying vertices */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (ln->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ctx, pa, iv, &point);
		  }
		geoms[numg++] =
		    (RTGEOM *) rtline_construct (ctx, gaia->Srid, NULL, pa);
		ln = ln->Next;
	    }
	  pg = gaia->FirstPolygon;
	  while (pg)
	    {
		/* copying POLYGONs */
		has_z = 0;
		has_m = 0;
		if (gaia->DimensionModel == GAIA_XY_Z
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_z = 1;
		if (gaia->DimensionModel == GAIA_XY_M
		    || gaia->DimensionModel == GAIA_XY_Z_M)
		    has_m = 1;
		ngeoms = pg->NumInteriors;
		ppaa = rtalloc (ctx, sizeof (RTPOINTARRAY *) * (ngeoms + 1));
		rng = pg->Exterior;
		close_ring = check_unclosed_ring (rng);
		if (close_ring)
		    ppaa[0] =
			ptarray_construct (ctx, has_z, has_m, rng->Points + 1);
		else
		    ppaa[0] =
			ptarray_construct (ctx, has_z, has_m, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* copying vertices - Exterior Ring */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ctx, ppaa[0], iv, &point);
		  }
		if (close_ring)
		  {
		      /* making an unclosed ring to be closed */
		      if (gaia->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
			}
		      else if (gaia->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
			}
		      else if (gaia->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, 0, &x, &y);
			}
		      point.x = x;
		      point.y = y;
		      if (has_z)
			  point.z = z;
		      if (has_m)
			  point.m = m;
		      ptarray_set_point4d (ctx, ppaa[0], rng->Points, &point);
		  }
		for (ib = 0; ib < pg->NumInteriors; ib++)
		  {
		      /* copying vertices - Interior Rings */
		      rng = pg->Interiors + ib;
		      close_ring = check_unclosed_ring (rng);
		      if (close_ring)
			  ppaa[1 + ib] =
			      ptarray_construct (ctx, has_z, has_m,
						 rng->Points + 1);
		      else
			  ppaa[1 + ib] =
			      ptarray_construct (ctx, has_z, has_m,
						 rng->Points);
		      for (iv = 0; iv < rng->Points; iv++)
			{
			    if (gaia->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (rng->Coords, iv, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (rng->Coords, iv, &x, &y);
			      }
			    point.x = x;
			    point.y = y;
			    if (has_z)
				point.z = z;
			    if (has_m)
				point.m = m;
			    ptarray_set_point4d (ctx, ppaa[1 + ib], iv, &point);
			}
		      if (close_ring)
			{
			    /* making an unclosed ring to be closed */
			    if (gaia->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
			      }
			    else if (gaia->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (rng->Coords, 0, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (rng->Coords, 0, &x, &y);
			      }
			    point.x = x;
			    point.y = y;
			    if (has_z)
				point.z = z;
			    if (has_m)
				point.m = m;
			    ptarray_set_point4d (ctx, ppaa[1 + ib], rng->Points,
						 &point);
			}
		  }
		geoms[numg++] =
		    (RTGEOM *) rtpoly_construct (ctx, gaia->Srid, NULL,
						 ngeoms + 1, ppaa);
		pg = pg->Next;
	    }
	  return (RTGEOM *) rtcollection_construct (ctx, type, gaia->Srid, NULL,
						    numg, geoms);
      }
    return NULL;
}

static gaiaGeomCollPtr
fromRTGeomIncremental (const RTCTX * ctx, gaiaGeomCollPtr gaia,
		       const RTGEOM * rtgeom)
{
/* converting a RTGEOM Geometry into a GAIA Geometry */
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dimension_model = gaia->DimensionModel;
    int declared_type = gaia->DeclaredType;
    RTGEOM *rtg2 = NULL;
    RTPOINT *rtp = NULL;
    RTLINE *rtl = NULL;
    RTPOLY *rtpoly = NULL;
    RTCOLLECTION *rtc = NULL;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int has_z;
    int has_m;
    int iv;
    int ib;
    int ngeoms;
    int ng;
    double x;
    double y;
    double z;
    double m;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    switch (rtgeom->type)
      {
      case RTPOINTTYPE:
	  rtp = (RTPOINT *) rtgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = rtp->point;
	  if (RTFLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (RTFLAGS_GET_M (pa->flags))
	      has_m = 1;
	  rt_getPoint4d_p (ctx, pa, 0, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  else
	      z = 0.0;
	  if (has_m)
	      m = pt4d.m;
	  else
	      m = 0.0;
	  if (dimension_model == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
	  else if (dimension_model == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (gaia, x, y, m);
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (gaia, x, y, z, m);
	  else
	      gaiaAddPointToGeomColl (gaia, x, y);
	  if (declared_type == GAIA_MULTIPOINT)
	      gaia->DeclaredType = GAIA_MULTIPOINT;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_POINT;
	  break;
      case RTLINETYPE:
	  rtl = (RTLINE *) rtgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = rtl->points;
	  if (RTFLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (RTFLAGS_GET_M (pa->flags))
	      has_m = 1;
	  ln = gaiaAddLinestringToGeomColl (gaia, pa->npoints);
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying LINESTRING vertices */
		rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (has_m)
		    m = pt4d.m;
		else
		    m = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ln->Coords, iv, x, y, m);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ln->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (ln->Coords, iv, x, y);
		  }
	    }
	  if (declared_type == GAIA_MULTILINESTRING)
	      gaia->DeclaredType = GAIA_MULTILINESTRING;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_LINESTRING;
	  break;
      case RTPOLYGONTYPE:
	  rtpoly = (RTPOLY *) rtgeom;
	  has_z = 0;
	  has_m = 0;
	  pa = rtpoly->rings[0];
	  if (RTFLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  if (RTFLAGS_GET_M (pa->flags))
	      has_m = 1;
	  pg = gaiaAddPolygonToGeomColl (gaia, pa->npoints, rtpoly->nrings - 1);
	  rng = pg->Exterior;
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying Exterion Ring vertices */
		rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (has_m)
		    m = pt4d.m;
		else
		    m = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (rng->Coords, iv, x, y, m);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (rng->Coords, iv, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
	  for (ib = 1; ib < rtpoly->nrings; ib++)
	    {
		has_z = 0;
		has_m = 0;
		pa = rtpoly->rings[ib];
		if (RTFLAGS_GET_Z (pa->flags))
		    has_z = 1;
		if (RTFLAGS_GET_M (pa->flags))
		    has_m = 1;
		rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
		for (iv = 0; iv < pa->npoints; iv++)
		  {
		      /* copying Exterion Ring vertices */
		      rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		      x = pt4d.x;
		      y = pt4d.y;
		      if (has_z)
			  z = pt4d.z;
		      else
			  z = 0.0;
		      if (has_m)
			  m = pt4d.m;
		      else
			  m = 0.0;
		      if (dimension_model == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			}
		      else if (dimension_model == GAIA_XY_M)
			{
			    gaiaSetPointXYM (rng->Coords, iv, x, y, m);
			}
		      else if (dimension_model == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (rng->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (rng->Coords, iv, x, y);
			}
		  }
	    }
	  if (declared_type == GAIA_MULTIPOLYGON)
	      gaia->DeclaredType = GAIA_MULTIPOLYGON;
	  else if (declared_type == GAIA_GEOMETRYCOLLECTION)
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  else
	      gaia->DeclaredType = GAIA_POLYGON;
	  break;
      case RTMULTIPOINTTYPE:
      case RTMULTILINETYPE:
      case RTMULTIPOLYGONTYPE:
      case RTCOLLECTIONTYPE:
	  if (rtgeom->type == RTMULTIPOINTTYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTIPOINT;
	    }
	  else if (rtgeom->type == RTMULTILINETYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTILINESTRING;
	    }
	  else if (rtgeom->type == RTMULTIPOLYGONTYPE)
	    {
		if (declared_type == GAIA_GEOMETRYCOLLECTION)
		    gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
		else
		    gaia->DeclaredType = GAIA_MULTIPOLYGON;
	    }
	  else
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;

	  rtc = (RTCOLLECTION *) rtgeom;
	  ngeoms = rtc->ngeoms;
	  if (ngeoms == 0)
	    {
		gaiaFreeGeomColl (gaia);
		gaia = NULL;
		break;
	    }
	  for (ng = 0; ng < ngeoms; ++ng)
	    {
		/* looping on elementary geometries */
		rtg2 = rtc->geoms[ng];
		switch (rtg2->type)
		  {
		  case RTPOINTTYPE:
		      rtp = (RTPOINT *) rtg2;
		      has_z = 0;
		      has_m = 0;
		      pa = rtp->point;
		      if (RTFLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (RTFLAGS_GET_M (pa->flags))
			  has_m = 1;
		      rt_getPoint4d_p (ctx, pa, 0, &pt4d);
		      x = pt4d.x;
		      y = pt4d.y;
		      if (has_z)
			  z = pt4d.z;
		      else
			  z = 0.0;
		      if (has_m)
			  m = pt4d.m;
		      else
			  m = 0.0;
		      if (dimension_model == GAIA_XY_Z)
			  gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
		      else if (dimension_model == GAIA_XY_M)
			  gaiaAddPointToGeomCollXYM (gaia, x, y, m);
		      else if (dimension_model == GAIA_XY_Z_M)
			  gaiaAddPointToGeomCollXYZM (gaia, x, y, z, m);
		      else
			  gaiaAddPointToGeomColl (gaia, x, y);
		      break;
		  case RTLINETYPE:
		      rtl = (RTLINE *) rtg2;
		      has_z = 0;
		      has_m = 0;
		      pa = rtl->points;
		      if (RTFLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (RTFLAGS_GET_M (pa->flags))
			  has_m = 1;
		      ln = gaiaAddLinestringToGeomColl (gaia, pa->npoints);
		      for (iv = 0; iv < pa->npoints; iv++)
			{
			    /* copying LINESTRING vertices */
			    rt_getPoint4d_p (ctx, pa, iv, &pt4d);
			    x = pt4d.x;
			    y = pt4d.y;
			    if (has_z)
				z = pt4d.z;
			    else
				z = 0.0;
			    if (has_m)
				m = pt4d.m;
			    else
				m = 0.0;
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (ln->Coords, iv, x, y, m);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (ln->Coords, iv, x, y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (ln->Coords, iv, x, y);
			      }
			}
		      break;
		  case RTPOLYGONTYPE:
		      rtpoly = (RTPOLY *) rtg2;
		      has_z = 0;
		      has_m = 0;
		      pa = rtpoly->rings[0];
		      if (RTFLAGS_GET_Z (pa->flags))
			  has_z = 1;
		      if (RTFLAGS_GET_M (pa->flags))
			  has_m = 1;
		      pg = gaiaAddPolygonToGeomColl (gaia, pa->npoints,
						     rtpoly->nrings - 1);
		      rng = pg->Exterior;
		      for (iv = 0; iv < pa->npoints; iv++)
			{
			    /* copying Exterion Ring vertices */
			    rt_getPoint4d_p (ctx, pa, iv, &pt4d);
			    x = pt4d.x;
			    y = pt4d.y;
			    if (has_z)
				z = pt4d.z;
			    else
				z = 0.0;
			    if (has_m)
				m = pt4d.m;
			    else
				m = 0.0;
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (rng->Coords, iv, x, y, m);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (rng->Coords, iv, x, y, z,
						    m);
			      }
			    else
			      {
				  gaiaSetPoint (rng->Coords, iv, x, y);
			      }
			}
		      for (ib = 1; ib < rtpoly->nrings; ib++)
			{
			    has_z = 0;
			    has_m = 0;
			    pa = rtpoly->rings[ib];
			    if (RTFLAGS_GET_Z (pa->flags))
				has_z = 1;
			    if (RTFLAGS_GET_M (pa->flags))
				has_m = 1;
			    rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
			    for (iv = 0; iv < pa->npoints; iv++)
			      {
				  /* copying Exterion Ring vertices */
				  rt_getPoint4d_p (ctx, pa, iv, &pt4d);
				  x = pt4d.x;
				  y = pt4d.y;
				  if (has_z)
				      z = pt4d.z;
				  else
				      z = 0.0;
				  if (has_m)
				      m = pt4d.m;
				  else
				      m = 0.0;
				  if (dimension_model == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (rng->Coords, iv, x,
							 y, z);
				    }
				  else if (dimension_model == GAIA_XY_M)
				    {
					gaiaSetPointXYM (rng->Coords, iv, x,
							 y, m);
				    }
				  else if (dimension_model == GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (rng->Coords, iv, x,
							  y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (rng->Coords, iv, x, y);
				    }
			      }
			}
		      break;
		  };
	    }
	  break;
      default:
	  gaiaFreeGeomColl (gaia);
	  gaia = NULL;
	  break;
      };

    return gaia;
}

SPATIALITE_PRIVATE void *
fromRTGeom (const void *pctx, const void *prtgeom, const int dimension_model,
	    const int declared_type)
{
/* converting a RTGEOM Geometry into a GAIA Geometry */
    gaiaGeomCollPtr gaia = NULL;
    const RTCTX *ctx = (const RTCTX *) pctx;
    const RTGEOM *rtgeom = (const RTGEOM *) prtgeom;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    if (dimension_model == GAIA_XY_Z)
	gaia = gaiaAllocGeomCollXYZ ();
    else if (dimension_model == GAIA_XY_M)
	gaia = gaiaAllocGeomCollXYM ();
    else if (dimension_model == GAIA_XY_Z_M)
	gaia = gaiaAllocGeomCollXYZM ();
    else
	gaia = gaiaAllocGeomColl ();
    gaia->DeclaredType = declared_type;
    fromRTGeomIncremental (ctx, gaia, rtgeom);

    return gaia;
}

static int
check_valid_type (const RTGEOM * rtgeom, int declared_type)
{
/* checking if the geometry type is a valid one */
    int ret = 0;
    switch (rtgeom->type)
      {
      case RTPOINTTYPE:
      case RTMULTIPOINTTYPE:
	  if (declared_type == GAIA_POINT || declared_type == GAIA_POINTZ
	      || declared_type == GAIA_POINTM || declared_type == GAIA_POINTZM)
	      ret = 1;
	  if (declared_type == GAIA_MULTIPOINT
	      || declared_type == GAIA_MULTIPOINTZ
	      || declared_type == GAIA_MULTIPOINTM
	      || declared_type == GAIA_MULTIPOINTZM)
	      ret = 1;
	  break;
      case RTLINETYPE:
      case RTMULTILINETYPE:
	  if (declared_type == GAIA_LINESTRING
	      || declared_type == GAIA_LINESTRINGZ
	      || declared_type == GAIA_LINESTRINGM
	      || declared_type == GAIA_LINESTRINGZM)
	      ret = 1;
	  if (declared_type == GAIA_MULTILINESTRING
	      || declared_type == GAIA_MULTILINESTRINGZ
	      || declared_type == GAIA_MULTILINESTRINGM
	      || declared_type == GAIA_MULTILINESTRINGZM)
	      ret = 1;
	  break;
      case RTPOLYGONTYPE:
      case RTMULTIPOLYGONTYPE:
	  if (declared_type == GAIA_POLYGON || declared_type == GAIA_POLYGONZ
	      || declared_type == GAIA_POLYGONM
	      || declared_type == GAIA_POLYGONZM)
	      ret = 1;
	  if (declared_type == GAIA_MULTIPOLYGON
	      || declared_type == GAIA_MULTIPOLYGONZ
	      || declared_type == GAIA_MULTIPOLYGONM
	      || declared_type == GAIA_MULTIPOLYGONZM)
	      ret = 1;
	  break;
      case RTCOLLECTIONTYPE:
	  if (declared_type == GAIA_GEOMETRYCOLLECTION
	      || declared_type == GAIA_GEOMETRYCOLLECTIONZ
	      || declared_type == GAIA_GEOMETRYCOLLECTIONM
	      || declared_type == GAIA_GEOMETRYCOLLECTIONZM)
	      ret = 1;
	  break;
      };
    return ret;
}

static gaiaGeomCollPtr
fromRTGeomValidated (const RTCTX * ctx, const RTGEOM * rtgeom,
		     const int dimension_model, const int declared_type)
{
/* 
/ converting a RTGEOM Geometry into a GAIA Geometry 
/ first collection - validated items
*/
    gaiaGeomCollPtr gaia = NULL;
    RTGEOM *rtg2 = NULL;
    RTCOLLECTION *rtc = NULL;
    int ngeoms;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    switch (rtgeom->type)
      {
      case RTCOLLECTIONTYPE:
	  rtc = (RTCOLLECTION *) rtgeom;
	  ngeoms = rtc->ngeoms;
	  if (ngeoms <= 2)
	    {
		rtg2 = rtc->geoms[0];
		if (check_valid_type (rtg2, declared_type))
		    gaia =
			fromRTGeom (ctx, rtg2, dimension_model, declared_type);
	    }
	  break;
      default:
	  if (check_valid_type (rtgeom, declared_type))
	      gaia = fromRTGeom (ctx, rtgeom, dimension_model, declared_type);
	  if (gaia == NULL)
	    {
		/* Andrea Peri: 2013-05-02 returning anyway the RTGEOM geometry,
		   / even if it has a mismatching type */
		int type = -1;
		switch (rtgeom->type)
		  {
		  case RTPOINTTYPE:
		      type = GAIA_POINT;
		      break;
		  case RTLINETYPE:
		      type = GAIA_LINESTRING;
		      break;
		  case RTPOLYGONTYPE:
		      type = GAIA_POLYGON;
		      break;
		  case RTMULTIPOINTTYPE:
		      type = GAIA_MULTIPOINT;
		      break;
		  case RTMULTILINETYPE:
		      type = GAIA_MULTILINESTRING;
		      break;
		  case RTMULTIPOLYGONTYPE:
		      type = GAIA_MULTIPOLYGON;
		      break;
		  };
		if (type >= 0)
		    gaia = fromRTGeom (ctx, rtgeom, dimension_model, type);
	    }
	  break;
      }
    return gaia;
}

static gaiaGeomCollPtr
fromRTGeomDiscarded (const RTCTX * ctx, const RTGEOM * rtgeom,
		     const int dimension_model, const int declared_type)
{
/* 
/ converting a RTGEOM Geometry into a GAIA Geometry 
/ second collection - discarded items
*/
    gaiaGeomCollPtr gaia = NULL;
    RTGEOM *rtg2 = NULL;
    RTCOLLECTION *rtc = NULL;
    int ngeoms;
    int ig;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    if (rtgeom->type == RTCOLLECTIONTYPE)
      {
	  if (dimension_model == GAIA_XY_Z)
	      gaia = gaiaAllocGeomCollXYZ ();
	  else if (dimension_model == GAIA_XY_M)
	      gaia = gaiaAllocGeomCollXYM ();
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaia = gaiaAllocGeomCollXYZM ();
	  else
	      gaia = gaiaAllocGeomColl ();
	  rtc = (RTCOLLECTION *) rtgeom;
	  ngeoms = rtc->ngeoms;
	  for (ig = 0; ig < ngeoms; ig++)
	    {
		rtg2 = rtc->geoms[ig];
		if (!check_valid_type (rtg2, declared_type))
		    fromRTGeomIncremental (ctx, gaia, rtg2);
	    }
      }
/*
Andrea Peri: 2013-05-02
when a single geometry is returned by RTGEOM it's always "valid"
and there are no discarded items at all

    else if (!check_valid_type (lwgeom, declared_type))
	gaia = fromRTGeom (lwgeom, dimension_model, declared_type);
*/
    return gaia;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeValid (const void *p_cache, gaiaGeomCollPtr geom)
{
/* wrapping RTGEOM MakeValid [collecting valid items] */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    gaiaGeomCollPtr result = NULL;

    if (!geom)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g1 = toRTGeom (ctx, geom);
    g2 = rtgeom_make_valid (ctx, g1);
    if (!g2)
      {
	  rtgeom_free (ctx, g1);
	  goto done;
      }
    result =
	fromRTGeomValidated (ctx, g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    result->Srid = geom->Srid;

  done:
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaMakeValidDiscarded (const void *p_cache, gaiaGeomCollPtr geom)
{
/* wrapping RTGEOM MakeValid [collecting discarder items] */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    gaiaGeomCollPtr result = NULL;

    if (!geom)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g1 = toRTGeom (ctx, geom);
    g2 = rtgeom_make_valid (ctx, g1);
    if (!g2)
      {
	  rtgeom_free (ctx, g1);
	  goto done;
      }
    result =
	fromRTGeomDiscarded (ctx, g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    result->Srid = geom->Srid;

  done:
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSegmentize (const void *p_cache, gaiaGeomCollPtr geom, double dist)
{
/* wrapping RTGEOM Segmentize */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    gaiaGeomCollPtr result = NULL;

    if (!geom)
	return NULL;
    if (dist <= 0.0)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g1 = toRTGeom (ctx, geom);
    g2 = rtgeom_segmentize2d (ctx, g1, dist);
    if (!g2)
      {
	  rtgeom_free (ctx, g1);
	  goto done;
      }
    result = fromRTGeom (ctx, g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    result->Srid = geom->Srid;

  done:
    return result;
}

static int
check_split_args (gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* testing Split arguments */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int i_lns = 0;
    int i_pgs = 0;
    int b_pts = 0;
    int b_lns = 0;

    if (!input)
	return 0;
    if (!blade)
	return 0;

/* testing the Input type */
    if (input->FirstPoint != NULL)
      {
	  /* Point(s) on Input is forbidden !!!! */
	  return 0;
      }
    ln = input->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  i_lns++;
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* counting how many Polygons are there */
	  i_pgs++;
	  pg = pg->Next;
      }
    if (i_lns + i_pgs == 0)
      {
	  /* empty Input */
	  return 0;
      }

/* testing the Blade type */
    pt = blade->FirstPoint;
    while (pt)
      {
	  /* counting how many Points are there */
	  b_pts++;
	  pt = pt->Next;
      }
    ln = blade->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  b_lns++;
	  ln = ln->Next;
      }
    if (blade->FirstPolygon != NULL)
      {
	  /* Polygon(s) on Blade is forbidden !!!! */
	  return 0;
      }
    if (b_pts + b_lns == 0)
      {
	  /* empty Blade */
	  return 0;
      }
    if (b_pts >= 1 && b_lns >= 1)
      {
	  /* invalid Blade [point + linestring] */
	  return 0;
      }

/* compatibility check */
    if (b_lns >= 1)
      {
	  /* Linestring blade is always valid */
	  return 1;
      }
    if (i_lns >= 1 && b_pts >= 1)
      {
	  /* Linestring or MultiLinestring input and Point blade is allowed */
	  return 1;
      }

    return 0;
}

static void
set_split_gtype (gaiaGeomCollPtr geom)
{
/* assignign the actual geometry type */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;

    pt = geom->FirstPoint;
    while (pt)
      {
	  /* counting how many Points are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  /* counting how many Linestrings are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* counting how many Polygons are there */
	  pgs++;
	  pg = pg->Next;
      }

    if (pts == 1 && lns == 0 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_POINT;
	  return;
      }
    if (pts > 1 && lns == 0 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_MULTIPOINT;
	  return;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_LINESTRING;
	  return;
      }
    if (pts == 0 && lns > 1 && pgs == 0)
      {
	  geom->DeclaredType = GAIA_MULTILINESTRING;
	  return;
      }
    if (pts == 0 && lns == 0 && pgs == 1)
      {
	  geom->DeclaredType = GAIA_POLYGON;
	  return;
      }
    if (pts == 0 && lns == 0 && pgs > 1)
      {
	  geom->DeclaredType = GAIA_MULTIPOLYGON;
	  return;
      }
    geom->DeclaredType = GAIA_GEOMETRYCOLLECTION;
}

static RTGEOM *
toRTGeomLinestring (const RTCTX * ctx, gaiaLinestringPtr ln, int srid)
{
/* converting a GAIA Linestring into a RTGEOM Geometry */
    int iv;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    int has_z = 0;
    int has_m = 0;
    RTPOINTARRAY *pa;
    RTPOINT4D point;

    if (ln->DimensionModel == GAIA_XY_Z || ln->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (ln->DimensionModel == GAIA_XY_M || ln->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    pa = ptarray_construct (ctx, has_z, has_m, ln->Points);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* copying vertices */
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
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  if (has_m)
	      point.m = m;
	  ptarray_set_point4d (ctx, pa, iv, &point);
      }
    return (RTGEOM *) rtline_construct (ctx, srid, NULL, pa);
}

static RTGEOM *
toRTGeomPolygon (const RTCTX * ctx, gaiaPolygonPtr pg, int srid)
{
/* converting a GAIA Linestring into a RTGEOM Geometry */
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    int ngeoms;
    int has_z = 0;
    int has_m = 0;
    int close_ring;
    gaiaRingPtr rng;
    RTPOINTARRAY **ppaa;
    RTPOINT4D point;

    if (pg->DimensionModel == GAIA_XY_Z || pg->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    if (pg->DimensionModel == GAIA_XY_M || pg->DimensionModel == GAIA_XY_Z_M)
	has_m = 1;
    ngeoms = pg->NumInteriors;
    ppaa = rtalloc (ctx, sizeof (RTPOINTARRAY *) * (ngeoms + 1));
    rng = pg->Exterior;
    close_ring = check_unclosed_ring (rng);
    if (close_ring)
	ppaa[0] = ptarray_construct (ctx, has_z, has_m, rng->Points + 1);
    else
	ppaa[0] = ptarray_construct (ctx, has_z, has_m, rng->Points);
    for (iv = 0; iv < rng->Points; iv++)
      {
	  /* copying vertices - Exterior Ring */
	  if (pg->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
	    }
	  else if (pg->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
	    }
	  else if (pg->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  if (has_m)
	      point.m = m;
	  ptarray_set_point4d (ctx, ppaa[0], iv, &point);
      }
    if (close_ring)
      {
	  /* making an unclosed ring to be closed */
	  if (pg->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
	    }
	  else if (pg->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
	    }
	  else if (pg->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, 0, &x, &y);
	    }
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  if (has_m)
	      point.m = m;
	  ptarray_set_point4d (ctx, ppaa[0], rng->Points, &point);
      }
    for (ib = 0; ib < pg->NumInteriors; ib++)
      {
	  /* copying vertices - Interior Rings */
	  rng = pg->Interiors + ib;
	  close_ring = check_unclosed_ring (rng);
	  if (close_ring)
	      ppaa[1 + ib] =
		  ptarray_construct (ctx, has_z, has_m, rng->Points + 1);
	  else
	      ppaa[1 + ib] = ptarray_construct (ctx, has_z, has_m, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		if (pg->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (pg->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (pg->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ctx, ppaa[1 + ib], iv, &point);
	    }
	  if (close_ring)
	    {
		/* making an unclosed ring to be closed */
		if (pg->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, 0, &x, &y, &z);
		  }
		else if (pg->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, 0, &x, &y, &m);
		  }
		else if (pg->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, 0, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, 0, &x, &y);
		  }
		point.x = x;
		point.y = y;
		if (has_z)
		    point.z = z;
		if (has_m)
		    point.m = m;
		ptarray_set_point4d (ctx, ppaa[0], rng->Points, &point);
	    }
      }
    return (RTGEOM *) rtpoly_construct (ctx, srid, NULL, ngeoms + 1, ppaa);
}

static gaiaGeomCollPtr
fromRTGeomLeft (const RTCTX * ctx, gaiaGeomCollPtr gaia, const RTGEOM * rtgeom)
{
/* 
/ converting a RTGEOM Geometry into a GAIA Geometry 
/ collecting "left side" items
*/
    RTGEOM *rtg2 = NULL;
    RTCOLLECTION *rtc = NULL;
    int ngeoms;
    int ig;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    if (rtgeom->type == RTCOLLECTIONTYPE)
      {
	  rtc = (RTCOLLECTION *) rtgeom;
	  ngeoms = rtc->ngeoms;
	  for (ig = 0; ig < ngeoms; ig += 2)
	    {
		rtg2 = rtc->geoms[ig];
		fromRTGeomIncremental (ctx, gaia, rtg2);
	    }
      }
    else
	gaia =
	    fromRTGeom (ctx, rtgeom, gaia->DimensionModel, gaia->DeclaredType);

    return gaia;
}

static gaiaGeomCollPtr
fromRTGeomRight (const RTCTX * ctx, gaiaGeomCollPtr gaia, const RTGEOM * rtgeom)
{
/* 
/ converting a RTGEOM Geometry into a GAIA Geometry 
/ collecting "right side" items
*/
    RTGEOM *rtg2 = NULL;
    RTCOLLECTION *rtc = NULL;
    int ngeoms;
    int ig;

    if (rtgeom == NULL)
	return NULL;
    if (rtgeom_is_empty (ctx, rtgeom))
	return NULL;

    if (rtgeom->type == RTCOLLECTIONTYPE)
      {
	  rtc = (RTCOLLECTION *) rtgeom;
	  ngeoms = rtc->ngeoms;
	  for (ig = 1; ig < ngeoms; ig += 2)
	    {
		rtg2 = rtc->geoms[ig];
		fromRTGeomIncremental (ctx, gaia, rtg2);
	    }
      }

    return gaia;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplit (const void *p_cache, gaiaGeomCollPtr input, gaiaGeomCollPtr blade)
{
/* wrapping RTGEOM Split */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    RTGEOM *g3;
    gaiaGeomCollPtr result = NULL;

    if (!check_split_args (input, blade))
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g1 = toRTGeom (ctx, input);
    g2 = toRTGeom (ctx, blade);
    g3 = rtgeom_split (ctx, g1, g2);
    if (!g3)
      {
	  rtgeom_free (ctx, g1);
	  rtgeom_free (ctx, g2);
	  goto done;
      }
    result = fromRTGeom (ctx, g3, input->DimensionModel, input->DeclaredType);
    spatialite_init_geos ();
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    rtgeom_free (ctx, g3);
    if (result == NULL)
	goto done;
    result->Srid = input->Srid;
    set_split_gtype (result);

  done:
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplitLeft (const void *p_cache, gaiaGeomCollPtr input,
	       gaiaGeomCollPtr blade)
{
/* wrapping RTGEOM Split [left half] */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    RTGEOM *g3;
    gaiaGeomCollPtr result = NULL;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;

    if (!check_split_args (input, blade))
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (input->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (input->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (input->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();

    g2 = toRTGeom (ctx, blade);

    ln = input->FirstLinestring;
    while (ln)
      {
	  /* splitting some Linestring */
	  g1 = toRTGeomLinestring (ctx, ln, input->Srid);
	  g3 = rtgeom_split (ctx, g1, g2);
	  if (g3)
	    {
		result = fromRTGeomLeft (ctx, result, g3);
		rtgeom_free (ctx, g3);
	    }
	  spatialite_init_geos ();
	  rtgeom_free (ctx, g1);
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* splitting some Polygon */
	  g1 = toRTGeomPolygon (ctx, pg, input->Srid);
	  g3 = rtgeom_split (ctx, g1, g2);
	  if (g3)
	    {
		result = fromRTGeomLeft (ctx, result, g3);
		rtgeom_free (ctx, g3);
	    }
	  spatialite_init_geos ();
	  rtgeom_free (ctx, g1);
	  pg = pg->Next;
      }

    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
      {
	  gaiaFreeGeomColl (result);
	  result = NULL;
	  goto done;
      }
    result->Srid = input->Srid;
    set_split_gtype (result);

  done:
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSplitRight (const void *p_cache, gaiaGeomCollPtr input,
		gaiaGeomCollPtr blade)
{
/* wrapping RTGEOM Split [right half] */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    RTGEOM *g3;
    gaiaGeomCollPtr result = NULL;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;

    if (!check_split_args (input, blade))
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    if (input->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (input->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (input->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();

    g2 = toRTGeom (ctx, blade);

    ln = input->FirstLinestring;
    while (ln)
      {
	  /* splitting some Linestring */
	  g1 = toRTGeomLinestring (ctx, ln, input->Srid);
	  g3 = rtgeom_split (ctx, g1, g2);
	  if (g3)
	    {
		result = fromRTGeomRight (ctx, result, g3);
		rtgeom_free (ctx, g3);
	    }
	  spatialite_init_geos ();
	  rtgeom_free (ctx, g1);
	  ln = ln->Next;
      }
    pg = input->FirstPolygon;
    while (pg)
      {
	  /* splitting some Polygon */
	  g1 = toRTGeomPolygon (ctx, pg, input->Srid);
	  g3 = rtgeom_split (ctx, g1, g2);
	  if (g3)
	    {
		result = fromRTGeomRight (ctx, result, g3);
		rtgeom_free (ctx, g3);
	    }
	  spatialite_init_geos ();
	  rtgeom_free (ctx, g1);
	  pg = pg->Next;
      }

    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
      {
	  gaiaFreeGeomColl (result);
	  result = NULL;
	  goto done;
      }
    result->Srid = input->Srid;
    set_split_gtype (result);

  done:
    return result;
}

GAIAGEO_DECLARE int
gaiaAzimuth (const void *p_cache, double xa, double ya, double xb, double yb,
	     double *azimuth)
{
/* wrapping RTGEOM Azimuth */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTPOINT2D pt1;
    RTPOINT2D pt2;
    double az;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    pt1.x = xa;
    pt1.y = ya;
    pt2.x = xb;
    pt2.y = yb;

    if (!azimuth_pt_pt (ctx, &pt1, &pt2, &az))
	ret = 0;
    *azimuth = az;

    return ret;
}

GAIAGEO_DECLARE int
gaiaEllipsoidAzimuth (const void *p_cache, double xa, double ya, double xb,
		      double yb, double a, double b, double *azimuth)
{
/* wrapping RTGEOM AzimuthSpheroid */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTPOINT *pt1;
    RTPOINT *pt2;
    SPHEROID ellips;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    pt1 = rtpoint_make2d (ctx, 0, xa, ya);
    pt2 = rtpoint_make2d (ctx, 0, xb, yb);
    spheroid_init (ctx, &ellips, a, b);
    *azimuth = rtgeom_azumith_spheroid (ctx, pt1, pt2, &ellips);
    rtpoint_free (ctx, pt1);
    rtpoint_free (ctx, pt2);

    return ret;
}

GAIAGEO_DECLARE int
gaiaProjectedPoint (const void *p_cache, double x1, double y1, double a,
		    double b, double distance, double azimuth, double *x2,
		    double *y2)
{
/* wrapping RTGEOM Project */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTPOINT *pt1;
    RTPOINT *pt2;
    SPHEROID ellips;
    int ret = 0;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    pt1 = rtpoint_make2d (ctx, 0, x1, y1);
    spheroid_init (ctx, &ellips, a, b);
    pt2 = rtgeom_project_spheroid (ctx, pt1, &ellips, distance, azimuth);
    rtpoint_free (ctx, pt1);
    if (pt2 != NULL)
      {
	  *x2 = rtpoint_get_x (ctx, pt2);
	  *y2 = rtpoint_get_y (ctx, pt2);
	  rtpoint_free (ctx, pt2);
	  ret = 1;
      }

    return ret;
}

GAIAGEO_DECLARE int
gaiaGeodesicArea (const void *p_cache, gaiaGeomCollPtr geom, double a, double b,
		  int use_ellipsoid, double *area)
{
/* wrapping RTGEOM AreaSphere and AreaSpheroid */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    SPHEROID ellips;
    RTGBOX gbox;
    double tolerance = 1e-12;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    g = toRTGeom (ctx, geom);
    spheroid_init (ctx, &ellips, a, b);
    if (g == NULL)
      {
	  ret = 0;
	  goto done;
      }
    rtgeom_calculate_gbox_geodetic (ctx, g, &gbox);
    if (use_ellipsoid)
      {
	  /* testing for "forbidden" calculations on the ellipsoid */
	  if ((gbox.zmax + tolerance) >= 1.0 || (gbox.zmin - tolerance) <= -1.0)
	      use_ellipsoid = 0;	/* can't circle the poles */
	  if (gbox.zmax > 0.0 && gbox.zmin < 0.0)
	      use_ellipsoid = 0;	/* can't cross the equator */
      }
    if (use_ellipsoid)
	*area = rtgeom_area_spheroid (ctx, g, &ellips);
    else
	*area = rtgeom_area_sphere (ctx, g, &ellips);
    rtgeom_free (ctx, g);

  done:
    return ret;
}

GAIAGEO_DECLARE char *
gaiaGeoHash (const void *p_cache, gaiaGeomCollPtr geom, int precision)
{
/* wrapping RTGEOM GeoHash */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    char *result;
    char *geo_hash = NULL;
    int len;

    if (!geom)
	return NULL;
    gaiaMbrGeometry (geom);
    if (geom->MinX < -180.0 || geom->MaxX > 180.0 || geom->MinY < -90.0
	|| geom->MaxY > 90.0)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g = toRTGeom (ctx, geom);
    result = rtgeom_geohash (ctx, g, precision);
    rtgeom_free (ctx, g);
    if (result == NULL)
	goto done;
    len = strlen (result);
    if (len == 0)
      {
	  rtfree (ctx, result);
	  goto done;
      }
    geo_hash = malloc (len + 1);
    strcpy (geo_hash, result);
    rtfree (ctx, result);

  done:
    return geo_hash;
}

GAIAGEO_DECLARE char *
gaiaAsX3D (const void *p_cache, gaiaGeomCollPtr geom, const char *srs,
	   int precision, int options, const char *defid)
{
/* wrapping RTGEOM AsX3D */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    char *result;
    char *x3d = NULL;
    int len;

    if (!geom)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    gaiaMbrGeometry (geom);
    g = toRTGeom (ctx, geom);
    result = rtgeom_to_x3d3 (ctx, g, (char *) srs, precision, options, defid);
    rtgeom_free (ctx, g);
    if (result == NULL)
	goto done;
    len = strlen (result);
    if (len == 0)
      {
	  rtfree (ctx, result);
	  goto done;
      }
    x3d = malloc (len + 1);
    strcpy (x3d, result);
    rtfree (ctx, result);

  done:
    return x3d;
}

GAIAGEO_DECLARE int
gaia3DDistance (const void *p_cache, gaiaGeomCollPtr geom1,
		gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping RTGEOM mindistance3d */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    double d;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    g1 = toRTGeom (ctx, geom1);
    g2 = toRTGeom (ctx, geom2);

    d = rtgeom_mindistance3d (ctx, g1, g2);
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    *dist = d;

    return ret;
}

GAIAGEO_DECLARE int
gaiaMaxDistance (const void *p_cache, gaiaGeomCollPtr geom1,
		 gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping RTGEOM maxdistance2d */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    double d;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    g1 = toRTGeom (ctx, geom1);
    g2 = toRTGeom (ctx, geom2);

    d = rtgeom_maxdistance2d (ctx, g1, g2);
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    *dist = d;

    return ret;
}

GAIAGEO_DECLARE int
gaia3DMaxDistance (const void *p_cache, gaiaGeomCollPtr geom1,
		   gaiaGeomCollPtr geom2, double *dist)
{
/* wrapping RTGEOM maxdistance2d */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    double d;
    int ret = 1;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    g1 = toRTGeom (ctx, geom1);
    g2 = toRTGeom (ctx, geom2);

    d = rtgeom_maxdistance3d (ctx, g1, g2);
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    *dist = d;

    return ret;
}

static RTLINE *
linestring2rtline (const RTCTX * ctx, gaiaLinestringPtr ln, int srid)
{
/* converting a Linestring into an RTLINE */
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    int iv;
    double x;
    double y;
    double z;
    double m;
    int has_z = 0;

    if (ln->DimensionModel == GAIA_XY_Z || ln->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;

    pa = ptarray_construct (ctx, has_z, 0, ln->Points);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* copying vertices */
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
	  point.x = x;
	  point.y = y;
	  if (has_z)
	      point.z = z;
	  else
	      point.z = 0.0;
	  point.m = 0.0;
	  ptarray_set_point4d (ctx, pa, iv, &point);
      }
    return rtline_construct (ctx, srid, NULL, pa);
}

GAIAGEO_DECLARE int
gaia3dLength (const void *p_cache, gaiaGeomCollPtr geom, double *length)
{
/* wrapping RTGEOM rtline_length */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTLINE *line;
    gaiaLinestringPtr ln;
    double l = 0.0;
    int ret = 0;

    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  ret = 1;
	  line = linestring2rtline (ctx, ln, geom->Srid);
	  l += rtgeom_length (ctx, (RTGEOM *) line);
	  rtline_free (ctx, line);
	  ln = ln->Next;
      }
    *length = l;

    return ret;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaNodeLines (const void *p_cache, gaiaGeomCollPtr geom)
{
/* wrapping RTGEOM rtgeom_node */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g1;
    RTGEOM *g2;
    gaiaGeomCollPtr result = NULL;

    if (!geom)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g1 = toRTGeom (ctx, geom);
    g2 = rtgeom_node (ctx, g1);
    if (!g2)
      {
	  rtgeom_free (ctx, g1);
	  goto done;
      }
    result = fromRTGeom (ctx, g2, geom->DimensionModel, geom->DeclaredType);
    spatialite_init_geos ();
    rtgeom_free (ctx, g1);
    rtgeom_free (ctx, g2);
    if (result == NULL)
	goto done;
    result->Srid = geom->Srid;

  done:
    return result;
}

GAIAGEO_DECLARE int
gaiaToTWKB (const void *p_cache, gaiaGeomCollPtr geom,
	    unsigned char precision_xy, unsigned char precision_z,
	    unsigned char precision_m, int with_size, int with_bbox,
	    unsigned char **twkb, int *size_twkb)
{
/* wrapping RTGEOM rtgeom_to_twkb */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    unsigned char variant = 0;
    RTGEOM *g;
    unsigned char *p_twkb;
    size_t twkb_size;

    *twkb = NULL;
    *size_twkb = 0;

    if (!geom)
	return 0;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (with_size)
	variant |= TWKB_SIZE;
    if (with_bbox)
	variant |= TWKB_BBOX;

    g = toRTGeom (ctx, geom);
    p_twkb =
	rtgeom_to_twkb (ctx, g, variant, precision_xy, precision_z, precision_m,
			&twkb_size);
    rtgeom_free (ctx, g);

    if (p_twkb == NULL)
	return 0;
    *twkb = p_twkb;
    *size_twkb = twkb_size;
    return 1;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromTWKB (const void *p_cache, const unsigned char *twkb, int twkb_size,
	      int srid)
{
/* wrapping RTGEOM rtgeom_from_twkb */
    const RTCTX *ctx = NULL;
    int dims = GAIA_XY_Z_M;
    int type = GAIA_GEOMETRYCOLLECTION;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    gaiaGeomCollPtr result;

    if (twkb == NULL)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g = rtgeom_from_twkb (ctx, (unsigned char *) twkb, twkb_size, 0);
    if (g == NULL)
	return NULL;
    if ((*(twkb + 0) & 0x01) == 0x01)
	type = GAIA_POINT;
    if ((*(twkb + 0) & 0x02) == 0x02)
	type = GAIA_LINESTRING;
    if ((*(twkb + 0) & 0x03) == 0x03)
	type = GAIA_POLYGON;
    if ((*(twkb + 0) & 0x04) == 0x04)
	type = GAIA_MULTIPOINT;
    if ((*(twkb + 0) & 0x05) == 0x05)
	type = GAIA_MULTILINESTRING;
    if ((*(twkb + 0) & 0x06) == 0x06)
	type = GAIA_MULTIPOLYGON;
    if ((*(twkb + 0) & 0x07) == 0x07)
	type = GAIA_GEOMETRYCOLLECTION;
    if ((*(twkb + 1) & 0x08) == 0x08)
      {
	  if ((*(twkb + 2) & 0x01) == 0x01)
	      dims = GAIA_XY_Z;
	  if ((*(twkb + 2) & 0x02) == 0x02)
	      dims = GAIA_XY_M;
	  if ((*(twkb + 2) & 0x03) == 0x03)
	      dims = GAIA_XY_Z_M;
      }
    else
	dims = GAIA_XY;
    result = fromRTGeom (ctx, g, dims, type);
    spatialite_init_geos ();
    rtgeom_free (ctx, g);
    if (result != NULL)
	result->Srid = srid;
    return result;
}

GAIAGEO_DECLARE int
gaiaAsEncodedPolyLine (const void *p_cache, gaiaGeomCollPtr geom,
		       unsigned char precision, char **encoded, int *len)
{
/* wrapping RTGEOM rtline_to_encoded_polyline */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    char *p_encoded;

    *encoded = NULL;
    *len = 0;

    if (!geom)
	return 0;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    g = toRTGeom (ctx, geom);
    p_encoded = rtgeom_to_encoded_polyline (ctx, g, precision);
    rtgeom_free (ctx, g);

    if (p_encoded == NULL)
	return 0;
    *encoded = p_encoded;
    *len = strlen (p_encoded);
    return 1;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLineFromEncodedPolyline (const void *p_cache, const char *encoded,
			     unsigned char precision)
{
/* wrapping RTGEOM rtline_from_encoded_polyline */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    RTGEOM *g;
    gaiaGeomCollPtr result;
    int dims = GAIA_XY;
    int type = GAIA_LINESTRING;

    if (encoded == NULL)
	return NULL;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

    g = rtgeom_from_encoded_polyline (ctx, encoded, precision);
    if (g == NULL)
	return NULL;
    result = fromRTGeom (ctx, g, dims, type);
    spatialite_init_geos ();
    rtgeom_free (ctx, g);
    if (result != NULL)
	result->Srid = 4326;
    return result;
}

static RTGEOM *
rtgeom_from_encoded_polyline (const RTCTX * ctx, const char *encodedpolyline,
			      int precision)
{
/*
 * RTTOPO lacks an implementation of rtgeom_from_encoded_polyline
 * 
 * this simply is a rearranged version of the original code available
 * on LWGOEM lwin_encoded_polyline.c
 * 
 * Copyright 2014 Kashif Rasul <kashif.rasul@gmail.com>
 * 
 * the original code was released under the terms of the GNU General 
 * Public License as published by the Free Software Foundation, either
 *  version 2 of the License, or (at your option) any later version.
*/
    RTGEOM *geom = NULL;
    RTPOINTARRAY *pa = NULL;
    int length = strlen (encodedpolyline);
    int idx = 0;
    double scale = pow (10, precision);

    float latitude = 0.0f;
    float longitude = 0.0f;

    pa = ptarray_construct_empty (ctx, RT_FALSE, RT_FALSE, 1);

    while (idx < length)
      {
	  RTPOINT4D pt;
	  char byte = 0;

	  int res = 0;
	  char shift = 0;
	  do
	    {
		byte = encodedpolyline[idx++] - 63;
		res |= (byte & 0x1F) << shift;
		shift += 5;
	    }
	  while (byte >= 0x20);
	  float deltaLat = ((res & 1) ? ~(res >> 1) : (res >> 1));
	  latitude += deltaLat;

	  shift = 0;
	  res = 0;
	  do
	    {
		byte = encodedpolyline[idx++] - 63;
		res |= (byte & 0x1F) << shift;
		shift += 5;
	    }
	  while (byte >= 0x20);
	  float deltaLon = ((res & 1) ? ~(res >> 1) : (res >> 1));
	  longitude += deltaLon;

	  pt.x = longitude / scale;
	  pt.y = latitude / scale;
	  pt.m = pt.z = 0.0;
	  ptarray_append_point (ctx, pa, &pt, RT_FALSE);
      }

    geom = (RTGEOM *) rtline_construct (ctx, 4326, NULL, pa);
    rtgeom_add_bbox (ctx, geom);

    return geom;
}


#endif /* end enabling RTTOPO support */
