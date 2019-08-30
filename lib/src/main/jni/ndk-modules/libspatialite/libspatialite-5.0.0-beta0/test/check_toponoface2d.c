/*

 check_toponoface2d.c -- SpatiaLite Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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
 
Portions created by the Initial Developer are Copyright (C) 2016
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */
#ifndef OMIT_ICONV		/* only if ICONV is enabled */

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 0 tests */
    int ret;
    char *err_msg = NULL;

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('topo')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -50;
	  return 0;
      }

/* adding four Linestrings forming a Rectangle */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-150 80, 150 80)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-150 -80, 150 -80)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -52;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-150 80, -150 -80)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #3 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -53;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(150 80, 150 -80)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNpFace() #4 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -54;
	  return 0;
      }

/* adding a closed Linestring */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-10 10, 10 10, 10 -10, -10 -10, -10 10)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #4 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -55;
	  return 0;
      }

/* adding a self-intersecting and closed Linestring */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-120 -70, 120 70, -120 70, 120 -70, -120 -70)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #5 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -56;
	  return 0;
      }

/* adding a first MultiLinestring */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('MULTILINESTRING("
		      "(-180 90, 180 90), (-180 80, 180 80), (-180 70, 180 70), (-180 60, 180 60), "
		      "(-180 50, 180 50), (-180 40, 180 40), (-180 30, 180 30), (-180 20, 180 20), "
		      "(-180 10, 180 10), (-180 0, 180 0), (-180 -10, 180 -10), (-180 -20, 180 -20), "
		      "(-180 -30, 180 -30), (-180 -40, 180 -40), (-180 -50, 180 -50), "
		      "(-180 -60, 180 -60), (-180 -70, 180 -70), (-180 -80, 180 -80), "
		      "(-180 -90, 180 -90))', 4326))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #6 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }

/* adding a second MultiLinestring */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('MULTILINESTRING("
		      "(-180 90, -180 -90), (-170 90, -170 -90), (-160 90, -160 -90), "
		      "(-150 90, -150 -90), (-140 90, -140 -90), (-130 90, -130 -90), "
		      "(-120 90, -120 -90), (-110 90, -110 -90), (-100 90, -100 -90), "
		      "(-90 90, -90 -90), (-80 90, -80 -90), (-70 90, -70 -90), (-60 90, -60 -90), "
		      "(-50 90, -50 -90), (-40 90, -40 -90), (-30 90, -30 -90), (-20 90, -20 -90), "
		      "(-10 90, -10 -90), (0 90, 0 -90), (10 90, 10 -90), (20 90, 20 -90), "
		      "(30 90, 30 -90), (40 90, 40 -90), (50 90, 50 -90), (60 90, 60 -90), "
		      "(70 90, 70 -90), (80 90, 80 -90), (90 90, 90 -90), (100 90, 100 -90), "
		      "(110 90, 110  -90), (120 90, 120 -90), (130 90, 130 -90), "
		      "(140 90, 140 -90), (150 90, 150 -90), (160 90, 160 -90), "
		      "(170 90, 170 -90), (180 90, 180 -90))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #7 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -58;
	  return 0;
      }

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('topo')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -59;
	  return 0;
      }

    return 1;
}

static int
do_level1_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 1 tests */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int invalid = 0;

/* adding more  closed Linestrings */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-175 85, -165 85, -165 75, -175 75, -175 85)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #8 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -60;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-175 -85, -165 -85, -165 -75, -175 -75, -175 -85)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #9 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(175 85, 165 85, 165 75, 175 75, 175 85)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #10 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(175 -85, 165 -85, 165 -75, 175 -75, 175 -85)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineStringNoFace() #11 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }

/* re-building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('topo')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -64;
	  return 0;
      }

/* re-building Faces - conditional mode */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('topo')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -65;
	  return 0;
      }

/* unconditionally re-building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('topo', 1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -65;
	  return 0;
      }

/* validating this TopoGeo */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('topo')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #1: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -66;
	  return 0;
      }

/* testing for a valid TopoGeo */
    ret =
	sqlite3_get_table (handle, "SELECT Count(*) FROM topo_validate_topogeo",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "test ValidateTopoGeo() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -67;
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      invalid = 1;
      }
    sqlite3_free_table (results);
    if (invalid)
      {
	  fprintf (stderr, "Topology 'topo' is invalid !!!");
	  *retcode = -68;
	  return 0;
      }

    return 1;
}

static int
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 2 tests */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int invalid = 0;

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elba', 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode = -70;
	  return 0;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFace('elba', NULL, 'elba_pg', NULL, 512)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFace() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }

/* loading a Linestring GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFace('elba', NULL, 'elba_ln', NULL, 512)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFace() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -72;
	  return 0;
      }

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
	  return 0;
      }

/* validating this TopoGeo */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #2: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -74;
	  return 0;
      }

/* testing for a valid TopoGeo */
    ret =
	sqlite3_get_table (handle, "SELECT Count(*) FROM elba_validate_topogeo",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "test ValidateTopoGeo() #2: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -75;
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      invalid = 1;
      }
    sqlite3_free_table (results);
    if (invalid)
      {
	  fprintf (stderr, "Topology 'elba' is invalid !!!");
	  *retcode = -76;
	  return 0;
      }

    return 1;
}

static int
do_level3_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 3 tests */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int invalid = 0;

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elbaext', 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode = -80;
	  return 0;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFaceExt('elbaext', NULL, 'elba_pg', NULL, 'dustbin', 'dustbinview', 512)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFaceExt() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -81;
	  return 0;
      }

/* loading a Linestring GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFaceExt('elbaext', NULL, 'elba_ln', NULL, 'dustbin2', 'dustbinview2', 512)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFaceExt() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -92;
	  return 0;
      }

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('elbaext')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -83;
	  return 0;
      }

/* validating this TopoGeo */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elbaext')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #3: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -84;
	  return 0;
      }

/* testing for a valid TopoGeo */
    ret =
	sqlite3_get_table (handle,
			   "SELECT Count(*) FROM elbaext_validate_topogeo",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "test ValidateTopoGeo() #3: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -85;
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  if (atoi (results[(i * columns) + 0]) > 0)
	      invalid = 1;
      }
    sqlite3_free_table (results);
    if (invalid)
      {
	  fprintf (stderr, "Topology 'elbaext' is invalid !!!");
	  *retcode = -86;
	  return 0;
      }

    return 1;
}

static int
do_level4_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 4 tests - INVALID CASES */
    int ret;
    char *err_msg = NULL;

/* adding a Linestring - wrong SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRING(-150 80, 150 80)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #12 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #12 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }
    sqlite3_free (err_msg);

/* adding a Linestring - wrong dimensions */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('LINESTRINGZ(-150 80 10, 150 80 20)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #13 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -92;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #13 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -93;
	  return 0;
      }
    sqlite3_free (err_msg);

/* adding a Point - wrong type */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineStringNoFace('topo', GeomFromText('POINT(-150 80)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #13 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -94;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid argument.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLineStringNoFace() #13 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -95;
	  return 0;
      }
    sqlite3_free (err_msg);

/* loading a Polygon GeoTable - not existing */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFace('elba', NULL, 'elba_wannabe', NULL, 512)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFace() #3 - unexpected success\n");
	  *retcode = -96;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid input GeoTable.")
	!= 0)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFace() #3 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -96;
	  return 0;
      }
    sqlite3_free (err_msg);

/* loading a Polygon GeoTable - invalid dimensions */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFace('elba', NULL, 'merano', NULL, 512)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFace() #4 - unexpected success\n");
	  *retcode = -97;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFace() #4 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -99;
	  return 0;
      }
    sqlite3_free (err_msg);

/* loading a Polygon GeoTable Extended - not existing */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFaceExt('elbaext', NULL, 'elba_wannabe', NULL, 'dustbin5', 'dustbinview5', 512)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #3 - unexpected success\n");
	  *retcode = -100;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid input GeoTable.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #3 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -101;
	  return 0;
      }
    sqlite3_free (err_msg);

/* loading a Polygon GeoTable Extended - invalid dimensions */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFaceExt('elbaext', NULL, 'merano', NULL, 'dustbin6', 'dustbinview6', 512)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #4 - unexpected success\n");
	  *retcode = -102;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #4 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -103;
	  return 0;
      }
    sqlite3_free (err_msg);

/* loading a Linestring GeoTable Extended - already existing dustbin view */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFaceExt('elbaext', NULL, 'elba_ln', NULL, 'dustbin7', 'dustbinview', 512)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #4 unexpected success\n");
	  *retcode = -104;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - unable to create the dustbin view.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableNoFaceExt() #4 - unexpected: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -105;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

#endif
#endif

int
main (int argc, char *argv[])
{
    int retcode = 0;

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */
#ifndef OMIT_ICONV		/* only if ICONV is enabled */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();
    char *old_SPATIALITE_SECURITY_ENV = NULL;
#ifdef _WIN32
    char *env;
#endif /* not WIN32 */

    old_SPATIALITE_SECURITY_ENV = getenv ("SPATIALITE_SECURITY");
#ifdef _WIN32
    putenv ("SPATIALITE_SECURITY=relaxed");
#else /* not WIN32 */
    setenv ("SPATIALITE_SECURITY", "relaxed", 1);
#endif

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \":memory:\" database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    if (sqlite3_libversion_number () < 3008003)
      {
	  fprintf (stderr,
		   "*** check_toponoface2d skipped: libsqlite < 3.8.3 !!!\n");
	  goto end;
      }

    ret = sqlite3_exec (handle, "PRAGMA foreign_keys=1", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys=1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -2;
      }

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateTopology('topo', 4326, 0, 0.000001)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* importing Elba (polygons) from SHP */
    ret =
	sqlite3_exec (handle,
		      "SELECT ImportSHP('./elba-pg', 'elba_pg', 'CP1252', 32632)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportSHP() elba-pg error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -5;
      }

/* importing Elba (linestrings) from SHP */
    ret =
	sqlite3_exec (handle,
		      "SELECT ImportSHP('./elba-ln', 'elba_ln', 'CP1252', 32632)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportSHP() elba-ln error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -6;
      }

/* importing Merano Roads (linestrings) from SHP */
    ret =
	sqlite3_exec (handle,
		      "SELECT ImportSHP('./shp/merano-3d/polygons', 'merano', 'UTF-8', 32632)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportSHP() roads error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -7;
      }

    if (old_SPATIALITE_SECURITY_ENV)
      {
#ifdef _WIN32
	  env =
	      sqlite3_mprintf ("SPATIALITE_SECURITY=%s",
			       old_SPATIALITE_SECURITY_ENV);
	  putenv (env);
	  sqlite3_free (env);
#else /* not WIN32 */
	  setenv ("SPATIALITE_SECURITY", old_SPATIALITE_SECURITY_ENV, 1);
#endif
      }
    else
      {
#ifdef _WIN32
	  putenv ("SPATIALITE_SECURITY=");
#else /* not WIN32 */
	  unsetenv ("SPATIALITE_SECURITY");
#endif
      }

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elba', 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -8;
      }

/*tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

/*tests: level 1 */
    if (!do_level1_tests (handle, &retcode))
	goto end;

/*tests: level 2 */
    if (!do_level2_tests (handle, &retcode))
	goto end;

/*tests: level 3 */
    if (!do_level3_tests (handle, &retcode))
	goto end;

/*tests: level 4 */
    if (!do_level4_tests (handle, &retcode))
	goto end;

  end:
    spatialite_finalize_topologies (cache);
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);

#endif
#endif /* end RTTOPO conditional */

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    return retcode;
}
