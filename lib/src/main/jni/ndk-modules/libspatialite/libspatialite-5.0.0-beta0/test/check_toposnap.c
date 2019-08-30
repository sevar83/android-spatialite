/*

 check_toposnap.c -- SpatiaLite Test Case

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

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 0 tests */
    int ret;
    char *err_msg = NULL;

/* loading the sezcen_2011 GeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_FromGeoTableNoFace('elba', 'ext', 'sezcen_2011', NULL, 512)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_FromGeoTableNoFace() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -21;
	  return 0;
      }

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -22;
	  return 0;
      }

/* removing useless Nodes - mode Mod */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ModEdgeHeal('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ModEdgeHeal() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -23;
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

/* importing the sezcen_2001 GeoTable */
    ret = sqlite3_exec (handle,
			"SELECT TopoGeo_AddLinestringNoFace('elba', LinesFromRings(TopoGeo_TopoSnap('elba', geometry, 1, 1, 0))) "
			"FROM ext.sezcen_2001 WHERE rowid < 5", NULL, NULL,
			&err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_AddLinestringNoFace() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -31;
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

/* building Faces */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_Polygonize('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_Polygonize() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }

/* validating this TopoGeo */
    ret =
	sqlite3_exec (handle,
		      "SELECT ST_ValidateTopoGeo('elba')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ValidateTopoGeo() #1: error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -52;
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
	  *retcode = -53;
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
	  fprintf (stderr, "Topology 'elba' #2 is invalid !!!");
	  *retcode = -54;
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

/* removing useless Nodes - mode Mod */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ModEdgeHeal('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ModEdgeHeal() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }

/* splitting Edges - mode Mod */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ModEdgeSplit('elba', 128)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ModEdgeSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }

/* removing useless Nodes - mode New */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_NewEdgeHeal('elba')", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_NewEdgeHeal() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }

/* splitting Edges - mode New */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_NewEdgesSplit('elba', 256)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_NewEdgesSplit() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }

    return 1;
}

static int
do_level4_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 4 tests */
    int ret;
    char *err_msg = NULL;

/* disabled: indecently slow !!! */
    return 1;

/* testing TopoGeo_SnappedGeoTable */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_SnappedGeoTable('elba', 'ext', 'sezcen_2001', NULL, 'snapped_2001', 1, 1)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnappedGeoTable() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }

    return 1;
}

static int
do_level5_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 5 tests  - INVALID CASES */
    int ret;
    char *err_msg = NULL;

/* testing TopoGeo_SnappedGeoTable - invalid input table */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_SnappedGeoTable('elba', 'ext', 'sezcen_1234', NULL, 'snapped', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnappedGeoTable() #2 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -60;
	  return 0;
      }
    if (strcmp
	(err_msg, "SQL/MM Spatial exception - invalid input GeoTable.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnappedGeoTable() #3 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -61;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_SnappedGeoTable -already existing output table
 * disabled - indecently slow !!!
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_SnappedGeoTable('elba', 'ext', 'sezcen_2001', NULL, 'snapped_2001', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnappedGeoTable() #4 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_SnappedGeoTable: output GeoTable already exists.") != 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnappedGeoTable() #5 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    sqlite3_free (err_msg); 
*/

/* testing TopoGeo_SnappedGeoTable -invalid input SRID or dimensions */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_SnappedGeoTable('elba', 'ext', 'points', NULL, 'snapped', 1)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_SnappedGeoTable() #6 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -62;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_SnappedGeoTable() #7 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    sqlite3_free (err_msg);

/* invalidating all Edges */
    ret =
	sqlite3_exec (handle,
		      "UPDATE elba_edge SET left_face = NULL, right_face = NULL",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "invalidating Edges error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -64;
	  return 0;
      }

/* removing all Faces except the Universal Face */
    ret =
	sqlite3_exec (handle,
		      "DELETE FROM elba_face WHERE face_id <> 0", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "removing Faces error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -65;
	  return 0;
      }

/* testing TopoGeo_NewEdgeHeal - inconsistent topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_NewEdgeHeal('elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_NewEdgeHeal() #2 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -66;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_NewEdgeHeal exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_NewEdgeHeal() #2 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -67;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing ModEdgeHeal - inconsistent topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ModEdgeHeal('elba')",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ModEdgeHeal() #2 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -68;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ModEdgeHeal exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ModEdgeHeal() #2 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -69;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing TopoGeo_NewEdgesSplit - inconsistent topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_NewEdgesSplit('elba', 256)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_NewEdgesSplit() #2 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -70;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_NewEdgesSplit exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_NewEdgesSplit() #2 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -71;
	  return 0;
      }
    sqlite3_free (err_msg);

/* testing ModEdgeSplit - inconsistent topology */
    ret =
	sqlite3_exec (handle,
		      "SELECT TopoGeo_ModEdgeSplit('elba', 256)",
		      NULL, NULL, &err_msg);
    if (ret == SQLITE_OK)
      {
	  fprintf (stderr, "TopoGeo_ModEdgeSplit() #2 unexpected succes\n");
	  sqlite3_free (err_msg);
	  *retcode = -72;
	  return 0;
      }
    if (strcmp
	(err_msg,
	 "TopoGeo_ModEdgeSplit exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.")
	!= 0)
      {
	  fprintf (stderr,
		   "TopoGeo_ModEdgeSplit() #2 - unexpected: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -73;
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
		   "*** check_toposnap skipped: libsqlite < 3.8.3 !!!\n");
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
		      "SELECT CreateTopology('elba', 32632, 0, 0.000001)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateTopology() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* attaching an external DB */
    ret =
	sqlite3_exec (handle,
		      "ATTACH DATABASE \"./elba-sezcen.sqlite\" AS ext", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -5;
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

/*tests: level 5 */
    if (!do_level5_tests (handle, &retcode))
	goto end;

/* detaching the external DB */
    ret = sqlite3_exec (handle, "DETACH DATABASE ext", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DETACH DATABASE error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -6;
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
