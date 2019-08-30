/*

 gg_advanced.c -- Gaia advanced geometric operations
  
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
#include <math.h>
#include <float.h>
#include <string.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>

#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiageo.h>
#include <spatialite/debug.h>

GAIAGEO_DECLARE double
gaiaMeasureLength (int dims, double *coords, int vert)
{
/* computes the total length */
    double lung = 0.0;
    double xx1;
    double xx2;
    double yy1;
    double yy2;
    double x;
    double y;
    double z;
    double m;
    double dist;
    int ind;
    if (vert <= 0)
	return lung;
    if (dims == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (coords, 0, &xx1, &yy1, &z);
      }
    else if (dims == GAIA_XY_M)
      {
	  gaiaGetPointXYM (coords, 0, &xx1, &yy1, &m);
      }
    else if (dims == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (coords, 0, &xx1, &yy1, &z, &m);
      }
    else
      {
	  gaiaGetPoint (coords, 0, &xx1, &yy1);
      }
    for (ind = 1; ind < vert; ind++)
      {
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, ind, &xx2, &yy2, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, ind, &xx2, &yy2, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, ind, &xx2, &yy2, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, ind, &xx2, &yy2);
	    }
	  x = xx1 - xx2;
	  y = yy1 - yy2;
	  dist = sqrt ((x * x) + (y * y));
	  lung += dist;
	  xx1 = xx2;
	  yy1 = yy2;
      }
    return lung;
}

GAIAGEO_DECLARE double
gaiaMeasureArea (gaiaRingPtr ring)
{
/* computes the area */
    int iv;
    double xx;
    double yy;
    double x;
    double y;
    double z;
    double m;
    double area = 0.0;
    if (!ring)
	return 0.0;
    if (ring->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ring->Coords, 0, &xx, &yy, &z);
      }
    else if (ring->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ring->Coords, 0, &xx, &yy, &m);
      }
    else if (ring->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ring->Coords, 0, &xx, &yy, &z, &m);
      }
    else
      {
	  gaiaGetPoint (ring->Coords, 0, &xx, &yy);
      }
    for (iv = 1; iv < ring->Points; iv++)
      {
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, iv, &x, &y);
	    }
	  area += ((xx * y) - (x * yy));
	  xx = x;
	  yy = y;
      }
    area /= 2.0;
    return fabs (area);
}

GAIAGEO_DECLARE void
gaiaRingCentroid (gaiaRingPtr ring, double *rx, double *ry)
{
/* computes the simple ring centroid */
    double cx = 0.0;
    double cy = 0.0;
    double xx;
    double yy;
    double x;
    double y;
    double z;
    double m;
    double coeff;
    double area;
    double term;
    int iv;
    if (!ring)
      {
	  *rx = -DBL_MAX;
	  *ry = -DBL_MAX;
	  return;
      }
    area = gaiaMeasureArea (ring);
    coeff = 1.0 / (area * 6.0);
    if (ring->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (ring->Coords, 0, &xx, &yy, &z);
      }
    else if (ring->DimensionModel == GAIA_XY_M)
      {
	  gaiaGetPointXYM (ring->Coords, 0, &xx, &yy, &m);
      }
    else if (ring->DimensionModel == GAIA_XY_Z_M)
      {
	  gaiaGetPointXYZM (ring->Coords, 0, &xx, &yy, &z, &m);
      }
    else
      {
	  gaiaGetPoint (ring->Coords, 0, &xx, &yy);
      }
    for (iv = 1; iv < ring->Points; iv++)
      {
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, iv, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, iv, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, iv, &x, &y);
	    }
	  term = (xx * y) - (x * yy);
	  cx += (xx + x) * term;
	  cy += (yy + y) * term;
	  xx = x;
	  yy = y;
      }
    *rx = fabs (cx * coeff);
    *ry = fabs (cy * coeff);
}

GAIAGEO_DECLARE void
gaiaClockwise (gaiaRingPtr p)
{
/* determines clockwise or anticlockwise direction */
    int ind;
    int ix;
    double xx;
    double yy;
    double x;
    double y;
    double z;
    double m;
    double area = 0.0;
    for (ind = 0; ind < p->Points; ind++)
      {
	  if (p->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (p->Coords, ind, &xx, &yy, &z);
	    }
	  else if (p->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (p->Coords, ind, &xx, &yy, &m);
	    }
	  else if (p->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (p->Coords, ind, &xx, &yy, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (p->Coords, ind, &xx, &yy);
	    }
	  ix = (ind + 1) % p->Points;
	  if (p->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (p->Coords, ix, &x, &y, &z);
	    }
	  else if (p->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (p->Coords, ix, &x, &y, &m);
	    }
	  else if (p->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (p->Coords, ix, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (p->Coords, ix, &x, &y);
	    }
	  area += ((xx * y) - (x * yy));
      }
    area /= 2.0;
    if (area >= 0.0)
	p->Clockwise = 0;
    else
	p->Clockwise = 1;
}

GAIAGEO_DECLARE int
gaiaIsPointOnRingSurface (gaiaRingPtr ring, double pt_x, double pt_y)
{
/* tests if a POINT falls inside a RING */
    int isInternal = 0;
    int cnt;
    int i;
    int j;
    double x;
    double y;
    double z;
    double m;
    double *vert_x;
    double *vert_y;
    double minx = DBL_MAX;
    double miny = DBL_MAX;
    double maxx = -DBL_MAX;
    double maxy = -DBL_MAX;
    cnt = ring->Points;
    cnt--;			/* ignoring last vertex because surely identical to the first one */
    if (cnt < 2)
	return 0;
/* allocating and loading an array of vertices */
    vert_x = malloc (sizeof (double) * (cnt));
    vert_y = malloc (sizeof (double) * (cnt));
    for (i = 0; i < cnt; i++)
      {
	  if (ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ring->Coords, i, &x, &y, &z);
	    }
	  else if (ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ring->Coords, i, &x, &y, &m);
	    }
	  else if (ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ring->Coords, i, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ring->Coords, i, &x, &y);
	    }
	  vert_x[i] = x;
	  vert_y[i] = y;
	  if (x < minx)
	      minx = x;
	  if (x > maxx)
	      maxx = x;
	  if (y < miny)
	      miny = y;
	  if (y > maxy)
	      maxy = y;
      }
    if (pt_x < minx || pt_x > maxx)
	goto end;		/* outside the bounding box (x axis) */
    if (pt_y < miny || pt_y > maxy)
	goto end;		/* outside the bounding box (y axis) */
    for (i = 0, j = cnt - 1; i < cnt; j = i++)
      {
/* The definitive reference is "Point in Polyon Strategies" by
/  Eric Haines [Gems IV]  pp. 24-46.
/  The code in the Sedgewick book Algorithms (2nd Edition, p.354) is 
/  incorrect.
*/
	  if ((((vert_y[i] <= pt_y) && (pt_y < vert_y[j]))
	       || ((vert_y[j] <= pt_y) && (pt_y < vert_y[i])))
	      && (pt_x <
		  (vert_x[j] - vert_x[i]) * (pt_y - vert_y[i]) / (vert_y[j] -
								  vert_y[i]) +
		  vert_x[i]))
	      isInternal = !isInternal;
      }
  end:
    free (vert_x);
    free (vert_y);
    return isInternal;
}

GAIAGEO_DECLARE double
gaiaMinDistance (double x0, double y0, int dims, double *coords, int n_vert)
{
/* computing minimal distance between a POINT and a linestring/ring */
    double x;
    double y;
    double z;
    double m;
    double ox;
    double oy;
    double lineMag;
    double u;
    double px;
    double py;
    double dist;
    double min_dist = DBL_MAX;
    int iv;
    if (n_vert < 2)
	return min_dist;	/* not a valid linestring */
/* computing distance from first vertex */
    ox = *(coords + 0);
    oy = *(coords + 1);
    min_dist = sqrt (((x0 - ox) * (x0 - ox)) + ((y0 - oy) * (y0 - oy)));
    for (iv = 1; iv < n_vert; iv++)
      {
	  /* segment start-end coordinates */
	  if (dims == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (coords, iv - 1, &ox, &oy, &z);
		gaiaGetPointXYZ (coords, iv, &x, &y, &z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaGetPointXYM (coords, iv - 1, &ox, &oy, &m);
		gaiaGetPointXYM (coords, iv, &x, &y, &m);
	    }
	  else if (dims == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (coords, iv - 1, &ox, &oy, &z, &m);
		gaiaGetPointXYZM (coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (coords, iv - 1, &ox, &oy);
		gaiaGetPoint (coords, iv, &x, &y);
	    }
	  /* computing distance from vertex */
	  dist = sqrt (((x0 - x) * (x0 - x)) + ((y0 - y) * (y0 - y)));
	  if (dist < min_dist)
	      min_dist = dist;
	  /* computing a projection */
	  lineMag = ((x - ox) * (x - ox)) + ((y - oy) * (y - oy));
	  u = (((x0 - ox) * (x - ox)) + ((y0 - oy) * (y - oy))) / lineMag;
	  if (u < 0.0 || u > 1.0)
	      ;			/* closest point does not fall within the line segment */
	  else
	    {
		px = ox + u * (x - ox);
		py = oy + u * (y - oy);
		dist = sqrt (((x0 - px) * (x0 - px)) + ((y0 - py) * (y0 - py)));
		if (dist < min_dist)
		    min_dist = dist;
	    }
      }
    return min_dist;
}

GAIAGEO_DECLARE int
gaiaIsPointOnPolygonSurface (gaiaPolygonPtr polyg, double x, double y)
{
/* tests if a POINT falls inside a POLYGON */
    int ib;
    gaiaRingPtr ring = polyg->Exterior;
    if (gaiaIsPointOnRingSurface (ring, x, y))
      {
	  /* ok, the POINT falls inside the polygon */
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		ring = polyg->Interiors + ib;
		if (gaiaIsPointOnRingSurface (ring, x, y))
		  {
		      /* no, the POINT fall inside some hole */
		      return 0;
		  }
	    }
	  return 1;
      }
    return 0;
}

GAIAGEO_DECLARE int
gaiaIntersect (double *x0, double *y0, double x1, double y1, double x2,
	       double y2, double x3, double y3, double x4, double y4)
{
/* computes intersection [if any] between two line segments
/  the intersection POINT has coordinates (x0, y0) 
/  first line is identified by(x1, y1)  and (x2, y2)
/  second line is identified by (x3, y3) and (x4, y4)
*/
    double x;
    double y;
    double a1;
    double b1;
    double c1;
    double a2;
    double b2;
    double c2;
    double m1;
    double m2;
    double p;
    double det_inv;
    double minx1;
    double miny1;
    double maxx1;
    double maxy1;
    double minx2;
    double miny2;
    double maxx2;
    double maxy2;
    int ok1 = 0;
    int ok2 = 0;
/* building line segment's MBRs */
    if (x2 < x1)
      {
	  minx1 = x2;
	  maxx1 = x1;
      }
    else
      {
	  minx1 = x1;
	  maxx1 = x2;
      }
    if (y2 < y1)
      {
	  miny1 = y2;
	  maxy1 = y1;
      }
    else
      {
	  miny1 = y1;
	  maxy1 = y2;
      }
    if (x4 < x3)
      {
	  minx2 = x4;
	  maxx2 = x3;
      }
    else
      {
	  minx2 = x3;
	  maxx2 = x4;
      }
    if (y4 < y3)
      {
	  miny2 = y4;
	  maxy2 = y3;
      }
    else
      {
	  miny2 = y3;
	  maxy2 = y4;
      }
/* checking MBRs first */
    if (minx1 > maxx2)
	return 0;
    if (miny1 > maxy2)
	return 0;
    if (maxx1 < minx2)
	return 0;
    if (maxy1 < miny2)
	return 0;
    if (minx2 > maxx1)
	return 0;
    if (miny2 > maxy1)
	return 0;
    if (maxx2 < minx1)
	return 0;
    if (maxy2 < miny1)
	return 0;
/* there is an MBRs intersection - proceeding */
    if ((x2 - x1) != 0.0)
	m1 = (y2 - y1) / (x2 - x1);
    else
	m1 = DBL_MAX;
    if ((x4 - x3) != 0)
	m2 = (y4 - y3) / (x4 - x3);
    else
	m2 = DBL_MAX;
    if (m1 == m2)		/* parallel lines */
	return 0;
    if (m1 == DBL_MAX)
	c1 = y1;
    else
	c1 = (y1 - m1 * x1);
    if (m2 == DBL_MAX)
	c2 = y3;
    else
	c2 = (y3 - m2 * x3);
    if (m1 == DBL_MAX)
      {
	  x = x1;
	  p = m2 * x1;
	  y = p + c2;		/*  first line is vertical */
	  goto check_bbox;
      }
    if (m2 == DBL_MAX)
      {
	  x = x3;
	  p = m1 * x3;
	  y = p + c1;		/* second line is vertical */
	  goto check_bbox;
      }
    a1 = m1;
    a2 = m2;
    b1 = -1;
    b2 = -1;
    det_inv = 1 / (a1 * b2 - a2 * b1);
    x = ((b1 * c2 - b2 * c1) * det_inv);
    y = ((a2 * c1 - a1 * c2) * det_inv);
/* now checking if intersection falls within both segment boundaries */
  check_bbox:
    if (x >= minx1 && x <= maxx1 && y >= miny1 && y <= maxy1)
	ok1 = 1;
    if (x >= minx2 && x <= maxx2 && y >= miny2 && y <= maxy2)
	ok2 = 1;
    if (ok1 && ok2)
      {
	  /* intersection point falls within the segments */
	  *x0 = x;
	  *y0 = y;
	  return 1;
      }
    return 0;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaSanitize (gaiaGeomCollPtr geom)
{
/* 
/ sanitizes a GEOMETRYCOLLECTION:
/ - repeated vertices are omitted
/ - ring closure is enforced anyway  
*/
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    int points;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaLinestringPtr new_line;
    gaiaPolygonPtr polyg;
    gaiaPolygonPtr new_polyg;
    gaiaGeomCollPtr new_geom;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    if (!geom)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	new_geom = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	new_geom = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	new_geom = gaiaAllocGeomCollXYZM ();
    else
	new_geom = gaiaAllocGeomColl ();
    new_geom->Srid = geom->Srid;
    new_geom->DeclaredType = geom->DeclaredType;
    point = geom->FirstPoint;
    while (point)
      {
	  /* copying POINTs */
	  gaiaAddPointToGeomCollXYZM (new_geom, point->X, point->Y, point->Z,
				      point->M);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* sanitizing LINESTRINGs */
	  points = 0;
	  for (iv = 0; iv < line->Points; iv++)
	    {
		/* PASS I - checking points */
		z = 0.0;
		m = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (last_x == x && last_y == y && last_z == z)
			  ;
		      else
			  points++;
		  }
		else
		    points++;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
	  if (points < 2)
	    {
		/* illegal LINESTRING - copying the original one */
		new_line = gaiaAddLinestringToGeomColl (new_geom, line->Points);
		gaiaCopyLinestringCoords (new_line, line);
	    }
	  else
	    {
		/* valid LINESTRING - sanitizing */
		new_line = gaiaAddLinestringToGeomColl (new_geom, points);
		points = 0;
		for (iv = 0; iv < line->Points; iv++)
		  {
		      /* PASS II - inserting points */
		      z = 0.0;
		      m = 0.0;
		      if (line->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
			}
		      else if (line->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
			}
		      else if (line->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (line->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    if (last_x == x && last_y == y && last_z == z)
				;
			    else
			      {
				  if (new_line->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (new_line->Coords,
							 points, x, y, z);
				    }
				  else if (new_line->DimensionModel ==
					   GAIA_XY_M)
				    {
					gaiaSetPointXYM (new_line->Coords,
							 points, x, y, m);
				    }
				  else if (new_line->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (new_line->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (new_line->Coords, points,
						      x, y);
				    }
				  points++;
			      }
			}
		      else
			{
			    if (new_line->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (new_line->Coords, points, x,
						   y, z);
			      }
			    else if (new_line->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (new_line->Coords, points, x,
						   y, m);
			      }
			    else if (new_line->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (new_line->Coords, points, x,
						    y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (new_line->Coords, points, x, y);
			      }
			    points++;
			}
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* copying POLYGONs */
	  i_ring = polyg->Exterior;
	  /* sanitizing EXTERIOR RING */
	  points = 0;
	  for (iv = 0; iv < i_ring->Points; iv++)
	    {
		/* PASS I - checking points */
		z = 0.0;
		m = 0.0;
		if (i_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (i_ring->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (last_x == x && last_y == y && last_z == z)
			  ;
		      else
			  points++;
		  }
		else
		    points++;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
	  if (last_x == x && last_y == y && last_z == z)
	      ;
	  else
	    {
		/* forcing RING closure */
		points++;
	    }
	  if (points < 4)
	    {
		/* illegal RING - copying the original one */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, i_ring->Points,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		gaiaCopyRingCoords (o_ring, i_ring);
	    }
	  else
	    {
		/* valid RING - sanitizing */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, points,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		points = 0;
		for (iv = 0; iv < i_ring->Points; iv++)
		  {
		      /* PASS II - inserting points */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    if (last_x == x && last_y == y && last_z == z)
				;
			    else
			      {
				  if (o_ring->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (o_ring->Coords, points,
							 x, y, z);
				    }
				  else if (o_ring->DimensionModel == GAIA_XY_M)
				    {
					gaiaSetPointXYM (o_ring->Coords, points,
							 x, y, m);
				    }
				  else if (o_ring->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (o_ring->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (o_ring->Coords, points, x,
						      y);
				    }
				  points++;
			      }
			}
		      else
			{
			    if (o_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (o_ring->Coords, points, x,
						   y, z);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (o_ring->Coords, points, x,
						   y, m);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (o_ring->Coords, points, x,
						    y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (o_ring->Coords, points, x, y);
			      }
			    points++;
			}
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
	    }
	  /* PASS III - forcing RING closure */
	  z = 0.0;
	  m = 0.0;
	  if (i_ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (i_ring->Coords, 0, &x, &y);
	    }
	  points = o_ring->Points - 1;
	  if (o_ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (o_ring->Coords, points, x, y, z);
	    }
	  else if (o_ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (o_ring->Coords, points, x, y, m);
	    }
	  else if (o_ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (o_ring->Coords, points, x, y, z, m);
	    }
	  else
	    {
		gaiaSetPoint (o_ring->Coords, points, x, y);
	    }
	  for (ib = 0; ib < new_polyg->NumInteriors; ib++)
	    {
		/* copying each INTERIOR RING [if any] */
		i_ring = polyg->Interiors + ib;
		/* sanitizing an INTERIOR RING */
		points = 0;
		for (iv = 0; iv < i_ring->Points; iv++)
		  {
		      /* PASS I - checking points */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    if (last_x == x && last_y == y && last_z == z)
				;
			    else
				points++;
			}
		      else
			  points++;
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
		if (last_x == x && last_y == y && last_z == z)
		    ;
		else
		  {
		      /* forcing RING closure */
		      points++;
		  }
		if (points < 4)
		  {
		      /* illegal RING - copying the original one */
		      o_ring =
			  gaiaAddInteriorRing (new_polyg, ib, i_ring->Points);
		      gaiaCopyRingCoords (o_ring, i_ring);
		  }
		else
		  {
		      /* valid RING - sanitizing */
		      o_ring = gaiaAddInteriorRing (new_polyg, ib, points);
		      points = 0;
		      for (iv = 0; iv < i_ring->Points; iv++)
			{
			    /* PASS II - inserting points */
			    z = 0.0;
			    m = 0.0;
			    if (i_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y,
						   &z);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (i_ring->Coords, iv, &x, &y,
						   &m);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			      }
			    if (iv > 0)
			      {
				  if (last_x == x && last_y == y && last_z == z)
				      ;
				  else
				    {
					if (o_ring->DimensionModel == GAIA_XY_Z)
					  {
					      gaiaSetPointXYZ (o_ring->Coords,
							       points, x, y, z);
					  }
					else if (o_ring->DimensionModel ==
						 GAIA_XY_M)
					  {
					      gaiaSetPointXYM (o_ring->Coords,
							       points, x, y, m);
					  }
					else if (o_ring->DimensionModel ==
						 GAIA_XY_Z_M)
					  {
					      gaiaSetPointXYZM (o_ring->Coords,
								points, x, y, z,
								m);
					  }
					else
					  {
					      gaiaSetPoint (o_ring->Coords,
							    points, x, y);
					  }
					points++;
				    }
			      }
			    else
			      {
				  if (o_ring->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (o_ring->Coords, points,
							 x, y, z);
				    }
				  else if (o_ring->DimensionModel == GAIA_XY_M)
				    {
					gaiaSetPointXYM (o_ring->Coords, points,
							 x, y, m);
				    }
				  else if (o_ring->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (o_ring->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (o_ring->Coords, points, x,
						      y);
				    }
				  points++;
			      }
			    last_x = x;
			    last_y = y;
			    last_z = z;
			}
		      /* PASS III - forcing RING closure */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, 0, &x, &y);
			}
		      points = o_ring->Points - 1;
		      if (o_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (o_ring->Coords, points, x, y, z);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (o_ring->Coords, points, x, y, m);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (o_ring->Coords, points, x, y, z,
					      m);
			}
		      else
			{
			    gaiaSetPoint (o_ring->Coords, points, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    return new_geom;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaEnsureClosedRings (gaiaGeomCollPtr geom)
{
/* 
/ sanitizes a GEOMETRYCOLLECTION:
/ - ring closure is enforced anyway  
*/
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    int points;
    int end_point;
    int enforce;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaLinestringPtr new_line;
    gaiaPolygonPtr polyg;
    gaiaPolygonPtr new_polyg;
    gaiaGeomCollPtr new_geom;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    if (!geom)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	new_geom = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	new_geom = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	new_geom = gaiaAllocGeomCollXYZM ();
    else
	new_geom = gaiaAllocGeomColl ();
    new_geom->Srid = geom->Srid;
    new_geom->DeclaredType = geom->DeclaredType;
    point = geom->FirstPoint;
    while (point)
      {
	  /* copying POINTs */
	  gaiaAddPointToGeomCollXYZM (new_geom, point->X, point->Y, point->Z,
				      point->M);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* copying LINESTRINGs */
	  new_line = gaiaAddLinestringToGeomColl (new_geom, line->Points);
	  gaiaCopyLinestringCoords (new_line, line);
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* copying POLYGONs */
	  i_ring = polyg->Exterior;
	  /* sanitizing EXTERIOR RING */
	  points = i_ring->Points;
	  z = 0.0;
	  m = 0.0;
	  if (i_ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (i_ring->Coords, 0, &x, &y);
	    }
	  last_z = 0.0;
	  m = 0.0;
	  end_point = points - 1;
	  if (i_ring->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (i_ring->Coords, end_point, &last_x, &last_y,
				 &last_z);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (i_ring->Coords, end_point, &last_x, &last_y,
				 &m);
	    }
	  else if (i_ring->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (i_ring->Coords, end_point, &last_x, &last_y,
				  &last_z, &m);
	    }
	  else
	    {
		gaiaGetPoint (i_ring->Coords, end_point, &last_x, &last_y);
	    }
	  if (last_x == x && last_y == y && last_z == z)
	      enforce = 0;
	  else
	    {
		/* forcing RING closure */
		enforce = 1;
	    }
	  if (!enforce)
	    {
		/* already closed */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, i_ring->Points,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		gaiaCopyRingCoords (o_ring, i_ring);
	    }
	  else
	    {
		/* forcing closure */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, i_ring->Points + 1,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		for (iv = 0; iv < i_ring->Points; iv++)
		  {
		      /* inserting points */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			}
		      if (o_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (o_ring->Coords, iv, x, y, z);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (o_ring->Coords, iv, x, y, m);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (o_ring->Coords, iv, x, y, z, m);
			}
		      else
			{
			    gaiaSetPoint (o_ring->Coords, iv, x, y);
			}
		  }
		/* adding last vertex so to ensure closure */
		z = 0.0;
		m = 0.0;
		if (i_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (i_ring->Coords, 0, &x, &y);
		  }
		end_point = o_ring->Points - 1;
		if (o_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (o_ring->Coords, end_point, x, y, z);
		  }
		else if (o_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaSetPointXYM (o_ring->Coords, end_point, x, y, m);
		  }
		else if (o_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaSetPointXYZM (o_ring->Coords, end_point, x, y, z, m);
		  }
		else
		  {
		      gaiaSetPoint (o_ring->Coords, end_point, x, y);
		  }
	    }

	  for (ib = 0; ib < new_polyg->NumInteriors; ib++)
	    {
		/* copying each INTERIOR RING [if any] */
		i_ring = polyg->Interiors + ib;
		points = i_ring->Points;
		z = 0.0;
		m = 0.0;
		if (i_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (i_ring->Coords, 0, &x, &y);
		  }
		last_z = 0.0;
		m = 0.0;
		end_point = points - 1;
		if (i_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (i_ring->Coords, end_point, &last_x,
				       &last_y, &last_z);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (i_ring->Coords, end_point, &last_x,
				       &last_y, &m);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (i_ring->Coords, end_point, &last_x,
					&last_y, &last_z, &m);
		  }
		else
		  {
		      gaiaGetPoint (i_ring->Coords, end_point, &last_x,
				    &last_y);
		  }
		last_z = z;
		if (last_x == x && last_y == y && last_z == z)
		    enforce = 0;
		else
		  {
		      /* forcing RING closure */
		      enforce = 1;
		  }
		if (!enforce)
		  {
		      /* already closed */
		      o_ring =
			  gaiaAddInteriorRing (new_polyg, ib, i_ring->Points);
		      gaiaCopyRingCoords (o_ring, i_ring);
		  }
		else
		  {
		      /* forcing closure */
		      o_ring =
			  gaiaAddInteriorRing (new_polyg, ib,
					       i_ring->Points + 1);
		      for (iv = 0; iv < i_ring->Points; iv++)
			{
			    /* inserting points */
			    z = 0.0;
			    m = 0.0;
			    if (i_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y,
						   &z);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (i_ring->Coords, iv, &x, &y,
						   &m);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			      }
			    if (o_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (o_ring->Coords, iv, x, y, z);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (o_ring->Coords, iv, x, y, m);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (o_ring->Coords,
						    iv, x, y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (o_ring->Coords, iv, x, y);
			      }
			}
		      /* adding last vertex so to ensure closure */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, 0, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, 0, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, 0, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, 0, &x, &y);
			}
		      end_point = o_ring->Points - 1;
		      if (o_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaSetPointXYZ (o_ring->Coords, end_point, x, y,
					     z);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaSetPointXYM (o_ring->Coords, end_point, x, y,
					     m);
			}
		      else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaSetPointXYZM (o_ring->Coords, end_point, x, y,
					      z, m);
			}
		      else
			{
			    gaiaSetPoint (o_ring->Coords, end_point, x, y);
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    return new_geom;
}

static double
point_point_distance (double x1, double y1, double x2, double y2)
{
    return sqrt (((x1 - x2) * (x1 - x2)) + ((y1 - y2) * (y1 - y2)));
}

static int
repeated_matching_point (gaiaGeomCollPtr geom, double x, double y, double z,
			 double tolerance)
{
/* checking for duplicate points - multipoint */
    gaiaPointPtr point;
    point = geom->FirstPoint;
    while (point)
      {
	  if (tolerance <= 0.0)
	    {
		if (point->X == x && point->Y == y && point->Z == z)
		    return 1;
	    }
	  else
	    {
		if (point_point_distance (x, y, point->X, point->Y) <=
		    tolerance)
		    return 1;
	    }
	  point = point->Next;
      }
    return 0;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaRemoveRepeatedPoints (gaiaGeomCollPtr geom, double tolerance)
{
/* 
/ sanitizes a GEOMETRYCOLLECTION:
/ - repeated vertices are omitted 
*/
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    double last_x = 0.0;
    double last_y = 0.0;
    double last_z = 0.0;
    int points;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaLinestringPtr new_line;
    gaiaPolygonPtr polyg;
    gaiaPolygonPtr new_polyg;
    gaiaGeomCollPtr new_geom;
    gaiaRingPtr i_ring;
    gaiaRingPtr o_ring;
    if (!geom)
	return NULL;
    if (geom->DimensionModel == GAIA_XY_Z)
	new_geom = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	new_geom = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	new_geom = gaiaAllocGeomCollXYZM ();
    else
	new_geom = gaiaAllocGeomColl ();
    new_geom->Srid = geom->Srid;
    new_geom->DeclaredType = geom->DeclaredType;
    point = geom->FirstPoint;
    while (point)
      {
	  /* copying POINTs */
	  if (!repeated_matching_point
	      (new_geom, point->X, point->Y, point->Z, tolerance))
	      gaiaAddPointToGeomCollXYZM (new_geom, point->X, point->Y,
					  point->Z, point->M);
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* sanitizing LINESTRINGs */
	  points = 0;
	  for (iv = 0; iv < line->Points; iv++)
	    {
		/* PASS I - checking points */
		z = 0.0;
		m = 0.0;
		if (line->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
		  }
		else if (line->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
		  }
		else if (line->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (line->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (tolerance <= 0.0)
			{
			    if (last_x == x && last_y == y && last_z == z)
				;
			    else
				points++;
			}
		      else
			{
			    if (point_point_distance (x, y, last_x, last_y) <=
				tolerance)
				;
			    else
				points++;
			}
		  }
		else
		    points++;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
	  if (points < 2)
	    {
		/* illegal LINESTRING - copying the original one */
		new_line = gaiaAddLinestringToGeomColl (new_geom, line->Points);
		gaiaCopyLinestringCoords (new_line, line);
	    }
	  else
	    {
		/* valid LINESTRING - sanitizing */
		new_line = gaiaAddLinestringToGeomColl (new_geom, points);
		points = 0;
		for (iv = 0; iv < line->Points; iv++)
		  {
		      /* PASS II - inserting points */
		      z = 0.0;
		      m = 0.0;
		      if (line->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (line->Coords, iv, &x, &y, &z);
			}
		      else if (line->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (line->Coords, iv, &x, &y, &m);
			}
		      else if (line->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (line->Coords, iv, &x, &y, &z, &m);
			}
		      else
			{
			    gaiaGetPoint (line->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    int skip = 0;
			    if (tolerance <= 0.0)
			      {
				  if (last_x == x && last_y == y && last_z == z)
				      skip = 1;
			      }
			    else
			      {
				  if (point_point_distance
				      (x, y, last_x, last_y) <= tolerance)
				      skip = 1;
			      }
			    if (skip)
				;
			    else
			      {
				  if (new_line->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (new_line->Coords,
							 points, x, y, z);
				    }
				  else if (new_line->DimensionModel ==
					   GAIA_XY_M)
				    {
					gaiaSetPointXYM (new_line->Coords,
							 points, x, y, m);
				    }
				  else if (new_line->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (new_line->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (new_line->Coords, points,
						      x, y);
				    }
				  points++;
			      }
			}
		      else
			{
			    if (new_line->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (new_line->Coords, points, x,
						   y, z);
			      }
			    else if (new_line->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (new_line->Coords, points, x,
						   y, m);
			      }
			    else if (new_line->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (new_line->Coords, points, x,
						    y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (new_line->Coords, points, x, y);
			      }
			    points++;
			}
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* copying POLYGONs */
	  i_ring = polyg->Exterior;
	  /* sanitizing EXTERIOR RING */
	  points = 0;
	  for (iv = 0; iv < i_ring->Points; iv++)
	    {
		/* PASS I - checking points */
		z = 0.0;
		m = 0.0;
		if (i_ring->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
		  }
		else if (i_ring->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (i_ring->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (tolerance <= 0.0)
			{
			    if (last_x == x && last_y == y && last_z == z)
				;
			    else
				points++;
			}
		      else
			{
			    if (point_point_distance (x, y, last_x, last_y) <=
				tolerance)
				;
			    else
				points++;
			}
		  }
		else
		    points++;
		last_x = x;
		last_y = y;
		last_z = z;
	    }
	  if (points < 4)
	    {
		/* illegal RING - copying the original one */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, i_ring->Points,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		gaiaCopyRingCoords (o_ring, i_ring);
	    }
	  else
	    {
		/* valid RING - sanitizing */
		new_polyg =
		    gaiaAddPolygonToGeomColl (new_geom, points,
					      polyg->NumInteriors);
		o_ring = new_polyg->Exterior;
		points = 0;
		for (iv = 0; iv < i_ring->Points; iv++)
		  {
		      /* PASS II - inserting points */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    int skip = 0;
			    if (tolerance <= 0.0)
			      {
				  if (last_x == x && last_y == y && last_z == z)
				      skip = 1;
			      }
			    else
			      {
				  if (point_point_distance
				      (x, y, last_x, last_y) <= tolerance)
				      skip = 1;
			      }
			    if (skip)
				;
			    else
			      {
				  if (o_ring->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (o_ring->Coords, points,
							 x, y, z);
				    }
				  else if (o_ring->DimensionModel == GAIA_XY_M)
				    {
					gaiaSetPointXYM (o_ring->Coords, points,
							 x, y, m);
				    }
				  else if (o_ring->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (o_ring->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (o_ring->Coords, points, x,
						      y);
				    }
				  points++;
			      }
			}
		      else
			{
			    if (o_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaSetPointXYZ (o_ring->Coords, points, x,
						   y, z);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaSetPointXYM (o_ring->Coords, points, x,
						   y, m);
			      }
			    else if (o_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaSetPointXYZM (o_ring->Coords, points, x,
						    y, z, m);
			      }
			    else
			      {
				  gaiaSetPoint (o_ring->Coords, points, x, y);
			      }
			    points++;
			}
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
	    }

	  for (ib = 0; ib < new_polyg->NumInteriors; ib++)
	    {
		/* copying each INTERIOR RING [if any] */
		i_ring = polyg->Interiors + ib;
		/* sanitizing an INTERIOR RING */
		points = 0;
		for (iv = 0; iv < i_ring->Points; iv++)
		  {
		      /* PASS I - checking points */
		      z = 0.0;
		      m = 0.0;
		      if (i_ring->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y, &z);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (i_ring->Coords, iv, &x, &y, &m);
			}
		      else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y, &z,
					      &m);
			}
		      else
			{
			    gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    if (tolerance <= 0.0)
			      {
				  if (last_x == x && last_y == y && last_z == z)
				      ;
				  else
				      points++;
			      }
			    else
			      {
				  if (point_point_distance
				      (x, y, last_x, last_y) <= tolerance)
				      ;
				  else
				      points++;
			      }
			}
		      else
			  points++;
		      last_x = x;
		      last_y = y;
		      last_z = z;
		  }
		if (points < 4)
		  {
		      /* illegal RING - copying the original one */
		      o_ring =
			  gaiaAddInteriorRing (new_polyg, ib, i_ring->Points);
		      gaiaCopyRingCoords (o_ring, i_ring);
		  }
		else
		  {
		      /* valid RING - sanitizing */
		      o_ring = gaiaAddInteriorRing (new_polyg, ib, points);
		      points = 0;
		      for (iv = 0; iv < i_ring->Points; iv++)
			{
			    /* PASS II - inserting points */
			    z = 0.0;
			    m = 0.0;
			    if (i_ring->DimensionModel == GAIA_XY_Z)
			      {
				  gaiaGetPointXYZ (i_ring->Coords, iv, &x, &y,
						   &z);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_M)
			      {
				  gaiaGetPointXYM (i_ring->Coords, iv, &x, &y,
						   &m);
			      }
			    else if (i_ring->DimensionModel == GAIA_XY_Z_M)
			      {
				  gaiaGetPointXYZM (i_ring->Coords, iv, &x, &y,
						    &z, &m);
			      }
			    else
			      {
				  gaiaGetPoint (i_ring->Coords, iv, &x, &y);
			      }
			    if (iv > 0)
			      {
				  int skip = 0;
				  if (tolerance <= 0.0)
				    {
					if (last_x == x && last_y == y
					    && last_z == z)
					    skip = 1;
				    }
				  else
				    {
					if (point_point_distance
					    (x, y, last_x, last_y) <= tolerance)
					    skip = 1;
				    }
				  if (skip)
				      ;
				  else
				    {
					if (o_ring->DimensionModel == GAIA_XY_Z)
					  {
					      gaiaSetPointXYZ (o_ring->Coords,
							       points, x, y, z);
					  }
					else if (o_ring->DimensionModel ==
						 GAIA_XY_M)
					  {
					      gaiaSetPointXYM (o_ring->Coords,
							       points, x, y, m);
					  }
					else if (o_ring->DimensionModel ==
						 GAIA_XY_Z_M)
					  {
					      gaiaSetPointXYZM (o_ring->Coords,
								points, x, y, z,
								m);
					  }
					else
					  {
					      gaiaSetPoint (o_ring->Coords,
							    points, x, y);
					  }
					points++;
				    }
			      }
			    else
			      {
				  if (o_ring->DimensionModel == GAIA_XY_Z)
				    {
					gaiaSetPointXYZ (o_ring->Coords, points,
							 x, y, z);
				    }
				  else if (o_ring->DimensionModel == GAIA_XY_M)
				    {
					gaiaSetPointXYM (o_ring->Coords, points,
							 x, y, m);
				    }
				  else if (o_ring->DimensionModel ==
					   GAIA_XY_Z_M)
				    {
					gaiaSetPointXYZM (o_ring->Coords,
							  points, x, y, z, m);
				    }
				  else
				    {
					gaiaSetPoint (o_ring->Coords, points, x,
						      y);
				    }
				  points++;
			      }
			    last_x = x;
			    last_y = y;
			    last_z = z;
			}
		  }
	    }
	  polyg = polyg->Next;
      }
    return new_geom;
}

static int
gaiaIsToxicLinestring (gaiaLinestringPtr line)
{
/* checking a Linestring */
    if (line->Points < 2)
	return 1;

/* not a valid Linestring, simply a degenerated Point */
    return 0;
}

static int
gaiaIsToxicRing (gaiaRingPtr ring)
{
/* checking a Ring */
    if (ring->Points < 4)
	return 1;
    return 0;
}

GAIAGEO_DECLARE int
gaiaIsToxic (gaiaGeomCollPtr geom)
{
    return gaiaIsToxic_r (NULL, geom);
}

GAIAGEO_DECLARE int
gaiaIsToxic_r (const void *cache, gaiaGeomCollPtr geom)
{
/* 
/ identifying toxic geometries 
/ i.e. geoms making GEOS to crash !!!
*/
    int ib;
    gaiaPointPtr point;
    gaiaLinestringPtr line;
    gaiaPolygonPtr polyg;
    gaiaRingPtr ring;
    if (!geom)
	return 0;
    if (gaiaIsEmpty (geom))
	return 1;
    point = geom->FirstPoint;
    while (point)
      {
	  /* checking POINTs */
	  point = point->Next;
      }
    line = geom->FirstLinestring;
    while (line)
      {
	  /* checking LINESTRINGs */
	  if (gaiaIsToxicLinestring (line))
	    {
		if (cache != NULL)
		    gaiaSetGeosAuxErrorMsg_r
			(cache,
			 "gaiaIsToxic detected a toxic Linestring: < 2 pts");
		else
		    gaiaSetGeosAuxErrorMsg
			("gaiaIsToxic detected a toxic Linestring: < 2 pts");
		return 1;
	    }
	  line = line->Next;
      }
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* checking POLYGONs */
	  ring = polyg->Exterior;
	  if (gaiaIsToxicRing (ring))
	    {
		if (cache != NULL)
		    gaiaSetGeosAuxErrorMsg_r
			(cache, "gaiaIsToxic detected a toxic Ring: < 4 pts");
		else
		    gaiaSetGeosAuxErrorMsg
			("gaiaIsToxic detected a toxic Ring: < 4 pts");
		return 1;
	    }
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		ring = polyg->Interiors + ib;
		if (gaiaIsToxicRing (ring))
		  {
		      if (cache != NULL)
			  gaiaSetGeosAuxErrorMsg_r
			      (cache,
			       "gaiaIsToxic detected a toxic Ring: < 4 pts");
		      else
			  gaiaSetGeosAuxErrorMsg
			      ("gaiaIsToxic detected a toxic Ring: < 4 pts");
		      return 1;
		  }
	    }
	  polyg = polyg->Next;
      }
    return 0;
}

GAIAGEO_DECLARE int
gaiaIsNotClosedRing (gaiaRingPtr ring)
{
    return gaiaIsNotClosedRing_r (NULL, ring);
}

GAIAGEO_DECLARE int
gaiaIsNotClosedRing_r (const void *cache, gaiaRingPtr ring)
{
/* checking a Ring for closure */
    double x0;
    double y0;
    double z0;
    double m0;
    double x1;
    double y1;
    double z1;
    double m1;
    gaiaRingGetPoint (ring, 0, &x0, &y0, &z0, &m0);
    gaiaRingGetPoint (ring, ring->Points - 1, &x1, &y1, &z1, &m1);
    if (x0 == x1 && y0 == y1 && z0 == z1 && m0 == m1)
	return 0;
    else
      {
	  if (cache != NULL)
	      gaiaSetGeosAuxErrorMsg_r (cache,
					"gaia detected a not-closed Ring");
	  else
	      gaiaSetGeosAuxErrorMsg ("gaia detected a not-closed Ring");
	  return 1;
      }
}

GAIAGEO_DECLARE int
gaiaIsNotClosedGeomColl (gaiaGeomCollPtr geom)
{
    return gaiaIsNotClosedGeomColl_r (NULL, geom);
}

GAIAGEO_DECLARE int
gaiaIsNotClosedGeomColl_r (const void *cache, gaiaGeomCollPtr geom)
{
/* 
/ identifying not properly closed Rings 
/ i.e. geoms making GEOS to crash !!!
*/
    int ret;
    int ib;
    gaiaPolygonPtr polyg;
    gaiaRingPtr ring;
    if (!geom)
	return 0;
    polyg = geom->FirstPolygon;
    while (polyg)
      {
	  /* checking POLYGONs */
	  ring = polyg->Exterior;
	  if (cache != NULL)
	      ret = gaiaIsNotClosedRing_r (cache, ring);
	  else
	      ret = gaiaIsNotClosedRing (ring);
	  if (ret)
	      return 1;
	  for (ib = 0; ib < polyg->NumInteriors; ib++)
	    {
		ring = polyg->Interiors + ib;
		if (cache != NULL)
		    ret = gaiaIsNotClosedRing_r (cache, ring);
		else
		    ret = gaiaIsNotClosedRing (ring);
		if (ret)
		    return 1;
	    }
	  polyg = polyg->Next;
      }
    return 0;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaLinearize (gaiaGeomCollPtr geom, int force_multi)
{
/* attempts to rearrange a generic Geometry into a (multi)linestring */
    int pts = 0;
    int lns = 0;
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr new_ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    double z;
    if (!geom)
	return NULL;
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
    if (pts || lns)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;
    if (force_multi)
	result->DeclaredType = GAIA_MULTILINESTRING;

    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* dissolving any POLYGON as simple LINESTRINGs (rings) */
	  rng = pg->Exterior;
	  new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying the EXTERIOR RING as LINESTRING */
		if (geom->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else if (geom->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (geom->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
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
		      /* copying an INTERIOR RING as LINESTRING */
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
			}
		      else if (geom->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
			}
		      else if (geom->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
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
    if (result->FirstLinestring == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDissolveSegments (gaiaGeomCollPtr geom)
{
/* attempts to dissolve a Geometry into segments */
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr segment;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    double x0 = 0.0;
    double y0 = 0.0;
    double z0 = 0.0;
    double m0 = 0.0;
    if (!geom)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    pt = geom->FirstPoint;
    while (pt)
      {
	  if (geom->DimensionModel == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (result, pt->X, pt->Y, pt->Z, pt->M);
	  else if (geom->DimensionModel == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y, pt->Z);
	  else if (geom->DimensionModel == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (result, pt->X, pt->Y, pt->M);
	  else
	      gaiaAddPointToGeomColl (result, pt->X, pt->Y);
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			{
			    if (x != x0 || y != y0 || z != z0 || m != m0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYZM (segment->Coords, 0, x0, y0,
						    z0, m0);
				  gaiaSetPointXYZM (segment->Coords, 1, x, y, z,
						    m);
			      }
			}
		      else if (geom->DimensionModel == GAIA_XY_Z)
			{
			    if (x != x0 || y != y0 || z != z0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYZ (segment->Coords, 0, x0, y0,
						   z0);
				  gaiaSetPointXYZ (segment->Coords, 1, x, y, z);
			      }
			}
		      else if (geom->DimensionModel == GAIA_XY_M)
			{
			    if (x != x0 || y != y0 || m != m0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYM (segment->Coords, 0, x0, y0,
						   m0);
				  gaiaSetPointXYM (segment->Coords, 1, x, y, m);
			      }
			}
		      else
			{
			    if (x != x0 || y != y0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPoint (segment->Coords, 0, x0, y0);
				  gaiaSetPoint (segment->Coords, 1, x, y);
			      }
			}
		  }
		x0 = x;
		y0 = y;
		z0 = z;
		m0 = m;
	    }
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior Ring */
		if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		if (iv > 0)
		  {
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			{
			    if (x != x0 || y != y0 || z != z0 || m != m0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYZM (segment->Coords, 0, x0, y0,
						    z0, m0);
				  gaiaSetPointXYZM (segment->Coords, 1, x, y, z,
						    m);
			      }
			}
		      else if (geom->DimensionModel == GAIA_XY_Z)
			{
			    if (x != x0 || y != y0 || z != z0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYZ (segment->Coords, 0, x0, y0,
						   z0);
				  gaiaSetPointXYZ (segment->Coords, 1, x, y, z);
			      }
			}
		      else if (geom->DimensionModel == GAIA_XY_M)
			{
			    if (x != x0 || y != y0 || m != m0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPointXYM (segment->Coords, 0, x0, y0,
						   m0);
				  gaiaSetPointXYM (segment->Coords, 1, x, y, m);
			      }
			}
		      else
			{
			    if (x != x0 || y != y0)
			      {
				  segment =
				      gaiaAddLinestringToGeomColl (result, 2);
				  gaiaSetPoint (segment->Coords, 0, x0, y0);
				  gaiaSetPoint (segment->Coords, 1, x, y);
			      }
			}
		  }
		x0 = x;
		y0 = y;
		z0 = z;
		m0 = m;
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* interior Ring */
		      if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      if (iv > 0)
			{
			    if (geom->DimensionModel == GAIA_XY_Z_M)
			      {
				  if (x != x0 || y != y0 || z != z0 || m != m0)
				    {
					segment =
					    gaiaAddLinestringToGeomColl (result,
									 2);
					gaiaSetPointXYZM (segment->Coords, 0,
							  x0, y0, z0, m0);
					gaiaSetPointXYZM (segment->Coords, 1, x,
							  y, z, m);
				    }
			      }
			    else if (geom->DimensionModel == GAIA_XY_Z)
			      {
				  if (x != x0 || y != y0 || z != z0)
				    {
					segment =
					    gaiaAddLinestringToGeomColl (result,
									 2);
					gaiaSetPointXYZ (segment->Coords, 0, x0,
							 y0, z0);
					gaiaSetPointXYZ (segment->Coords, 1, x,
							 y, z);
				    }
			      }
			    else if (geom->DimensionModel == GAIA_XY_M)
			      {
				  if (x != x0 || y != y0 || m != m0)
				    {
					segment =
					    gaiaAddLinestringToGeomColl (result,
									 2);
					gaiaSetPointXYM (segment->Coords, 0, x0,
							 y0, m0);
					gaiaSetPointXYM (segment->Coords, 1, x,
							 y, m);
				    }
			      }
			    else
			      {
				  if (x != x0 || y != y0)
				    {
					segment =
					    gaiaAddLinestringToGeomColl (result,
									 2);
					gaiaSetPoint (segment->Coords, 0, x0,
						      y0);
					gaiaSetPoint (segment->Coords, 1, x, y);
				    }
			      }
			}
		      x0 = x;
		      y0 = y;
		      z0 = z;
		      m0 = m;
		  }
	    }
	  pg = pg->Next;
      }
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDissolvePoints (gaiaGeomCollPtr geom)
{
/* attempts to dissolve a Geometry into points */
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    if (!geom)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    pt = geom->FirstPoint;
    while (pt)
      {
	  if (geom->DimensionModel == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (result, pt->X, pt->Y, pt->Z, pt->M);
	  else if (geom->DimensionModel == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y, pt->Z);
	  else if (geom->DimensionModel == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (result, pt->X, pt->Y, pt->M);
	  else
	      gaiaAddPointToGeomColl (result, pt->X, pt->Y);
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln)
      {
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		  }
		if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAddPointToGeomCollXYZM (result, x, y, z, m);
		else if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAddPointToGeomCollXYZ (result, x, y, z);
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAddPointToGeomCollXYM (result, x, y, m);
		else
		    gaiaAddPointToGeomColl (result, x, y);
	    }
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* exterior Ring */
		if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		if (geom->DimensionModel == GAIA_XY_Z_M)
		    gaiaAddPointToGeomCollXYZM (result, x, y, z, m);
		else if (geom->DimensionModel == GAIA_XY_Z)
		    gaiaAddPointToGeomCollXYZ (result, x, y, z);
		else if (geom->DimensionModel == GAIA_XY_M)
		    gaiaAddPointToGeomCollXYM (result, x, y, m);
		else
		    gaiaAddPointToGeomColl (result, x, y);
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* interior Ring */
		      if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			}
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			  gaiaAddPointToGeomCollXYZM (result, x, y, z, m);
		      else if (geom->DimensionModel == GAIA_XY_Z)
			  gaiaAddPointToGeomCollXYZ (result, x, y, z);
		      else if (geom->DimensionModel == GAIA_XY_M)
			  gaiaAddPointToGeomCollXYM (result, x, y, m);
		      else
			  gaiaAddPointToGeomColl (result, x, y);
		  }
	    }
	  pg = pg->Next;
      }
    result->Srid = geom->Srid;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaExtractPointsFromGeomColl (gaiaGeomCollPtr geom)
{
/* extracts any POINT from a GeometryCollection */
    gaiaGeomCollPtr result;
    gaiaPointPtr pt;
    int pts = 0;
    if (!geom)
	return NULL;

    pt = geom->FirstPoint;
    while (pt)
      {
	  pts++;
	  pt = pt->Next;
      }
    if (!pts)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    pt = geom->FirstPoint;
    while (pt)
      {
	  if (geom->DimensionModel == GAIA_XY_Z_M)
	      gaiaAddPointToGeomCollXYZM (result, pt->X, pt->Y, pt->Z, pt->M);
	  else if (geom->DimensionModel == GAIA_XY_Z)
	      gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y, pt->Z);
	  else if (geom->DimensionModel == GAIA_XY_M)
	      gaiaAddPointToGeomCollXYM (result, pt->X, pt->Y, pt->M);
	  else
	      gaiaAddPointToGeomColl (result, pt->X, pt->Y);
	  pt = pt->Next;
      }
    result->Srid = geom->Srid;
    if (pts == 1)
	result->DeclaredType = GAIA_POINT;
    else
	result->DeclaredType = GAIA_MULTIPOINT;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaExtractLinestringsFromGeomColl (gaiaGeomCollPtr geom)
{
/* extracts any LINESTRING from a GeometryCollection */
    gaiaGeomCollPtr result;
    gaiaLinestringPtr ln;
    gaiaLinestringPtr new_ln;
    int lns = 0;
    int iv;
    double x;
    double y;
    double z;
    double m;
    if (!geom)
	return NULL;

    ln = geom->FirstLinestring;
    while (ln)
      {
	  lns++;
	  ln = ln->Next;
      }
    if (!lns)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    ln = geom->FirstLinestring;
    while (ln)
      {
	  new_ln = gaiaAddLinestringToGeomColl (result, ln->Points);
	  for (iv = 0; iv < ln->Points; iv++)
	    {
		if (ln->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else if (ln->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (ln->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (ln->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  ln = ln->Next;
      }
    result->Srid = geom->Srid;
    if (lns == 1)
	result->DeclaredType = GAIA_LINESTRING;
    else
	result->DeclaredType = GAIA_MULTILINESTRING;
    return result;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaExtractPolygonsFromGeomColl (gaiaGeomCollPtr geom)
{
/* extracts any POLYGON from a GeometryCollection */
    gaiaGeomCollPtr result;
    gaiaPolygonPtr pg;
    gaiaPolygonPtr new_pg;
    gaiaRingPtr rng;
    gaiaRingPtr new_rng;
    int pgs = 0;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;
    if (!geom)
	return NULL;

    pg = geom->FirstPolygon;
    while (pg)
      {
	  pgs++;
	  pg = pg->Next;
      }
    if (!pgs)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    pg = geom->FirstPolygon;
    while (pg)
      {
	  rng = pg->Exterior;
	  new_pg =
	      gaiaAddPolygonToGeomColl (result, rng->Points, pg->NumInteriors);
	  new_rng = new_pg->Exterior;
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_rng->Coords, iv, x, y, z, m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_rng->Coords, iv, x, y, z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_rng->Coords, iv, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (new_rng->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		new_rng = gaiaAddInteriorRing (new_pg, ib, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      if (rng->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_rng->Coords, iv, x, y, z, m);
			}
		      else if (rng->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_rng->Coords, iv, x, y, z);
			}
		      else if (rng->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_rng->Coords, iv, x, y, m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    gaiaSetPoint (new_rng->Coords, iv, x, y);
			}
		  }
	    }
	  pg = pg->Next;
      }
    result->Srid = geom->Srid;
    if (pgs == 1)
	result->DeclaredType = GAIA_POLYGON;
    else
	result->DeclaredType = GAIA_MULTIPOLYGON;
    return result;
}

SPATIALITE_PRIVATE int
gaia_do_check_linestring (const void *g)
{
/* testing if the Geometry is a simple Linestring */
    gaiaGeomCollPtr geom = (gaiaGeomCollPtr) g;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int pts = 0;
    int lns = 0;
    int pgs = 0;
    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  /* counting how many Points are there */
	  pts++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  /* counting how many Linestrings are there */
	  lns++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  /* counting how many Polygons are there */
	  pgs++;
	  pg = pg->Next;
      }
    if (pts == 0 && lns == 1 && pgs == 0)
	return 1;
    return 0;
}

static int
do_create_points (sqlite3 * mem_db, const char *table)
{
/* creating a table for storing Points */
    int ret;
    char *sql;
    char *err_msg = NULL;

/* creating the main table */
    if (strcmp (table, "points1") == 0)
	sql = sqlite3_mprintf ("CREATE TABLE %s "
			       "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
			       "geom BLOB NOT NULL, "
			       "needs_interpolation INTEGER NOT NULL)", table);
    else
	sql = sqlite3_mprintf ("CREATE TABLE %s "
			       "(id INTEGER PRIMARY KEY AUTOINCREMENT, "
			       "geom BLOB NOT NULL)", table);
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaDrapeLine: CREATE TABLE \"%s\" error: %s\n",
			table, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    if (strcmp (table, "points1") == 0)
	return 1;

/* creating the companion R*Tree table */
    sql = sqlite3_mprintf ("CREATE VIRTUAL TABLE rtree_%s "
			   "USING rtree(pkid, xmin, xmax, ymin, ymax)", table);
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaDrapeLine: CREATE TABLE \"rtree_%s\" error: %s\n",
			table, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_insert_point (sqlite3 * mem_db, sqlite3_stmt * stmt_pts,
		 sqlite3_stmt * stmt_rtree_pts, double x,
		 double y, double z, double m)
{
/* inserting into the Points2 helper table */
    int ret;
    sqlite3_int64 rowid;

    sqlite3_reset (stmt_pts);
    sqlite3_clear_bindings (stmt_pts);
    sqlite3_bind_double (stmt_pts, 1, x);
    sqlite3_bind_double (stmt_pts, 2, y);
    sqlite3_bind_double (stmt_pts, 3, z);
    sqlite3_bind_double (stmt_pts, 4, m);
    ret = sqlite3_step (stmt_pts);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	rowid = sqlite3_last_insert_rowid (mem_db);
    else
      {
	  spatialite_e ("INSERT INTO \"Points\" error: \"%s\"\n",
			sqlite3_errmsg (mem_db));
	  goto error;
      }

/* inserting into the companion R*Tree */
    sqlite3_reset (stmt_rtree_pts);
    sqlite3_clear_bindings (stmt_rtree_pts);
    sqlite3_bind_int64 (stmt_rtree_pts, 1, rowid);
    sqlite3_bind_double (stmt_rtree_pts, 2, x);
    sqlite3_bind_double (stmt_rtree_pts, 3, x);
    sqlite3_bind_double (stmt_rtree_pts, 4, y);
    sqlite3_bind_double (stmt_rtree_pts, 5, y);
    ret = sqlite3_step (stmt_rtree_pts);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("INSERT INTO \"RTree_Points\" error: \"%s\"\n",
			sqlite3_errmsg (mem_db));
	  goto error;
      }

    return 1;

  error:
    return 0;
}

static int
do_populate_points2 (sqlite3 * mem_db, gaiaGeomCollPtr geom)
{
/* populating Points-2 */
    int ret;
    const char *sql;
    int iv;
    gaiaLinestringPtr ln;
    sqlite3_stmt *stmt_pts = NULL;
    sqlite3_stmt *stmt_rtree_pts = NULL;
    double ox;
    double oy;
    double oz;
    double om;
    double fx;
    double fy;
    double fz;
    double fm;

/* creating an SQL statement for inserting rows into the Points2 table */
    sql = "INSERT INTO points2 (id, geom) VALUES "
	"(NULL, MakePointZM(?, ?, ?, ?))";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt_pts, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO Points2: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

/* creating an SQL statement for inserting rows into the R*TREE table */
    sql = "INSERT INTO rtree_points2 (pkid, xmin, xmax, ymin, ymax) "
	"VALUES (?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt_rtree_pts, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO RTree_Points2: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

/* starting a Transaction */
    sql = "BEGIN";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("BEGIN: error: %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

    ln = geom->FirstLinestring;
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* processing all Vertices from the input Linestring */
	  double x;
	  double y;
	  double z = 0.0;
	  double m = 0.0;
	  int repeated;
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
	  repeated = 0;
	  if (iv != 0)
	    {
		/* checking for repeated points */
		if (x == ox && y == oy && z == oz && m == om)
		    repeated = 1;
	    }
	  if (iv == ln->Points - 1)
	    {
		/* checkink for a closed Linestring */
		if (x == fx && y == fy && z == fz && m == fm)
		    repeated = 1;
	    }
	  if (!repeated)
	    {
		/* inserting a Point */
		if (!do_insert_point
		    (mem_db, stmt_pts, stmt_rtree_pts, x, y, z, m))
		    goto error;
	    }
	  /* saving the current coords */
	  ox = x;
	  oy = y;
	  oz = z;
	  om = m;
	  if (iv == 0)
	    {
		/* saving the Start Point coords */
		fx = x;
		fy = y;
		fz = z;
		fm = m;
	    }
      }

/* committing the pending Transaction */
    sql = "COMMIT";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("COMMIT: error: %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

    sqlite3_finalize (stmt_pts);
    sqlite3_finalize (stmt_rtree_pts);
    return 1;

  error:
    if (stmt_pts != NULL)
	sqlite3_finalize (stmt_pts);
    if (stmt_rtree_pts != NULL)
	sqlite3_finalize (stmt_rtree_pts);
    return 0;
}

static int
do_insert_draped_point (sqlite3 * mem_db, sqlite3_stmt * stmt_pts,
			int needs_interpolation, gaiaGeomCollPtr geom)
{
/* inserting into the Points helper table */
    int ret;
    gaiaPointPtr pt = geom->FirstPoint;

    if (pt == NULL)
	return 0;

    sqlite3_reset (stmt_pts);
    sqlite3_clear_bindings (stmt_pts);
    sqlite3_bind_double (stmt_pts, 1, pt->X);
    sqlite3_bind_double (stmt_pts, 2, pt->Y);
    sqlite3_bind_double (stmt_pts, 3, pt->Z);
    sqlite3_bind_double (stmt_pts, 4, pt->M);
    sqlite3_bind_int (stmt_pts, 5, needs_interpolation);
    ret = sqlite3_step (stmt_pts);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

    spatialite_e ("INSERT INTO \"Points1\" error: \"%s\"\n",
		  sqlite3_errmsg (mem_db));
    return 0;
}

static gaiaGeomCollPtr
do_point_drape_coords (int srid, double x, double y, gaiaGeomCollPtr geom_3d)
{
/* draping a Point */
    gaiaPointPtr pt_3d = geom_3d->FirstPoint;
    gaiaGeomCollPtr geom = gaiaAllocGeomCollXYZM ();
    geom->Srid = srid;
    gaiaAddPointToGeomCollXYZM (geom, x, y, pt_3d->Z, pt_3d->M);
    return geom;
}

static gaiaGeomCollPtr
do_point_same_coords (int srid, double x, double y, double z, double m)
{
/* creating a Point (unchanged coords) */
    gaiaGeomCollPtr geom;
    geom = gaiaAllocGeomCollXYZM ();
    geom->Srid = srid;
    gaiaAddPointToGeomCollXYZM (geom, x, y, z, m);
    return geom;
}

static int
do_drape_vertex (sqlite3 * mem_db, sqlite3_stmt * stmt,
		 sqlite3_stmt * stmt_pts, int srid, double tolerance,
		 double x, double y, double z, double m)
{
/* draping the segment */
    double minx = x;
    double miny = y;
    double maxx = x;
    double maxy = y;
    int ret;
    int count = 0;
    gaiaGeomCollPtr g2;

/* preparing an extendend BBOX */
    minx = x - (tolerance * 2.0);
    miny = y - (tolerance * 2.0);
    maxx = x + (tolerance * 2.0);
    maxy = y + (tolerance * 2.0);

/* querying Points-2 by distance from Segment */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, minx);	/* R*Tree BBOX */
    sqlite3_bind_double (stmt, 2, miny);
    sqlite3_bind_double (stmt, 3, maxx);
    sqlite3_bind_double (stmt, 4, maxy);
    sqlite3_bind_double (stmt, 5, x);	/* point (distance) */
    sqlite3_bind_double (stmt, 6, y);
    sqlite3_bind_double (stmt, 7, tolerance);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		gaiaGeomCollPtr g2 = NULL;
		gaiaGeomCollPtr geom = NULL;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *p_blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int n_bytes = sqlite3_column_bytes (stmt, 0);
		      geom = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		  }
		if (geom != NULL)
		  {
		      /* found a valid Point */
		      g2 = do_point_drape_coords (srid, x, y, geom);
		      gaiaFreeGeomColl (geom);
		      if (!do_insert_draped_point (mem_db, stmt_pts, 0, g2))
			  goto error;
		      gaiaFreeGeomColl (g2);
		      count++;
		  }
	    }
      }
    if (count == 0)
      {
	  /* not found - inserting the Point itself */
	  g2 = do_point_same_coords (srid, x, y, z, m);
	  if (!do_insert_draped_point (mem_db, stmt_pts, 1, g2))
	      goto error;
	  gaiaFreeGeomColl (g2);
      }
    return 1;

  error:
    return 0;
}

static int
do_drape_line (sqlite3 * mem_db, gaiaGeomCollPtr geom, double tolerance)
{
/* draping the line */
    int ret;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_pts = NULL;
    const char *sql;
    gaiaLinestringPtr ln;
    int iv;

/* creating an SQL statement for querying Points-2 */
    sql = "SELECT geom FROM points2 "
	"WHERE ROWID IN (SELECT pkid FROM rtree_points2 WHERE "
	"MbrIntersects(geom, BuildMbr(?, ?, ?, ?)) = 1) "
	"AND ST_Distance(geom, MakePoint(?, ?)) <= ? " "ORDER BY id";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT Points2: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

/* creating an SQL statement for inserting rows into the Points-1 table */
    sql = "INSERT INTO points1 (id, geom, needs_interpolation) VALUES "
	"(NULL, MakePointZM(?, ?, ?, ?), ?)";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt_pts, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO Points1: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

/* starting a Transaction */
    sql = "BEGIN";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("BEGIN: error: %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

    ln = geom->FirstLinestring;
    for (iv = 0; iv < ln->Points; iv++)
      {
	  /* processing all Vertices from the input Linestring */
	  double x;
	  double y;
	  double z = 0.0;
	  double m = 0.0;
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
	  /* processing a Vertex */
	  if (!do_drape_vertex
	      (mem_db, stmt, stmt_pts, geom->Srid, tolerance, x, y, z, m))
	      goto error;
      }

/* committing the pending Transaction */
    sql = "COMMIT";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("COMMIT: error: %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto error;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_pts);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_pts != NULL)
	sqlite3_finalize (stmt_pts);
    return 0;
}

static int
get_prev_coords (int index, gaiaDynamicLinePtr dyn, double *z, double *m,
		 double *dist)
{
/* retrieving the previous Point */
    double ox;
    double oy;
    double x;
    double y;
    double zz;
    double mm;
    int ok = 0;
    int count = 0;
    gaiaPointPtr pt = dyn->First;
    while (pt != NULL)
      {
	  if (index - 1 == count)
	    {
		/* this is the previous point */
		ox = pt->X;
		oy = pt->Y;
		zz = pt->Z;
		mm = pt->M;
		ok = 1;
	    }
	  if (index == count)
	    {
		/* this is the current point */
		x = pt->X;
		y = pt->Y;
		if (ok)
		  {
		      *z = zz;
		      *m = mm;
		      *dist =
			  sqrt (((ox - x) * (ox - x)) + ((oy - y) * (oy - y)));
		      return 1;
		  }
		return 0;
	    }
	  count++;
	  pt = pt->Next;
      }
    return 0;
}

static int
get_next_coords (int index, gaiaDynamicLinePtr dyn, const char *interpolate,
		 double *z, double *m, double *dist)
{
/* retrieving the next Point */
    double ox;
    double oy;
    double x;
    double y;
    double d = 0.0;
    int ok = 0;
    int count = 0;
    gaiaPointPtr pt = dyn->First;
    while (pt != NULL)
      {
	  if (index == count)
	    {
		/* this is the current Point */
		ox = pt->X;
		oy = pt->Y;
		ok = 1;
	    }
	  if (index < count)
	    {
		/* this is a following Point */
		if (!ok)
		    return 0;
		x = pt->X;
		y = pt->Y;
		d += sqrt (((ox - x) * (ox - x)) + ((oy - y) * (oy - y)));
		if (*(interpolate + count) == 'N')
		  {
		      /* found a valid Point */
		      *z = pt->Z;
		      *m = pt->M;
		      *dist = d;
		      return 1;
		  }
	    }
	  count++;
	  pt = pt->Next;
      }
    return 0;
}

static int
do_update_coords (int index, gaiaDynamicLinePtr dyn, double z, double m)
{
/* updating the coords */
    int count = 0;
    gaiaPointPtr pt = dyn->First;
    while (pt != NULL)
      {
	  if (index == count)
	    {
		pt->Z = z;
		pt->M = m;
		return 1;
	    }
	  count++;
	  pt = pt->Next;
      }
      return 0;
}

static void
do_interpolate_coords (int index, gaiaDynamicLinePtr dyn, char *interpolate)
{
/* attempting to interpolate coords */
    double pz;
    double pm;
    double nz;
    double nm;
    double z;
    double m;
    double pdist;
    double ndist;
    double perc;
    if (!get_prev_coords (index, dyn, &pz, &pm, &pdist))
	return;
    if (!get_next_coords (index, dyn, interpolate, &nz, &nm, &ndist))
	return;
    perc = pdist / (pdist + ndist);
    z = pz + ((nz - pz) * perc);
    m = pm + ((nm - pm) * perc);
    if (do_update_coords (index, dyn, z, m))
    *(interpolate+index) = 'I';
}

static gaiaGeomCollPtr
do_reassemble_line (sqlite3 * mem_db, int dims, int srid)
{
/* reassembling the Linestring to be returned */
    gaiaGeomCollPtr geom = NULL;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int count = 0;
    int needs_interpolation = 0;

/* creating an SQL statement for querying Points-1 */
    sql = "SELECT geom, needs_interpolation FROM points1 ORDER BY id";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT Points1: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto end;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		gaiaGeomCollPtr g = NULL;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *p_blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int n_bytes = sqlite3_column_bytes (stmt, 0);
		      g = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		  }
		if (g != NULL)
		  {
		      /* found a valid Point */
		      pt = g->FirstPoint;
		      if (dims == GAIA_XY_Z_M)
			  gaiaAppendPointZMToDynamicLine (dyn, pt->X, pt->Y,
							  pt->Z, pt->M);
		      else if (dims == GAIA_XY_Z)
			  gaiaAppendPointZToDynamicLine (dyn, pt->X, pt->Y,
							 pt->Z);
		      else if (dims == GAIA_XY_M)
			  gaiaAppendPointMToDynamicLine (dyn, pt->X, pt->Y,
							 pt->M);
		      else
			  gaiaAppendPointToDynamicLine (dyn, pt->X, pt->Y);
		      gaiaFreeGeomColl (g);
		  }
		if (sqlite3_column_int (stmt, 1) == 1)
		    needs_interpolation = 1;
	    }
      }

    pt = dyn->First;
    while (pt)
      {
	  /* counting how many points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count < 2)
	goto end;

    if (needs_interpolation)
      {
	  /* attempting to interpolate missing coords */
	  int i;
	  int max = count;
	  char *interpolate = malloc (max + 1);
	  memset (interpolate, '\0', max + 1);
	  sqlite3_reset (stmt);	/* rewinding the resultset */
	  count = 0;
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_int (stmt, 1) == 0)
			  *(interpolate + count) = 'N';
		      else
			  *(interpolate + count) = 'Y';
		      count++;
		  }
	    }
	  for (i = 0; i < max; i++)
	    {
		if (*(interpolate + i) == 'Y')
		    do_interpolate_coords (i, dyn, interpolate);
	    }
	  free (interpolate);
      }

    sqlite3_finalize (stmt);
    stmt = NULL;

/* creating the final Linestring */
    if (dims == GAIA_XY_Z_M)
	geom = gaiaAllocGeomCollXYZM ();
    else if (dims == GAIA_XY_Z)
	geom = gaiaAllocGeomCollXYZ ();
    else if (dims == GAIA_XY_M)
	geom = gaiaAllocGeomCollXYM ();
    else
	geom = gaiaAllocGeomColl ();
    geom->Srid = srid;

    ln = gaiaAddLinestringToGeomColl (geom, count);
    count = 0;
    pt = dyn->First;
    while (pt != NULL)
      {
	  if (dims == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, count, pt->X, pt->Y,
				  pt->Z, pt->M);
	    }
	  else if (dims == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, count, pt->X, pt->Y, pt->Z);
	    }
	  else if (dims == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, count, pt->X, pt->Y, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, count, pt->X, pt->Y);
	    }
	  count++;
	  pt = pt->Next;
      }

  end:
    gaiaFreeDynamicLine (dyn);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return geom;
}

static gaiaGeomCollPtr
do_reassemble_multi_point (sqlite3 * mem_db, int dims, int srid,
			   int interpolated)
{
/* reassembling the MultiPoint to be returned */
    gaiaGeomCollPtr geom = NULL;
    gaiaPointPtr pt;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int count = 0;
    int needs_interpolation = 0;
    char *interpolate = NULL;

/* creating an SQL statement for querying Points-1 */
    sql = "SELECT geom, needs_interpolation FROM points1 ORDER BY id";
    ret = sqlite3_prepare_v2 (mem_db, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT Points1: error %d \"%s\"\n",
			sqlite3_errcode (mem_db), sqlite3_errmsg (mem_db));
	  goto end;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		gaiaGeomCollPtr g = NULL;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *p_blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int n_bytes = sqlite3_column_bytes (stmt, 0);
		      g = gaiaFromSpatiaLiteBlobWkb (p_blob, n_bytes);
		  }
		if (g != NULL)
		  {
		      /* found a valid Point */
		      pt = g->FirstPoint;
		      if (dims == GAIA_XY_Z_M)
			  gaiaAppendPointZMToDynamicLine (dyn, pt->X, pt->Y,
							  pt->Z, pt->M);
		      else if (dims == GAIA_XY_Z)
			  gaiaAppendPointZToDynamicLine (dyn, pt->X, pt->Y,
							 pt->Z);
		      else if (dims == GAIA_XY_M)
			  gaiaAppendPointMToDynamicLine (dyn, pt->X, pt->Y,
							 pt->M);
		      else
			  gaiaAppendPointToDynamicLine (dyn, pt->X, pt->Y);
		      gaiaFreeGeomColl (g);
		  }
		if (sqlite3_column_int (stmt, 1) == 1)
		    needs_interpolation = 1;
	    }
      }

    pt = dyn->First;
    while (pt)
      {
	  /* counting how many points are there */
	  count++;
	  pt = pt->Next;
      }
    if (count < 2)
	goto end;

    if (needs_interpolation)
      {
	  /* attempting to interpolate missing coords */
	  int i;
	  int max = count;
	  interpolate = malloc (max + 1);
	  memset (interpolate, '\0', max + 1);
	  sqlite3_reset (stmt);	/* rewinding the resultset */
	  count = 0;
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      if (sqlite3_column_int (stmt, 1) == 0)
			  *(interpolate + count) = 'N';
		      else
			  *(interpolate + count) = 'Y';
		      count++;
		  }
	    }
	  for (i = 0; i < max; i++)
	    {
		if (*(interpolate + i) == 'Y')
		    do_interpolate_coords (i, dyn, interpolate);
	    }
      }

    sqlite3_finalize (stmt);
    stmt = NULL;

/* creating the final MultiPoint */
    if (dims == GAIA_XY_Z_M)
	geom = gaiaAllocGeomCollXYZM ();
    else if (dims == GAIA_XY_Z)
	geom = gaiaAllocGeomCollXYZ ();
    else if (dims == GAIA_XY_M)
	geom = gaiaAllocGeomCollXYM ();
    else
	geom = gaiaAllocGeomColl ();
    geom->Srid = srid;
    geom->DeclaredType = GAIA_MULTIPOINT;

    pt = dyn->First;
    count = 0;
    while (pt != NULL)
      {
	  int ok = 0;
	  if (*(interpolate + count) == 'Y')
	      ok = 1;
	  if (!interpolated && *(interpolate + count) == 'I')
	      ok = 1;
	  if (ok)
	    {
		if (dims == GAIA_XY_Z_M)
		    gaiaAddPointToGeomCollXYZM (geom, pt->X, pt->Y, pt->Z,
						pt->M);
		else if (dims == GAIA_XY_Z)
		    gaiaAddPointToGeomCollXYZ (geom, pt->X, pt->Y, pt->Z);
		else if (dims == GAIA_XY_M)
		    gaiaAddPointToGeomCollXYM (geom, pt->X, pt->Y, pt->M);
		else
		    gaiaAddPointToGeomColl (geom, pt->X, pt->Y);
	    }
	  count++;
	  pt = pt->Next;
      }

  end:
    if (interpolate != NULL)
	free (interpolate);
    gaiaFreeDynamicLine (dyn);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return geom;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDrapeLine (sqlite3 * db_handle, gaiaGeomCollPtr geom1,
	       gaiaGeomCollPtr geom2, double tolerance)
{
/* will return a 3D Linestring by draping line-1 (2D) over line-2 (3D) */
    int ret;
    sqlite3 *mem_db = NULL;
    void *cache;
    char *sql;
    char *err_msg = NULL;
    gaiaGeomCollPtr geom3 = NULL;

/* arguments validation */
    if (db_handle == NULL)
	return NULL;
    if (geom1 == NULL || geom2 == NULL)
	return NULL;
    if (tolerance < 0.0)
	return NULL;
    if (geom1->Srid != geom2->Srid)
	return NULL;
    if (geom1->DimensionModel != GAIA_XY)
	return NULL;
    if (geom2->DimensionModel != GAIA_XY_Z)
	return NULL;
    if (!gaia_do_check_linestring (geom1))
	return NULL;
    if (!gaia_do_check_linestring (geom2))
	return NULL;

/* creating a Temporary MemoryDB */
    ret =
	sqlite3_open_v2 (":memory:", &mem_db,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaDrapeLine: sqlite3_open_v2 error: %s\n",
			sqlite3_errmsg (mem_db));
	  sqlite3_close (mem_db);
	  return NULL;
      }
    cache = spatialite_alloc_connection ();
    spatialite_internal_init (mem_db, cache);

/* initializing a minimal SpatiaLite DB */
    sql = "SELECT InitSpatialMetadata(1, 'NONE')";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaDrapeLine: InitSpatialMetadata() error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  goto end;
      }

/* creating the helper tables on the Temporary MemoryDB */
    if (!do_create_points (mem_db, "points1"))
	goto end;
    if (!do_create_points (mem_db, "points2"))
	goto end;

/* populating the Points-2 helper table */
    if (!do_populate_points2 (mem_db, geom2))
	goto end;

/* draping the first line over Points-2 */
    if (!do_drape_line (mem_db, geom1, tolerance))
	goto end;

/* building the final linestring to be returned */
    geom3 = do_reassemble_line (mem_db, geom2->DimensionModel, geom2->Srid);

  end:
/* releasing the Temporary MemoryDB */
    ret = sqlite3_close (mem_db);
    if (ret != SQLITE_OK)
	spatialite_e ("gaiaDrapeLine: sqlite3_close() error: %s\n",
		      sqlite3_errmsg (mem_db));
    spatialite_internal_cleanup (cache);

    return geom3;
}

GAIAGEO_DECLARE gaiaGeomCollPtr
gaiaDrapeLineExceptions (sqlite3 * db_handle, gaiaGeomCollPtr geom1,
			 gaiaGeomCollPtr geom2, double tolerance,
			 int interpolated)
{
/*
 *  will return a 3D MultiPoint containing all Vertices from geom1
 * not being correctly draped over geom2
*/
    int ret;
    sqlite3 *mem_db = NULL;
    void *cache;
    char *sql;
    char *err_msg = NULL;
    gaiaGeomCollPtr geom3 = NULL;

/* arguments validation */
    if (db_handle == NULL)
	return NULL;
    if (geom1 == NULL || geom2 == NULL)
	return NULL;
    if (tolerance < 0.0)
	return NULL;
    if (geom1->Srid != geom2->Srid)
	return NULL;
    if (geom1->DimensionModel != GAIA_XY)
	return NULL;
    if (geom2->DimensionModel != GAIA_XY_Z)
	return NULL;
    if (!gaia_do_check_linestring (geom1))
	return NULL;
    if (!gaia_do_check_linestring (geom2))
	return NULL;

/* creating a Temporary MemoryDB */
    ret =
	sqlite3_open_v2 (":memory:", &mem_db,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("gaiaDrapeLine: sqlite3_open_v2 error: %s\n",
			sqlite3_errmsg (mem_db));
	  sqlite3_close (mem_db);
	  return NULL;
      }
    cache = spatialite_alloc_connection ();
    spatialite_internal_init (mem_db, cache);

/* initializing a minimal SpatiaLite DB */
    sql = "SELECT InitSpatialMetadata(1, 'NONE')";
    ret = sqlite3_exec (mem_db, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("gaiaDrapeLineExceptions: InitSpatialMetadata() error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  goto end;
      }

/* creating the helper tables on the Temporary MemoryDB */
    if (!do_create_points (mem_db, "points1"))
	goto end;
    if (!do_create_points (mem_db, "points2"))
	goto end;

/* populating the Points-2 helper table */
    if (!do_populate_points2 (mem_db, geom2))
	goto end;

/* draping the first line over Points-2 */
    if (!do_drape_line (mem_db, geom1, tolerance))
	goto end;

/* building the final MultiPoint to be returned */
    geom3 =
	do_reassemble_multi_point (mem_db, geom2->DimensionModel, geom2->Srid,
				   interpolated);

  end:
/* releasing the Temporary MemoryDB */
    ret = sqlite3_close (mem_db);
    if (ret != SQLITE_OK)
	spatialite_e ("gaiaDrapeLineExceptions: sqlite3_close() error: %s\n",
		      sqlite3_errmsg (mem_db));
    spatialite_internal_cleanup (cache);

    return geom3;
}
