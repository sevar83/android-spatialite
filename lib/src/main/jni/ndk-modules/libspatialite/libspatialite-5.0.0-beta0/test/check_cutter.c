/*

 check_cutter.c -- SpatiaLite Test Case

 uthor: Sandro Furieri <a.furieri@lqt.it>

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

#ifndef OMIT_GEOS		/* only if GEOS is enabled */

static void
do_get_cutter_message (sqlite3 * sqlite, const char *test_sql)
{
/* reporting the last Cutter Message */
    int ret;
    const char *sql = "SELECT GetCutterMessage()";
    sqlite3_stmt *stmt = NULL;

    fprintf (stderr,
	     "Failed ST_Cutter() test: %s\nGetCutterMessage() reports: ",
	     test_sql);

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "\n");
	  return;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *msg =
			  (const char *) sqlite3_column_text (stmt, 0);
		      fprintf (stderr, "%s", msg);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    fprintf (stderr, "\n");
}

static int
test_query (sqlite3 * sqlite, const char *sql)
{
/* testing some SQL query returning an INT value */
    int ret;
    int ok = 0;
    sqlite3_stmt *stmt = NULL;
    fprintf (stderr, "\n\n%s\n", sql);

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "%s\n: \"%s\"\n", sql, sqlite3_errmsg (sqlite));
	  return 0;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		  {
		      if (sqlite3_column_int (stmt, 0) >= 1)
			  ok = 1;
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    if (ok != 1)
	do_get_cutter_message (sqlite, sql);
    return ok;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
check_cutter_main (sqlite3 * handle, int *retcode)
{
/* testing ST_Cutter - MAIN DB */
    const char *sql;
    int ret;

/* cutting Points XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xy', NULL, NULL, 'blades_xyzm', NULL, 'out_points_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 1;
	  return 0;
      }

/* cutting Points XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xy', NULL, NULL, 'blades_xyz', NULL, 'out_points_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 2;
	  return 0;
      }

/* cutting Points XY - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xy', NULL, NULL, 'blades_xym', NULL, 'out_points_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 3;
	  return 0;
      }

/* cutting Points XY - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'points_xy', 'geometry', 'main', 'blades_xy', 'geometry', 'out_points_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 4;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xy', NULL, NULL, 'blades_xyzm', NULL, 'out_lines_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 5;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xy', NULL, NULL, 'blades_xyz', NULL, 'out_lines_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 6;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xy', NULL, NULL, 'blades_xym', NULL, 'out_lines_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 7;
	  return 0;
      }

/* cutting Linestrings XY - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'lines_xy', 'geometry', 'main', 'blades_xy', 'geometry', 'out_lines_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 8;
	  return 0;
      }

/* cutting Polygons XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xy', NULL, NULL, 'blades_xyzm', NULL, 'out_polygs_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 9;
	  return 0;
      }

/* cutting Polygons XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xy', NULL, NULL, 'blades_xyz', NULL, 'out_polygs_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 10;
	  return 0;
      }

/* cutting Polygons XY - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xy', NULL, NULL, 'blades_xym', NULL, 'out_polygs_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 11;
	  return 0;
      }

/* cutting Polygons XY - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'polygs_xy', 'geometry', 'main', 'blades_xy', 'geometry', 'out_polygs_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 12;
	  return 0;
      }

/* cutting Points XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyz', NULL, NULL, 'blades_xyzm', NULL, 'out_points_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 13;
	  return 0;
      }

/* cutting Points XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyz', NULL, NULL, 'blades_xyz', NULL, 'out_points_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 14;
	  return 0;
      }

/* cutting Points XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyz', NULL, NULL, 'blades_xym', NULL, 'out_points_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 15;
	  return 0;
      }

/* cutting Points XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'points_xyz', 'geometry', 'main', 'blades_xy', 'geometry', 'out_points_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 16;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyz', NULL, NULL, 'blades_xyzm', NULL, 'out_lines_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 17;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyz', NULL, NULL, 'blades_xyz', NULL, 'out_lines_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 18;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyz', NULL, NULL, 'blades_xym', NULL, 'out_lines_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 19;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'lines_xyz', 'geometry', 'main', 'blades_xy', 'geometry', 'out_lines_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 20;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyz', NULL, NULL, 'blades_xyzm', NULL, 'out_polygs_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 21;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyz', NULL, NULL, 'blades_xyz', NULL, 'out_polygs_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 22;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyz', NULL, NULL, 'blades_xym', NULL, 'out_polygs_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 23;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'polygs_xyz', 'geometry', 'main', 'blades_xy', 'geometry', 'out_polygs_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 24;
	  return 0;
      }

/* cutting Points XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xym', NULL, NULL, 'blades_xyzm', NULL, 'out_points_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 25;
	  return 0;
      }

/* cutting Points XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xym', NULL, NULL, 'blades_xyz', NULL, 'out_points_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 26;
	  return 0;
      }

/* cutting Points XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xym', NULL, NULL, 'blades_xym', NULL, 'out_points_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 27;
	  return 0;
      }

/* cutting Points XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'points_xym', 'geometry', 'main', 'blades_xy', 'geometry', 'out_points_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 28;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xym', NULL, NULL, 'blades_xyzm', NULL, 'out_lines_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 29;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xym', NULL, NULL, 'blades_xyz', NULL, 'out_lines_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 30;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xym', NULL, NULL, 'blades_xym', NULL, 'out_lines_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 31;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'lines_xym', 'geometry', 'main', 'blades_xy', 'geometry', 'out_lines_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 32;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xym', NULL, NULL, 'blades_xyzm', NULL, 'out_polygs_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 33;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xym', NULL, NULL, 'blades_xyz', NULL, 'out_polygs_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 34;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xym', NULL, NULL, 'blades_xym', NULL, 'out_polygs_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 35;
	  return 0;
      }

/* cutting Polygons XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'polygs_xym', 'geometry', 'main', 'blades_xy', 'geometry', 'out_polygs_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 36;
	  return 0;
      }

/* cutting Points XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyzm', NULL, NULL, 'blades_xyzm', NULL, 'out_points_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 37;
	  return 0;
      }

/* cutting Points XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyzm', NULL, NULL, 'blades_xyz', NULL, 'out_points_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 38;
	  return 0;
      }

/* cutting Points XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'points_xyzm', NULL, NULL, 'blades_xym', NULL, 'out_points_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 39;
	  return 0;
      }

/* cutting Points XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'points_xyzm', 'geometry', 'main', 'blades_xy', 'geometry', 'out_points_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 40;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyzm', NULL, NULL, 'blades_xyzm', NULL, 'out_lines_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 41;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyzm', NULL, NULL, 'blades_xyz', NULL, 'out_lines_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 42;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'lines_xyzm', NULL, NULL, 'blades_xym', NULL, 'out_lines_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 43;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'lines_xyzm', 'geometry', 'main', 'blades_xy', 'geometry', 'out_lines_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 44;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyzm', 'geometry', NULL, 'blades_xyzm', NULL, 'out_polygs_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 45;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyzm', 'geometry', NULL, 'blades_xyz', NULL, 'out_polygs_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 46;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter(NULL, 'polygs_xyzm', 'geometry', NULL, 'blades_xym', NULL, 'out_polygs_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 47;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('MAIN', 'polygs_xyzm', 'geometry', 'main', 'blades_xy', 'geometry', 'out_polygs_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 48;
	  return 0;
      }

    return 1;
}

static int
check_cutter_attach (sqlite3 * handle, int *retcode)
{
/* testing ST_Cutter - ATTACHed DB */
    const char *sql;
    int ret;

/* cutting Points XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'points_xy', NULL, 'blade', 'blades_xyzm', NULL, 'out_points_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 1;
	  return 0;
      }

/* cutting Points XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'points_xy', NULL, 'blade', 'blades_xyz', NULL, 'out_points_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 2;
	  return 0;
      }

/* cutting Points XY - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'points_xy', NULL, 'blade', 'blades_xym', NULL, 'out_points_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 3;
	  return 0;
      }

/* cutting Points XY - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'points_xy', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_points_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 4;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xy', NULL, 'blade', 'blades_xyzm', NULL, 'out_lines_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 5;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'lines_xy', NULL, 'blade', 'blades_xyz', NULL, 'out_lines_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 6;
	  return 0;
      }

/* cutting Linestrings XY - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xy', NULL, 'blade', 'blades_xym', NULL, 'out_lines_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 7;
	  return 0;
      }

/* cutting Linestrings XY - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'lines_xy', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_lines_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 8;
	  return 0;
      }

/* cutting Polygons XY - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xy', NULL, 'blade', 'blades_xyzm', NULL, 'out_polygs_xy_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 9;
	  return 0;
      }

/* cutting Polygons XY - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xy', NULL, 'blade', 'blades_xyz', NULL, 'out_polygs_xy_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 10;
	  return 0;
      }

/* cutting Polygons XY - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xy', NULL, 'blade', 'blades_xym', NULL, 'out_polygs_xy_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 11;
	  return 0;
      }

/* cutting Polygons XY - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xy', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_polygs_xy_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 12;
	  return 0;
      }

/* cutting Points XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'points_xyz', NULL, 'blade', 'blades_xyzm', NULL, 'out_points_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 13;
	  return 0;
      }

/* cutting Points XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'points_xyz', NULL, 'blade', 'blades_xyz', NULL, 'out_points_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 14;
	  return 0;
      }

/* cutting Points XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'points_xyz', NULL, 'blade', 'blades_xym', NULL, 'out_points_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 15;
	  return 0;
      }

/* cutting Points XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'points_xyz', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_points_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 16;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyz', NULL, 'blade', 'blades_xyzm', NULL, 'out_lines_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 17;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyz', NULL, 'blade', 'blades_xyz', NULL, 'out_lines_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 18;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyz', NULL, 'blade', 'blades_xym', NULL, 'out_lines_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 19;
	  return 0;
      }

/* cutting Linestrings XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyz', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_lines_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 20;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyz', NULL, 'blade', 'blades_xyzm', NULL, 'out_polygs_xyz_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 21;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyz', NULL, 'blade', 'blades_xyz', NULL, 'out_polygs_xyz_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 22;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyz', NULL, 'blade', 'blades_xym', NULL, 'out_polygs_xyz_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 23;
	  return 0;
      }

/* cutting Polygons XYZ - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyz', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_polygs_xyz_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 24;
	  return 0;
      }

/* cutting Points XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'points_xym', NULL, 'blade', 'blades_xyzm', NULL, 'out_points_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 25;
	  return 0;
      }

/* cutting Points XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'points_xym', NULL, 'blade', 'blades_xyz', NULL, 'out_points_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 26;
	  return 0;
      }

/* cutting Points XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'points_xym', NULL, 'blade', 'blades_xym', NULL, 'out_points_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 27;
	  return 0;
      }

/* cutting Points XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'points_xym', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_points_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 28;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xym', NULL, 'blade', 'blades_xyzm', NULL, 'out_lines_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 29;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'lines_xym', NULL, 'blade', 'blades_xyz', NULL, 'out_lines_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 30;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xym', NULL, 'blade', 'blades_xym', NULL, 'out_lines_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 31;
	  return 0;
      }

/* cutting Linestrings XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'lines_xym', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_lines_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 32;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xym', NULL, 'blade', 'blades_xyzm', NULL, 'out_polygs_xym_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 33;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xym', NULL, 'blade', 'blades_xyz', NULL, 'out_polygs_xym_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 34;
	  return 0;
      }

/* cutting Polygons XYM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xym', NULL, 'blade', 'blades_xym', NULL, 'out_polygs_xym_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 35;
	  return 0;
      }

/* cutting Polygons XYM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xym', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_polygs_xym_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 36;
	  return 0;
      }

/* cutting Points XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'points_xyzm', NULL, 'blade', 'blades_xyzm', NULL, 'out_points_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 37;
	  return 0;
      }

/* cutting Points XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'points_xyzm', NULL, 'blade', 'blades_xyz', NULL, 'out_points_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 38;
	  return 0;
      }

/* cutting Points XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'points_xyzm', NULL, 'blade', 'blades_xym', NULL, 'out_points_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 39;
	  return 0;
      }

/* cutting Points XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'points_xyzm', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_points_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 40;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyzm', NULL, 'blade', 'blades_xyzm', NULL, 'out_lines_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 41;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyzm', NULL, 'blade', 'blades_xyz', NULL, 'out_lines_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 42;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyzm', NULL, 'blade', 'blades_xym', NULL, 'out_lines_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 43;
	  return 0;
      }

/* cutting Linestrings XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'lines_xyzm', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_lines_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 44;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYZM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyzm', 'geometry', 'blade', 'blades_xyzm', NULL, 'out_polygs_xyzm_xyzm', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 45;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYZ */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyzm', 'geometry', 'blade', 'blades_xyz', NULL, 'out_polygs_xyzm_xyz', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 46;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XYM */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyzm', 'geometry', 'blade', 'blades_xym', NULL, 'out_polygs_xyzm_xym', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 47;
	  return 0;
      }

/* cutting Polygons XYZM - Blade XY */
    sql =
	"SELECT ST_Cutter('input', 'polygs_xyzm', 'geometry', 'blade', 'blades_xy', 'geometry', 'out_polygs_xyzm_xy', 1, 1)";
    ret = test_query (handle, sql);
    if (!ret)
      {
	  *retcode -= 48;
	  return 0;
      }

    return 1;
}

static int
create_points_xyzm (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POINT XYZM */
    int ret;
    char *err_msg = NULL;

    ret = sqlite3_exec (handle, "CREATE TABLE points_xyzm ("
			"pk_2 DOUBLE NOT NULL, pk_1 TEXT NOT NULL, pk_3 TEXT NOT NULL,"
			"CONSTRAINT points_xyzm_pk PRIMARY KEY (pk_1, pk_2, pk_3))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xyzm error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'points_xyzm', 'geometry', 4326, 'POINT', 'XYZM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xyzm Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'a', "
		      "GeomFromText('POINT ZM(-10 10 1 2)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'b', "
		      "GeomFromText('POINT ZM(16 3 2 22)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'c', "
		      "GeomFromText('POINT ZM(34 9 3 12)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 5;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'd', "
		      "GeomFromText('POINT ZM(14 15 4 15)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 6;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'e', "
		      "GeomFromText('POINT ZM(10 10 4.5 11)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 7;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'a', 'f', "
		      "GeomFromText('POINT ZM(7 10 14.5 17)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 8;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'a', "
		      "GeomFromText('POINT ZM(10 8 12.5 7)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 9;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'b', "
		      "GeomFromText('POINT ZM(10 8 2.5 7.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 10;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'c', "
		      "GeomFromText('POINT ZM(20 2 23.5 72.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 11;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'd', "
		      "GeomFromText('POINT ZM(24 4.25 23 74)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 12;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'e', "
		      "GeomFromText('POINT ZM(23 0 25 41)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 13;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'b', 'f', "
		      "GeomFromText('POINT ZM(5.1 5.1 22 51)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 14;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'a', "
		      "GeomFromText('POINT ZM(25.1 5.1 22 51)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 15;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'b', "
		      "GeomFromText('POINT ZM(8 2 32 71)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 16;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'c', "
		      "GeomFromText('POINT ZM(28 2 36 75)', 4326))", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 17;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'd', "
		      "GeomFromText('POINT ZM(11 111 3.6 7.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 18;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'e', "
		      "GeomFromText('POINT ZM(12 112 4.6 76.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 19;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (1.0, 'c', 'f', "
		      "GeomFromText('POINT ZM(17 117 46 75)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 20;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'a', "
		      "GeomFromText('POINT ZM(25 85 47 76)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 21;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'b', "
		      "GeomFromText('POINT ZM(15 95 74 67)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 22;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'c', "
		      "GeomFromText('POINT ZM(25 95 78 68)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 23;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'd', "
		      "GeomFromText('POINT ZM(55 85 79 58)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 24;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'e', "
		      "GeomFromText('POINT ZM(40 120 79.5 58.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 25;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (2.0, 'a', 'f', "
		      "GeomFromText('POINT ZM(42 120 79.5 58.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 26;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyzm VALUES (6.0, 'e', 'd', "
		      "GeomFromText('POINT ZM(45 120 9.5 8.5)', 4326))", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 27;
	  return 0;
      }
    return 1;
}

static int
create_points_xyz (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POINT XYZ */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE points_xyz (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xyz error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'points_xyz', 'geometry', 4326, 'POINT', 'XYZ')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xyz Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xyz SELECT NULL, CastToXYZ(geometry) FROM points_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xyz Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_points_xym (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POINT XYM */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE points_xym (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xym error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'points_xym', 'geometry', 4326, 'POINT', 'XYM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xym Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xym SELECT NULL, CastToXYM(geometry) FROM points_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xym Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_points_xy (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POINT XY */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE points_xy (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xy error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'points_xy', 'geometry', 4326, 'POINT', 'XY')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE points_xy Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO points_xy SELECT NULL, CastToXY(geometry) FROM points_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO points_xy Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_lines_xyzm (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - LINESTRING XYZM */
    int ret;
    char *err_msg = NULL;

    ret = sqlite3_exec (handle, "CREATE TABLE lines_xyzm ("
			"pk_1 DOUBLE NOT NULL, pk_2 TEXT NOT NULL, pk_3 TEXT NOT NULL,"
			"CONSTRAINT lines_xyzm_pk PRIMARY KEY (pk_1, pk_2, pk_3))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xyzm error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'lines_xyzm', 'geometry', 4326, 'LINESTRING', 'XYZM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xyzm Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'a', "
			"GeomFromText('LINESTRING ZM(-10 5.5 1 2, 35 5.5 3 4)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'b', "
			"GeomFromText('LINESTRING ZM(2 9 4 4, 7 9 5 5)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'c', "
			"GeomFromText('LINESTRING ZM(6 4 3 2, 10 4 2 3)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 5;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'd', "
			"GeomFromText('LINESTRING ZM(3 4.5 1 2, 7 4.5 4 3)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 6;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'e', "
			"GeomFromText('LINESTRING ZM(2 2 5 5, 8 8 5 5)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 7;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (1.0, 'a', 'f', "
			"GeomFromText('LINESTRING ZM(-1 2 2 3, 0 2 3 2, 0 0 2 3, 5 0 3 2, 5 -1 2 3)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 8;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'a', 'a', "
			"GeomFromText('LINESTRING ZM(30 11 1 2, 30 6 2 1, 31 5 1 2, 30 4 1 2, 30 0 2 1, "
			"24 0 2 1, 24 4 2 3, 26 4 3 2, 27 5 3 2, 26 6 2 3, 24 6 1 3, 24 10 3 1, 20 10 2 3, 20 3 2 1)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 9;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'b', 'a', "
			"GeomFromText('LINESTRING ZM(-2 52 3 4, 20 52 4 3, 20 45 3 2)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 10;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'c', 'a', "
			"GeomFromText('LINESTRING ZM(0 100 4 5, 100 100 5 4)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 11;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'd', 'a', "
			"GeomFromText('LINESTRING ZM(20 80 2 4, 70 80 4 2)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 12;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'e', 'a', "
			"GeomFromText('LINESTRING ZM(20 90 1 4, 70 90 2 3)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 13;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'f', 'a', "
			"GeomFromText('LINESTRING ZM(20 70 6 2, 70 70 6 4)', 4326))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 14;
	  return 0;
      }
    ret = sqlite3_exec (handle, "INSERT INTO lines_xyzm VALUES (2.0, 'f', 'f', "
			"GeomFromText('LINESTRING ZM(40 140 2 3, 60 140 3 4, 100 100 5 6, "
			"60 60 7 1, 50 60 4 2)', 4326))", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 15;
	  return 0;
      }
    return 1;
}

static int
create_lines_xyz (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - LINESTRING XYZ */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE lines_xyz (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xyz error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'lines_xyz', 'geometry', 4326, 'LINESTRING', 'XYZ')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xyz Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO lines_xyz SELECT NULL, CastToXYZ(geometry) FROM lines_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xyz Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_lines_xym (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - LINESTRING XYM */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE lines_xym (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xym error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'lines_xym', 'geometry', 4326, 'LINESTRING', 'XYM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xym Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO lines_xym SELECT NULL, CastToXYM(geometry) FROM lines_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xym Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_lines_xy (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - LINESTRING XY */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE lines_xy (pk_id INTEGER PRIMARY KEY)", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xy error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'lines_xy', 'geometry', 4326, 'LINESTRING', 'XY')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE lines_xy Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO lines_xy SELECT NULL, CastToXY(geometry) FROM lines_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO lines_xy Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_polygs_xyzm (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POLYGON XYZM */
    int ret;
    char *err_msg = NULL;

    ret = sqlite3_exec (handle, "CREATE TABLE polygs_xyzm ("
			"pk_3 DOUBLE NOT NULL, pk_1 TEXT NOT NULL, pk_2 TEXT NOT NULL,"
			"CONSTRAINT polygs_xyzm_pk PRIMARY KEY (pk_1, pk_2, pk_3))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polygs_xyzm error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'polygs_xyzm', 'geometry', 4326, 'POLYGON', 'XYZM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polygs_xyzm Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (1.5, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((-2 15 1 2, 35 15 2 3, 35 5 3 2, -2 5 2 2, -2 15 1 2))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (1.0, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((-2 5 1 1, 10 5 2 2, 10 -5 1 2, -2 -5 2 1, -2 5 1 1))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (1.1, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((10 5 3 4, 20 5 3 4, 20 -5 4 3, 10 -5 4 3, 10 5 3 4))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 5;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (2.0, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((20 5 2 5, 35 5 3 5, 35 -5 2 3, 20 -5 3 3, 20 5 2 5))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 6;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (3.5, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((0 100 1 1, 100 100 2 2, 100 150 2 1, 0 150 2 3, 0 100 1 1))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 7;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (0.5, 'a', 'a', "
		      "GeomFromText('POLYGON ZM((0 100 2 3, 100 100 3 1, 100 50 1 3, 0 50 2 1, 0 100 2 3))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 8;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES (2.4, 'b', 'a', "
		      "GeomFromText('POLYGON ZM((70 10 1 2, 100 10 2 1, 100 40 2 3, 70 40 3 2, 70 10 1 2), "
		      "(80 20 2 2, 90 20 3 3, 90 30 2 3, 80 30 3 2, 80 20 2 2))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 9;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyzm VALUES ('a', 'b', 'c', "
		      "GeomFromText('POLYGON ZM((80 20 1 2, 90 20 2 3, 90 30 3 4, 80 30 3 2, 80 20 1 2))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 10;
	  return 0;
      }

    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'polygs_xyzm', 'centroid', 4326, 'POINT', 'XY')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polygs_xyzm Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 11;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "UPDATE polygs_xyzm SET centroid = CastToXY(ST_Centroid(geometry))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "UPDATE polygs_xyzm SET Centroid error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 12;
	  return 0;
      }
    return 1;
}

static int
create_polygs_xyz (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POLYGON XYZ */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE polygs_xyz (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xyz error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'polygs_xyz', 'geometry', 4326, 'POLYGON', 'XYZ')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xyz Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xyz SELECT NULL, CastToXYZ(geometry) FROM polygs_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xyz Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_polygs_xym (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POLYGON XYM */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE polygs_xym (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xym error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'polygs_xym', 'geometry', 4326, 'POLYGON', 'XYM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xym Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xym SELECT NULL, CastToXYM(geometry) FROM polygs_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xym Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_polygs_xy (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - POLYGON XY */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE polygs_xy (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xy error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'polygs_xy', 'geometry', 4326, 'POLYGON', 'XY')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE polys_xy Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO polygs_xy SELECT NULL, CastToXY(geometry) FROM polygs_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO polygs_xy Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_blades_xyzm (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - BLADES XYZM */
    int ret;
    char *err_msg = NULL;

    ret = sqlite3_exec (handle, "CREATE TABLE blades_xyzm ("
			"pk_1 TEXT NOT NULL, pk_2 TEXT NOT NULL, pk_3 TEXT NOT NULL,"
			"CONSTRAINT blades_xyzm_pk PRIMARY KEY (pk_1, pk_2, pk_3))",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xyzm error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'blades_xyzm', 'geometry', 4326, 'POLYGON', 'XYZM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xyzm Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xyzm VALUES ('a', 'a', 'a', "
		      "GeomFromText('POLYGON ZM((0 0 0 0, 10 0 0 1, 10 10 1 1, 0 10 1 2, 0 0 0 0), "
		      "(4 4 1 2, 6 4 2 1, 6 6 3 2, 4 6 2 1, 4 4 1 2))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xyzm VALUES ('b', 'c', 'd', "
		      "GeomFromText('POLYGON ZM((20 0 1 2, 30 0 2 3, 30 4 3 2, 31 5 2 3, 30 6 2 1, 30 10 2 0, 20 10 0 2, 20 0 1 2), "
		      "(24 4 1 1, 26 4 2 2, 27 5 2 1, 26 6 1 2, 24 6 2 1, 24 4 1 1))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xyzm VALUES ('d', 'c', 'e', "
		      "GeomFromText('POLYGON ZM((50 50 1 2, 100 100 2 3, 50 150 3 2, 0 100 2 1, 0 50 1 0, 50 50 1 2), "
		      "(40 80 1 2, 60 80 2 3, 60 100 3 4, 40 80 1 2), "
		      "(10 100 1 3, 30 80 3 2, 30 100 2 3, 10 100 1 3))', 4326))",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xyzm Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 5;
	  return 0;
      }
    return 1;
}

static int
create_blades_xyz (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - BLADES XYZ */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE blades_xyz (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xyz error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'blades_xyz', 'geometry', 4326, 'POLYGON', 'XYZ')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xyz Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xyz SELECT NULL, CastToXYZ(geometry) FROM blades_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xyz Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
create_blades_xym (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - BLADES XYM */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE blades_xym (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xym error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'blades_xym', 'geometry', 4326, 'POLYGON', 'XYM')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xym Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "SELECT CreateSpatialIndex('blades_xym', 'geometry')",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xym Spatial Index error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xym SELECT NULL, CastToXYM(geometry) FROM blades_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xym Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }
    return 1;
}

static int
create_blades_xy (sqlite3 * handle, int *retcode)
{
/* creating and populating a test table - BLADES XY */
    int ret;
    char *err_msg = NULL;

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE blades_xy (pk_id INTEGER PRIMARY KEY)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xy error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }
    ret = sqlite3_exec (handle, "SELECT AddGeometryColumn("
			"'blades_xy', 'geometry', 4326, 'POLYGON', 'XY')",
			NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CREATE blades_xy Geometry error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }
    ret =
	sqlite3_exec (handle,
		      "INSERT INTO blades_xy SELECT NULL, CastToXY(geometry) FROM blades_xyzm",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "INSERT INTO blades_xy Geometry error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }
    return 1;
}

static int
check_cutter_attached (int *retcode)
{
/* testing ST_Cutter on ATTACHed DBs */
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \":memory:\" databse: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  *retcode -= 1;
	  return 0;
      }

    spatialite_init_ex (handle, cache, 0);

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 2;
	  return 0;
      }

    ret =
	sqlite3_exec (handle,
		      "ATTACH DATABASE \"./test_cutter.sqlite\" AS input", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE Input: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 3;
	  return 0;
      }

    ret =
	sqlite3_exec (handle,
		      "ATTACH DATABASE \"./test_cutter.sqlite\" AS blade", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "ATTACH DATABASE Blade: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 4;
	  return 0;
      }

    if (!check_cutter_attach (handle, retcode))
      {
	  *retcode -= 5;
	  sqlite3_close (handle);
	  return 0;
      }

    ret = sqlite3_exec (handle, "DETACH DATABASE input", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DETACH DATABASE Input: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 6;
	  return 0;
      }

    ret = sqlite3_exec (handle, "DETACH DATABASE blade", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "DETACH DATABASE Blade: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  *retcode -= 7;
	  return 0;
      }

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  *retcode -= 8;
	  return 0;
      }

    return 1;
}

#endif

int
main (int argc, char *argv[])
{
#ifndef OMIT_GEOS		/* only if GEOS is enabled */
    int ret;
    int retcode;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    unlink ("./test_cutter.sqlite");

    ret =
	sqlite3_open_v2 ("./test_cutter.sqlite", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \"test_cutter.sqlite\" database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -2;
      }

    ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "BEGIN TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }

/* creating and populating a test table - POINT XYZM */
    retcode = -10;
    if (!create_points_xyzm (handle, &retcode))
	return retcode;

/* creating and populating a test table - POINT XYZ */
    retcode = -10;
    if (!create_points_xyz (handle, &retcode))
	return retcode;

/* creating and populating a test table - POINT XYM */
    retcode = -50;
    if (!create_points_xym (handle, &retcode))
	return retcode;

/* creating and populating a test table - POINT XY */
    retcode = -100;
    if (!create_points_xy (handle, &retcode))
	return retcode;

/* creating and populating a test table - LINESTRING XYZM */
    retcode = -150;
    if (!create_lines_xyzm (handle, &retcode))
	return retcode;

/* creating and populating a test table - LINESTRING XYZ */
    retcode = -200;
    if (!create_lines_xyz (handle, &retcode))
	return retcode;

/* creating and populating a test table - LINESTRING XYM */
    retcode = -250;
    if (!create_lines_xym (handle, &retcode))
	return retcode;

/* creating and populating a test table - LINESTRING XY */
    retcode = -300;
    if (!create_lines_xy (handle, &retcode))
	return retcode;

/* creating and populating a test table - POLYGON XYZM */
    retcode = -350;
    if (!create_polygs_xyzm (handle, &retcode))
	return retcode;

/* creating and populating a test table - POLYGON XYZ */
    retcode = -400;
    if (!create_polygs_xyz (handle, &retcode))
	return retcode;

/* creating and populating a test table - POLYGON XYM */
    retcode = -450;
    if (!create_polygs_xym (handle, &retcode))
	return retcode;

/* creating and populating a test table - POLYGON XY */
    retcode = -500;
    if (!create_polygs_xy (handle, &retcode))
	return retcode;

/* creating and populating a test table - BLADES XYZM */
    retcode = -550;
    if (!create_blades_xyzm (handle, &retcode))
	return retcode;

/* creating and populating a test table - BLADES XYZ */
    retcode = -600;
    if (!create_blades_xyz (handle, &retcode))
	return retcode;

/* creating and populating a test table - BLADES XYM */
    retcode = -650;
    if (!create_blades_xym (handle, &retcode))
	return retcode;

/* creating and populating a test table - BLADES XY */
    retcode = -700;
    if (!create_blades_xy (handle, &retcode))
	return retcode;

    ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "COMMIT TRANSACTION error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -4;
      }

/* testing ST_Cutter - MAIN DB */
    retcode = -750;
    if (!check_cutter_main (handle, &retcode))
	return retcode;

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  return -5;
      }

    spatialite_cleanup_ex (cache);

/* testing ST_Cutter - ATTACHED DB */
    retcode = -800;
    if (!check_cutter_attached (&retcode))
	return retcode;

#endif /* end GEOS conditional */
    unlink ("./test_cutter.sqlite");

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    spatialite_shutdown ();
    return 0;
}
