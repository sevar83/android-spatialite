/*

 gaia_netstmts.c -- implementation of Topology-Network prepared statements
    
 version 4.3, 2015 August 11

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
#include <spatialite/gaia_network.h>

#include <spatialite_private.h>

#include <librttopo.h>

#include <lwn_network.h>

#include "network_private.h"

#define GAIA_UNUSED() if (argc || argv) argc = argc;


NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_getNetNodeWithinDistance2D (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the getNetNodeWithinDistance2D prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" "
			 "WHERE ST_Distance(geometry, MakePoint(?, ?)) <= ? AND ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geometry' AND search_frame = BuildCircleMBR(?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("Prepare_getNetNodeWithinDistance2D error: \"%s\"",
	       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_getLinkWithinDistance2D (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the getLinkWithinDistance2D prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT link_id FROM MAIN.\"%s\" "
			 "WHERE ST_Distance(geometry, MakePoint(?, ?)) <= ? AND ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geometry' AND search_frame = BuildCircleMBR(?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getLinkWithinDistance2D error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_insertNetNodes (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the insertNetNodes prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (node_id, geometry) VALUES (?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_insertNetNodes error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_insertLinks (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the insertLinks prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\" (link_id, start_node, end_node, geometry) "
	 "VALUES (?, ?, ?, ?)", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_insertLinks error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_deleteNetNodesById (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the deleteNodesById prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE node_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_deleteNetNodesById error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_getNetNodeWithinBox2D (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the getNetNodeWithinBox2D prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT node_id FROM MAIN.\"%s\" WHERE ROWID IN ("
			 "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geometry' AND search_frame = BuildMBR(?, ?, ?, ?))",
			 xtable, table);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getNetNodeWithinBox2D error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }
    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_getNextLinkId (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the getNextLinkId prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    if (net == NULL)
	return NULL;

    sql =
	sqlite3_mprintf
	("SELECT next_link_id FROM MAIN.networks WHERE Lower(network_name) = Lower(%Q)",
	 net->network_name);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getNextLinkId error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_setNextLinkId (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the setNextLinkId prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    if (net == NULL)
	return NULL;

    sql =
	sqlite3_mprintf
	("UPDATE MAIN.networks SET next_link_id = next_link_id + 1 "
	 "WHERE Lower(network_name) = Lower(%Q)", net->network_name);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_setNextLinkId error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

NETWORK_PRIVATE sqlite3_stmt *
do_create_stmt_deleteLinksById (GaiaNetworkAccessorPtr accessor)
{
/* attempting to create the deleteNodesById prepared statement */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    if (net == NULL)
	return NULL;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE link_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_deleteLinksById error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return NULL;
      }

    return stmt;
}

#endif /* end RTTOPO conditionals */
