/*

 gaia_topostmts.c -- implementation of Topology prepared statements
    
 version 4.3, 2015 July 18

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

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>
#include <spatialite/gaiageo.h>
#include <spatialite/gaiaaux.h>
#include <spatialite/gaia_topology.h>

#include <spatialite_private.h>

#include <librttopo.h>

#include "topology_private.h"

#define GAIA_UNUSED() if (argc || argv) argc = argc;


TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getNodeWithinDistance2D (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getNodeWithinDistance2D prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" "
			 "WHERE ST_Distance(geom, MakePoint(?, ?)) <= ? AND ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = BuildCircleMBR(?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getNodeWithinDistance2D error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getNodeWithinBox2D (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getNodeWithinBox2D prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" WHERE ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = BuildMBR(?, ?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getNodeWithinBox2D error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }
    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_insertNodes (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the insertNodes prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (node_id, containing_face, geom) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_insertNodes error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getEdgeWithinDistance2D (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getEdgeWithinDistance2D prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" "
			 "WHERE ST_Distance(geom, MakePoint(?, ?)) <= ? AND ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = BuildCircleMBR(?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getEdgeWithinDistance2D error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getEdgeWithinBox2D (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getEdgeWithinBox2D prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT edge_id FROM MAIN.\"%s\" WHERE ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = BuildMBR(?, ?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getEdgeWithinBox2D error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getAllEdges (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getAllEdges prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, start_node, end_node, left_face, right_face, "
	 "next_left_edge, next_right_edge, geom  FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getAllEdges error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }
    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getFaceContainingPoint_1 (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getFaceContainingPoint #1 prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *rtree;
    char *xrtree;
    if (topo == NULL)
	return NULL;

    rtree = sqlite3_mprintf ("idx_%s_face_mbr", topo->topology_name);
    xrtree = gaiaDoubleQuotedSql (rtree);
    sql =
	sqlite3_mprintf
	("SELECT pkid FROM MAIN.\"%s\" WHERE xmin <= ? AND xmax >= ? AND ymin <= ? AND ymax >= ?",
	 xrtree);
    free (xrtree);
    sqlite3_free (rtree);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("Prepare_getFaceContainingPoint #1 error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getFaceContainingPoint_2 (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getFaceContainingPoint #2 prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    if (topo == NULL)
	return NULL;

    sql =
	sqlite3_mprintf
	("SELECT ST_Contains(ST_GetFaceGeometry(%Q, ?), MakePoint(?, ?))",
	 topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("Prepare_getFaceContainingPoint #2 error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_insertEdges (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the insertEdges prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (edge_id, start_node, end_node, left_face, "
	 "right_face, next_left_edge, next_right_edge, geom) "
	 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_insertEdges error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getNextEdgeId (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getNextEdgeId prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    if (topo == NULL)
	return NULL;

    sql =
	sqlite3_mprintf
	("SELECT next_edge_id FROM MAIN.topologies WHERE Lower(topology_name) = Lower(%Q)",
	 topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getNextEdgeId error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_setNextEdgeId (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the setNextEdgeId prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    if (topo == NULL)
	return NULL;

    sql =
	sqlite3_mprintf
	("UPDATE MAIN.topologies SET next_edge_id = next_edge_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q)", topo->topology_name);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_setNextEdgeId error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getRingEdges (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getRingEdges prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("WITH RECURSIVE edgering AS ("
			 "SELECT ? as signed_edge_id, edge_id, next_left_edge, next_right_edge "
			 "FROM MAIN.\"%s\" WHERE edge_id = ABS(?) UNION SELECT CASE WHEN "
			 "p.signed_edge_id < 0 THEN p.next_right_edge ELSE p.next_left_edge END, "
			 "e.edge_id, e.next_left_edge, e.next_right_edge "
			 "FROM MAIN.\"%s\" AS e, edgering AS p WHERE "
			 "e.edge_id = CASE WHEN p.signed_edge_id < 0 THEN "
			 "ABS(p.next_right_edge) ELSE ABS(p.next_left_edge) END ) "
			 "SELECT * FROM edgering", xtable, xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getRingEdges error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_insertFaces (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the insertFaces prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (face_id, mbr) VALUES (?, BuildMBR(?, ?, ?, ?, %d))",
	 xtable, topo->srid);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_insertFaces error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_updateFacesById (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the updateFacesById prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE MAIN.\"%s\" SET mbr = BuildMBR(?, ?, ?, ?, %d) WHERE face_id = ?",
	 xtable, topo->srid);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateFacesById error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_deleteFacesById (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the deleteFacesById prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE face_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_deleteFacesById error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_deleteNodesById (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the deleteNodesById prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE node_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_deleteNodesById error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

TOPOLOGY_PRIVATE sqlite3_stmt *
do_create_stmt_getFaceWithinBox2D (GaiaTopologyAccessorPtr accessor)
{
/* attempting to create the getFaceWithinBox2D prepared statement */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (topo == NULL)
	return NULL;

    table = sqlite3_mprintf ("idx_%s_face_mbr", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("SELECT pkid, xmin, ymin, xmax, ymax FROM MAIN.\"%s\" "
	 "WHERE xmin <= ? AND xmax >= ? AND ymin <= ? AND ymax >= ?", xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getFaceWithinBox2D error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }
    return stmt;
}

#endif /* end RTTOPO conditionals */
