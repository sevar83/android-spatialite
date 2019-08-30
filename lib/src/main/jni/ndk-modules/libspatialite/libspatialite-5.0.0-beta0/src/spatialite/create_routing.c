/*

 create_routing.c -- Creating a VirtualRouting from an input table

 version 4.5, 2017 December 15

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
 
Portions created by the Initial Developer are Copyright (C) 2017
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

/* 64 bit integer: portable format for printf() */
#if defined(_WIN32) && !defined(__MINGW32__)
#define FRMT64 "%I64d"
#else
#define FRMT64 "%lld"
#endif

#define MAX_BLOCK	1048576

static void
gaia_create_routing_set_error (const void *ctx, const char *errmsg)
{
/* setting the CreateRouting Last Error Message */
    struct splite_internal_cache *cache = (struct splite_internal_cache *) ctx;
    if (cache != NULL)
      {
	  int len;
	  if (cache->createRoutingError != NULL)
	    {
		free (cache->createRoutingError);
		cache->createRoutingError = NULL;
	    }
	  if (errmsg == NULL)
	      return;

	  len = strlen (errmsg);
	  cache->createRoutingError = malloc (len + 1);
	  strcpy (cache->createRoutingError, errmsg);
      }
}

SPATIALITE_DECLARE const char *
gaia_create_routing_get_last_error (const void *p_cache)
{
/* return the last CreateRouting Error Message (if any) */
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;

    if (cache == NULL)
	return NULL;
    return cache->createRoutingError;
}

static void
do_drop_temp_tables (sqlite3 * db_handle)
{
/* attempting to drop existing temp-tables (just in case) */
    const char *sql;

    /* attempting to drop the Nodes Temp-Table */
    sql = "DROP TABLE IF EXISTS create_routing_nodes";
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);

/* attempting to drop the Routing Data Table */
    sql = "DROP TABLE IF EXISTS create_routing_links";
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
}

static void
do_drop_tables (sqlite3 * db_handle, const char *routing_data_table,
		const char *virtual_routing_table)
{
/* attempting to drop existing tables (just in case) */
    char *xtable;
    char *sql;

/* attempting to drop the VirtualRouting Table */
    xtable = gaiaDoubleQuotedSql (virtual_routing_table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);

/* attempting to drop the Routing Data Table */
    xtable = gaiaDoubleQuotedSql (routing_data_table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
}

static int
do_check_data_table (sqlite3 * db_handle, const char *routing_data_table)
{
/* testing if the Routing Data Table is already defined */
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int nr = 0;

    xtable = gaiaDoubleQuotedSql (routing_data_table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	nr++;
    sqlite3_free_table (results);
    return nr;
}

static int
do_check_virtual_table (sqlite3 * db_handle, const char *virtual_routing_table)
{
/* testing if the VirtualRouting Table is already defined */
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int nr = 0;

    xtable = gaiaDoubleQuotedSql (virtual_routing_table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	nr++;
    sqlite3_free_table (results);
    return nr;
}

static int
check_geom (gaiaGeomCollPtr geom, int *is3d, double *x_from, double *y_from,
	    double *z_from, double *x_to, double *y_to, double *z_to)
{
/* checking an Arc/Link Geometry for validity */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int points = 0;
    int lines = 0;
    int polygons = 0;
    int end;

    if (geom == NULL)
	return 0;

    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  points++;
	  pt = pt->Next;
      }
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  lines++;
	  ln = ln->Next;
      }
    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  polygons++;
	  pg = pg->Next;
      }
    if (points == 0 && lines == 1 && polygons == 0)
	;
    else
	return 0;

    ln = geom->FirstLinestring;
    end = ln->Points - 1;
    if (ln->DimensionModel == GAIA_XY_Z)
      {
	  *is3d = 1;
	  gaiaGetPointXYZ (ln->Coords, 0, x_from, y_from, z_from);
	  gaiaGetPointXYZ (ln->Coords, end, x_to, y_to, z_to);
      }
    else if (ln->DimensionModel == GAIA_XY_Z_M)
      {
	  double m;
	  *is3d = 1;
	  gaiaGetPointXYZM (ln->Coords, 0, x_from, y_from, z_from, &m);
	  gaiaGetPointXYZM (ln->Coords, end, x_to, y_to, z_to, &m);
      }
    else if (ln->DimensionModel == GAIA_XY_M)
      {
	  double m;
	  *is3d = 0;
	  gaiaGetPointXYM (ln->Coords, 0, x_from, y_from, &m);
	  *z_from = 0.0;
	  gaiaGetPointXYM (ln->Coords, end, x_to, y_to, &m);
	  *z_to = 0.0;
      }
    else
      {
	  *is3d = 0;
	  gaiaGetPoint (ln->Coords, 0, x_from, y_from);
	  *z_from = 0.0;
	  gaiaGetPoint (ln->Coords, end, x_to, y_to);
	  *z_to = 0.0;
      }

    return 1;
}

static int
do_update_internal_index (sqlite3 * db_handle, const void *cache,
			  sqlite3_stmt * stmt, sqlite3_int64 rowid, int index)
{
/* assigning the Internal Index to some Node */
    int ret;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, index);
    sqlite3_bind_int64 (stmt, 2, rowid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

static int
do_search_view (sqlite3 * handle, const char *view, const char *geom,
		int *srid, int *dims, int *is_geographic)
{
/* seraching the SRID and DIMS for Geometry - Spatial View */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int xsrid;
    int type;
    int count = 0;

    sql =
	sqlite3_mprintf
	("SELECT g.srid, g.geometry_type FROM views_geometry_columns AS v "
	 "JOIN geometry_columns AS g ON (g.f_table_name = v.f_table_name "
	 "AND g.f_geometry_column = v.f_geometry_column) WHERE "
	 "Lower(v.view_name) = Lower(%Q) AND Lower(v.view_geometry) = Lower(%Q)",
	 view, geom);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		xsrid = sqlite3_column_int (stmt, 0);
		type = sqlite3_column_int (stmt, 1);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  int geographic;
	  if (!srid_is_geographic (handle, xsrid, &geographic))
	      return 0;
	  switch (type)
	    {
	    case GAIA_LINESTRING:
		*dims = GAIA_XY;
		break;
	    case GAIA_LINESTRINGZ:
		*dims = GAIA_XY_Z;
		break;
	    case GAIA_LINESTRINGM:
		*dims = GAIA_XY_M;
		break;
	    case GAIA_LINESTRINGZM:
		*dims = GAIA_XY_Z_M;
		break;
	    default:
		return 0;
	    };
	  *srid = xsrid;
	  *is_geographic = geographic;
	  return 1;
      }
    return 0;
}

static int
do_search_srid (sqlite3 * handle, const char *table, const char *geom,
		int *srid, int *dims, int *is_geographic)
{
/* seraching the SRID and DIMS for Geometry */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int xsrid;
    int type;
    int count = 0;

    sql =
	sqlite3_mprintf
	("SELECT srid, geometry_type FROM geometry_columns WHERE "
	 "Lower(f_table_name) = Lower(%Q) AND Lower(f_geometry_column) = Lower(%Q)",
	 table, geom);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		xsrid = sqlite3_column_int (stmt, 0);
		type = sqlite3_column_int (stmt, 1);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  int geographic;
	  if (!srid_is_geographic (handle, xsrid, &geographic))
	      return 0;
	  switch (type)
	    {
	    case GAIA_LINESTRING:
		*dims = GAIA_XY;
		break;
	    case GAIA_LINESTRINGZ:
		*dims = GAIA_XY_Z;
		break;
	    case GAIA_LINESTRINGM:
		*dims = GAIA_XY_M;
		break;
	    case GAIA_LINESTRINGZM:
		*dims = GAIA_XY_Z_M;
		break;
	    default:
		return 0;
	    };
	  *srid = xsrid;
	  *is_geographic = geographic;
	  return 1;
      }
      
      if (count == 0)
      {
		  /* testing for a possible Spatial View */
		  if (do_search_view (handle, table, geom, srid, dims, is_geographic))
		  return 1;
	  }
    return 0;
}

static int
do_check_input_table (sqlite3 * db_handle, const void *cache,
		      const char *input_table, const char *from_column,
		      const char *to_column, const char *geom_column,
		      const char *cost_column, const char *name_column,
		      const char *oneway_from, const char *oneway_to,
		      int a_star_enabled, int bidirectional, int *has_ids,
		      int *n_nodes, int *max_code_length, double *a_star_coeff)
{
/* testing if the Input Table exists and it's correctly defined */
    char *xtable;
    char *sql;
    char *prev;
    char *xname;
    sqlite3_stmt *stmt;
    sqlite3_stmt *stmt_ins_nodes = NULL;
    sqlite3_stmt *stmt_ins_links = NULL;
    sqlite3_stmt *stmt_check_nodes = NULL;
    sqlite3_stmt *stmt_update_nodes = NULL;
    sqlite3_stmt *stmt_sel_nodes = NULL;
    sqlite3_stmt *stmt_update_links_from = NULL;
    sqlite3_stmt *stmt_update_links_to = NULL;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int ok_from = 0;
    int ok_to = 0;
    int ok_geom = 0;
    int ok_cost = 0;
    int ok_name = 0;
    int ok_oneway_from = 0;
    int ok_oneway_to = 0;
    int error = 0;
    int first = 1;
    int glob_srid;
    int glob_dims;
    sqlite3_int64 last_id;
    char *last_code = NULL;
    double last_x;
    double last_y;
    double last_z;
    int is_geographic = 0;

/* setting a Savepoint */
    sql = "SAVEPOINT create_routing_one";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    xtable = gaiaDoubleQuotedSql (input_table);
    sql = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
      {
	  const char *col = results[(i * columns) + 1];
	  if (strcasecmp (col, from_column) == 0)
	      ok_from = 1;
	  if (strcasecmp (col, to_column) == 0)
	      ok_to = 1;
	  if (geom_column != NULL)
	    {
		if (strcasecmp (col, geom_column) == 0)
		    ok_geom = 1;
	    }
	  if (cost_column != NULL)
	    {
		if (strcasecmp (col, cost_column) == 0)
		    ok_cost = 1;
	    }
	  if (name_column != NULL)
	    {
		if (strcasecmp (col, name_column) == 0)
		    ok_name = 1;
	    }
	  if (oneway_from != NULL)
	    {
		if (strcasecmp (col, oneway_from) == 0)
		    ok_oneway_from = 1;
	    }
	  if (oneway_to != NULL)
	    {
		if (strcasecmp (col, oneway_to) == 0)
		    ok_oneway_to = 1;
	    }
      }
    sqlite3_free_table (results);

/* final validity check */
    if (!ok_from)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("FromNode Column \"%s\" is not defined in the Input Table",
	       from_column);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    if (!ok_to)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ToNode Column \"%s\" is not defined in the Input Table",
	       to_column);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    if (geom_column != NULL)
      {
	  if (!ok_geom)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Geometry Column \"%s\" is not defined in the Input Table",
		     geom_column);
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    if (cost_column != NULL)
      {
	  if (!ok_cost)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Cost Column \"%s\" is not defined in the Input Table",
		     cost_column);
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    if (name_column != NULL)
      {
	  if (!ok_name)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Name Column \"%s\" is not defined in the Input Table",
		     name_column);
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    if (oneway_from != NULL)
      {
	  if (!ok_oneway_from)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("OnewayFromTo Column \"%s\" is not defined in the Input Table",
		     oneway_from);
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    if (!ok_oneway_to)
      {
	  if (oneway_to != NULL)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("OnewayToFrom Column \"%s\" is not defined in the Input Table",
		     oneway_to);
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (geom_column != NULL)
      {
	  /* searching the SRID and DIMS of Geometry */
	  if (!do_search_srid
	      (db_handle, input_table, geom_column, &glob_srid, &glob_dims,
	       &is_geographic))
	    {
		char *msg = sqlite3_mprintf ("Unable to find geometry %Q.%Q",
					     input_table, geom_column);
		gaia_create_routing_set_error (cache, msg);
		return 0;
	    }
      }

/* testing actual values for consistency */
    sql = sqlite3_mprintf ("SELECT ROWID");
    prev = sql;
    xname = gaiaDoubleQuotedSql (from_column);
    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
    free (xname);
    sqlite3_free (prev);
    prev = sql;
    xname = gaiaDoubleQuotedSql (to_column);
    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
    free (xname);
    sqlite3_free (prev);
    prev = sql;
    if (geom_column == NULL)
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  xname = gaiaDoubleQuotedSql (geom_column);
	  sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    prev = sql;
    if (a_star_enabled || cost_column == NULL)
      {
	  xname = gaiaDoubleQuotedSql (geom_column);
	  if (is_geographic)
	      sql = sqlite3_mprintf ("%s, ST_Length(\"%s\", 1)", prev, xname);
	  else
	      sql = sqlite3_mprintf ("%s, ST_Length(\"%s\")", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    else
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    prev = sql;
    if (cost_column == NULL)
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  xname = gaiaDoubleQuotedSql (cost_column);
	  sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    prev = sql;
    if (name_column == NULL)
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  xname = gaiaDoubleQuotedSql (name_column);
	  sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    prev = sql;
    if (oneway_from == NULL)
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  xname = gaiaDoubleQuotedSql (oneway_from);
	  sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    prev = sql;
    if (oneway_to == NULL)
      {
	  sql = sqlite3_mprintf ("%s, NULL", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  xname = gaiaDoubleQuotedSql (oneway_to);
	  sql = sqlite3_mprintf ("%s, \"%s\"", prev, xname);
	  free (xname);
	  sqlite3_free (prev);
      }
    prev = sql;
    xtable = gaiaDoubleQuotedSql (input_table);
    sql = sqlite3_mprintf ("%s FROM \"%s\"", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* preparing the temp-nodes table */
    sql = "CREATE TEMPORARY TABLE create_routing_nodes ("
	"internal_index INTEGER,\n"
	"node_id INTEGER,\n"
	"node_code TEXT,\n"
	"node_x DOUBLE,\n" "node_y DOUBLE,\n" "node_z DOUBLE)";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto stop;
      }

    sql = "INSERT INTO create_routing_nodes "
	"(node_id, node_code, node_x, node_y, node_z) "
	"VALUES (?, ?, ?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_ins_nodes,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto stop;
      }

/* preparing the temp-links table */
    sql = "CREATE TEMPORARY TABLE create_routing_links ("
	"rowid INTEGER,\n"
	"id_node_from INTEGER,\n"
	"cod_node_from TEXT,\n"
	"id_node_to INTEGER,\n"
	"cod_node_to INTEGER,\n"
	"cost DOUBLE,\n" "index_from INTEGER,\n" "index_to INTEGER)";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto stop;
      }

    sql = "INSERT INTO create_routing_links "
	"(rowid, id_node_from, cod_node_from, id_node_to, cod_node_to, cost) "
	"VALUES (?, ?, ?, ?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_ins_links,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto stop;
      }

/* exploring all arcs/links */
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid;
		sqlite3_int64 id_from;
		sqlite3_int64 id_to;
		const char *from;
		const char *to;
		int int_from;
		int int_to;
		int is3d;
		double x_from;
		double y_from;
		double z_from;
		double x_to;
		double y_to;
		double z_to;
		double length = -1.0;
		double cost = -1.0;
		int from_to = -1;
		int to_from = -1;
		int len;
		rowid = sqlite3_column_int64 (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		  {
		      id_from = sqlite3_column_int64 (stmt, 1);
		      int_from = 1;
		  }
		else if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		  {
		      from = (const char *) sqlite3_column_text (stmt, 1);
		      len = strlen (from);
		      if (len > *max_code_length)
			  *max_code_length = len;
		      int_from = 0;
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("NodeFrom column \"%s\": found a value that's neither TEXT nor INTEGER",
			   from_column);
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      error = 1;
		      goto stop;
		  }
		if (first)
		    *has_ids = int_from;
		if (int_from != *has_ids)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("NodeFrom column \"%s\": found a mismatching value",
			   from_column);
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      error = 1;
		      goto stop;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      id_to = sqlite3_column_int64 (stmt, 2);
		      int_to = 1;
		  }
		else if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
		  {
		      to = (const char *) sqlite3_column_text (stmt, 2);
		      len = strlen (to);
		      if (len > *max_code_length)
			  *max_code_length = len;
		      int_to = 0;
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("NodeTo column \"%s\": found a value that's neither TEXT nor INTEGER",
			   to_column);
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      error = 1;
		      goto stop;
		  }
		if (int_to != *has_ids)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("NodeTo column \"%s\": found a mismatching value",
			   to_column);
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      error = 1;
		      goto stop;
		  }
		if (geom_column != NULL)
		  {
		      if (sqlite3_column_type (stmt, 3) == SQLITE_BLOB)
			{
			    int srid;
			    int dims;
			    struct splite_internal_cache *p_cache =
				(struct splite_internal_cache *) cache;
			    int gpkg_amphibious = 0;
			    int gpkg_mode = 0;
			    unsigned char *blob =
				(unsigned char *) sqlite3_column_blob (stmt, 3);
			    int bytes = sqlite3_column_bytes (stmt, 3);
			    if (cache != NULL)
			      {
				  gpkg_amphibious =
				      p_cache->gpkg_amphibious_mode;
				  gpkg_mode = p_cache->gpkg_mode;
			      }
			    gaiaGeomCollPtr g =
				gaiaFromSpatiaLiteBlobWkbEx (blob, bytes,
							     gpkg_mode,
							     gpkg_amphibious);
			    if (g == NULL)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("Geometry column \"%s\": found a BLOB value that's not a Geometry",
				       geom_column);
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  error = 1;
				  goto stop;
			      }
			    ok_geom =
				check_geom (g, &is3d, &x_from, &y_from, &z_from,
					    &x_to, &y_to, &z_to);
			    if (ok_geom)
			      {
				  srid = g->Srid;
				  dims = g->DimensionModel;
			      }
			    gaiaFreeGeomColl (g);
			    if (!ok_geom)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("Geometry column \"%s\": found a Geometry that's not a simple Linestring",
				       geom_column);
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  error = 1;
				  goto stop;
			      }
			    if (srid != glob_srid)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("Geometry column \"%s\": found a mismatching SRID",
				       geom_column);
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  error = 1;
				  goto stop;
			      }
			    if (dims != glob_dims)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("Geometry column \"%s\": found mismatching Dimensions",
				       geom_column);
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  error = 1;
				  goto stop;
			      }
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("Geometry column \"%s\": found a value that's not a BLOB",
				 geom_column);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		else
		  {
		      /* no geometry columns */
		      x_from = 0.0;
		      y_from = 0.0;
		      z_from = 0.0;
		      x_to = 0.0;
		      y_to = 0.0;
		      z_to = 0.0;
		  }
		if (a_star_enabled || cost_column == NULL)
		  {
		      if (sqlite3_column_type (stmt, 4) == SQLITE_FLOAT)
			  length = sqlite3_column_double (stmt, 4);
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("Geometry column \"%s\": ST_Length() returned an invalid value",
				 geom_column);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		if (cost_column != NULL)
		  {
		      if (sqlite3_column_type (stmt, 5) == SQLITE_FLOAT)
			{
			    cost = sqlite3_column_double (stmt, 5);
			    if (cost <= 0.0)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("Cost column \"%s\": found a negative or zero value",
				       cost_column);
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  error = 1;
				  goto stop;
			      }
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("Cost column \"%s\": found a value that's not a DOUBLE",
				 cost_column);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		else
		    cost = length;
		if (a_star_enabled)
		  {
		      if (cost_column == NULL)
			  *a_star_coeff = 1.0;
		      else
			{
			    double coeff = cost / length;
			    if (coeff < *a_star_coeff)
				*a_star_coeff = coeff;
			}
		  }
		if (name_column != NULL)
		  {
		      if (sqlite3_column_type (stmt, 6) == SQLITE_TEXT)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("Name column \"%s\": found a value that's not a TEXT string",
				 name_column);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		if (oneway_from != NULL)
		  {
		      if (sqlite3_column_type (stmt, 7) == SQLITE_INTEGER)
			  from_to = sqlite3_column_int (stmt, 7);
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("OnewayFromTo column \"%s\": found a value that's not an INTEGER",
				 oneway_from);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		if (oneway_to != NULL)
		  {
		      if (sqlite3_column_type (stmt, 8) == SQLITE_INTEGER)
			  to_from = sqlite3_column_int (stmt, 8);
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("OnewayToFrom column \"%s\": found a value that's not an INTEGER",
				 oneway_to);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto stop;
			}
		  }
		if (first)
		    first = 0;

		/* inserting NodeFrom */
		sqlite3_reset (stmt_ins_nodes);
		sqlite3_clear_bindings (stmt_ins_nodes);
		if (int_from)
		  {
		      sqlite3_bind_int64 (stmt_ins_nodes, 1, id_from);
		      sqlite3_bind_null (stmt_ins_nodes, 2);
		  }
		else
		  {
		      sqlite3_bind_null (stmt_ins_nodes, 1);
		      sqlite3_bind_text (stmt_ins_nodes, 2, from, strlen (from),
					 SQLITE_STATIC);
		  }
		sqlite3_bind_double (stmt_ins_nodes, 3, x_from);
		sqlite3_bind_double (stmt_ins_nodes, 4, y_from);
		sqlite3_bind_double (stmt_ins_nodes, 5, z_from);
		ret = sqlite3_step (stmt_ins_nodes);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("SQL error: %s", sqlite3_errmsg (db_handle));
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      goto stop;
		  }

		/* inserting NodeTo */
		sqlite3_reset (stmt_ins_nodes);
		sqlite3_clear_bindings (stmt_ins_nodes);
		if (int_to)
		  {
		      sqlite3_bind_int64 (stmt_ins_nodes, 1, id_to);
		      sqlite3_bind_null (stmt_ins_nodes, 2);
		  }
		else
		  {
		      sqlite3_bind_null (stmt_ins_nodes, 1);
		      sqlite3_bind_text (stmt_ins_nodes, 2, to, strlen (to),
					 SQLITE_STATIC);
		  }
		sqlite3_bind_double (stmt_ins_nodes, 3, x_to);
		sqlite3_bind_double (stmt_ins_nodes, 4, y_to);
		sqlite3_bind_double (stmt_ins_nodes, 5, z_to);
		ret = sqlite3_step (stmt_ins_nodes);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("SQL error: %s", sqlite3_errmsg (db_handle));
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      goto stop;
		  }

		/* inserting the Link(s) */
		if (bidirectional)
		  {
		      if (from_to)
			{
			    sqlite3_reset (stmt_ins_links);
			    sqlite3_clear_bindings (stmt_ins_links);
			    sqlite3_bind_int64 (stmt_ins_links, 1, rowid);
			    if (int_from)
			      {
				  sqlite3_bind_int64 (stmt_ins_links, 2,
						      id_from);
				  sqlite3_bind_null (stmt_ins_links, 3);
			      }
			    else
			      {
				  sqlite3_bind_null (stmt_ins_links, 2);
				  sqlite3_bind_text (stmt_ins_links, 3, from,
						     strlen (from),
						     SQLITE_STATIC);
			      }
			    if (int_to)
			      {
				  sqlite3_bind_int64 (stmt_ins_links, 4, id_to);
				  sqlite3_bind_null (stmt_ins_links, 5);
			      }
			    else
			      {
				  sqlite3_bind_null (stmt_ins_links, 4);
				  sqlite3_bind_text (stmt_ins_links, 5, to,
						     strlen (to),
						     SQLITE_STATIC);
			      }
			    sqlite3_bind_double (stmt_ins_links, 6, cost);
			    ret = sqlite3_step (stmt_ins_links);
			    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				;
			    else
			      {
				  char *msg = sqlite3_mprintf ("SQL error: %s",
							       sqlite3_errmsg
							       (db_handle));
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  goto stop;
			      }
			}
		      if (to_from)
			{
			    sqlite3_reset (stmt_ins_links);
			    sqlite3_clear_bindings (stmt_ins_links);
			    sqlite3_bind_int64 (stmt_ins_links, 1, rowid);
			    if (int_to)
			      {
				  sqlite3_bind_int64 (stmt_ins_links, 2, id_to);
				  sqlite3_bind_null (stmt_ins_links, 3);
			      }
			    else
			      {
				  sqlite3_bind_null (stmt_ins_links, 2);
				  sqlite3_bind_text (stmt_ins_links, 3, to,
						     strlen (to),
						     SQLITE_STATIC);
			      }
			    if (int_from)
			      {
				  sqlite3_bind_int64 (stmt_ins_links, 4,
						      id_from);
				  sqlite3_bind_null (stmt_ins_links, 5);
			      }
			    else
			      {
				  sqlite3_bind_null (stmt_ins_links, 4);
				  sqlite3_bind_text (stmt_ins_links, 5, from,
						     strlen (from),
						     SQLITE_STATIC);
			      }
			    sqlite3_bind_double (stmt_ins_links, 6, cost);
			    ret = sqlite3_step (stmt_ins_links);
			    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				;
			    else
			      {
				  char *msg = sqlite3_mprintf ("SQL error: %s",
							       sqlite3_errmsg
							       (db_handle));
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  goto stop;
			      }
			}
		  }
		else
		  {
		      sqlite3_reset (stmt_ins_links);
		      sqlite3_clear_bindings (stmt_ins_links);
		      sqlite3_bind_int64 (stmt_ins_links, 1, rowid);
		      if (int_from)
			{
			    sqlite3_bind_int64 (stmt_ins_links, 2, id_from);
			    sqlite3_bind_null (stmt_ins_links, 3);
			}
		      else
			{
			    sqlite3_bind_null (stmt_ins_links, 2);
			    sqlite3_bind_text (stmt_ins_links, 3, from,
					       strlen (from), SQLITE_STATIC);
			}
		      if (int_to)
			{
			    sqlite3_bind_int64 (stmt_ins_links, 4, id_to);
			    sqlite3_bind_null (stmt_ins_links, 5);
			}
		      else
			{
			    sqlite3_bind_null (stmt_ins_links, 4);
			    sqlite3_bind_text (stmt_ins_links, 5, to,
					       strlen (to), SQLITE_STATIC);
			}
		      sqlite3_bind_double (stmt_ins_links, 6, length);
		      sqlite3_bind_double (stmt_ins_links, 7, cost);
		      ret = sqlite3_step (stmt_ins_links);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("SQL error: %s", sqlite3_errmsg (db_handle));
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    goto stop;
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto stop;
	    }
      }
  stop:
    if (stmt_ins_nodes != NULL)
	sqlite3_finalize (stmt_ins_nodes);
    if (stmt_ins_links != NULL)
	sqlite3_finalize (stmt_ins_links);
    sqlite3_finalize (stmt);
    if (error)
      {
	  /* rolling back the Savepoint */
	  sql = "ROLLBACK TO create_routing_one";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
	  return 0;
      }

/* releasing the Savepoint */
    sql = "RELEASE SAVEPOINT create_routing_one";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto stop;
      }

/* setting a Savepoint */
    sql = "SAVEPOINT create_routing_two";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* preparing the UPDATE tmp-Nodes statement */
    sql = "UPDATE create_routing_nodes SET internal_index = ? WHERE rowid = ?";
    ret =
	sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_update_nodes,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking Nodes topological consistency */
    if (*has_ids)
      {
	  sql = "SELECT rowid, node_id, node_x, node_y, node_z "
	      "FROM create_routing_nodes ORDER BY node_id, rowid";
      }
    else
      {
	  sql = "SELECT rowid, node_code, node_x, node_y, node_z "
	      "FROM create_routing_nodes ORDER BY node_code, rowid";
      }
    ret =
	sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_check_nodes,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    first = 1;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_check_nodes);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid;
		const char *code;
		sqlite3_int64 id;
		double x;
		double y;
		double z;
		int mismatch = 0;
		rowid = sqlite3_column_int64 (stmt_check_nodes, 0);
		if (*has_ids)
		    id = sqlite3_column_int64 (stmt_check_nodes, 1);
		else
		    code =
			(const char *) sqlite3_column_text (stmt_check_nodes,
							    1);
		x = sqlite3_column_double (stmt_check_nodes, 2);
		y = sqlite3_column_double (stmt_check_nodes, 3);
		z = sqlite3_column_double (stmt_check_nodes, 4);
		if (first)
		  {
		      first = 0;
		      if (!do_update_internal_index
			  (db_handle, cache, stmt_update_nodes, rowid,
			   *n_nodes))
			  goto error;
		      *n_nodes += 1;
		      goto skip_first;
		  }
		if (*has_ids)
		  {
		      if (last_id != id)
			{
			    last_x = x;
			    last_y = y;
			    last_z = z;
			    if (!do_update_internal_index
				(db_handle, cache, stmt_update_nodes, rowid,
				 *n_nodes))
				goto error;
			    *n_nodes += 1;
			}
		  }
		else
		  {
		      if (strcmp (last_code, code) != 0)
			{
			    last_x = x;
			    last_y = y;
			    last_z = z;
			    if (!do_update_internal_index
				(db_handle, cache, stmt_update_nodes, rowid,
				 *n_nodes))
				goto error;
			    *n_nodes += 1;
			}
		  }
		if (x != last_x)
		    mismatch = 1;
		if (y != last_y)
		    mismatch = 1;
		if (z != last_z)
		    mismatch = 1;
		if (mismatch)
		  {
		      if (*has_ids)
			{
			    char *msg;
			    char xid[64];
			    sprintf (xid, FRMT64, id);
			    msg = sqlite3_mprintf
				("Node Id=%s: topology error", xid);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto error;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("Node Code=%s: topology error", code);
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto error;
			}
		  }
	      skip_first:
		if (*has_ids)
		    last_id = id;
		else
		  {
		      int len = strlen (code);
		      if (last_code != NULL)
			  free (last_code);
		      last_code = malloc (len + 1);
		      strcpy (last_code, code);
		  }
		last_x = x;
		last_y = y;
		last_z = z;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto error;
	    }
      }
  error:
    if (last_code != NULL)
	free (last_code);
    if (stmt_check_nodes != NULL)
	sqlite3_finalize (stmt_check_nodes);
    if (stmt_update_nodes != NULL)
	sqlite3_finalize (stmt_update_nodes);
    if (error)
      {
	  /* rolling back the Savepoint */
	  sql = "ROLLBACK TO create_routing_two";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
	  return 0;
      }

/* releasing the Savepoint */
    sql = "RELEASE SAVEPOINT create_routing_two";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto end;
      }

/* setting a Savepoint */
    sql = "SAVEPOINT create_routing_three";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* preparing an index supporting Temp-Nodes - internal index */
    sql = "CREATE INDEX idx_create_routing_internal "
	"ON create_routing_nodes (internal_index)";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto end;
      }

    if (*has_ids)
      {
	  /* preparing an index supporting Temp-Links - Id Node From */
	  sql =
	      "CREATE INDEX idx_create_routing_internal_from ON create_routing_links (id_node_from)";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
	  /* preparing an index supporting Temp-Links - Id Node To */
	  sql =
	      "CREATE INDEX idx_create_routing_internal_to ON create_routing_links (id_node_to)";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
      }
    else
      {
	  /* preparing an index supporting Temp-Links - Code Node From */
	  sql =
	      "CREATE INDEX idx_create_routing_internal_from ON create_routing_links (cod_node_from)";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		goto end;
		error = 1;
	    }
	  /* preparing an index supporting Temp-Links - Code Node To */
	  sql =
	      "CREATE INDEX idx_create_routing_internal_to ON create_routing_links (cod_node_to)";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
      }

    if (*has_ids)
      {
	  /* preparing the UPDATE tmp-Links statement - index from */
	  sql =
	      "UPDATE create_routing_links SET index_from = ? WHERE id_node_from = ?";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql),
				  &stmt_update_links_from, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
	  /* preparing the UPDATE tmp-Links statement - index to */
	  sql =
	      "UPDATE create_routing_links SET index_to = ? WHERE id_node_to = ?";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql),
				  &stmt_update_links_to, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
	  /* preparing the SELECT tmp-Nodes statement */
	  sql = "SELECT internal_index, node_id FROM create_routing_nodes "
	      "WHERE internal_index IS NOT NULL";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_sel_nodes,
				  NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
      }
    else
      {
	  /* preparing the UPDATE tmp-Links statement - index from */
	  sql =
	      "UPDATE create_routing_links SET index_from = ? WHERE cod_node_from = ?";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql),
				  &stmt_update_links_from, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
	  /* preparing the UPDATE tmp-Links statement - index to */
	  sql =
	      "UPDATE create_routing_links SET index_to = ? WHERE cod_node_to = ?";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql),
				  &stmt_update_links_to, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
	  /* preparing the SELECT tmp-Nodes statement */
	  sql = "SELECT internal_index, node_code FROM create_routing_nodes "
	      "WHERE internal_index IS NOT NULL";
	  ret =
	      sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_sel_nodes,
				  NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_sel_nodes);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int index;
		const char *code;
		sqlite3_int64 id;
		index = sqlite3_column_int (stmt_sel_nodes, 0);
		if (*has_ids)
		    id = sqlite3_column_int64 (stmt_sel_nodes, 1);
		else
		    code =
			(const char *) sqlite3_column_text (stmt_sel_nodes, 1);
		/* updating Index From */
		sqlite3_reset (stmt_update_links_from);
		sqlite3_clear_bindings (stmt_update_links_from);
		sqlite3_bind_int (stmt_update_links_from, 1, index);
		if (*has_ids)
		    sqlite3_bind_int64 (stmt_update_links_from, 2, id);
		else
		    sqlite3_bind_text (stmt_update_links_from, 2, code,
				       strlen (code), SQLITE_STATIC);
		ret = sqlite3_step (stmt_update_links_from);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("SQL error: %s", sqlite3_errmsg (db_handle));
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
		/* updating Index To */
		sqlite3_reset (stmt_update_links_to);
		sqlite3_clear_bindings (stmt_update_links_to);
		sqlite3_bind_int (stmt_update_links_to, 1, index);
		if (*has_ids)
		    sqlite3_bind_int64 (stmt_update_links_to, 2, id);
		else
		    sqlite3_bind_text (stmt_update_links_to, 2, code,
				       strlen (code), SQLITE_STATIC);
		ret = sqlite3_step (stmt_update_links_to);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("SQL error: %s", sqlite3_errmsg (db_handle));
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto end;
	    }
      }

/* dropping Temp-Links indices */
    sql = "DROP INDEX idx_create_routing_internal_from";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("SQL error: %s",
				       sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto end;
      }
    sql = "DROP INDEX idx_create_routing_internal_to";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("SQL error: %s",
				       sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto end;
      }
/* preparing an index supporting Temp-Links - final */
    sql =
	"CREATE INDEX idx_create_routing_internal_final ON create_routing_links (index_from, cost, index_to)";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("SQL error: %s",
				       sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto end;
      }

  end:
    if (stmt_sel_nodes != NULL)
	sqlite3_finalize (stmt_sel_nodes);
    if (stmt_update_links_from != NULL)
	sqlite3_finalize (stmt_update_links_from);
    if (stmt_update_links_to != NULL)
	sqlite3_finalize (stmt_update_links_to);
    if (error)
      {
	  /* rolling back the Savepoint */
	  sql = "ROLLBACK TO create_routing_three";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
	  return 0;
      }

/* releasing the Savepoint */
    sql = "RELEASE SAVEPOINT create_routing_three";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
do_prepare_header (unsigned char *buf, int endian_arch, int n_nodes,
		   int has_ids, int max_code_length, const char *input_table,
		   const char *from_column, const char *to_column,
		   const char *geom_column, const char *name_column,
		   int a_star_supported, double a_star_coeff)
{
/* preparing the HEADER block */
    int len;
    unsigned char *out = buf;
    if (a_star_supported)
	*out++ = GAIA_NET64_A_STAR_START;
    else
	*out++ = GAIA_NET64_START;
    *out++ = GAIA_NET_HEADER;
    gaiaExport32 (out, n_nodes, 1, endian_arch);	/* how many Nodes are there */
    out += 4;
    if (has_ids)
	*out++ = GAIA_NET_ID;	/* Nodes are identified by an INTEGER id */
    else
	*out++ = GAIA_NET_CODE;	/* Nodes are identified by a TEXT code */
    if (has_ids)
	*out++ = 0x00;
    else
	*out++ = max_code_length;	/* max TEXT code length */
/* inserting the main Table name */
    *out++ = GAIA_NET_TABLE;
    len = strlen (input_table) + 1;
    gaiaExport16 (out, len, 1, endian_arch);	/* the Table Name length, including last '\0' */
    out += 2;
    memset (out, '\0', len);
    strcpy ((char *) out, input_table);
    out += len;
/* inserting the NodeFrom column name */
    *out++ = GAIA_NET_FROM;
    len = strlen (from_column) + 1;
    gaiaExport16 (out, len, 1, endian_arch);	/* the NodeFrom column Name length, including last '\0' */
    out += 2;
    memset (out, '\0', len);
    strcpy ((char *) out, from_column);
    out += len;
/* inserting the NodeTo column name */
    *out++ = GAIA_NET_TO;
    len = strlen (to_column) + 1;
    gaiaExport16 (out, len, 1, endian_arch);	/* the NodeTo column Name length, including last '\0' */
    out += 2;
    memset (out, '\0', len);
    strcpy ((char *) out, to_column);
    out += len;
/* inserting the Geometry column name */
    *out++ = GAIA_NET_GEOM;
    if (!geom_column)
	len = 1;
    else
	len = strlen (geom_column) + 1;
    gaiaExport16 (out, len, 1, endian_arch);	/* the Geometry column Name length, including last '\0' */
    out += 2;
    memset (out, '\0', len);
    if (geom_column)
	strcpy ((char *) out, geom_column);
    out += len;
/* inserting the Name column name - may be empty */
    *out++ = GAIA_NET_NAME;
    if (!name_column)
	len = 1;
    else
	len = strlen (name_column) + 1;
    gaiaExport16 (out, len, 1, endian_arch);	/* the Name column Name length, including last '\0' */
    out += 2;
    memset (out, '\0', len);
    if (name_column)
	strcpy ((char *) out, name_column);
    out += len;
    if (a_star_supported)
      {
	  /* inserting the A* Heuristic Coeff */
	  *out++ = GAIA_NET_A_STAR_COEFF;
	  gaiaExport64 (out, a_star_coeff, 1, endian_arch);
	  out += 8;
      }
    *out++ = GAIA_NET_END;
    return out - buf;
}

static int
output_node (unsigned char *auxbuf, int *size, int index, int has_ids,
	     int max_code_length, int endian_arch, int a_star_enabled,
	     sqlite3_int64 id, const char *code, double x, double y,
	     short count_outcomings, sqlite3 * db_handle, const void *cache,
	     sqlite3_stmt * stmt_to)
{
/* exporting a Node into NETWORK-DATA */
    int ret;
    unsigned char *out = auxbuf;
    *out++ = GAIA_NET_NODE;
    gaiaExport32 (out, index, 1, endian_arch);	/* the Node internal index */
    out += 4;
    if (has_ids)
      {
	  /* Nodes are identified by an INTEGER Id */
	  gaiaExportI64 (out, id, 1, endian_arch);	/* the Node ID */
	  out += 8;
      }
    else
      {
	  /* Nodes are identified by a TEXT Code */
	  memset (out, '\0', max_code_length);
	  strcpy ((char *) out, code);
	  out += max_code_length;
      }
    if (a_star_enabled)
      {
	  /* in order to support the A* algorithm [X,Y] are required for each node */
	  gaiaExport64 (out, x, 1, endian_arch);
	  out += 8;
	  gaiaExport64 (out, y, 1, endian_arch);
	  out += 8;
      }
    gaiaExport16 (out, count_outcomings, 1, endian_arch);	/* # of outcoming arcs */
    out += 2;

/* querying all outcoming arcs/links */
    sqlite3_reset (stmt_to);
    sqlite3_clear_bindings (stmt_to);
    sqlite3_bind_int (stmt_to, 1, index);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_to);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_to, 0);
		int index_to = sqlite3_column_int (stmt_to, 1);
		double cost = sqlite3_column_double (stmt_to, 2);
		*out++ = GAIA_NET_ARC;
		gaiaExportI64 (out, rowid, 1, endian_arch);	/* the Arc rowid */
		out += 8;
		gaiaExport32 (out, index_to, 1, endian_arch);	/* the ToNode internal index */
		out += 4;
		gaiaExport64 (out, cost, 1, endian_arch);	/* the Arc Cost */
		out += 8;
		*out++ = GAIA_NET_END;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    *out++ = GAIA_NET_END;
    *size = out - auxbuf;
    return 1;
}

static int
do_create_data (sqlite3 * db_handle, const void *cache,
		const char *output_table, const char *input_table,
		const char *from_column, const char *to_column,
		const char *geom_column, const char *name_column,
		int a_star_enabled, double a_star_coeff, int has_ids,
		int n_nodes, int max_code_length)
{
/* creating and populating the Routing Data table */
    char *sql;
    int ret;
    char *xtable;
    sqlite3_stmt *stmt_from = NULL;
    sqlite3_stmt *stmt_to = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int error = 0;
    unsigned char *auxbuf = NULL;
    unsigned char *buf = NULL;
    unsigned char *out;
    int nodes_cnt;
    int size;
    int endian_arch = gaiaEndianArch ();

/* setting a Savepoint */
    sql = "SAVEPOINT create_routing_four";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* creating the NETWORK-DATA table */
    xtable = gaiaDoubleQuotedSql (output_table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" ("
			   "Id INTEGER PRIMARY KEY,\nNetworkData BLOB NOT NULL)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto error;
      }

/* preparing the Insert SQL statement */
    xtable = gaiaDoubleQuotedSql (output_table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (Id, NetworkData) VALUES (?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto error;
      }

/* preparing the Select Node From SQL statement */
    if (has_ids)
	sql =
	    "SELECT n.internal_index, n.node_id, n.node_x, n.node_y, Count(l.rowid) "
	    "FROM create_routing_nodes AS n "
	    "LEFT JOIN create_routing_links as l ON (l.index_from = n.internal_index) "
	    "WHERE n.internal_index IS NOT NULL " "GROUP BY n.internal_index";
    else
	sql =
	    "SELECT n.internal_index, n.node_code, n.node_x, n.node_y, Count(l.rowid) "
	    "FROM create_routing_nodes AS n "
	    "LEFT JOIN create_routing_links as l ON (l.index_from = n.internal_index) "
	    "WHERE n.internal_index IS NOT NULL " "GROUP BY n.internal_index";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_from, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto error;
      }

/* preparing the Select Node To SQL statement */
    sql = "SELECT rowid, index_to, cost "
	"FROM create_routing_links "
	"WHERE index_from = ? " "ORDER BY cost, index_to";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_to, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto error;
      }

/* preparing and inserting the Routing Header block */
    buf = malloc (MAX_BLOCK);
    size =
	do_prepare_header (buf, endian_arch, n_nodes, has_ids, max_code_length,
			   input_table, from_column, to_column, geom_column,
			   name_column, a_star_enabled, a_star_coeff);
    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    sqlite3_bind_int (stmt_out, 1, 0);
    sqlite3_bind_blob (stmt_out, 2, buf, size, SQLITE_STATIC);
    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  error = 1;
	  goto error;
      }

/*
/ looping on Nodes 
/ preparing a new data block 
*/
    out = buf;
    *out++ = GAIA_NET_BLOCK;
    gaiaExport16 (out, 0, 1, endian_arch);	/* how many Nodes are into this block */
    out += 2;
    nodes_cnt = 0;
    auxbuf = malloc (MAX_BLOCK);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_from);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int index;
		const char *code = NULL;
		sqlite3_int64 id = -1;
		double x;
		double y;
		int count_outcomings;
		index = sqlite3_column_int (stmt_from, 0);
		if (has_ids)
		    id = sqlite3_column_int64 (stmt_from, 1);
		else
		    code = (const char *) sqlite3_column_text (stmt_from, 1);
		x = sqlite3_column_double (stmt_from, 2);
		y = sqlite3_column_double (stmt_from, 3);
		count_outcomings = sqlite3_column_int (stmt_from, 4);
		if (!output_node
		    (auxbuf, &size, index, has_ids, max_code_length,
		     endian_arch, a_star_enabled, id, code, x, y,
		     count_outcomings, db_handle, cache, stmt_to))
		  {
		      error = 1;
		      goto error;
		  }
		if (size >= (MAX_BLOCK - (out - buf)))
		  {
		      /* inserting the last block */
		      gaiaExport16 (buf + 1, nodes_cnt, 1, endian_arch);	/* how many Nodes are into this block */
		      sqlite3_reset (stmt_out);
		      sqlite3_clear_bindings (stmt_out);
		      sqlite3_bind_null (stmt_out, 1);
		      sqlite3_bind_blob (stmt_out, 2, buf, out - buf,
					 SQLITE_STATIC);
		      ret = sqlite3_step (stmt_out);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("SQL error: %s", sqlite3_errmsg (db_handle));
			    gaia_create_routing_set_error (cache, msg);
			    sqlite3_free (msg);
			    error = 1;
			    goto error;
			}
		      /* preparing a new block */
		      out = buf;
		      *out++ = GAIA_NET_BLOCK;
		      gaiaExport16 (out, 0, 1, endian_arch);	/* how many Nodes are into this block */
		      out += 2;
		      nodes_cnt = 0;
		  }
		/* inserting the current Node into the block */
		nodes_cnt++;
		memcpy (out, auxbuf, size);
		out += size;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto error;
	    }
      }
    if (nodes_cnt)
      {
	  /* inserting the last data block */
	  gaiaExport16 (buf + 1, nodes_cnt, 1, endian_arch);	/* how many Nodes are into this block */
	  sqlite3_reset (stmt_out);
	  sqlite3_clear_bindings (stmt_out);
	  sqlite3_bind_null (stmt_out, 1);
	  sqlite3_bind_blob (stmt_out, 2, buf, out - buf, SQLITE_STATIC);
	  ret = sqlite3_step (stmt_out);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("SQL error: %s", sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		error = 1;
		goto error;
	    }
      }

  error:
    if (auxbuf != NULL)
	free (auxbuf);
    if (buf != NULL)
	free (buf);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    if (stmt_from != NULL)
	sqlite3_finalize (stmt_from);
    if (stmt_to != NULL)
	sqlite3_finalize (stmt_to);
    if (error)
      {
	  /* rolling back the Savepoint */
	  sql = "ROLLBACK TO create_routing_four";
	  ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		return 0;
	    }
	  return 0;
      }

/* releasing the Savepoint */
    sql = "RELEASE SAVEPOINT create_routing_four";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
do_create_virtual_routing (sqlite3 * db_handle, const void *cache,
			   const char *routing_data_table,
			   const char *virtual_routing_table)
{
/* creating the VirtualRouting table - dropping the Temp-Tables */
    char *xtable1;
    char *xtable2;
    char *sql;
    int ret;

    xtable1 = gaiaDoubleQuotedSql (virtual_routing_table);
    xtable2 = gaiaDoubleQuotedSql (routing_data_table);
    sql =
	sqlite3_mprintf
	("CREATE VIRTUAL TABLE \"%s\" USING  VirtualRouting(\"%s\")", xtable1,
	 xtable2);
    free (xtable1);
    free (xtable2);
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto rollback;
      }
    do_drop_temp_tables (db_handle);

/* releasing the Savepoint */
    sql = "RELEASE SAVEPOINT create_routing_zero";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;

  rollback:
    sql = "ROLLBACK TO create_routing_zero";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 0;
}

static int
do_check_registered_spatial_table (sqlite3 * db_handle, const char *db_prefix,
				   const char *table, const char *geom_column,
				   char **geom_name)
{
/* checking for a valid Spatial Table */
    char *xprefix;
    char *sql;
    char *name = NULL;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int type;
    int count = 0;
    int is_linestring = 0;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);

    if (geom_column == NULL)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT f_geometry_column, geometry_type FROM \"%s\".geometry_columns WHERE "
	       "Lower(f_table_name) = Lower(%Q)", xprefix, table);
      }
    else
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT f_geometry_column, geometry_type FROM \"%s\".geometry_columns WHERE "
	       "Lower(f_table_name) = Lower(%Q) AND Lower(f_geometry_column) = Lower(%Q)",
	       xprefix, table, geom_column);
      }
    free (xprefix);
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *geom = (const char *) sqlite3_column_text (stmt, 0);
		int len = strlen (geom);
		if (name != NULL)
		    free (name);
		name = malloc (len + 1);
		strcpy (name, geom);
		type = sqlite3_column_int (stmt, 1);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  switch (type)
	    {
	    case GAIA_LINESTRING:
	    case GAIA_LINESTRINGZ:
	    case GAIA_LINESTRINGM:
	    case GAIA_LINESTRINGZM:
		is_linestring = 1;
		break;
	    default:
		is_linestring = 0;
		break;
	    };
      }
    if (is_linestring)
      {
	  *geom_name = name;
	  return 1;
      }
    if (name != NULL)
	free (name);
    return 0;
}

static int
do_check_spatial_table (sqlite3 * db_handle, const void *cache,
			const char *db_prefix, const char *table,
			const char *geom_column, const char *from_column,
			const char *to_column, char **geom_name)
{
/* testing if the Spatial Table exists and it's correctly defined */
    char *xprefix;
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int ok_from = 0;
    int ok_to = 0;
    int ok_geom = 0;

/* checking for a valid Spatial Table */
    *geom_name = NULL;
    if (!do_check_registered_spatial_table
	(db_handle, db_prefix, table, geom_column, geom_name))
      {
	  char *msg;
	  if (geom_column == NULL)
	      msg =
		  sqlite3_mprintf
		  ("%Q is not a valid Spatial Table (LINESTRING)", table);
	  else
	      msg =
		  sqlite3_mprintf
		  ("%Q.%Q is not a valid Spatial Table (LINESTRING)", table,
		   geom_column);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free(msg);
	  return 0;
      }

/* checking the table's columns */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
      {
	  const char *col = results[(i * columns) + 1];
	  if (strcasecmp (col, from_column) == 0)
	      ok_from = 1;
	  if (strcasecmp (col, to_column) == 0)
	      ok_to = 1;
	  if (strcasecmp (col, *geom_name) == 0)
	      ok_geom = 1;
      }
    sqlite3_free_table (results);

/* final validity check */
    if (ok_from)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("FromNode Column \"%s\" is already defined in the Spatial Table",
	       from_column);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    if (ok_to)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ToNode Column \"%s\" is already defined in the Spatial Table",
	       to_column);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    if (!ok_geom)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("Geometry Column \"%s\" is not defined in the Spatial Table",
	       geom_name);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
do_insert_temp_aux_node (sqlite3 * db_handle, sqlite3_stmt * stmt, double x,
			 double y, double z, char **msg)
{
/* inserting an Aux Node */
    int ret;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, x);
    sqlite3_bind_double (stmt, 2, y);
    sqlite3_bind_double (stmt, 3, z);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

/* some error occurred */
    *msg = sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
    return 0;
}

static int
do_update_link (sqlite3 * db_handle, sqlite3_stmt * stmt,
		sqlite3_stmt * stmt_upd, int rowid, double from_x,
		double from_y, double from_z, double to_x, double to_y,
		double to_z, char **msg)
{
/* updating the Spatial Table - step #1 */
    int ret;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, from_x);
    sqlite3_bind_double (stmt, 2, from_y);
    sqlite3_bind_double (stmt, 3, from_z);
    sqlite3_bind_double (stmt, 4, to_x);
    sqlite3_bind_double (stmt, 5, to_y);
    sqlite3_bind_double (stmt, 6, to_z);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int node_from = sqlite3_column_int (stmt, 0);
		int node_to = sqlite3_column_int (stmt, 1);

		sqlite3_reset (stmt_upd);
		sqlite3_clear_bindings (stmt_upd);
		sqlite3_bind_int (stmt_upd, 1, node_from);
		sqlite3_bind_int (stmt_upd, 2, node_to);
		sqlite3_bind_int (stmt_upd, 3, rowid);
		ret = sqlite3_step (stmt_upd);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    return 1;
		else
		  {
		      *msg =
			  sqlite3_mprintf ("SQL error: %s",
					   sqlite3_errmsg (db_handle));
		      return 0;
		  }

/* some error occurred */
		*msg =
		    sqlite3_mprintf ("SQL error: %s",
				     sqlite3_errmsg (db_handle));
		return 0;
	    }
	  else
	    {
		*msg =
		    sqlite3_mprintf ("SQL error: %s",
				     sqlite3_errmsg (db_handle));
		return 0;
	    }
      }
    return 1;
}

static int
do_create_routing_nodes (sqlite3 * db_handle, const void *cache,
			 const char *db_prefix, const char *table,
			 const char *geom_column, const char *from_column,
			 const char *to_column)
{
/* creating and populating the Routing Nodes columns */
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *xfrom;
    char *xto;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_q1 = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_q2 = NULL;
    sqlite3_stmt *stmt_upd = NULL;

/* adding the FromNode column */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (from_column);
    sql =
	sqlite3_mprintf ("ALTER TABLE \"%s\".\"%s\" ADD COLUMN \"%s\" INTEGER",
			 xprefix, xtable, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* adding the ToNode column */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (to_column);
    sql =
	sqlite3_mprintf ("ALTER TABLE \"%s\".\"%s\" ADD COLUMN \"%s\" INTEGER",
			 xprefix, xtable, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* dropping the Temporary Aux Table (just in case) */
    sql = "DROP TABLE temp_create_routing_aux_nodes";
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);

/* creating the Temporary Aux Table for Nodes */
    sql = "CREATE TEMPORARY TABLE temp_create_routing_aux_nodes ("
	"x DOUBLE, y DOUBLE, z DOUBLE, "
	"CONSTRAINT pk_create_routing_aux_nodes PRIMARY KEY (x, y, z))";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the query statement for identifying all Nodes */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (geom_column);
    sql =
	sqlite3_mprintf
	("SELECT ROWID, ST_StartPoint(\"%s\"), ST_EndPoint(\"%s\") "
	 "FROM \"%s\".\"%s\"", xcolumn, xcolumn, xprefix, xtable);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_q1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the INSERT statement for Aux Nodes */
    sql = "INSERT OR IGNORE INTO temp_create_routing_aux_nodes "
	"VALUES (?, ?, ?)";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_ins, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the query statement for identifying all Node-IDs */
    sql = "SELECT a.ROWID, b.ROWID "
	"FROM temp_create_routing_aux_nodes AS a, "
	"temp_create_routing_aux_nodes AS b "
	"WHERE a.x = ? AND a.y = ? AND a.z = ? "
	"AND b.x = ? AND b.y = ? AND b.z = ?";
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_q2, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the UPDATE statement */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xfrom = gaiaDoubleQuotedSql (from_column);
    xto = gaiaDoubleQuotedSql (to_column);
    sql = sqlite3_mprintf ("UPDATE \"%s\".\"%s\" SET \"%s\" = ?, \"%s\" = ? "
			   "WHERE ROWID = ?", xprefix, xtable, xfrom, xto);
    free (xprefix);
    free (xtable);
    free (xfrom);
    free (xto);
    ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* populating the Temp Aux Nodes */
    while (1)
      {
	  ret = sqlite3_step (stmt_q1);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob;
		int size;
		gaiaGeomCollPtr geom;
		gaiaPointPtr pt;
		double x;
		double y;
		double z;
		char *msg;
		if (sqlite3_column_type (stmt_q1, 1) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_q1,
								       1);
		      size = sqlite3_column_bytes (stmt_q1, 1);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom)
			{
			    pt = geom->FirstPoint;
			    x = pt->X;
			    y = pt->Y;
			    if (geom->DimensionModel == GAIA_XY_Z
				|| geom->DimensionModel == GAIA_XY_Z_M)
				z = pt->Z;
			    else
				z = 0.0;
			    gaiaFreeGeomColl (geom);
			    if (!do_insert_temp_aux_node
				(db_handle, stmt_ins, x, y, z, &msg))
			      {
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  goto error;
			      }
			}
		  }
		if (sqlite3_column_type (stmt_q1, 2) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_q1,
								       2);
		      size = sqlite3_column_bytes (stmt_q1, 2);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom)
			{
			    pt = geom->FirstPoint;
			    x = pt->X;
			    y = pt->Y;
			    if (geom->DimensionModel == GAIA_XY_Z
				|| geom->DimensionModel == GAIA_XY_Z_M)
				z = pt->Z;
			    else
				z = 0.0;
			    gaiaFreeGeomColl (geom);
			    if (!do_insert_temp_aux_node
				(db_handle, stmt_ins, x, y, z, &msg))
			      {
				  gaia_create_routing_set_error (cache, msg);
				  sqlite3_free (msg);
				  goto error;
			      }
			}
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

/* updating the Spatial Table */
    sqlite3_reset (stmt_q1);
    while (1)
      {
	  ret = sqlite3_step (stmt_q1);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob;
		int size;
		gaiaGeomCollPtr geom;
		gaiaPointPtr pt;
		double from_x = DBL_MIN;
		double from_y = DBL_MIN;
		double from_z = DBL_MIN;
		double to_x = DBL_MIN;
		double to_y = DBL_MIN;
		double to_z = DBL_MIN;
		char *msg;
		int rowid = sqlite3_column_int (stmt_q1, 0);
		if (sqlite3_column_type (stmt_q1, 1) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_q1,
								       1);
		      size = sqlite3_column_bytes (stmt_q1, 1);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom)
			{
			    pt = geom->FirstPoint;
			    from_x = pt->X;
			    from_y = pt->Y;
			    if (geom->DimensionModel == GAIA_XY_Z
				|| geom->DimensionModel == GAIA_XY_Z_M)
				from_z = pt->Z;
			    else
				from_z = 0.0;
			    gaiaFreeGeomColl (geom);
			}
		  }
		if (sqlite3_column_type (stmt_q1, 2) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_q1,
								       2);
		      size = sqlite3_column_bytes (stmt_q1, 2);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
		      if (geom)
			{
			    pt = geom->FirstPoint;
			    to_x = pt->X;
			    to_y = pt->Y;
			    if (geom->DimensionModel == GAIA_XY_Z
				|| geom->DimensionModel == GAIA_XY_Z_M)
				to_z = pt->Z;
			    else
				to_z = 0.0;
			    gaiaFreeGeomColl (geom);
			}
		  }
		if (!do_update_link
		    (db_handle, stmt_q2, stmt_upd, rowid, from_x, from_y,
		     from_z, to_x, to_y, to_z, &msg))
		  {
		      gaia_create_routing_set_error (cache, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("SQL error: %s",
					     sqlite3_errmsg (db_handle));
		gaia_create_routing_set_error (cache, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_q1);
    sqlite3_finalize (stmt_q2);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_upd);

/* dropping the Temporary Aux Table */
    sql = "DROP TABLE temp_create_routing_aux_nodes";
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);

/* COMMIT */
    sql = "RELEASE SAVEPOINT create_routing_nodes";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    return 1;

  error:
    if (stmt_q1 != NULL)
	sqlite3_finalize (stmt_q1);
    if (stmt_q2 != NULL)
	sqlite3_finalize (stmt_q2);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
/* ROLLBACK */
    sql = "ROLLBACK TO create_routing_nodes";
    sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    return 0;
}

SPATIALITE_DECLARE int
gaia_create_routing_nodes (sqlite3 * db_handle,
			   const void *cache,
			   const char *db_prefix,
			   const char *table,
			   const char *geom_column,
			   const char *from_column, const char *to_column)
{
/* attempting to create a Routing Nodes columns for a spatial table */
    const char *sql;
    int ret;
    char *geom_name = NULL;

    if (db_handle == NULL || cache == NULL)
	return 0;

    if (table == NULL)
      {
	  gaia_create_routing_set_error (cache, "Spatial Table Name is NULL");
	  return 0;
      }
    if (from_column == NULL)
      {
	  gaia_create_routing_set_error (cache, "FromNode Column Name is NULL");
	  return 0;
      }
    if (to_column == NULL)
      {
	  gaia_create_routing_set_error (cache, "ToNode Column Name is NULL");
	  return 0;
      }

/* testing the spatial table */
    if (!do_check_spatial_table
	(db_handle, cache, db_prefix, table, geom_column, from_column,
	 to_column, &geom_name))
	goto error;

/* setting a global Savepoint */
    sql = "SAVEPOINT create_routing_nodes";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* creating and populating the Routing Nodes columns */
    if (!do_create_routing_nodes
	(db_handle, cache, db_prefix, table, geom_name, from_column, to_column))
	goto error;

    free (geom_name);
    return 1;

  error:
    if (geom_name != NULL)
	free (geom_name);
    return 0;
}

SPATIALITE_DECLARE int
gaia_create_routing (sqlite3 * db_handle,
		     const void *cache,
		     const char *routing_data_table,
		     const char
		     *virtual_routing_table,
		     const char *input_table,
		     const char *from_column,
		     const char *to_column,
		     const char *geom_column,
		     const char *cost_column,
		     const char *name_column,
		     int a_star_enabled,
		     int bidirectional,
		     const char *oneway_from,
		     const char *oneway_to, int overwrite)
{
/* attempting to create a VirtualRouting from an input table */
    int has_ids;
    int n_nodes = 0;
    int max_code_length = 0;
    double a_star_coeff = DBL_MAX;
    const char *sql;
    int ret;

    if (db_handle == NULL || cache == NULL)
	return 0;

    gaia_create_routing_set_error (cache, NULL);
    if (routing_data_table == NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "Routing Data Table Name is NULL");
	  return 0;
      }
    if (virtual_routing_table == NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "VirtualRouting Table Name is NULL");
	  return 0;
      }
    if (input_table == NULL)
      {
	  gaia_create_routing_set_error (cache, "Input Table Name is NULL");
	  return 0;
      }
    if (from_column == NULL)
      {
	  gaia_create_routing_set_error (cache, "FromNode Column Name is NULL");
	  return 0;
      }
    if (to_column == NULL)
      {
	  gaia_create_routing_set_error (cache, "ToNode Column Name is NULL");
	  return 0;
      }
    if (geom_column == NULL && cost_column == NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "Both Geometry Column and Cost Column Names are NULL at the same time");
	  return 0;
      }
    if (oneway_from == NULL && oneway_to != NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "OnewayFromTo is NULL but OnewayToFrom is NOT NULL");
	  return 0;
      }
    if (oneway_from != NULL && oneway_to == NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "OnewayFromTo is NOT NULL but OnewayToFrom is NULL");
	  return 0;
      }
    if (oneway_from != NULL && oneway_to != NULL && !bidirectional)
      {
	  gaia_create_routing_set_error (cache,
					 "Both OnewayFromTo and OnewayToFrom are NOT NULL but Unidirectional has been specified");
	  return 0;
      }
    if (a_star_enabled && geom_column == NULL)
      {
	  gaia_create_routing_set_error (cache,
					 "Geometry Columns is NULL but A* is enabled");
	  return 0;
      }

/* setting a global Savepoint */
    sql = "SAVEPOINT create_routing_zero";
    ret = sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("SQL error: %s", sqlite3_errmsg (db_handle));
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* dropping the TempTables (just in case) */
    do_drop_temp_tables (db_handle);

    if (overwrite)
      {
	  /* attempting to drop existing tables (just in case) */
	  do_drop_tables (db_handle, routing_data_table, virtual_routing_table);
      }

/* testing for existing tables */
    if (do_check_data_table (db_handle, routing_data_table))
      {
	  char *msg =
	      sqlite3_mprintf ("Routing Data Table \"%s\" already exists",
			       routing_data_table);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    if (do_check_virtual_table (db_handle, virtual_routing_table))
      {
	  char *msg =
	      sqlite3_mprintf ("VirtualRouting Table \"%s\" already exists",
			       virtual_routing_table);
	  gaia_create_routing_set_error (cache, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* testing the input table */
    if (!do_check_input_table
	(db_handle, cache, input_table, from_column, to_column, geom_column,
	 cost_column, name_column, oneway_from, oneway_to, a_star_enabled,
	 bidirectional, &has_ids, &n_nodes, &max_code_length, &a_star_coeff))
	return 0;

/* creating and populating the Routing Data table */
    if (!do_create_data
	(db_handle, cache, routing_data_table, input_table, from_column,
	 to_column, geom_column, name_column, a_star_enabled, a_star_coeff,
	 has_ids, n_nodes, max_code_length))
	return 0;

/* creating the VirtualRouting table */
    if (!do_create_virtual_routing
	(db_handle, cache, routing_data_table, virtual_routing_table))
	return 0;

    return 1;
}
