/*

 gg_geoscvt.c -- Gaia / GEOS conversion [Geometry]
    
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

static GEOSGeometry *
toGeosGeometry (const void *cache, GEOSContextHandle_t handle,
		const gaiaGeomCollPtr gaia, int mode)
{
/* converting a GAIA Geometry into a GEOS Geometry */
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    int type;
    int geos_type;
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
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    GEOSGeometry *geos = NULL;
    GEOSGeometry *geos_ext;
    GEOSGeometry *geos_int;
    GEOSGeometry *geos_item;
    GEOSGeometry **geos_holes;
    GEOSGeometry **geos_coll;
    GEOSCoordSequence *cs;
    int ring_points;
    int n_items;

#ifdef GEOS_USE_ONLY_R_API	/* only fully thread-safe GEOS API */
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
    if (mode == GAIA2GEOS_ONLY_POINTS && pts == 0)
	return NULL;
    if (mode == GAIA2GEOS_ONLY_LINESTRINGS && lns == 0)
	return NULL;
    if (mode == GAIA2GEOS_ONLY_POLYGONS && pgs == 0)
	return NULL;
    if (pts == 0 && lns == 0 && pgs == 0)
	return NULL;
    else if (pts == 1 && lns == 0 && pgs == 0)
      {
	  if (gaia->DeclaredType == GAIA_MULTIPOINT)
	      type = GAIA_MULTIPOINT;
	  else if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_POINT;
      }
    else if (pts == 0 && lns == 1 && pgs == 0)
      {
	  if (gaia->DeclaredType == GAIA_MULTILINESTRING)
	      type = GAIA_MULTILINESTRING;
	  else if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_LINESTRING;
      }
    else if (pts == 0 && lns == 0 && pgs == 1)
      {
	  if (gaia->DeclaredType == GAIA_MULTIPOLYGON)
	      type = GAIA_MULTIPOLYGON;
	  else if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_POLYGON;
      }
    else if (pts > 1 && lns == 0 && pgs == 0)
      {
	  if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_MULTIPOINT;
      }
    else if (pts == 0 && lns > 1 && pgs == 0)
      {
	  if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_MULTILINESTRING;
      }
    else if (pts == 0 && lns == 0 && pgs > 1)
      {
	  if (gaia->DeclaredType == GAIA_GEOMETRYCOLLECTION)
	      type = GAIA_GEOMETRYCOLLECTION;
	  else
	      type = GAIA_MULTIPOLYGON;
      }
    else
	type = GAIA_GEOMETRYCOLLECTION;
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
    switch (type)
      {
      case GAIA_POINT:
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_POINTS)
	    {
		pt = gaia->FirstPoint;
		if (handle != NULL)
		  {
		      cs = GEOSCoordSeq_create_r (handle, 1, dims);
		      switch (gaia->DimensionModel)
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
		      geos = GEOSGeom_createPoint_r (handle, cs);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      cs = GEOSCoordSeq_create (1, dims);
		      switch (gaia->DimensionModel)
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
		      geos = GEOSGeom_createPoint (cs);
#endif
		  }
	    }
	  break;
      case GAIA_LINESTRING:
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_LINESTRINGS)
	    {
		ln = gaia->FirstLinestring;
		if (handle != NULL)
		    cs = GEOSCoordSeq_create_r (handle, ln->Points, dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    cs = GEOSCoordSeq_create (ln->Points, dims);
#endif
		for (iv = 0; iv < ln->Points; iv++)
		  {
		      switch (ln->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
				  GEOSCoordSeq_setZ_r (handle, cs, iv, z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
				  GEOSCoordSeq_setZ (cs, iv, z);
#endif
			      }
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
#endif
			      }
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
				  GEOSCoordSeq_setZ_r (handle, cs, iv, z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
				  GEOSCoordSeq_setZ (cs, iv, z);
#endif
			      }
			    break;
			default:
			    gaiaGetPoint (ln->Coords, iv, &x, &y);
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
#endif
			      }
			    break;
			};
		  }
		if (handle != NULL)
		    geos = GEOSGeom_createLineString_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    geos = GEOSGeom_createLineString (cs);
#endif
	    }
	  break;
      case GAIA_POLYGON:
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_POLYGONS)
	    {
		pg = gaia->FirstPolygon;
		rng = pg->Exterior;
		/* exterior ring */
		ring_points = rng->Points;
		if (cache)
		  {
		      if (gaiaIsNotClosedRing_r (cache, rng))
			  ring_points++;
		  }
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		  {
		      if (gaiaIsNotClosedRing (rng))
			  ring_points++;
		  }
#endif
		if (handle != NULL)
		    cs = GEOSCoordSeq_create_r (handle, ring_points, dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    cs = GEOSCoordSeq_create (ring_points, dims);
#endif
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    if (iv == 0)
			      {
				  /* saving the first vertex */
				  x0 = x;
				  y0 = y;
				  z0 = z;
			      }
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
				  GEOSCoordSeq_setZ_r (handle, cs, iv, z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
				  GEOSCoordSeq_setZ (cs, iv, z);
#endif
			      }
			    break;
			case GAIA_XY_M:
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    if (iv == 0)
			      {
				  /* saving the first vertex */
				  x0 = x;
				  y0 = y;
			      }
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
#endif
			      }
			    break;
			case GAIA_XY_Z_M:
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    if (iv == 0)
			      {
				  /* saving the first vertex */
				  x0 = x;
				  y0 = y;
				  z0 = z;
			      }
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
				  GEOSCoordSeq_setZ_r (handle, cs, iv, z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
				  GEOSCoordSeq_setZ (cs, iv, z);
#endif
			      }
			    break;
			default:
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    if (iv == 0)
			      {
				  /* saving the first vertex */
				  x0 = x;
				  y0 = y;
			      }
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x);
				  GEOSCoordSeq_setY (cs, iv, y);
#endif
			      }
			    break;
			};
		  }
		if (ring_points > rng->Points)
		  {
		      /* ensuring Ring's closure */
		      iv = ring_points - 1;
		      switch (rng->DimensionModel)
			{
			case GAIA_XY_Z:
			case GAIA_XY_Z_M:
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x0);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y0);
				  GEOSCoordSeq_setZ_r (handle, cs, iv, z0);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x0);
				  GEOSCoordSeq_setY (cs, iv, y0);
				  GEOSCoordSeq_setZ (cs, iv, z0);
#endif
			      }
			    break;
			default:
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, iv, x0);
				  GEOSCoordSeq_setY_r (handle, cs, iv, y0);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, iv, x0);
				  GEOSCoordSeq_setY (cs, iv, y0);
#endif
			      }
			    break;
			};
		  }
		if (handle != NULL)
		    geos_ext = GEOSGeom_createLinearRing_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    geos_ext = GEOSGeom_createLinearRing (cs);
#endif
		geos_holes = NULL;
		if (pg->NumInteriors > 0)
		  {
		      geos_holes =
			  malloc (sizeof (GEOSGeometry *) * pg->NumInteriors);
		      for (ib = 0; ib < pg->NumInteriors; ib++)
			{
			    /* interior ring */
			    rng = pg->Interiors + ib;
			    ring_points = rng->Points;
			    if (cache != NULL)
			      {
				  if (gaiaIsNotClosedRing_r (cache, rng))
				      ring_points++;
			      }
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    else
			      {
				  if (gaiaIsNotClosedRing (rng))
				      ring_points++;
			      }
#endif
			    if (handle != NULL)
				cs = GEOSCoordSeq_create_r (handle, ring_points,
							    dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    else
				cs = GEOSCoordSeq_create (ring_points, dims);
#endif
			    for (iv = 0; iv < rng->Points; iv++)
			      {
				  switch (rng->DimensionModel)
				    {
				    case GAIA_XY_Z:
					gaiaGetPointXYZ (rng->Coords, iv, &x,
							 &y, &z);
					if (iv == 0)
					  {
					      /* saving the first vertex */
					      x0 = x;
					      y0 = y;
					      z0 = z;
					  }
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y);
					      GEOSCoordSeq_setZ_r (handle, cs,
								   iv, z);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x);
					      GEOSCoordSeq_setY (cs, iv, y);
					      GEOSCoordSeq_setZ (cs, iv, z);
#endif
					  }
					break;
				    case GAIA_XY_M:
					gaiaGetPointXYM (rng->Coords, iv, &x,
							 &y, &m);
					if (iv == 0)
					  {
					      /* saving the first vertex */
					      x0 = x;
					      y0 = y;
					  }
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x);
					      GEOSCoordSeq_setY (cs, iv, y);
#endif
					  }
					break;
				    case GAIA_XY_Z_M:
					gaiaGetPointXYZM (rng->Coords, iv, &x,
							  &y, &z, &m);
					if (iv == 0)
					  {
					      /* saving the first vertex */
					      x0 = x;
					      y0 = y;
					      z0 = z;
					  }
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y);
					      GEOSCoordSeq_setZ_r (handle, cs,
								   iv, z);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x);
					      GEOSCoordSeq_setY (cs, iv, y);
					      GEOSCoordSeq_setZ (cs, iv, z);
#endif
					  }
					break;
				    default:
					gaiaGetPoint (rng->Coords, iv, &x, &y);
					if (iv == 0)
					  {
					      /* saving the first vertex */
					      x0 = x;
					      y0 = y;
					  }
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x);
					      GEOSCoordSeq_setY (cs, iv, y);
#endif
					  }
					break;
				    };
			      }
			    if (ring_points > rng->Points)
			      {
				  /* ensuring Ring's closure */
				  iv = ring_points - 1;
				  switch (rng->DimensionModel)
				    {
				    case GAIA_XY_Z:
				    case GAIA_XY_Z_M:
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x0);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y0);
					      GEOSCoordSeq_setZ_r (handle, cs,
								   iv, z0);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x0);
					      GEOSCoordSeq_setY (cs, iv, y0);
					      GEOSCoordSeq_setZ (cs, iv, z0);
#endif
					  }
					break;
				    default:
					if (handle != NULL)
					  {
					      GEOSCoordSeq_setX_r (handle, cs,
								   iv, x0);
					      GEOSCoordSeq_setY_r (handle, cs,
								   iv, y0);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_setX (cs, iv, x0);
					      GEOSCoordSeq_setY (cs, iv, y0);
#endif
					  }
					break;
				    };
			      }
			    if (handle != NULL)
				geos_int =
				    GEOSGeom_createLinearRing_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    else
				geos_int = GEOSGeom_createLinearRing (cs);
#endif
			    *(geos_holes + ib) = geos_int;
			}
		  }
		if (handle != NULL)
		    geos =
			GEOSGeom_createPolygon_r (handle, geos_ext, geos_holes,
						  pg->NumInteriors);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		else
		    geos =
			GEOSGeom_createPolygon (geos_ext, geos_holes,
						pg->NumInteriors);
#endif
		if (geos_holes)
		    free (geos_holes);
	    }
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTIPOLYGON:
      case GAIA_GEOMETRYCOLLECTION:
	  nItem = 0;
	  if (mode == GAIA2GEOS_ONLY_POINTS)
	    {
		geos_coll = malloc (sizeof (GEOSGeometry *) * (pts));
		n_items = pts;
	    }
	  else if (mode == GAIA2GEOS_ONLY_LINESTRINGS)
	    {
		geos_coll = malloc (sizeof (GEOSGeometry *) * (lns));
		n_items = lns;
	    }
	  else if (mode == GAIA2GEOS_ONLY_POLYGONS)
	    {
		geos_coll = malloc (sizeof (GEOSGeometry *) * (pgs));
		n_items = pgs;
	    }
	  else
	    {
		geos_coll =
		    malloc (sizeof (GEOSGeometry *) * (pts + lns + pgs));
		n_items = pts + lns + pgs;
	    }
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_POINTS)
	    {
		pt = gaia->FirstPoint;
		while (pt)
		  {
		      if (handle != NULL)
			  cs = GEOSCoordSeq_create_r (handle, 1, dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  cs = GEOSCoordSeq_create (1, dims);
#endif
		      switch (pt->DimensionModel)
			{
			case GAIA_XY_Z:
			case GAIA_XY_Z_M:
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, pt->X);
				  GEOSCoordSeq_setY_r (handle, cs, 0, pt->Y);
				  GEOSCoordSeq_setZ_r (handle, cs, 0, pt->Z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, 0, pt->X);
				  GEOSCoordSeq_setY (cs, 0, pt->Y);
				  GEOSCoordSeq_setZ (cs, 0, pt->Z);
#endif
			      }
			    break;
			default:
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_setX_r (handle, cs, 0, pt->X);
				  GEOSCoordSeq_setY_r (handle, cs, 0, pt->Y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_setX (cs, 0, pt->X);
				  GEOSCoordSeq_setY (cs, 0, pt->Y);
#endif
			      }
			    break;
			};
		      if (handle != NULL)
			  geos_item = GEOSGeom_createPoint_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  geos_item = GEOSGeom_createPoint (cs);
#endif
		      *(geos_coll + nItem++) = geos_item;
		      pt = pt->Next;
		  }
	    }
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_LINESTRINGS)
	    {
		ln = gaia->FirstLinestring;
		while (ln)
		  {
		      if (handle != NULL)
			  cs = GEOSCoordSeq_create_r (handle, ln->Points, dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  cs = GEOSCoordSeq_create (ln->Points, dims);
#endif
		      for (iv = 0; iv < ln->Points; iv++)
			{
			    switch (ln->DimensionModel)
			      {
			      case GAIA_XY_Z:
				  gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
					GEOSCoordSeq_setZ_r (handle, cs, iv, z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
					GEOSCoordSeq_setZ (cs, iv, z);
#endif
				    }
				  break;
			      case GAIA_XY_M:
				  gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
#endif
				    }
				  break;
			      case GAIA_XY_Z_M:
				  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z,
						    &m);
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
					GEOSCoordSeq_setZ_r (handle, cs, iv, z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
					GEOSCoordSeq_setZ (cs, iv, z);
#endif
				    }
				  break;
			      default:
				  gaiaGetPoint (ln->Coords, iv, &x, &y);
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
#endif
				    }
				  break;
			      };
			}
		      if (handle != NULL)
			  geos_item = GEOSGeom_createLineString_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  geos_item = GEOSGeom_createLineString (cs);
#endif
		      *(geos_coll + nItem++) = geos_item;
		      ln = ln->Next;
		  }
	    }
	  if (mode == GAIA2GEOS_ALL || mode == GAIA2GEOS_ONLY_POLYGONS)
	    {
		pg = gaia->FirstPolygon;
		while (pg)
		  {
		      rng = pg->Exterior;
		      /* exterior ring */
		      ring_points = rng->Points;
		      if (cache != NULL)
			{
			    if (gaiaIsNotClosedRing_r (cache, rng))
				ring_points++;
			}
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			{
			    if (gaiaIsNotClosedRing (rng))
				ring_points++;
			}
#endif
		      if (handle != NULL)
			  cs = GEOSCoordSeq_create_r (handle, ring_points,
						      dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  cs = GEOSCoordSeq_create (ring_points, dims);
#endif
		      for (iv = 0; iv < rng->Points; iv++)
			{
			    switch (rng->DimensionModel)
			      {
			      case GAIA_XY_Z:
				  gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
				  if (iv == 0)
				    {
					/* saving the first vertex */
					x0 = x;
					y0 = y;
					z0 = z;
				    }
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
					GEOSCoordSeq_setZ_r (handle, cs, iv, z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
					GEOSCoordSeq_setZ (cs, iv, z);
#endif
				    }
				  break;
			      case GAIA_XY_M:
				  gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
				  if (iv == 0)
				    {
					/* saving the first vertex */
					x0 = x;
					y0 = y;
				    }
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
#endif
				    }
				  break;
			      case GAIA_XY_Z_M:
				  gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z,
						    &m);
				  if (iv == 0)
				    {
					/* saving the first vertex */
					x0 = x;
					y0 = y;
					z0 = z;
				    }
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
					GEOSCoordSeq_setZ_r (handle, cs, iv, z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
					GEOSCoordSeq_setZ (cs, iv, z);
#endif
				    }
				  break;
			      default:
				  gaiaGetPoint (rng->Coords, iv, &x, &y);
				  if (iv == 0)
				    {
					/* saving the first vertex */
					x0 = x;
					y0 = y;
				    }
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv, x);
					GEOSCoordSeq_setY_r (handle, cs, iv, y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x);
					GEOSCoordSeq_setY (cs, iv, y);
#endif
				    }
				  break;
			      };
			}
		      if (ring_points > rng->Points)
			{
			    /* ensuring Ring's closure */
			    iv = ring_points - 1;
			    switch (rng->DimensionModel)
			      {
			      case GAIA_XY_Z:
			      case GAIA_XY_Z_M:
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv,
							     x0);
					GEOSCoordSeq_setY_r (handle, cs, iv,
							     y0);
					GEOSCoordSeq_setZ_r (handle, cs, iv,
							     z0);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x0);
					GEOSCoordSeq_setY (cs, iv, y0);
					GEOSCoordSeq_setZ (cs, iv, z0);
#endif
				    }
				  break;
			      default:
				  if (handle != NULL)
				    {
					GEOSCoordSeq_setX_r (handle, cs, iv,
							     x0);
					GEOSCoordSeq_setY_r (handle, cs, iv,
							     y0);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_setX (cs, iv, x0);
					GEOSCoordSeq_setY (cs, iv, y0);
#endif
				    }
				  break;
			      };
			}
		      if (handle != NULL)
			  geos_ext = GEOSGeom_createLinearRing_r (handle, cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  geos_ext = GEOSGeom_createLinearRing (cs);
#endif
		      geos_holes = NULL;
		      if (pg->NumInteriors > 0)
			{
			    geos_holes =
				malloc (sizeof (GEOSGeometry *) *
					pg->NumInteriors);
			    for (ib = 0; ib < pg->NumInteriors; ib++)
			      {
				  /* interior ring */
				  rng = pg->Interiors + ib;
				  ring_points = rng->Points;
				  if (cache != NULL)
				    {
					if (gaiaIsNotClosedRing_r (cache, rng))
					    ring_points++;
				    }
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  else
				    {
					if (gaiaIsNotClosedRing (rng))
					    ring_points++;
				    }
#endif
				  if (handle != NULL)
				      cs = GEOSCoordSeq_create_r (handle,
								  ring_points,
								  dims);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  else
				      cs = GEOSCoordSeq_create (ring_points,
								dims);
#endif
				  for (iv = 0; iv < rng->Points; iv++)
				    {
					switch (rng->DimensionModel)
					  {
					  case GAIA_XY_Z:
					      gaiaGetPointXYZ (rng->Coords, iv,
							       &x, &y, &z);
					      if (iv == 0)
						{
						    /* saving the first vertex */
						    x0 = x;
						    y0 = y;
						    z0 = z;
						}
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y);
						    GEOSCoordSeq_setZ_r (handle,
									 cs, iv,
									 z);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x);
						    GEOSCoordSeq_setY (cs, iv,
								       y);
						    GEOSCoordSeq_setZ (cs, iv,
								       z);
#endif
						}
					      break;
					  case GAIA_XY_M:
					      gaiaGetPointXYM (rng->Coords, iv,
							       &x, &y, &m);
					      if (iv == 0)
						{
						    /* saving the first vertex */
						    x0 = x;
						    y0 = y;
						}
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x);
						    GEOSCoordSeq_setY (cs, iv,
								       y);
#endif
						}
					      break;
					  case GAIA_XY_Z_M:
					      gaiaGetPointXYZM (rng->Coords, iv,
								&x, &y, &z, &m);
					      if (iv == 0)
						{
						    /* saving the first vertex */
						    x0 = x;
						    y0 = y;
						    z0 = z;
						}
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y);
						    GEOSCoordSeq_setZ_r (handle,
									 cs, iv,
									 z);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x);
						    GEOSCoordSeq_setY (cs, iv,
								       y);
						    GEOSCoordSeq_setZ (cs, iv,
								       z);
#endif
						}
					      break;
					  default:
					      gaiaGetPoint (rng->Coords, iv, &x,
							    &y);
					      if (iv == 0)
						{
						    /* saving the first vertex */
						    x0 = x;
						    y0 = y;
						}
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x);
						    GEOSCoordSeq_setY (cs, iv,
								       y);
#endif
						}
					      break;
					  };
				    }
				  if (ring_points > rng->Points)
				    {
					/* ensuring Ring's closure */
					iv = ring_points - 1;
					switch (rng->DimensionModel)
					  {
					  case GAIA_XY_Z:
					  case GAIA_XY_Z_M:
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x0);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y0);
						    GEOSCoordSeq_setZ_r (handle,
									 cs, iv,
									 z0);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x0);
						    GEOSCoordSeq_setY (cs, iv,
								       y0);
						    GEOSCoordSeq_setZ (cs, iv,
								       z0);
#endif
						}
					      break;
					  default:
					      if (handle != NULL)
						{
						    GEOSCoordSeq_setX_r (handle,
									 cs, iv,
									 x0);
						    GEOSCoordSeq_setY_r (handle,
									 cs, iv,
									 y0);
						}
					      else
						{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
						    GEOSCoordSeq_setX (cs, iv,
								       x0);
						    GEOSCoordSeq_setY (cs, iv,
								       y0);
#endif
						}
					      break;
					  };
				    }
				  if (handle != NULL)
				      geos_int =
					  GEOSGeom_createLinearRing_r (handle,
								       cs);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  else
				      geos_int = GEOSGeom_createLinearRing (cs);
#endif
				  *(geos_holes + ib) = geos_int;
			      }
			}
		      if (handle != NULL)
			  geos_item =
			      GEOSGeom_createPolygon_r (handle, geos_ext,
							geos_holes,
							pg->NumInteriors);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  geos_item =
			      GEOSGeom_createPolygon (geos_ext, geos_holes,
						      pg->NumInteriors);
#endif
		      if (geos_holes)
			  free (geos_holes);
		      *(geos_coll + nItem++) = geos_item;
		      pg = pg->Next;
		  }
	    }
	  geos_type = GEOS_GEOMETRYCOLLECTION;
	  if (type == GAIA_MULTIPOINT)
	      geos_type = GEOS_MULTIPOINT;
	  if (type == GAIA_MULTILINESTRING)
	      geos_type = GEOS_MULTILINESTRING;
	  if (type == GAIA_MULTIPOLYGON)
	      geos_type = GEOS_MULTIPOLYGON;
	  if (handle != NULL)
	      geos =
		  GEOSGeom_createCollection_r (handle, geos_type, geos_coll,
					       n_items);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  else
	      geos = GEOSGeom_createCollection (geos_type, geos_coll, n_items);
#endif
	  if (geos_coll)
	      free (geos_coll);
	  break;
      default:
	  geos = NULL;
      };
    if (geos)
      {
	  if (handle != NULL)
	      GEOSSetSRID_r (handle, geos, gaia->Srid);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  else
	      GEOSSetSRID (geos, gaia->Srid);
#endif
      }
    return geos;
}

static gaiaGeomCollPtr
fromGeosGeometry (GEOSContextHandle_t handle, const GEOSGeometry * geos,
		  const int dimension_model)
{
/* converting a GEOS Geometry into a GAIA Geometry */
    int type;
    int itemType;
    unsigned int dims;
    int iv;
    int ib;
    int it;
    int sub_it;
    int nItems;
    int nSubItems;
    int holes;
    unsigned int points;
    double x;
    double y;
    double z;
    const GEOSCoordSequence *cs;
    const GEOSGeometry *geos_ring;
    const GEOSGeometry *geos_item;
    const GEOSGeometry *geos_sub_item;
    gaiaGeomCollPtr gaia = NULL;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;

#ifdef GEOS_USE_ONLY_R_API	/* only fully thread-safe GEOS API */
    if (handle == NULL)
	return NULL;
#endif
    if (!geos)
	return NULL;

    if (handle != NULL)
	type = GEOSGeomTypeId_r (handle, geos);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
    else
	type = GEOSGeomTypeId (geos);
#endif
    switch (type)
      {
      case GEOS_POINT:
	  if (dimension_model == GAIA_XY_Z)
	      gaia = gaiaAllocGeomCollXYZ ();
	  else if (dimension_model == GAIA_XY_M)
	      gaia = gaiaAllocGeomCollXYM ();
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaia = gaiaAllocGeomCollXYZM ();
	  else
	      gaia = gaiaAllocGeomColl ();
	  gaia->DeclaredType = GAIA_POINT;
	  if (handle != NULL)
	    {
		gaia->Srid = GEOSGetSRID_r (handle, geos);
		cs = GEOSGeom_getCoordSeq_r (handle, geos);
		GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		gaia->Srid = GEOSGetSRID (geos);
		cs = GEOSGeom_getCoordSeq (geos);
		GEOSCoordSeq_getDimensions (cs, &dims);
#endif
	    }
	  if (dims == 3)
	    {
		if (handle != NULL)
		  {
		      GEOSCoordSeq_getX_r (handle, cs, 0, &x);
		      GEOSCoordSeq_getY_r (handle, cs, 0, &y);
		      GEOSCoordSeq_getZ_r (handle, cs, 0, &z);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      GEOSCoordSeq_getX (cs, 0, &x);
		      GEOSCoordSeq_getY (cs, 0, &y);
		      GEOSCoordSeq_getZ (cs, 0, &z);
#endif
		  }
	    }
	  else
	    {
		if (handle != NULL)
		  {
		      GEOSCoordSeq_getX_r (handle, cs, 0, &x);
		      GEOSCoordSeq_getY_r (handle, cs, 0, &y);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      GEOSCoordSeq_getX (cs, 0, &x);
		      GEOSCoordSeq_getY (cs, 0, &y);
#endif
		  }
		z = 0.0;
	    }
	  if (dimension_model == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
	  else if (dimension_model == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (gaia, x, y, 0.0);
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (gaia, x, y, z, 0.0);
	  else
	      gaiaAddPointToGeomColl (gaia, x, y);
	  break;
      case GEOS_LINESTRING:
	  if (dimension_model == GAIA_XY_Z)
	      gaia = gaiaAllocGeomCollXYZ ();
	  else if (dimension_model == GAIA_XY_M)
	      gaia = gaiaAllocGeomCollXYM ();
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaia = gaiaAllocGeomCollXYZM ();
	  else
	      gaia = gaiaAllocGeomColl ();
	  gaia->DeclaredType = GAIA_LINESTRING;
	  if (handle != NULL)
	    {
		gaia->Srid = GEOSGetSRID_r (handle, geos);
		cs = GEOSGeom_getCoordSeq_r (handle, geos);
		GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
		GEOSCoordSeq_getSize_r (handle, cs, &points);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		gaia->Srid = GEOSGetSRID (geos);
		cs = GEOSGeom_getCoordSeq (geos);
		GEOSCoordSeq_getDimensions (cs, &dims);
		GEOSCoordSeq_getSize (cs, &points);
#endif
	    }
	  ln = gaiaAddLinestringToGeomColl (gaia, points);
	  for (iv = 0; iv < (int) points; iv++)
	    {
		if (dims == 3)
		  {
		      if (handle != NULL)
			{
			    GEOSCoordSeq_getX_r (handle, cs, iv, &x);
			    GEOSCoordSeq_getY_r (handle, cs, iv, &y);
			    GEOSCoordSeq_getZ_r (handle, cs, iv, &z);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    GEOSCoordSeq_getX (cs, iv, &x);
			    GEOSCoordSeq_getY (cs, iv, &y);
			    GEOSCoordSeq_getZ (cs, iv, &z);
#endif
			}
		  }
		else
		  {
		      if (handle != NULL)
			{
			    GEOSCoordSeq_getX_r (handle, cs, iv, &x);
			    GEOSCoordSeq_getY_r (handle, cs, iv, &y);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    GEOSCoordSeq_getX (cs, iv, &x);
			    GEOSCoordSeq_getY (cs, iv, &y);
#endif
			}
		      z = 0.0;
		  }
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (ln->Coords, iv, x, y, 0.0);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (ln->Coords, iv, x, y, z, 0.0);
		  }
		else
		  {
		      gaiaSetPoint (ln->Coords, iv, x, y);
		  }
	    }
	  break;
      case GEOS_POLYGON:
	  if (dimension_model == GAIA_XY_Z)
	      gaia = gaiaAllocGeomCollXYZ ();
	  else if (dimension_model == GAIA_XY_M)
	      gaia = gaiaAllocGeomCollXYM ();
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaia = gaiaAllocGeomCollXYZM ();
	  else
	      gaia = gaiaAllocGeomColl ();
	  gaia->DeclaredType = GAIA_POLYGON;
	  if (handle != NULL)
	      gaia->Srid = GEOSGetSRID_r (handle, geos);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
	  else
	      gaia->Srid = GEOSGetSRID (geos);
#endif
	  /* exterior ring */
	  if (handle != NULL)
	    {
		holes = GEOSGetNumInteriorRings_r (handle, geos);
		geos_ring = GEOSGetExteriorRing_r (handle, geos);
		cs = GEOSGeom_getCoordSeq_r (handle, geos_ring);
		GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
		GEOSCoordSeq_getSize_r (handle, cs, &points);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		holes = GEOSGetNumInteriorRings (geos);
		geos_ring = GEOSGetExteriorRing (geos);
		cs = GEOSGeom_getCoordSeq (geos_ring);
		GEOSCoordSeq_getDimensions (cs, &dims);
		GEOSCoordSeq_getSize (cs, &points);
#endif
	    }
	  pg = gaiaAddPolygonToGeomColl (gaia, points, holes);
	  rng = pg->Exterior;
	  for (iv = 0; iv < (int) points; iv++)
	    {
		if (dims == 3)
		  {
		      if (handle != NULL)
			{
			    GEOSCoordSeq_getX_r (handle, cs, iv, &x);
			    GEOSCoordSeq_getY_r (handle, cs, iv, &y);
			    GEOSCoordSeq_getZ_r (handle, cs, iv, &z);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    GEOSCoordSeq_getX (cs, iv, &x);
			    GEOSCoordSeq_getY (cs, iv, &y);
			    GEOSCoordSeq_getZ (cs, iv, &z);
#endif
			}
		  }
		else
		  {
		      if (handle != NULL)
			{
			    GEOSCoordSeq_getX_r (handle, cs, iv, &x);
			    GEOSCoordSeq_getY_r (handle, cs, iv, &y);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    GEOSCoordSeq_getX (cs, iv, &x);
			    GEOSCoordSeq_getY (cs, iv, &y);
#endif
			}
		      z = 0.0;
		  }
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else if (dimension_model == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (rng->Coords, iv, x, y, 0.0);
		  }
		else if (dimension_model == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (rng->Coords, iv, x, y, z, 0.0);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < holes; ib++)
	    {
		/* interior rings */
		if (handle != NULL)
		  {
		      geos_ring = GEOSGetInteriorRingN_r (handle, geos, ib);
		      cs = GEOSGeom_getCoordSeq_r (handle, geos_ring);
		      GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
		      GEOSCoordSeq_getSize_r (handle, cs, &points);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      geos_ring = GEOSGetInteriorRingN (geos, ib);
		      cs = GEOSGeom_getCoordSeq (geos_ring);
		      GEOSCoordSeq_getDimensions (cs, &dims);
		      GEOSCoordSeq_getSize (cs, &points);
#endif
		  }
		rng = gaiaAddInteriorRing (pg, ib, points);
		for (iv = 0; iv < (int) points; iv++)
		  {
		      if (dims == 3)
			{
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_getX_r (handle, cs, iv, &x);
				  GEOSCoordSeq_getY_r (handle, cs, iv, &y);
				  GEOSCoordSeq_getZ_r (handle, cs, iv, &z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_getX (cs, iv, &x);
				  GEOSCoordSeq_getY (cs, iv, &y);
				  GEOSCoordSeq_getZ (cs, iv, &z);
#endif
			      }
			}
		      else
			{
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_getX_r (handle, cs, iv, &x);
				  GEOSCoordSeq_getY_r (handle, cs, iv, &y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_getX (cs, iv, &x);
				  GEOSCoordSeq_getY (cs, iv, &y);
#endif
			      }
			    z = 0.0;
			}
		      if (dimension_model == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			}
		      else if (dimension_model == GAIA_XY_M)
			{
			    gaiaSetPointXYM (rng->Coords, iv, x, y, 0.0);
			}
		      else if (dimension_model == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (rng->Coords, iv, x, y, z, 0.0);
			}
		      else
			{
			    gaiaSetPoint (rng->Coords, iv, x, y);
			}
		  }
	    }
	  break;
      case GEOS_MULTIPOINT:
      case GEOS_MULTILINESTRING:
      case GEOS_MULTIPOLYGON:
      case GEOS_GEOMETRYCOLLECTION:
	  if (dimension_model == GAIA_XY_Z)
	      gaia = gaiaAllocGeomCollXYZ ();
	  else if (dimension_model == GAIA_XY_M)
	      gaia = gaiaAllocGeomCollXYM ();
	  else if (dimension_model == GAIA_XY_Z_M)
	      gaia = gaiaAllocGeomCollXYZM ();
	  else
	      gaia = gaiaAllocGeomColl ();
	  if (type == GEOS_MULTIPOINT)
	      gaia->DeclaredType = GAIA_MULTIPOINT;
	  else if (type == GEOS_MULTILINESTRING)
	      gaia->DeclaredType = GAIA_MULTILINESTRING;
	  else if (type == GEOS_MULTIPOLYGON)
	      gaia->DeclaredType = GAIA_MULTIPOLYGON;
	  else
	      gaia->DeclaredType = GAIA_GEOMETRYCOLLECTION;
	  if (handle != NULL)
	    {
		gaia->Srid = GEOSGetSRID_r (handle, geos);
		nItems = GEOSGetNumGeometries_r (handle, geos);
	    }
	  else
	    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		gaia->Srid = GEOSGetSRID (geos);
		nItems = GEOSGetNumGeometries (geos);
#endif
	    }
	  for (it = 0; it < nItems; it++)
	    {
		/* looping on elementaty geometries */
		if (handle != NULL)
		  {
		      geos_item = GEOSGetGeometryN_r (handle, geos, it);
		      itemType = GEOSGeomTypeId_r (handle, geos_item);
		  }
		else
		  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      geos_item = GEOSGetGeometryN (geos, it);
		      itemType = GEOSGeomTypeId (geos_item);
#endif
		  }
		switch (itemType)
		  {
		  case GEOS_POINT:
		      if (handle != NULL)
			{
			    cs = GEOSGeom_getCoordSeq_r (handle, geos_item);
			    GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    cs = GEOSGeom_getCoordSeq (geos_item);
			    GEOSCoordSeq_getDimensions (cs, &dims);
#endif
			}
		      if (dims == 3)
			{
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_getX_r (handle, cs, 0, &x);
				  GEOSCoordSeq_getY_r (handle, cs, 0, &y);
				  GEOSCoordSeq_getZ_r (handle, cs, 0, &z);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_getX (cs, 0, &x);
				  GEOSCoordSeq_getY (cs, 0, &y);
				  GEOSCoordSeq_getZ (cs, 0, &z);
#endif
			      }
			}
		      else
			{
			    if (handle != NULL)
			      {
				  GEOSCoordSeq_getX_r (handle, cs, 0, &x);
				  GEOSCoordSeq_getY_r (handle, cs, 0, &y);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  GEOSCoordSeq_getX (cs, 0, &x);
				  GEOSCoordSeq_getY (cs, 0, &y);
#endif
			      }
			    z = 0.0;
			}
		      if (dimension_model == GAIA_XY_Z)
			  gaiaAddPointToGeomCollXYZ (gaia, x, y, z);
		      else if (dimension_model == GAIA_XY_M)
			  gaiaAddPointToGeomCollXYM (gaia, x, y, 0.0);
		      else if (dimension_model == GAIA_XY_Z_M)
			  gaiaAddPointToGeomCollXYZM (gaia, x, y, z, 0.0);
		      else
			  gaiaAddPointToGeomColl (gaia, x, y);
		      break;
		  case GEOS_LINESTRING:
		      if (handle != NULL)
			{
			    cs = GEOSGeom_getCoordSeq_r (handle, geos_item);
			    GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
			    GEOSCoordSeq_getSize_r (handle, cs, &points);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    cs = GEOSGeom_getCoordSeq (geos_item);
			    GEOSCoordSeq_getDimensions (cs, &dims);
			    GEOSCoordSeq_getSize (cs, &points);
#endif
			}
		      ln = gaiaAddLinestringToGeomColl (gaia, points);
		      for (iv = 0; iv < (int) points; iv++)
			{
			    if (dims == 3)
			      {
				  if (handle != NULL)
				    {
					GEOSCoordSeq_getX_r (handle, cs, iv,
							     &x);
					GEOSCoordSeq_getY_r (handle, cs, iv,
							     &y);
					GEOSCoordSeq_getZ_r (handle, cs, iv,
							     &z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_getX (cs, iv, &x);
					GEOSCoordSeq_getY (cs, iv, &y);
					GEOSCoordSeq_getZ (cs, iv, &z);
#endif
				    }
			      }
			    else
			      {
				  if (handle != NULL)
				    {
					GEOSCoordSeq_getX_r (handle, cs, iv,
							     &x);
					GEOSCoordSeq_getY_r (handle, cs, iv,
							     &y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_getX (cs, iv, &x);
					GEOSCoordSeq_getY (cs, iv, &y);
#endif
				    }
				  z = 0.0;
			      }
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (ln->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (ln->Coords, iv, x, y, 0.0);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (ln->Coords, iv, x, y, z,
						    0.0);
			      }
			    else
			      {
				  gaiaSetPoint (ln->Coords, iv, x, y);
			      }
			}
		      break;
		  case GEOS_MULTILINESTRING:
		      if (handle != NULL)
			  nSubItems =
			      GEOSGetNumGeometries_r (handle, geos_item);
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
		      else
			  nSubItems = GEOSGetNumGeometries (geos_item);
#endif
		      for (sub_it = 0; sub_it < nSubItems; sub_it++)
			{
			    /* looping on elementaty geometries */
			    if (handle != NULL)
			      {
				  geos_sub_item =
				      GEOSGetGeometryN_r (handle, geos_item,
							  sub_it);
				  cs = GEOSGeom_getCoordSeq_r (handle,
							       geos_sub_item);
				  GEOSCoordSeq_getDimensions_r (handle, cs,
								&dims);
				  GEOSCoordSeq_getSize_r (handle, cs, &points);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  geos_sub_item =
				      GEOSGetGeometryN (geos_item, sub_it);
				  cs = GEOSGeom_getCoordSeq (geos_sub_item);
				  GEOSCoordSeq_getDimensions (cs, &dims);
				  GEOSCoordSeq_getSize (cs, &points);
#endif
			      }
			    ln = gaiaAddLinestringToGeomColl (gaia, points);
			    for (iv = 0; iv < (int) points; iv++)
			      {
				  if (dims == 3)
				    {
					if (handle != NULL)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   iv, &x);
					      GEOSCoordSeq_getY_r (handle, cs,
								   iv, &y);
					      GEOSCoordSeq_getZ_r (handle, cs,
								   iv, &z);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_getX (cs, iv, &x);
					      GEOSCoordSeq_getY (cs, iv, &y);
					      GEOSCoordSeq_getZ (cs, iv, &z);
#endif
					  }
				    }
				  else
				    {
					if (handle != NULL)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   iv, &x);
					      GEOSCoordSeq_getY_r (handle, cs,
								   iv, &y);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_getX (cs, iv, &x);
					      GEOSCoordSeq_getY (cs, iv, &y);
#endif
					  }
					z = 0.0;
				    }
				  if (dimension_model == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (ln->Coords, iv, x, y,
							 z);
				    }
				  else if (dimension_model == GAIA_XY_M)
				    {
					gaiaSetPointXYM (ln->Coords, iv, x, y,
							 0.0);
				    }
				  else if (dimension_model == GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (ln->Coords, iv, x, y,
							  z, 0.0);
				    }
				  else
				    {
					gaiaSetPoint (ln->Coords, iv, x, y);
				    }
			      }
			}
		      break;
		  case GEOS_POLYGON:
		      /* exterior ring */
		      if (handle != NULL)
			{
			    holes =
				GEOSGetNumInteriorRings_r (handle, geos_item);
			    geos_ring =
				GEOSGetExteriorRing_r (handle, geos_item);
			    cs = GEOSGeom_getCoordSeq_r (handle, geos_ring);
			    GEOSCoordSeq_getDimensions_r (handle, cs, &dims);
			    GEOSCoordSeq_getSize_r (handle, cs, &points);
			}
		      else
			{
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
			    holes = GEOSGetNumInteriorRings (geos_item);
			    geos_ring = GEOSGetExteriorRing (geos_item);
			    cs = GEOSGeom_getCoordSeq (geos_ring);
			    GEOSCoordSeq_getDimensions (cs, &dims);
			    GEOSCoordSeq_getSize (cs, &points);
#endif
			}
		      pg = gaiaAddPolygonToGeomColl (gaia, points, holes);
		      rng = pg->Exterior;
		      for (iv = 0; iv < (int) points; iv++)
			{
			    if (dims == 3)
			      {
				  if (handle != NULL)
				    {
					GEOSCoordSeq_getX_r (handle, cs, iv,
							     &x);
					GEOSCoordSeq_getY_r (handle, cs, iv,
							     &y);
					GEOSCoordSeq_getZ_r (handle, cs, iv,
							     &z);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_getX (cs, iv, &x);
					GEOSCoordSeq_getY (cs, iv, &y);
					GEOSCoordSeq_getZ (cs, iv, &z);
#endif
				    }
			      }
			    else
			      {
				  if (handle != NULL)
				    {
					GEOSCoordSeq_getX_r (handle, cs, iv,
							     &x);
					GEOSCoordSeq_getY_r (handle, cs, iv,
							     &y);
				    }
				  else
				    {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					GEOSCoordSeq_getX (cs, iv, &x);
					GEOSCoordSeq_getY (cs, iv, &y);
#endif
				    }
				  z = 0.0;
			      }
			    if (dimension_model == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
			      }
			    else if (dimension_model == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (rng->Coords, iv, x, y, 0.0);
			      }
			    else if (dimension_model == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (rng->Coords, iv, x, y, z,
						    0.0);
			      }
			    else
			      {
				  gaiaSetPoint (rng->Coords, iv, x, y);
			      }
			}
		      for (ib = 0; ib < holes; ib++)
			{
			    /* interior rings */
			    if (handle != NULL)
			      {
				  geos_ring =
				      GEOSGetInteriorRingN_r (handle, geos_item,
							      ib);
				  cs = GEOSGeom_getCoordSeq_r (handle,
							       geos_ring);
				  GEOSCoordSeq_getDimensions_r (handle, cs,
								&dims);
				  GEOSCoordSeq_getSize_r (handle, cs, &points);
			      }
			    else
			      {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
				  geos_ring =
				      GEOSGetInteriorRingN (geos_item, ib);
				  cs = GEOSGeom_getCoordSeq (geos_ring);
				  GEOSCoordSeq_getDimensions (cs, &dims);
				  GEOSCoordSeq_getSize (cs, &points);
#endif
			      }
			    rng = gaiaAddInteriorRing (pg, ib, points);
			    for (iv = 0; iv < (int) points; iv++)
			      {
				  if (dims == 3)
				    {
					if (handle != NULL)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   iv, &x);
					      GEOSCoordSeq_getY_r (handle, cs,
								   iv, &y);
					      GEOSCoordSeq_getZ_r (handle, cs,
								   iv, &z);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_getX (cs, iv, &x);
					      GEOSCoordSeq_getY (cs, iv, &y);
					      GEOSCoordSeq_getZ (cs, iv, &z);
#endif
					  }
				    }
				  else
				    {
					if (handle != NULL)
					  {
					      GEOSCoordSeq_getX_r (handle, cs,
								   iv, &x);
					      GEOSCoordSeq_getY_r (handle, cs,
								   iv, &y);
					  }
					else
					  {
#ifndef GEOS_USE_ONLY_R_API	/* obsolete versions non fully thread-safe */
					      GEOSCoordSeq_getX (cs, iv, &x);
					      GEOSCoordSeq_getY (cs, iv, &y);
#endif
					  }
					z = 0.0;
				    }
				  if (dimension_model == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (rng->Coords, iv, x, y,
							 z);
				    }
				  else if (dimension_model == GAIA_XY_M)
				    {
					gaiaSetPointXYM (rng->Coords, iv, x, y,
							 0.0);
				    }
				  else if (dimension_model == GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (rng->Coords, iv, x, y,
							  z, 0.0);
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
      };
    return gaia;
}

GAIAGEO_DECLARE void *
gaiaToGeos (const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry */
    return toGeosGeometry (NULL, NULL, gaia, GAIA2GEOS_ALL);
}

GAIAGEO_DECLARE void *
gaiaToGeos_r (const void *p_cache, const gaiaGeomCollPtr gaia)
{
/* converting a GAIA Geometry into a GEOS Geometry */
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
    return toGeosGeometry (cache, handle, gaia, GAIA2GEOS_ALL);
}

GAIAGEO_DECLARE void *
gaiaToGeosSelective (const gaiaGeomCollPtr gaia, int mode)
{
/* converting a GAIA Geometry into a GEOS Geometry (selected type) */
    if (mode == GAIA2GEOS_ONLY_POINTS || mode == GAIA2GEOS_ONLY_LINESTRINGS
	|| mode == GAIA2GEOS_ONLY_POLYGONS)
	;
    else
	mode = GAIA2GEOS_ALL;
    return toGeosGeometry (NULL, NULL, gaia, mode);
}

GAIAGEO_DECLARE void *
gaiaToGeosSelective_r (const void *p_cache, const gaiaGeomCollPtr gaia,
		       int mode)
{
/* converting a GAIA Geometry into a GEOS Geometry (selected type) */
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
    if (mode == GAIA2GEOS_ONLY_POINTS || mode == GAIA2GEOS_ONLY_LINESTRINGS
	|| mode == GAIA2GEOS_ONLY_POLYGONS)
	;
    else
	mode = GAIA2GEOS_ALL;
    return toGeosGeometry (cache, handle, gaia, mode);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XY (const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XY] */
    const GEOSGeometry *geos = xgeos;
    return fromGeosGeometry (NULL, geos, GAIA_XY);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYZ (const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYZ] */
    const GEOSGeometry *geos = xgeos;
    return fromGeosGeometry (NULL, geos, GAIA_XY_Z);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYM (const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYM] */
    const GEOSGeometry *geos = xgeos;
    return fromGeosGeometry (NULL, geos, GAIA_XY_M);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYZM (const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYZM] */
    const GEOSGeometry *geos = xgeos;
    return fromGeosGeometry (NULL, geos, GAIA_XY_Z_M);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XY_r (const void *p_cache, const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XY] */
    const GEOSGeometry *geos = xgeos;
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
    return fromGeosGeometry (handle, geos, GAIA_XY);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYZ_r (const void *p_cache, const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYZ] */
    const GEOSGeometry *geos = xgeos;
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
    return fromGeosGeometry (handle, geos, GAIA_XY_Z);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYM_r (const void *p_cache, const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYM] */
    const GEOSGeometry *geos = xgeos;
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
    return fromGeosGeometry (handle, geos, GAIA_XY_M);
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaFromGeos_XYZM_r (const void *p_cache, const void *xgeos)
{
/* converting a GEOS Geometry into a GAIA Geometry [XYZM] */
    const GEOSGeometry *geos = xgeos;
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
    return fromGeosGeometry (handle, geos, GAIA_XY_Z_M);
}

#endif /* end including GEOS */
