/*

 virtualknn.c -- SQLite3 extension [VIRTUAL TABLE RTree metahandler]

 version 4.4, 2015 November 27

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
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

/*
 
CREDITS:

this module has been completely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(K-Nearest Neighbors [KNN] module) 

CIG: 644544015A

*/

/*

IMPORTANT NOTE: how KNN works

the KNN module is implemented on the top of an SQLite's R*Tree, and
more specifically is built around the sqlite3_rtree_query_callback()
function.

this API allows to directly explore the R*Tree hierarchy; for each
node found within the Tree the registered callback will be invoked
so to determine if the current Tree node (aka BBOX) should be further
expanded or should be ignored.
while the process goes on all Tree levels will be traversed until it
reaches Level=0 (Leaves aka Terminal Nodes) where the indexed
Geometries will be finally referenced by their individual ROWID and BBOX

in a most usual Spatial Index query the R*Tree will be simply
traversed starting from Root Nodes (higher level) and progressively
descending level by level toward Leaf Nodes (indexed Geometries); in
this specific case the BBOX to be searched is clearly defined, so the
callback function should simply check if each Node BBOX do effectively
intersect the reference BBOX and that's all.

the access strategy required by a KNN query is much more complex than
this, because there is no reference BBOX at all (the search radius is
unknown in this case); so it should be dynamically built by repeatedly
querying the R*Tree until a satisfying solution is found.

step #1
-------
we'll descend the Tree by exploring a Level at each time so to
identify the lower-level BBOX containing the reference Geometry.
if no BBOX directly contains the reference Geometry then the
nearest one will be identified.

step #2
-------
the reference Geometry will now be buffered so to completely
cover the BBOX identified in the previous step.

step #3
-------
an ordinary Spatial Index query will be now performed, using
the buffered Geometry as the search frame, so to identify
all nearest features.

step #4
-------
if the total number of nearest features identified by the
previous query is less than requested, then the buffer
radius will be repeatedly expanded until all requested
nearest features will be found.

*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifndef OMIT_GEOS		/* GEOS is supported */
#ifndef OMIT_KNN		/* only if KNN is enabled */

#include <spatialite/sqlite.h>

#include <spatialite/spatialite.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaiageo.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp    _stricmp
#define strncasecmp    _strnicmp
#endif

static struct sqlite3_module my_knn_module;

/******************************************************************************
/
/ VirtualTable structs
/
******************************************************************************/

typedef struct VKnnItemStruct
{
/* a Feature item into the KNN sorted array */
    sqlite3_int64 rowid;
    double dist;
} VKnnItem;
typedef VKnnItem *VKnnItemPtr;

typedef struct VKnnContextStruct
{
/* current KNN context */
    char *table_name;
    char *column_name;
    unsigned char *blob;
    int blob_size;
    sqlite3_stmt *stmt_dist;
    sqlite3_stmt *stmt_map_dist;
    sqlite3_stmt *stmt_rect_dist;
    sqlite3_stmt *stmt_pt_dist;
    sqlite3_stmt *stmt_buffer;
    sqlite3_stmt *stmt_rtree;
    sqlite3_stmt *stmt_rtree_count;
    double bbox_minx;
    double bbox_miny;
    double bbox_maxx;
    double bbox_maxy;
    double minx;
    double miny;
    double maxx;
    double maxy;
    double min_dist;
    double rtree_minx;
    double rtree_miny;
    double rtree_maxx;
    double rtree_maxy;
    int level;
    int current_level;
    VKnnItemPtr knn_array;
    int max_items;
    double max_dist;
    int curr_items;
    int rtree_count;
} VKnnContext;
typedef VKnnContext *VKnnContextPtr;

typedef struct VirtualKnnStruct
{
/* extends the sqlite3_vtab struct */
    const sqlite3_module *pModule;	/* ptr to sqlite module: USED INTERNALLY BY SQLITE */
    int nRef;			/* # references: USED INTERNALLY BY SQLITE */
    char *zErrMsg;		/* error message: USE INTERNALLY BY SQLITE */
    sqlite3 *db;		/* the sqlite db holding the virtual table */
    VKnnContextPtr knn_ctx;	/* KNN context */
} VirtualKnn;
typedef VirtualKnn *VirtualKnnPtr;

typedef struct VirtualKnnCursorStruct
{
/* extends the sqlite3_vtab_cursor struct */
    VirtualKnnPtr pVtab;	/* Virtual table of this cursor */
    int eof;			/* the EOF marker */
    int CurrentIndex;		/* index of the current KNN item */
} VirtualKnnCursor;
typedef VirtualKnnCursor *VirtualKnnCursorPtr;

static void
vknn_empty_context (VKnnContextPtr ctx)
{
/* setting an empty KNN context */
    if (ctx == NULL)
	return;
    ctx->table_name = NULL;
    ctx->column_name = NULL;
    ctx->blob = NULL;
    ctx->blob_size = 0;
    ctx->stmt_dist = NULL;
    ctx->stmt_map_dist = NULL;
    ctx->stmt_rect_dist = NULL;
    ctx->stmt_pt_dist = NULL;
    ctx->stmt_buffer = NULL;
    ctx->stmt_rtree = NULL;
    ctx->stmt_rtree_count = NULL;
    ctx->bbox_minx = -DBL_MAX;
    ctx->bbox_miny = -DBL_MAX;
    ctx->bbox_maxx = DBL_MAX;
    ctx->bbox_maxy = DBL_MAX;
    ctx->minx = DBL_MAX;
    ctx->miny = DBL_MAX;
    ctx->maxx = -DBL_MAX;
    ctx->maxy = -DBL_MAX;
    ctx->min_dist = DBL_MAX;
    ctx->rtree_minx = -DBL_MAX;
    ctx->rtree_miny = -DBL_MAX;
    ctx->rtree_maxx = DBL_MAX;
    ctx->rtree_maxy = DBL_MAX;
    ctx->current_level = 0;
    ctx->max_items = 0;
    ctx->knn_array = NULL;
    ctx->curr_items = 0;
    ctx->rtree_count = 0;
    ctx->max_dist = -DBL_MAX;
}

static VKnnContextPtr
vknn_create_context (void)
{
/* creating an empty KNN context */
    VKnnContextPtr ctx = malloc (sizeof (VKnnContext));
    vknn_empty_context (ctx);
    return ctx;
}

static void
vknn_reset_context (VKnnContextPtr ctx)
{
/* freeing a KNN context */
    if (ctx == NULL)
	return;
    if (ctx->table_name != NULL)
	free (ctx->table_name);
    if (ctx->column_name != NULL)
	free (ctx->column_name);
    if (ctx->blob != NULL)
	free (ctx->blob);
    if (ctx->stmt_dist != NULL)
	sqlite3_finalize (ctx->stmt_dist);
    if (ctx->stmt_map_dist != NULL)
	sqlite3_finalize (ctx->stmt_map_dist);
    if (ctx->stmt_rect_dist != NULL)
	sqlite3_finalize (ctx->stmt_rect_dist);
    if (ctx->stmt_pt_dist != NULL)
	sqlite3_finalize (ctx->stmt_pt_dist);
    if (ctx->stmt_buffer != NULL)
	sqlite3_finalize (ctx->stmt_buffer);
    if (ctx->stmt_rtree != NULL)
	sqlite3_finalize (ctx->stmt_rtree);
    if (ctx->stmt_rtree_count != NULL)
	sqlite3_finalize (ctx->stmt_rtree_count);
    if (ctx->knn_array != NULL)
	free (ctx->knn_array);
    vknn_empty_context (ctx);
}

static void
vknn_rtree_count (VKnnContextPtr ctx)
{
/* approximatively counting how many entries are into the R*Tree */
    int ret;
    sqlite3_stmt *stmt;


    ctx->rtree_count = 0;
    if (ctx == NULL)
	return;
    stmt = ctx->stmt_rtree_count;
    if (stmt == NULL)
	return;
    sqlite3_reset (stmt);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		ctx->rtree_count += 1;
	    }
	  else
	    {
		ctx->rtree_count = 0;
		break;
	    }
      }
}

static void
vknn_init_context (VKnnContextPtr ctx, const char *table, const char *column,
		   gaiaGeomCollPtr geom, int max_items,
		   sqlite3_stmt * stmt_dist, sqlite3_stmt * stmt_map_dist,
		   sqlite3_stmt * stmt_rect_dist, sqlite3_stmt * stmt_pt_dist,
		   sqlite3_stmt * stmt_buffer, sqlite3_stmt * stmt_rtree,
		   sqlite3_stmt * stmt_rtree_count)
{
/* initializing a KNN context */
    int i;
    if (ctx == NULL)
	return;
    vknn_reset_context (ctx);
    i = strlen (table);
    ctx->table_name = malloc (i + 1);
    strcpy (ctx->table_name, table);
    i = strlen (column);
    ctx->column_name = malloc (i + 1);
    strcpy (ctx->column_name, column);
    gaiaToSpatiaLiteBlobWkb (geom, &(ctx->blob), &(ctx->blob_size));
    ctx->stmt_dist = stmt_dist;
    ctx->stmt_map_dist = stmt_map_dist;
    ctx->stmt_rect_dist = stmt_rect_dist;
    ctx->stmt_pt_dist = stmt_pt_dist;
    ctx->stmt_buffer = stmt_buffer;
    ctx->stmt_rtree = stmt_rtree;
    ctx->stmt_rtree_count = stmt_rtree_count;
    ctx->max_items = max_items;
    ctx->knn_array = malloc (sizeof (VKnnItem) * max_items);
    for (i = 0; i < max_items; i++)
      {
	  /* initializing the KNN sorted array */
	  VKnnItemPtr item = ctx->knn_array + i;
	  item->rowid = 0;
	  item->dist = DBL_MAX;
      }
    ctx->curr_items = 0;
    vknn_rtree_count (ctx);
}

static void
vknn_free_context (void *p)
{
/* freeing a KNN context */
    VKnnContextPtr ctx = (VKnnContextPtr) p;
    vknn_reset_context (ctx);
    free (ctx);
}

static int
vknn_check_view_rtree (sqlite3 * sqlite, const char *table_name,
		       const char *geom_column, char **real_table,
		       char **real_geom, int *is_geographic)
{
/* checks if the required RTree is actually defined - SpatialView */
    sqlite3_stmt *stmt;
    char *sql_statement;
    int ret;
    int count = 0;
    char *rt = NULL;
    char *rg = NULL;
    int is_longlat = 0;

/* testing if views_geometry_columns exists */
    sql_statement = sqlite3_mprintf ("SELECT tbl_name FROM sqlite_master "
				     "WHERE type = 'table' AND tbl_name = 'views_geometry_columns'");
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      count++;
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    count = 0;

/* attempting to find the RTree Geometry Column */
    sql_statement =
	sqlite3_mprintf
	("SELECT a.f_table_name, a.f_geometry_column, SridIsGeographic(b.srid) "
	 "FROM views_geometry_columns AS a " "JOIN geometry_columns AS b ON ("
	 "Upper(a.f_table_name) = Upper(b.f_table_name) AND "
	 "Upper(a.f_geometry_column) = Upper(b.f_geometry_column)) "
	 "WHERE Upper(a.view_name) = Upper(%Q) "
	 "AND Upper(a.view_geometry) = Upper(%Q) AND b.spatial_index_enabled = 1",
	 table_name, geom_column);
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *v = (const char *) sqlite3_column_text (stmt, 0);
		int len = sqlite3_column_bytes (stmt, 0);
		if (rt)
		    free (rt);
		rt = malloc (len + 1);
		strcpy (rt, v);
		v = (const char *) sqlite3_column_text (stmt, 1);
		len = sqlite3_column_bytes (stmt, 1);
		if (rg)
		    free (rg);
		rg = malloc (len + 1);
		strcpy (rg, v);
		is_longlat = sqlite3_column_int (stmt, 2);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    if (!validateRowid (sqlite, rt))
      {
	  free (rt);
	  free (rg);
	  return 0;
      }
    *real_table = rt;
    *real_geom = rg;
    *is_geographic = is_longlat;
    return 1;
}

static int
vknn_check_rtree (sqlite3 * sqlite, const char *db_prefix,
		  const char *table_name, const char *geom_column,
		  char **real_table, char **real_geom, int *is_geographic)
{
/* checks if the required RTree is actually defined */
    sqlite3_stmt *stmt;
    char *sql_statement;
    int ret;
    int count = 0;
    char *rt = NULL;
    char *rg = NULL;
    int is_longlat = 0;

    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT f_table_name, f_geometry_column, SridIsGeographic(srid) "
	       "FROM main.geometry_columns WHERE Upper(f_table_name) = Upper(%Q) AND "
	       "Upper(f_geometry_column) = Upper(%Q) AND spatial_index_enabled = 1",
	       table_name, geom_column);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT f_table_name, f_geometry_column, SridIsGeographic(srid) "
	       "FROM \"%s\".geometry_columns WHERE Upper(f_table_name) = Upper(%Q) AND "
	       "Upper(f_geometry_column) = Upper(%Q) AND spatial_index_enabled = 1",
	       quoted_db, table_name, geom_column);
	  free (quoted_db);
      }
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *v = (const char *) sqlite3_column_text (stmt, 0);
		int len = sqlite3_column_bytes (stmt, 0);
		if (rt)
		    free (rt);
		rt = malloc (len + 1);
		strcpy (rt, v);
		v = (const char *) sqlite3_column_text (stmt, 1);
		len = sqlite3_column_bytes (stmt, 1);
		if (rg)
		    free (rg);
		rg = malloc (len + 1);
		strcpy (rg, v);
		is_longlat = sqlite3_column_int (stmt, 2);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return vknn_check_view_rtree (sqlite, table_name, geom_column,
				      real_table, real_geom, is_geographic);
    else
      {
	  *real_table = rt;
	  *real_geom = rg;
	  *is_geographic = is_longlat;
      }
    return 1;
}

static int
vknn_find_view_rtree (sqlite3 * sqlite, const char *db_prefix,
		      const char *table_name, char **real_table,
		      char **real_geom, int *is_geographic)
{
/* attempts to find the corresponding RTree Geometry Column - SpatialView */
    sqlite3_stmt *stmt;
    char *sql_statement;
    int ret;
    int count = 0;
    char *rt = NULL;
    char *rg = NULL;
    int is_longlat = 0;

/* testing if views_geometry_columns exists */
    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf ("SELECT tbl_name FROM main.sqlite_master "
			       "WHERE type = 'table' AND tbl_name = 'views_geometry_columns'");
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf ("SELECT tbl_name FROM \"%s\".sqlite_master "
			       "WHERE type = 'table' AND tbl_name = 'views_geometry_columns'",
			       quoted_db);
	  free (quoted_db);
      }
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      count++;
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    count = 0;

/* attempting to find the RTree Geometry Column */
    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT a.f_table_name, a.f_geometry_column, SridIsGeographic(b.srid) "
	       "FROM main.views_geometry_columns AS a "
	       "JOIN main.geometry_columns AS b ON ("
	       "Upper(a.f_table_name) = Upper(b.f_table_name) AND "
	       "Upper(a.f_geometry_column) = Upper(b.f_geometry_column)) "
	       "WHERE Upper(a.view_name) = Upper(%Q) AND b.spatial_index_enabled = 1",
	       table_name);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT a.f_table_name, a.f_geometry_column, SridIsGeographic(b.srid) "
	       "FROM \"%s\".views_geometry_columns AS a "
	       "JOIN \"%s\".geometry_columns AS b ON ("
	       "Upper(a.f_table_name) = Upper(b.f_table_name) AND "
	       "Upper(a.f_geometry_column) = Upper(b.f_geometry_column)) "
	       "WHERE Upper(a.view_name) = Upper(%Q) AND b.spatial_index_enabled = 1",
	       quoted_db, quoted_db, table_name);
	  free (quoted_db);
      }
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *v = (const char *) sqlite3_column_text (stmt, 0);
		int len = sqlite3_column_bytes (stmt, 0);
		if (rt)
		    free (rt);
		rt = malloc (len + 1);
		strcpy (rt, v);
		v = (const char *) sqlite3_column_text (stmt, 1);
		len = sqlite3_column_bytes (stmt, 1);
		if (rg)
		    free (rg);
		rg = malloc (len + 1);
		strcpy (rg, v);
		is_longlat = sqlite3_column_int (stmt, 2);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    *real_table = rt;
    *real_geom = rg;
    *is_geographic = is_longlat;
    return 1;
}

static int
vknn_find_rtree (sqlite3 * sqlite, const char *db_prefix,
		 const char *table_name, char **real_table, char **real_geom,
		 int *is_geographic)
{
/* attempts to find the corresponding RTree Geometry Column */
    sqlite3_stmt *stmt;
    char *sql_statement;
    int ret;
    int count = 0;
    char *rt = NULL;
    char *rg = NULL;
    int is_longlat = 0;

    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT f_table_name, f_geometry_column, SridIsGeographic(srid) "
	       " FROM main.geometry_columns WHERE Upper(f_table_name) = Upper(%Q) "
	       "AND spatial_index_enabled = 1", table_name);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT f_table_name, f_geometry_column, SridIsGeographic(srid) "
	       " FROM \"%s\".geometry_columns WHERE Upper(f_table_name) = Upper(%Q) "
	       "AND spatial_index_enabled = 1", quoted_db, table_name);
	  free (quoted_db);
      }
    ret =
	sqlite3_prepare_v2 (sqlite, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	return 0;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *v = (const char *) sqlite3_column_text (stmt, 0);
		int len = sqlite3_column_bytes (stmt, 0);
		if (rt)
		    free (rt);
		rt = malloc (len + 1);
		strcpy (rt, v);
		v = (const char *) sqlite3_column_text (stmt, 1);
		len = sqlite3_column_bytes (stmt, 1);
		if (rg)
		    free (rg);
		rg = malloc (len + 1);
		strcpy (rg, v);
		is_longlat = sqlite3_column_int (stmt, 2);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return vknn_find_view_rtree (sqlite, db_prefix, table_name,
				     real_table, real_geom, is_geographic);
    else
      {
	  *real_table = rt;
	  *real_geom = rg;
	  *is_geographic = is_longlat;
      }
    return 1;
}

static void
vknn_parse_table_name (const char *tn, char **db_prefix, char **table_name)
{
/* attempting to extract an eventual DB prefix */
    int i;
    int len = strlen (tn);
    int i_dot = -1;
    if (strncasecmp (tn, "DB=", 3) == 0)
      {
	  int l_db;
	  int l_tbl;
	  for (i = 3; i < len; i++)
	    {
		if (tn[i] == '.')
		  {
		      i_dot = i;
		      break;
		  }
	    }
	  if (i_dot > 1)
	    {
		l_db = i_dot - 3;
		l_tbl = len - (i_dot + 1);
		*db_prefix = malloc (l_db + 1);
		memset (*db_prefix, '\0', l_db + 1);
		memcpy (*db_prefix, tn + 3, l_db);
		*table_name = malloc (l_tbl + 1);
		strcpy (*table_name, tn + i_dot + 1);
		return;
	    }
      }
    *table_name = malloc (len + 1);
    strcpy (*table_name, tn);
}

static int
vknn_create (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	     sqlite3_vtab ** ppVTab, char **pzErr)
{
/* creates the virtual table for R*Tree KNN metahandling */
    VirtualKnnPtr p_vt;
    char *buf;
    char *vtable;
    char *xname;
    if (pAux)
	pAux = pAux;		/* unused arg warning suppression */
    if (argc == 3)
      {
	  vtable = gaiaDequotedSql ((char *) argv[2]);
      }
    else
      {
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualKNN module] CREATE VIRTUAL: illegal arg list {void}\n");
	  return SQLITE_ERROR;
      }
    p_vt = (VirtualKnnPtr) sqlite3_malloc (sizeof (VirtualKnn));
    if (!p_vt)
	return SQLITE_NOMEM;
    p_vt->db = db;
    p_vt->pModule = &my_knn_module;
    p_vt->nRef = 0;
    p_vt->zErrMsg = NULL;
    p_vt->knn_ctx = vknn_create_context ();
/* preparing the COLUMNs for this VIRTUAL TABLE */
    xname = gaiaDoubleQuotedSql (vtable);
    buf = sqlite3_mprintf ("CREATE TABLE \"%s\" (f_table_name TEXT, "
			   "f_geometry_column TEXT, ref_geometry BLOB, max_items INTEGER, "
			   "pos INTEGER, fid INTEGER, distance DOUBLE)", xname);
    free (xname);
    free (vtable);
    if (sqlite3_declare_vtab (db, buf) != SQLITE_OK)
      {
	  sqlite3_free (buf);
	  *pzErr =
	      sqlite3_mprintf
	      ("[VirtualKNN module] CREATE VIRTUAL: invalid SQL statement \"%s\"",
	       buf);
	  return SQLITE_ERROR;
      }
    sqlite3_free (buf);
    *ppVTab = (sqlite3_vtab *) p_vt;
    return SQLITE_OK;
}

static int
vknn_connect (sqlite3 * db, void *pAux, int argc, const char *const *argv,
	      sqlite3_vtab ** ppVTab, char **pzErr)
{
/* connects the virtual table - simply aliases vknn_create() */
    return vknn_create (db, pAux, argc, argv, ppVTab, pzErr);
}

static int
vknn_best_index (sqlite3_vtab * pVTab, sqlite3_index_info * pIdxInfo)
{
/* best index selection */
    int i;
    int err = 1;
    int table = 0;
    int geom_col = 0;
    int ref_geom = 0;
    int max_items = 0;
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    for (i = 0; i < pIdxInfo->nConstraint; i++)
      {
	  /* verifying the constraints */
	  struct sqlite3_index_constraint *p = &(pIdxInfo->aConstraint[i]);
	  if (p->usable)
	    {
		if (p->iColumn == 0 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		    table++;
		else if (p->iColumn == 1 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		    geom_col++;
		else if (p->iColumn == 2 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		    ref_geom++;
		else if (p->iColumn == 3 && p->op == SQLITE_INDEX_CONSTRAINT_EQ)
		    max_items++;
	    }
      }
    if (table == 1 && (geom_col == 0 || geom_col == 1) && ref_geom == 1
	&& (max_items == 0 || max_items == 1))
      {
	  /* this one is a valid KNN query */
	  if (geom_col == 1)
	    {
		if (max_items == 1)
		    pIdxInfo->idxNum = 3;
		else
		    pIdxInfo->idxNum = 1;
	    }
	  else
	    {
		if (max_items == 1)
		    pIdxInfo->idxNum = 4;
		else
		    pIdxInfo->idxNum = 2;
	    }
	  pIdxInfo->estimatedCost = 1.0;
	  for (i = 0; i < pIdxInfo->nConstraint; i++)
	    {
		if (pIdxInfo->aConstraint[i].usable)
		  {
		      pIdxInfo->aConstraintUsage[i].argvIndex = i + 1;
		      pIdxInfo->aConstraintUsage[i].omit = 1;
		  }
	    }
	  err = 0;
      }
    if (err)
      {
	  /* illegal query */
	  pIdxInfo->idxNum = 0;
      }
    return SQLITE_OK;
}

static int
vknn_disconnect (sqlite3_vtab * pVTab)
{
/* disconnects the virtual table */
    VirtualKnnPtr p_vt = (VirtualKnnPtr) pVTab;
    if (p_vt->knn_ctx != NULL)
	vknn_free_context (p_vt->knn_ctx);
    sqlite3_free (p_vt);
    return SQLITE_OK;
}

static int
vknn_destroy (sqlite3_vtab * pVTab)
{
/* destroys the virtual table - simply aliases vknn_disconnect() */
    return vknn_disconnect (pVTab);
}

static int
vknn_open (sqlite3_vtab * pVTab, sqlite3_vtab_cursor ** ppCursor)
{
/* opening a new cursor */
    VirtualKnnCursorPtr cursor =
	(VirtualKnnCursorPtr) sqlite3_malloc (sizeof (VirtualKnnCursor));
    if (cursor == NULL)
	return SQLITE_ERROR;
    cursor->pVtab = (VirtualKnnPtr) pVTab;
    cursor->eof = 1;
    *ppCursor = (sqlite3_vtab_cursor *) cursor;
    return SQLITE_OK;
}

static int
vknn_close (sqlite3_vtab_cursor * pCursor)
{
/* closing the cursor */
    sqlite3_free (pCursor);
    return SQLITE_OK;
}

static void
vknn_shift_items (VKnnContextPtr ctx, int index)
{
/* shifting down the Features sorted array */
    int i;
    for (i = ctx->max_items - 1; i > index; i--)
      {
	  VKnnItemPtr item1 = ctx->knn_array + i - 1;
	  VKnnItemPtr item2 = ctx->knn_array + i;
	  item2->rowid = item1->rowid;
	  item2->dist = item1->dist;
	  if ((i == ctx->max_items - 1) && item2->dist != DBL_MAX)
	      ctx->max_dist = item2->dist;
      }
}

static void
vknn_update_items (VKnnContextPtr ctx, sqlite3_int64 rowid, double dist)
{
/* updating the Features sorted array */
    int i;
    if (ctx->curr_items == ctx->max_items)
      {
	  if (dist >= ctx->max_dist)
	      return;
      }
    for (i = 0; i < ctx->max_items; i++)
      {
	  VKnnItemPtr item = ctx->knn_array + i;
	  if (rowid == item->rowid)
	      return;
	  if (dist < item->dist)
	    {
		vknn_shift_items (ctx, i);
		item->rowid = rowid;
		item->dist = dist;
		break;
	    }
      }
    if (dist > ctx->max_dist)
	ctx->max_dist = dist;
    if (ctx->curr_items < ctx->max_items)
	ctx->curr_items += 1;
}

static int
vknn_check_mbr (VKnnContextPtr ctx, double rtree_minx, double rtree_miny,
		double rtree_maxx, double rtree_maxy)
{
/* comparing two MBRs */
    if (rtree_minx >= ctx->bbox_minx && rtree_maxx <= ctx->bbox_maxx
	&& rtree_miny >= ctx->bbox_miny && rtree_maxy <= ctx->bbox_maxy)
	return FULLY_WITHIN;
    if (rtree_maxx < ctx->bbox_minx)
	return NOT_WITHIN;
    if (rtree_minx > ctx->bbox_maxx)
	return NOT_WITHIN;
    if (rtree_maxy < ctx->bbox_miny)
	return NOT_WITHIN;
    if (rtree_miny > ctx->bbox_maxy)
	return NOT_WITHIN;
    return PARTLY_WITHIN;
}

static double
vknn_compute_distance (VKnnContextPtr ctx, sqlite3_int64 rowid)
{
/* computing the distance between two geometries (in meters) */
    double dist = DBL_MAX;
    int ret;
    sqlite3_stmt *stmt;
    if (ctx == NULL)
	return DBL_MAX;
    if (ctx->blob == NULL)
	return DBL_MAX;
    if (ctx->stmt_dist == NULL)
	return DBL_MAX;
    stmt = ctx->stmt_dist;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, ctx->blob, ctx->blob_size, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, rowid);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		    dist = sqlite3_column_double (stmt, 0);
	    }
	  else
	    {
		dist = DBL_MAX;
		break;
	    }
      }
    return dist;
}

static double
vknn_rect_distance (VKnnContextPtr ctx, double minx, double miny, double maxx,
		    double maxy)
{
/* computing the distance between the geometry and an R*Tree BBOX */
    double dist = DBL_MAX;
    int ret;
    sqlite3_stmt *stmt;
    if (ctx == NULL)
	return DBL_MAX;
    if (ctx->blob == NULL)
	return DBL_MAX;
    if (ctx->stmt_rect_dist == NULL)
	return DBL_MAX;
    stmt = ctx->stmt_rect_dist;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, ctx->blob, ctx->blob_size, SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, minx);
    sqlite3_bind_double (stmt, 3, miny);
    sqlite3_bind_double (stmt, 4, maxx);
    sqlite3_bind_double (stmt, 5, maxy);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		    dist = sqlite3_column_double (stmt, 0);
	    }
	  else
	    {
		dist = DBL_MAX;
		break;
	    }
      }
    return dist;
}

static double
vknn_pt_distance (VKnnContextPtr ctx, double x, double y)
{
/* computing the distance between the geometry and a point */
    double dist = DBL_MAX;
    int ret;
    sqlite3_stmt *stmt;
    if (ctx == NULL)
	return DBL_MAX;
    if (ctx->blob == NULL)
	return DBL_MAX;
    if (ctx->stmt_pt_dist == NULL)
	return DBL_MAX;
    stmt = ctx->stmt_pt_dist;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, ctx->blob, ctx->blob_size, SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, x);
    sqlite3_bind_double (stmt, 3, y);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		    dist = sqlite3_column_double (stmt, 0);
	    }
	  else
	    {
		dist = DBL_MAX;
		break;
	    }
      }
    return dist;
}

static int
vknn_bufferize (VKnnContextPtr ctx, double radius)
{
/* computing the frame for an R*Tree query */
    int ret;
    sqlite3_stmt *stmt;
    if (ctx == NULL)
	return 0;
    if (ctx->blob == NULL)
	return 0;
    if (ctx->stmt_buffer == NULL)
	return 0;
    stmt = ctx->stmt_buffer;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, ctx->blob, ctx->blob_size, SQLITE_STATIC);
    sqlite3_bind_double (stmt, 2, radius);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		    ctx->rtree_minx = sqlite3_column_double (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_FLOAT)
		    ctx->rtree_miny = sqlite3_column_double (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		    ctx->rtree_maxx = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_FLOAT)
		    ctx->rtree_maxy = sqlite3_column_double (stmt, 3);
	    }
	  else
	    {
		return 0;
		break;
	    }
      }
    return 1;
}

static int
vknn_rtree_query (VKnnContextPtr ctx)
{
/* Querying the RTree - Intersections */
    int err = 0;
    int count = 0;
    int ret;
    sqlite3_stmt *stmt;
    if (ctx == NULL)
	return 0;
    if (ctx->stmt_rtree == NULL)
	return 0;
    stmt = ctx->stmt_rtree;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, ctx->rtree_maxx);
    sqlite3_bind_double (stmt, 2, ctx->rtree_minx);
    sqlite3_bind_double (stmt, 3, ctx->rtree_maxy);
    sqlite3_bind_double (stmt, 4, ctx->rtree_miny);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		double dist;
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt, 0);
		dist = vknn_compute_distance (ctx, rowid);
		vknn_update_items (ctx, rowid, dist);
		count++;
	    }
	  else
	    {
		err = 1;
		break;
	    }
      }
    if (err)
	return -1;
    return count;
}

static int
vknn_query_callback (sqlite3_rtree_query_info * info)
{
/* R*Tree Query Callback function */
    double rtree_minx;
    double rtree_maxx;
    double rtree_miny;
    double rtree_maxy;
    double dist;
    int mode;
    VKnnContextPtr ctx = (VKnnContextPtr) (info->pContext);
    if (info->nCoord != 4)
      {
	  /* invalid RTree */
	  info->eWithin = NOT_WITHIN;
	  return SQLITE_OK;
      }

/* fetching the node's BBOX */
    rtree_minx = info->aCoord[0];
    rtree_maxx = info->aCoord[1];
    rtree_miny = info->aCoord[2];
    rtree_maxy = info->aCoord[3];
    if (info->iLevel > ctx->current_level)
      {
	  mode =
	      vknn_check_mbr (ctx, rtree_minx, rtree_miny, rtree_maxx,
			      rtree_maxy);
	  if (mode == FULLY_WITHIN || mode == PARTLY_WITHIN)
	    {
		/* overlaps the current reference frame; to be further expanded */
		info->eWithin = FULLY_WITHIN;
	    }
	  else
	      info->eWithin = NOT_WITHIN;
      }
    else
      {
	  dist =
	      vknn_rect_distance (ctx, rtree_minx, rtree_miny, rtree_maxx,
				  rtree_maxy);
	  if (dist < ctx->min_dist)
	    {
		ctx->minx = rtree_minx;
		ctx->miny = rtree_miny;
		ctx->maxx = rtree_maxx;
		ctx->maxy = rtree_maxy;
		ctx->min_dist = dist;
		ctx->level = info->iLevel;
	    }
	  info->eWithin = NOT_WITHIN;
      }
    return SQLITE_OK;
}

static int
vknn_filter (sqlite3_vtab_cursor * pCursor, int idxNum, const char *idxStr,
	     int argc, sqlite3_value ** argv)
{
/* setting up a cursor filter */
    char *db_prefix = NULL;
    char *table_name = NULL;
    char *geom_column = NULL;
    char *xtable = NULL;
    char *xgeom = NULL;
    char *xgeomQ;
    char *xtableQ;
    char *idx_name;
    char *idx_nameQ;
    char *sql_statement;
    gaiaGeomCollPtr geom = NULL;
    int ok_table = 0;
    int ok_geom = 0;
    int ok_max = 0;
    int max_items = 3;
    int is_geographic;
    const unsigned char *blob;
    int size;
    int exists;
    int ret;
    double radius;
    double dist;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_dist = NULL;
    sqlite3_stmt *stmt_map_dist = NULL;
    sqlite3_stmt *stmt_rect_dist = NULL;
    sqlite3_stmt *stmt_pt_dist = NULL;
    sqlite3_stmt *stmt_buffer = NULL;
    sqlite3_stmt *stmt_rtree = NULL;
    sqlite3_stmt *stmt_rtree_count = NULL;
    VirtualKnnCursorPtr cursor = (VirtualKnnCursorPtr) pCursor;
    VirtualKnnPtr knn = (VirtualKnnPtr) cursor->pVtab;
    VKnnContextPtr vknn_context = knn->knn_ctx;
    if (idxStr)
	idxStr = idxStr;	/* unused arg warning suppression */
    cursor->eof = 1;
    if (idxStr)
	idxStr = idxStr;	/* unused arg warning suppression */
    cursor->eof = 1;
    if (idxNum == 1 && argc == 3)
      {
	  /* retrieving the Table/Column/Geometry params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		char *tn = (char *) sqlite3_value_text (argv[0]);
		vknn_parse_table_name (tn, &db_prefix, &table_name);
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	    {
		geom_column = (char *) sqlite3_value_text (argv[1]);
		ok_geom = 1;
	    }
	  if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[2]);
		size = sqlite3_value_bytes (argv[2]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (ok_table && ok_geom && geom)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }
    if (idxNum == 2 && argc == 2)
      {
	  /* retrieving the Table/Geometry params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		char *tn = (char *) sqlite3_value_text (argv[0]);
		vknn_parse_table_name (tn, &db_prefix, &table_name);
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[1]);
		size = sqlite3_value_bytes (argv[1]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (ok_table && geom)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }
    if (idxNum == 3 && argc == 4)
      {
	  /* retrieving the Table/Column/Geometry/MaxItems params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		char *tn = (char *) sqlite3_value_text (argv[0]);
		vknn_parse_table_name (tn, &db_prefix, &table_name);
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	    {
		geom_column = (char *) sqlite3_value_text (argv[1]);
		ok_geom = 1;
	    }
	  if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[2]);
		size = sqlite3_value_bytes (argv[2]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	    {
		max_items = sqlite3_value_int (argv[3]);
		if (max_items > 1024)
		    max_items = 1024;
		if (max_items < 1)
		    max_items = 1;
		ok_max = 1;
	    }
	  if (ok_table && ok_geom && geom && ok_max)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }
    if (idxNum == 4 && argc == 3)
      {
	  /* retrieving the Table/Geometry/MaxItems params */
	  if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	    {
		char *tn = (char *) sqlite3_value_text (argv[0]);
		vknn_parse_table_name (tn, &db_prefix, &table_name);
		ok_table = 1;
	    }
	  if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
	    {
		blob = sqlite3_value_blob (argv[1]);
		size = sqlite3_value_bytes (argv[1]);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, size);
	    }
	  if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		max_items = sqlite3_value_int (argv[2]);
		if (max_items > 1024)
		    max_items = 1024;
		if (max_items < 1)
		    max_items = 1;
		ok_max = 1;
	    }
	  if (ok_table && geom && ok_max)
	      ;
	  else
	    {
		/* invalid args */
		goto stop;
	    }
      }

/* checking if the corresponding R*Tree exists */
    if (ok_geom)
	exists =
	    vknn_check_rtree (knn->db, db_prefix, table_name, geom_column,
			      &xtable, &xgeom, &is_geographic);
    else
	exists =
	    vknn_find_rtree (knn->db, db_prefix, table_name, &xtable,
			     &xgeom, &is_geographic);
    if (!exists)
	goto stop;

/* building the Distance query */
    xgeomQ = gaiaDoubleQuotedSql (xgeom);
    xtableQ = gaiaDoubleQuotedSql (xtable);
    if (is_geographic)
	sql_statement =
	    sqlite3_mprintf
	    ("SELECT ST_Distance(?, \"%s\", 1) FROM \"%s\" WHERE rowid = ?",
	     xgeomQ, xtableQ);
    else
	sql_statement =
	    sqlite3_mprintf
	    ("SELECT ST_Distance(?, \"%s\") FROM \"%s\" WHERE rowid = ?",
	     xgeomQ, xtableQ);
    free (xgeomQ);
    free (xtableQ);
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_dist, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	goto stop;

/* building the Distance query - map units */
    xgeomQ = gaiaDoubleQuotedSql (xgeom);
    xtableQ = gaiaDoubleQuotedSql (xtable);
    sql_statement =
	sqlite3_mprintf
	("SELECT ST_Distance(?, \"%s\") FROM \"%s\" WHERE rowid = ?",
	 xgeomQ, xtableQ);
    free (xgeomQ);
    free (xtableQ);
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_map_dist, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	goto stop;

/* building the Distance query - RTree MBR */
    sql_statement = "SELECT ST_Distance(?, BuildMbr(?, ?, ?, ?))";
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_rect_dist, NULL);
    if (ret != SQLITE_OK)
	goto stop;

/* building the Distance query - Point */
    sql_statement = "SELECT ST_Distance(?, MakePoint(?, ?))";
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_pt_dist, NULL);
    if (ret != SQLITE_OK)
	goto stop;

/* building the Buffer query */
    sql_statement =
	"SELECT MbrMinX(x.g), MbrMinY(x.g), MbrMaxX(x.g), MbrMaxY(x.g) "
	"FROM (SELECT ST_Buffer(ST_Envelope(?), ?) AS g) AS x";
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_buffer, NULL);
    if (ret != SQLITE_OK)
	goto stop;

/* building the RTree query - Intersects */
    idx_name = sqlite3_mprintf ("idx_%s_%s", xtable, xgeom);
    idx_nameQ = gaiaDoubleQuotedSql (idx_name);
    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT pkid FROM main.\"%s\" WHERE xmin <= ? AND xmax >= ? AND ymin <= ? AND ymax >= ?",
	       idx_nameQ);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT pkid FROM \"%s\".\"%s\" WHERE xmin <= ? AND xmax >= ? AND ymin <= ? AND ymax >= ?",
	       quoted_db, idx_nameQ);
	  free (quoted_db);
      }
    free (idx_nameQ);
    sqlite3_free (idx_name);
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_rtree, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	goto stop;

/* building the RTree query - count items */
    idx_name = sqlite3_mprintf ("idx_%s_%s", xtable, xgeom);
    idx_nameQ = gaiaDoubleQuotedSql (idx_name);
    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf ("SELECT pkid FROM \"%s\" LIMIT 1024", idx_nameQ);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf ("SELECT pkid FROM \"%s\".\"%s\" LIMIT 1024",
			       quoted_db, idx_nameQ);
	  free (quoted_db);
      }
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt_rtree_count, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	goto stop;

/* installing the R*Tree query callback */
    gaiaMbrGeometry (geom);
    vknn_init_context (vknn_context, xtable, xgeom, geom, max_items,
		       stmt_dist, stmt_map_dist, stmt_rect_dist, stmt_pt_dist,
		       stmt_buffer, stmt_rtree, stmt_rtree_count);
    gaiaFreeGeomColl (geom);
    geom = NULL;		/* releasing ownership on geom */
    stmt_dist = NULL;		/* releasing ownership on stmt_dist */
    stmt_map_dist = NULL;	/* releasing ownership on stmt_map_dist */
    stmt_rect_dist = NULL;	/* releasing ownership on stmt_rect */
    stmt_pt_dist = NULL;	/* releasing ownership on stmt_point */
    stmt_buffer = NULL;		/* releasing ownership on stmt_buffer */
    stmt_rtree = NULL;		/* releasing ownership on stmt_rtree */
    stmt_rtree_count = NULL;	/* releasing ownership on stmt_rtree_count */
    sqlite3_rtree_query_callback (knn->db, "knn_position", vknn_query_callback,
				  vknn_context, NULL);

/* building the RTree query - callback */
    idx_name = sqlite3_mprintf ("idx_%s_%s", xtable, xgeom);
    idx_nameQ = gaiaDoubleQuotedSql (idx_name);
    if (db_prefix == NULL)
      {
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT pkid FROM main.\"%s\" WHERE pkid MATCH knn_position(1)",
	       idx_nameQ);
      }
    else
      {
	  char *quoted_db = gaiaDoubleQuotedSql (db_prefix);
	  sql_statement =
	      sqlite3_mprintf
	      ("SELECT pkid FROM \"%s\".\"%s\" WHERE pkid MATCH knn_position(1)",
	       quoted_db, idx_nameQ);
	  free (quoted_db);
      }
    free (idx_nameQ);
    sqlite3_free (idx_name);
    ret =
	sqlite3_prepare_v2 (knn->db, sql_statement, strlen (sql_statement),
			    &stmt, NULL);
    sqlite3_free (sql_statement);
    if (ret != SQLITE_OK)
	goto stop;

    vknn_context->bbox_minx = -DBL_MAX;
    vknn_context->bbox_miny = -DBL_MAX;
    vknn_context->bbox_maxx = DBL_MAX;
    vknn_context->bbox_maxy = DBL_MAX;
    vknn_context->current_level = 1024;
    while (1)
      {
	  /* repeatedly querying the R*Tree until finding the nearest BBOX */
	  vknn_context->minx = DBL_MAX;
	  vknn_context->miny = DBL_MAX;
	  vknn_context->maxx = -DBL_MAX;
	  vknn_context->maxy = -DBL_MAX;
	  vknn_context->min_dist = DBL_MAX;
	  sqlite3_step (stmt);
	  vknn_context->bbox_minx = vknn_context->minx;
	  vknn_context->bbox_miny = vknn_context->miny;
	  vknn_context->bbox_maxx = vknn_context->maxx;
	  vknn_context->bbox_maxy = vknn_context->maxy;
	  if (vknn_context->level <= 1)
	      break;
	  vknn_context->current_level = vknn_context->level - 1;
      }
    radius =
	vknn_pt_distance (vknn_context, vknn_context->bbox_minx,
			  vknn_context->bbox_miny);
    dist =
	vknn_pt_distance (vknn_context, vknn_context->bbox_minx,
			  vknn_context->bbox_maxy);
    if (dist > radius)
	radius = dist;
    dist =
	vknn_pt_distance (vknn_context, vknn_context->bbox_maxx,
			  vknn_context->bbox_miny);
    if (dist > radius)
	radius = dist;
    dist =
	vknn_pt_distance (vknn_context, vknn_context->bbox_maxx,
			  vknn_context->bbox_maxy);
    if (dist > radius)
	radius = dist;

    while (1)
      {
	  if (!vknn_bufferize (vknn_context, radius))
	      break;
	  ret = vknn_rtree_query (vknn_context);
	  if (ret <= 0 || ret >= max_items || ret >= vknn_context->rtree_count)
	      break;
	  radius *= 1.05;
      }

    if (vknn_context->curr_items == 0)
	cursor->eof = 1;
    else
	cursor->eof = 0;
    cursor->CurrentIndex = 0;
  stop:
    if (geom)
	gaiaFreeGeomColl (geom);
    if (xtable)
	free (xtable);
    if (xgeom)
	free (xgeom);
    if (db_prefix)
	free (db_prefix);
    if (table_name)
	free (table_name);
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_dist != NULL)
	sqlite3_finalize (stmt_dist);
    if (stmt_map_dist != NULL)
	sqlite3_finalize (stmt_map_dist);
    if (stmt_rect_dist != NULL)
	sqlite3_finalize (stmt_rect_dist);
    if (stmt_pt_dist != NULL)
	sqlite3_finalize (stmt_pt_dist);
    if (stmt_buffer != NULL)
	sqlite3_finalize (stmt_buffer);
    if (stmt_rtree != NULL)
	sqlite3_finalize (stmt_rtree);
    if (stmt_rtree_count != NULL)
	sqlite3_finalize (stmt_rtree_count);
    return SQLITE_OK;
}

static int
vknn_next (sqlite3_vtab_cursor * pCursor)
{
/* fetching a next row from cursor */
    VirtualKnnCursorPtr cursor = (VirtualKnnCursorPtr) pCursor;
    VKnnContextPtr ctx = cursor->pVtab->knn_ctx;
    cursor->CurrentIndex += 1;
    if (cursor->CurrentIndex >= ctx->curr_items)
	cursor->eof = 1;
    return SQLITE_OK;
}

static int
vknn_eof (sqlite3_vtab_cursor * pCursor)
{
/* cursor EOF */
    VirtualKnnCursorPtr cursor = (VirtualKnnCursorPtr) pCursor;
    return cursor->eof;
}

static int
vknn_column (sqlite3_vtab_cursor * pCursor, sqlite3_context * pContext,
	     int column)
{
/* fetching value for the Nth column */
    VirtualKnnCursorPtr cursor = (VirtualKnnCursorPtr) pCursor;
    VKnnContextPtr ctx = cursor->pVtab->knn_ctx;
    VKnnItemPtr item = NULL;
    if (cursor || column)
	cursor = cursor;	/* unused arg warning suppression */
    if (column)
	column = column;	/* unused arg warning suppression */
    if (cursor->CurrentIndex < ctx->curr_items)
	item = ctx->knn_array + cursor->CurrentIndex;
    if (column == 0)
      {
	  /* the Table Name column */
	  sqlite3_result_text (pContext, ctx->table_name,
			       strlen (ctx->table_name), SQLITE_STATIC);
      }
    else if (column == 1)
      {
	  /* the GeometryColumn Name column */
	  sqlite3_result_text (pContext, ctx->column_name,
			       strlen (ctx->column_name), SQLITE_STATIC);
      }
    else if (column == 2)
      {
	  /* the Reference Geometry column */
	  sqlite3_result_blob (pContext, ctx->blob, ctx->blob_size,
			       SQLITE_STATIC);
      }
    else if (column == 3)
      {
	  /* the Max Items column */
	  sqlite3_result_int (pContext, ctx->max_items);
      }
    else if (column == 4)
      {
	  /* the index column */
	  sqlite3_result_int (pContext, cursor->CurrentIndex + 1);
      }
    else if ((column == 5 || column == 6) && item != NULL)
      {
	  if (column == 5)
	    {
		/* the RowID column */
		sqlite3_result_int64 (pContext, item->rowid);
	    }
	  else if (column == 6)
	    {
		/* the Distance column */
		sqlite3_result_double (pContext, item->dist);
	    }
	  else
	      sqlite3_result_null (pContext);
      }
    else
	sqlite3_result_null (pContext);
    return SQLITE_OK;
}

static int
vknn_rowid (sqlite3_vtab_cursor * pCursor, sqlite_int64 * pRowid)
{
/* fetching the ROWID */
    VirtualKnnCursorPtr cursor = (VirtualKnnCursorPtr) pCursor;
    *pRowid = cursor->CurrentIndex;
    return SQLITE_OK;
}

static int
vknn_update (sqlite3_vtab * pVTab, int argc, sqlite3_value ** argv,
	     sqlite_int64 * pRowid)
{
/* generic update [INSERT / UPDATE / DELETE */
    if (pRowid || argc || argv || pVTab)
	pRowid = pRowid;	/* unused arg warning suppression */
/* read only datasource */
    return SQLITE_READONLY;
}

static int
vknn_begin (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vknn_sync (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vknn_commit (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vknn_rollback (sqlite3_vtab * pVTab)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    return SQLITE_OK;
}

static int
vknn_rename (sqlite3_vtab * pVTab, const char *zNew)
{
/* BEGIN TRANSACTION */
    if (pVTab)
	pVTab = pVTab;		/* unused arg warning suppression */
    if (zNew)
	zNew = zNew;		/* unused arg warning suppression */
    return SQLITE_ERROR;
}

static int
spliteKnnInit (sqlite3 * db)
{
    int rc = SQLITE_OK;
    my_knn_module.iVersion = 1;
    my_knn_module.xCreate = &vknn_create;
    my_knn_module.xConnect = &vknn_connect;
    my_knn_module.xBestIndex = &vknn_best_index;
    my_knn_module.xDisconnect = &vknn_disconnect;
    my_knn_module.xDestroy = &vknn_destroy;
    my_knn_module.xOpen = &vknn_open;
    my_knn_module.xClose = &vknn_close;
    my_knn_module.xFilter = &vknn_filter;
    my_knn_module.xNext = &vknn_next;
    my_knn_module.xEof = &vknn_eof;
    my_knn_module.xColumn = &vknn_column;
    my_knn_module.xRowid = &vknn_rowid;
    my_knn_module.xUpdate = &vknn_update;
    my_knn_module.xBegin = &vknn_begin;
    my_knn_module.xSync = &vknn_sync;
    my_knn_module.xCommit = &vknn_commit;
    my_knn_module.xRollback = &vknn_rollback;
    my_knn_module.xFindFunction = NULL;
    my_knn_module.xRename = &vknn_rename;
    sqlite3_create_module_v2 (db, "VirtualKNN", &my_knn_module, NULL, 0);
    return rc;
}

SPATIALITE_PRIVATE int
virtual_knn_extension_init (void *xdb)
{
    sqlite3 *db = (sqlite3 *) xdb;
    return spliteKnnInit (db);
}

#endif /* end KNN conditional */
#endif /* end GEOS conditional */
