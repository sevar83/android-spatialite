/*

 check_topology3d.c -- SpatiaLite Test Case

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

static int
do_level6_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 6 */
    int ret;
    char *err_msg = NULL;

/* retrieving a Node by Point */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNodeByPoint('topo', MakePointZ(152, 160, 1))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetNodeByPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -300;
	  return 0;
      }

/* attempting to retrieve a Node by Point (two Nodes found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNodeByPoint('topo', MakePoint(152, 160), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetNodeByPoint() #2: expected failure\n");
	  *retcode = -301;
	  return 0;
      }
    if (strcmp (err_msg, "Two or more nodes found") != 0)
      {
	  fprintf (stderr, "GetNodeByPoint() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -302;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a Node by Point (not found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetNodeByPoint('topo', MakePoint(1, 1))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetNodeByPoint() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -303;
	  return 0;
      }

/* retrieving an Edge by Point */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetEdgeByPoint('topo', MakePointZ(154, 167, 1))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetEdgeByPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -304;
	  return 0;
      }

/* attempting to retrieve an Edge by Point (two Edges found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetEdgeByPoint('topo', MakePoint(151, 159), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetEdgeByPoint() #2: expected failure\n");
	  *retcode = -305;
	  return 0;
      }
    if (strcmp (err_msg, "Two or more edges found") != 0)
      {
	  fprintf (stderr, "GetEdgeByPoint() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -306;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve an Edge by Point (not found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetEdgeByPoint('topo', MakePoint(1, 1))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetEdgeByPoint() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -307;
	  return 0;
      }

/* retrieving a Face by Point */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetFaceByPoint('topo', MakePoint(153, 161))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetFaceByPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -308;
	  return 0;
      }

/* attempting to retrieve a Face by Point (two Faces found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetFaceByPoint('topo', MakePoint(149, 149), 3.0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "GetFaceByPoint() #2: expected failure\n");
	  *retcode = -309;
	  return 0;
      }
    if (strcmp (err_msg, "Two or more faces found") != 0)
      {
	  fprintf (stderr, "GetFaceByPoint() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -310;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to retrieve a Face by Point (not found) */
    ret =
	sqlite3_exec (handle,
		      "SELECT GetFaceByPoint('topo', MakePointZ(1, 1, 1))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "GetFaceByPoint() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -311;
	  return 0;
      }

/* adding four Points */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(10, -10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -312;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(25, -10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -313;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(50, -10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -314;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(100, -10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -315;
	  return 0;
      }

/* adding four Linestrings */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -10 1, 100 -10 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -316;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -25 1, 100 -25 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -317;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -50 1, 100 -50 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -318;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -100 1, 100 -100 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -319;
	  return 0;
      }

/* adding four more Points */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(10, -100, 1, 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -320;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(25, -100, 1, 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -321;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(50, -100, 1, 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -322;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(100, -100, 1, 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -323;
	  return 0;
      }

/* adding five more Linestrings */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -10 1, 10 -100 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -324;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(25 -10 1, 25 -100 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -325;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(50 -10 1, 50 -100 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -326;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(100 -10 1, 100 -100 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -327;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('LINESTRINGZ(10 -10 1, 100 -100 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #9 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -328;
	  return 0;
      }

/* adding two more Points (MultiPoint) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', GeomFromText('MULTIPOINTZ(130 -30 1, 90 -30 1)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddPoint() #11 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -329;
	  return 0;
      }

/* adding five more Linestrings (MultiLinestring) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLineString('topo', GeomFromText('MULTILINESTRINGZ((90 -30 1, 130 -30 1), "
		      "(90 -70 1, 130 -70 1), (90 -83 1, 130 -83 1), (90 -30 1, 90 -83 1), (130 -83 1, 130 -30 1))', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLineString() #12 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -330;
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

/* inserting four Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-50, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #20 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -250;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-50, 20, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #21 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -251;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-50, 40, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #22 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -252;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-50, 50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #23 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -253;
	  return 0;
      }

/* inserting Edges */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 18, 19, GeomFromText('LINESTRINGZ(-50 10 1,  -50 20 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #13 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -254;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 21, 20, GeomFromText('LINESTRINGZ(-50 50 1,  -50 40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #14 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -255;
	  return 0;
      }

/* adding more Edges (ModFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 20, 19, GeomFromText('LINESTRINGZ(-50 40 1, -50 20 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #11 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -256;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 18, 21, GeomFromText('LINESTRINGZ(-50 10 1, -100 10 1, -100 50 1, -50 50 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #12 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -257;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 19, 20, GeomFromText('LINESTRINGZ(-50 20 1, -80 20 1, -80 40 1, -50 40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #13 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -258;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 19, 20, GeomFromText('LINESTRINGZ(-50 20 1, -30 20 1, -30 40 1, -50 40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #14 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -259;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 21, 18, GeomFromText('LINESTRINGZ(-50 50 1, -10 50 1, -10 10 1, -50 10 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #15 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -260;
	  return 0;
      }

/* attempting to remove an Edge (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeModFace('topo', 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeModFace() #1: expected failure\n");
	  *retcode = -261;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_RemEdgeModFace() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -262;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove an Edge (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeNewFace('topo', 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeNewFace() #1: expected failure\n");
	  *retcode = -263;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_RemEdgeNewFace() #1: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -264;
	  return 0;
      }
    sqlite3_free (err_msg);

/* removing an Edge (NewFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeNewFace('topo', 17)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeNewFace() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -265;
	  return 0;
      }

/* removing an Edge (NewFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeNewFace('topo', 18)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeNewFace() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -266;
	  return 0;
      }

/* attempting to heal Edges (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeHeal('topo', 14, 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #1: expected failure\n");
	  *retcode = -267;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -268;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeHeal('topo', 333, 14)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #2: expected failure\n");
	  *retcode = -269;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -270;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgeHeal('topo', 14, 333)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #1: expected failure\n");
	  *retcode = -271;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -272;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (non-existing Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgeHeal('topo', 333, 14)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #2: expected failure\n");
	  *retcode = -273;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 333") !=
	0)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -274;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (no common Node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgeHeal('topo', 14, 13)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #3: expected failure\n");
	  *retcode = -275;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected edges") != 0)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -276;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (no common Node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeHeal('topo', 13, 14)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #3: expected failure\n");
	  *retcode = -277;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-connected edges") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -278;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (other Edges connected) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeHeal('topo', 14, 16)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #4: expected failure\n");
	  *retcode = -280;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - other edges connected (19)") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -281;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to heal Edges (other Edges connected) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NeWEdgeHeal('topo', 16, 14)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #4: expected failure\n");
	  *retcode = -282;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - other edges connected (19)") != 0)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -283;
	  return 0;
      }
    sqlite3_free (err_msg);

/* healing Edges (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeHeal('topo', 14, 15)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeHeal() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -284;
	  return 0;
      }

/* healing Edges (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgeHeal('topo', 13, 14)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -285;
	  return 0;
      }

/* removing an Edge (NewFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeNewFace('topo', 20)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeNewFace() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -286;
	  return 0;
      }

/* healing Edges (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgeHeal('topo', 16, 19)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeHeal() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -287;
	  return 0;
      }

/* removing an Edge (ModFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemEdgeModFace('topo', 21)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemEdgeModFace() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -288;
	  return 0;
      }

/* removing a Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNode('topo', 18)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNode() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -289;
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

/* inserting three Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-40, -50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #16 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -150;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-49, -49, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #17 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -151;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-50, -40, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #18 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -152;
	  return 0;
      }

/* inserting an Edge */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 14, 16, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #11 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -153;
	  return 0;
      }

/* moving a Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 15, MakePointZ(-50, -50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -154;
	  return 0;
      }

/* attempting to move a Node (non-existing node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 333, MakePointZ(-51, -51, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #2: expected failure\n");
	  *retcode = -155;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -156;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 15, MakePointZ(-40, -50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #3: expected failure\n");
	  *retcode = -157;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -158;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (edge crosses node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 15, MakePointZ(-45, -45, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #4: expected failure\n");
	  *retcode = -159;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - edge crosses node.") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -160;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (non-isolated node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 14, MakePointZ(-45, -45, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #5: expected failure\n");
	  *retcode = -161;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node") != 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -162;
	  return 0;
      }
    sqlite3_free (err_msg);

/* changing an Edge Geom */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -50 1, -40 -40 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -163;
	  return 0;
      }

/* attempting to change en Edge Geom (curve not simple) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -50 1, -50 -30 1, -40 -30 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #2: expected failure\n");
	  *retcode = -164;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - curve not simple") != 0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -165;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change en Edge Geom (start geometry not geometry start point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -51 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #3: expected failure\n");
	  *retcode = -166;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -167;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change en Edge Geom (end geometry not geometry end point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -50 1, -51 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #4: expected failure\n");
	  *retcode = -168;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -169;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change en Edge Geom (geometry crosses a node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -50 1, -50 -50 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #5: expected failure\n");
	  *retcode = -170;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node")
	!= 0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #5: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -171;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change en Edge Geom (non existing edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 111, GeomFromText('LINESTRINGZ(-40 -50 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #6: expected failure\n");
	  *retcode = -172;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge 111") !=
	0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #6: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -173;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting yet another Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-44, -44, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #19 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -174;
	  return 0;
      }

/* inserting yet another Edge */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 15, 17, GeomFromText('LINESTRINGZ(-50 -50 1, -44 -44 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #12 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -175;
	  return 0;
      }

/* attempting to change en Edge Geom (geometry intersects an edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 11, GeomFromText('LINESTRINGZ(-40 -50 1, -50 -40 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #7: expected failure\n");
	  *retcode = -170;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses edge 12")
	!= 0)
      {
	  fprintf (stderr, "ST_ChangeEdgeGeom() #7: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -171;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a Node (non-isolated) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNode('topo', 15)", NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNode() #1: expected failure\n");
	  *retcode = -172;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not isolated node") != 0)
      {
	  fprintf (stderr, "ST_RemIsoNode() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -173;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to remove a Node (not existing) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNode('topo', 155)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNode() #2: expected failure\n");
	  *retcode = -174;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_RemIsoNode() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -175;
	  return 0;
      }
    sqlite3_free (err_msg);

/* removing a Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_RemIsoNode('topo', 5)", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_RemIsoNode() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -176;
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

/* inserting a first Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(150, 150, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #9 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -70;
	  return 0;
      }

/* inserting a second Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(180, 180, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #10 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }

/* inserting an Edge */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 8, 9, GeomFromText('LINESTRINGZ(150 150 1,  180 150 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -72;
	  return 0;
      }

/* adding an Edge (ModFace) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 8, 9, GeomFromText('LINESTRINGZ(150 150 1, 150 180 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
	  return 0;
      }

/* testing Face Geometry */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceGeometry('topo', 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -74;
	  return 0;
      }

/* attempting to test Face Geometry (non-existing face) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceGeometry('topo', 111)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #2: expected failure\n");
	  *retcode = -75;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent face.") != 0)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -76;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to test Face Geometry (universe face) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceGeometry('topo', 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #3: expected failure\n");
	  *retcode = -77;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - universal face has no geometry") != 0)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -78;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing Face Edges */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceEdges('topo', 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceEdges() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -79;
	  return 0;
      }

/* attempting to add an invalid Edge (curve not simple) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 8, 9, GeomFromText('LINESTRINGZ(150 150 1, 170 150 1, 160 150 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #2: expected failure\n");
	  *retcode = -80;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - curve not simple") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -81;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (non-existent start node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 18, 9, GeomFromText('LINESTRINGZ(150 150 1, 150 180 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #3: expected failure\n");
	  *retcode = -82;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -83;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (non-existent end node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 8, 19, GeomFromText('LINESTRINGZ(150 150 1, 150 180 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #4: expected failure\n");
	  *retcode = -84;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -85;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (start node not geometry start point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 8, 9, GeomFromText('LINESTRINGZ(151 150 1, 150 180 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #5: expected failure\n");
	  *retcode = -86;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #5: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -87;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (end node not geometry end point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 8, 9, GeomFromText('LINESTRINGZ(150 150 1, 150 180 1, 181 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #6: expected failure\n");
	  *retcode = -88;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #6: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -89;
	  return 0;
      }
    sqlite3_free (err_msg);

/* adding an Edge (NewFaces) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 8, 9, GeomFromText('LINESTRINGZ(150 150 1, 180 180 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }

/* inserting three Nodes within a Face */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(152, 160, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #11 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -91;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(154, 160, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #12 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -92;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', 3, MakePointZ(152, 170, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #12 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -93;
	  return 0;
      }

/* inserting a Node within the other Face */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(178, 170, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #13 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -94;
	  return 0;
      }

/* attempting to add an invalid Node (node crosses edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(178, 178, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #14: expected failure\n");
	  *retcode = -95;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - edge crosses node.") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() #14: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -96;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Node (not within face) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', 3, MakePointZ(179, 160, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #15: expected failure\n");
	  *retcode = -97;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not within face") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() #15: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -98;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting an Edge within a Face */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -99;
	  return 0;
      }

/* attempting to add an invalid Edge (nodes in different faces) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 12, 13, GeomFromText('LINESTRINGZ(152 170 1, 178 170 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #9: expected failure\n");
	  *retcode = -100;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - nodes in different faces") != 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #9: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -101;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (geometry crosses a Node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 152 170 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #7: expected failure\n");
	  *retcode = -102;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node")
	!= 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #7: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -103;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (geometry crosses an Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 153 161 1, 153 159 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #8: expected failure\n");
	  *retcode = -104;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses edge 8")
	!= 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #8: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -105;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (coincident Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 11, 10, GeomFromText('LINESTRINGZ(154 160 1, 152 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #9: expected failure\n");
	  *retcode = -106;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident edge 8") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #9: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -107;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (curve not simple) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 154 162 1, 152 162 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #2: expected failure\n");
	  *retcode = -108;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - curve not simple") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -109;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (non-existent start node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 18, 11, GeomFromText('LINESTRINGZ(152 160 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #3: expected failure\n");
	  *retcode = -110;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -111;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (non-existent end node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 19, GeomFromText('LINESTRINGZ(152 160 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #4: expected failure\n");
	  *retcode = -112;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent node") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -113;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (start node not geometry start point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 11, GeomFromText('LINESTRINGZ(52 160 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #5: expected failure\n");
	  *retcode = -114;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #5: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -115;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (end node not geometry end point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 54 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #6: expected failure\n");
	  *retcode = -116;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #6: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -117;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (geometry crosses a Node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 152 170 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #7: expected failure\n");
	  *retcode = -118;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node")
	!= 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #7: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -119;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (geometry crosses an Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 10, 11, GeomFromText('LINESTRINGZ(152 160 1, 153 161 1, 153 159 1, 154 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #8: expected failure\n");
	  *retcode = -120;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses edge 8")
	!= 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #8: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -121;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an invalid Edge (coincident Edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 11, 10, GeomFromText('LINESTRINGZ(154 160 1, 152 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #9: expected failure\n");
	  *retcode = -122;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident edge 8") != 0)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #9: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -123;
	  return 0;
      }
    sqlite3_free (err_msg);

/* creating new Faces (hole) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 11, 10, GeomFromText('LINESTRINGZ(154 160 1, 154 167 1, 152 167 1, 152 160 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeNewFaces() #10 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -124;
	  return 0;
      }

/* attempting to add an invalid Edge (closed ring) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 13, 13, GeomFromText('LINESTRINGZ(178 170 1, 178 161 1, 170 161 1, 178 170 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #10: expected failure\n");
	  *retcode = -125;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "Closed edges would not be isolated, try rtt_AddEdgeNewFaces") != 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #10: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -126;
	  return 0;
      }
    sqlite3_free (err_msg);

/* adding an Edge/Face (closed ring) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 13, 13, GeomFromText('LINESTRINGZ(178 170 1, 178 161 1, 170 161 1, 178 170 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddEdgeModFace() #10 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -127;
	  return 0;
      }

/* testing Face Geometry (Faces with internal hole) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceGeometry('topo', 6)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -128;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceGeometry('topo', 4)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceGeometry() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -129;
	  return 0;
      }

/* testing Face Edges (Faces with internal hole) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceEdges('topo', 4)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceEdges() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -130;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_GetFaceEdges('topo', 6)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_GetFaceEdges() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -131;
	  return 0;
      }

    return 1;
}

static int
do_level2_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 2 */
    int ret;
    char *err_msg = NULL;

/* splitting an Edge (Mod) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 1, MakePointZ(0, 50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -50;
	  return 0;
      }

/* attempting to split an Edge (non-existent edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 100, MakePointZ(0, 25, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #2: expected failure\n");
	  *retcode = -51;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -52;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (point not on edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 2, MakePointZ(0, 25, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #3: expected failure\n");
	  *retcode = -53;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - point not on edge") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -54;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 1, MakePointZ(0, 50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #4: expected failure\n");
	  *retcode = -55;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -56;
	  return 0;
      }
    sqlite3_free (err_msg);

/* splitting an Edge (New) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 2, MakePointZ(0, 75, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgeSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }

/* attempting to split an Edge (non-existent edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 100, MakePointZ(0, 25, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgesSplit() #2: expected failure\n");
	  *retcode = -58;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - non-existent edge") != 0)
      {
	  fprintf (stderr, "ST_NewEdgesSplit() #2: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -59;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (point not on edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 3, MakePointZ(0, 25, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgesSplit() #3: expected failure\n");
	  *retcode = -60;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - point not on edge") != 0)
      {
	  fprintf (stderr, "ST_ModEdgeSplit() #3: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (coincident node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 1, MakePointZ(0, 50, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_NewEdgesSplit() #4: expected failure\n");
	  *retcode = -62;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node") != 0)
      {
	  fprintf (stderr, "ST_NewEdgesSplit() #4: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

static int
do_level1_tests (sqlite3 * handle, int *retcode)
{
/* performing basic tests: Level 1 */
    int ret;
    char *err_msg = NULL;

/* inserting a first Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(0, 0, 2, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -10;
	  return 0;
      }

/* inserting a second Node */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(0, 100, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -11;
	  return 0;
      }

/* attempting to insert an invalid Edge (curve not simple) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 1, 2, GeomFromText('LINESTRINGZ(0 0 1, 0 90 1, 10 90 1, 10 80 1, 0 80 1, 0 100 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #1: expected failure\n");
	  *retcode = -12;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - curve not simple") != 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #1: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -13;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert an invalid Edge (start node not geometry start point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 1, 2, GeomFromText('LINESTRINGZ(1 0 1, 0 100 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #2: expected failure\n");
	  *retcode = -14;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - start node not geometry start point.") !=
	0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #2: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -15;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert an invalid Edge (end node not geometry end point) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 1, 2, GeomFromText('LINESTRINGZ(0 0 1, 1 100 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #3: expected failure\n");
	  *retcode = -16;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - end node not geometry end point.") != 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -17;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting an Edge */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 1, 2, GeomFromText('LINESTRINGZ(0 0 1, 0 100 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -18;
	  return 0;
      }

/* attempting to insert an invalid Node (coincident) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(0, 0, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #3: expected failure\n");
	  *retcode = -19;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - coincident node") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() #3: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -20;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert an invalid Node (edge crosses node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(0, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #4: expected failure\n");
	  *retcode = -21;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - edge crosses node.") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() #4: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -22;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert an invalid Node (not within face) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', 1, MakePointZ(10, 10, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #5: expected failure\n");
	  *retcode = -23;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - not within face") != 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -24;
	  return 0;
      }
    sqlite3_free (err_msg);

/* inserting more Nodes */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(-1, 99, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -25;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(1, 101, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #7 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -26;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(1, 98, 1, 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() #8 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -26;
	  return 0;
      }

/* attempting to insert an invalid Edge (geometry crosses a node) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 3, 4, GeomFromText('LINESTRINGZ(-1 99 1, 1 101 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #5: expected failure\n");
	  *retcode = -27;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses a node")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #5: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -28;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert an invalid Edge (geometry intersects an edge) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 3, 5, GeomFromText('LINESTRINGZ(-1 99 1, 1 98 1)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #6: expected failure\n");
	  *retcode = -29;
	  return 0;
      }
    if (strcmp (err_msg, "SQL/MM Spatial exception - geometry crosses edge 1")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() #6: unexpected \"%s\"\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -30;
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

/* attempting to insert a Node (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() invalid SRID: expected failure\n");
	  *retcode = -190;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -191;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to insert a Node (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoNode('topo', NULL, MakePoint(0, 10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoNode() invalid SRID: expected failure\n");
	  *retcode = -192;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -193;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: expected failure\n");
	  *retcode = -194;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -195;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to move a Node (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_MoveIsoNode('topo', 1, MakePoint(0, 10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: expected failure\n");
	  *retcode = -196;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -197;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 14, 16, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() invalid SRID: expected failure\n");
	  *retcode = -198;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -199;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddIsoEdge('topo', 14, 16, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_AddIsoEdge() invalid SRID: expected failure\n");
	  *retcode = -200;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr, "ST_AddIsoEdge() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -201;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 1, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeEdgeGeom() invalid SRID: expected failure\n");
	  *retcode = -202;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeEdgeGeom() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -203;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to change an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ChangeEdgeGeom('topo', 1, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_ChangeEdgeGeom() invalid SRID: expected failure\n");
	  *retcode = -204;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ChangeEdgeGeom() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -205;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: expected failure\n");
	  *retcode = -206;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModEdgeSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -207;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ModEdgeSplit('topo', 1, MakePoint(0, 10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "ST_MoveIsoNode() invalid SRID: expected failure\n");
	  *retcode = -208;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_ModEdgeSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -209;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 1, MakePointZ(0, 10, 1, 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewEdgesSplit() invalid SRID: expected failure\n");
	  *retcode = -210;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewEdgesSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -211;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to split an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_NewEdgesSplit('topo', 1, MakePoint(0, 10, 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_NewEdgesSplit() invalid SRID: expected failure\n");
	  *retcode = -212;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_NewEdgesSplit() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -213;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 1, 2, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddEdgeModFace() invalid SRID: expected failure\n");
	  *retcode = -214;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddEdgeModFace() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -215;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeModFace('topo', 1, 2, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddEdgeModFace() invalid SRID: expected failure\n");
	  *retcode = -216;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddEdgeModFace() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -217;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 1, 2, GeomFromText('LINESTRINGZ(-40 -50 1,  -50 -40 2)', 3003))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddEdgeNewFaces() invalid SRID: expected failure\n");
	  *retcode = -218;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddEdgeNewFaces() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -221;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add an Edge (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_AddEdgeNewFaces('topo', 1, 2, GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326))",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "ST_AddEdgeNewFaces() invalid SRID: expected failure\n");
	  *retcode = -220;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "ST_AddEdgeNewFaces() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -221;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add a Point (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePointZ(1, 1, 1, 3003), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddPoint() invalid SRID: expected failure\n");
	  *retcode = -222;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddPoint() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -223;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add a Point (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddPoint('topo', MakePoint(1, 1, 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddPoint() invalid SRID: expected failure\n");
	  *retcode = -224;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddPoint() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -225;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add a Linestring (invalid SRID) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLinestring('topo', GeomFromText('LINESTRINGZ(-40 -50 1, -50 -40 1)', 3003), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLinestring() invalid SRID: expected failure\n");
	  *retcode = -226;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLinestring() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -227;
	  return 0;
      }
    sqlite3_free (err_msg);

/* attempting to add a Linestring (invalid DIMs) */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_AddLinestring('topo', GeomFromText('LINESTRING(-40 -50,  -50 -40)', 4326), 0)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLinestring() invalid SRID: expected failure\n");
	  *retcode = -228;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_AddLinestring() invalid SRID: unexpected \"%s\"\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -229;
	  return 0;
      }
    sqlite3_free (err_msg);

    return 1;
}

#endif

int
main (int argc, char *argv[])
{
    int retcode = 0;

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

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
		   "*** check_topology3d skipped: libsqlite < 3.8.3 !!!\n");
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

/* creating a Topology 3D */
    ret =
	sqlite3_exec (handle, "SELECT CreateTopology('topo', 4326, 1, 0)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* basic tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

/* basic tests: level 1 */
    if (!do_level1_tests (handle, &retcode))
	goto end;

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

/* dropping the Topology 3D */
    ret =
	sqlite3_exec (handle, "SELECT DropTopology('topo')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DropTopology() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -5;
      }

  end:
    spatialite_finalize_topologies (cache);
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);

#endif /* end RTTOPO conditional */

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    return retcode;
}
