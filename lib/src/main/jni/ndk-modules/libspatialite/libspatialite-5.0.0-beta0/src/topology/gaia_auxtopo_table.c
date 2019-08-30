/*

 gaia_auxtopo_table.c -- implementation of the Topology module
                         methods processing a whole GeoTable
    
 version 4.3, 2015 July 15

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
(Topology support) 

CIG: 6038019AE5

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <math.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaia_topology.h>
#include <spatialite/gaia_network.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <librttopo.h>

#include <lwn_network.h>

#include "topology_private.h"
#include "network_private.h"

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

#define GAIA_UNUSED() if (argc || argv) argc = argc;

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTable (GaiaTopologyAccessorPtr accessor,
			  const char *db_prefix, const char *table,
			  const char *column, double tolerance,
			  int line_max_points, double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (topo == NULL)
	return 0;
    if (topo->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (topo->cache);
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }

/* building the SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (column);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\"", xcolumn,
			 xprefix, xtable);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTable error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_NULL)
		    continue;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
						       gpkg_amphibious);
		      if (geom != NULL)
			{
			    if (!auxtopo_insert_into_topology
				(accessor, geom, tolerance, line_max_points,
				 max_length, GAIA_MODE_TOPO_FACE, NULL))
			      {
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    gaiaFreeGeomColl (geom);
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_FromGeoTable error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_FromGeoTable error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_FromGeoTable error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTableNoFace (GaiaTopologyAccessorPtr accessor,
				const char *db_prefix, const char *table,
				const char *column, double tolerance,
				int line_max_points, double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry
/  without determining generated faces */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (topo == NULL)
	return 0;
    if (topo->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (topo->cache);
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }

/* building the SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    xcolumn = gaiaDoubleQuotedSql (column);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\"", xcolumn,
			 xprefix, xtable);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_FromGeoTableNoFace error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_NULL)
		    continue;
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
						       gpkg_amphibious);
		      if (geom != NULL)
			{
			    if (!auxtopo_insert_into_topology
				(accessor, geom, tolerance, line_max_points,
				 max_length, GAIA_MODE_TOPO_NO_FACE, NULL))
			      {
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    gaiaFreeGeomColl (geom);
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_FromGeoTableNoFace error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_FromGeoTableNoFace error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_FromGeoTableNoFace error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
insert_into_dustbin (sqlite3 * sqlite, const void *cache,
		     sqlite3_stmt * stmt_dustbin, sqlite3_int64 pk_value,
		     const char *message, double tolerance, int *count,
		     gaiaGeomCollPtr geom)
{
/* failing feature: inserting a reference into the dustbin table */
    int ret;

    start_topo_savepoint (sqlite, cache);
    sqlite3_reset (stmt_dustbin);
    sqlite3_clear_bindings (stmt_dustbin);
/* binding the Primary Key */
    sqlite3_bind_int64 (stmt_dustbin, 1, pk_value);
/* binding the error message */
    sqlite3_bind_text (stmt_dustbin, 2, message, strlen (message),
		       SQLITE_STATIC);
/* binding the tolerance value */
    sqlite3_bind_double (stmt_dustbin, 3, tolerance);
/* binding the failing geometry */
    if (geom == NULL)
	sqlite3_bind_null (stmt_dustbin, 4);
    else
      {
	  unsigned char *blob = NULL;
	  int blob_size = 0;
	  gaiaToSpatiaLiteBlobWkb (geom, &blob, &blob_size);
	  if (blob == NULL)
	      sqlite3_bind_null (stmt_dustbin, 4);
	  else
	      sqlite3_bind_blob (stmt_dustbin, 4, blob, blob_size, free);
      }
    ret = sqlite3_step (stmt_dustbin);

/* inserting the row into the dustbin table */
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  release_topo_savepoint (sqlite, cache);
	  *count += 1;
	  return 1;
      }

    /* some unexpected error occurred */
    spatialite_e ("TopoGeo_FromGeoTableExt error: \"%s\"",
		  sqlite3_errmsg (sqlite));
    rollback_topo_savepoint (sqlite, cache);
    return 0;
}

static int
do_FromGeoTableExtended_block (GaiaTopologyAccessorPtr accessor,
			       sqlite3_stmt * stmt, sqlite3_stmt * stmt_dustbin,
			       double tolerance, int line_max_points,
			       double max_length, sqlite3_int64 start,
			       sqlite3_int64 * last, sqlite3_int64 * invalid,
			       int *dustbin_count, sqlite3_int64 * dustbin_row,
			       int mode)
{
/* attempting to import a whole block of input features */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int totcnt = 0;
    sqlite3_int64 last_rowid;

    if (topo->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (topo->cache);
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }

    start_topo_savepoint (topo->db_handle, topo->cache);

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, start);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt, 0);
		int igeo = sqlite3_column_count (stmt) - 1;	/* geometry always corresponds to the last resultset column */
		if (rowid == *invalid)
		  {
		      /* succesfully recovered a previously failing block */
		      release_topo_savepoint (topo->db_handle, topo->cache);
		      *last = last_rowid;
		      return 1;
		  }
		totcnt++;
		if (totcnt > 256)
		  {
		      /* succesfully imported a full block */
		      release_topo_savepoint (topo->db_handle, topo->cache);
		      *last = last_rowid;
		      return 1;
		  }
		if (sqlite3_column_type (stmt, igeo) == SQLITE_NULL)
		  {
		      last_rowid = rowid;
		      continue;
		  }
		if (sqlite3_column_type (stmt, igeo) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt, igeo);
		      int blob_sz = sqlite3_column_bytes (stmt, igeo);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
						       gpkg_amphibious);
		      if (geom != NULL)
			{
			    gaiaGeomCollPtr failing_geometry = NULL;
			    gaiatopo_reset_last_error_msg (accessor);
			    if (tolerance < 0.0)
				tolerance = topo->tolerance;
			    if (!auxtopo_insert_into_topology
				(accessor, geom, tolerance, line_max_points,
				 max_length, mode, &failing_geometry))
			      {
				  char *msg;
				  const char *rt_msg =
				      gaiaGetRtTopoErrorMsg (topo->cache);
				  if (rt_msg == NULL)
				      msg =
					  sqlite3_mprintf
					  ("TopoGeo_FromGeoTableExt exception: UNKNOWN reason");
				  else
				      msg = sqlite3_mprintf ("%s", rt_msg);
				  rollback_topo_savepoint (topo->db_handle,
							   topo->cache);
				  gaiaFreeGeomColl (geom);
				  if (tolerance < 0.0)
				      tolerance = topo->tolerance;
				  if (!insert_into_dustbin
				      (topo->db_handle, topo->cache,
				       stmt_dustbin, rowid, msg, tolerance,
				       dustbin_count, failing_geometry))
				    {
					sqlite3_free (msg);
					goto error;
				    }
				  sqlite3_free (msg);
				  if (failing_geometry != NULL)
				      gaiaFreeGeomColl (failing_geometry);
				  last_rowid = rowid;
				  *invalid = rowid;
				  *dustbin_row =
				      sqlite3_last_insert_rowid
				      (topo->db_handle);
				  return 0;
			      }
			    gaiaFreeGeomColl (geom);
			    if (failing_geometry != NULL)
				gaiaFreeGeomColl (failing_geometry);
			    last_rowid = rowid;
			}
		      else
			{
			    rollback_topo_savepoint (topo->db_handle,
						     topo->cache);
			    if (tolerance < 0.0)
				tolerance = topo->tolerance;
			    if (!insert_into_dustbin
				(topo->db_handle, topo->cache, stmt_dustbin,
				 rowid,
				 "TopoGeo_FromGeoTableExt error: Invalid Geometry",
				 tolerance, dustbin_count, NULL))
				goto error;
			}
		      last_rowid = rowid;
		  }
		else
		  {
		      rollback_topo_savepoint (topo->db_handle, topo->cache);
		      if (!insert_into_dustbin
			  (topo->db_handle, topo->cache, stmt_dustbin, rowid,
			   "TopoGeo_FromGeoTableExt error: not a BLOB value",
			   tolerance, dustbin_count, NULL))
			  goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		rollback_topo_savepoint (topo->db_handle, topo->cache);
		goto error;
	    }
      }
/* eof */
    release_topo_savepoint (topo->db_handle, topo->cache);
    return 2;

  error:
    return -1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTableExtended (GaiaTopologyAccessorPtr accessor,
				  const char *sql_in, const char *sql_out,
				  const char *sql_in2, double tolerance,
				  int line_max_points, double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry - Extended mode */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_dustbin = NULL;
    sqlite3_stmt *stmt_retry = NULL;
    int ret;
    int dustbin_count = 0;
    sqlite3_int64 start = -1;
    sqlite3_int64 last;
    sqlite3_int64 invalid = -1;
    sqlite3_int64 dustbin_row = -1;

    if (topo == NULL)
	return 0;
    if (sql_in == NULL)
	return 0;
    if (sql_out == NULL)
	return 0;

/* building the SQL statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_in, strlen (sql_in), &stmt,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the SQL dustbin statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_out, strlen (sql_out),
			    &stmt_dustbin, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the SQL retry statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_in2, strlen (sql_in2),
			    &stmt_retry, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_FromGeoTableExt error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* main loop: attempting to import a block of features */
	  ret =
	      do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
					     tolerance, line_max_points,
					     max_length, start, &last, &invalid,
					     &dustbin_count, &dustbin_row,
					     GAIA_MODE_TOPO_FACE);
	  if (ret < 0)		/* some unexpected error occurred */
	      goto error;
	  if (ret > 1)
	    {
		/* eof */
		break;
	    }
	  if (ret == 0)
	    {
		/* found a failing feature; recovering */
		ret =
		    do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
						   tolerance, line_max_points,
						   max_length, start, &last,
						   &invalid, &dustbin_count,
						   &dustbin_row,
						   GAIA_MODE_TOPO_FACE);
		if (ret != 1)
		    goto error;
		start = invalid;
		invalid = -1;
		dustbin_row = -1;
		continue;
	    }
	  start = last;
	  invalid = -1;
	  dustbin_row = -1;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_dustbin);
    sqlite3_finalize (stmt_retry);
    return dustbin_count;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_dustbin != NULL)
	sqlite3_finalize (stmt_dustbin);
    return -1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_FromGeoTableNoFaceExtended (GaiaTopologyAccessorPtr accessor,
					const char *sql_in, const char *sql_out,
					const char *sql_in2, double tolerance,
					int line_max_points, double max_length)
{
/* attempting to import a whole GeoTable into a Topology-Geometry 
/  without determining generated faces - Extended mode */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_dustbin = NULL;
    sqlite3_stmt *stmt_retry = NULL;
    int ret;
    int dustbin_count = 0;
    sqlite3_int64 start = -1;
    sqlite3_int64 last;
    sqlite3_int64 invalid = -1;
    sqlite3_int64 dustbin_row = -1;

    if (topo == NULL)
	return 0;
    if (sql_in == NULL)
	return 0;
    if (sql_out == NULL)
	return 0;

/* building the SQL statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_in, strlen (sql_in), &stmt,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_FromGeoTableNoFaceExt error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the SQL dustbin statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_out, strlen (sql_out),
			    &stmt_dustbin, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_FromGeoTableNoFaceExt error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the SQL retry statement */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql_in2, strlen (sql_in2),
			    &stmt_retry, NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_FromGeoTableNoFaceExt error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* main loop: attempting to import a block of features */
	  ret =
	      do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
					     tolerance, line_max_points,
					     max_length, start, &last, &invalid,
					     &dustbin_count, &dustbin_row,
					     GAIA_MODE_TOPO_NO_FACE);
	  if (ret < 0)		/* some unexpected error occurred */
	      goto error;
	  if (ret > 1)
	    {
		/* eof */
		break;
	    }
	  if (ret == 0)
	    {
		/* found a failing feature; recovering */
		ret =
		    do_FromGeoTableExtended_block (accessor, stmt, stmt_dustbin,
						   tolerance, line_max_points,
						   max_length, start, &last,
						   &invalid, &dustbin_count,
						   &dustbin_row,
						   GAIA_MODE_TOPO_NO_FACE);
		if (ret != 1)
		    goto error;
		start = invalid;
		invalid = -1;
		dustbin_row = -1;
		continue;
	    }
	  start = last;
	  invalid = -1;
	  dustbin_row = -1;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_dustbin);
    sqlite3_finalize (stmt_retry);
    return dustbin_count;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_dustbin != NULL)
	sqlite3_finalize (stmt_dustbin);
    return -1;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetEdgeSeed (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge)
{
/* attempting to get a Point (seed) identifying a Topology Edge */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    gaiaGeomCollPtr point = NULL;
    if (topo == NULL)
	return NULL;

/* building the SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("GetEdgeSeed error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, edge);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (geom != NULL)
			{
			    int iv;
			    double x;
			    double y;
			    double z = 0.0;
			    double m = 0.0;
			    gaiaLinestringPtr ln = geom->FirstLinestring;
			    if (ln == NULL)
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("TopoGeo_GetEdgeSeed error: Invalid Geometry");
				  gaiatopo_set_last_error_msg (accessor, msg);
				  sqlite3_free (msg);
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    if (ln->Points == 2)
			      {
				  /*
				     // special case: if the Edge has only 2 points then
				     // the Seed will always be placed on the first point
				   */
				  iv = 0;
			      }
			    else
			      {
				  /* ordinary case: placing the Seed into the middle */
				  iv = ln->Points / 2;
			      }
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
				  gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z,
						    &m);
			      }
			    else
			      {
				  gaiaGetPoint (ln->Coords, iv, &x, &y);
			      }
			    gaiaFreeGeomColl (geom);
			    if (topo->has_z)
			      {
				  point = gaiaAllocGeomCollXYZ ();
				  gaiaAddPointToGeomCollXYZ (point, x, y, z);
			      }
			    else
			      {
				  point = gaiaAllocGeomColl ();
				  gaiaAddPointToGeomColl (point, x, y);
			      }
			    point->Srid = topo->srid;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_GetEdgeSeed error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_GetEdgeSeed error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_GetEdgeSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return point;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetFaceSeed (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* attempting to get a Point (seed) identifying a Topology Face */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    gaiaGeomCollPtr point = NULL;
    if (topo == NULL)
	return NULL;

/* building the SQL statement */
    sql =
	sqlite3_mprintf ("SELECT ST_PointOnSurface(ST_GetFaceGeometry(%Q, ?))",
			 topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("GetFaceSeed error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      point = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (point == NULL)
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_GetFaceSeed error: Invalid Geometry");
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_GetFaceSeed error: not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_GetFaceSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    return point;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

static int
delete_all_seeds (struct gaia_topology *topo)
{
/* deleting all existing Seeds */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;

    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

static int
update_outdated_edge_seeds (struct gaia_topology *topo)
{
/* updating all outdated Edge Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_in;

/* preparing the UPDATE statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET geom = "
			   "TopoGeo_GetEdgeSeed(%Q, edge_id) WHERE edge_id = ?",
			   xseeds, topo->topology_name);
    free (xseeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT s.edge_id FROM MAIN.\"%s\" AS s "
			   "JOIN MAIN.\"%s\" AS e ON (e.edge_id = s.edge_id) "
			   "WHERE s.edge_id IS NOT NULL AND e.timestamp > s.timestamp",
			   xseeds, xedges);
    free (xseeds);
    free (xedges);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		sqlite3_bind_int64 (stmt_out, 1,
				    sqlite3_column_int64 (stmt_in, 0));
		/* updating the Seeds table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_UpdateSeeds() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
update_outdated_face_seeds (struct gaia_topology *topo)
{
/* updating all outdated Face Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *xfaces;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_in;

/* preparing the UPDATE statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET geom = "
			   "TopoGeo_GetFaceSeed(%Q, face_id) WHERE face_id = ?",
			   xseeds, topo->topology_name);
    free (xseeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT x.face_id FROM MAIN.\"%s\" AS s, "
			   "(SELECT f.face_id AS face_id, Max(e.timestamp) AS max_tm "
			   "FROM MAIN.\"%s\" AS f "
			   "JOIN MAIN.\"%s\" AS e ON (e.left_face = f.face_id OR e.right_face = f.face_id) "
			   "GROUP BY f.face_id) AS x "
			   "WHERE s.face_id IS NOT NULL AND s.face_id = x.face_id AND x.max_tm > s.timestamp",
			   xseeds, xfaces, xedges);
    free (xseeds);
    free (xedges);
    free (xfaces);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		sqlite3_bind_int64 (stmt_out, 1,
				    sqlite3_column_int64 (stmt_in, 0));
		/* updating the Seeds table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_UpdateSeeds() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeoUpdateSeeds (GaiaTopologyAccessorPtr accessor, int incremental_mode)
{
/* updating all TopoGeo Seeds */
    char *table;
    char *xseeds;
    char *xedges;
    char *xfaces;
    char *sql;
    char *errMsg;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (!incremental_mode)
      {
	  /* deleting all existing Seeds */
	  if (!delete_all_seeds (topo))
	      return 0;
      }

/* paranoid precaution: deleting all orphan Edge Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE edge_id IN ("
			   "SELECT s.edge_id FROM MAIN.\"%s\" AS s "
			   "LEFT JOIN MAIN.\"%s\" AS e ON (s.edge_id = e.edge_id) "
			   "WHERE s.edge_id IS NOT NULL AND e.edge_id IS NULL)",
			   xseeds, xseeds, xedges);
    free (xseeds);
    free (xedges);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* paranoid precaution: deleting all orphan Face Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE face_id IN ("
			   "SELECT s.face_id FROM MAIN.\"%s\" AS s "
			   "LEFT JOIN MAIN.\"%s\" AS f ON (s.face_id = f.face_id) "
			   "WHERE s.face_id IS NOT NULL AND f.face_id IS NULL)",
			   xseeds, xseeds, xfaces);
    free (xseeds);
    free (xfaces);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* updating all outdated Edge Seeds */
    if (!update_outdated_edge_seeds (topo))
	return 0;

/* updating all outdated Facee Seeds */
    if (!update_outdated_face_seeds (topo))
	return 0;

/* inserting all missing Edge Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (seed_id, edge_id, face_id, geom) "
	 "SELECT NULL, e.edge_id, NULL, TopoGeo_GetEdgeSeed(%Q, e.edge_id) "
	 "FROM MAIN.\"%s\" AS e "
	 "LEFT JOIN MAIN.\"%s\" AS s ON (e.edge_id = s.edge_id) WHERE s.edge_id IS NULL",
	 xseeds, topo->topology_name, xedges, xseeds);
    free (xseeds);
    free (xedges);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    /* inserting all missing Face Seeds */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (seed_id, edge_id, face_id, geom) "
	 "SELECT NULL, NULL, f.face_id, TopoGeo_GetFaceSeed(%Q, f.face_id) "
	 "FROM MAIN.\"%s\" AS f "
	 "LEFT JOIN MAIN.\"%s\" AS s ON (f.face_id = s.face_id) "
	 "WHERE s.face_id IS NULL AND f.face_id <> 0", xseeds,
	 topo->topology_name, xfaces, xseeds);
    free (xseeds);
    free (xfaces);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeoSnapPointToSeed (GaiaTopologyAccessorPtr accessor,
			    gaiaGeomCollPtr pt, double distance)
{
/* snapping a Point to TopoSeeds */
    char *sql;
    char *table;
    char *xtable;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_snap = NULL;
    int ret;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr result = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

/* preparing the Snap statement */
    sql = "SELECT ST_Snap(?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_snap,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapPointToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT geom "
			   "FROM \"%s\" WHERE ST_Distance(?, geom) <= ? AND rowid IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ST_Buffer(?, ?))",
			   xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapPointToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* querying Seeds */
    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = pt->Srid;
    gaiaToSpatiaLiteBlobWkb (pt, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (pt, &blob2, &blob_size2);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_double (stmt, 2, distance);
    sqlite3_bind_blob (stmt, 3, blob2, blob_size2, free);
    sqlite3_bind_double (stmt, 4, distance * 1.2);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *p_blob = sqlite3_column_blob (stmt, 0);
		int blobsz = sqlite3_column_bytes (stmt, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_SnapPointToSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    stmt = NULL;
    if (result->FirstPoint == NULL)
	goto error;

/* Snap */
    gaiaToSpatiaLiteBlobWkb (pt, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (result, &blob2, &blob_size2);
    gaiaFreeGeomColl (result);
    result = NULL;
    sqlite3_reset (stmt_snap);
    sqlite3_clear_bindings (stmt_snap);
    sqlite3_bind_blob (stmt_snap, 1, blob, blob_size, free);
    sqlite3_bind_blob (stmt_snap, 2, blob2, blob_size2, free);
    sqlite3_bind_double (stmt_snap, 3, distance);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_snap);

	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_snap, 0) != SQLITE_NULL)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt_snap, 0);
		      int blobsz = sqlite3_column_bytes (stmt_snap, 0);
		      if (result != NULL)
			  gaiaFreeGeomColl (result);
		      result = gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_SnapPointToSeed error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_snap);
    stmt_snap = NULL;
    if (result == NULL)
	goto error;
    if (result->FirstLinestring != NULL || result->FirstPolygon != NULL)
	goto error;
    if (result->FirstPoint == NULL)
	goto error;
    if (result->FirstPoint != result->LastPoint)
	goto error;
    return result;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_snap != NULL)
	sqlite3_finalize (stmt_snap);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    return NULL;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeoSnapLinestringToSeed (GaiaTopologyAccessorPtr accessor,
				 gaiaGeomCollPtr ln, double distance)
{
/* snapping a Linestring to TopoSeeds */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    unsigned char *blob;
    int blob_size;
    unsigned char *blob2;
    int blob_size2;
    gaiaGeomCollPtr result = NULL;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_snap = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

/* preparing the Snap statement */
    sql = "SELECT ST_Snap(?, ?, ?)";
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_snap,
			    NULL);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapLinestringToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT edge_id, geom "
			   "FROM \"%s\" WHERE ST_Distance(?, geom) <= ? AND rowid IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ST_Buffer(?, ?))",
			   xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnapLinestringToSeed() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* querying Seeds */
    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = ln->Srid;
    gaiaToSpatiaLiteBlobWkb (ln, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (ln, &blob2, &blob_size2);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_double (stmt, 2, distance);
    sqlite3_bind_blob (stmt, 3, blob2, blob_size2, free);
    sqlite3_bind_double (stmt, 4, distance * 1.2);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) != SQLITE_NULL)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt, 1);
		      int blobsz = sqlite3_column_bytes (stmt, 1);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		      if (geom != NULL)
			{
			    gaiaPointPtr pt = geom->FirstPoint;
			    while (pt != NULL)
			      {
				  /* copying all Points into the result Geometry */
				  if (topo->has_z)
				      gaiaAddPointToGeomCollXYZ (result, pt->X,
								 pt->Y, pt->Z);
				  else
				      gaiaAddPointToGeomColl (result, pt->X,
							      pt->Y);
				  pt = pt->Next;
			      }
			    gaiaFreeGeomColl (geom);
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_SnapLinestringToSeed error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    stmt = NULL;
    if (result->FirstPoint == NULL)
	goto error;

/* Snap */
    gaiaToSpatiaLiteBlobWkb (ln, &blob, &blob_size);
    gaiaToSpatiaLiteBlobWkb (result, &blob2, &blob_size2);
    gaiaFreeGeomColl (result);
    result = NULL;
    sqlite3_reset (stmt_snap);
    sqlite3_clear_bindings (stmt_snap);
    sqlite3_bind_blob (stmt_snap, 1, blob, blob_size, free);
    sqlite3_bind_blob (stmt_snap, 2, blob2, blob_size2, free);
    sqlite3_bind_double (stmt_snap, 3, distance);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_snap);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_snap, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *p_blob =
			  sqlite3_column_blob (stmt_snap, 0);
		      int blobsz = sqlite3_column_bytes (stmt_snap, 0);
		      if (result != NULL)
			  gaiaFreeGeomColl (result);
		      result = gaiaFromSpatiaLiteBlobWkb (p_blob, blobsz);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_SnapLinestringToSeed error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_snap);
    stmt_snap = NULL;
    if (result == NULL)
	goto error;
    if (result->FirstPoint != NULL || result->FirstPolygon != NULL)
	goto error;
    if (result->FirstLinestring == NULL)
	goto error;
    if (result->FirstLinestring != result->LastLinestring)
	goto error;
    return result;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_snap != NULL)
	sqlite3_finalize (stmt_snap);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    return NULL;
}

static gaiaGeomCollPtr
make_geom_from_polyg (int srid, gaiaPolygonPtr pg)
{
/* quick constructor: Geometry based on external polyg */
    gaiaGeomCollPtr reference;
    if (pg->DimensionModel == GAIA_XY_Z_M)
	reference = gaiaAllocGeomCollXYZM ();
    else if (pg->DimensionModel == GAIA_XY_Z)
	reference = gaiaAllocGeomCollXYZ ();
    else if (pg->DimensionModel == GAIA_XY_M)
	reference = gaiaAllocGeomCollXYM ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    pg->Next = NULL;
    reference->FirstPolygon = pg;
    reference->LastPolygon = pg;
    return reference;
}

static void
do_eval_topogeo_point (struct gaia_topology *topo, gaiaGeomCollPtr result,
		       gaiaGeomCollPtr reference, sqlite3_stmt * stmt_node)
{
/* retrieving Points from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Node query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_blob (stmt_node, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_node, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_node, 0);
		int blob_sz = sqlite3_column_bytes (stmt_node, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_collect_topo_edges (struct gaia_topology *topo, gaiaGeomCollPtr sparse,
		       sqlite3_stmt * stmt_edge, sqlite3_int64 edge_id)
{
/* collecting Edge Geometries one by one */
    int ret;

/* initializing the Topo-Edge query */
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_int64 (stmt_edge, 1, edge_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_edge, 0);
		int blob_sz = sqlite3_column_bytes (stmt_edge, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaLinestringPtr ln = geom->FirstLinestring;
		      while (ln != NULL)
			{
			    if (topo->has_z)
				auxtopo_copy_linestring3d (ln, sparse);
			    else
				auxtopo_copy_linestring (ln, sparse);
			    ln = ln->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_eval_topogeo_line (struct gaia_topology *topo, gaiaGeomCollPtr result,
		      gaiaGeomCollPtr reference, sqlite3_stmt * stmt_seed_edge,
		      sqlite3_stmt * stmt_edge)
{
/* retrieving Linestrings from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr rearranged;
    gaiaLinestringPtr ln;

    if (topo->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = topo->srid;

/* initializing the Topo-Seed-Edge query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_edge);
    sqlite3_clear_bindings (stmt_seed_edge);
    sqlite3_bind_blob (stmt_seed_edge, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_edge, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id =
		    sqlite3_column_int64 (stmt_seed_edge, 0);
		do_collect_topo_edges (topo, sparse, stmt_edge, edge_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		gaiaFreeGeomColl (sparse);
		return;
	    }
      }

/* attempting to rearrange sparse lines */
    rearranged = gaiaLineMerge_r (topo->cache, sparse);
    gaiaFreeGeomColl (sparse);
    if (rearranged == NULL)
	return;
    ln = rearranged->FirstLinestring;
    while (ln != NULL)
      {
	  if (topo->has_z)
	      auxtopo_copy_linestring3d (ln, result);
	  else
	      auxtopo_copy_linestring (ln, result);
	  ln = ln->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static void
do_explode_topo_face (struct gaia_topology *topo, struct face_edges *list,
		      sqlite3_stmt * stmt_face, sqlite3_int64 face_id)
{
/* exploding all Edges required by the same face */
    int ret;

/* initializing the Topo-Face query */
    sqlite3_reset (stmt_face);
    sqlite3_clear_bindings (stmt_face);
    sqlite3_bind_int64 (stmt_face, 1, face_id);
    sqlite3_bind_int64 (stmt_face, 2, face_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_face, 0);
		sqlite3_int64 left_face = sqlite3_column_int64 (stmt_face, 1);
		sqlite3_int64 right_face = sqlite3_column_int64 (stmt_face, 2);
		const unsigned char *blob = sqlite3_column_blob (stmt_face, 3);
		int blob_sz = sqlite3_column_bytes (stmt_face, 3);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		    auxtopo_add_face_edge (list, face_id, edge_id, left_face,
					   right_face, geom);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_copy_ring3d (gaiaRingPtr in, gaiaRingPtr out)
{
/* inserting/copying a Ring 3D into another Ring */
    int iv;
    double x;
    double y;
    double z;
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	  gaiaSetPointXYZ (out->Coords, iv, x, y, z);
      }
}

static void
do_copy_ring (gaiaRingPtr in, gaiaRingPtr out)
{
/* inserting/copying a Ring into another Ring */
    int iv;
    double x;
    double y;
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPoint (in->Coords, iv, &x, &y);
	  gaiaSetPoint (out->Coords, iv, x, y);
      }
}

static void
do_copy_polygon3d (gaiaPolygonPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Polygon 3D into another Geometry */
    int ib;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out =
	gaiaAddPolygonToGeomColl (geom, rng_in->Points, in->NumInteriors);
    rng_out = out->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  rng_in = in->Interiors + ib;
	  rng_out = gaiaAddInteriorRing (out, ib, rng_in->Points);
	  do_copy_ring3d (rng_in, rng_out);
      }
}

static void
do_copy_polygon (gaiaPolygonPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Polygon into another Geometry */
    int ib;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out =
	gaiaAddPolygonToGeomColl (geom, rng_in->Points, in->NumInteriors);
    rng_out = out->Exterior;
    do_copy_ring (rng_in, rng_out);
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  rng_in = in->Interiors + ib;
	  rng_out = gaiaAddInteriorRing (out, ib, rng_in->Points);
	  do_copy_ring (rng_in, rng_out);
      }
}

static void
do_copy_filter_polygon3d (gaiaPolygonPtr in, gaiaGeomCollPtr geom,
			  const void *cache, double tolerance)
{
/* inserting/copying a Polygon 3D into another Geometry (with tolerance) */
    int ib;
    gaiaGeomCollPtr polyg;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out;
    double area;
    int ret;
    int numints = 0;

    polyg = gaiaAllocGeomCollXYZ ();
    pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
    rng_out = pg->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    ret = gaiaGeomCollArea_r (cache, polyg, &area);
    gaiaFreeGeomColl (polyg);
    if (!ret)
	return;
    if ((tolerance * tolerance) > area)
	return;			/* skipping smaller polygons */

    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* counting how many interior rings do we really have */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomCollXYZ ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring3d (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  numints++;
      }

    rng_in = in->Exterior;
    out = gaiaAddPolygonToGeomColl (geom, rng_in->Points, numints);
    rng_out = out->Exterior;
    do_copy_ring3d (rng_in, rng_out);
    numints = 0;
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* copying interior rings */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomCollXYZ ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring3d (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  rng_out = gaiaAddInteriorRing (out, numints++, rng_in->Points);
	  do_copy_ring3d (rng_in, rng_out);
      }
}

static void
do_copy_filter_polygon (gaiaPolygonPtr in, gaiaGeomCollPtr geom,
			const void *cache, double tolerance)
{
/* inserting/copying a Polygon into another Geometry (with tolerance) */
    int ib;
    gaiaGeomCollPtr polyg;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng_out;
    gaiaRingPtr rng_in = in->Exterior;
    gaiaPolygonPtr out;
    double area;
    int ret;
    int numints = 0;

    polyg = gaiaAllocGeomColl ();
    pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
    rng_out = pg->Exterior;
    do_copy_ring (rng_in, rng_out);
    ret = gaiaGeomCollArea_r (cache, polyg, &area);
    gaiaFreeGeomColl (polyg);
    if (!ret)
	return;
    if ((tolerance * tolerance) > area)
	return;			/* skipping smaller polygons */

    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* counting how many interior rings do we really have */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomColl ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  numints++;
      }

    rng_in = in->Exterior;
    out = gaiaAddPolygonToGeomColl (geom, rng_in->Points, numints);
    rng_out = out->Exterior;
    do_copy_ring (rng_in, rng_out);
    numints = 0;
    for (ib = 0; ib < in->NumInteriors; ib++)
      {
	  /* copying interior rings */
	  rng_in = in->Interiors + ib;
	  polyg = gaiaAllocGeomColl ();
	  pg = gaiaAddPolygonToGeomColl (polyg, rng_in->Points, 0);
	  rng_out = pg->Exterior;
	  do_copy_ring (rng_in, rng_out);
	  ret = gaiaGeomCollArea_r (cache, polyg, &area);
	  gaiaFreeGeomColl (polyg);
	  if (!ret)
	      continue;
	  if ((tolerance * tolerance) > area)
	      continue;		/* skipping smaller holes */
	  rng_out = gaiaAddInteriorRing (out, numints++, rng_in->Points);
	  do_copy_ring (rng_in, rng_out);
      }
}

static void
do_eval_topo_polyg (struct gaia_topology *topo, gaiaGeomCollPtr result,
		    gaiaGeomCollPtr reference, sqlite3_stmt * stmt_seed_face,
		    sqlite3_stmt * stmt_face)
{
/* retrieving Polygons from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr rearranged;
    gaiaPolygonPtr pg;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

/* initializing the Topo-Seed-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_face);
    sqlite3_clear_bindings (stmt_seed_face);
    sqlite3_bind_blob (stmt_seed_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id =
		    sqlite3_column_int64 (stmt_seed_face, 0);
		do_explode_topo_face (topo, list, stmt_face, face_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		auxtopo_free_face_edges (list);
		return;
	    }
      }

/* attempting to rearrange sparse lines into Polygons */
    auxtopo_select_valid_face_edges (list);
    rearranged = auxtopo_polygonize_face_edges (list, topo->cache);
    auxtopo_free_face_edges (list);
    if (rearranged == NULL)
	return;
    pg = rearranged->FirstPolygon;
    while (pg != NULL)
      {
	  if (topo->has_z)
	      do_copy_polygon3d (pg, result);
	  else
	      do_copy_polygon (pg, result);
	  pg = pg->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static void
do_eval_topo_polyg_generalize (struct gaia_topology *topo,
			       gaiaGeomCollPtr result,
			       gaiaGeomCollPtr reference,
			       sqlite3_stmt * stmt_seed_face,
			       sqlite3_stmt * stmt_face, double tolerance)
{
/* retrieving Polygons from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr rearranged;
    gaiaPolygonPtr pg;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

/* initializing the Topo-Seed-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_face);
    sqlite3_clear_bindings (stmt_seed_face);
    sqlite3_bind_blob (stmt_seed_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id =
		    sqlite3_column_int64 (stmt_seed_face, 0);
		do_explode_topo_face (topo, list, stmt_face, face_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoGeo_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		auxtopo_free_face_edges (list);
		return;
	    }
      }

/* attempting to rearrange sparse lines into Polygons */
    auxtopo_select_valid_face_edges (list);
    rearranged = auxtopo_polygonize_face_edges_generalize (list, topo->cache);
    auxtopo_free_face_edges (list);
    if (rearranged == NULL)
	return;
    pg = rearranged->FirstPolygon;
    while (pg != NULL)
      {
	  if (topo->has_z)
	      do_copy_filter_polygon3d (pg, result, topo->cache, tolerance);
	  else
	      do_copy_filter_polygon (pg, result, topo->cache, tolerance);
	  pg = pg->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static gaiaGeomCollPtr
do_eval_topogeo_geom (struct gaia_topology *topo, gaiaGeomCollPtr geom,
		      sqlite3_stmt * stmt_seed_edge,
		      sqlite3_stmt * stmt_seed_face, sqlite3_stmt * stmt_node,
		      sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_face,
		      int out_type, double tolerance)
{
/* retrieving Topology-Geometry geometries via matching Seeds */
    gaiaGeomCollPtr result;

    if (topo->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = topo->srid;
    result->DeclaredType = out_type;

    if (out_type == GAIA_POINT || out_type == GAIA_MULTIPOINT
	|| out_type == GAIA_GEOMETRYCOLLECTION || out_type == GAIA_UNKNOWN)
      {
	  /* processing all Points */
	  gaiaPointPtr pt = geom->FirstPoint;
	  while (pt != NULL)
	    {
		gaiaPointPtr next = pt->Next;
		gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
		    auxtopo_make_geom_from_point (topo->srid, topo->has_z, pt);
		do_eval_topogeo_point (topo, result, reference, stmt_node);
		auxtopo_destroy_geom_from (reference);
		pt->Next = next;
		pt = pt->Next;
	    }
      }

    if (out_type == GAIA_MULTILINESTRING || out_type == GAIA_GEOMETRYCOLLECTION
	|| out_type == GAIA_UNKNOWN)
      {
	  /* processing all Linestrings */
	  gaiaLinestringPtr ln = geom->FirstLinestring;
	  while (ln != NULL)
	    {
		gaiaLinestringPtr next = ln->Next;
		gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
		    auxtopo_make_geom_from_line (topo->srid, ln);
		do_eval_topogeo_line (topo, result, reference, stmt_seed_edge,
				      stmt_edge);
		auxtopo_destroy_geom_from (reference);
		ln->Next = next;
		ln = ln->Next;
	    }
      }

    if (out_type == GAIA_MULTIPOLYGON || out_type == GAIA_GEOMETRYCOLLECTION
	|| out_type == GAIA_UNKNOWN)
      {
	  /* processing all Polygons */
	  gaiaPolygonPtr pg = geom->FirstPolygon;
	  while (pg != NULL)
	    {
		gaiaPolygonPtr next = pg->Next;
		gaiaGeomCollPtr reference =
		    make_geom_from_polyg (topo->srid, pg);
		if (tolerance > 0.0)
		    do_eval_topo_polyg_generalize (topo, result, reference,
						   stmt_seed_face, stmt_face,
						   tolerance);
		else
		    do_eval_topo_polyg (topo, result, reference, stmt_seed_face,
					stmt_face);
		auxtopo_destroy_geom_from (reference);
		pg->Next = next;
		pg = pg->Next;
	    }
      }

    if (result->FirstPoint == NULL && result->FirstLinestring == NULL
	&& result->FirstPolygon == NULL)
	goto error;
    return result;

  error:
    gaiaFreeGeomColl (result);
    return NULL;
}

static int
do_eval_topogeo_seeds (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
		       int ref_geom_col, sqlite3_stmt * stmt_ins,
		       sqlite3_stmt * stmt_seed_edge,
		       sqlite3_stmt * stmt_seed_face, sqlite3_stmt * stmt_node,
		       sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_face,
		       int out_type, double tolerance)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      if (icol == ref_geom_col)
			{
			    /* the geometry column */
			    const unsigned char *blob =
				sqlite3_column_blob (stmt_ref, icol);
			    int blob_sz = sqlite3_column_bytes (stmt_ref, icol);
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    if (geom != NULL)
			      {
				  gaiaGeomCollPtr result;
				  unsigned char *p_blob;
				  int n_bytes;
				  int gpkg_mode = 0;
				  int tiny_point = 0;
				  if (topo->cache != NULL)
				    {
					struct splite_internal_cache *cache =
					    (struct splite_internal_cache
					     *) (topo->cache);
					gpkg_mode = cache->gpkg_mode;
					tiny_point = cache->tinyPointEnabled;
				    }
				  result = do_eval_topogeo_geom (topo, geom,
								 stmt_seed_edge,
								 stmt_seed_face,
								 stmt_node,
								 stmt_edge,
								 stmt_face,
								 out_type,
								 tolerance);
				  gaiaFreeGeomColl (geom);
				  if (result != NULL)
				    {
					gaiaToSpatiaLiteBlobWkbEx2 (result,
								    &p_blob,
								    &n_bytes,
								    gpkg_mode,
								    tiny_point);
					gaiaFreeGeomColl (result);
					sqlite3_bind_blob (stmt_ins, icol + 1,
							   p_blob, n_bytes,
							   free);
				    }
				  else
				      sqlite3_bind_null (stmt_ins, icol + 1);
			      }
			    else
				sqlite3_bind_null (stmt_ins, icol + 1);
			    continue;
			}
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
					   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ToGeoTableGeneralize (GaiaTopologyAccessorPtr accessor,
				  const char *db_prefix, const char *ref_table,
				  const char *ref_column, const char *out_table,
				  double tolerance, int with_spatial_index)
{
/* 
/ attempting to create and populate a new GeoTable out from a Topology-Geometry 
/ (simplified/generalized form)
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_seed_edge = NULL;
    sqlite3_stmt *stmt_seed_face = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *create;
    char *select;
    char *insert;
    char *sql;
    char *errMsg;
    char *xprefix;
    char *xtable;
    int ref_type;
    const char *type;
    int out_type;
    int ref_geom_col;
    if (topo == NULL)
	return 0;

/* incrementally updating all Topology Seeds */
    if (!gaiaTopoGeoUpdateSeeds (accessor, 1))
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_togeotable_sql
	(topo->db_handle, db_prefix, ref_table, ref_column, out_table, &create,
	 &select, &insert, &ref_geom_col))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_geometry_type
	(topo->db_handle, db_prefix, ref_table, ref_column, &ref_type))
	goto error;
    switch (ref_type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  type = "POINT";
	  out_type = GAIA_POINT;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  type = "MULTIPOINT";
	  out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  type = "MULTILINESTRING";
	  out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  type = "MULTIPOLYGON";
	  out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  type = "GEOMETRYCOLLECTION";
	  out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  type = "GEOMETRY";
	  out_type = GAIA_UNKNOWN;
	  break;
      };

/* creating the output Geometry Column */
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), Lower(%Q), %d, '%s', '%s')",
	 out_table, ref_column, topo->srid, type,
	 (topo->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (with_spatial_index)
      {
	  /* adding a Spatial Index supporting the Geometry Column */
	  sql =
	      sqlite3_mprintf
	      ("SELECT CreateSpatialIndex(Lower(%Q), Lower(%Q))",
	       out_table, ref_column);
	  ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
		     errMsg);
		sqlite3_free (errMsg);
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Edges query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" "
			   "WHERE edge_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_seed_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Faces query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT face_id FROM MAIN.\"%s\" "
			   "WHERE face_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_seed_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    if (tolerance > 0.0)
	sql =
	    sqlite3_mprintf
	    ("SELECT ST_SimplifyPreserveTopology(geom, %1.6f) FROM MAIN.\"%s\" WHERE edge_id = ?",
	     tolerance, xtable, xprefix);
    else
	sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			       xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    if (tolerance > 0.0)
	sql =
	    sqlite3_mprintf
	    ("SELECT edge_id, left_face, right_face, ST_SimplifyPreserveTopology(geom, %1.6f) FROM MAIN.\"%s\" "
	     "WHERE left_face = ? OR right_face = ?", tolerance, xtable);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	     "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ToGeoTableGeneralize() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating feature/topology matching via coincident topo-seeds */
    if (!do_eval_topogeo_seeds
	(topo, stmt_ref, ref_geom_col, stmt_ins, stmt_seed_edge, stmt_seed_face,
	 stmt_node, stmt_edge, stmt_face, out_type, tolerance))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_seed_edge);
    sqlite3_finalize (stmt_seed_face);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_seed_edge != NULL)
	sqlite3_finalize (stmt_seed_edge);
    if (stmt_seed_face != NULL)
	sqlite3_finalize (stmt_seed_face);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
do_remove_small_faces2 (struct gaia_topology *topo, sqlite3_int64 edge_id,
			sqlite3_stmt * stmt_rem)
{
/* removing an Edge from a Face (step 2) */
    int ret;
    sqlite3_reset (stmt_rem);
    sqlite3_clear_bindings (stmt_rem);
    sqlite3_bind_int64 (stmt_rem, 1, edge_id);

    ret = sqlite3_step (stmt_rem);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;
    else
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_RemoveSmallFaces error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
      }
    return 0;
}

static int
do_remove_small_faces1 (struct gaia_topology *topo, sqlite3_int64 face_id,
			sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_rem)
{
/* removing the longer Edge from a Face (step 1) */
    int ret;
    int first = 1;
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_int64 (stmt_edge, 1, face_id);
    sqlite3_bind_int64 (stmt_edge, 2, face_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_edge, 0);
		if (first)
		  {
		      first = 0;
		      if (do_remove_small_faces2 (topo, edge_id, stmt_rem))
			  goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_RemoveSmallFaces error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    return 1;

  error:
    return 0;

}

GAIATOPO_DECLARE int
gaiaTopoGeo_RemoveSmallFaces (GaiaTopologyAccessorPtr accessor,
			      double min_circularity, double min_area)
{
/* 
/ attempting to remove all small faces from a Topology-Geometry 
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_rem = NULL;
    sqlite3_stmt *stmt_face = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    int count;
    if (topo == NULL)
	return 0;

/* preparing the SELECT Face query */
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    if (min_circularity < 1.0 && min_area > 0.0)
      {
	  sql = sqlite3_mprintf ("SELECT face_id FROM (SELECT face_id, "
				 "ST_GetFaceGeometry(%Q, face_id) AS geom FROM MAIN.\"%s\" "
				 "WHERE face_id > 0) WHERE Circularity(geom) < %1.12f "
				 "AND ST_Area(geom) < %1.12f",
				 topo->topology_name, xtable, min_circularity,
				 min_area);
      }
    else if (min_circularity >= 1.0 && min_area > 0.0)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT face_id FROM MAIN.\"%s\" WHERE face_id > 0 "
	       "AND ST_Area(ST_GetFaceGeometry(%Q, face_id)) < %1.12f", xtable,
	       topo->topology_name, min_area);
      }
    else if (min_circularity < 1.0 && min_area <= 0.0)
      {
	  sql =
	      sqlite3_mprintf
	      ("SELECT face_id FROM MAIN.\"%s\" WHERE face_id > 0 "
	       "AND Circularity(ST_GetFaceGeometry(%Q, face_id)) < %1.12f",
	       xtable, topo->topology_name, min_circularity);
      }
    else
      {
	  free (xtable);
	  return 0;
      }
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    sql = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveSmallFaces() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT Edge query */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" WHERE right_face = ? "
			 "OR left_face = ? ORDER BY ST_Length(geom) DESC",
			 xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    sql = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveSmallFaces() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the ST_RemEdgeNewFace() query */
    sql =
	sqlite3_mprintf ("SELECT ST_RemEdgeNewFace(%Q, ?)",
			 topo->topology_name);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rem,
			    NULL);
    sqlite3_free (sql);
    sql = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveSmallFaces() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    count = 1;
    while (count)
      {
	  sqlite3_reset (stmt_face);
	  sqlite3_clear_bindings (stmt_face);
	  count = 0;
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt_face);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      sqlite3_int64 face_id =
			  sqlite3_column_int64 (stmt_face, 0);
		      if (do_remove_small_faces1
			  (topo, face_id, stmt_edge, stmt_rem))
			  goto error;
		      count++;
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_RemoveSmallFaces error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
      }

    sqlite3_finalize (stmt_face);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_rem);
    return 1;

  error:
    if (sql != NULL)
	sqlite3_free (sql);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_rem != NULL)
	sqlite3_finalize (stmt_rem);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_RemoveDanglingEdges (GaiaTopologyAccessorPtr accessor)
{
/* 
/ attempting to remove all dangling edges from a Topology-Geometry 
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    if (topo == NULL)
	return 0;

/* preparing the ST_RemEdgeNewFace() query */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT ST_RemEdgeNewFace(%Q, edge_id) FROM MAIN.\"%s\" "
	 "WHERE left_face = right_face", topo->topology_name, xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveDanglingEdges error: \"%s\"",
			       err_msg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (err_msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_RemoveDanglingNodes (GaiaTopologyAccessorPtr accessor)
{
/* 
/ attempting to remove all dangling nodes from a Topology-Geometry 
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    if (topo == NULL)
	return 0;

/* preparing the ST_RemIsoNode() query */
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT ST_RemIsoNode(%Q, node_id) FROM MAIN.\"%s\" "
			 "WHERE containing_face IS NOT NULL",
			 topo->topology_name, xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveDanglingNodes error: \"%s\"",
			       err_msg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (err_msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ToGeoTable (GaiaTopologyAccessorPtr accessor,
			const char *db_prefix, const char *ref_table,
			const char *ref_column, const char *out_table,
			int with_spatial_index)
{
/* attempting to create and populate a new GeoTable out from a Topology-Geometry */
    return gaiaTopoGeo_ToGeoTableGeneralize (accessor, db_prefix, ref_table,
					     ref_column, out_table, -1.0,
					     with_spatial_index);
}

SPATIALITE_PRIVATE int
gaia_do_eval_disjoint (const void *handle, const char *matrix)
{
/* same as ST_Disjoint */
    sqlite3 *sqlite = (sqlite3 *) handle;
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    int value = 0;
    char *sql =
	sqlite3_mprintf ("SELECT ST_RelateMatch(%Q, 'FF*FF****')", matrix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	value = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);
    return value;
}

SPATIALITE_PRIVATE int
gaia_do_eval_overlaps (const void *handle, const char *matrix)
{
/* same as ST_Overlaps */
    sqlite3 *sqlite = (sqlite3 *) handle;
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    int value = 0;
    char *sql = sqlite3_mprintf ("SELECT ST_RelateMatch(%Q, 'T*T***T**') "
				 "OR ST_RelateMatch(%Q, '1*T***T**')", matrix,
				 matrix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	value = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);
    return value;
}

SPATIALITE_PRIVATE int
gaia_do_eval_covers (const void *handle, const char *matrix)
{
/* same as ST_Covers */
    sqlite3 *sqlite = (sqlite3 *) handle;
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    int value = 0;
    char *sql = sqlite3_mprintf ("SELECT ST_RelateMatch(%Q, 'T*****FF*') "
				 "OR ST_RelateMatch(%Q, '*T****FF*') OR ST_RelateMatch(%Q, '***T**FF*') "
				 "OR ST_RelateMatch(%Q, '****T*FF*')", matrix,
				 matrix, matrix, matrix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	value = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);
    return value;
}

SPATIALITE_PRIVATE int
gaia_do_eval_covered_by (const void *handle, const char *matrix)
{
/* same as ST_CoveredBy */
    sqlite3 *sqlite = (sqlite3 *) handle;
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    int value = 0;
    char *sql = sqlite3_mprintf ("SELECT ST_RelateMatch(%Q, 'T*F**F***') "
				 "OR ST_RelateMatch(%Q, '*TF**F***') OR ST_RelateMatch(%Q, '**FT*F***') "
				 "OR ST_RelateMatch(%Q, '**F*TF***')", matrix,
				 matrix, matrix, matrix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
	value = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);
    return value;
}

static int
find_polyface_matches (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
		       sqlite3_stmt * stmt_ins, sqlite3_int64 face_id,
		       sqlite3_int64 containing_face)
{
/* retrieving PolyFace matches */
    int ret;
    int count = 0;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    sqlite3_bind_int64 (stmt_ref, 1, face_id);

    while (1)
      {
	  /* scrolling the result set rows - Spatial Relationships */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_ref, 0);

		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		sqlite3_bind_int64 (stmt_ins, 1, face_id);
		if (containing_face <= 0)
		  {
		      sqlite3_bind_int (stmt_ins, 2, 0);
		      sqlite3_bind_null (stmt_ins, 3);
		  }
		else
		  {
		      sqlite3_bind_int (stmt_ins, 2, 1);
		      sqlite3_bind_int64 (stmt_ins, 3, containing_face);
		  }
		sqlite3_bind_int64 (stmt_ins, 4, rowid);
		/* inserting a row into the output table */
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    count++;
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (count == 0)
      {
	  /* unrelated Face */
	  sqlite3_reset (stmt_ins);
	  sqlite3_clear_bindings (stmt_ins);
	  sqlite3_bind_int64 (stmt_ins, 1, face_id);
	  if (containing_face <= 0)
	    {
		sqlite3_bind_int (stmt_ins, 2, 0);
		sqlite3_bind_null (stmt_ins, 3);
	    }
	  else
	    {
		sqlite3_bind_int (stmt_ins, 2, 1);
		sqlite3_bind_int64 (stmt_ins, 3, containing_face);
	    }
	  sqlite3_bind_null (stmt_ins, 4);
	  /* inserting a row into the output table */
	  ret = sqlite3_step (stmt_ins);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
insert_polyface_reverse (struct gaia_topology *topo, sqlite3_stmt * stmt_ins,
			 sqlite3_int64 polygon_id)
{
/* found a mismatching RefPolygon - inserting into the output table */
    int ret;

    sqlite3_reset (stmt_ins);
    sqlite3_clear_bindings (stmt_ins);
    sqlite3_bind_null (stmt_ins, 1);
    sqlite3_bind_int (stmt_ins, 2, 0);
    sqlite3_bind_null (stmt_ins, 3);
    sqlite3_bind_int64 (stmt_ins, 4, polygon_id);
    /* inserting a row into the output table */
    ret = sqlite3_step (stmt_ins);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

static sqlite3_int64
check_hole_face (struct gaia_topology *topo, sqlite3_stmt * stmt_holes,
		 sqlite3_int64 face_id)
{
/* checking if some Face actually is an "hole" within another Face */
    int ret;
    int count_edges = 0;
    int count_valid = 0;
    sqlite3_int64 containing_face = -1;

    sqlite3_reset (stmt_holes);
    sqlite3_clear_bindings (stmt_holes);
    sqlite3_bind_int64 (stmt_holes, 1, face_id);

    while (1)
      {
	  /* scrolling the result set rows - containing face */
	  ret = sqlite3_step (stmt_holes);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 other_face_id =
		    sqlite3_column_int64 (stmt_holes, 0);
		count_edges++;
		if (containing_face < 0)
		    containing_face = other_face_id;
		if (containing_face == other_face_id)
		    count_valid++;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return -1;
	    }
      }
    if (count_edges == count_valid && count_edges > 0 && containing_face > 0)
	;
    else
	containing_face = -1;

    return containing_face;
}

SPATIALITE_PRIVATE int
gaia_check_spatial_index (const void *handle, const char *db_prefix,
			  const char *ref_table, const char *ref_column)
{
/* testing if the RefTable has an R*Tree Spatial Index */
    sqlite3 *sqlite = (sqlite3 *) handle;
    char *sql;
    char *xprefix;
    int has_rtree = 0;
    char **results;
    int ret;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT spatial_index_enabled FROM \"%s\".geometry_columns "
	 "WHERE f_table_name = %Q AND f_geometry_column = %Q", xprefix,
	 ref_table, ref_column);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    for (i = 1; i <= rows; i++)
      {
	  has_rtree = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    return has_rtree;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_PolyFacesList (GaiaTopologyAccessorPtr accessor,
			   const char *db_prefix, const char *ref_table,
			   const char *ref_column, const char *out_table)
{
/* creating and populating a new Table reporting about Faces/Polygon correspondencies */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_faces = NULL;
    sqlite3_stmt *stmt_holes = NULL;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_rev = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    int ret;
    char *sql;
    char *table;
    char *idx_name;
    char *xtable;
    char *xprefix;
    char *xcolumn;
    char *xidx_name;
    char *rtree_name;
    char *seeds;
    char *xseeds;
    int ref_has_spatial_index = 0;
    if (topo == NULL)
	return 0;

/* attempting to build the output table */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("CREATE TABLE main.\"%s\" (\n"
			   "\tid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tface_id INTEGER,\n"
			   "\tis_hole INTEGER NOT NULL,\n"
			   "\tcontaining_face INTEGER,\n"
			   "\tref_rowid INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    idx_name = sqlite3_mprintf ("idx_%s_face_id", out_table);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sqlite3_free (idx_name);
    xtable = gaiaDoubleQuotedSql (out_table);
    sql =
	sqlite3_mprintf
	("CREATE INDEX main.\"%s\" ON \"%s\" (face_id, ref_rowid)", xidx_name,
	 xtable);
    free (xidx_name);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    idx_name = sqlite3_mprintf ("idx_%s_holes", out_table);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sqlite3_free (idx_name);
    xtable = gaiaDoubleQuotedSql (out_table);
    sql =
	sqlite3_mprintf
	("CREATE INDEX main.\"%s\" ON \"%s\" (containing_face, face_id)",
	 xidx_name, xtable);
    free (xidx_name);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Faces SQL statement */
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT face_id FROM main.\"%s\" WHERE face_id > 0",
			 xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_faces,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the IsHole SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT left_face AS other_face FROM main.\"%s\" "
			   "WHERE right_face = ? UNION "
			   "SELECT right_face AS other_face FROM main.\"%s\" WHERE left_face = ?",
			   xtable, xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_holes,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the RefTable SQL statement */
    seeds = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix, ref_table);
    ref_has_spatial_index =
	gaia_check_spatial_index (topo->db_handle, db_prefix, ref_table,
				  ref_column);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    xcolumn = gaiaDoubleQuotedSql (ref_column);
    xseeds = gaiaDoubleQuotedSql (seeds);
    if (ref_has_spatial_index)
	sql =
	    sqlite3_mprintf
	    ("SELECT r.rowid FROM MAIN.\"%s\" AS s, \"%s\".\"%s\" AS r "
	     "WHERE ST_Intersects(r.\"%s\", s.geom) == 1 AND s.face_id = ? "
	     "AND r.rowid IN (SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q "
	     "AND f_geometry_column = %Q AND search_frame = s.geom)", xseeds,
	     xprefix, xtable, xcolumn, rtree_name, xcolumn);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT r.rowid FROM MAIN.\"%s\" AS s, \"%s\".\"%s\" AS r "
	     "WHERE  ST_Intersects(r.\"%s\", s.geom) == 1 AND s.face_id = ?",
	     xseeds, xprefix, xtable, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    free (xseeds);
    sqlite3_free (rtree_name);
    sqlite3_free (seeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_ref,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Reverse RefTable SQL statement */
    seeds = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    rtree_name =
	sqlite3_mprintf ("DB=%s.%s_seeds", db_prefix, topo->topology_name);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    xcolumn = gaiaDoubleQuotedSql (ref_column);
    xseeds = gaiaDoubleQuotedSql (seeds);
    sql = sqlite3_mprintf ("SELECT r.rowid FROM \"%s\".\"%s\" AS r "
			   "LEFT JOIN MAIN.\"%s\" AS s ON (ST_Intersects(r.\"%s\", s.geom) = 1 "
			   "AND s.face_id IS NOT NULL AND s.rowid IN (SELECT rowid FROM SpatialIndex "
			   "WHERE f_table_name = %Q AND search_frame = r.\"%s\")) "
			   "WHERE s.face_id IS NULL", xprefix, xtable, xseeds,
			   xcolumn, rtree_name, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    free (xseeds);
    sqlite3_free (rtree_name);
    sqlite3_free (seeds);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rev,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Insert SQL statement */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (id, face_id, is_hole, "
			   "containing_face, ref_rowid) "
			   "VALUES (NULL, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_ins,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - Faces */
	  ret = sqlite3_step (stmt_faces);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_faces, 0);
		sqlite3_int64 containing_face =
		    check_hole_face (topo, stmt_holes, face_id);
		if (!find_polyface_matches
		    (topo, stmt_ref, stmt_ins, face_id, containing_face))
		    goto error;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    while (1)
      {
	  /* scrolling the Reverse result set rows - Polygons */
	  ret = sqlite3_step (stmt_rev);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 polyg_id = sqlite3_column_int64 (stmt_rev, 0);
		if (!insert_polyface_reverse (topo, stmt_ins, polyg_id))
		    goto error;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_faces);
    sqlite3_finalize (stmt_holes);
    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_rev);
    sqlite3_finalize (stmt_ins);
    return 1;

  error:
    if (stmt_faces != NULL)
	sqlite3_finalize (stmt_faces);
    if (stmt_holes != NULL)
	sqlite3_finalize (stmt_holes);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_rev != NULL)
	sqlite3_finalize (stmt_rev);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    return 0;
}

static int
do_find_matching_point (gaiaLinestringPtr ln1, int *idx1, gaiaLinestringPtr ln2,
			int *idx2)
{
/* searching for a common point in both Linestrings */
    int i1;
    int i2;
    double x1;
    double y1;
    double z1;
    double m1;
    double x2;
    double y2;
    double z2;
    double m2;
    for (i1 = 0; i1 < ln1->Points; i1++)
      {
	  /* extracting a Vertex from the first Linestring */
	  z1 = 0.0;
	  m1 = 0.0;
	  if (ln1->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln1->Coords, i1, &x1, &y1, &z1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln1->Coords, i1, &x1, &y1, &m1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln1->Coords, i1, &x1, &y1, &z1, &m1);
	    }
	  else
	    {
		gaiaGetPoint (ln1->Coords, i1, &x1, &y1);
	    }
	  for (i2 = 0; i2 < ln2->Points; i2++)
	    {
		/* extracting a Vertex from the second Linestring */
		z2 = 0.0;
		m2 = 0.0;
		if (ln2->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln2->Coords, i2, &x2, &y2, &z2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln2->Coords, i2, &x2, &y2, &m2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln2->Coords, i2, &x2, &y2, &z2, &m2);
		  }
		else
		  {
		      gaiaGetPoint (ln2->Coords, i2, &x2, &y2);
		  }
		if (x1 == x2 && y1 == y2 && z1 == z2 && m1 == m2)
		  {
		      *idx1 = i1;
		      *idx2 = i2;
		      return 1;
		  }
	    }
      }
    *idx1 = -1;
    *idx2 = -1;
    return 0;
}

static int
do_check_forward (gaiaLinestringPtr ln1, int idx1, gaiaLinestringPtr ln2,
		  int idx2)
{
/* testing for further matching Vertices (same directions) */
    int i1;
    int i2;
    double x1;
    double y1;
    double z1;
    double m1;
    double x2;
    double y2;
    double z2;
    double m2;
    int count = 0;
    for (i1 = idx1; i1 < ln1->Points; i1++)
      {
	  /* extracting a Vertex from the first Linestring */
	  z1 = 0.0;
	  m1 = 0.0;
	  if (ln1->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln1->Coords, i1, &x1, &y1, &z1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln1->Coords, i1, &x1, &y1, &m1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln1->Coords, i1, &x1, &y1, &z1, &m1);
	    }
	  else
	    {
		gaiaGetPoint (ln1->Coords, i1, &x1, &y1);
	    }
	  for (i2 = idx2; i2 < ln2->Points; i2++)
	    {
		/* extracting a Vertex from the second Linestring */
		z2 = 0.0;
		m2 = 0.0;
		if (ln2->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln2->Coords, i2, &x2, &y2, &z2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln2->Coords, i2, &x2, &y2, &m2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln2->Coords, i2, &x2, &y2, &z2, &m2);
		  }
		else
		  {
		      gaiaGetPoint (ln2->Coords, i2, &x2, &y2);
		  }
		if (x1 == x2 && y1 == y2 && z1 == z2 && m1 == m2)
		  {
		      idx2++;
		      count++;
		      break;
		  }
	    }
      }
    if (count >= 2)
	return 1;
    return 0;
}

static int
do_check_backward (gaiaLinestringPtr ln1, int idx1, gaiaLinestringPtr ln2,
		   int idx2)
{
/* testing for further matching Vertices (opposite directions) */
    int i1;
    int i2;
    double x1;
    double y1;
    double z1;
    double m1;
    double x2;
    double y2;
    double z2;
    double m2;
    int count = 0;
    for (i1 = idx1; i1 < ln1->Points; i1++)
      {
	  /* extracting a Vertex from the first Linestring */
	  z1 = 0.0;
	  m1 = 0.0;
	  if (ln1->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln1->Coords, i1, &x1, &y1, &z1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln1->Coords, i1, &x1, &y1, &m1);
	    }
	  else if (ln1->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln1->Coords, i1, &x1, &y1, &z1, &m1);
	    }
	  else
	    {
		gaiaGetPoint (ln1->Coords, i1, &x1, &y1);
	    }
	  for (i2 = idx2; i2 >= 0; i2--)
	    {
		/* extracting a Vertex from the second Linestring */
		z2 = 0.0;
		m2 = 0.0;
		if (ln2->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (ln2->Coords, i2, &x2, &y2, &z2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (ln2->Coords, i2, &x2, &y2, &m2);
		  }
		else if (ln2->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (ln2->Coords, i2, &x2, &y2, &z2, &m2);
		  }
		else
		  {
		      gaiaGetPoint (ln2->Coords, i2, &x2, &y2);
		  }
		if (x1 == x2 && y1 == y2 && z1 == z2 && m1 == m2)
		  {
		      idx2--;
		      count++;
		      break;
		  }
	    }
      }
    if (count >= 2)
	return 1;
    return 0;
}

SPATIALITE_PRIVATE void
gaia_do_check_direction (const void *x1, const void *x2, char *direction)
{
/* checking if two Linestrings have the same direction */
    gaiaGeomCollPtr g1 = (gaiaGeomCollPtr) x1;
    gaiaGeomCollPtr g2 = (gaiaGeomCollPtr) x2;
    int idx1;
    int idx2;
    gaiaLinestringPtr ln1 = g1->FirstLinestring;
    gaiaLinestringPtr ln2 = g2->FirstLinestring;
    while (ln2 != NULL)
      {
	  /* the second Geometry could be a MultiLinestring */
	  if (do_find_matching_point (ln1, &idx1, ln2, &idx2))
	    {
		if (do_check_forward (ln1, idx1, ln2, idx2))
		  {
		      /* ok, same directions */
		      *direction = '+';
		      return;
		  }
		if (do_check_backward (ln1, idx1, ln2, idx2))
		  {
		      /* ok, opposite directions */
		      *direction = '-';
		      return;
		  }
	    }
	  ln2 = ln2->Next;
      }
    *direction = '?';
}

static int
find_lineedge_relationships (struct gaia_topology *topo,
			     sqlite3_stmt * stmt_ref, sqlite3_stmt * stmt_ins,
			     sqlite3_int64 edge_id, const unsigned char *blob,
			     int blob_sz)
{
/* retrieving LineEdge relationships */
    int ret;
    int count = 0;
    char direction[2];
    strcpy (direction, "?");

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    sqlite3_bind_blob (stmt_ref, 1, blob, blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_ref, 2, blob, blob_sz, SQLITE_STATIC);

    while (1)
      {
	  /* scrolling the result set rows - Spatial Relationships */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int overlaps = 0;
		int covers = 0;
		int covered_by = 0;
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_ref, 0);
		const char *matrix =
		    (const char *) sqlite3_column_text (stmt_ref, 1);
		if (gaia_do_eval_disjoint (topo->db_handle, matrix))
		    continue;
		overlaps = gaia_do_eval_overlaps (topo->db_handle, matrix);
		covers = gaia_do_eval_covers (topo->db_handle, matrix);
		covered_by = gaia_do_eval_covered_by (topo->db_handle, matrix);
		if (!overlaps && !covers && !covered_by)
		    continue;

		if (sqlite3_column_type (stmt_ref, 2) == SQLITE_BLOB)
		  {
		      /* testing directions */
		      gaiaGeomCollPtr geom_edge = NULL;
		      gaiaGeomCollPtr geom_line = NULL;
		      const unsigned char *blob2 =
			  sqlite3_column_blob (stmt_ref, 2);
		      int blob2_sz = sqlite3_column_bytes (stmt_ref, 2);
		      geom_edge = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      geom_line = gaiaFromSpatiaLiteBlobWkb (blob2, blob2_sz);
		      if (geom_edge != NULL && geom_line != NULL)
			  gaia_do_check_direction (geom_edge, geom_line,
						   direction);
		      if (geom_edge != NULL)
			  gaiaFreeGeomColl (geom_edge);
		      if (geom_line != NULL)
			  gaiaFreeGeomColl (geom_line);
		  }

		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		sqlite3_bind_int64 (stmt_ins, 1, edge_id);
		sqlite3_bind_int64 (stmt_ins, 2, rowid);
		sqlite3_bind_text (stmt_ins, 3, direction, 1, SQLITE_STATIC);
		sqlite3_bind_text (stmt_ins, 4, matrix, strlen (matrix),
				   SQLITE_STATIC);
		sqlite3_bind_int (stmt_ins, 5, overlaps);
		sqlite3_bind_int (stmt_ins, 6, covers);
		sqlite3_bind_int (stmt_ins, 7, covered_by);
		/* inserting a row into the output table */
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    count++;
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("LineEdgesList error: \"%s\"",
					   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (count == 0)
      {
	  /* unrelated Edge */
	  sqlite3_reset (stmt_ins);
	  sqlite3_clear_bindings (stmt_ins);
	  sqlite3_bind_int64 (stmt_ins, 1, edge_id);
	  sqlite3_bind_null (stmt_ins, 2);
	  sqlite3_bind_null (stmt_ins, 3);
	  sqlite3_bind_null (stmt_ins, 4);
	  sqlite3_bind_null (stmt_ins, 5);
	  sqlite3_bind_null (stmt_ins, 6);
	  sqlite3_bind_null (stmt_ins, 7);
	  /* inserting a row into the output table */
	  ret = sqlite3_step (stmt_ins);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_LineEdgesList (GaiaTopologyAccessorPtr accessor,
			   const char *db_prefix, const char *ref_table,
			   const char *ref_column, const char *out_table)
{
/* creating and populating a new Table reporting about Edges/Linestring correspondencies */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_edges = NULL;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    int ret;
    char *sql;
    char *table;
    char *idx_name;
    char *xtable;
    char *xprefix;
    char *xcolumn;
    char *xidx_name;
    char *rtree_name;
    int ref_has_spatial_index = 0;
    if (topo == NULL)
	return 0;

/* attempting to build the output table */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("CREATE TABLE main.\"%s\" (\n"
			   "\tid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tedge_id INTEGER NOT NULL,\n"
			   "\tref_rowid INTEGER,\n"
			   "\tdirection TEXT,\n"
			   "\tmatrix TEXT,\n"
			   "\toverlaps INTEGER,\n"
			   "\tcovers INTEGER,\n"
			   "\tcovered_by INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    idx_name = sqlite3_mprintf ("idx_%s_edge_id", out_table);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sqlite3_free (idx_name);
    xtable = gaiaDoubleQuotedSql (out_table);
    sql =
	sqlite3_mprintf
	("CREATE INDEX main.\"%s\" ON \"%s\" (edge_id, ref_rowid)", xidx_name,
	 xtable);
    free (xidx_name);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Edges SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT edge_id, geom FROM main.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edges,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the RefTable SQL statement */
    rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix, ref_table);
    ref_has_spatial_index =
	gaia_check_spatial_index (topo->db_handle, db_prefix, ref_table,
				  ref_column);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    xcolumn = gaiaDoubleQuotedSql (ref_column);
    if (ref_has_spatial_index)
	sql =
	    sqlite3_mprintf
	    ("SELECT rowid, ST_Relate(?, \"%s\"), \"%s\" FROM \"%s\".\"%s\" "
	     "WHERE  rowid IN ("
	     "SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
	     "f_geometry_column = %Q AND search_frame = ?)", xcolumn, xcolumn,
	     xprefix, xtable, rtree_name, ref_column);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT rowid, ST_Relate(?, \"%s\"), \"%s\"  FROM \"%s\".\"%s\" "
	     "WHERE MbrIntersects(?, \"%s\")", xcolumn, xcolumn, xprefix,
	     xtable, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    sqlite3_free (rtree_name);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_ref,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Insert SQL statement */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (id, edge_id, ref_rowid, "
			   "direction, matrix, overlaps, covers, covered_by) "
			   "VALUES (NULL, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_ins,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - Edges */
	  ret = sqlite3_step (stmt_edges);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_edges, 0);
		if (sqlite3_column_type (stmt_edges, 1) == SQLITE_BLOB)
		  {
		      if (!find_lineedge_relationships
			  (topo, stmt_ref, stmt_ins, edge_id,
			   sqlite3_column_blob (stmt_edges, 1),
			   sqlite3_column_bytes (stmt_edges, 1)))
			  goto error;
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("LineEdgesList error: Edge not a BLOB value");
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("LineEdgesList error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_edges);
    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    return 1;

  error:
    if (stmt_edges != NULL)
	sqlite3_finalize (stmt_edges);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    return 0;
}

static struct face_item *
create_face_item (sqlite3_int64 face_id)
{
/* creating a Face Item */
    struct face_item *item = malloc (sizeof (struct face_item));
    item->face_id = face_id;
    item->next = NULL;
    return item;
}

static void
destroy_face_item (struct face_item *item)
{
/* destroying a Face Item */
    if (item == NULL)
	return;
    free (item);
}

static struct face_edge_item *
create_face_edge_item (sqlite3_int64 edge_id, sqlite3_int64 left_face,
		       sqlite3_int64 right_face, gaiaGeomCollPtr geom)
{
/* creating a Face-Edge Item */
    struct face_edge_item *item = malloc (sizeof (struct face_edge_item));
    item->edge_id = edge_id;
    item->left_face = left_face;
    item->right_face = right_face;
    item->geom = geom;
    item->count = 0;
    item->next = NULL;
    return item;
}

static void
destroy_face_edge_item (struct face_edge_item *item)
{
/* destroying a Face-Edge Item */
    if (item == NULL)
	return;
    if (item->geom != NULL)
	gaiaFreeGeomColl (item->geom);
    free (item);
}

TOPOLOGY_PRIVATE struct face_edges *
auxtopo_create_face_edges (int has_z, int srid)
{
/* creating an empty Face-Edges list */
    struct face_edges *list = malloc (sizeof (struct face_edges));
    list->has_z = has_z;
    list->srid = srid;
    list->first_edge = NULL;
    list->last_edge = NULL;
    list->first_face = NULL;
    list->last_face = NULL;
    return list;
}

TOPOLOGY_PRIVATE void
auxtopo_free_face_edges (struct face_edges *list)
{
/* destroying a Face-Edges list */
    struct face_edge_item *fe;
    struct face_edge_item *fen;
    struct face_item *f;
    struct face_item *fn;
    if (list == NULL)
	return;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  /* destroying all Face-Edge items */
	  fen = fe->next;
	  destroy_face_edge_item (fe);
	  fe = fen;
      }
    f = list->first_face;
    while (f != NULL)
      {
	  /* destroying all Face items */
	  fn = f->next;
	  destroy_face_item (f);
	  f = fn;
      }
    free (list);
}

TOPOLOGY_PRIVATE void
auxtopo_add_face_edge (struct face_edges *list, sqlite3_int64 face_id,
		       sqlite3_int64 edge_id, sqlite3_int64 left_face,
		       sqlite3_int64 right_face, gaiaGeomCollPtr geom)
{
/* adding a Face-Edge Item into the list */
    struct face_item *f;
    struct face_edge_item *fe =
	create_face_edge_item (edge_id, left_face, right_face, geom);
    if (list->first_edge == NULL)
	list->first_edge = fe;
    if (list->last_edge != NULL)
	list->last_edge->next = fe;
    list->last_edge = fe;

    f = list->first_face;
    while (f != NULL)
      {
	  if (f->face_id == face_id)
	      return;
	  f = f->next;
      }

    /* inserting the Face-ID into the list */
    f = create_face_item (face_id);
    if (list->first_face == NULL)
	list->first_face = f;
    if (list->last_face != NULL)
	list->last_face->next = f;
    list->last_face = f;
}

TOPOLOGY_PRIVATE void
auxtopo_select_valid_face_edges (struct face_edges *list)
{
/* identifying all useless Edges */
    struct face_edge_item *fe = list->first_edge;
    while (fe != NULL)
      {
	  struct face_item *f = list->first_face;
	  while (f != NULL)
	    {
		if (f->face_id == fe->left_face)
		    fe->count += 1;
		if (f->face_id == fe->right_face)
		    fe->count += 1;
		f = f->next;
	    }
	  fe = fe->next;
      }
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_polygonize_face_edges (struct face_edges *list, const void *cache)
{
/* attempting to reaggregrate Polygons from valid Edges */
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr rearranged;
    struct face_edge_item *fe;

    if (list->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = list->srid;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  if (fe->count < 2)
	    {
		/* found a valid Edge: adding to the MultiListring */
		gaiaLinestringPtr ln = fe->geom->FirstLinestring;
		while (ln != NULL)
		  {
		      if (list->has_z)
			  auxtopo_copy_linestring3d (ln, sparse);
		      else
			  auxtopo_copy_linestring (ln, sparse);
		      ln = ln->Next;
		  }
	    }
	  fe = fe->next;
      }
    rearranged = gaiaPolygonize_r (cache, sparse, 0);
    gaiaFreeGeomColl (sparse);
    return rearranged;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_polygonize_face_edges_generalize (struct face_edges * list,
					  const void *cache)
{
/* attempting to reaggregrate Polygons from valid Edges */
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr renoded;
    gaiaGeomCollPtr rearranged;
    struct face_edge_item *fe;

    if (list->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = list->srid;

    fe = list->first_edge;
    while (fe != NULL)
      {
	  if (fe->count < 2)
	    {
		/* found a valid Edge: adding to the MultiListring */
		gaiaLinestringPtr ln = fe->geom->FirstLinestring;
		while (ln != NULL)
		  {
		      if (list->has_z)
			  auxtopo_copy_linestring3d (ln, sparse);
		      else
			  auxtopo_copy_linestring (ln, sparse);
		      ln = ln->Next;
		  }
	    }
	  fe = fe->next;
      }
    renoded = gaiaNodeLines (cache, sparse);
    gaiaFreeGeomColl (sparse);
    if (renoded == NULL)
	return NULL;
    rearranged = gaiaPolygonize_r (cache, renoded, 0);
    gaiaFreeGeomColl (renoded);
    return rearranged;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_make_geom_from_point (int srid, int has_z, gaiaPointPtr pt)
{
/* quick constructor: Geometry based on external point */
    gaiaGeomCollPtr reference;
    if (has_z)
	reference = gaiaAllocGeomCollXYZ ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    pt->Next = NULL;
    reference->FirstPoint = pt;
    reference->LastPoint = pt;
    return reference;
}

TOPOLOGY_PRIVATE gaiaGeomCollPtr
auxtopo_make_geom_from_line (int srid, gaiaLinestringPtr ln)
{
/* quick constructor: Geometry based on external line */
    gaiaGeomCollPtr reference;
    if (ln->DimensionModel == GAIA_XY_Z_M)
	reference = gaiaAllocGeomCollXYZM ();
    else if (ln->DimensionModel == GAIA_XY_Z)
	reference = gaiaAllocGeomCollXYZ ();
    else if (ln->DimensionModel == GAIA_XY_M)
	reference = gaiaAllocGeomCollXYM ();
    else
	reference = gaiaAllocGeomColl ();
    reference->Srid = srid;
    ln->Next = NULL;
    reference->FirstLinestring = ln;
    reference->LastLinestring = ln;
    return reference;
}

TOPOLOGY_PRIVATE void
auxtopo_copy_linestring3d (gaiaLinestringPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Linestring 3D into another Geometry */
    int iv;
    double x;
    double y;
    double z;
    gaiaLinestringPtr out = gaiaAddLinestringToGeomColl (geom, in->Points);
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	  gaiaSetPointXYZ (out->Coords, iv, x, y, z);
      }
}

TOPOLOGY_PRIVATE void
auxtopo_copy_linestring (gaiaLinestringPtr in, gaiaGeomCollPtr geom)
{
/* inserting/copying a Linestring into another Geometry */
    int iv;
    double x;
    double y;
    gaiaLinestringPtr out = gaiaAddLinestringToGeomColl (geom, in->Points);
    for (iv = 0; iv < in->Points; iv++)
      {
	  gaiaGetPoint (in->Coords, iv, &x, &y);
	  gaiaSetPoint (out->Coords, iv, x, y);
      }
}

TOPOLOGY_PRIVATE void
auxtopo_destroy_geom_from (gaiaGeomCollPtr reference)
{
/* safely destroying a reference geometry */
    if (reference == NULL)
	return;

/* releasing ownership on external points, lines and polygs */
    reference->FirstPoint = NULL;
    reference->LastPoint = NULL;
    reference->FirstLinestring = NULL;
    reference->LastLinestring = NULL;
    reference->FirstPolygon = NULL;
    reference->LastPolygon = NULL;

    gaiaFreeGeomColl (reference);
}

TOPOLOGY_PRIVATE int
auxtopo_retrieve_geometry_type (sqlite3 * db_handle, const char *db_prefix,
				const char *table, const char *column,
				int *ref_type)
{
/* attempting to retrive the reference Geometry Type */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int type = -1;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT geometry_type "
	 "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	 "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret =
	sqlite3_get_table (db_handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  type = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (type < 0)
	return 0;

    *ref_type = type;
    return 1;
}

TOPOLOGY_PRIVATE int
auxtopo_create_togeotable_sql (sqlite3 * db_handle, const char *db_prefix,
			       const char *ref_table, const char *ref_column,
			       const char *out_table, char **xcreate,
			       char **xselect, char **xinsert,
			       int *ref_geom_col)
{
/* composing the CREATE TABLE output-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *xprefix;
    char *xtable;
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int pk_no;
    int ret;
    int first_create = 1;
    int first_select = 1;
    int first_insert = 1;
    int npk = 0;
    int ipk;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    int xref_geom_col;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;
    *ref_geom_col = -1;

    xtable = gaiaDoubleQuotedSql (out_table);
    create = sqlite3_mprintf ("CREATE TABLE MAIN.\"%s\" (", xtable);
    select = sqlite3_mprintf ("SELECT ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (", xtable);
    free (xtable);

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		/* counting how many PK columns are there */
		if (atoi (results[(i * columns) + 5]) != 0)
		    npk++;
	    }
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		pk_no = atoi (results[(i * columns) + 5]);
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		if (strcasecmp (name, ref_column) == 0)
		  {
		      /* saving the index of ref-geometry */
		      xref_geom_col = ref_col;
		  }
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		if (strcasecmp (name, ref_column) == 0)
		  {
		      /* skipping the geometry column */
		      continue;
		  }
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (first_create)
		  {
		      first_create = 0;
		      if (notnull)
			  create =
			      sqlite3_mprintf ("%s\n\t\"%s\" %s NOT NULL", prev,
					       xprefix, type);
		      else
			  create =
			      sqlite3_mprintf ("%s\n\t\"%s\" %s", prev, xprefix,
					       type);
		  }
		else
		  {
		      if (notnull)
			  create =
			      sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					       prev, xprefix, type);
		      else
			  create =
			      sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					       xprefix, type);
		  }
		free (xprefix);
		sqlite3_free (prev);
		if (npk == 1 && pk_no != 0)
		  {
		      /* declaring a single-column Primary Key */
		      prev = create;
		      create = sqlite3_mprintf ("%s PRIMARY KEY", prev);
		      sqlite3_free (prev);
		  }
	    }
	  if (npk > 1)
	    {
		/* declaring a multi-column Primary Key */
		prev = create;
		sql = sqlite3_mprintf ("pk_%s", out_table);
		xprefix = gaiaDoubleQuotedSql (sql);
		sqlite3_free (sql);
		create =
		    sqlite3_mprintf ("%s,\n\tCONSTRAINT \"%s\" PRIMARY KEY (",
				     prev, xprefix);
		free (xprefix);
		sqlite3_free (prev);
		for (ipk = 1; ipk <= npk; ipk++)
		  {
		      /* searching a Primary Key column */
		      for (i = 1; i <= rows; i++)
			{
			    if (atoi (results[(i * columns) + 5]) == ipk)
			      {
				  /* declaring a Primary Key column */
				  name = results[(i * columns) + 1];
				  xprefix = gaiaDoubleQuotedSql (name);
				  prev = create;
				  if (ipk == 1)
				      create =
					  sqlite3_mprintf ("%s\"%s\"", prev,
							   xprefix);
				  else
				      create =
					  sqlite3_mprintf ("%s, \"%s\"", prev,
							   xprefix);
				  free (xprefix);
				  sqlite3_free (prev);
			      }
			}
		  }
		prev = create;
		create = sqlite3_mprintf ("%s)", prev);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    select = sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s) VALUES (", prev);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    *ref_geom_col = xref_geom_col;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
is_geometry_column (sqlite3 * db_handle, const char *db_prefix,
		    const char *table, const char *column)
{
/* testing for Geometry columns */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int count = 0;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) "
	 "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	 "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret =
	sqlite3_get_table (db_handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  count = atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);

    if (count > 0)
	return 1;
    return 0;
}

static int
auxtopo_create_features_sql (sqlite3 * db_handle, const char *db_prefix,
			     const char *ref_table, const char *ref_column,
			     const char *topology_name,
			     sqlite3_int64 topolayer_id, char **xcreate,
			     char **xselect, char **xinsert)
{
/* composing the CREATE TABLE fatures-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *xprefix;
    char *xgeom;
    char *table;
    char *xtable;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;

    sprintf (dummy, "%lld", topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    create =
	sqlite3_mprintf
	("CREATE TABLE MAIN.\"%s\" (\n\tfid INTEGER PRIMARY KEY AUTOINCREMENT",
	 xtable);
    select = sqlite3_mprintf ("SELECT ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (", xtable);
    free (xtable);

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (db_handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		if (strcasecmp (name, "fid") == 0)
		    continue;
		if (is_geometry_column (db_handle, db_prefix, ref_table, name))
		    continue;
		if (ref_column != NULL)
		  {
		      if (strcasecmp (ref_column, name) == 0)
			  continue;
		  }
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (notnull)
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					 prev, xprefix, type);
		else
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					 xprefix, type);
		free (xprefix);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    if (ref_column != NULL)
      {
	  xgeom = gaiaDoubleQuotedSql (ref_column);
	  select =
	      sqlite3_mprintf ("%s, \"%s\" FROM \"%s\".\"%s\"", prev, xgeom,
			       xprefix, xtable);
	  free (xgeom);
      }
    else
	select =
	    sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s) VALUES (", prev);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
do_register_topolayer (struct gaia_topology *topo, const char *topolayer_name,
		       sqlite3_int64 * topolayer_id)
{
/* attempting to register a new TopoLayer */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    char *err_msg = NULL;

    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (topolayer_name) VALUES (Lower(%Q))", xtable,
	 topolayer_name);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("RegisterTopoLayer error: \"%s\"", err_msg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (err_msg);
	  sqlite3_free (msg);
	  return 0;
      }

    *topolayer_id = sqlite3_last_insert_rowid (topo->db_handle);
    return 1;
}

static int
do_eval_topolayer_point (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			 sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_rels,
			 sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Points from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Node query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_blob (stmt_node, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_node, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_node, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_int64 (stmt_rels, 1, node_id);
		sqlite3_bind_null (stmt_rels, 2);
		sqlite3_bind_null (stmt_rels, 3);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_line (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			sqlite3_stmt * stmt_edge, sqlite3_stmt * stmt_rels,
			sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Linestrings from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Edge query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_blob (stmt_edge, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_edge, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_edge, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_null (stmt_rels, 1);
		sqlite3_bind_int64 (stmt_rels, 2, edge_id);
		sqlite3_bind_null (stmt_rels, 3);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_polyg (struct gaia_topology *topo, gaiaGeomCollPtr reference,
			 sqlite3_stmt * stmt_face, sqlite3_stmt * stmt_rels,
			 sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Polygon from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;

/* initializing the Topo-Face query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_face);
    sqlite3_clear_bindings (stmt_face);
    sqlite3_bind_blob (stmt_face, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_face, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_face);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_face, 0);
		sqlite3_reset (stmt_rels);
		sqlite3_clear_bindings (stmt_rels);
		sqlite3_bind_null (stmt_rels, 1);
		sqlite3_bind_null (stmt_rels, 2);
		sqlite3_bind_int64 (stmt_rels, 3, face_id);
		sqlite3_bind_int64 (stmt_rels, 4, topolayer_id);
		sqlite3_bind_int64 (stmt_rels, 5, fid);
		ret = sqlite3_step (stmt_rels);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
do_eval_topolayer_geom (struct gaia_topology *topo, gaiaGeomCollPtr geom,
			sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			sqlite3_stmt * stmt_face, sqlite3_stmt * stmt_rels,
			sqlite3_int64 topolayer_id, sqlite3_int64 fid)
{
/* retrieving Features via matching Seeds/Geometries */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    int ret;

    /* processing all Points */
    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  gaiaPointPtr next = pt->Next;
	  gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
	      auxtopo_make_geom_from_point (topo->srid, topo->has_z, pt);
	  ret =
	      do_eval_topolayer_point (topo, reference, stmt_node, stmt_rels,
				       topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  pt->Next = next;
	  if (ret == 0)
	      return 0;
	  pt = pt->Next;
      }

    /* processing all Linestrings */
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  gaiaLinestringPtr next = ln->Next;
	  gaiaGeomCollPtr reference = (gaiaGeomCollPtr)
	      auxtopo_make_geom_from_line (topo->srid, ln);
	  ret =
	      do_eval_topolayer_line (topo, reference, stmt_edge, stmt_rels,
				      topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  ln->Next = next;
	  if (ret == 0)
	      return 0;
	  ln = ln->Next;
      }

    /* processing all Polygons */
    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  gaiaPolygonPtr next = pg->Next;
	  gaiaGeomCollPtr reference = make_geom_from_polyg (topo->srid, pg);
	  ret = do_eval_topolayer_polyg (topo, reference, stmt_face, stmt_rels,
					 topolayer_id, fid);
	  auxtopo_destroy_geom_from (reference);
	  pg->Next = next;
	  if (ret == 0)
	      return 0;
	  pg = pg->Next;
      }

    return 1;
}

static int
do_eval_topolayer_seeds (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
			 sqlite3_stmt * stmt_ins, sqlite3_stmt * stmt_rels,
			 sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			 sqlite3_stmt * stmt_face, sqlite3_int64 topolayer_id)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  sqlite3_int64 fid;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      if (icol == ncol - 1)
			{
			    /* the last column is always expected to be the geometry column */
			    const unsigned char *blob =
				sqlite3_column_blob (stmt_ref, icol);
			    int blob_sz = sqlite3_column_bytes (stmt_ref, icol);
			    geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    continue;
			}
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
		fid = sqlite3_last_insert_rowid (topo->db_handle);
		/* evaluating the feature's geometry */
		if (geom != NULL)
		  {
		      ret =
			  do_eval_topolayer_geom (topo, geom, stmt_node,
						  stmt_edge, stmt_face,
						  stmt_rels, topolayer_id, fid);
		      gaiaFreeGeomColl (geom);
		      if (ret == 0)
			  return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_CreateTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *db_prefix, const char *ref_table,
			     const char *ref_column, const char *topolayer_name)
{
/* attempting to create a TopoLayer */
    sqlite3_int64 topolayer_id;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *sql;
    char *create;
    char *select;
    char *insert;
    char *xprefix;
    char *table;
    char *xtable;
    char *errMsg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* attempting to register a new TopoLayer */
    if (!do_register_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* incrementally updating all Topology Seeds */
    if (!gaiaTopoGeoUpdateSeeds (accessor, 1))
	return 0;

/* composing the CREATE TABLE feature-table statement */
    if (!auxtopo_create_features_sql
	(topo->db_handle, db_prefix, ref_table, ref_column, topo->topology_name,
	 topolayer_id, &create, &select, &insert))
	goto error;

/* creating the feature-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-ref" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (node_id, edge_id, face_id, topolayer_id, fid) "
	 "VALUES (?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    sql = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Edges query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" "
			   "WHERE edge_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Faces query */
    xprefix = sqlite3_mprintf ("%s_seeds", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT face_id FROM MAIN.\"%s\" "
			   "WHERE face_id IS NOT NULL AND ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geom, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating feature/topology matching via coincident topo-seeds */
    if (!do_eval_topolayer_seeds
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
do_populate_topolayer (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
		       sqlite3_stmt * stmt_ins)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_InitTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_InitTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_InitTopoLayer (GaiaTopologyAccessorPtr accessor,
			   const char *db_prefix, const char *ref_table,
			   const char *topolayer_name)
{
/* attempting to create a TopoLayer */
    sqlite3_int64 topolayer_id;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    int ret;
    char *create;
    char *select;
    char *insert;
    char *errMsg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* attempting to register a new TopoLayer */
    if (!do_register_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* composing the CREATE TABLE feature-table statement */
    if (!auxtopo_create_features_sql
	(topo->db_handle, db_prefix, ref_table, NULL, topo->topology_name,
	 topolayer_id, &create, &select, &insert))
	goto error;

/* creating the feature-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_InitTopoLayer() error: \"%s\"",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* populating the TopoFeatures table */
    if (!do_populate_topolayer (topo, stmt_ref, stmt_ins))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    return 0;
}

static int
check_topolayer (struct gaia_topology *topo, const char *topolayer_name,
		 sqlite3_int64 * topolayer_id)
{
/* checking if a TopoLayer do really exist */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    int found = 0;
    sqlite3_stmt *stmt = NULL;

/* creating the SQL statement - SELECT */
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT topolayer_id FROM \"%s\" WHERE topolayer_name = Lower(%Q)",
	 xtable, topolayer_name);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Check_TopoLayer() error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* retrieving the TopoLayer ID */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*topolayer_id = sqlite3_column_int64 (stmt, 0);
		found = 1;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("Check_TopoLayer() error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    if (!found)
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_unregister_topolayer (struct gaia_topology *topo, const char *topolayer_name,
			 sqlite3_int64 * topolayer_id)
{
/* attempting to unregister a existing TopoLayer */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_int64 id;
    sqlite3_stmt *stmt = NULL;

    if (!check_topolayer (topo, topolayer_name, &id))
	return 0;

/* creating the first SQL statement - DELETE */
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE topolayer_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* deleting the TopoLayer */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    *topolayer_id = id;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_RemoveTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *topolayer_name)
{
/* attempting to remove a TopoLayer */
    sqlite3_int64 topolayer_id;
    int ret;
    char *sql;
    char *errMsg;
    char *table;
    char *xtable;
    char *xtable2;
    char dummy[64];
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (topo == NULL)
	return 0;

/* deleting all Feature relations */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE topolayer_id = "
			   "(SELECT topolayer_id FROM \"%s\" WHERE topolayer_name = Lower(%Q))",
			   xtable, xtable2, topolayer_name);
    free (xtable);
    free (xtable2);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* unregistering the TopoLayer */
    if (!do_unregister_topolayer (topo, topolayer_name, &topolayer_id))
	return 0;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* dropping the TopoFeatures Table */
    sprintf (dummy, "%lld", topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_RemoveTopoLayer() error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
is_unique_geom_name (sqlite3 * sqlite, const char *table, const char *geom)
{
/* checking for duplicate names */
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;

    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcasecmp (name, geom) == 0)
		    continue;
	    }
      }
    sqlite3_free_table (results);

    return 1;
}

static int
auxtopo_create_export_sql (struct gaia_topology *topo,
			   const char *topolayer_name, const char *out_table,
			   char **xcreate, char **xselect, char **xinsert,
			   char **geometry, sqlite3_int64 * topolayer_id)
{
/* composing the CREATE TABLE export-table statement */
    char *create = NULL;
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *table;
    char *xtable;
    char *xprefix;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    const char *type;
    int notnull;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    char *geometry_name;
    int geom_alias = 0;

    *xcreate = NULL;
    *xselect = NULL;
    *xinsert = NULL;

/* checking the TopoLayer */
    if (!check_topolayer (topo, topolayer_name, topolayer_id))
	return 0;

    xtable = gaiaDoubleQuotedSql (out_table);
    create =
	sqlite3_mprintf
	("CREATE TABLE MAIN.\"%s\" (\n\tfid INTEGER PRIMARY KEY", xtable);
    select = sqlite3_mprintf ("SELECT fid, ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (fid, ", xtable);
    free (xtable);
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcmp (name, "fid") == 0)
		    continue;
		type = results[(i * columns) + 2];
		notnull = atoi (results[(i * columns) + 3]);
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
		/* CREATE: adding a column definition */
		prev = create;
		xprefix = gaiaDoubleQuotedSql (name);
		if (notnull)
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s NOT NULL",
					 prev, xprefix, type);
		else
		    create =
			sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev,
					 xprefix, type);
		free (xprefix);
		sqlite3_free (prev);
	    }
      }
    sqlite3_free_table (results);

    geometry_name = malloc (strlen ("geometry") + 1);
    strcpy (geometry_name, "geometry");
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    while (1)
      {
	  /* searching an unique Geometry name */
	  if (is_unique_geom_name (topo->db_handle, table, geometry_name))
	      break;
	  sprintf (dummy, "geom_%d", ++geom_alias);
	  free (geometry_name);
	  geometry_name = malloc (strlen (dummy) + 1);
	  strcpy (geometry_name, dummy);
      }
    sqlite3_free (table);

/* completing the SQL statements */
    prev = create;
    create = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);
    prev = select;
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    select = sqlite3_mprintf ("%s FROM MAIN.\"%s\"", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s, \"%s\") VALUES (?, ", prev, geometry_name);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

    *xcreate = create;
    *xselect = select;
    *xinsert = insert;
    *geometry = geometry_name;
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
auxtopo_retrieve_export_geometry_type (struct gaia_topology *topo,
				       const char *topolayer_name,
				       int *ref_type)
{
/* determining the Geometry Type for Export TopoLayer */
    char *table;
    char *xtable;
    char *xtable2;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int nodes;
    int edges;
    int faces;

    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT Count(f.node_id), Count(f.edge_id), Count(f.face_id) "
	 "FROM \"%s\" AS l JOIN \"%s\" AS f ON (l.topolayer_id = f.topolayer_id) "
	 "WHERE l.topolayer_name = Lower(%Q)", xtable, xtable2, topolayer_name);
    free (xtable);
    free (xtable2);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		nodes = atoi (results[(i * columns) + 0]);
		edges = atoi (results[(i * columns) + 1]);
		faces = atoi (results[(i * columns) + 2]);
	    }
      }
    sqlite3_free_table (results);

    *ref_type = GAIA_UNKNOWN;
    if (nodes && !edges && !faces)
	*ref_type = GAIA_POINT;
    if (!nodes && edges && !faces)
	*ref_type = GAIA_LINESTRING;
    if (!nodes && !edges && faces)
	*ref_type = GAIA_POLYGON;

    return 1;
}

static void
do_eval_topo_node (struct gaia_topology *topo, sqlite3_stmt * stmt_node,
		   sqlite3_int64 node_id, gaiaGeomCollPtr result)
{
/* retrieving Points from Topology Nodes */
    int ret;

/* initializing the Topo-Node query */
    sqlite3_reset (stmt_node);
    sqlite3_clear_bindings (stmt_node);
    sqlite3_bind_int64 (stmt_node, 1, node_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_node);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_node, 0);
		int blob_sz = sqlite3_column_bytes (stmt_node, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaPointPtr pt = geom->FirstPoint;
		      while (pt != NULL)
			{
			    /* copying all Points into the result Geometry */
			    if (topo->has_z)
				gaiaAddPointToGeomCollXYZ (result, pt->X, pt->Y,
							   pt->Z);
			    else
				gaiaAddPointToGeomColl (result, pt->X, pt->Y);
			    pt = pt->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_eval_topo_edge (struct gaia_topology *topo, sqlite3_stmt * stmt_edge,
		   sqlite3_int64 edge_id, gaiaGeomCollPtr result)
{
/* retrieving Linestrings from Topology Edges */
    int ret;

/* initializing the Topo-Edge query */
    sqlite3_reset (stmt_edge);
    sqlite3_clear_bindings (stmt_edge);
    sqlite3_bind_int64 (stmt_edge, 1, edge_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_edge);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_edge, 0);
		int blob_sz = sqlite3_column_bytes (stmt_edge, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaLinestringPtr ln = geom->FirstLinestring;
		      while (ln != NULL)
			{
			    /* copying all Linestrings into the result Geometry */
			    if (topo->has_z)
				auxtopo_copy_linestring3d (ln, result);
			    else
				auxtopo_copy_linestring (ln, result);
			    ln = ln->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static gaiaGeomCollPtr
do_eval_topo_geometry (struct gaia_topology *topo, sqlite3_stmt * stmt_rels,
		       sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
		       sqlite3_stmt * stmt_face, sqlite3_int64 fid,
		       sqlite3_int64 topolayer_id, int out_type)
{
/* materializing a Geometry out of Topology */
    int ret;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr sparse_lines;
    struct face_edges *list =
	auxtopo_create_face_edges (topo->has_z, topo->srid);

    if (topo->has_z)
      {
	  geom = gaiaAllocGeomCollXYZ ();
	  sparse_lines = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  geom = gaiaAllocGeomColl ();
	  sparse_lines = gaiaAllocGeomColl ();
      }
    geom->Srid = topo->srid;
    geom->DeclaredType = out_type;

    sqlite3_reset (stmt_rels);
    sqlite3_clear_bindings (stmt_rels);
    sqlite3_bind_int64 (stmt_rels, 1, topolayer_id);
    sqlite3_bind_int64 (stmt_rels, 2, fid);
    while (1)
      {
	  /* scrolling the result set rows */
	  sqlite3_int64 id;
	  ret = sqlite3_step (stmt_rels);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_rels, 0) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 0);
		      do_eval_topo_node (topo, stmt_node, id, geom);
		  }
		if (sqlite3_column_type (stmt_rels, 1) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 1);
		      do_eval_topo_edge (topo, stmt_edge, id, sparse_lines);
		  }
		if (sqlite3_column_type (stmt_rels, 2) != SQLITE_NULL)
		  {
		      id = sqlite3_column_int64 (stmt_rels, 2);
		      do_explode_topo_face (topo, list, stmt_face, id);
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_FeatureFromTopoLayer() error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }


    if (sparse_lines->FirstLinestring != NULL)
      {
	  /* attempting to better rearrange sparse lines */
	  gaiaGeomCollPtr rearranged =
	      gaiaLineMerge_r (topo->cache, sparse_lines);
	  gaiaFreeGeomColl (sparse_lines);
	  if (rearranged != NULL)
	    {
		gaiaLinestringPtr ln = rearranged->FirstLinestring;
		while (ln != NULL)
		  {
		      if (topo->has_z)
			  auxtopo_copy_linestring3d (ln, geom);
		      else
			  auxtopo_copy_linestring (ln, geom);
		      ln = ln->Next;
		  }
		gaiaFreeGeomColl (rearranged);
	    }
      }
    else
	gaiaFreeGeomColl (sparse_lines);
    sparse_lines = NULL;

    if (list->first_edge != NULL)
      {
	  /* attempting to rearrange sparse lines into Polygons */
	  gaiaGeomCollPtr rearranged;
	  auxtopo_select_valid_face_edges (list);
	  rearranged = auxtopo_polygonize_face_edges (list, topo->cache);
	  auxtopo_free_face_edges (list);
	  if (rearranged != NULL)
	    {
		gaiaPolygonPtr pg = rearranged->FirstPolygon;
		while (pg != NULL)
		  {
		      if (topo->has_z)
			  do_copy_polygon3d (pg, geom);
		      else
			  do_copy_polygon (pg, geom);
		      pg = pg->Next;
		  }
		gaiaFreeGeomColl (rearranged);
	    }
      }

    if (geom->FirstPoint == NULL && geom->FirstLinestring == NULL
	&& geom->FirstPolygon == NULL)
	goto error;
    return geom;

  error:
    gaiaFreeGeomColl (geom);
    if (sparse_lines != NULL)
	gaiaFreeGeomColl (sparse_lines);
    return NULL;
}

static int
do_eval_topogeo_features (struct gaia_topology *topo, sqlite3_stmt * stmt_ref,
			  sqlite3_stmt * stmt_ins, sqlite3_stmt * stmt_rels,
			  sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_edge,
			  sqlite3_stmt * stmt_face, sqlite3_int64 topolayer_id,
			  int out_type)
{
/* querying the ref-table */
    int ret;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  sqlite3_int64 fid;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		fid = sqlite3_column_int64 (stmt_ref, 0);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		/* the Geometry column */
		ncol = sqlite3_bind_parameter_count (stmt_ins);
		geom =
		    do_eval_topo_geometry (topo, stmt_rels, stmt_node,
					   stmt_edge, stmt_face, fid,
					   topolayer_id, out_type);
		if (geom != NULL)
		  {
		      unsigned char *p_blob;
		      int n_bytes;
		      gaiaToSpatiaLiteBlobWkb (geom, &p_blob, &n_bytes);
		      sqlite3_bind_blob (stmt_ins, ncol, p_blob, n_bytes,
					 SQLITE_TRANSIENT);
		      free (p_blob);
		      gaiaFreeGeomColl (geom);
		  }
		else
		    sqlite3_bind_null (stmt_ins, ncol);
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ExportTopoLayer (GaiaTopologyAccessorPtr accessor,
			     const char *topolayer_name, const char *out_table,
			     int with_spatial_index, int create_only)
{
/* attempting to export a full TopoLayer */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *create = NULL;
    char *select = NULL;
    char *insert;
    char *geometry_name;
    char *sql;
    char *errMsg;
    char *xprefix;
    char *table;
    char *xtable;
    int ref_type;
    const char *type;
    int out_type;
    sqlite3_int64 topolayer_id;
    if (topo == NULL)
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_export_sql
	(topo, topolayer_name, out_table, &create,
	 &select, &insert, &geometry_name, &topolayer_id))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_export_geometry_type
	(topo, topolayer_name, &ref_type))
	goto error;
    switch (ref_type)
      {
      case GAIA_POINT:
	  type = "MULTIPOINT";
	  out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
	  type = "MULTILINESTRING";
	  out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
	  type = "MULTIPOLYGON";
	  out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
	  type = "GEOMETRYCOLLECTION";
	  out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  type = "GEOMETRY";
	  out_type = GAIA_UNKNOWN;
	  break;
      };

/* creating the output Geometry Column */
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), %Q, %d, '%s', '%s')",
	 out_table, geometry_name, topo->srid, type,
	 (topo->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (with_spatial_index)
      {
	  /* creating a Spatial Index supporting the Geometry Column */
	  sql =
	      sqlite3_mprintf
	      ("SELECT CreateSpatialIndex(Lower(%Q), %Q)",
	       out_table, geometry_name);
	  ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
				     errMsg);
		sqlite3_free (errMsg);
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    free (geometry_name);
    if (create_only)
      {
	  sqlite3_free (select);
	  sqlite3_free (insert);
	  return 1;
      }

/* preparing the "SELECT * FROM topo-features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM topo-features" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT node_id, edge_id, face_id FROM \"%s\" "
			   "WHERE topolayer_id = ? AND fid = ?", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE node_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	 "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_ExportTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating TopoLayer's Features */
    if (!do_eval_topogeo_features
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id, out_type))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
check_output_table (struct gaia_topology *topo, const char *out_table,
		    int *out_type)
{
/* checking the output table */
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    int count = 0;
    int type;

    sql =
	sqlite3_mprintf
	("SELECT geometry_type FROM MAIN.geometry_columns WHERE f_table_name = Lower(%Q)",
	 out_table);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		type = atoi (results[(i * columns) + 0]);
		count++;
	    }
      }
    sqlite3_free_table (results);

    if (count != 1)
	return 0;

    switch (type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  *out_type = GAIA_POINT;
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  *out_type = GAIA_MULTIPOINT;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  *out_type = GAIA_MULTILINESTRING;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  *out_type = GAIA_MULTIPOLYGON;
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  *out_type = GAIA_GEOMETRYCOLLECTION;
	  break;
      default:
	  *out_type = GAIA_UNKNOWN;
	  break;
      };
    return 1;
}

static int
auxtopo_export_feature_sql (struct gaia_topology *topo,
			    const char *topolayer_name, const char *out_table,
			    char **xselect, char **xinsert,
			    sqlite3_int64 * topolayer_id, int *out_type)
{
/* composing the CREATE TABLE insert-feature statement */
    char *select = NULL;
    char *insert = NULL;
    char *prev;
    char *sql;
    char *table;
    char *xtable;
    char *xprefix;
    char dummy[64];
    int i;
    char **results;
    int rows;
    int columns;
    const char *name;
    int ret;
    int first_select = 1;
    int first_insert = 1;
    int ncols = 0;
    int icol;
    int ref_col = 0;
    char *geometry_name = NULL;
    int geom_alias = 0;

    *xselect = NULL;
    *xinsert = NULL;

/* checking the TopoLayer */
    if (!check_topolayer (topo, topolayer_name, topolayer_id))
	return 0;

/* checking the output table */
    if (!check_output_table (topo, out_table, out_type))
	return 0;

    xtable = gaiaDoubleQuotedSql (out_table);
    select = sqlite3_mprintf ("SELECT fid, ");
    insert = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (fid, ", xtable);
    free (xtable);
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		name = results[(i * columns) + 1];
		if (strcmp (name, "fid") == 0)
		    continue;
		/* SELECT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = select;
		if (first_select)
		    select = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    select = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_select = 0;
		free (xprefix);
		sqlite3_free (prev);
		ref_col++;
		/* INSERT: adding a column */
		xprefix = gaiaDoubleQuotedSql (name);
		prev = insert;
		if (first_insert)
		    insert = sqlite3_mprintf ("%s\"%s\"", prev, xprefix);
		else
		    insert = sqlite3_mprintf ("%s, \"%s\"", prev, xprefix);
		first_insert = 0;
		free (xprefix);
		sqlite3_free (prev);
		ncols++;
	    }
      }
    sqlite3_free_table (results);

    geometry_name = malloc (strlen ("geometry") + 1);
    strcpy (geometry_name, "geometry");
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    while (1)
      {
	  /* searching an unique Geometry name */
	  if (is_unique_geom_name (topo->db_handle, table, geometry_name))
	      break;
	  sprintf (dummy, "geom_%d", ++geom_alias);
	  free (geometry_name);
	  geometry_name = malloc (strlen (dummy) + 1);
	  strcpy (geometry_name, dummy);
      }
    sqlite3_free (table);

/* completing the SQL statements */
    prev = select;
    sprintf (dummy, "%lld", *topolayer_id);
    table = sqlite3_mprintf ("%s_topofeatures_%s", topo->topology_name, dummy);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    select =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE fid = ?", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = insert;
    insert = sqlite3_mprintf ("%s, \"%s\") VALUES (?, ", prev, geometry_name);
    sqlite3_free (prev);
    for (icol = 0; icol < ncols; icol++)
      {
	  prev = insert;
	  if (icol == 0)
	      insert = sqlite3_mprintf ("%s?", prev);
	  else
	      insert = sqlite3_mprintf ("%s, ?", prev);
	  sqlite3_free (prev);
      }
    prev = insert;
    insert = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

    free (geometry_name);
    *xselect = select;
    *xinsert = insert;
    return 1;

  error:
    if (geometry_name != NULL)
	free (geometry_name);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    return 0;
}

static int
do_eval_topogeo_single_feature (struct gaia_topology *topo,
				sqlite3_stmt * stmt_ref,
				sqlite3_stmt * stmt_ins,
				sqlite3_stmt * stmt_rels,
				sqlite3_stmt * stmt_node,
				sqlite3_stmt * stmt_edge,
				sqlite3_stmt * stmt_face,
				sqlite3_int64 topolayer_id, int out_type,
				sqlite3_int64 fid)
{
/* querying the ref-table */
    int ret;
    int count = 0;

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    sqlite3_bind_int64 (stmt_ref, 1, fid);
    while (1)
      {
	  /* scrolling the result set rows */
	  gaiaGeomCollPtr geom = NULL;
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_ref);
		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_ref, icol);
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_ins, icol + 1,
						sqlite3_column_int64 (stmt_ref,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_ins, icol + 1,
						 sqlite3_column_double
						 (stmt_ref, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_ins, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_ins, icol + 1,
					       sqlite3_column_blob (stmt_ref,
								    icol),
					       sqlite3_column_bytes (stmt_ref,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_ins, icol + 1);
			    break;
			};
		  }
		/* the Geometry column */
		ncol = sqlite3_bind_parameter_count (stmt_ins);
		geom =
		    do_eval_topo_geometry (topo, stmt_rels, stmt_node,
					   stmt_edge, stmt_face, fid,
					   topolayer_id, out_type);
		if (geom != NULL)
		  {
		      unsigned char *p_blob;
		      int n_bytes;
		      gaiaToSpatiaLiteBlobWkb (geom, &p_blob, &n_bytes);
		      sqlite3_bind_blob (stmt_ins, ncol, p_blob, n_bytes,
					 SQLITE_TRANSIENT);
		      free (p_blob);
		      gaiaFreeGeomColl (geom);
		  }
		else
		    sqlite3_bind_null (stmt_ins, ncol);
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("InsertFeatureFromTopoLayer() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
		count++;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("InsertFeatureFromTopoLayer() error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (count <= 0)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("InsertFeatureFromTopoLayer(): not existing TopoFeature");
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_InsertFeatureFromTopoLayer (GaiaTopologyAccessorPtr accessor,
					const char *topolayer_name,
					const char *out_table,
					sqlite3_int64 fid)
{
/* attempting to insert a single TopoLayer's Feature into the output GeoTable */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_rels = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_edge = NULL;
    sqlite3_stmt *stmt_face = NULL;
    int ret;
    char *select;
    char *insert;
    char *sql;
    char *xprefix;
    char *table;
    char *xtable;
    int out_type;
    sqlite3_int64 topolayer_id;
    if (topo == NULL)
	return 0;

/* composing the SQL statements */
    if (!auxtopo_export_feature_sql
	(topo, topolayer_name, out_table, &select, &insert, &topolayer_id,
	 &out_type))
	goto error;

/* preparing the "SELECT * FROM topo-features-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  goto error;
      }

/* preparing the "SELECT * FROM topo-features" query */
    table = sqlite3_mprintf ("%s_topofeatures", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT node_id, edge_id, face_id FROM \"%s\" "
			   "WHERE topolayer_id = ? AND fid = ?", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rels,
			    NULL);
    sqlite3_free (sql);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE node_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Edges query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geom FROM MAIN.\"%s\" WHERE edge_id = ?",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_edge,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Faces query */
    xprefix = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, left_face, right_face, geom FROM MAIN.\"%s\" "
	 "WHERE left_face = ? OR right_face = ?", xtable);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_face,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("InsertFeatureFromTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating TopoLayer's Features */
    if (!do_eval_topogeo_single_feature
	(topo, stmt_ref, stmt_ins, stmt_rels, stmt_node, stmt_edge, stmt_face,
	 topolayer_id, out_type, fid))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_rels);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_edge);
    sqlite3_finalize (stmt_face);
    return 1;

  error:
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    if (stmt_rels != NULL)
	sqlite3_finalize (stmt_rels);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_edge != NULL)
	sqlite3_finalize (stmt_edge);
    if (stmt_face != NULL)
	sqlite3_finalize (stmt_face);
    return 0;
}

static int
do_topo_snap (struct gaia_topology *topo, int geom_col, int geo_type,
	      double tolerance_snap, double tolerance_removal, int iterate,
	      sqlite3_stmt * stmt_in, sqlite3_stmt * stmt_out)
{
/* snapping geometries againt Topology */
    int ret;

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int icol;
		int ncol = sqlite3_column_count (stmt_in);
		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		for (icol = 0; icol < ncol; icol++)
		  {
		      int col_type = sqlite3_column_type (stmt_in, icol);
		      if (icol == geom_col)
			{
			    /* the geometry column */
			    const unsigned char *blob =
				sqlite3_column_blob (stmt_in, icol);
			    int blob_sz = sqlite3_column_bytes (stmt_in, icol);
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    if (geom != NULL)
			      {
				  gaiaGeomCollPtr result;
				  unsigned char *p_blob;
				  int n_bytes;
				  int gpkg_mode = 0;
				  int tiny_point = 0;
				  if (topo->cache != NULL)
				    {
					struct splite_internal_cache *cache =
					    (struct splite_internal_cache
					     *) (topo->cache);
					gpkg_mode = cache->gpkg_mode;
					tiny_point = cache->tinyPointEnabled;
				    }
				  result =
				      gaiaTopoSnap ((GaiaTopologyAccessorPtr)
						    topo, geom, tolerance_snap,
						    tolerance_removal, iterate);
				  gaiaFreeGeomColl (geom);
				  if (result != NULL)
				    {
					result->DeclaredType = geo_type;
					gaiaToSpatiaLiteBlobWkbEx2 (result,
								    &p_blob,
								    &n_bytes,
								    gpkg_mode,
								    tiny_point);
					gaiaFreeGeomColl (result);
					sqlite3_bind_blob (stmt_out, icol + 1,
							   p_blob, n_bytes,
							   free);
				    }
				  else
				      sqlite3_bind_null (stmt_out, icol + 1);
			      }
			    else
				sqlite3_bind_null (stmt_out, icol + 1);
			    continue;
			}
		      switch (col_type)
			{
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_out, icol + 1,
						sqlite3_column_int64 (stmt_in,
								      icol));
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_out, icol + 1,
						 sqlite3_column_double
						 (stmt_in, icol));
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_out, icol + 1,
					       (const char *)
					       sqlite3_column_text (stmt_in,
								    icol),
					       sqlite3_column_bytes (stmt_in,
								     icol),
					       SQLITE_STATIC);
			    break;
			case SQLITE_BLOB:
			    sqlite3_bind_blob (stmt_out, icol + 1,
					       sqlite3_column_blob (stmt_in,
								    icol),
					       sqlite3_column_bytes (stmt_in,
								     icol),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_out, icol + 1);
			    break;
			};
		  }
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoGeo_SnappedGeoTable() error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr)
						   topo, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_SnappedGeoTable() error: \"%s\"",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_SnappedGeoTable (GaiaTopologyAccessorPtr accessor,
			     const char *db_prefix, const char *table,
			     const char *column, const char *out_table,
			     double tolerance_snap, double tolerance_removal,
			     int iterate)
{
/* 
/ attempting to create and populate a new GeoTable by snapping all Geometries
/ contained into another GeoTable against a given Topology
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;
    char *create;
    char *select;
    char *insert;
    char *sql;
    char *errMsg;
    int geo_type;
    const char *type;
    int geom_col;
    if (topo == NULL)
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_togeotable_sql
	(topo->db_handle, db_prefix, table, column, out_table, &create,
	 &select, &insert, &geom_col))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (topo->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnappedGeoTable() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_geometry_type
	(topo->db_handle, db_prefix, table, column, &geo_type))
	goto error;
    switch (geo_type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  type = "POINT";
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  type = "MULTIPOINT";
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
	  type = "LINESTRING";
	  break;
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  type = "MULTILINESTRING";
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
	  type = "POLYGON";
	  break;
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  type = "MULTIPOLYGON";
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  type = "GEOMETRYCOLLECTION";
	  break;
      default:
	  type = "GEOMETRY";
	  break;
      };

/* creating the output Geometry Column */
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), Lower(%Q), %d, '%s', '%s')",
	 out_table, column, topo->srid, type,
	 (topo->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnappedGeoTable() error: \"%s\"",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, select, strlen (select), &stmt_in,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnappedGeoTable() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (topo->db_handle, insert, strlen (insert), &stmt_out,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_SnappedGeoTable() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* snapping all Geometries against the Topology */
    if (!do_topo_snap
	(topo, geom_col, geo_type, tolerance_snap, tolerance_removal, iterate,
	 stmt_in, stmt_out))
	goto error;

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (create != NULL)
	sqlite3_free (create);
    if (select != NULL)
	sqlite3_free (select);
    if (insert != NULL)
	sqlite3_free (insert);
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

SPATIALITE_PRIVATE int
test_inconsistent_topology (const void *handle)
{
/* testing for a Topology presenting an inconsistent state */
    struct gaia_topology *topo = (struct gaia_topology *) handle;
    int ret;
    char *errMsg = NULL;
    int count = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *sql;
    char *table;
    char *xtable;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM \"%s\" WHERE left_face IS NULL "
			 "OR right_face IS NULL", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("test_inconsistent_topology error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return -1;
      }
    for (i = 1; i <= rows; i++)
	count = atoi (results[(i * columns) + 0]);
    sqlite3_free_table (results);
    return count;
}

static int
topoGeo_EdgeHeal_common (GaiaTopologyAccessorPtr accessor, int mode_new)
{
/* common implementation of GeoTable EdgeHeal */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    char *sql;
    char *node;
    char *xnode;
    char *edge;
    char *xedge;
    int loop = 1;
    sqlite3_stmt *stmt1 = NULL;
    sqlite3_stmt *stmt2 = NULL;
    sqlite3_stmt *stmt3 = NULL;
    if (topo == NULL)
	return 0;

    ret = test_inconsistent_topology (accessor);
    if (ret != 0)
	return 0;

/* preparing the SQL query identifying all Nodes of cardinality = 2 */
    node = sqlite3_mprintf ("%s_node", topo->topology_name);
    xnode = gaiaDoubleQuotedSql (node);
    sqlite3_free (node);
    edge = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedge = gaiaDoubleQuotedSql (edge);
    sqlite3_free (edge);
    sql =
	sqlite3_mprintf ("SELECT n.node_id, Count(*) AS cnt FROM \"%s\" AS n "
			 "JOIN \"%s\" AS e ON (n.node_id = e.start_node OR n.node_id = e.end_node) "
			 "GROUP BY n.node_id HAVING cnt = 2", xnode, xedge);
    free (xnode);
    free (xedge);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_%sEdgeHeal error: \"%s\"",
				       mode_new ? "New" : "Mod",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SQL query identifying a couple of Edges to be Healed */
    node = sqlite3_mprintf ("%s_node", topo->topology_name);
    xnode = gaiaDoubleQuotedSql (node);
    sqlite3_free (node);
    edge = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedge = gaiaDoubleQuotedSql (edge);
    sqlite3_free (edge);
    sql =
	sqlite3_mprintf ("SELECT e1.edge_id, e2.edge_id FROM \"%s\" AS n "
			 "JOIN \"%s\" AS e1 ON (n.node_id = e1.end_node) "
			 "JOIN \"%s\" AS e2 ON (n.node_id = e2.start_node) "
			 "WHERE n.node_id = ? AND e1.start_node <> e1.end_node "
			 "AND e2.start_node <> e2.end_node",
			 xnode, xedge, xedge);
    free (xnode);
    free (xedge);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt2, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_%sEdgeHeal error: \"%s\"",
				       mode_new ? "New" : "Mod",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SQL query Healing a couple of Edges */
    sql =
	sqlite3_mprintf ("SELECT ST_%sEdgeHeal(%Q, ?, ?)",
			 mode_new ? "New" : "Mod", topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt3, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_%sEdgeHeal error: \"%s\"",
				       mode_new ? "New" : "Mod",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (loop)
      {
	  /* looping until all possible Edges have been healed */
	  sqlite3_reset (stmt1);
	  sqlite3_clear_bindings (stmt1);
	  loop = 0;

	  while (1)
	    {
		/* scrolling the result set rows */
		sqlite3_int64 edge_1_id = -1;
		sqlite3_int64 edge_2_id = -1;
		ret = sqlite3_step (stmt1);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      sqlite3_reset (stmt2);
		      sqlite3_clear_bindings (stmt2);
		      sqlite3_bind_int64 (stmt2, 1,
					  sqlite3_column_int64 (stmt1, 0));

		      while (1)
			{
			    /* scrolling the result set rows */
			    ret = sqlite3_step (stmt2);
			    if (ret == SQLITE_DONE)
				break;	/* end of result set */
			    if (ret == SQLITE_ROW)
			      {
				  edge_1_id = sqlite3_column_int64 (stmt2, 0);
				  edge_2_id = sqlite3_column_int64 (stmt2, 1);
				  if (edge_1_id == edge_2_id)
				    {
					/* can't Heal - dog chasing its tail */
					edge_1_id = -1;
					edge_2_id = -1;
					continue;
				    }
				  break;
			      }
			    else
			      {
				  char *msg =
				      sqlite3_mprintf
				      ("TopoGeo_%sEdgeHeal error: \"%s\"",
				       mode_new ? "New" : "Mod",
				       sqlite3_errmsg (topo->db_handle));
				  gaiatopo_set_last_error_msg (accessor, msg);
				  sqlite3_free (msg);
				  goto error;
			      }
			}
		      if (edge_1_id < 0 && edge_2_id < 0)
			  break;

		      /* healing a couple of Edges */
		      sqlite3_reset (stmt3);
		      sqlite3_clear_bindings (stmt3);
		      sqlite3_bind_int64 (stmt3, 1, edge_1_id);
		      sqlite3_bind_int64 (stmt3, 2, edge_2_id);
		      ret = sqlite3_step (stmt3);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			{
			    loop = 1;
			    continue;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoGeo_%sEdgeHeal error: \"%s\"",
				 mode_new ? "New" : "Mod",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
	    }
      }

    sqlite3_finalize (stmt1);
    sqlite3_finalize (stmt2);
    sqlite3_finalize (stmt3);
    return 1;

  error:
    if (stmt1 != NULL)
	sqlite3_finalize (stmt1);
    if (stmt2 != NULL)
	sqlite3_finalize (stmt2);
    if (stmt3 != NULL)
	sqlite3_finalize (stmt3);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_NewEdgeHeal (GaiaTopologyAccessorPtr ptr)
{
    return topoGeo_EdgeHeal_common (ptr, 1);
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ModEdgeHeal (GaiaTopologyAccessorPtr ptr)
{
    return topoGeo_EdgeHeal_common (ptr, 0);
}

static int
do_split_edge (GaiaTopologyAccessorPtr accessor, sqlite3 * sqlite,
	       sqlite3_stmt * stmt, sqlite3_int64 edge_id, gaiaGeomCollPtr geom,
	       int line_max_points, double max_length, int *count)
{
/* attempting to split an Edge in two halves */
    int nlns = 0;
    char *msg;
    double x;
    double y;
    double z;
    int last;
    unsigned char *blob = NULL;
    int blob_size = 0;
    int ret;
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr point = NULL;
    gaiaGeomCollPtr split =
	gaiaTopoGeo_SubdivideLines (geom, line_max_points, max_length);

    ln = split->FirstLinestring;
    while (ln != NULL)
      {
	  nlns++;
	  ln = ln->Next;
      }
    if (nlns < 2)
	return 1;		/* Edge not requiring to be split */

    ln = split->FirstLinestring;
    last = ln->Points - 1;
    if (split->DimensionModel == GAIA_XY_Z)
      {
	  /* 3D topology */
	  point = gaiaAllocGeomCollXYZ ();
	  point->Srid = geom->Srid;
	  gaiaGetPointXYZ (ln->Coords, last, &x, &y, &z);
	  gaiaAddPointToGeomCollXYZ (point, x, y, z);
      }
    else
      {
	  /* 2D topology */
	  point = gaiaAllocGeomColl ();
	  point->Srid = geom->Srid;
	  gaiaGetPoint (ln->Coords, last, &x, &y);
	  gaiaAddPointToGeomColl (point, x, y);
      }

/* splitting the Edge */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, edge_id);
    gaiaToSpatiaLiteBlobWkb (point, &blob, &blob_size);
    sqlite3_bind_blob (stmt, 2, blob, blob_size, free);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  *count += 1;
	  return 1;
      }

/* some unexpected error occurred */
    msg = sqlite3_mprintf ("Edge Split error: \"%s\"", sqlite3_errmsg (sqlite));
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_free (msg);
    return 0;
}

static int
topoGeo_EdgeSplit_common (GaiaTopologyAccessorPtr accessor, int mode_new,
			  int line_max_points, double max_length)
{
/* common implementation of GeoTable EdgeSplit */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    char *sql;
    char *edge;
    char *xedge;
    sqlite3_stmt *stmt1 = NULL;
    sqlite3_stmt *stmt2 = NULL;
    if (topo == NULL)
	return 0;

    ret = test_inconsistent_topology (accessor);
    if (ret != 0)
	return 0;

/* preparing the SQL query identifying all Edges */
    edge = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedge = gaiaDoubleQuotedSql (edge);
    sqlite3_free (edge);
    sql =
	sqlite3_mprintf ("SELECT edge_id, geom FROM \"%s\" ORDER BY edge_id",
			 xedge);
    free (xedge);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt1, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_%sSplit error: \"%s\"",
				       mode_new ? "NewEdges" : "ModEdge",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SQL query splitting an Edge in two halves */
    sql =
	sqlite3_mprintf ("SELECT ST_%sSplit(%Q, ?, ?)",
			 mode_new ? "NewEdges" : "ModEdge",
			 topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt2, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoGeo_%sSplit error: \"%s\"",
				       mode_new ? "NewEdges" : "ModEdge",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* repeatedly looping on all Edges */
	  int count = 0;
	  sqlite3_reset (stmt1);
	  sqlite3_clear_bindings (stmt1);

	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt1);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      sqlite3_int64 edge_id = sqlite3_column_int64 (stmt1, 0);
		      if (sqlite3_column_type (stmt1, 1) == SQLITE_BLOB)
			{
			    const unsigned char *blob =
				sqlite3_column_blob (stmt1, 1);
			    int blob_sz = sqlite3_column_bytes (stmt1, 1);
			    gaiaGeomCollPtr geom =
				gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
			    if (geom != NULL)
			      {
				  if (!do_split_edge
				      (accessor, topo->db_handle, stmt2,
				       edge_id, geom, line_max_points,
				       max_length, &count))
				    {
					gaiaFreeGeomColl (geom);
					goto error;
				    }
			      }
			    gaiaFreeGeomColl (geom);
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("TopoGeo_%sSplit error: \"%s\"",
					   mode_new ? "NewEdges" : "ModEdge",
					   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  if (count == 0)
	      break;
      }

    sqlite3_finalize (stmt1);
    sqlite3_finalize (stmt2);
    return 1;

  error:
    if (stmt1 != NULL)
	sqlite3_finalize (stmt1);
    if (stmt2 != NULL)
	sqlite3_finalize (stmt2);
    return 0;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_NewEdgesSplit (GaiaTopologyAccessorPtr ptr, int line_max_points,
			   double max_length)
{
    return topoGeo_EdgeSplit_common (ptr, 1, line_max_points, max_length);
}

GAIATOPO_DECLARE int
gaiaTopoGeo_ModEdgeSplit (GaiaTopologyAccessorPtr ptr, int line_max_points,
			  double max_length)
{
    return topoGeo_EdgeSplit_common (ptr, 0, line_max_points, max_length);
}

static gaiaGeomCollPtr
do_interpolate_middlepoint (gaiaGeomCollPtr geom)
{
/* building a three-point segment */
    gaiaGeomCollPtr newg;
    gaiaLinestringPtr old_ln;
    gaiaLinestringPtr new_ln;
    double x0;
    double y0;
    double z0;
    double x1;
    double y1;
    double z1;
    double mx;
    double my;
    double mz;

    if (geom == NULL)
	return NULL;
    if (geom->FirstPoint != NULL || geom->FirstPolygon != NULL)
	return NULL;
    if (geom->FirstLinestring != geom->LastLinestring)
	return NULL;
    old_ln = geom->FirstLinestring;
    if (old_ln == NULL)
	return NULL;
    if (old_ln->Points != 2)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z)
      {
	  gaiaGetPointXYZ (old_ln->Coords, 0, &x0, &y0, &z0);
	  gaiaGetPointXYZ (old_ln->Coords, 1, &x1, &y1, &z1);
	  newg = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  gaiaGetPoint (old_ln->Coords, 0, &x0, &y0);
	  gaiaGetPoint (old_ln->Coords, 1, &x1, &y1);
	  newg = gaiaAllocGeomColl ();
      }
    newg->Srid = geom->Srid;

    if (x0 > x1)
	mx = x1 + ((x0 - x1) / 2.0);
    else
	mx = x0 + ((x1 - x0) / 2.0);
    if (y0 > y1)
	my = y1 + ((y0 - y1) / 2.0);
    else
	my = y0 + ((y1 - y0) / 2.0);
    if (geom->DimensionModel == GAIA_XY_Z)
      {
	  if (z0 > z1)
	      mz = z1 + ((z0 - z1) / 2.0);
	  else
	      mz = z0 + ((z1 - z0) / 2.0);
      }

    new_ln = gaiaAddLinestringToGeomColl (newg, 3);
    if (newg->DimensionModel == GAIA_XY_Z)
      {
	  gaiaSetPointXYZ (new_ln->Coords, 0, x0, y0, z0);
	  gaiaSetPointXYZ (new_ln->Coords, 1, mx, my, mz);
	  gaiaSetPointXYZ (new_ln->Coords, 2, x1, y1, z1);
      }
    else
      {
	  gaiaSetPoint (new_ln->Coords, 0, x0, y0);
	  gaiaSetPoint (new_ln->Coords, 1, mx, my);
	  gaiaSetPoint (new_ln->Coords, 2, x1, y1);
      }

    return newg;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_DisambiguateSegmentEdges (GaiaTopologyAccessorPtr accessor)
{
/*
/ Ensures that all Edges on a Topology-Geometry will have not less
/ than three vertices; for all Edges found beign simple two-points 
/ segments a third intermediate point will be interpolated.
*/
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    int ret;
    char *sql;
    char *edge;
    char *xedge;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int count = 0;
    if (topo == NULL)
	return -1;

/* preparing the SQL query identifying all two-points Edges */
    edge = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xedge = gaiaDoubleQuotedSql (edge);
    sqlite3_free (edge);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, geom FROM \"%s\" WHERE ST_NumPoints(geom) = 2 "
	 "ORDER BY edge_id", xedge);
    free (xedge);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_DisambiguateSegmentEdges error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the UPDATE SQL query */
    sql =
	sqlite3_mprintf ("SELECT ST_ChangeEdgeGeom(%Q, ?, ?)",
			 topo->topology_name);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_DisambiguateSegmentEdges error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_in, 1);
		      int blob_sz = sqlite3_column_bytes (stmt_in, 1);
		      gaiaGeomCollPtr geom =
			  gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (geom != NULL)
			{
			    gaiaGeomCollPtr newg =
				do_interpolate_middlepoint (geom);
			    gaiaFreeGeomColl (geom);
			    if (newg != NULL)
			      {
				  unsigned char *outblob = NULL;
				  int outblob_size = 0;
				  sqlite3_reset (stmt_out);
				  sqlite3_clear_bindings (stmt_out);
				  sqlite3_bind_int64 (stmt_out, 1, edge_id);
				  gaiaToSpatiaLiteBlobWkb (newg, &outblob,
							   &outblob_size);
				  gaiaFreeGeomColl (newg);
				  if (blob == NULL)
				      continue;
				  else
				      sqlite3_bind_blob (stmt_out, 2, outblob,
							 outblob_size, free);
				  /* updating the Edges table */
				  ret = sqlite3_step (stmt_out);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      count++;
				  else
				    {
					char *msg =
					    sqlite3_mprintf
					    ("TopoGeo_DisambiguateSegmentEdges() error: \"%s\"",
					     sqlite3_errmsg (topo->db_handle));
					gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
					sqlite3_free (msg);
					goto error;
				    }
			      }
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("TopoGeo_DisambiguateSegmentEdges error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return count;

  error:
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return -1;
}

#endif /* end ENABLE_RTTOPO conditionals */
