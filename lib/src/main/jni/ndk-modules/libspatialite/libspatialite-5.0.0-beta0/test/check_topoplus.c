/*

 check_topoplus.c -- SpatiaLite Test Case

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
 
Portions created by the Initial Developer are Copyright (C) 2011
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
do_level11_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 11 */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int changed_links = 0;

/* creating a Network 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateNetwork('netsegments', 1, 4326, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -300;
	  return 0;
      }

/* inserting four Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('netsegments', MakePoint(-45, -45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -301;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('netsegments', MakePoint(-45, 45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -302;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('netsegments', MakePoint(45, -45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -303;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNetNode('netsegments', MakePoint(45, 45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNetNode() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -304;
	  return 0;
      }

/* inserting Links */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('netsegments', 1, 2, GeomFromText('LINESTRING(-45 -45,  -45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -305;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('netsegments', 3, 4, GeomFromText('LINESTRING(45 -45,  45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -306;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('netsegments', 1, 3, GeomFromText('LINESTRING(-45 -45,  45 -45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -307;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddLink('netsegments', 2, 4, GeomFromText('LINESTRING(-45 45,  0 45, 45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddLink() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -308;
	  return 0;
      }

/* disambiguating segment Links */
    ret = sqlite3_get_table
	(handle,
	 "SELECT TopoNet_DisambiguateSegmentLinks('netsegments')",
	 &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_DisambiguateSegmentLinks() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -309;
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  changed_links = atoi (value);
      }
    sqlite3_free_table (results);
    if (changed_links != 3)
      {
	  fprintf (stderr,
		   "TopoNet_DisambiguateSegmentLinks() #1 invalid count: %d\n",
		   changed_links);
	  *retcode = -310;
	  return 0;
      }

    return 1;
}

static int
do_level10_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 10 */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int changed_edges = 0;

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateTopology('segments', 4326, 0, 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #10 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -300;
	  return 0;
      }

/* inserting four Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('segments', NULL, MakePoint(-45, -45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -301;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('segments', NULL, MakePoint(-45, 45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -302;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('segments', NULL, MakePoint(45, -45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -303;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('segments', NULL, MakePoint(45, 45, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -304;
	  return 0;
      }

/* inserting Edges */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('segments', 1, 2, GeomFromText('LINESTRING(-45 -45,  -45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -305;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('segments', 3, 4, GeomFromText('LINESTRING(45 -45,  45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -306;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('segments', 1, 3, GeomFromText('LINESTRING(-45 -45,  45 -45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -307;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('segments', 2, 4, GeomFromText('LINESTRING(-45 45,  0 45, 45 45)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -308;
	  return 0;
      }

/* disambiguating segment Edges */
    ret = sqlite3_get_table
	(handle,
	 "SELECT TopoGeo_DisambiguateSegmentEdges('segments')",
	 &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_DisambiguateSegmentEdges() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -309;
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  changed_edges = atoi (value);
      }
    sqlite3_free_table (results);
    if (changed_edges != 3)
      {
	  fprintf (stderr,
		   "TopoGeo_DisambiguateSegmentEdges() #1 invalid count: %d\n",
		   changed_edges);
	  *retcode = -310;
	  return 0;
      }

    return 1;
}

static int
do_level9_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 9 */
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    int valid;

/* updating TopoSeeds */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_UpdateSeeds('elba_clone')", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_UpdateSeeds() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -300;
	  return 0;
      }

/* testing TopoGeo_SnapPointToSeed - ok */
    if (sqlite3_get_table
	(handle,
	 "SELECT ST_AsText(TopoGeo_SnapPointToSeed(MakePoint(612452.7, 4730202.4, 32632), 'elba_clone', 1.0))",
	 &results, &rows, &columns, &err_msg) != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnapPointToSeed() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -301;
	  return 0;
      }
    valid = 1;
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  if (strcmp (value, "POINT(612452.924936 4730202.975964)") != 0)
	    {
		fprintf (stderr, "TopoGeo_SnapPointToSeed() #2 error: %s\n",
			 value);
		valid = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!valid)
      {
	  *retcode = -302;
	  return 0;
      }

/* testing TopoGeo_SnapPointToSeed - no Seed within distance */
    if (sqlite3_get_table
	(handle,
	 "SELECT ST_AsText(TopoGeo_SnapPointToSeed(MakePoint(2.7, 4.9, 32632), 'elba_clone', 1.0))",
	 &results, &rows, &columns, &err_msg) != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnapPointToSeed() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -303;
	  return 0;
      }
    valid = 1;
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  if (value != NULL)
	    {
		fprintf (stderr, "TopoGeo_SnapPointToSeed() #4 error: %s\n",
			 value);
		valid = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!valid)
      {
	  *retcode = -304;
	  return 0;
      }

/* testing TopoGeo_SnapPointToSeed - invalid SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AsText(TopoGeo_SnapPointToSeed(MakePoint(2.7, 4.9, 4326), 'elba_clone', 1.0))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapPointToSeed() wrong SRID: expected failure\n");
	  *retcode = -305;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Point (mismatching SRID od dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapPointToSeed() wrong SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -306;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_SnapPointToSeed - invalid dims */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AsText(TopoGeo_SnapPointToSeed(MakePointZ(2.7, 4.9, 10, 32632), 'elba_clone', 1.0))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapPointToSeed() wrong dims: expected failure\n");
	  *retcode = -305;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Point (mismatching SRID od dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapPointToSeed() wrong dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -306;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_SnapLineToSeed - ok */
    if (sqlite3_get_table
	(handle,
	 "SELECT ST_AsText(TopoGeo_SnapLineToSeed(ST_GeomFromText("
	 "'LINESTRING(612385 4730247.99, 612389 4730247.95)', 32632), 'elba_clone', 1.0))",
	 &results, &rows, &columns, &err_msg) != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnapLineToSeed() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -309;
	  return 0;
      }
    valid = 1;
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  if (strcmp
	      (value,
	       "LINESTRING(612385 4730247.99, 612387.425489 4730247.975612, 612389 4730247.95)")
	      != 0)
	    {
		fprintf (stderr, "TopoGeo_SnapLineToSeed() #2 error: %s\n",
			 value);
		valid = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!valid)
      {
	  *retcode = -310;
	  return 0;
      }

/* testing TopoGeo_SnapLineToSeed - no Seed within distance */
    if (sqlite3_get_table
	(handle,
	 "SELECT ST_AsText(TopoGeo_SnapLineToSeed(ST_GeomFromText("
	 "'LINESTRING(5 7.99, 9 7.95)', 32632), 'elba_clone', 1.0))", &results,
	 &rows, &columns, &err_msg) != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnapLineToSeed() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -311;
	  return 0;
      }
    valid = 1;
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns)];
	  if (value != NULL)
	    {
		fprintf (stderr, "TopoGeo_SnapLineToSeed() #4 error: %s\n",
			 value);
		valid = 0;
	    }
      }
    sqlite3_free_table (results);
    if (!valid)
      {
	  *retcode = -312;
	  return 0;
      }

/* testing TopoGeo_SnapLineToSeed - invalid SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AsText(TopoGeo_SnapLineToSeed(ST_GeomFromText("
		      "'LINESTRING(5 7.99, 9 7.95)', 4325), 'elba_clone', 1.0))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapLineToSeed() wrong SRID: expected failure\n");
	  *retcode = -313;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Line (mismatching SRID od dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapLineToSeed() wrong SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -314;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_SnapLineToSeed - invalid dims */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AsText(TopoGeo_SnapLineToSeed(ST_GeomFromText("
		      "'LINESTRINGZ(5 7.99 1, 9 7.95 2)', 32632), 'elba_clone', 1.0))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapLineToSeed() wrong dims: expected failure\n");
	  *retcode = -315;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Line (mismatching SRID od dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnapLineToSeed() wrong dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -316;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing RemoveSmallFaces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveSmallFaces('elba_clone', 0.7, 1000)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_RemoveSmallFaces() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -317;
	  return 0;
      }

/* testing RemoveDanglingEdges */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveDanglingEdges('elba_clone')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_RemoveDanglingEdges() error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -318;
	  return 0;
      }

/* testing RemoveDanglingNodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveDanglingNodes('elba_clone')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_RemoveDanglingNodes() error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -319;
	  return 0;
      }

    return 1;
}


static int
do_level8_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 8 */
    int ret;
    char *err_msg = NULL;

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateTopology('diagnostic', 23032, 0, 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -300;
	  return 0;
      }

/* attaching an external DB */
    ret =
	sqlite3_exec (handle,
		      "ATTACH DATABASE \"./test_geos.sqlite\" AS inputDB", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -301;
	  return 0;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('diagnostic', 'inputDB', 'comuni', NULL, 'dustbin', 'dustbinview', 650)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() no PK: expected failure\n");
	  *retcode = -302;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - unable to create the dustbin table.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -302;
	  return 0;
      }

/* detaching the external DB */
    ret =
	sqlite3_exec (handle, "DETACH DATABASE inputDB", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DETACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -303;
	  return 0;
      }

/* attempting to load a Topology - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('wannebe', NULL, 'elba_ln', NULL, 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing Topology: expected failure\n");
	  *retcode = -304;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -305;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - non-existing GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('diagnostic', NULL, 'wannabe', NULL, 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing GeoTable: expected failure\n");
	  *retcode = -306;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -307;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('diagnostic', 'lollypop', 'elba_ln', NULL, 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() wrong DB-prefix: expected failure\n");
	  *retcode = -308;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -309;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('diagnostic', NULL, 'elba_ln', 'none', 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing Geometry: expected failure\n");
	  *retcode = -310;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -311;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('badelba1', NULL, 'elba_ln', 'geometry', 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() mismatching SRID: expected failure\n");
	  *retcode = -312;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -313;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - mismatching dims */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('badelba2', NULL, 'elba_ln', 'GEOMETRY', 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() mismatching dims: expected failure\n");
	  *retcode = -314;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() mismatching dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -315;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('diagnostic', NULL, 'elba_pg', NULL, 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() ambiguous Geometry: expected failure\n");
	  *retcode = -316;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -317;
	  return 0;
      }
    sqlite3_free (err_msg);

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateTopology('ext', 32632, 0, 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #9 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -316;
      }

/* attempting to load a Topology  */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('ext', NULL, 'export_elba1', NULL, 'dustbin', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableExt() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -317;
	  return 0;
      }

/* attempting to load a Topology - already existing dustbin-table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('ext', NULL, 'export_elba1', NULL, 'dustbin', 'dustbinview2')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() existing dustbin-table: expected failure\n");
	  *retcode = -318;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - unable to create the dustbin table.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() existing dustbin-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -319;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - already existing dustbin-view */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableExt('ext', NULL, 'export_elba1', NULL, 'dustbin2', 'dustbinview')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() existing dustbin-view: expected failure\n");
	  *retcode = -320;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - unable to create the dustbin view.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTableExt() existing dustbin-view: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -321;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

static int
do_level7_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 7 */
    int ret;
    char *err_msg = NULL;

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('topocom', 23032, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -200;
      }

/* attaching an external DB */
    ret =
	sqlite3_exec (handle,
		      "ATTACH DATABASE \"./test_geos.sqlite\" AS inputDB", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -201;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('topocom', 'inputDB', 'comuni', NULL, 650)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -202;
	  return 0;
      }

/* detaching the external DB */
    ret =
	sqlite3_exec (handle, "DETACH DATABASE inputDB", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DETACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -203;
	  return 0;
      }

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elbasplit', 32632, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -204;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elbasplit', 'main', 'elba_pg', 'geometry', 256, 1000)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -205;
	  return 0;
      }

/* creating a Topology 2D */
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateTopology('elbalnsplit', 32632, 0, 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -206;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elbalnsplit', 'main', 'elba_ln', 'geometry', 256, 500)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -207;
	  return 0;
      }

/* loading a GeoTable into a TopoNet */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'roads', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_FromGeoTable() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -208;
	  return 0;
      }

/* testing TopNet_LineLinksList - ok */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_LineLinksList('roads', NULL, 'roads', 'geometry', 'line_links_list')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_LineLinksList() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -209;
	  return 0;
      }

/* testing TopoGeo_GetEdgeSeed */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_GetEdgeSeed('elbasplit', edge_id) FROM MAIN.elbasplit_edge",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_GetEdgeSeed() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -210;
	  return 0;
      }

/* testing TopoGeo_GetFaceSeed */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_GetFaceSeed('elbasplit', face_id) FROM MAIN.elbasplit_face WHERE face_id <> 0",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_GetFaceSeed() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -211;
	  return 0;
      }

/* testing TopoNet_GetLinkSeed */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_GetLinkSeed('roads', link_id) FROM MAIN.roads_link",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_GetLinkSeed() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -212;
	  return 0;
      }

/* testing TopoGeo_UpdateSeeds */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_UpdateSeeds('elbasplit')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_UpdateSeeds() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -213;
	  return 0;
      }

/* testing TopoNet_UpdateSeeds */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_UpdateSeeds('roads')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_UpdateSeeds() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -214;
	  return 0;
      }

/* sleeping for 2 secs, so to be sure that 'now' really changes */
    sqlite3_sleep (2000);

/* Edge's fake update */
    ret =
	sqlite3_exec (handle,
		      "UPDATE MAIN.elbasplit_edge SET left_face = left_face",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE elbasplit error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -215;
	  return 0;
      }

/* testing TopoGeo_UpdateSeeds */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_UpdateSeeds('elbasplit', 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_UpdateSeeds() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -216;
	  return 0;
      }

/* Link's fake update */
    ret =
	sqlite3_exec (handle,
		      "UPDATE MAIN.roads_link SET start_node = start_node",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE roads error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -217;
	  return 0;
      }

/* testing TopoNet_UpdateSeeds */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_UpdateSeeds('roads', 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_UpdateSeeds() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -218;
	  return 0;
      }

/* testing TopoGeo_ToGeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elbasplit', NULL, 'elba_ln', NULL, 'export_elba1')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ToGeoTable() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -219;
	  return 0;
      }

/* testing TopoGeo_ToGeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elbasplit', NULL, 'elba_pg', 'geometry', 'export_elba2', 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ToGeoTable() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -220;
	  return 0;
      }

/* testing TopoNet_ToGeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', 'geometry', 'export_roads')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_ToGeoTable() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -221;
	  return 0;
      }

/* testing TopoGeo_ToGeoTableGeneralize */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elbasplit', NULL, 'elba_ln', NULL, 'export_elba1gen', 10)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ToGeoTableGeneralize() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -222;
	  return 0;
      }

/* testing TopoGeo_ToGeoTableGeneralize */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elbasplit', NULL, 'elba_pg', 'geometry', 'export_elba2gen', 10, 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ToGeoTableGeneralize() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -223;
	  return 0;
      }

/* testing TopoNet_ToGeoTableGeneralize */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'geometry', 'export_roads1gen', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_ToGeoTableGeneralize() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -224;
	  return 0;
      }

/* testing TopoNet_ToGeoTableGeneralize */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'geometry', 'export_roads2gen', 10, 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_ToGeoTableGeneralize() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -225;
	  return 0;
      }

/* testing TopoGeo_CreateTopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elbasplit', NULL, 'elba_pg', 'geometry', 'myelba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_CreateTopoLayer() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -226;
	  return 0;
      }

/* testing TopoGeo_ExportTopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ExportTopoLayer('elbasplit', 'myelba', 'out_myelba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ExportTopoLayer() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -227;
	  return 0;
      }

/* testing TopoGeo_RemoveTopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveTopoLayer('elbasplit', 'myelba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_RemoveTopoLayer() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -228;
	  return 0;
      }

/* testing TopoGeo_CreateTopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elbasplit', NULL, 'elba_pg', 'geometry', 'yourelba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_CreateTopoLayer() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -229;
	  return 0;
      }

/* testing TopoGeo_ExportTopoLayer - create only */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ExportTopoLayer('elbasplit', 'yourelba', 'out_yourelba', 0, 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ExportTopoLayer() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -230;
	  return 0;
      }

/* testing TopoGeo_InsertFeatureFromTopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InsertFeatureFromTopoLayer('elbasplit', 'yourelba', 'out_yourelba', 5)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -231;
	  return 0;
      }

/* testing CreateTopoGeo - ok */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_CreateTopoGeo('badelba1', GeomFromText('GEOMETRYCOLLECTION(POINT(1 2), LINESTRING(3 3, 4 4), POLYGON((10 10, 11 10, 11 11, 10 11, 10 10)))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_CreateTopoGeo() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -232;
	  return 0;
      }

/* testing CreateTopoGeo - already populated Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_CreateTopoGeo('badelba1', MakePoint(1, 2, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_CreateTopoGeo() #2: expected failure\n");
	  *retcode = -233;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-empty topology.") != 0)
      {
	  fprintf (stderr,
		   "ST_CreateTopoGeo() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -234;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing SpatNetFromGeom - ok */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromGeom('badnet1', GeomFromText('GEOMETRYCOLLECTION(POINT(1 2), LINESTRING(3 3, 4 4), POLYGON((10 10, 11 10, 11 11, 10 11, 10 10)))', 3003))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_SpatNetFromGeom() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -235;
	  return 0;
      }

/* testing SpatNetFromGeom - already populated Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromGeom('badnet1', MakePoint(1, 2, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_SpatNetFromGeom() #2: expected failure\n");
	  *retcode = -236;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-empty network.") != 0)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -237;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_PolyFacesList - ok */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_PolyFacesList('elbasplit', NULL, 'elba_pg', 'geometry', 'poly_faces_list')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_PolyFacesList() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -238;
	  return 0;
      }

/* testing TopoGeo_LineEdgesList - ok */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_LineEdgesList('elbasplit', NULL, 'elba_ln', 'geometry', 'line_edges_list')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_LineEdgesList() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -239;
	  return 0;
      }

    return 1;
}


static int
do_level6_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 6 */
    int ret;
    char *err_msg = NULL;

/* Validating a Topology - valid */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -180;
	  return 0;
      }

/* dirtying the Topology */
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_node (node_id, containing_face, geom) "
		      "SELECT NULL, NULL, ST_PointN(geom, 33) FROM elba_edge WHERE edge_id = 13",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Node #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -182;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_edge SET geom = GeomFromText('LINESTRING(604477 4736752, 604483 4736751, 604480 4736755, 604483 4736751, 604840 4736648)', 32632) WHERE edge_id = 26",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Edge error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -183;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_face (face_id, mbr) VALUES (NULL, BuildMBR(610000, 4700000, 610001, 4700001, 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Face #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -184;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_edge SET left_face = NULL WHERE left_face = 0",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Edge #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -185;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_edge SET right_face = NULL WHERE right_face = 0",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Edge #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -186;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_node SET containing_face = NULL WHERE containing_face = 0",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Edge #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -187;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "DELETE FROM elba_face WHERE face_id = 0",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DELETE FROM Face #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -188;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_node (node_id, containing_face, geom) "
		      "VALUES (NULL, NULL, MakePoint(612771, 4737829, 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Node #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -189;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_face (face_id, mbr) VALUES (NULL, BuildMBR(612771, 4737329, 613771, 4737829, 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Face #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -190;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_edge (edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom) "
		      "VALUES (NULL, 49, 49, 46, -46, 28, 33, GeomFromText('LINESTRING(612771 4737829, 613771 4737829, 613771 4737329, 612771 4737329, 612771 4737829)', 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Edge #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -191;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_node (node_id, containing_face, geom) "
		      "VALUES (NULL, NULL, MakePoint(613200, 4737700, 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Node #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -192;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_face (face_id, mbr) VALUES (NULL, BuildMbr(611771, 4738329, 612771, 4738829, 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Face #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -193;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO elba_edge (edge_id, start_node, end_node, next_left_edge, next_right_edge, left_face, right_face, geom) "
		      "VALUES (NULL, 50, 50, 47, -47, 29, 33, GeomFromText('LINESTRING(613200 4737700, 613400 4737700, 613400 4737400, 613200 4737400, 613200 4737700)', 32632))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO Edge #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -194;
	  return 0;
      }

/* Validating yet again the dirtied Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -195;
	  return 0;
      }

    return 1;
}

static int
do_level5_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 5 */
    int ret;
    char *err_msg = NULL;

/* Validating a Spatial Network - valid */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidSpatialNet('spatnet')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidSpatialNet() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -160;
	  return 0;
      }

/* dirtying the Spatial Network */
    ret =
	sqlite3_exec (handle,
		      "UPDATE spatnet_link SET geometry = NULL WHERE link_id = 1",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Spatial Link error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -162;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE spatnet_link SET geometry = GeomFromText('LINESTRING(604477 4736752, 604840 4736648)', 32632) WHERE link_id = 26",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Spatial Link error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -163;
	  return 0;
      }

/* Validating yet again the dirtied Spatial Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidSpatialNet('spatnet')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidSpatialNet() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -164;
	  return 0;
      }

    return 1;
}

static int
do_level4_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 4 */
    int ret;
    char *err_msg = NULL;

/* Validating a Logical Network - valid */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidLogicalNet('loginet')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidLogicalNet() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -150;
	  return 0;
      }

/* dirtying the Logical Network */
    ret =
	sqlite3_exec (handle,
		      "UPDATE loginet_link SET geometry = GeomFromText('LINESTRING(0 0, 1 1)', -1) WHERE link_id = 1",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE Logical Link error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -152;
	  return 0;
      }

/* Validating yet again the dirtied Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidLogicalNet('loginet')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidLogicalNet() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -153;
	  return 0;
      }

    return 1;
}

static int
do_level3_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 3 */
    int ret;
    char *err_msg = NULL;

/* cloning a Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Clone(NULL, 'elba', 'elba_clone')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Clone() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -120;
	  return 0;
      }

/* attempting to clone a Topology - already existing destination Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Clone('MAIN', 'elba', 'elba_clone')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_Clone() already existing destination Topology: expected failure\n");
	  *retcode = -121;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid topology name (destination).") !=
	0)
      {
	  fprintf (stderr,
		   "TopoGeo_Clone() already existing destination Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -122;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Logical Network (non-existing) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_LogiNetFromTGeo('loginet', 'lollypop')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() non-existing Topology: expected failure\n");
	  *retcode = -124;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid topology name.") !=
	0)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -125;
	  return 0;
      }
    sqlite3_free (err_msg);

/* transforming a Topology into a Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_LogiNetFromTGeo('loginet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "LogiNetFromTGeo() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -126;
	  return 0;
      }

/* attempting to transform a Topology into a Spatial Network (non-existing) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('spatnet', 'lollypop')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() non-existing Topology: expected failure\n");
	  *retcode = -127;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - invalid topology name.") !=
	0)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -128;
	  return 0;
      }
    sqlite3_free (err_msg);

/* transforming a Topology into a Spatial Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('spatnet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SpatNetFromTGeo() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -129;
	  return 0;
      }

/* attempting to transform a Topology into a Logical Network (non-empty) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_LogiNetFromTGeo('loginet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() already populated Network: expected failure\n");
	  *retcode = -130;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-empty network.") != 0)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() already populated Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -131;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Spatial Network (non-empty) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('spatnet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() already populated Network: expected failure\n");
	  *retcode = -132;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-empty network.") != 0)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() already populated Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -133;
	  return 0;
      }
    sqlite3_free (err_msg);

/* cloning a Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_Clone(NULL, 'loginet', 'loginet_clone')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_Clone() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -137;
	  return 0;
      }

/* attempting to clone a Logical Network - already existing destination Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_Clone('MAIN', 'loginet', 'loginet_clone')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_Clone() already existing destination Logical Network: expected failure\n");
	  *retcode = -138;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid network name (destination).") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_Clone() already existing destination Logical Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -139;
	  return 0;
      }
    sqlite3_free (err_msg);

/* cloning a Spatial Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_Clone('MAIN', 'spatnet', 'spatnet_clone')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoNet_Clone() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -140;
	  return 0;
      }

/* attempting to clone a Spatial Network - already existing destination Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_Clone(NULL, 'spatnet', 'spatnet_clone')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_Clone() already existing destination Spatial Network: expected failure\n");
	  *retcode = -141;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid network name (destination).") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_Clone() already existing destination Spatial Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -142;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}


static int
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 2 */
    int ret;
    char *err_msg = NULL;

/* loading a Point GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_pg', 'centroid')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -100;
	  return 0;
      }

/* loading a Polygon GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_pg', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -101;
	  return 0;
      }

    return 1;
}

static int
do_level1_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 1 */
    int ret;
    char *err_msg = NULL;

/* loading a Linestring GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_ln', NULL)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }

/* loading a Point GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_pg', 'centroid')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTable() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }

    return 1;
}

static int
do_level00_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 00 */
    int ret;
    char *err_msg = NULL;

/* attempting to load a Network - non-existing Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('wannebe', NULL, 'roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing Network: expected failure\n");
	  *retcode = -220;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid network name.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -221;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - non-existing GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'wannabe', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing GeoTable: expected failure\n");
	  *retcode = -222;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -223;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', 'lollypop', 'roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() wrong DB-prefix: expected failure\n");
	  *retcode = -224;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -225;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'roads', 'none')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing Geometry: expected failure\n");
	  *retcode = -226;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -227;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'roads', 'wgs')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching SRID: expected failure\n");
	  *retcode = -228;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID, dimensions or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -229;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - mismatching dims */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'roads', 'g3d')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching dims: expected failure\n");
	  *retcode = -230;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID, dimensions or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -231;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - mismatching class */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'elba_pg', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching class: expected failure\n");
	  *retcode = -232;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID, dimensions or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() mismatching class: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -233;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Network - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('roads', NULL, 'roads', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() ambiguous Geometry: expected failure\n");
	  *retcode = -235;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -235;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_FromGeoTable('loginet', NULL, 'roads', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() Logical Network: expected failure\n");
	  *retcode = -236;
	  return 0;
      }
    if (strcmp
	(err_msg, "FromGeoTable() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_FromGeoTable() Logical Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -237;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - non-existing Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('wannebe', NULL, 'roads', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing Network: expected failure\n");
	  *retcode = -238;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid network name.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -239;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - non-existing GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'wannabe', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing ref-GeoTable: expected failure\n");
	  *retcode = -240;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing ref-GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -241;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', 'lollypop', 'roads', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() wrong DB-prefix: expected failure\n");
	  *retcode = -242;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -243;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', 'none', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing Geometry: expected failure\n");
	  *retcode = -244;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -245;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', 'wgs', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() mismatching SRID: expected failure\n");
	  *retcode = -246;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -247;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - mismatching class */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'elba_pg', 'geometry', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() mismatching class: expected failure\n");
	  *retcode = -248;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() mismatching class: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -249;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() ambiguous Geometry: expected failure\n");
	  *retcode = -250;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -251;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('loginet', NULL, 'roads', 'geometry', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() Logical Network: expected failure\n");
	  *retcode = -252;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTable() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() Logical Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -253;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - already existing out-table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', 'geometry', 'elba_pg')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() already existing out-table: expected failure\n");
	  *retcode = -254;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -255;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Network - already existing out-table (non-geo) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTable('roads', NULL, 'roads', 'geometry', 'geometry_columns')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() already existing out-table (non-geo): expected failure\n");
	  *retcode = -256;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoNet_ToGeoTable: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() already existing out-table (non-geo): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -257;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - non-existing Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('wannebe', NULL, 'roads', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() non-existing Network: expected failure\n");
	  *retcode = -258;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid network name.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() non-existing Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -259;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - non-existing GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'wannabe', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() non-existing ref-GeoTable: expected failure\n");
	  *retcode = -260;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() non-existing ref-GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -261;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', 'lollypop', 'roads', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() wrong DB-prefix: expected failure\n");
	  *retcode = -262;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -263;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'none', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() non-existing Geometry: expected failure\n");
	  *retcode = -264;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -265;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'wgs', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() mismatching SRID: expected failure\n");
	  *retcode = -266;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -267;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - mismatching class */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'elba_pg', 'geometry', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() mismatching class: expected failure\n");
	  *retcode = -268;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() mismatching class: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -269;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() ambiguous Geometry: expected failure\n");
	  *retcode = -270;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -271;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Logical Network */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('loginet', NULL, 'roads', 'geometry', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() Logical Network: expected failure\n");
	  *retcode = -272;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize() cannot be applied to Logical Network.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() Logical Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -273;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - already existing out-table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'geometry', 'elba_pg', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() already existing out-table: expected failure\n");
	  *retcode = -274;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -275;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Network - already existing out-table (non-geo) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoNet_ToGeoTableGeneralize('roads', NULL, 'roads', 'geometry', 'geometry_columns', 10)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() already existing out-table (non-geo): expected failure\n");
	  *retcode = -276;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoNet_ToGeoTableGeneralize: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoNet_ToGeoTableGeneralize() already existing out-table (non-geo): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -277;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 0 */
    int ret;
    char *err_msg = NULL;

/* attempting to load a Topology - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('wannebe', NULL, 'elba_ln', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing Topology: expected failure\n");
	  *retcode = -50;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - non-existing GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'wannabe', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing GeoTable: expected failure\n");
	  *retcode = -52;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -53;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', 'lollypop', 'elba_ln', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() wrong DB-prefix: expected failure\n");
	  *retcode = -54;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -55;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_ln', 'none')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing Geometry: expected failure\n");
	  *retcode = -56;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('badelba1', NULL, 'elba_ln', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() mismatching SRID: expected failure\n");
	  *retcode = -58;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -59;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - mismatching dims */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('badelba2', NULL, 'elba_ln', 'GEOMETRY')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() mismatching dims: expected failure\n");
	  *retcode = -60;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() mismatching dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to load a Topology - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTable('elba', NULL, 'elba_pg', NULL)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() ambiguous Geometry: expected failure\n");
	  *retcode = -62;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_FromGeoTable() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Logical Network (Spatial Network indeed) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_LogiNetFromTGeo('spatnet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() Spatial Network: expected failure\n");
	  *retcode = -64;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "ST_LogiNetFromTGeo() cannot be applied to Spatial Network.") != 0)
      {
	  fprintf (stderr,
		   "LogiNetFromTGeo() Spatial Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -65;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Spatial Network (Logical Network indeed) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('loginet', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() Logical Network: expected failure\n");
	  *retcode = -66;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "ST_SpatNetFromTGeo() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() Logical Network: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -67;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Spatial Network (mismatching SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('badnet1', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() mismatching SRID: expected failure\n");
	  *retcode = -68;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - mismatching SRID or dimensions.") != 0)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -69;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to transform a Topology into a Spatial Network (mismatching dims) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromTGeo('badnet2', 'elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() mismatching dims: expected failure\n");
	  *retcode = -70;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - mismatching SRID or dimensions.") != 0)
      {
	  fprintf (stderr,
		   "SpatNetFromTGeo() mismatching dims: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to validate a Logical Network (Spatial indeed) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidLogicalNet('spatnet')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ValidLogicalNet() mismatching type: expected failure\n");
	  *retcode = -72;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "ST_ValidLogicalNet() cannot be applied to Spatial Network.") != 0)
      {
	  fprintf (stderr,
		   "ValidLogicalNet() mismatching type: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to validate a Logical Network (empty) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidLogicalNet('loginet')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ValidLogicalNet() empty: expected failure\n");
	  *retcode = -74;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - empty network.") != 0)
      {
	  fprintf (stderr,
		   "ValidLogicalNet() empty: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -75;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to validate a Spatial Network (Logical indeed) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidSpatialNet('loginet')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ValidSpatialNet() mismatching type: expected failure\n");
	  *retcode = -76;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "ST_ValidSpatialNet() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr,
		   "ValidSpatialNet() mismatching type: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -77;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to validate a Spatial Network (empty) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidSpatialNet('spatnet')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ValidSpatialNet() empty: expected failure\n");
	  *retcode = -78;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - empty network.") != 0)
      {
	  fprintf (stderr,
		   "ValidSpatialNet() empty: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -79;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to validate a TopoGeo (empty) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() empty: expected failure\n");
	  *retcode = -80;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - empty topology.") != 0)
      {
	  fprintf (stderr,
		   "ValidateTopoGeo() empty: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -81;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('wannebe', NULL, 'elba_ln', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing Topology: expected failure\n");
	  *retcode = -82;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -83;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - non-existing ref-GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', NULL, 'wannabe', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing ref-GeoTable: expected failure\n");
	  *retcode = -84;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -85;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', 'lollypop', 'elba_ln', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() wrong DB-prefix: expected failure\n");
	  *retcode = -86;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -87;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', NULL, 'elba_ln', 'none', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing Geometry: expected failure\n");
	  *retcode = -88;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -89;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('badelba1', NULL, 'elba_ln', 'geometry', 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() mismatching SRID: expected failure\n");
	  *retcode = -90;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', NULL, 'elba_pg', NULL, 'out-table')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() ambiguous Geometry: expected failure\n");
	  *retcode = -92;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -93;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - already existing out-table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', NULL, 'elba_ln', NULL, 'elba_pg')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() already existing out-table: expected failure\n");
	  *retcode = -94;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -95;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Topology - already existing out-table (non-geo) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTable('elba', NULL, 'elba_ln', NULL, 'geometry_columns')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() already existing out-table (non-geo): expected failure\n");
	  *retcode = -96;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ToGeoTable: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() already existing out-table (non-geo): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -97;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('wannebe', NULL, 'elba_ln', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() non-existing Topology: expected failure\n");
	  *retcode = -82;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -98;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - non-existing ref-GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', NULL, 'wannabe', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() non-existing ref-GeoTable: expected failure\n");
	  *retcode = -99;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -100;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', 'lollypop', 'elba_ln', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() wrong DB-prefix: expected failure\n");
	  *retcode = -101;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -102;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', NULL, 'elba_ln', 'none', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() non-existing Geometry: expected failure\n");
	  *retcode = -103;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -104;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('badelba1', NULL, 'elba_ln', 'geometry', 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() mismatching SRID: expected failure\n");
	  *retcode = -105;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -106;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', NULL, 'elba_pg', NULL, 'out-table', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTable() ambiguous Geometry: expected failure\n");
	  *retcode = -107;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -108;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - already existing out-table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', NULL, 'elba_ln', NULL, 'elba_pg', 10.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() already existing out-table: expected failure\n");
	  *retcode = -109;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -110;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a Generalized Topology - already existing out-table (non-geo) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ToGeoTableGeneralize('elba', NULL, 'elba_ln', NULL, 'geometry_columns', 10)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() already existing out-table (non-geo): expected failure\n");
	  *retcode = -111;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ToGeoTableGeneralize: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ToGeoTableGeneralize() already existing out-table (non-geo): unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -112;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('wannebe', NULL, 'elba_ln', NULL, 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing Topology: expected failure\n");
	  *retcode = -113;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -114;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - non-existing ref-GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', NULL, 'wannabe', NULL, 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing ref-GeoTable: expected failure\n");
	  *retcode = -115;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_CreateTopoLayer: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing GeoTable: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -116;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', 'lollypop', 'elba_ln', NULL, 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() wrong DB-prefix: expected failure\n");
	  *retcode = -117;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_CreateTopoLayer: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -118;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - wrong geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', NULL, 'elba_ln', 'none', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing Geometry: expected failure\n");
	  *retcode = -119;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_CreateTopoLayer: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() non-existing Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -120;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('badelba1', NULL, 'elba_ln', 'geometry', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() mismatching SRID: expected failure\n");
	  *retcode = -121;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -122;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - mismatching View SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('badelba1', NULL, 'elba_ln', 'geometry', 'topolyr', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() mismatching SRID: expected failure\n");
	  *retcode = -123;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_CreateTopoLayer: invalid reference View (invalid Geometry).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -124;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to create a TopoLayer - ambiguous geometry column */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', NULL, 'elba_pg', NULL, 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() ambiguous Geometry: expected failure\n");
	  *retcode = -125;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_CreateTopoLayer: invalid reference GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() ambiguos Geometry: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -126;
	  return 0;
      }
    sqlite3_free (err_msg);

/* creating a TopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', NULL, 'elba_ln', NULL, 'elba_ln')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_CreateTopoLayer error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -127;
      }

/* attempting to create a TopoLayer - already existing  */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_CreateTopoLayer('elba', NULL, 'elba_ln', NULL, 'elba_ln')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() already existing out-table: expected failure\n");
	  *retcode = -128;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_CreateTopoLayer: a TopoLayer of the same name already exists.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_CreateTopoLayer() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -129;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a TopoLayer - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveTopoLayer('wannebe', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_RemoveTopoLayer() non-existing Topology: expected failure\n");
	  *retcode = -130;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_RemoveTopoLayer() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -131;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a TopoLayer - non-existing Topolayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_RemoveTopoLayer('elba', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_RemoveTopoLayer() non-existing TopoLayer: expected failure\n");
	  *retcode = -132;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_RemoveTopoLayer: not existing TopoLayer.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_RemoveTopoLayer() non-existing TopoLayer: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -133;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a TopoLayer - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ExportTopoLayer('wannebe', 'topolyr', 'outtable')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() non-existing Topology: expected failure\n");
	  *retcode = -134;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -135;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a TopoLayer - non-existing Topolayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ExportTopoLayer('elba', 'topolyr', 'outtable')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() non-existing TopoLayer: expected failure\n");
	  *retcode = -136;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_ExportTopoLayer: not existing TopoLayer.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() non-existing TopoLayer: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -137;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export a TopoLayer - already existing output table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ExportTopoLayer('elba', 'elba_ln', 'elba_pg')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() already-existing out-table: expected failure\n");
	  *retcode = -138;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ExportTopoLayer: the output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() already-existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -139;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export TopoFeatures - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InsertFeatureFromTopoLayer('wannebe', 'topolyr', 'outtable', 100)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() non-existing Topology: expected failure\n");
	  *retcode = -140;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -141;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export TopoFeatures - non-existing Topolayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InsertFeatureFromTopoLayer('elba', 'topolyr', 'outtable', 100)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() non-existing TopoLayer: expected failure\n");
	  *retcode = -142;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_InsertFeatureFromTopoLayer: non-existing TopoLayer.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() non-existing TopoLayer: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -143;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to export TopoFeatures - already existing output table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InsertFeatureFromTopoLayer('elba', 'elba_ln', 'outtable', 100)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InsertFeatureFromTopoLayer() non-existing out-table: expected failure\n");
	  *retcode = -144;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_InsertFeatureFromTopoLayer: the output GeoTable does not exists.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ExportTopoLayer() non-existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -145;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing CreateTopoGeo - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_CreateTopoGeo('elba', MakePoint(1, 2, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_CreateTopoGeo() mismatching SRID: expected failure\n");
	  *retcode = -146;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_CreateTopoGeo() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -147;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing CreateTopoGeo - mismatching DIMs */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_CreateTopoGeo('elba', MakePointZ(1, 2, 3, 32632))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_CreateTopoGeo() mismatching DIMs: expected failure\n");
	  *retcode = -148;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_CreateTopoGeo() mismatching DIMs: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -149;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing SpatNetFromGeom - mismatching SRID */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromGeom('roads', MakePoint(1, 2, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching SRID: expected failure\n");
	  *retcode = -150;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -151;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing SpatNetFromGeom - mismatching DIMs */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromGeom('roads', MakePointZ(1, 2, 3, 32632))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching DIMs: expected failure\n");
	  *retcode = -152;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching DIMs: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -153;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing SpatNetFromGeom - logical network */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_SpatNetFromGeom('loginet', MakePointZ(1, 2, 3, 32632))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching DIMs: expected failure\n");
	  *retcode = -154;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "ST_ValidSpatialNet() cannot be applied to Logical Network.") != 0)
      {
	  fprintf (stderr,
		   "ST_SpatNetFromGeom() mismatching DIMs: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -155;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to initialize a TopoLayer - non-existing Topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InitTopoLayer('wannebe', NULL, 'elba_vw', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() non-existing Topology: expected failure\n");
	  *retcode = -156;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid topology name.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() non-existing Topology: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -157;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to initialize a TopoLayer - non-existing ref-ìTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InitTopoLayer('elba', NULL, 'wannabe', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() non-existing ref-Table: expected failure\n");
	  *retcode = -158;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_InitTopoLayer: invalid reference Table.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() non-existing Table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -159;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to initialize a TopoLayer - wrong DB-prefix */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InitTopoLayer('elba', 'lollypop', 'elba_vw', 'topolyr')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() wrong DB-prefix: expected failure\n");
	  *retcode = -160;
	  return 0;
      }
    if (strcmp
	(err_msg, "TopoGeo_InitTopoLayer: invalid reference Table.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() wrong DB-prefix: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -161;
	  return 0;
      }
    sqlite3_free (err_msg);

/* initializing a TopoLayer */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InitTopoLayer('elba', NULL, 'elba_vw', 'elba_vw')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_InitTopoLayer error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -162;
      }

/* attempting to initialize a TopoLayer - already existing  */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_InitTopoLayer('elba', NULL, 'elba_vw', 'elba_vw')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() already existing out-table: expected failure\n");
	  *retcode = -163;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_InitTopoLayer: a TopoLayer of the same name already exists.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_InitTopoLayer() already existing out-table: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -164;
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
#ifndef	OMIT_ICONV		/* only if ICONV is enabled */
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
		   "*** check_topoplus skipped: libsqlite < 3.8.3 !!!\n");
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
	  return -4;
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
	  return -5;
      }

/* importing Merano Roads (linestrings) from SHP */
    ret =
	sqlite3_exec (handle,
		      "SELECT ImportSHP('./shp/merano-3d/roads', 'roads', 'UTF-8', 32632)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ImportSHP() roads error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -6;
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

/* adding a second Geometry to Elba-polygons */
    ret =
	sqlite3_exec (handle,
		      "SELECT AddGeometryColumn('elba_pg', 'centroid', 32632, 'POINT', 'XY')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn elba-pg error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -7;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_pg SET centroid = ST_Centroid(geometry)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Update elba-pg centroids error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -7;
      }

/* adding a second Geometry to Roads */
    ret =
	sqlite3_exec (handle,
		      "SELECT AddGeometryColumn('roads', 'wgs', 4326, 'LINESTRING', 'XY')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn roads error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -8;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE roads SET wgs = ST_Transform(geometry, 4326)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Update Roads WGS84 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -9;
      }

/* adding a thirdd Geometry to Roads */
    ret =
	sqlite3_exec (handle,
		      "SELECT AddGeometryColumn('roads', 'g3d', 32632, 'LINESTRING', 'XYZ')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn roads error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -10;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE roads SET g3d = CastToXYZ(geometry)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Update Roads 3D error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -11;
      }

/* creating a Topology 2D (wrong SRID) */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('badelba1', 4326, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -12;
      }

/* creating a Topology 3D (wrong dims) */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('badelba2', 32632, 1, 01)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -13;
      }

/* creating a Topology 2D (ok) */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elba', 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -14;
      }

/* creating a Logical Network */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('loginet', 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -15;
      }

/* creating a Network 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('spatnet', 1, 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -16;
      }

/* creating a Network 2D - wrong SRID */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('badnet1', 1, 3003, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -17;
      }

/* creating a Network 3D */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('badnet2', 1, 32632, 1, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -18;
      }

/* creating a Network 2D */
    ret =
	sqlite3_exec (handle, "SELECT CreateNetwork('roads', 1, 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateNetwork() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -19;
      }

/* creating a View */
    ret =
	sqlite3_exec (handle,
		      "CREATE VIEW elba_vw AS SELECT pk_uid, cod_istat, nome FROM elba_ln",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Create View error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -20;
      }

/* basic tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

/* basic tests: level 00 */
    if (!do_level00_tests (handle, &retcode))
	goto end;

/* basic tests: level 1 */
    if (!do_level1_tests (handle, &retcode))
	goto end;

/* dropping and recreating again a Topology 2D (ok) */
    ret =
	sqlite3_exec (handle, "SELECT DropTopology('elba')", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -15;
      }
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('elba', 32632, 0, 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -16;
      }

/* basic tests: level 2 */
    if (!do_level2_tests (handle, &retcode))
	goto end;

/* basic tests: level 3 */
    if (!do_level3_tests (handle, &retcode))
	goto end;

/* basic tests: level 4 */
    if (!do_level4_tests (handle, &retcode))
	goto end;

/* basic tests: level 5 */
    if (!do_level5_tests (handle, &retcode))
	goto end;

/* basic tests: level 6 */
    if (!do_level6_tests (handle, &retcode))
	goto end;

/* basic tests: level 7 */
    if (!do_level7_tests (handle, &retcode))
	goto end;

/* basic tests: level 8 */
    if (!do_level8_tests (handle, &retcode))
	goto end;

/* basic tests: level 9 */
    if (!do_level9_tests (handle, &retcode))
	goto end;

/* basic tests: level 10 */
    if (!do_level10_tests (handle, &retcode))
	goto end;

/* basic tests: level 11 */
    if (!do_level11_tests (handle, &retcode))
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
