/*

 se_helpers.c -- SLD/SE helper functions 

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

/*
 
CREDITS:

this module has been partly funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(implementing XML support - ISO Metadata and SLD/SE Styles) 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

#ifdef ENABLE_LIBXML2		/* including LIBXML2 */

/* Constant definitions: Vector Coverage Types */
#define VECTOR_UNKNOWN		0
#define VECTOR_GEOTABLE		1
#define VECTOR_SPATIALVIEW	2
#define VECTOR_VIRTUALSHP	3
#define VECTOR_TOPOGEO		4
#define VECTOR_TOPONET		5

static int
check_external_graphic (sqlite3 * sqlite, const char *xlink_href)
{
/* checks if an ExternalGraphic Resource already exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    sql = "SELECT xlink_href FROM SE_external_graphics WHERE xlink_href = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("checkExternalGraphic: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);
    return exists;
}

SPATIALITE_PRIVATE int
register_external_graphic (void *p_sqlite, const char *xlink_href,
			   const unsigned char *p_blob, int n_bytes,
			   const char *title, const char *abstract,
			   const char *file_name)
{
/* auxiliary function: inserts or updates an ExternalGraphic Resource */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int extras = 0;
    int retval = 0;

/* checking if already exists */
    if (xlink_href == NULL)
	return 0;
    exists = check_external_graphic (sqlite, xlink_href);

    if (title != NULL && abstract != NULL && file_name != NULL)
	extras = 1;
    if (exists)
      {
	  /* update */
	  if (extras)
	    {
		/* full infos */
		sql = "UPDATE SE_external_graphics "
		    "SET resource = ?, title = ?, abstract = ?, file_name = ? "
		    "WHERE xlink_href = ?";
	    }
	  else
	    {
		/* limited basic infos */
		sql = "UPDATE SE_external_graphics "
		    "SET resource = ? WHERE xlink_href = ?";
	    }
      }
    else
      {
	  /* insert */
	  if (extras)
	    {
		/* full infos */
		sql = "INSERT INTO SE_external_graphics "
		    "(xlink_href, resource, title, abstract, file_name) "
		    "VALUES (?, ?, ?, ?, ?)";
	    }
	  else
	    {
		/* limited basic infos */
		sql = "INSERT INTO SE_external_graphics "
		    "(xlink_href, resource) VALUES (?, ?)";
	    }
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerExternalGraphic: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  if (extras)
	    {
		/* full infos */
		sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, abstract, strlen (abstract),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 4, file_name, strlen (file_name),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 5, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
	    }
	  else
	    {
		/* limited basic infos */
		sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
	    }
      }
    else
      {
	  /* insert */
	  if (extras)
	    {
		/* full infos */
		sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
		sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 4, abstract, strlen (abstract),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 5, file_name, strlen (file_name),
				   SQLITE_STATIC);
	    }
	  else
	    {
		/* limited basic infos */
		sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href),
				   SQLITE_STATIC);
		sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
	    }
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerExternalGraphic() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_external_graphic (void *p_sqlite, const char *xlink_href)
{
/* auxiliary function: deletes an ExternalGraphic Resource */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

/* checking if already exists */
    if (xlink_href == NULL)
	return 0;
    exists = check_external_graphic (sqlite, xlink_href);
    if (!exists)
	return 0;

    sql = "DELETE FROM SE_external_graphics WHERE xlink_href = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterExternalGraphic: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterExternalGraphic() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

static int
vector_style_causes_duplicate_name (sqlite3 * sqlite, sqlite3_int64 id,
				    const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: checks for an eventual duplicate name */
    int count = 0;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "SELECT Count(*) FROM SE_vector_styles "
	"WHERE Lower(style_name) = Lower(XB_GetName(?)) AND style_id <> ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("VectorStyle duplicate Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    if (count != 0)
	return 1;
    return 0;
}

SPATIALITE_PRIVATE int
register_vector_style (void *p_sqlite, const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts a Vector Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to insert the Vector Style */
	  if (vector_style_causes_duplicate_name (sqlite, -1, p_blob, n_bytes))
	      return 0;
	  sql = "INSERT INTO SE_vector_styles "
	      "(style_id, style) VALUES (NULL, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

static int
check_vector_style_by_id (sqlite3 * sqlite, int style_id)
{
/* checks if a Vector Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_vector_styles " "WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Style by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_vector_style_by_name (sqlite3 * sqlite, const char *style_name,
			    sqlite3_int64 * id)
{
/* checks if a Vector Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_vector_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Style by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_vector_style_refs_by_id (sqlite3 * sqlite, int style_id, int *has_refs)
{
/* checks if a Vector Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;

    sql = "SELECT s.style_id, l.style_id FROM SE_vector_styles AS s "
	"LEFT JOIN SE_vector_styled_layers AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count >= 1)
      {
	  if (ref_count > 0)
	      *has_refs = 1;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_vector_style_refs_by_name (sqlite3 * sqlite, const char *style_name,
				 sqlite3_int64 * id, int *has_refs)
{
/* checks if a Vector Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_vector_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Style Refs by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    *id = xid;
    sql = "SELECT s.style_id, l.style_id FROM SE_vector_styles AS s "
	"LEFT JOIN SE_vector_styled_layers AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, *id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (ref_count > 0)
	*has_refs = 1;
    return 1;
  stop:
    return 0;
}

static int
do_insert_vector_style_layer (sqlite3 * sqlite, const char *coverage_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really inserting a Vector Styled Layer */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "INSERT INTO SE_vector_styled_layers "
	"(coverage_name, style_id) VALUES (?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerVectorStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerVectorStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

static int
do_delete_vector_style_refs (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: deleting all Vector Style references */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_vector_styled_layers WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterVectorStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

static int
do_delete_vector_style (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: really deleting a Vector Style */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_vector_styles WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterVectorStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_vector_style (void *p_sqlite, int style_id,
			 const char *style_name, int remove_all)
{
/* auxiliary function: deletes a Vector Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;
    int has_refs = 0;

    if (style_id >= 0)
      {
	  /* checking if the Vector Style do actually exists */
	  if (check_vector_style_refs_by_id (sqlite, style_id, &has_refs))
	      id = style_id;
	  else
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_vector_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Vector Style */
	  return do_delete_vector_style (sqlite, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Vector Style do actually exists */
	  if (!check_vector_style_refs_by_name
	      (sqlite, style_name, &id, &has_refs))
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_vector_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Vector Style */
	  return do_delete_vector_style (sqlite, id);
      }
    else
	return 0;
}

static int
do_reload_vector_style (sqlite3 * sqlite, sqlite3_int64 id,
			const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Vector Style definition */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to update the Vector Style */
	  sql = "UPDATE SE_vector_styles SET style = ? " "WHERE style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("reloadVectorStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_int64 (stmt, 2, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("reloadVectorStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
reload_vector_style (void *p_sqlite, int style_id,
		     const char *style_name,
		     const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Vector Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (style_id >= 0)
      {
	  /* checking if the Vector Style do actually exists */
	  if (check_vector_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* reloading the Vector Style */
	  if (vector_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_vector_style (sqlite, id, p_blob, n_bytes);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Vector Style do actually exists */
	  if (!check_vector_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* reloading the Vector Style */
	  if (vector_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_vector_style (sqlite, id, p_blob, n_bytes);
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_vector_styled_layer_ex (void *p_sqlite, const char *coverage_name,
				 int style_id, const char *style_name)
{
/* auxiliary function: inserts a Vector Styled Layer definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (coverage_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Vector Style do actually exists */
	  if (check_vector_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* inserting the Vector Styled Layer */
	  return do_insert_vector_style_layer (sqlite, coverage_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Vector Style do actually exists */
	  if (!check_vector_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* inserting the Vector Styled Layer */
	  return do_insert_vector_style_layer (sqlite, coverage_name, id);
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_vector_styled_layer (void *p_sqlite, const char *f_table_name,
			      const char *f_geometry_column, int style_id,
			      const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts a Vector Styled Layer definition - DEPRECATED */
    if (p_blob != NULL && n_bytes <= 0 && f_geometry_column != NULL)
      {
	  /* silencing compiler complaints */
	  p_blob = NULL;
	  n_bytes = 0;
	  f_geometry_column = NULL;
      }
    return register_vector_styled_layer_ex (p_sqlite, f_table_name, style_id,
					    NULL);
}

static int
check_vector_styled_layer_by_id (sqlite3 * sqlite, const char *coverage_name,
				 int style_id)
{
/* checks if a Vector Styled Layer do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_vector_styled_layers "
	"WHERE Lower(coverage_name) = Lower(?) AND style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Styled Layer by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_vector_styled_layer_by_name (sqlite3 * sqlite, const char *coverage_name,
				   const char *style_name, sqlite3_int64 * id)
{
/* checks if a Vector Styled Layer do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT l.style_id FROM SE_vector_styled_layers AS l "
	"JOIN SE_vector_styles AS s ON (l.style_id = s.style_id) "
	"WHERE Lower(l.coverage_name) = Lower(?) "
	"AND Lower(s.style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Styled Layer by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_delete_vector_style_layer (sqlite3 * sqlite, const char *coverage_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really deleting a Vector Styled Layer */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_vector_styled_layers "
	"WHERE Lower(coverage_name) = Lower(?) AND style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterVectorStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_vector_styled_layer (void *p_sqlite, const char *coverage_name,
				int style_id, const char *style_name)
{
/* auxiliary function: removes a Vector Styled Layer definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (coverage_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Vector Styled Layer do actually exists */
	  if (check_vector_styled_layer_by_id (sqlite, coverage_name, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* removing the Vector Styled Layer */
	  return do_delete_vector_style_layer (sqlite, coverage_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Vector Styled Layer do actually exists */
	  if (!check_vector_styled_layer_by_name
	      (sqlite, coverage_name, style_name, &id))
	      return 0;
	  /* removing the Vector Styled Layer */
	  return do_delete_vector_style_layer (sqlite, coverage_name, id);
      }
    else
	return 0;
}

static int
raster_style_causes_duplicate_name (sqlite3 * sqlite, sqlite3_int64 id,
				    const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: checks for an eventual duplicate name */
    int count = 0;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "SELECT Count(*) FROM SE_raster_styles "
	"WHERE Lower(style_name) = Lower(XB_GetName(?)) AND style_id <> ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("RasterStyle duplicate Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    if (count != 0)
	return 1;
    return 0;
}

SPATIALITE_PRIVATE int
register_raster_style (void *p_sqlite, const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts a Raster Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to insert the Raster Style */
	  if (raster_style_causes_duplicate_name (sqlite, -1, p_blob, n_bytes))
	      return 0;
	  sql = "INSERT INTO SE_raster_styles "
	      "(style_id, style) VALUES (NULL, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerRasterStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerRasterStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

static int
do_delete_raster_style_refs (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: deleting all Raster Style references */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_raster_styled_layers WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterRasterStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterRasterStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

static int
check_raster_style_refs_by_id (sqlite3 * sqlite, int style_id, int *has_refs)
{
/* checks if a Raster Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;

    sql = "SELECT s.style_id, l.style_id FROM SE_raster_styles AS s "
	"LEFT JOIN SE_raster_styled_layers AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count >= 1)
      {
	  if (ref_count > 0)
	      *has_refs = 1;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_raster_style_refs_by_name (sqlite3 * sqlite, const char *style_name,
				 sqlite3_int64 * id, int *has_refs)
{
/* checks if a Raster Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_raster_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Style Refs by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    *id = xid;
    sql = "SELECT s.style_id, l.style_id FROM SE_raster_styles AS s "
	"LEFT JOIN SE_raster_styled_layers AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, *id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (ref_count > 0)
	*has_refs = 1;
    return 1;
  stop:
    return 0;
}

static int
do_delete_raster_style (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: really deleting a Raster Style */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_raster_styles WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterRasterStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterRasterStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_raster_style (void *p_sqlite, int style_id,
			 const char *style_name, int remove_all)
{
/* auxiliary function: deletes a Raster Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;
    int has_refs = 0;

    if (style_id >= 0)
      {
	  /* checking if the Raster Style do actually exists */
	  if (check_raster_style_refs_by_id (sqlite, style_id, &has_refs))
	      id = style_id;
	  else
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_raster_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Raster Style */
	  return do_delete_raster_style (sqlite, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Raster Style do actually exists */
	  if (!check_raster_style_refs_by_name
	      (sqlite, style_name, &id, &has_refs))
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_raster_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Raster Style */
	  return do_delete_raster_style (sqlite, id);
      }
    else
	return 0;
}

static int
check_raster_style_by_id (sqlite3 * sqlite, int style_id)
{
/* checks if a Raster Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_raster_styles " "WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Style by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_raster_style_by_name (sqlite3 * sqlite, const char *style_name,
			    sqlite3_int64 * id)
{
/* checks if a Raster Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_raster_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Style by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_reload_raster_style (sqlite3 * sqlite, sqlite3_int64 id,
			const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Raster Style definition */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to update the Raster Style */
	  sql = "UPDATE SE_raster_styles SET style = ? " "WHERE style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("reloadRasterStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_int64 (stmt, 2, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("reloadRasterStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
reload_raster_style (void *p_sqlite, int style_id,
		     const char *style_name,
		     const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Raster Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (style_id >= 0)
      {
	  /* checking if the Raster Style do actually exists */
	  if (check_raster_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* reloading the Raster Style */
	  if (raster_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_raster_style (sqlite, id, p_blob, n_bytes);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Raster Style do actually exists */
	  if (!check_raster_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* reloading the Raster Style */
	  if (raster_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_raster_style (sqlite, id, p_blob, n_bytes);
      }
    else
	return 0;
}

static int
do_insert_raster_style_layer (sqlite3 * sqlite, const char *coverage_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really inserting a Raster Styled Layer */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "INSERT INTO SE_raster_styled_layers "
	"(coverage_name, style_id) VALUES (?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerRasterStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerRasterStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_raster_styled_layer_ex (void *p_sqlite, const char *coverage_name,
				 int style_id, const char *style_name)
{
/* auxiliary function: inserts a Raster Styled Layer definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (coverage_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Raster Style do actually exists */
	  if (check_raster_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* inserting the Raster Styled Layer */
	  return do_insert_raster_style_layer (sqlite, coverage_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Raster Style do actually exists */
	  if (!check_raster_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* inserting the Raster Styled Layer */
	  return do_insert_raster_style_layer (sqlite, coverage_name, id);
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_raster_styled_layer (void *p_sqlite, const char *coverage_name,
			      int style_id, const unsigned char *p_blob,
			      int n_bytes)
{
/* auxiliary function: inserts a Raster Styled Layer definition - DEPRECATED */
    if (p_blob != NULL && n_bytes <= 0)
      {
	  /* silencing compiler complaints */
	  p_blob = NULL;
	  n_bytes = 0;
      }
    return register_raster_styled_layer_ex (p_sqlite, coverage_name, style_id,
					    NULL);
}

static int
check_raster_styled_layer_by_id (sqlite3 * sqlite, const char *coverage_name,
				 int style_id)
{
/* checks if a Raster Styled Layer do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_raster_styled_layers "
	"WHERE Lower(coverage_name) = Lower(?) AND style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Styled Layer by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_raster_styled_layer_by_name (sqlite3 * sqlite, const char *coverage_name,
				   const char *style_name, sqlite3_int64 * id)
{
/* checks if a Raster Styled Layer do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT l.style_id FROM SE_raster_styled_layers AS l "
	"JOIN SE_raster_styles AS s ON (l.style_id = s.style_id) "
	"WHERE Lower(l.coverage_name) = Lower(?) AND "
	"Lower(s.style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Styled Layer by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_delete_raster_style_layer (sqlite3 * sqlite, const char *coverage_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really deleting a Raster Styled Layer */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_raster_styled_layers "
	"WHERE Lower(coverage_name) = Lower(?) AND " "style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterRasterStyledLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterRasterStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_raster_styled_layer (void *p_sqlite, const char *coverage_name,
				int style_id, const char *style_name)
{
/* auxiliary function: removes a Raster Styled Layer definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (coverage_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Raster Styled Layer do actually exists */
	  if (check_raster_styled_layer_by_id (sqlite, coverage_name, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* removing the Raster Styled Layer */
	  return do_delete_raster_style_layer (sqlite, coverage_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Raster Styled Layer do actually exists */
	  if (!check_raster_styled_layer_by_name
	      (sqlite, coverage_name, style_name, &id))
	      return 0;
	  /* removing the Raster Styled Layer */
	  return do_delete_raster_style_layer (sqlite, coverage_name, id);
      }
    else
	return 0;
}

static int
check_styled_group (sqlite3 * sqlite, const char *group_name)
{
/* checking if the Group already exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;

    sql = "SELECT group_name FROM SE_styled_groups "
	"WHERE group_name = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("checkStyledGroup: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);
    return exists;
}

static int
do_insert_styled_group (sqlite3 * sqlite, const char *group_name,
			const char *title, const char *abstract)
{
/* inserting a Styled Group */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;

    if (title != NULL && abstract != NULL)
	sql =
	    "INSERT INTO SE_styled_groups (group_name, title, abstract) VALUES (?, ?, ?)";
    else
	sql = "INSERT INTO SE_styled_groups (group_name) VALUES (?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("insertStyledGroup: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    if (title != NULL && abstract != NULL)
      {
	  sqlite3_bind_text (stmt, 2, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, abstract, strlen (abstract),
			     SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("insertStyledGroup() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
}

static int
get_next_paint_order (sqlite3 * sqlite, const char *group_name)
{
/* retrieving the next available Paint Order for a Styled Group */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int paint_order = 0;

    sql = "SELECT Max(paint_order) FROM SE_styled_group_refs "
	"WHERE group_name = Lower(?) ";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("nextPaintOrder: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		    paint_order = sqlite3_column_int (stmt, 0) + 1;
	    }
      }
    sqlite3_finalize (stmt);
    return paint_order;
}

static int
get_next_paint_order_by_item (sqlite3 * sqlite, int item_id)
{
/* retrieving the next available Paint Order for a Styled Group - BY ITEM ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int paint_order = 0;

    sql = "SELECT Max(r.paint_order) FROM SE_styled_group_refs AS x "
	"JOIN SE_styled_groups AS g ON (x.group_name = g.group_name) "
	"JOIN SE_styled_group_refs AS r ON (r.group_name = g.group_name) "
	"WHERE x.id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("nextPaintOrderByItem: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, item_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		    paint_order = sqlite3_column_int (stmt, 0) + 1;
	    }
      }
    sqlite3_finalize (stmt);
    return paint_order;
}

SPATIALITE_PRIVATE int
register_styled_group_ex (void *p_sqlite, const char *group_name,
			  const char *vector_coverage_name,
			  const char *raster_coverage_name)
{
/* auxiliary function: inserts a Styled Group Item */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists_group = 0;
    int retval = 0;
    int paint_order;

    if (vector_coverage_name == NULL && raster_coverage_name == NULL)
	return 0;
    if (vector_coverage_name != NULL && raster_coverage_name != NULL)
	return 0;

    /* checking if the Raster Styled Layer do actually exists */
    exists_group = check_styled_group (sqlite, group_name);

    if (!exists_group)
      {
	  /* insert group */
	  retval = do_insert_styled_group (sqlite, group_name, NULL, NULL);
	  if (retval == 0)
	      goto stop;
	  retval = 0;
      }

    /* assigning the next paint_order value */
    paint_order = get_next_paint_order (sqlite, group_name);

    /* insert */
    if (vector_coverage_name != NULL)
      {
	  /* vector styled layer */
	  sql = "INSERT INTO SE_styled_group_refs "
	      "(id, group_name, vector_coverage_name, paint_order) "
	      "VALUES (NULL, ?, ?, ?)";
      }
    else
      {
	  /* raster styled layer */
	  sql = "INSERT INTO SE_styled_group_refs "
	      "(id, group_name, raster_coverage_name, paint_order) "
	      "VALUES (NULL, ?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerStyledGroupsRefs: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    /* insert */
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    if (vector_coverage_name != NULL)
      {
	  /* vector styled layer */
	  sqlite3_bind_text (stmt, 2, vector_coverage_name,
			     strlen (vector_coverage_name), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, paint_order);
      }
    else
      {
	  /* raster styled layer */
	  sqlite3_bind_text (stmt, 2, raster_coverage_name,
			     strlen (raster_coverage_name), SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 3, paint_order);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerStyledGroupsRefs() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_styled_group (void *p_sqlite, const char *group_name,
		       const char *f_table_name,
		       const char *f_geometry_column,
		       const char *coverage_name, int paint_order)
{
/* auxiliary function: inserts a Styled Group Item - DEPRECATED */
    if (paint_order < 0 || f_geometry_column != NULL)
      {
	  f_geometry_column = NULL;
	  paint_order = -1;	/* silencing compiler complaints */
      }
    return register_styled_group_ex (p_sqlite, group_name, f_table_name,
				     coverage_name);
}

SPATIALITE_PRIVATE int
styled_group_set_infos (void *p_sqlite, const char *group_name,
			const char *title, const char *abstract)
{
/* auxiliary function: inserts or updates the Styled Group descriptive infos */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (group_name == NULL)
	return 0;

    /* checking if the Raster Styled Layer do actually exists */
    exists = check_styled_group (sqlite, group_name);

    if (!exists)
      {
	  /* insert group */
	  retval = do_insert_styled_group (sqlite, group_name, title, abstract);
      }
    else
      {
	  /* update group */
	  sql =
	      "UPDATE SE_styled_groups SET title = ?, abstract = ? WHERE group_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("styledGroupSetInfos: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (title == NULL)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  if (abstract == NULL)
	      sqlite3_bind_null (stmt, 2);
	  else
	      sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
				 SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, group_name, strlen (group_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      retval = 1;
	  else
	      spatialite_e ("styledGroupSetInfos() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
      }
    return retval;
  stop:
    return 0;
}

static int
do_delete_styled_group (sqlite3 * sqlite, const char *group_name)
{
/* completely removing a Styled Group */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;

/* deleting Group Styles */
    sql =
	"DELETE FROM SE_styled_group_styles WHERE Lower(group_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("deleteStyledGroup: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("deleteStyledGroup() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    if (!retval)
	return 0;

/* deleting Group Items */
    retval = 0;
    sql = "DELETE FROM SE_styled_group_refs WHERE Lower(group_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("deleteStyledGroup: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("deleteStyledGroup() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    if (!retval)
	return 0;

/* deleting the Styled Group itself */
    retval = 0;
    sql = "DELETE FROM SE_styled_groups WHERE Lower(group_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("deleteStyledGroup: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("deleteStyledGroup() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
}

SPATIALITE_PRIVATE int
unregister_styled_group (void *p_sqlite, const char *group_name)
{
/* auxiliary function: completely removes a Styled Group definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (group_name == NULL)
	return 0;

    /* checking if the Raster Styled Layer do actually exists */
    if (!check_styled_group (sqlite, group_name))
	return 0;
    /* removing the Styled Group */
    return do_delete_styled_group (sqlite, group_name);
}

static int
check_styled_group_layer_by_id (sqlite3 * sqlite, int id)
{
/* checks if a Group Layer Item exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;

    sql = "SELECT id FROM SE_styled_group_refs " "WHERE id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("checkStyledGroupItem: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      exists = 1;
      }
    sqlite3_finalize (stmt);
    return exists;
}

static int
check_styled_group_raster (sqlite3 * sqlite, const char *group_name,
			   const char *coverage_name, sqlite3_int64 * id)
{
/* checks if a Styled Group Layer (Raster) do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT id FROM SE_styled_group_refs WHERE "
	"Lower(group_name) = Lower(?) AND Lower(raster_coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("checkStyledGroupRasterItem: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_styled_group_vector (sqlite3 * sqlite, const char *group_name,
			   const char *coverage_name, sqlite3_int64 * id)
{
/* checks if a Styled Group Layer (Vector) do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT id FROM SE_styled_group_refs WHERE "
	"Lower(group_name) = Lower(?) AND Lower(vector_coverage_name) = Lower(?) ";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("checkStyledGroupVectorItem: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_update_styled_group_layer_paint_order (sqlite3 * sqlite, sqlite3_int64 id,
					  int paint_order)
{
/* auxiliary function: really updating a Group Styled Layer Paint Order */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "UPDATE SE_styled_group_refs SET paint_order = ? " "WHERE id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updatePaintOrder: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, paint_order);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("updatePaintOrder error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
set_styled_group_layer_paint_order (void *p_sqlite, int item_id,
				    const char *group_name,
				    const char *vector_coverage_name,
				    const char *raster_coverage_name,
				    int paint_order)
{
/* auxiliary function: set the Paint Order for a Layer within a Styled Group */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;
    int pos = paint_order;

    if (vector_coverage_name != NULL && raster_coverage_name != NULL)
	return 0;

    if (item_id >= 0)
      {
	  /* checking if the Layer Item do actually exists */
	  if (check_styled_group_layer_by_id (sqlite, item_id))
	      id = item_id;
	  else
	      return 0;
	  if (pos < 0)
	      pos = get_next_paint_order_by_item (sqlite, item_id);
	  /* updating the Styled Group Layer Paint Order */
	  return do_update_styled_group_layer_paint_order (sqlite, id, pos);
      }
    else if (group_name != NULL && raster_coverage_name != NULL)
      {
	  /* checking if a Raster Layer Item do actually exists */
	  if (!check_styled_group_raster
	      (sqlite, group_name, raster_coverage_name, &id))
	      return 0;
	  if (pos < 0)
	      pos = get_next_paint_order (sqlite, group_name);
	  /* updating the Styled Group Layer Paint Order */
	  return do_update_styled_group_layer_paint_order (sqlite, id, pos);
      }
    else if (group_name != NULL && vector_coverage_name != NULL)
      {
	  /* checking if a Vector Layer Item do actually exists */
	  if (!check_styled_group_vector
	      (sqlite, group_name, vector_coverage_name, &id))
	      return 0;
	  if (pos < 0)
	      pos = get_next_paint_order (sqlite, group_name);
	  /* updating the Styled Group Layer Paint Order */
	  return do_update_styled_group_layer_paint_order (sqlite, id, pos);
      }
    else
	return 0;
}

static int
do_delete_styled_group_layer (sqlite3 * sqlite, sqlite3_int64 id)
{
/* completely removing a Styled Group */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;

    sql = "DELETE FROM SE_styled_group_refs WHERE id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("deleteStyledGroupLayer: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("deleteStyledGroupLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
}

SPATIALITE_PRIVATE int
unregister_styled_group_layer (void *p_sqlite, int item_id,
			       const char *group_name,
			       const char *vector_coverage_name,
			       const char *raster_coverage_name)
{
/* auxiliary function: removing a Layer form within a Styled Group */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (item_id >= 0)
      {
	  /* checking if the Layer Item do actually exists */
	  if (check_styled_group_layer_by_id (sqlite, item_id))
	      id = item_id;
	  else
	      return 0;
	  /* removing the Styled Group Layer */
	  return do_delete_styled_group_layer (sqlite, id);
      }
    else if (group_name != NULL && raster_coverage_name != NULL)
      {
	  /* checking if a Raster Layer Item do actually exists */
	  if (!check_styled_group_raster
	      (sqlite, group_name, raster_coverage_name, &id))
	      return 0;
	  /* removing the Styled Group Layer */
	  return do_delete_styled_group_layer (sqlite, id);
      }
    else if (group_name != NULL && vector_coverage_name != NULL)
      {
	  /* checking if a Vector Layer Item do actually exists */
	  if (!check_styled_group_vector
	      (sqlite, group_name, vector_coverage_name, &id))
	      return 0;
	  /* removing the Styled Group Layer */
	  return do_delete_styled_group_layer (sqlite, id);
      }
    else
	return 0;
}

static int
group_style_causes_duplicate_name (sqlite3 * sqlite, sqlite3_int64 id,
				   const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: checks for an eventual duplicate name */
    int count = 0;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "SELECT Count(*) FROM SE_group_styles "
	"WHERE Lower(style_name) = Lower(XB_GetName(?)) AND style_id <> ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("GroupStyle duplicate Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      count = sqlite3_column_int (stmt, 0);
      }
    sqlite3_finalize (stmt);
    if (count != 0)
	return 1;
    return 0;
}

SPATIALITE_PRIVATE int
register_group_style_ex (void *p_sqlite, const unsigned char *p_blob,
			 int n_bytes)
{
/* auxiliary function: inserts a Group Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to insert the Group Style */
	  if (group_style_causes_duplicate_name (sqlite, -1, p_blob, n_bytes))
	      return 0;
	  sql = "INSERT INTO SE_group_styles "
	      "(style_id, style) VALUES (NULL, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerGroupStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerGroupStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_group_style (void *p_sqlite, const char *group_name, int style_id,
		      const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: inserts a Group Style - DEPRECATED */
    if (group_name == NULL || style_id < 0)
      {
	  /* silencing compiler complaints */
	  group_name = NULL;
	  style_id = -1;
      }
    return register_group_style_ex (p_sqlite, p_blob, n_bytes);
}

static int
do_delete_group_style_refs (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: deleting all Group Style references */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_styled_group_styles WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterGroupStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterGroupStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

static int
check_group_style_refs_by_id (sqlite3 * sqlite, int style_id, int *has_refs)
{
/* checks if a Group Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;

    sql = "SELECT s.style_id, l.style_id FROM SE_group_styles AS s "
	"LEFT JOIN SE_styled_group_styles AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Group Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  if (ref_count > 0)
	      *has_refs = 1;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_group_style_refs_by_name (sqlite3 * sqlite, const char *style_name,
				sqlite3_int64 * id, int *has_refs)
{
/* checks if a Group Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int ref_count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_group_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Group Style Refs by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
	return 0;
    *id = xid;
    sql = "SELECT s.style_id, l.style_id FROM SE_group_styles AS s "
	"LEFT JOIN SE_styled_group_styles AS l ON (l.style_id = s.style_id) "
	"WHERE s.style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Group Style Refs by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, *id);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		    ref_count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (ref_count > 0)
	*has_refs = 1;
    return 1;
  stop:
    return 0;
}

static int
do_delete_group_style (sqlite3 * sqlite, sqlite3_int64 id)
{
/* auxiliary function: really deleting a Group Style */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_group_styles WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterGroupStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterGroupStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_group_style (void *p_sqlite, int style_id,
			const char *style_name, int remove_all)
{
/* auxiliary function: deletes a Group Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;
    int has_refs = 0;

    if (style_id >= 0)
      {
	  /* checking if the Group Style do actually exists */
	  if (check_group_style_refs_by_id (sqlite, style_id, &has_refs))
	      id = style_id;
	  else
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_group_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Group Style */
	  return do_delete_group_style (sqlite, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Group Style do actually exists */
	  if (!check_group_style_refs_by_name
	      (sqlite, style_name, &id, &has_refs))
	      return 0;
	  if (has_refs)
	    {
		if (!remove_all)
		    return 0;
		/* deleting all references */
		if (!do_delete_group_style_refs (sqlite, id))
		    return 0;
	    }
	  /* deleting the Group Style */
	  return do_delete_group_style (sqlite, id);
      }
    else
	return 0;
}

static int
check_group_style_by_id (sqlite3 * sqlite, int style_id)
{
/* checks if a Group Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_group_styles " "WHERE style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Group Style by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int (stmt, 1, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_group_style_by_name (sqlite3 * sqlite, const char *style_name,
			   sqlite3_int64 * id)
{
/* checks if a Group Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT style_id FROM SE_group_styles "
	"WHERE Lower(style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Group Style by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_reload_group_style (sqlite3 * sqlite, sqlite3_int64 id,
		       const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Group Style definition */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (p_blob != NULL && n_bytes > 0)
      {
	  /* attempting to update the Group Style */
	  sql = "UPDATE SE_group_styles SET style = ? " "WHERE style_id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("reloadGroupStyle: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_int64 (stmt, 2, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("reloadGroupStyle() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
reload_group_style (void *p_sqlite, int style_id,
		    const char *style_name,
		    const unsigned char *p_blob, int n_bytes)
{
/* auxiliary function: reloads a Group Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (style_id >= 0)
      {
	  /* checking if the Group Style do actually exists */
	  if (check_group_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* reloading the Group Style */
	  if (group_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_group_style (sqlite, id, p_blob, n_bytes);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Group Style do actually exists */
	  if (!check_group_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* reloading the Group Style */
	  if (group_style_causes_duplicate_name (sqlite, id, p_blob, n_bytes))
	      return 0;
	  return do_reload_group_style (sqlite, id, p_blob, n_bytes);
      }
    else
	return 0;
}

static int
do_insert_styled_group_style (sqlite3 * sqlite, const char *group_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really inserting a Styled Group Style */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "INSERT INTO SE_styled_group_styles "
	"(group_name, style_id) VALUES (?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerStyledGroupStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerGroupStyledLayer() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_styled_group_style (void *p_sqlite, const char *group_name,
			     int style_id, const char *style_name)
{
/* auxiliary function: inserts a Styled Group Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (group_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Group Style do actually exists */
	  if (check_group_style_by_id (sqlite, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* inserting the Styled Group Style */
	  return do_insert_styled_group_style (sqlite, group_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Group Style do actually exists */
	  if (!check_group_style_by_name (sqlite, style_name, &id))
	      return 0;
	  /* inserting the Styled Group Style */
	  return do_insert_styled_group_style (sqlite, group_name, id);
      }
    else
	return 0;
}

static int
check_styled_group_style_by_id (sqlite3 * sqlite, const char *group_name,
				int style_id)
{
/* checks if a Styled Group Style do actually exists - by ID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT style_id FROM SE_styled_group_styles "
	"WHERE Lower(group_name) = Lower(?) AND style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Styled Group Style by ID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, style_id);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_styled_group_style_by_name (sqlite3 * sqlite, const char *group_name,
				  const char *style_name, sqlite3_int64 * id)
{
/* checks if a Styled Group Style do actually exists - by name */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    sqlite3_int64 xid = 0;

    sql = "SELECT l.style_id FROM SE_styled_group_styles AS l "
	"JOIN SE_group_styles AS s ON (l.style_id = s.style_id) "
	"WHERE Lower(l.group_name) = Lower(?) AND "
	"Lower(s.style_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Styled Group Style by Name: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, style_name, strlen (style_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		xid = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
      {
	  *id = xid;
	  return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
do_delete_styled_group_style (sqlite3 * sqlite, const char *group_name,
			      sqlite3_int64 id)
{
/* auxiliary function: really deleting a Styled Group Style */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM SE_styled_group_styles "
	"WHERE Lower(group_name) = Lower(?) AND " "style_id = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterStyledGroupStyle: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, group_name, strlen (group_name), SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 2, id);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterStyledGroupStyle() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_styled_group_style (void *p_sqlite, const char *group_name,
			       int style_id, const char *style_name)
{
/* auxiliary function: removes a Styled Group Style definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 id;

    if (group_name == NULL)
	return 0;

    if (style_id >= 0)
      {
	  /* checking if the Styled Group Style do actually exists */
	  if (check_styled_group_style_by_id (sqlite, group_name, style_id))
	      id = style_id;
	  else
	      return 0;
	  /* removing the Styled Group Style */
	  return do_delete_styled_group_style (sqlite, group_name, id);
      }
    else if (style_name != NULL)
      {
	  /* checking if the Styled Group Style do actually exists */
	  if (!check_styled_group_style_by_name
	      (sqlite, group_name, style_name, &id))
	      return 0;
	  /* removing the Styled Group Style */
	  return do_delete_styled_group_style (sqlite, group_name, id);
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_vector_coverage (void *p_sqlite, const char *coverage_name,
			  const char *f_table_name,
			  const char *f_geometry_column, const char *title,
			  const char *abstract, int is_queryable,
			  int is_editable)
{
/* auxiliary function: inserts a Vector Coverage definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name != NULL && f_table_name != NULL
	&& f_geometry_column != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, title, "
	      "abstract, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 6, is_queryable);
	  sqlite3_bind_int (stmt, 7, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (coverage_name != NULL && f_table_name != NULL
	     && f_geometry_column != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, "
	      "is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 4, is_queryable);
	  sqlite3_bind_int (stmt, 5, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_spatial_view_coverage (void *p_sqlite, const char *coverage_name,
				const char *view_name,
				const char *view_geometry, const char *title,
				const char *abstract, int is_queryable,
				int is_editable)
{
/* auxiliary function: inserts a Spatial View Coverage definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name != NULL && view_name != NULL
	&& view_geometry != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, view_name, view_geometry, title, "
	      "abstract, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, view_name, strlen (view_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, view_geometry,
			     strlen (view_geometry), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 6, is_queryable);
	  sqlite3_bind_int (stmt, 7, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (coverage_name != NULL && view_name != NULL
	     && view_geometry != NULL)
      {
	  /* attempting to insert the Spatial View Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, view_name, view_geometry, "
	      "is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, view_name, strlen (view_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, view_geometry,
			     strlen (view_geometry), SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 4, is_queryable);
	  sqlite3_bind_int (stmt, 5, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_virtual_shp_coverage (void *p_sqlite, const char *coverage_name,
			       const char *virt_name, const char *virt_geometry,
			       const char *title, const char *abstract,
			       int is_queryable)
{
/* auxiliary function: inserts a VirtualShapefile Coverage definition */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name != NULL && virt_name != NULL
	&& virt_geometry != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, virt_name, virt_geometry, title, "
	      "abstract, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, virt_name, strlen (virt_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, virt_geometry,
			     strlen (virt_geometry), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  sqlite3_bind_int (stmt, 6, is_queryable);
	  sqlite3_bind_int (stmt, 7, 0);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (coverage_name != NULL && virt_name != NULL
	     && virt_geometry != NULL)
      {
	  /* attempting to insert the VirtualShapefile Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, virt_name, virt_geometry, "
	      "is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerVectorCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, virt_name, strlen (virt_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, virt_geometry,
			     strlen (virt_geometry), SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  sqlite3_bind_int (stmt, 4, is_queryable);
	  sqlite3_bind_int (stmt, 5, 0);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerVectorCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
register_topogeo_coverage (void *p_sqlite, const char *coverage_name,
			   const char *topogeo_name, const char *title,
			   const char *abstract, int is_queryable,
			   int is_editable)
{
/* auxiliary function: inserts a Vector Coverage definition 
 * based on some Topology-Geometry */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    char *xsql;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *f_table_name = NULL;
    char *f_geometry_column = NULL;
    sqlite3_stmt *stmt;

    if (topogeo_name == NULL)
	return 0;

/* testing if the Topology-Geometry do really exist */
    xsql = sqlite3_mprintf ("SELECT topology_name "
			    "FROM topologies WHERE Lower(topology_name) = %Q",
			    topogeo_name);
    ret = sqlite3_get_table (sqlite, xsql, &results, &rows, &columns, &errMsg);
    sqlite3_free (xsql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  if (f_table_name != NULL)
	      sqlite3_free (f_table_name);
	  if (f_geometry_column != NULL)
	      sqlite3_free (f_geometry_column);
	  f_table_name = sqlite3_mprintf ("%s_edge", value);
	  f_geometry_column = sqlite3_mprintf ("geom");
      }
    sqlite3_free_table (results);

    if (coverage_name != NULL && f_table_name != NULL
	&& f_geometry_column != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, "
	      "topology_name, title, abstract, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), Lower(?), ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerTopoGeoCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     sqlite3_free);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), sqlite3_free);
	  sqlite3_bind_text (stmt, 4, topogeo_name, strlen (topogeo_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 6, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 7, is_queryable);
	  sqlite3_bind_int (stmt, 8, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerTopoGeoCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (coverage_name != NULL && f_table_name != NULL
	     && f_geometry_column != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, "
	      "topology_name, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), Lower(?), ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerTopoGeoCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     sqlite3_free);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), sqlite3_free);
	  sqlite3_bind_text (stmt, 4, topogeo_name, strlen (topogeo_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 5, is_queryable);
	  sqlite3_bind_int (stmt, 6, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerTopoGeoCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
      {
	  if (f_table_name != NULL)
	      sqlite3_free (f_table_name);
	  if (f_geometry_column != NULL)
	      sqlite3_free (f_geometry_column);
	  return 0;
      }
}

SPATIALITE_PRIVATE int
register_toponet_coverage (void *p_sqlite, const char *coverage_name,
			   const char *toponet_name, const char *title,
			   const char *abstract, int is_queryable,
			   int is_editable)
{
/* auxiliary function: inserts a Vector Coverage definition  
 * based on some Topology-Network */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    char *xsql;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *f_table_name = NULL;
    char *f_geometry_column = NULL;
    sqlite3_stmt *stmt;

    if (toponet_name == NULL)
	return 0;

/* testing if the Topology-Network do really exist */
    xsql = sqlite3_mprintf ("SELECT network_name "
			    "FROM networks WHERE Lower(network_name) = %Q",
			    toponet_name);
    ret = sqlite3_get_table (sqlite, xsql, &results, &rows, &columns, &errMsg);
    sqlite3_free (xsql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  if (f_table_name != NULL)
	      sqlite3_free (f_table_name);
	  if (f_geometry_column != NULL)
	      sqlite3_free (f_geometry_column);
	  f_table_name = sqlite3_mprintf ("%s_link", value);
	  f_geometry_column = sqlite3_mprintf ("geometry");
      }
    sqlite3_free_table (results);

    if (coverage_name != NULL && f_table_name != NULL
	&& f_geometry_column != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, "
	      "network_name, title, abstract, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), Lower(?), ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerTopoNetCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     sqlite3_free);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), sqlite3_free);
	  sqlite3_bind_text (stmt, 4, toponet_name, strlen (toponet_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 6, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  if (is_queryable)
	      is_queryable = 1;
	  if (is_editable)
	      is_editable = 1;
	  sqlite3_bind_int (stmt, 7, is_queryable);
	  sqlite3_bind_int (stmt, 8, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerTopoNetCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (coverage_name != NULL && f_table_name != NULL
	     && f_geometry_column != NULL)
      {
	  /* attempting to insert the Vector Coverage */
	  sql = "INSERT INTO vector_coverages "
	      "(coverage_name, f_table_name, f_geometry_column, "
	      "network_name, is_queryable, is_editable) VALUES "
	      "(Lower(?), Lower(?), Lower(?), Lower(?), ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerTopoNetCoverage: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, f_table_name, strlen (f_table_name),
			     sqlite3_free);
	  sqlite3_bind_text (stmt, 3, f_geometry_column,
			     strlen (f_geometry_column), sqlite3_free);
	  sqlite3_bind_text (stmt, 4, toponet_name, strlen (toponet_name),
			     SQLITE_STATIC);
	  sqlite3_bind_int (stmt, 5, is_queryable);
	  sqlite3_bind_int (stmt, 6, is_editable);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("registerTopoNetCoverage() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
      {
	  if (f_table_name != NULL)
	      sqlite3_free (f_table_name);
	  if (f_geometry_column != NULL)
	      sqlite3_free (f_geometry_column);
	  return 0;
      }
}

static int
check_vector_coverage (sqlite3 * sqlite, const char *coverage_name)
{
/* checks if a Vector Coverage do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT coverage_name FROM vector_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Coverage: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static void
do_delete_vector_coverage_srid (sqlite3 * sqlite, const char *coverage_name,
				int srid)
{
/* auxiliary function: deleting a Vector Coverage alternative SRID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (srid < 0)
	sql = "DELETE FROM vector_coverages_srid "
	    "WHERE Lower(coverage_name) = Lower(?)";
    else
	sql = "DELETE FROM vector_coverages_srid "
	    "WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorCoverageSrid: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    if (srid >= 0)
	sqlite3_bind_int (stmt, 2, srid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterVectorCoverageSrid() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

static void
do_delete_vector_coverage_keyword (sqlite3 * sqlite, const char *coverage_name,
				   const char *keyword)
{
/* auxiliary function: deleting an Vector Coverage Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (keyword == NULL)
	sql = "DELETE FROM vector_coverages_keyword "
	    "WHERE Lower(coverage_name) = Lower(?)";
    else
	sql = "DELETE FROM vector_coverages_keyword "
	    "WHERE Lower(coverage_name) = Lower(?) AND Lower(keyword) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorCoverageKeyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    if (keyword != NULL)
	sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterVectorCoverageKeyword() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

static void
do_delete_vector_coverage_styled_layers (sqlite3 * sqlite,
					 const char *coverage_name)
{
/* auxiliary function: deleting all Vector Coverage Styled references */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    sql = "DELETE FROM SE_vector_styled_layers WHERE coverage_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorCoverageStyles: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterVectorCoverageStyles() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

static void
do_delete_vector_coverage_styled_groups (sqlite3 * sqlite,
					 const char *coverage_name)
{
/* auxiliary function: deleting all Vector Coverage Styled Group references */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    sql = "DELETE FROM SE_styled_group_refs WHERE vector_coverage_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorCoverageGroups: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterVectorCoverageGroups() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

static int
do_delete_vector_coverage (sqlite3 * sqlite, const char *coverage_name)
{
/* auxiliary function: deleting a Vector Coverage */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int retval = 0;
    sql = "DELETE FROM vector_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterVectorCoverage: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("unregisterVectorCoverage() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
unregister_vector_coverage (void *p_sqlite, const char *coverage_name)
{
/* auxiliary function: deletes a Vector Coverage definition (and any related) */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (coverage_name == NULL)
	return 0;

    /* checking if the Vector Coverage do actually exists */
    if (!check_vector_coverage (sqlite, coverage_name))
	return 0;
    /* deleting all alternative SRIDs */
    do_delete_vector_coverage_srid (sqlite, coverage_name, -1);
    /* deleting all Keywords */
    do_delete_vector_coverage_keyword (sqlite, coverage_name, NULL);
    /* deleting all Styled Layers */
    do_delete_vector_coverage_styled_layers (sqlite, coverage_name);
    /* deleting all Styled Group references */
    do_delete_vector_coverage_styled_groups (sqlite, coverage_name);
    /* deleting the Vector Coverage itself */
    return do_delete_vector_coverage (sqlite, coverage_name);
}

SPATIALITE_PRIVATE int
set_vector_coverage_infos (void *p_sqlite, const char *coverage_name,
			   const char *title, const char *abstract,
			   int is_queryable, int is_editable)
{
/* auxiliary function: updates the descriptive infos supporting a Vector Coverage */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int prev_changes;
    int curr_changes;

    if (coverage_name != NULL && title != NULL && abstract != NULL)
      {
	  prev_changes = sqlite3_total_changes (sqlite);

	  /* attempting to update the Vector Coverage */
	  if (is_queryable < 0 || is_editable < 0)
	    {
		sql = "UPDATE vector_coverages SET title = ?, abstract = ? "
		    "WHERE Lower(coverage_name) = Lower(?)";
		ret =
		    sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("setVectorCoverageInfos: \"%s\"\n",
				    sqlite3_errmsg (sqlite));
		      return 0;
		  }
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 3, coverage_name,
				   strlen (coverage_name), SQLITE_STATIC);
	    }
	  else
	    {
		sql = "UPDATE vector_coverages SET title = ?, abstract = ?, "
		    "is_queryable = ?, is_editable = ? WHERE Lower(coverage_name) = Lower(?)";
		ret =
		    sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("setVectorCoverageInfos: \"%s\"\n",
				    sqlite3_errmsg (sqlite));
		      return 0;
		  }
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, title, strlen (title),
				   SQLITE_STATIC);
		sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
				   SQLITE_STATIC);
		if (is_queryable)
		    is_queryable = 1;
		if (is_editable)
		    is_editable = 1;
		sqlite3_bind_int (stmt, 3, is_queryable);
		sqlite3_bind_int (stmt, 4, is_editable);
		sqlite3_bind_text (stmt, 5, coverage_name,
				   strlen (coverage_name), SQLITE_STATIC);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("setVectorCoverageInfos() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);

	  curr_changes = sqlite3_total_changes (sqlite);
	  if (prev_changes == curr_changes)
	      return 0;
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
set_vector_coverage_copyright (void *p_sqlite, const char *coverage_name,
			       const char *copyright, const char *license)
{
/* auxiliary function: updates the copyright infos supporting a Vector Coverage */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name == NULL)
	return 0;
    if (copyright == NULL && license == NULL)
	return 1;

    if (copyright == NULL)
      {
	  /* just updating the License */
	  sql = "UPDATE vector_coverages SET license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setVectorCoverageCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name,
			     strlen (coverage_name), SQLITE_STATIC);
      }
    else if (license == NULL)
      {
	  /* just updating the Copyright */
	  sql = "UPDATE vector_coverages SET copyright = ? "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setVectorCoverageCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    else
      {
	  /* updating both Copyright and License */
	  sql = "UPDATE vector_coverages SET copyright = ?, license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE Lower(coverage_name) = Lower(?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setVectorCoverageCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, coverage_name,
			     strlen (coverage_name), SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("setVectorCoverageCopyright() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

static int
check_vector_coverage_srid2 (sqlite3 * sqlite, const char *coverage_name,
			     int srid)
{
/* checks if a Vector Coverage SRID do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT srid FROM vector_coverages_srid "
	"WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Coverage SRID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
find_vector_coverage_type (sqlite3 * sqlite, const char *coverage_name)
{
/* determining the Vector Coverage Type */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *value1;
    char *value2;
    int type = VECTOR_UNKNOWN;
    char *sql;

    sql =
	sqlite3_mprintf
	("SELECT f_table_name, f_geometry_column, view_name, view_geometry, "
	 "virt_name, virt_geometry, topology_name, network_name "
	 "FROM vector_coverages WHERE coverage_name = %Q", coverage_name);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return VECTOR_UNKNOWN;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value1 = results[(i * columns) + 0];
		value2 = results[(i * columns) + 1];
		if (value1 != NULL && value2 != NULL)
		    type = VECTOR_GEOTABLE;
		value1 = results[(i * columns) + 2];
		value2 = results[(i * columns) + 3];
		if (value1 != NULL && value2 != NULL)
		    type = VECTOR_SPATIALVIEW;
		value1 = results[(i * columns) + 4];
		value2 = results[(i * columns) + 5];
		if (value1 != NULL && value2 != NULL)
		    type = VECTOR_VIRTUALSHP;
		value1 = results[(i * columns) + 6];
		if (value1 != NULL)
		    type = VECTOR_TOPOGEO;
		value1 = results[(i * columns) + 7];
		if (value1 != NULL)
		    type = VECTOR_TOPONET;
	    }
      }
    sqlite3_free_table (results);
    return type;
}

static int
check_vector_coverage_srid1 (sqlite3 * sqlite, const char *coverage_name,
			     int srid)
{
/* checks if a Vector Coverage do actually exists and check its SRID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int same_srid = 0;
    int type = find_vector_coverage_type (sqlite, coverage_name);

    switch (type)
      {
      case VECTOR_GEOTABLE:
	  sql = sqlite3_mprintf ("SELECT c.srid FROM vector_coverages AS v "
				 "JOIN geometry_columns AS c ON (v.f_table_name IS NOT NULL AND v.f_geometry_column IS NOT NULL "
				 "AND v.topology_name IS NULL AND v.network_name IS NULL AND "
				 "Lower(v.f_table_name) = Lower(c.f_table_name) "
				 "AND Lower(v.f_geometry_column) = Lower(c.f_geometry_column)) "
				 "WHERE Lower(v.coverage_name) = Lower(%Q)",
				 coverage_name);
	  break;
      case VECTOR_SPATIALVIEW:
	  sql = sqlite3_mprintf ("SELECT c.srid FROM vector_coverages AS v "
				 "JOIN views_geometry_columns AS x ON (v.view_name IS NOT NULL AND v.view_geometry IS NOT NULL "
				 "AND Lower(v.view_name) = Lower(x.view_name) AND Lower(v.view_geometry) = Lower(x.view_geometry)) "
				 "JOIN geometry_columns AS c ON (Lower(x.f_table_name) = Lower(c.f_table_name) "
				 "AND Lower(x.f_geometry_column) = Lower(c.f_geometry_column)) "
				 "WHERE Lower(v.coverage_name) = Lower(%Q)",
				 coverage_name);
	  break;
      case VECTOR_VIRTUALSHP:
	  sql = sqlite3_mprintf ("SELECT c.srid FROM vector_coverages AS v "
				 "JOIN virts_geometry_columns AS c ON (v.virt_name IS NOT NULL AND v.virt_geometry IS NOT NULL "
				 "AND Lower(v.virt_name) = Lower(c.virt_name) AND Lower(v.virt_geometry) = Lower(c.virt_geometry)) "
				 "WHERE Lower(v.coverage_name) = Lower(%Q)",
				 coverage_name);
	  break;
      case VECTOR_TOPOGEO:
	  sql = sqlite3_mprintf ("SELECT c.srid FROM vector_coverages AS v "
				 "JOIN topologies AS c ON (v.topology_name IS NOT NULL "
				 "AND Lower(v.topology_name) = Lower(c.topology_name)) "
				 "WHERE Lower(v.coverage_name) = Lower(%Q)",
				 coverage_name);
	  break;
      case VECTOR_TOPONET:
	  sql = sqlite3_mprintf ("SELECT c.srid FROM vector_coverages AS v "
				 "JOIN networks AS c ON (v.network_name IS NOT NULL "
				 "AND Lower(v.network_name) = Lower(c.network_name)) "
				 "WHERE Lower(v.coverage_name) = Lower(%Q)",
				 coverage_name);
	  break;
      case VECTOR_UNKNOWN:
      default:
	  goto stop;
	  break;
      };

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto stop;
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int natural_srid = sqlite3_column_int (stmt, 0);
		if (srid == natural_srid)
		    same_srid++;
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1 && same_srid == 0)
      {
	  if (check_vector_coverage_srid2 (sqlite, coverage_name, srid))
	      return 0;
	  else
	      return 1;
      }
    return 0;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_vector_coverage_srid (void *p_sqlite, const char *coverage_name,
			       int srid)
{
/* auxiliary function: inserting a Vector Coverage alternative SRID */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name == NULL)
	return 0;
    if (srid <= 0)
	return 0;

    /* checking if the Vector Coverage do actually exists */
    if (!check_vector_coverage_srid1 (sqlite, coverage_name, srid))
	return 0;

    /* attempting to insert the Vector Coverage alternative SRID */
    sql = "INSERT INTO vector_coverages_srid "
	"(coverage_name, srid) VALUES (Lower(?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerVectorCoverageSrid: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("registerVectorCoverageSrid() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

SPATIALITE_PRIVATE int
unregister_vector_coverage_srid (void *p_sqlite, const char *coverage_name,
				 int srid)
{
/* auxiliary function: deletes a Vector Coverage alternative SRID */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (coverage_name == NULL)
	return 0;

    /* checking if the Vector Coverage SRID do actually exists */
    if (!check_vector_coverage_srid2 (sqlite, coverage_name, srid))
	return 0;
    /* deleting the alternative SRID */
    do_delete_vector_coverage_srid (sqlite, coverage_name, srid);
    return 1;
}

static int
check_vector_coverage_keyword0 (sqlite3 * sqlite, const char *coverage_name)
{
/* checks if a Vector Coverage do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql =
	"SELECT coverage_name FROM vector_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
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
    if (count == 0)
	return 0;
    return 1;
  stop:
    return 0;
}

static int
check_vector_coverage_keyword1 (sqlite3 * sqlite, const char *coverage_name,
				const char *keyword)
{
/* checks if a Vector Coverage do actually exists and check the Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int same_kw = 0;

    sql =
	"SELECT keyword FROM vector_coverages_keyword WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *kw = (const char *) sqlite3_column_text (stmt, 0);
		if (strcasecmp (kw, keyword) == 0)
		    same_kw++;
	    }
      }
    sqlite3_finalize (stmt);
    if (same_kw == 0)
      {
	  if (!check_vector_coverage_keyword0 (sqlite, coverage_name))
	      return 0;
	  else
	      return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_vector_coverage_keyword2 (sqlite3 * sqlite, const char *coverage_name,
				const char *keyword)
{
/* checks if a Vector Coverage do actually exists and check the Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT keyword FROM vector_coverages_keyword "
	"WHERE Lower(coverage_name) = Lower(?) AND Lower(keyword) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Vector Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
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
    if (count == 0)
	return 0;
    return 1;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_vector_coverage_keyword (void *p_sqlite, const char *coverage_name,
				  const char *keyword)
{
/* auxiliary function: inserting a Vector Coverage Keyword */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name == NULL)
	return 0;
    if (keyword == NULL)
	return 0;

    /* checking if the Vector Coverage do actually exists */
    if (!check_vector_coverage_keyword1 (sqlite, coverage_name, keyword))
	return 0;

    /* attempting to insert the Vector Coverage Keyword */
    sql = "INSERT INTO vector_coverages_keyword "
	"(coverage_name, keyword) VALUES (Lower(?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerVectorCoverageKeyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("registerVectorCoverageKeyword() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

SPATIALITE_PRIVATE int
unregister_vector_coverage_keyword (void *p_sqlite, const char *coverage_name,
				    const char *keyword)
{
/* auxiliary function: deletes a Vector Coverage Keyword */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (coverage_name == NULL)
	return 0;
    if (keyword == NULL)
	return 0;

    /* checking if the Vector Coverage Keyword do actually exists */
    if (!check_vector_coverage_keyword2 (sqlite, coverage_name, keyword))
	return 0;
    /* deleting the Keyword */
    do_delete_vector_coverage_keyword (sqlite, coverage_name, keyword);
    return 1;
}

static int
do_null_vector_coverage_extents (sqlite3 * sqlite, sqlite3_stmt * stmt_upd_cvg,
				 sqlite3_stmt * stmt_null_srid,
				 const char *coverage_name)
{
/* setting the main Coverage Extent to NULL */
    int ret;
    sqlite3_reset (stmt_upd_cvg);
    sqlite3_clear_bindings (stmt_upd_cvg);
    sqlite3_bind_null (stmt_upd_cvg, 1);
    sqlite3_bind_null (stmt_upd_cvg, 2);
    sqlite3_bind_null (stmt_upd_cvg, 3);
    sqlite3_bind_null (stmt_upd_cvg, 4);
    sqlite3_bind_null (stmt_upd_cvg, 5);
    sqlite3_bind_null (stmt_upd_cvg, 6);
    sqlite3_bind_null (stmt_upd_cvg, 7);
    sqlite3_bind_null (stmt_upd_cvg, 8);
    sqlite3_bind_text (stmt_upd_cvg, 9, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_upd_cvg);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateVectorCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
/* setting all alternativ Coverage Extent to NULL */
    sqlite3_reset (stmt_null_srid);
    sqlite3_clear_bindings (stmt_null_srid);
    sqlite3_bind_text (stmt_null_srid, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_null_srid);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateVectorCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    return 1;
}

static int
do_update_vector_coverage_extents (sqlite3 * sqlite, const void *cache,
				   sqlite3_stmt * stmt_upd_cvg,
				   sqlite3_stmt * stmt_srid,
				   sqlite3_stmt * stmt_upd_srid,
				   const char *coverage_name, int natural_srid,
				   double minx, double miny, double maxx,
				   double maxy)
{
/* updating the Coverage Extents */
    int ret;
    int geographic = 0;
    double geo_minx = minx;
    double geo_miny = miny;
    double geo_maxx = maxx;
    double geo_maxy = maxy;
    char *proj_from = NULL;
    char *proj_to = NULL;
    gaiaGeomCollPtr in;
    gaiaGeomCollPtr out;
    gaiaPointPtr pt;

    getProjParams (sqlite, natural_srid, &proj_from);
    if (proj_from == NULL)
	goto error;

    ret = srid_is_geographic (sqlite, natural_srid, &geographic);
    if (!ret)
	return 0;
    if (!geographic)
      {
	  /* computing the geographic extent */
	  getProjParams (sqlite, 4326, &proj_to);
	  if (proj_to == NULL)
	      goto error;
	  in = gaiaAllocGeomColl ();
	  in->Srid = natural_srid;
	  gaiaAddPointToGeomColl (in, minx, miny);
	  if (cache != NULL)
	      out = gaiaTransform_r (cache, in, proj_from, proj_to);
	  else
	      out = gaiaTransform (in, proj_from, proj_to);
	  if (out == NULL)
	    {
		gaiaFreeGeomColl (in);
		goto error;
	    }
	  pt = out->FirstPoint;
	  if (pt == NULL)
	    {
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		goto error;
	    }
	  geo_minx = pt->X;
	  geo_miny = pt->Y;
	  gaiaFreeGeomColl (in);
	  gaiaFreeGeomColl (out);
	  in = gaiaAllocGeomColl ();
	  in->Srid = natural_srid;
	  gaiaAddPointToGeomColl (in, maxx, maxy);
	  if (cache != NULL)
	      out = gaiaTransform_r (cache, in, proj_from, proj_to);
	  else
	      out = gaiaTransform (in, proj_from, proj_to);
	  if (out == NULL)
	    {
		gaiaFreeGeomColl (in);
		goto error;
	    }
	  pt = out->FirstPoint;
	  if (pt == NULL)
	    {
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		goto error;
	    }
	  geo_maxx = pt->X;
	  geo_maxy = pt->Y;
	  gaiaFreeGeomColl (in);
	  gaiaFreeGeomColl (out);
	  free (proj_to);
	  proj_to = NULL;
      }

/* setting the main Coverage Extent */
    sqlite3_reset (stmt_upd_cvg);
    sqlite3_clear_bindings (stmt_upd_cvg);
    sqlite3_bind_double (stmt_upd_cvg, 1, geo_minx);
    sqlite3_bind_double (stmt_upd_cvg, 2, geo_miny);
    sqlite3_bind_double (stmt_upd_cvg, 3, geo_maxx);
    sqlite3_bind_double (stmt_upd_cvg, 4, geo_maxy);
    sqlite3_bind_double (stmt_upd_cvg, 5, minx);
    sqlite3_bind_double (stmt_upd_cvg, 6, miny);
    sqlite3_bind_double (stmt_upd_cvg, 7, maxx);
    sqlite3_bind_double (stmt_upd_cvg, 8, maxy);
    sqlite3_bind_text (stmt_upd_cvg, 9, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_upd_cvg);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateVectorCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

/* updating any alternative SRID supporting this Vector Coverage */
    sqlite3_reset (stmt_srid);
    sqlite3_clear_bindings (stmt_srid);
    sqlite3_bind_text (stmt_srid, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_srid);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a single alternative SRID Extent */
		double alt_minx;
		double alt_miny;
		double alt_maxx;
		double alt_maxy;
		int srid = sqlite3_column_int (stmt_srid, 0);
		getProjParams (sqlite, srid, &proj_to);
		if (proj_to == NULL)
		    goto error;
		in = gaiaAllocGeomColl ();
		in->Srid = natural_srid;
		gaiaAddPointToGeomColl (in, minx, miny);
		if (cache != NULL)
		    out = gaiaTransform_r (cache, in, proj_from, proj_to);
		else
		    out = gaiaTransform (in, proj_from, proj_to);
		if (out == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      goto error;
		  }
		pt = out->FirstPoint;
		if (pt == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      gaiaFreeGeomColl (out);
		      goto error;
		  }
		alt_minx = pt->X;
		alt_miny = pt->Y;
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		in = gaiaAllocGeomColl ();
		in->Srid = natural_srid;
		gaiaAddPointToGeomColl (in, maxx, maxy);
		if (cache != NULL)
		    out = gaiaTransform_r (cache, in, proj_from, proj_to);
		else
		    out = gaiaTransform (in, proj_from, proj_to);
		if (out == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      goto error;
		  }
		pt = out->FirstPoint;
		if (pt == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      gaiaFreeGeomColl (out);
		      goto error;
		  }
		alt_maxx = pt->X;
		alt_maxy = pt->Y;
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		free (proj_to);
		proj_to = NULL;

/* setting the alternative Srid Extent */
		sqlite3_reset (stmt_upd_srid);
		sqlite3_clear_bindings (stmt_upd_srid);
		sqlite3_bind_double (stmt_upd_srid, 1, alt_minx);
		sqlite3_bind_double (stmt_upd_srid, 2, alt_miny);
		sqlite3_bind_double (stmt_upd_srid, 3, alt_maxx);
		sqlite3_bind_double (stmt_upd_srid, 4, alt_maxy);
		sqlite3_bind_text (stmt_upd_srid, 5, coverage_name,
				   strlen (coverage_name), SQLITE_STATIC);
		sqlite3_bind_int (stmt_upd_srid, 6, srid);
		ret = sqlite3_step (stmt_upd_srid);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e
			  ("updateVectorCoverageExtent error: \"%s\"\n",
			   sqlite3_errmsg (sqlite));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("updateVectorCoverageExtent() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

    free (proj_from);
    return 1;

  error:
    if (proj_from)
	free (proj_from);
    if (proj_to)
	free (proj_to);
    return 0;
}

SPATIALITE_PRIVATE int
update_vector_coverage_extent (void *p_sqlite, const void *cache,
			       const char *coverage_name, int transaction)
{
/* updates one (or all) Vector Coverage Extents */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_ext = NULL;
    sqlite3_stmt *stmt_upd_cvg = NULL;
    sqlite3_stmt *stmt_upd_srid = NULL;
    sqlite3_stmt *stmt_null_srid = NULL;
    sqlite3_stmt *stmt_srid = NULL;
    sqlite3_stmt *stmt_virt = NULL;

/* preparing the ancillary SQL statements */
    sql = "SELECT srid FROM vector_coverages_srid "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE vector_coverages SET geo_minx = ?, geo_miny = ?, "
	"geo_maxx = ?, geo_maxy = ?, extent_minx = ?, extent_miny = ?, "
	"extent_maxx = ?, extent_maxy = ? "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_cvg, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE vector_coverages_srid SET extent_minx = NULL, "
	"extent_miny = NULL, extent_maxx = NULL, extent_maxy = NULL "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_null_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE vector_coverages_srid SET extent_minx = ?, "
	"extent_miny = ?, extent_maxx = ?, extent_maxy = ? "
	"WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

/* preparing the main SQL statement */
    if (coverage_name == NULL)
      {
	  sql = "SELECT v.coverage_name, v.f_table_name, v.f_geometry_column, "
	      "c.srid FROM vector_coverages AS v "
	      "JOIN geometry_columns AS c ON (Lower(v.f_table_name) = "
	      "Lower(c.f_table_name) AND Lower(v.f_geometry_column) = "
	      "Lower(c.f_geometry_column)) "
	      "WHERE v.f_table_name IS NOT NULL AND v.f_geometry_column IS NOT NULL "
	      "UNION "
	      "SELECT v.coverage_name, w.f_table_name, w.f_geometry_column, c.srid "
	      "FROM vector_coverages AS v "
	      "JOIN views_geometry_columns AS w ON "
	      "(Lower(v.view_name) = Lower(w.view_name) AND "
	      "Lower(v.view_geometry) = Lower(w.view_geometry)) "
	      "JOIN geometry_columns AS c ON "
	      "(Lower(w.f_table_name) = Lower(c.f_table_name) AND "
	      "Lower(w.f_geometry_column) = Lower(c.f_geometry_column)) "
	      "WHERE v.view_name IS NOT NULL AND v.view_geometry IS NOT NULL";
      }
    else
      {
	  sql = "SELECT v.coverage_name, v.f_table_name, v.f_geometry_column, "
	      "c.srid FROM vector_coverages AS v "
	      "JOIN geometry_columns AS c ON (Lower(v.f_table_name) = "
	      "Lower(c.f_table_name) AND Lower(v.f_geometry_column) = "
	      "Lower(c.f_geometry_column)) "
	      "WHERE Lower(v.coverage_name) = Lower(?) AND "
	      "v.f_table_name IS NOT NULL AND v.f_geometry_column IS NOT NULL "
	      "UNION "
	      "SELECT v.coverage_name, w.f_table_name, w.f_geometry_column, c.srid "
	      "FROM vector_coverages AS v "
	      "JOIN views_geometry_columns AS w ON "
	      "(Lower(v.view_name) = Lower(w.view_name) AND "
	      "Lower(v.view_geometry) = Lower(w.view_geometry)) "
	      "JOIN geometry_columns AS c ON "
	      "(Lower(w.f_table_name) = Lower(c.f_table_name) AND "
	      "Lower(w.f_geometry_column) = Lower(c.f_geometry_column)) "
	      "WHERE Lower(v.coverage_name) = Lower(?) AND "
	      "v.view_name IS NOT NULL AND v.view_geometry IS NOT NULL";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    if (transaction)
      {
	  /* starting a Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (coverage_name != NULL)
      {
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a single Vector Coverage */
		char *table;
		char *geom;
		const char *cvg = (const char *) sqlite3_column_text (stmt, 0);
		const char *xtable =
		    (const char *) sqlite3_column_text (stmt, 1);
		const char *xgeom =
		    (const char *) sqlite3_column_text (stmt, 2);
		int natural_srid = sqlite3_column_int (stmt, 3);
		table = gaiaDoubleQuotedSql (xtable);
		geom = gaiaDoubleQuotedSql (xgeom);
		sql =
		    sqlite3_mprintf
		    ("SELECT Min(MbrMinX(\"%s\")), Min(MbrMinY(\"%s\")), "
		     "Max(MbrMaxX(\"%s\")), Max(MbrMaxY(\"%s\")) "
		     "FROM \"%s\"", geom, geom, geom, geom, table);
		free (table);
		free (geom);
		ret =
		    sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
				    sqlite3_errmsg (sqlite));
		      goto error;
		  }
		while (1)
		  {
		      /* scrolling the result set rows */
		      ret = sqlite3_step (stmt_ext);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    int null_minx = 1;
			    int null_miny = 1;
			    int null_maxx = 1;
			    int null_maxy = 1;
			    double minx = 0.0;
			    double miny = 0.0;
			    double maxx = 0.0;
			    double maxy = 0.0;
			    if (sqlite3_column_type (stmt_ext, 0) ==
				SQLITE_FLOAT)
			      {
				  minx = sqlite3_column_double (stmt_ext, 0);
				  null_minx = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 1) ==
				SQLITE_FLOAT)
			      {
				  miny = sqlite3_column_double (stmt_ext, 1);
				  null_miny = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 2) ==
				SQLITE_FLOAT)
			      {
				  maxx = sqlite3_column_double (stmt_ext, 2);
				  null_maxx = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 3) ==
				SQLITE_FLOAT)
			      {
				  maxy = sqlite3_column_double (stmt_ext, 3);
				  null_maxy = 0;
			      }
			    if (null_minx || null_miny || null_maxx
				|| null_maxy)
				ret =
				    do_null_vector_coverage_extents (sqlite,
								     stmt_upd_cvg,
								     stmt_null_srid,
								     cvg);
			    else
				ret =
				    do_update_vector_coverage_extents (sqlite,
								       cache,
								       stmt_upd_cvg,
								       stmt_srid,
								       stmt_upd_srid,
								       cvg,
								       natural_srid,
								       minx,
								       miny,
								       maxx,
								       maxy);
			    if (!ret)
				goto error;
			}
		      else
			{
			    spatialite_e
				("updateVectorCoverageExtent() error: \"%s\"\n",
				 sqlite3_errmsg (sqlite));
			    goto error;
			}
		  }
		sqlite3_finalize (stmt_ext);
		stmt_ext = NULL;
	    }
	  else
	    {
		spatialite_e ("updateVectorCoverageExtent() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

/* updating VirtualShapes Extents */
    sql = "UPDATE vector_coverages SET "
	"geo_minx = MbrMinX(ST_Transform(GetShapefileExtent(virt_name), 4326)), "
	"geo_miny = MbrMinY(ST_Transform(GetShapefileExtent(virt_name), 4326)), "
	"geo_maxx = MbrMaxX(ST_Transform(GetShapefileExtent(virt_name), 4326)), "
	"geo_maxy = MbrMaxY(ST_Transform(GetShapefileExtent(virt_name), 4326)), "
	"extent_minx = MbrMinX(GetShapefileExtent(virt_name)), "
	"extent_miny = MbrMinY(GetShapefileExtent(virt_name)), "
	"extent_maxx = MbrMaxX(GetShapefileExtent(virt_name)), "
	"extent_maxy = MbrMaxY(GetShapefileExtent(virt_name)) "
	"WHERE virt_name IS NOT NULL";
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql =
	"SELECT coverage_name, virt_name FROM vector_coverages WHERE virt_name IS NOT NULL";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_virt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateVectorCoverageExtent: PREPUPPAMELO !!! \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_virt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *coverage_name =
		    (const char *) sqlite3_column_text (stmt_virt, 0);
		const char *virt_name =
		    (const char *) sqlite3_column_text (stmt_virt, 1);
		sql =
		    sqlite3_mprintf ("UPDATE vector_coverages_srid SET "
				     "extent_minx = MbrMinX(ST_Transform(GetShapefileExtent(%Q), srid)), "
				     "extent_miny = MbrMinY(ST_Transform(GetShapefileExtent(%Q), srid)), "
				     "extent_maxx = MbrMaxX(ST_Transform(GetShapefileExtent(%Q), srid)), "
				     "extent_maxy = MbrMaxY(ST_Transform(GetShapefileExtent(%Q), srid)) "
				     "WHERE coverage_name = %Q", virt_name,
				     virt_name, virt_name, virt_name,
				     coverage_name);
		ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("updateVectorCoverageExtent: PUPPAMELO !!! %d \"%s\"\n", ret,
				    sqlite3_errmsg (sqlite));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e
		    ("updateVectorCoverageExtent() error: \"%s\"\n",
		     sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

    if (transaction)
      {
	  /* committing the still pending Transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_upd_cvg);
    sqlite3_finalize (stmt_upd_srid);
    sqlite3_finalize (stmt_null_srid);
    sqlite3_finalize (stmt_srid);
    sqlite3_finalize (stmt_virt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_ext != NULL)
	sqlite3_finalize (stmt_ext);
    if (stmt_upd_cvg != NULL)
	sqlite3_finalize (stmt_upd_cvg);
    if (stmt_upd_srid != NULL)
	sqlite3_finalize (stmt_upd_srid);
    if (stmt_null_srid != NULL)
	sqlite3_finalize (stmt_null_srid);
    if (stmt_srid != NULL)
	sqlite3_finalize (stmt_srid);
    if (stmt_virt != NULL)
	sqlite3_finalize (stmt_virt);

    return 0;
}

static int
check_raster_coverage_srid2 (sqlite3 * sqlite, const char *coverage_name,
			     int srid)
{
/* checks if a Raster Coverage SRID do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT srid FROM raster_coverages_srid "
	"WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Coverage SRID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
check_raster_coverage_srid1 (sqlite3 * sqlite, const char *coverage_name,
			     int srid)
{
/* checks if a Raster Coverage do actually exists and check its SRID */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;
    int same_srid = 0;

    sql = "SELECT srid FROM raster_coverages "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Coverage SRID: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int natural_srid = sqlite3_column_int (stmt, 0);
		if (srid == natural_srid)
		    same_srid++;
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1 && same_srid == 0)
      {
	  if (check_raster_coverage_srid2 (sqlite, coverage_name, srid))
	      return 0;
	  else
	      return 1;
      }
    return 0;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_raster_coverage_srid (void *p_sqlite, const char *coverage_name,
			       int srid)
{
/* auxiliary function: inserting a Raster Coverage alternative SRID */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name == NULL)
	return 0;
    if (srid <= 0)
	return 0;

    /* checking if the Raster Coverage do actually exists */
    if (!check_raster_coverage_srid1 (sqlite, coverage_name, srid))
	return 0;

    /* attempting to insert the Raster Coverage alternative SRID */
    sql = "INSERT INTO raster_coverages_srid "
	"(coverage_name, srid) VALUES (Lower(?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerRasterCoverageSrid: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("registerRasterCoverageSrid() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

static void
do_delete_raster_coverage_srid (sqlite3 * sqlite, const char *coverage_name,
				int srid)
{
/* auxiliary function: deleting all Raster Coverage alternative SRIDs */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM raster_coverages_srid "
	"WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterRasterCoverageSrid: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, srid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterRasterCoverageSrid() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

SPATIALITE_PRIVATE int
unregister_raster_coverage_srid (void *p_sqlite, const char *coverage_name,
				 int srid)
{
/* auxiliary function: deletes a Raster Coverage alternative SRID */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (coverage_name == NULL)
	return 0;

    /* checking if the Raster Coverage SRID do actually exists */
    if (!check_raster_coverage_srid2 (sqlite, coverage_name, srid))
	return 0;
    /* deleting the alternative SRID */
    do_delete_raster_coverage_srid (sqlite, coverage_name, srid);
    return 1;
}

static int
check_raster_coverage_keyword0 (sqlite3 * sqlite, const char *coverage_name)
{
/* checks if a Raster Coverage do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql =
	"SELECT coverage_name FROM raster_coverages WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
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
    if (count == 0)
	return 0;
    return 1;
  stop:
    return 0;
}

static int
check_raster_coverage_keyword1 (sqlite3 * sqlite, const char *coverage_name,
				const char *keyword)
{
/* checks if a Raster Coverage do actually exists and check the Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int same_kw = 0;

    sql =
	"SELECT keyword FROM raster_coverages_keyword WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *kw = (const char *) sqlite3_column_text (stmt, 0);
		if (strcasecmp (kw, keyword) == 0)
		    same_kw++;
	    }
      }
    sqlite3_finalize (stmt);
    if (same_kw == 0)
      {
	  if (!check_raster_coverage_keyword0 (sqlite, coverage_name))
	      return 0;
	  else
	      return 1;
      }
    return 0;
  stop:
    return 0;
}

static int
check_raster_coverage_keyword2 (sqlite3 * sqlite, const char *coverage_name,
				const char *keyword)
{
/* checks if a Raster Coverage do actually exists and check the Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT keyword FROM raster_coverages_keyword "
	"WHERE Lower(coverage_name) = Lower(?) AND Lower(keyword) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check Raster Coverage Keyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
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
    if (count == 0)
	return 0;
    return 1;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_raster_coverage_keyword (void *p_sqlite, const char *coverage_name,
				  const char *keyword)
{
/* auxiliary function: inserting a Raster Coverage Keyword */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (coverage_name == NULL)
	return 0;
    if (keyword == NULL)
	return 0;

    /* checking if the Raster Coverage do actually exists */
    if (!check_raster_coverage_keyword1 (sqlite, coverage_name, keyword))
	return 0;

    /* attempting to insert the Raster Coverage Keyword */
    sql = "INSERT INTO raster_coverages_keyword "
	"(coverage_name, keyword) VALUES (Lower(?), ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerRasterCoverageKeyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("registerRasterCoverageKeyword() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

static void
do_delete_raster_coverage_keyword (sqlite3 * sqlite, const char *coverage_name,
				   const char *keyword)
{
/* auxiliary function: deleting all Raster Coverage Keyword */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM raster_coverages_keyword "
	"WHERE Lower(coverage_name) = Lower(?) AND Lower(keyword) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterRasterCoverageKeyword: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, keyword, strlen (keyword), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("unregisterRasterCoverageKeyword() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

SPATIALITE_PRIVATE int
unregister_raster_coverage_keyword (void *p_sqlite, const char *coverage_name,
				    const char *keyword)
{
/* auxiliary function: deletes a Raster Coverage Keyword */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (coverage_name == NULL)
	return 0;
    if (keyword == NULL)
	return 0;

    /* checking if the Raster Coverage Keyword do actually exists */
    if (!check_raster_coverage_keyword2 (sqlite, coverage_name, keyword))
	return 0;
    /* deleting the Keyword */
    do_delete_raster_coverage_keyword (sqlite, coverage_name, keyword);
    return 1;
}

static int
do_null_raster_coverage_extents (sqlite3 * sqlite, sqlite3_stmt * stmt_upd_cvg,
				 sqlite3_stmt * stmt_null_srid,
				 const char *coverage_name)
{
/* setting the main Coverage Extent to NULL */
    int ret;
    sqlite3_reset (stmt_upd_cvg);
    sqlite3_clear_bindings (stmt_upd_cvg);
    sqlite3_bind_null (stmt_upd_cvg, 1);
    sqlite3_bind_null (stmt_upd_cvg, 2);
    sqlite3_bind_null (stmt_upd_cvg, 3);
    sqlite3_bind_null (stmt_upd_cvg, 4);
    sqlite3_bind_null (stmt_upd_cvg, 5);
    sqlite3_bind_null (stmt_upd_cvg, 6);
    sqlite3_bind_null (stmt_upd_cvg, 7);
    sqlite3_bind_null (stmt_upd_cvg, 8);
    sqlite3_bind_text (stmt_upd_cvg, 9, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_upd_cvg);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateRasterCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
/* setting all alternativ Coverage Extent to NULL */
    sqlite3_reset (stmt_null_srid);
    sqlite3_clear_bindings (stmt_null_srid);
    sqlite3_bind_text (stmt_null_srid, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_null_srid);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateRasterCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    return 1;
}

static int
do_update_raster_coverage_extents (sqlite3 * sqlite, const void *cache,
				   sqlite3_stmt * stmt_upd_cvg,
				   sqlite3_stmt * stmt_srid,
				   sqlite3_stmt * stmt_upd_srid,
				   const char *coverage_name, int natural_srid,
				   double minx, double miny, double maxx,
				   double maxy)
{
/* updating the Coverage Extents */
    int ret;
    int geographic = 0;
    double geo_minx = minx;
    double geo_miny = miny;
    double geo_maxx = maxx;
    double geo_maxy = maxy;
    char *proj_from = NULL;
    char *proj_to = NULL;
    gaiaGeomCollPtr in;
    gaiaGeomCollPtr out;
    gaiaPointPtr pt;

    getProjParams (sqlite, natural_srid, &proj_from);
    if (proj_from == NULL)
	goto error;

    ret = srid_is_geographic (sqlite, natural_srid, &geographic);
    if (!ret)
	return 0;
    if (!geographic)
      {
	  /* computing the geographic extent */
	  getProjParams (sqlite, 4326, &proj_to);
	  if (proj_to == NULL)
	      goto error;
	  in = gaiaAllocGeomColl ();
	  in->Srid = natural_srid;
	  gaiaAddPointToGeomColl (in, minx, miny);
	  if (cache != NULL)
	      out = gaiaTransform_r (cache, in, proj_from, proj_to);
	  else
	      out = gaiaTransform (in, proj_from, proj_to);
	  if (out == NULL)
	    {
		gaiaFreeGeomColl (in);
		goto error;
	    }
	  pt = out->FirstPoint;
	  if (pt == NULL)
	    {
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		goto error;
	    }
	  geo_minx = pt->X;
	  geo_miny = pt->Y;
	  gaiaFreeGeomColl (in);
	  gaiaFreeGeomColl (out);
	  in = gaiaAllocGeomColl ();
	  in->Srid = natural_srid;
	  gaiaAddPointToGeomColl (in, maxx, maxy);
	  if (cache != NULL)
	      out = gaiaTransform_r (cache, in, proj_from, proj_to);
	  else
	      out = gaiaTransform (in, proj_from, proj_to);
	  if (out == NULL)
	    {
		gaiaFreeGeomColl (in);
		goto error;
	    }
	  pt = out->FirstPoint;
	  if (pt == NULL)
	    {
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		goto error;
	    }
	  geo_maxx = pt->X;
	  geo_maxy = pt->Y;
	  gaiaFreeGeomColl (in);
	  gaiaFreeGeomColl (out);
	  free (proj_to);
	  proj_to = NULL;
      }

/* setting the main Coverage Extent */
    sqlite3_reset (stmt_upd_cvg);
    sqlite3_clear_bindings (stmt_upd_cvg);
    sqlite3_bind_double (stmt_upd_cvg, 1, geo_minx);
    sqlite3_bind_double (stmt_upd_cvg, 2, geo_miny);
    sqlite3_bind_double (stmt_upd_cvg, 3, geo_maxx);
    sqlite3_bind_double (stmt_upd_cvg, 4, geo_maxy);
    sqlite3_bind_double (stmt_upd_cvg, 5, minx);
    sqlite3_bind_double (stmt_upd_cvg, 6, miny);
    sqlite3_bind_double (stmt_upd_cvg, 7, maxx);
    sqlite3_bind_double (stmt_upd_cvg, 8, maxy);
    sqlite3_bind_text (stmt_upd_cvg, 9, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    ret = sqlite3_step (stmt_upd_cvg);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("updateRasterCoverageExtent error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

/* updating any alternative SRID supporting this Raster Coverage */
    sqlite3_reset (stmt_srid);
    sqlite3_clear_bindings (stmt_srid);
    sqlite3_bind_text (stmt_srid, 1, coverage_name, strlen (coverage_name),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_srid);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a single alternative SRID Extent */
		double alt_minx;
		double alt_miny;
		double alt_maxx;
		double alt_maxy;
		int srid = sqlite3_column_int (stmt_srid, 0);
		getProjParams (sqlite, srid, &proj_to);
		if (proj_to == NULL)
		    goto error;
		in = gaiaAllocGeomColl ();
		in->Srid = natural_srid;
		gaiaAddPointToGeomColl (in, minx, miny);
		if (cache != NULL)
		    out = gaiaTransform_r (cache, in, proj_from, proj_to);
		else
		    out = gaiaTransform (in, proj_from, proj_to);
		if (out == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      goto error;
		  }
		pt = out->FirstPoint;
		if (pt == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      gaiaFreeGeomColl (out);
		      goto error;
		  }
		alt_minx = pt->X;
		alt_miny = pt->Y;
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		in = gaiaAllocGeomColl ();
		in->Srid = natural_srid;
		gaiaAddPointToGeomColl (in, maxx, maxy);
		if (cache != NULL)
		    out = gaiaTransform_r (cache, in, proj_from, proj_to);
		else
		    out = gaiaTransform (in, proj_from, proj_to);
		if (out == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      goto error;
		  }
		pt = out->FirstPoint;
		if (pt == NULL)
		  {
		      gaiaFreeGeomColl (in);
		      gaiaFreeGeomColl (out);
		      goto error;
		  }
		alt_maxx = pt->X;
		alt_maxy = pt->Y;
		gaiaFreeGeomColl (in);
		gaiaFreeGeomColl (out);
		free (proj_to);
		proj_to = NULL;

/* setting the alternative Srid Extent */
		sqlite3_reset (stmt_upd_srid);
		sqlite3_clear_bindings (stmt_upd_srid);
		sqlite3_bind_double (stmt_upd_srid, 1, alt_minx);
		sqlite3_bind_double (stmt_upd_srid, 2, alt_miny);
		sqlite3_bind_double (stmt_upd_srid, 3, alt_maxx);
		sqlite3_bind_double (stmt_upd_srid, 4, alt_maxy);
		sqlite3_bind_text (stmt_upd_srid, 5, coverage_name,
				   strlen (coverage_name), SQLITE_STATIC);
		sqlite3_bind_int (stmt_upd_srid, 6, srid);
		ret = sqlite3_step (stmt_upd_srid);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e
			  ("updateRasterCoverageExtent error: \"%s\"\n",
			   sqlite3_errmsg (sqlite));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("updateRasterCoverageExtent() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

    free (proj_from);
    return 1;

  error:
    if (proj_from)
	free (proj_from);
    if (proj_to)
	free (proj_to);
    return 0;
}

SPATIALITE_PRIVATE int
update_raster_coverage_extent (void *p_sqlite, const void *cache,
			       const char *coverage_name, int transaction)
{
/* updates one (or all) Raster Coverage Extents */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    char *sql;
    sqlite3_stmt *stmt = NULL;
    sqlite3_stmt *stmt_ext = NULL;
    sqlite3_stmt *stmt_upd_cvg = NULL;
    sqlite3_stmt *stmt_upd_srid = NULL;
    sqlite3_stmt *stmt_null_srid = NULL;
    sqlite3_stmt *stmt_srid = NULL;

/* preparing the ancillary SQL statements */
    sql = "SELECT srid FROM raster_coverages_srid "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE raster_coverages SET geo_minx = ?, geo_miny = ?, "
	"geo_maxx = ?, geo_maxy = ?, extent_minx = ?, extent_miny = ?, "
	"extent_maxx = ?, extent_maxy = ? "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_cvg, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE raster_coverages_srid SET extent_minx = NULL, "
	"extent_miny = NULL, extent_maxx = NULL, extent_maxy = NULL "
	"WHERE Lower(coverage_name) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_null_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    sql = "UPDATE raster_coverages_srid SET extent_minx = ?, "
	"extent_miny = ?, extent_maxx = ?, extent_maxy = ? "
	"WHERE Lower(coverage_name) = Lower(?) AND srid = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd_srid, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

/* preparing the main SQL statement */
    if (coverage_name == NULL)
      {
	  sql = "SELECT coverage_name, srid FROM raster_coverages";
      }
    else
      {

	  sql = "SELECT coverage_name, srid FROM raster_coverages "
	      "WHERE Lower(coverage_name) = Lower(?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto error;
      }

    if (transaction)
      {
	  /* starting a Transaction */
	  ret = sqlite3_exec (sqlite, "BEGIN", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (coverage_name != NULL)
	sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			   SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* processing a single Raster Coverage */
		char *xtile_table;
		char *tile_table;
		const char *cvg = (const char *) sqlite3_column_text (stmt, 0);
		int natural_srid = sqlite3_column_int (stmt, 1);
		xtile_table = sqlite3_mprintf ("%s_tiles", cvg);
		tile_table = gaiaDoubleQuotedSql (xtile_table);
		sqlite3_free (xtile_table);
		sql =
		    sqlite3_mprintf
		    ("SELECT Min(MbrMinX(geometry)), Min(MbrMinY(geometry)), "
		     "Max(MbrMaxX(geometry)), Max(MbrMaxY(geometry)) FROM \"%s\"",
		     tile_table);
		free (tile_table);
		ret =
		    sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_ext,
					NULL);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("updateRasterCoverageExtent: \"%s\"\n",
				    sqlite3_errmsg (sqlite));
		      goto error;
		  }
		while (1)
		  {
		      /* scrolling the result set rows */
		      ret = sqlite3_step (stmt_ext);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    int null_minx = 1;
			    int null_miny = 1;
			    int null_maxx = 1;
			    int null_maxy = 1;
			    double minx = 0.0;
			    double miny = 0.0;
			    double maxx = 0.0;
			    double maxy = 0.0;
			    if (sqlite3_column_type (stmt_ext, 0) ==
				SQLITE_FLOAT)
			      {
				  minx = sqlite3_column_double (stmt_ext, 0);
				  null_minx = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 1) ==
				SQLITE_FLOAT)
			      {
				  miny = sqlite3_column_double (stmt_ext, 1);
				  null_miny = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 2) ==
				SQLITE_FLOAT)
			      {
				  maxx = sqlite3_column_double (stmt_ext, 2);
				  null_maxx = 0;
			      }
			    if (sqlite3_column_type (stmt_ext, 3) ==
				SQLITE_FLOAT)
			      {
				  maxy = sqlite3_column_double (stmt_ext, 3);
				  null_maxy = 0;
			      }
			    if (null_minx || null_miny || null_maxx
				|| null_maxy)
				ret =
				    do_null_raster_coverage_extents (sqlite,
								     stmt_upd_cvg,
								     stmt_null_srid,
								     cvg);
			    else
				ret =
				    do_update_raster_coverage_extents (sqlite,
								       cache,
								       stmt_upd_cvg,
								       stmt_srid,
								       stmt_upd_srid,
								       cvg,
								       natural_srid,
								       minx,
								       miny,
								       maxx,
								       maxy);
			    if (!ret)
				goto error;
			}
		      else
			{
			    spatialite_e
				("updateRasterCoverageExtent() error: \"%s\"\n",
				 sqlite3_errmsg (sqlite));
			    goto error;
			}
		  }
		sqlite3_finalize (stmt_ext);
		stmt_ext = NULL;
	    }
	  else
	    {
		spatialite_e ("updateRasterCoverageExtent() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto error;
	    }
      }

    if (transaction)
      {
	  /* committing the still pending Transaction */
	  ret = sqlite3_exec (sqlite, "COMMIT", NULL, NULL, NULL);
	  if (ret != SQLITE_OK)
	      goto error;
      }

    sqlite3_finalize (stmt);
    sqlite3_finalize (stmt_upd_cvg);
    sqlite3_finalize (stmt_upd_srid);
    sqlite3_finalize (stmt_null_srid);
    sqlite3_finalize (stmt_srid);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (stmt_ext != NULL)
	sqlite3_finalize (stmt_ext);
    if (stmt_upd_cvg != NULL)
	sqlite3_finalize (stmt_upd_cvg);
    if (stmt_upd_srid != NULL)
	sqlite3_finalize (stmt_upd_srid);
    if (stmt_null_srid != NULL)
	sqlite3_finalize (stmt_null_srid);
    if (stmt_srid != NULL)
	sqlite3_finalize (stmt_srid);
    return 0;
}

SPATIALITE_PRIVATE int
get_iso_metadata_id (void *p_sqlite, const char *fileIdentifier, void *p_id)
{
/* auxiliary function: return the ID of the row corresponding to "fileIdentifier" */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 *p64 = (sqlite3_int64 *) p_id;
    sqlite3_int64 id = 0;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int ok = 0;

    sql = "SELECT id FROM ISO_metadata WHERE fileId = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("getIsoMetadataId: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, fileIdentifier, strlen (fileIdentifier),
		       SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		ok++;
		id = sqlite3_column_int64 (stmt, 0);
	    }
      }
    sqlite3_finalize (stmt);

    if (ok == 1)
      {
	  *p64 = id;
	  return 1;
      }
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_iso_metadata (void *p_sqlite, const char *scope,
		       const unsigned char *p_blob, int n_bytes, void *p_id,
		       const char *fileIdentifier)
{
/* auxiliary function: inserts or updates an ISO Metadata */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    sqlite3_int64 *p64 = (sqlite3_int64 *) p_id;
    sqlite3_int64 id = *p64;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int exists = 0;
    int retval = 0;

    if (id >= 0)
      {
	  /* checking if already exists - by ID */
	  sql = "SELECT id FROM ISO_metadata WHERE id = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerIsoMetadata: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		    exists = 1;
	    }
	  sqlite3_finalize (stmt);
      }
    if (fileIdentifier != NULL)
      {
	  /* checking if already exists - by fileIdentifier */
	  sql = "SELECT id FROM ISO_metadata WHERE fileId = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("registerIsoMetadata: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		goto stop;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, fileIdentifier, strlen (fileIdentifier),
			     SQLITE_STATIC);
	  while (1)
	    {
		/* scrolling the result set rows */
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE)
		    break;	/* end of result set */
		if (ret == SQLITE_ROW)
		  {
		      exists = 1;
		      id = sqlite3_column_int64 (stmt, 0);
		  }
	    }
	  sqlite3_finalize (stmt);
      }

    if (exists)
      {
	  /* update */
	  sql = "UPDATE ISO_metadata SET md_scope = ?, metadata = ? "
	      "WHERE id = ?";
      }
    else
      {
	  /* insert */
	  sql = "INSERT INTO ISO_metadata "
	      "(id, md_scope, metadata) VALUES (?, ?, ?)";
      }
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerIsoMetadata: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    if (exists)
      {
	  /* update */
	  sqlite3_bind_text (stmt, 1, scope, strlen (scope), SQLITE_STATIC);
	  sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, SQLITE_STATIC);
	  sqlite3_bind_int64 (stmt, 3, id);
      }
    else
      {
	  /* insert */
	  if (id < 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, id);
	  sqlite3_bind_text (stmt, 2, scope, strlen (scope), SQLITE_STATIC);
	  sqlite3_bind_blob (stmt, 3, p_blob, n_bytes, SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	retval = 1;
    else
	spatialite_e ("registerIsoMetadata() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return retval;
  stop:
    return 0;
}

#endif /* end including LIBXML2 */

SPATIALITE_PRIVATE int
register_wms_getcapabilities (void *p_sqlite, const char *url,
			      const char *title, const char *abstract)
{
/* auxiliary function: inserts a WMS GetCapabilities */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL && title != NULL && abstract != NULL)
      {
	  /* attempting to insert the WMS GetCapabilities */
	  sql = "INSERT INTO wms_getcapabilities (url, title, abstract) "
	      "VALUES (?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_RegisterGetCapabilities: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_RegisterGetCapabilities() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else if (url != NULL)
      {
	  /* attempting to insert the WMS GetCapabilities */
	  sql = "INSERT INTO wms_getcapabilities (url) VALUES (?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_RegisterGetCapabilities: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_RegisterGetCapabilities() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

static int
check_wms_getcapabilities (sqlite3 * sqlite, const char *url)
{
/* checks if a WMS GetCapabilities do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT url FROM wms_getcapabilities WHERE url = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check WMS GetCapabilities: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
do_delete_wms_getcapabilities (sqlite3 * sqlite, const char *url)
{
/* auxiliary function: deleting a WMS GetCapabilities */
    int ret;
    int result = 0;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM wms_getcapabilities WHERE url = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterGetCapabilities: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_UnRegisterGetCapabilities() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return result;
}

static void
do_delete_wms_settings_0 (sqlite3 * sqlite, const char *url)
{
/* auxiliary function: deleting WMS settings (from GetCapabilities) */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM wms_settings WHERE id IN ("
	"SELECT s.id FROM wms_getcapabilities AS c "
	"JOIN wms_getmap AS m ON (c.id = m.parent_id) "
	"JOIN wms_settings AS s ON (m.id = s.parent_id) " "WHERE c.url = ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterGetCapabilities: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("WMS_UnRegisterGetCapabilities() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

static void
do_delete_wms_getmap_0 (sqlite3 * sqlite, const char *url)
{
/* auxiliary function: deleting WMS GetMap (from GetCapabilities) */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM wms_getmap WHERE id IN ("
	"SELECT m.id FROM wms_getcapabilities AS c "
	"JOIN wms_getmap AS m ON (c.id = m.parent_id) " "WHERE c.url = ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterGetCapabilities: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("WMS_UnRegisterGetCapabilities() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

SPATIALITE_PRIVATE int
unregister_wms_getcapabilities (void *p_sqlite, const char *url)
{
/* auxiliary function: deletes a WMS GetCapabilities definition (and any related) */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL)
	return 0;

    /* checking if the WMS GetCapabilities do actually exists */
    if (!check_wms_getcapabilities (sqlite, url))
	return 0;
    /* deleting all WMS settings */
    do_delete_wms_settings_0 (sqlite, url);
    /* deleting all WMS GetMap */
    do_delete_wms_getmap_0 (sqlite, url);
    /* deleting the WMS GetCapability itself */
    return do_delete_wms_getcapabilities (sqlite, url);
}

SPATIALITE_PRIVATE int
set_wms_getcapabilities_infos (void *p_sqlite, const char *url,
			       const char *title, const char *abstract)
{
/* auxiliary function: updates the descriptive infos supporting a WMS GetCapabilities */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL && title != NULL && abstract != NULL)
      {
	  /* checking if the WMS GetCapabilities do actually exists */
	  if (!check_wms_getcapabilities (sqlite, url))
	      return 0;

	  /* attempting to update the WMS GetCapabilities */
	  sql =
	      "UPDATE wms_getcapabilities SET title = ?, abstract = ? WHERE url = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetCapabilitiesInfos: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, url, strlen (url), SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_SetGetCapabilitiesInfos() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

static int
wms_getmap_parentid (sqlite3 * sqlite, const char *url, sqlite3_int64 * id)
{
/* retieving the WMS GetCapabilities ID value */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT id FROM wms_getcapabilities WHERE url = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("GetMap parent_id: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*id = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

SPATIALITE_PRIVATE int
register_wms_getmap (void *p_sqlite, const char *getcapabilities_url,
		     const char *getmap_url, const char *layer_name,
		     const char *title, const char *abstract,
		     const char *version, const char *ref_sys,
		     const char *image_format, const char *style,
		     int transparent, int flip_axes, int tiled, int cached,
		     int tile_width, int tile_height, const char *bgcolor,
		     int is_queryable, const char *getfeatureinfo_url)
{
/* auxiliary function: inserts a WMS GetMap */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    sqlite3_int64 parent_id;
    const char *sql;
    sqlite3_stmt *stmt;

    if (getcapabilities_url == NULL)
	return 0;

    if (!wms_getmap_parentid (sqlite, getcapabilities_url, &parent_id))
      {
	  spatialite_e ("WMS_RegisterGetMap: missing parent GetCapabilities\n");
	  return 0;
      }

    if (getmap_url != NULL && layer_name != NULL && title != NULL
	&& abstract != NULL)
      {
	  /* attempting to insert the WMS GetMap */
	  sql =
	      "INSERT INTO wms_getmap (parent_id, url, layer_name, title, abstract, "
	      "version, srs, format, style, transparent, flip_axes, tiled, "
	      "is_cached, tile_width, tile_height, bgcolor, is_queryable, "
	      "getfeatureinfo_url) VALUES "
	      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_RegisterGetMap: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, parent_id);
	  sqlite3_bind_text (stmt, 2, getmap_url, strlen (getmap_url),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 6, version, strlen (version), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 7, ref_sys, strlen (ref_sys), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 8, image_format, strlen (image_format),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 9, style, strlen (style), SQLITE_STATIC);
	  if (transparent != 0)
	      transparent = 1;
	  sqlite3_bind_int (stmt, 10, transparent);
	  if (flip_axes != 0)
	      flip_axes = 1;
	  sqlite3_bind_int (stmt, 11, flip_axes);
	  if (tiled != 0)
	      tiled = 1;
	  sqlite3_bind_int (stmt, 12, tiled);
	  if (cached != 0)
	      cached = 1;
	  sqlite3_bind_int (stmt, 13, cached);
	  if (tile_width < 256)
	      tile_width = 256;
	  if (tile_height > 5000)
	      tile_width = 5000;
	  sqlite3_bind_int (stmt, 14, tile_width);
	  if (tile_height < 256)
	      tile_height = 256;
	  if (tile_height > 5000)
	      tile_height = 5000;
	  sqlite3_bind_int (stmt, 15, tile_height);
	  if (bgcolor == NULL)
	      sqlite3_bind_null (stmt, 16);
	  else
	      sqlite3_bind_text (stmt, 16, bgcolor, strlen (bgcolor),
				 SQLITE_STATIC);
	  if (is_queryable != 0)
	      is_queryable = 1;
	  sqlite3_bind_int (stmt, 17, is_queryable);
	  if (getfeatureinfo_url == NULL)
	      sqlite3_bind_null (stmt, 18);
	  else
	      sqlite3_bind_text (stmt, 18, getfeatureinfo_url,
				 strlen (getfeatureinfo_url), SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_RegisterGetMap() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
      }
    else if (getmap_url != NULL && layer_name != NULL)
      {
	  /* attempting to insert the WMS GetMap */
	  sql =
	      "INSERT INTO wms_getmap (parent_id, url, layer_name, version, srs, "
	      "format, style, transparent, flip_axes, tiled, is_cached, "
	      "tile_width, tile_height, is_queryable) VALUES "
	      "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_RegisterGetMap: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, parent_id);
	  sqlite3_bind_text (stmt, 2, getmap_url, strlen (getmap_url),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, version, strlen (version), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 5, ref_sys, strlen (ref_sys), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 6, image_format, strlen (image_format),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 7, style, strlen (style), SQLITE_STATIC);
	  if (transparent != 0)
	      transparent = 1;
	  sqlite3_bind_int (stmt, 8, transparent);
	  if (flip_axes != 0)
	      flip_axes = 1;
	  sqlite3_bind_int (stmt, 9, flip_axes);
	  if (tiled != 0)
	      tiled = 1;
	  sqlite3_bind_int (stmt, 10, tiled);
	  if (cached != 0)
	      cached = 1;
	  sqlite3_bind_int (stmt, 11, cached);
	  if (tile_width < 256)
	      tile_width = 256;
	  if (tile_height > 5000)
	      tile_width = 5000;
	  sqlite3_bind_int (stmt, 12, tile_width);
	  if (tile_height < 256)
	      tile_height = 256;
	  if (tile_height > 5000)
	      tile_height = 5000;
	  sqlite3_bind_int (stmt, 13, tile_height);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_RegisterGetMap() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
      }
    return 1;
}

static int
check_wms_getmap (sqlite3 * sqlite, const char *url, const char *layer_name)
{
/* checks if a WMS GetMap do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT url FROM wms_getmap WHERE url = ? AND layer_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check WMS GetMap: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
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
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
do_delete_wms_getmap (sqlite3 * sqlite, const char *url, const char *layer_name)
{
/* auxiliary function: deleting a WMS GetMap */
    int ret;
    int result = 0;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM wms_getmap WHERE url = ? AND layer_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterGetMap: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_UnRegisterGetMap() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return result;
}

static void
do_delete_wms_settings_1 (sqlite3 * sqlite, const char *url,
			  const char *layer_name)
{
/* auxiliary function: deleting WMS settings (from GetMap) */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    sql = "DELETE FROM wms_settings WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_settings AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterGetMap: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	spatialite_e ("WMS_UnRegisterGetMap() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
}

SPATIALITE_PRIVATE int
unregister_wms_getmap (void *p_sqlite, const char *url, const char *layer_name)
{
/* auxiliary function: deletes a WMS GetMap definition (and related settings) */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL || layer_name == NULL)
	return 0;

    /* checking if the WMS GetMap do actually exists */
    if (!check_wms_getmap (sqlite, url, layer_name))
	return 0;
    /* deleting all WMS settings */
    do_delete_wms_settings_1 (sqlite, url, layer_name);
    /* deleting the WMS GetMap itself */
    return do_delete_wms_getmap (sqlite, url, layer_name);
}

SPATIALITE_PRIVATE int
set_wms_getmap_infos (void *p_sqlite, const char *url, const char *layer_name,
		      const char *title, const char *abstract)
{
/* auxiliary function: updates the descriptive infos supporting a WMS GetMap */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL && title != NULL && abstract != NULL)
      {
	  /* checking if the WMS GetMap do actually exists */
	  if (!check_wms_getmap (sqlite, url, layer_name))
	      return 0;

	  /* attempting to update the WMS GetGetMap */
	  sql = "UPDATE wms_getmap SET title = ?, abstract = ? "
	      "WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetMapInfos: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, title, strlen (title), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, abstract, strlen (abstract),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_SetGetMapInfos() error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
set_wms_getmap_copyright (void *p_sqlite, const char *url,
			  const char *layer_name, const char *copyright,
			  const char *license)
{
/* auxiliary function: updates the copyright infos supporting a WMS layer */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url == NULL || layer_name == NULL)
	return 0;
    if (copyright == NULL && license == NULL)
	return 1;

    if (copyright == NULL)
      {
	  /* just updating the License */
	  sql = "UPDATE wms_getmap SET license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setWMSLayerCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name,
			     strlen (layer_name), SQLITE_STATIC);
      }
    else if (license == NULL)
      {
	  /* just updating the Copyright */
	  sql = "UPDATE wms_getmap SET copyright = ? "
	      "WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setWMSLayerCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name,
			     strlen (layer_name), SQLITE_STATIC);
      }
    else
      {
	  /* updating both Copyright and License */
	  sql = "UPDATE wms_getmap SET copyright = ?, license = ("
	      "SELECT id FROM data_licenses WHERE name = ?) "
	      "WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("setWMSLayerCopyright: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, copyright, strlen (copyright),
			     SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, license, strlen (license), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, layer_name,
			     strlen (layer_name), SQLITE_STATIC);
      }
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("setWMSLayerCopyright() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

SPATIALITE_PRIVATE int
set_wms_getmap_bgcolor (void *p_sqlite,
			const char *url, const char *layer_name,
			const char *bgcolor)
{
/* updating WMS GetMap Options - BgColor */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL)
      {
	  /* checking if the WMS GetMap do actually exists */
	  if (!check_wms_getmap (sqlite, url, layer_name))
	      return 0;

	  /* attempting to update the WMS GetMap */
	  sql =
	      "UPDATE wms_getmap SET bgcolor = ? WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetMapOptions (BGCOLOR): \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (bgcolor == NULL)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_text (stmt, 1, bgcolor, strlen (bgcolor),
				 SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_SetGetMapOptions (BGCOLOR) error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
set_wms_getmap_queryable (void *p_sqlite,
			  const char *url, const char *layer_name,
			  int is_queryable, const char *getfeatureinfo_url)
{
/* updating WMS GetMap Options - IsQueryable */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL)
      {
	  /* checking if the WMS GetMap do actually exists */
	  if (!check_wms_getmap (sqlite, url, layer_name))
	      return 0;

	  /* attempting to update the WMS GetMap */
	  sql =
	      "UPDATE wms_getmap SET is_queryable = ?, getfeatureinfo_url = ? WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetMapOptions (IsQueryable): \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (is_queryable != 0)
	      is_queryable = 1;
	  sqlite3_bind_int (stmt, 1, is_queryable);
	  if (getfeatureinfo_url == NULL)
	      sqlite3_bind_null (stmt, 2);
	  else
	      sqlite3_bind_text (stmt, 2, getfeatureinfo_url,
				 strlen (getfeatureinfo_url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e
		    ("WMS_SetGetMapOptions (IsQueryable) error: \"%s\"\n",
		     sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
set_wms_getmap_options (void *p_sqlite,
			const char *url, const char *layer_name,
			int transparent, int flip_axes)
{
/* updating WMS GetMap Options - Flags */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL)
      {
	  /* checking if the WMS GetMap do actually exists */
	  if (!check_wms_getmap (sqlite, url, layer_name))
	      return 0;

	  /* attempting to update the WMS GetMap */
	  sql =
	      "UPDATE wms_getmap SET transparent = ?, flip_axes = ? WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetMapOptions (Flags): \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (transparent != 0)
	      transparent = 1;
	  sqlite3_bind_int (stmt, 1, transparent);
	  if (flip_axes != 0)
	      flip_axes = 1;
	  sqlite3_bind_int (stmt, 2, flip_axes);
	  sqlite3_bind_text (stmt, 3, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 4, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_SetGetMapOptions (Flags) error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

SPATIALITE_PRIVATE int
set_wms_getmap_tiled (void *p_sqlite,
		      const char *url, const char *layer_name,
		      int tiled, int cached, int tile_width, int tile_height)
{
/* updating WMS GetMap Options - Tiled */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (url != NULL)
      {
	  /* checking if the WMS GetMap do actually exists */
	  if (!check_wms_getmap (sqlite, url, layer_name))
	      return 0;

	  /* attempting to update the WMS GetMap */
	  sql =
	      "UPDATE wms_getmap SET tiled = ?, is_cached = ?, tile_width = ?, tile_height = ? "
	      "WHERE url = ? AND layer_name = ?";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_SetGetMapOptions (Tiled): \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (tiled != 0)
	      tiled = 1;
	  sqlite3_bind_int (stmt, 1, tiled);
	  if (cached != 0)
	      cached = 1;
	  sqlite3_bind_int (stmt, 2, cached);
	  if (tile_width < 256)
	      tile_width = 256;
	  if (tile_height > 5000)
	      tile_width = 5000;
	  sqlite3_bind_int (stmt, 3, tile_width);
	  if (tile_height < 256)
	      tile_height = 256;
	  if (tile_height > 5000)
	      tile_height = 5000;
	  sqlite3_bind_int (stmt, 4, tile_height);
	  sqlite3_bind_text (stmt, 5, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 6, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		spatialite_e ("WMS_SetGetMapOptions (Tiled) error: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		sqlite3_finalize (stmt);
		return 0;
	    }
	  sqlite3_finalize (stmt);
	  return 1;
      }
    else
	return 0;
}

static int
wms_setting_parentid (sqlite3 * sqlite, const char *url, const char *layer_name,
		      sqlite3_int64 * id)
{
/* retieving the WMS GetMap ID value */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT id FROM wms_getmap WHERE url = ? AND layer_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS Setting parent_id: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		*id = sqlite3_column_int64 (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
	return 1;
    return 0;
  stop:
    return 0;
}

static int
do_wms_set_default (sqlite3 * sqlite, const char *url, const char *layer_name,
		    const char *key, const char *value)
{
/* auxiliary function: updating a WMS GetMap default setting */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int result = 0;

/* resetting an eventual previous default */
    sql = "UPDATE wms_settings SET is_default = 0 WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_settings AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.key = Lower(?) AND s.value <> ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_DefaultSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, key, strlen (key), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 4, value, strlen (value), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_DefaultSetting() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    if (!result)
	return 0;

/* setting the current default */
    sql = "UPDATE wms_settings SET is_default = 1 WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_settings AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.key = Lower(?) AND s.value = ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_DefaultSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, key, strlen (key), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 4, value, strlen (value), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_DefaultSetting() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);

    if (result)
      {
	  /* updating the WMS GetMap */
	  sql = NULL;
	  if (strcasecmp (key, "version") == 0)
	      sql =
		  "UPDATE wms_getmap SET version = ? WHERE url = ? AND layer_name = ?";
	  if (strcasecmp (key, "format") == 0)
	      sql =
		  "UPDATE wms_getmap SET format = ? WHERE url = ? AND layer_name = ?";
	  if (strcasecmp (key, "style") == 0)
	      sql =
		  "UPDATE wms_getmap SET style = ? WHERE url = ? AND layer_name = ?";
	  if (sql == NULL)
	      return 0;
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_DefaultSetting: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  result = 0;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, value, strlen (value), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      result = 1;
	  else
	      spatialite_e ("WMS_DefaultSetting() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
      }
    return result;
}

static int
do_wms_srs_default (sqlite3 * sqlite, const char *url, const char *layer_name,
		    const char *ref_sys)
{
/* auxiliary function: updating a WMS GetMap default SRS */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int result = 0;

/* resetting an eventual previous default */
    sql = "UPDATE wms_ref_sys SET is_default = 0 WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_ref_sys AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.srs <> Upper(?))";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_DefaultSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, ref_sys, strlen (ref_sys), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_DefaultSRS() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    if (!result)
	return 0;

/* setting the current default */
    sql = "UPDATE wms_ref_sys SET is_default = 1 WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_ref_sys AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.srs = Lower(?))";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_DefaultSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, ref_sys, strlen (ref_sys), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_DefaultSRS() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);

    if (result)
      {
	  /* updating the WMS GetMap */
	  sql = NULL;
	  sql =
	      "UPDATE wms_getmap SET srs = ? WHERE url = ? AND layer_name = ?";
	  if (sql == NULL)
	      return 0;
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e ("WMS_DefaultSRS: \"%s\"\n",
			      sqlite3_errmsg (sqlite));
		return 0;
	    }
	  result = 0;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, ref_sys, strlen (ref_sys), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 3, layer_name, strlen (layer_name),
			     SQLITE_STATIC);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      result = 1;
	  else
	      spatialite_e ("WMS_DefaultSRS() error: \"%s\"\n",
			    sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
      }
    return result;
}

SPATIALITE_PRIVATE int
register_wms_setting (void *p_sqlite, const char *url, const char *layer_name,
		      const char *key, const char *value, int is_default)
{
/* auxiliary function: inserts a WMS GetMap Setting */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    sqlite3_int64 parent_id;
    const char *sql;
    sqlite3_stmt *stmt;

    if (!wms_setting_parentid (sqlite, url, layer_name, &parent_id))
      {
	  spatialite_e ("WMS_RegisterSetting: missing parent GetMap\n");
	  return 0;
      }

    /* attempting to insert the WMS Setting */
    sql = "INSERT INTO wms_settings (parent_id, key, value, is_default) "
	"VALUES (?, Lower(?), ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_RegisterSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, parent_id);
    sqlite3_bind_text (stmt, 2, key, strlen (key), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, value, strlen (value), SQLITE_STATIC);
    if (is_default != 0)
	is_default = 1;
    sqlite3_bind_int (stmt, 4, 0);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("WMS_RegisterSetting() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);

    if (is_default)
	return do_wms_set_default (sqlite, url, layer_name, key, value);
    return 1;
}

static int
check_wms_setting (sqlite3 * sqlite, const char *url, const char *layer_name,
		   const char *key, const char *value, int mode_delete)
{
/* checks if a WMS GetMap Setting do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT s.is_default FROM wms_getmap AS m "
	"LEFT JOIN wms_settings AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.key = Lower(?) AND s.value = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check WMS GetMap: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, key, strlen (key), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 4, value, strlen (value), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int is_default = sqlite3_column_int (stmt, 0);
		if (mode_delete && is_default)
		    ;
		else
		    count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
	return 1;
  stop:
    return 0;
}

static int
do_delete_wms_setting (sqlite3 * sqlite, const char *url,
		       const char *layer_name, const char *key,
		       const char *value)
{
/* auxiliary function: deleting a WMS GetMap setting */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int result = 0;

    sql = "DELETE FROM wms_settings WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_settings AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.key = Lower(?) AND s.value = ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterSetting: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, key, strlen (key), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 4, value, strlen (value), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_UnRegisterSetting() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return result;
}

SPATIALITE_PRIVATE int
unregister_wms_setting (void *p_sqlite, const char *url, const char *layer_name,
			const char *key, const char *value)
{
/* auxiliary function: deletes a WMS GetMap Setting */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL)
	return 0;

    /* checking if the WMS GetMap do actually exists */
    if (!check_wms_setting (sqlite, url, layer_name, key, value, 1))
	return 0;
    /* deleting the WMS GetMap Setting itself */
    return do_delete_wms_setting (sqlite, url, layer_name, key, value);
}

SPATIALITE_PRIVATE int
set_wms_default_setting (void *p_sqlite, const char *url,
			 const char *layer_name, const char *key,
			 const char *value)
{
/* auxiliary function: updating a WMS GetMap Default Setting */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL)
	return 0;

    /* checking if the WMS GetMap do actually exists */
    if (!check_wms_setting (sqlite, url, layer_name, key, value, 0))
	return 0;
    /* updating the WMS GetMap Default Setting */
    return do_wms_set_default (sqlite, url, layer_name, key, value);
}

SPATIALITE_PRIVATE int
register_wms_srs (void *p_sqlite, const char *url, const char *layer_name,
		  const char *ref_sys, double minx, double miny, double maxx,
		  double maxy, int is_default)
{
/* auxiliary function: inserts a WMS GetMap SRS */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    sqlite3_int64 parent_id;
    const char *sql;
    sqlite3_stmt *stmt;

    if (!wms_setting_parentid (sqlite, url, layer_name, &parent_id))
      {
	  spatialite_e ("WMS_RegisterSRS: missing parent GetMap\n");
	  return 0;
      }

    /* attempting to insert the WMS Setting */
    sql =
	"INSERT INTO wms_ref_sys (parent_id, srs, minx, miny, maxx, maxy, is_default) "
	"VALUES (?, Upper(?), ?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_RegisterSRS: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, parent_id);
    sqlite3_bind_text (stmt, 2, ref_sys, strlen (ref_sys), SQLITE_STATIC);
    sqlite3_bind_double (stmt, 3, minx);
    sqlite3_bind_double (stmt, 4, miny);
    sqlite3_bind_double (stmt, 5, maxx);
    sqlite3_bind_double (stmt, 6, maxy);
    if (is_default != 0)
	is_default = 1;
    sqlite3_bind_int (stmt, 7, is_default);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("WMS_RegisterSRS() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);

    if (is_default)
	return do_wms_srs_default (sqlite, url, layer_name, ref_sys);
    return 1;
}

static int
check_wms_srs (sqlite3 * sqlite, const char *url, const char *layer_name,
	       const char *ref_sys, int mode_delete)
{
/* checks if a WMS GetMap SRS do actually exists */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int count = 0;

    sql = "SELECT s.is_default FROM wms_getmap AS m "
	"LEFT JOIN wms_ref_sys AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.srs = Upper(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("check WMS GetMap: \"%s\"\n", sqlite3_errmsg (sqlite));
	  goto stop;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, ref_sys, strlen (ref_sys), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int is_default = sqlite3_column_int (stmt, 0);
		if (mode_delete && is_default)
		    ;
		else
		    count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count == 1)
	return 1;
  stop:
    return 0;
}

static int
do_delete_wms_srs (sqlite3 * sqlite, const char *url,
		   const char *layer_name, const char *ref_sys)
{
/* auxiliary function: deleting a WMS GetMap SRS */
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int result = 0;

    sql = "DELETE FROM wms_ref_sys WHERE id IN ("
	"SELECT s.id FROM wms_getmap AS m "
	"JOIN wms_ref_sys AS s ON (m.id = s.parent_id) "
	"WHERE m.url = ? AND m.layer_name = ? AND s.srs = Upper(?))";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_UnRegisterSRS: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 3, ref_sys, strlen (ref_sys), SQLITE_STATIC);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	result = 1;
    else
	spatialite_e ("WMS_UnRegisterSRSg() error: \"%s\"\n",
		      sqlite3_errmsg (sqlite));
    sqlite3_finalize (stmt);
    return result;
}

SPATIALITE_PRIVATE int
unregister_wms_srs (void *p_sqlite, const char *url, const char *layer_name,
		    const char *ref_sys)
{
/* auxiliary function: deletes a WMS GetMap SRS */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL)
	return 0;

    /* checking if the WMS GetMap do actually exists */
    if (!check_wms_srs (sqlite, url, layer_name, ref_sys, 1))
	return 0;
    /* deleting the WMS GetMap SRS itself */
    return do_delete_wms_srs (sqlite, url, layer_name, ref_sys);
}

SPATIALITE_PRIVATE int
set_wms_default_srs (void *p_sqlite, const char *url,
		     const char *layer_name, const char *ref_sys)
{
/* auxiliary function: updating a WMS GetMap Default SRS */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;

    if (url == NULL)
	return 0;

    /* checking if the WMS GetMap do actually exists */
    if (!check_wms_srs (sqlite, url, layer_name, ref_sys, 0))
	return 0;
    /* updating the WMS GetMap Default SRS */
    return do_wms_srs_default (sqlite, url, layer_name, ref_sys);
}

SPATIALITE_PRIVATE char *
wms_getmap_request_url (void *p_sqlite, const char *getmap_url,
			const char *layer_name, int width, int height,
			double minx, double miny, double maxx, double maxy)
{
/* return a WMS GetMap request URL */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    char *request_url = NULL;

    if (getmap_url == NULL)
	return NULL;

    sql = "SELECT version, srs, format, style, transparent, flip_axes, "
	"bgcolor FROM wms_getmap WHERE url = ? AND layer_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_GetMapRequestURL: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return NULL;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, getmap_url, strlen (getmap_url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *ref_sys_prefix = "CRS";
		const char *bgcolor = NULL;
		const char *version =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *ref_sys =
		    (const char *) sqlite3_column_text (stmt, 1);
		const char *format =
		    (const char *) sqlite3_column_text (stmt, 2);
		const char *style =
		    (const char *) sqlite3_column_text (stmt, 3);
		int transparent = sqlite3_column_int (stmt, 4);
		int flip_axes = sqlite3_column_int (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_TEXT)
		    bgcolor = (const char *) sqlite3_column_text (stmt, 6);
		/* preparing the request URL */
		if (strcmp (version, "1.3.0") < 0)
		  {
		      /* earlier versions of the protocol require SRS instead of CRS */
		      ref_sys_prefix = "SRS";
		  }
		if (flip_axes)
		  {
		      request_url =
			  sqlite3_mprintf
			  ("%s?SERVICE=WMS&REQUEST=GetMap&VERSION=%s"
			   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
			   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
			   "&TRANSPARENT=%s", getmap_url, version, layer_name,
			   ref_sys_prefix, ref_sys, miny, minx, maxy, maxx,
			   width, height, style, format,
			   (transparent == 0) ? "FALSE" : "TRUE");
		  }
		else
		  {
		      request_url =
			  sqlite3_mprintf
			  ("%s?SERVICE=WMS&REQUEST=GetMap&VERSION=%s"
			   "&LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
			   "&WIDTH=%d&HEIGHT=%d&STYLES=%s&FORMAT=%s"
			   "&TRANSPARENT=%s", getmap_url, version, layer_name,
			   ref_sys_prefix, ref_sys, minx, miny, maxx, maxy,
			   width, height, style, format,
			   (transparent == 0) ? "FALSE" : "TRUE");
		  }
		if (bgcolor != NULL)
		  {
		      char *prev = request_url;
		      request_url =
			  sqlite3_mprintf ("%s&BGCOLOR=0x%s", prev, bgcolor);
		      sqlite3_free (prev);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return request_url;
}

SPATIALITE_PRIVATE char *
wms_getfeatureinfo_request_url (void *p_sqlite, const char *getmap_url,
				const char *layer_name, int width, int height,
				int x, int y, double minx, double miny,
				double maxx, double maxy, int feature_count)
{
/* return a WMS GetFeatureInfo request URL */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    char *request_url = NULL;

    if (getmap_url == NULL)
	return NULL;

    sql = "SELECT version, srs, flip_axes, is_queryable, getfeatureinfo_url "
	"FROM wms_getmap WHERE url = ? AND layer_name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("WMS_GetFeatureInfoRequestURL: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return NULL;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, getmap_url, strlen (getmap_url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, layer_name, strlen (layer_name), SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const char *ref_sys_prefix = "CRS";
		const char *getfeatureinfo_url = NULL;
		const char *version =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *ref_sys =
		    (const char *) sqlite3_column_text (stmt, 1);
		int flip_axes = sqlite3_column_int (stmt, 2);
		int is_queryable = sqlite3_column_int (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
		    getfeatureinfo_url =
			(const char *) sqlite3_column_text (stmt, 4);
		if (!is_queryable || getfeatureinfo_url == NULL)
		    return NULL;

		/* preparing the request URL */
		if (feature_count < 1)
		    feature_count = 1;
		if (strcmp (version, "1.3.0") < 0)
		  {
		      /* earlier versions of the protocol require SRS instead of CRS */
		      ref_sys_prefix = "SRS";
		  }
		if (flip_axes)
		  {
		      request_url =
			  sqlite3_mprintf
			  ("%s?SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s"
			   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
			   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&FEATURE_COUNT=%d",
			   getfeatureinfo_url, version, layer_name,
			   ref_sys_prefix, ref_sys, miny, minx, maxy, maxx,
			   width, height, x, y, feature_count);
		  }
		else
		  {
		      request_url =
			  sqlite3_mprintf
			  ("%s?SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=%s"
			   "&QUERY_LAYERS=%s&%s=%s&BBOX=%1.6f,%1.6f,%1.6f,%1.6f"
			   "&WIDTH=%d&HEIGHT=%d&X=%d&Y=%d&FEATURE_COUNT=%d",
			   getfeatureinfo_url, version, layer_name,
			   ref_sys_prefix, ref_sys, minx, miny, maxx, maxy,
			   width, height, x, y, feature_count);
		  }
	    }
      }
    sqlite3_finalize (stmt);
    return request_url;
}

SPATIALITE_PRIVATE int
register_data_license (void *p_sqlite, const char *license_name,
		       const char *url)
{
/* auxiliary function: inserts a Data License */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (license_name == NULL)
	return 0;

    sql = "INSERT INTO data_licenses (name, url) VALUES (?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("registerDataLicense: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, license_name,
		       strlen (license_name), SQLITE_STATIC);
    if (url == NULL)
	sqlite3_bind_null (stmt, 2);
    else
	sqlite3_bind_text (stmt, 2, url, strlen (url), SQLITE_STATIC);

    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("registerDataLicense() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

SPATIALITE_PRIVATE int
unregister_data_license (void *p_sqlite, const char *license_name)
{
/* auxiliary function: deletes a Data License */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;

    if (license_name == NULL)
	return 0;

    sql = "DELETE FROM data_licenses WHERE name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("unregisterDataLicense: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, license_name,
		       strlen (license_name), SQLITE_STATIC);

    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("unregisterDataLicense() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);
    return 1;
}

SPATIALITE_PRIVATE int
rename_data_license (void *p_sqlite, const char *old_name, const char *new_name)
{
/* auxiliary function: renames a Data License */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int prev_changes;
    int curr_changes;

    if (old_name == NULL || new_name == NULL)
	return 0;

    prev_changes = sqlite3_total_changes (sqlite);

    sql = "UPDATE data_licenses SET name = ? WHERE name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("renameDataLicense: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, new_name, strlen (new_name), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, old_name, strlen (old_name), SQLITE_STATIC);

    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("renameDataLicense() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);

    curr_changes = sqlite3_total_changes (sqlite);
    if (prev_changes == curr_changes)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE int
set_data_license_url (void *p_sqlite, const char *license_name, const char *url)
{
/* auxiliary function: updates a Data License URL */
    sqlite3 *sqlite = (sqlite3 *) p_sqlite;
    int ret;
    const char *sql;
    sqlite3_stmt *stmt;
    int prev_changes;
    int curr_changes;

    if (license_name == NULL || url == NULL)
	return 0;

    prev_changes = sqlite3_total_changes (sqlite);

    sql = "UPDATE data_licenses SET url = ? WHERE name = ?";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("setDataLicenseUrl: \"%s\"\n", sqlite3_errmsg (sqlite));
	  return 0;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, url, strlen (url), SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, license_name,
		       strlen (license_name), SQLITE_STATIC);

    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  spatialite_e ("setDataLicenseUrl() error: \"%s\"\n",
			sqlite3_errmsg (sqlite));
	  sqlite3_finalize (stmt);
	  return 0;
      }
    sqlite3_finalize (stmt);

    curr_changes = sqlite3_total_changes (sqlite);
    if (prev_changes == curr_changes)
	return 0;
    return 1;
}
