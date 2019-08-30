/*

 check_virtualknn.c -- SpatiaLite Test Case

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
 
Portions created by the Initial Developer are Copyright (C) 2015
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
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

#ifndef OMIT_GEOS		/* GEOS is supported */
#ifndef OMIT_KNN		/* only if KNN is enabled */

static int
create_table (sqlite3 * sqlite)
{
/* creating a test table */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql = "CREATE TABLE points (id INTEGER PRIMARY KEY AUTOINCREMENT)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE TABLE \"points\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql = "SELECT AddGeometryColumn('points', 'geom', 32632, 'POINT', 'XY')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"points.geom\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql = "SELECT CreateSpatialIndex('points', 'geom')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"points.geom\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
populate_table (sqlite3 * sqlite)
{
/* creating a test table */
    int ret;
    char *err_msg = NULL;
    const char *sql;
    sqlite3_stmt *stmt;
    double x;
    double y;

    sql = "INSERT INTO points VALUES (NULL, MakePoint(?, ?, 32632))";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

    ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    for (y = 4000000.0; y < 4001000.0; y += 20.0)
      {
	  for (x = 100000.0; x < 101000.0; x += 20.0)
	    {
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_double (stmt, 1, x);
		sqlite3_bind_double (stmt, 2, y);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      fprintf (stderr, "INSERT error: %s\n",
			       sqlite3_errmsg (sqlite));
		      goto end;
		  }
	    }
      }
    for (y = 4000501.0; y < 4001000.0; y += 10.0)
      {
	  for (x = 100501.0; x < 101000.0; x += 10.0)
	    {
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_double (stmt, 1, x);
		sqlite3_bind_double (stmt, 2, y);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      fprintf (stderr, "INSERT error: %s\n",
			       sqlite3_errmsg (sqlite));
		      goto end;
		  }
	    }
      }
    for (y = 4000750.5; y < 4001000.0; y += 5.0)
      {
	  for (x = 100750.5; x < 101000.0; x += 5.0)
	    {
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_double (stmt, 1, x);
		sqlite3_bind_double (stmt, 2, y);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      fprintf (stderr, "INSERT error: %s\n",
			       sqlite3_errmsg (sqlite));
		      goto end;
		  }
	    }
      }

    ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "COMMIT TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

  end:
    sqlite3_finalize (stmt);
    return 1;
}

static int
add_second_geom (sqlite3 * sqlite)
{
/* adding a second geometry column */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql = "SELECT AddGeometryColumn('points', 'geometry', 4326, 'POINT', 'XY')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "AddGeometryColumn \"points.geometry\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql = "UPDATE points SET geometry = ST_Transform(geom, 4326)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE \"knn\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
add_second_rtree (sqlite3 * sqlite)
{
/* adding a second geometry Spatial Index */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql = "SELECT CreateSpatialIndex('points', 'geometry')";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CreateSpatialIndex \"points.geometry\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
create_knn (sqlite3 * sqlite)
{
/* creating a test table */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql = "CREATE VIRTUAL TABLE knn USING VirtualKNN ()";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE VIRTUAL TABLE \"knn\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
create_spatial_view_1 (sqlite3 * sqlite)
{
/* creating the first Spatial View */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql = "CREATE VIEW view_1 AS SELECT id AS rowid, geom AS geom FROM points";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE VIEW \"view_1\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	"INSERT INTO views_geometry_columns VALUES('view_1', 'geom', 'rowid', 'points', 'geom', 1)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Register SpatialView \"view_1\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
create_spatial_view_2 (sqlite3 * sqlite)
{
/* creating the second Spatial View */
    int ret;
    char *err_msg = NULL;
    const char *sql;

    sql =
	"CREATE VIEW view_2 AS SELECT id AS rowid, geometry AS geom FROM points";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE VIEW \"view_2\" error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    sql =
	"INSERT INTO views_geometry_columns VALUES('view_2', 'geom', 'rowid', 'points', 'geometry', 1)";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Register SpatialView \"view_2\" error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
test_knn (sqlite3 * sqlite, int mode)
{
/* testing a resultset */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    double x;
    double y;
    int rows = 0;

    switch (mode)
      {
      case 0:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'DB=main.points' AND ref_geometry = MakePoint(?, ?)";
	  break;
      case 1:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'points' AND f_geometry_column = 'geom' "
	      "AND ref_geometry = MakePoint(?, ?) AND max_items = 32632";
	  break;
      case 2:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'points' AND f_geometry_column = 'geomx' "
	      "AND ref_geometry = MakePoint(?, ?)";
	  break;
      case 3:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'pointsx' AND ref_geometry = MakePoint(?, ?)";
	  break;
      case 4:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'points' AND f_geometry_column = 'geometry' "
	      "AND ref_geometry = ST_Transform(MakePoint(?, ?, 32632), 4326) AND max_items = -10";
	  break;
      case 5:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'view_1' AND ref_geometry = MakePoint(?, ?)";
	  break;
      case 6:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'view_2' AND f_geometry_column = 'geom' "
	      "AND ref_geometry = ST_Transform(MakePoint(?, ?, 32632), 4326)";
	  break;
      case 7:
	  sql =
	      "SELECT * FROM knn WHERE f_table_name = 'points' AND ref_geometry = MakePoint(?, ?) "
	      "AND max_items = 10";
	  break;
      };
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "SELECT FROM \"knn\": \"%s\"\n",
		   sqlite3_errmsg (sqlite));
	  return 0;
      }

    for (y = 3800000.25; y < 4002000.0; y += 12345.12345)
      {
	  for (x = 80000.25; x < 102000.0; x += 12345.12345)
	    {
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_double (stmt, 1, x);
		sqlite3_bind_double (stmt, 2, y);
		while (1)
		  {
		      /* scrolling the result set rows */
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT
				&& sqlite3_column_type (stmt, 1) == SQLITE_TEXT
				&& sqlite3_column_type (stmt, 2) == SQLITE_BLOB
				&& sqlite3_column_type (stmt,
							3) == SQLITE_INTEGER
				&& sqlite3_column_type (stmt,
							4) == SQLITE_INTEGER
				&& sqlite3_column_type (stmt,
							5) == SQLITE_INTEGER
				&& sqlite3_column_type (stmt,
							6) == SQLITE_FLOAT)
				;
			    else
				goto error;
			    rows++;
			}
		      else
			  goto error;
		  }
	    }
      }
    if (!rows)
	goto error;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

#endif
#endif

int
main (int argc, char *argv[])
{
    sqlite3 *db_handle = NULL;
    int ret;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &db_handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (db_handle));
	  sqlite3_close (db_handle);
	  db_handle = NULL;
	  return -1;
      }

    spatialite_init_ex (db_handle, cache, 0);

#ifndef OMIT_GEOS		/* GEOS is supported */
#ifndef OMIT_KNN		/* only if KNN is enabled */

    ret =
	sqlite3_exec (db_handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (db_handle);
	  return -2;
      }

/* Creating and populating the test table */
    ret = create_table (db_handle);
    if (!ret)
      {
	  sqlite3_close (db_handle);
	  return -3;
      }
    ret = populate_table (db_handle);
    if (!ret)
      {
	  sqlite3_close (db_handle);
	  return -4;
      }

/* Creating the VirtualKNN table */
    ret = create_knn (db_handle);
    if (ret)
      {
	  fprintf (stderr, "CREATE VIRTUAL TABLE knn: expected failure !!!\n");
	  sqlite3_close (db_handle);
	  return -5;
      }

/* Testing KNN - #1 */
    ret = test_knn (db_handle, 0);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #1: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -6;
      }

/* Testing KNN - #2 */
    ret = test_knn (db_handle, 1);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #2: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -7;
      }

/* Testing KNN - #3 */
    ret = test_knn (db_handle, 2);
    if (ret)
      {
	  fprintf (stderr, "Check KNN #3: unexpected success\n");
	  sqlite3_close (db_handle);
	  return -8;
      }

/* creating a first SpatialView */
    ret = create_spatial_view_1 (db_handle);
    if (!ret)
      {
	  fprintf (stderr, "Create Spatial View #1: unexpected failure !!!\n");
	  sqlite3_close (db_handle);
	  return -9;
      }

/* Testing KNN - #4 */
    ret = test_knn (db_handle, 5);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #4: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -10;
      }

/* Testing KNN - #5 */
    ret = test_knn (db_handle, 7);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #5: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -11;
      }

/* adding a second geometry column */
    ret = add_second_geom (db_handle);
    if (!ret)
      {
	  fprintf (stderr, "Add Second Geometry: unexpected failure !!!\n");
	  sqlite3_close (db_handle);
	  return -12;
      }

/* Testing KNN - #6 */
    ret = test_knn (db_handle, 3);
    if (ret)
      {
	  fprintf (stderr, "Check KNN #6: unexpected success\n");
	  sqlite3_close (db_handle);
	  return -13;
      }

/* Testing KNN - #7 */
    ret = test_knn (db_handle, 1);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #7: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -14;
      }

/* Testing KNN - #8 */
    ret = test_knn (db_handle, 4);
    if (ret)
      {
	  fprintf (stderr, "Check KNN #8: unexpected success\n");
	  sqlite3_close (db_handle);
	  return -15;
      }

/* creating a second SpatialIndex */
    ret = add_second_rtree (db_handle);
    if (!ret)
      {
	  fprintf (stderr,
		   "Add Second Spatial Index: unexpected failure !!!\n");
	  sqlite3_close (db_handle);
	  return -16;
      }

/* Testing KNN - #9 */
    ret = test_knn (db_handle, 4);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #9: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -17;
      }

/* creating a second SpatialView */
    ret = create_spatial_view_2 (db_handle);
    if (!ret)
      {
	  fprintf (stderr, "Create Spatial View #2: unexpected failure !!!\n");
	  sqlite3_close (db_handle);
	  return -18;
      }

/* Testing KNN - #10 */
    ret = test_knn (db_handle, 6);
    if (!ret)
      {
	  fprintf (stderr, "Check KNN #10: unexpected failure\n");
	  sqlite3_close (db_handle);
	  return -19;
      }

#endif /* end KNN conditional */
#endif /* end GEOS conditional */

    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();

    return 0;
}
