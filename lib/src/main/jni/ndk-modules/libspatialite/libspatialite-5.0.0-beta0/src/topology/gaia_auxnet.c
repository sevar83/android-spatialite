/*

 gaia_auxnet.c -- implementation of the Topology-Network module methods
    
 version 4.3, 2015 August 12

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
#include <spatialite/gaia_network.h>
#include <spatialite/gaia_topology.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <librttopo.h>

#include <lwn_network.h>

#include "network_private.h"
#include "topology_private.h"

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

#define GAIA_UNUSED() if (argc || argv) argc = argc;

SPATIALITE_PRIVATE void
free_internal_cache_networks (void *firstNetwork)
{
/* destroying all Networks registered into the Internal Connection Cache */
    struct gaia_network *p_net = (struct gaia_network *) firstNetwork;
    struct gaia_network *p_net_n;

    while (p_net != NULL)
      {
	  p_net_n = p_net->next;
	  gaiaNetworkDestroy ((GaiaNetworkAccessorPtr) p_net);
	  p_net = p_net_n;
      }
}

SPATIALITE_PRIVATE int
do_create_networks (void *sqlite_handle)
{
/* attempting to create the Networks table (if not already existing) */
    const char *sql;
    char *err_msg = NULL;
    int ret;
    sqlite3 *handle = (sqlite3 *) sqlite_handle;

    sql = "CREATE TABLE IF NOT EXISTS networks (\n"
	"\tnetwork_name TEXT NOT NULL PRIMARY KEY,\n"
	"\tspatial INTEGER NOT NULL,\n"
	"\tsrid INTEGER NOT NULL,\n"
	"\thas_z INTEGER NOT NULL,\n"
	"\tallow_coincident INTEGER NOT NULL,\n"
	"\tnext_node_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tnext_link_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tCONSTRAINT net_srid_fk FOREIGN KEY (srid) "
	"REFERENCES spatial_ref_sys (srid))";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE networks - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating Networks triggers */
    sql = "CREATE TRIGGER IF NOT EXISTS network_name_insert\n"
	"BEFORE INSERT ON 'networks'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on networks violates constraint: "
	"network_name value must not contain a single quote')\n"
	"WHERE NEW.network_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on networks violates constraint: "
	"network_name value must not contain a double quote')\n"
	"WHERE NEW.network_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on networks violates constraint: "
	"network_name value must be lower case')\n"
	"WHERE NEW.network_name <> lower(NEW.network_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER IF NOT EXISTS network_name_update\n"
	"BEFORE UPDATE OF 'network_name' ON 'networks'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on networks violates constraint: "
	"network_name value must not contain a single quote')\n"
	"WHERE NEW.network_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on networks violates constraint: "
	"network_name value must not contain a double quote')\n"
	"WHERE NEW.network_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on networks violates constraint: "
	"network_name value must be lower case')\n"
	"WHERE NEW.network_name <> lower(NEW.network_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
check_new_network (sqlite3 * handle, const char *network_name)
{
/* testing if some already defined DB object forbids creating the new Network */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the same Network is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.networks WHERE "
			   "Lower(network_name) = Lower(%Q)", network_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table/geom is already defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", network_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geometry')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_link", network_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geometry')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if some table is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM sqlite_master WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", network_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_link", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geometry", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_link_geometry", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

NETWORK_PRIVATE LWN_LINE *
gaianet_convert_linestring_to_lwnline (gaiaLinestringPtr ln, int srid,
				       int has_z)
{
/* converting a Linestring into an LWN_LINE */
    int iv;
    LWN_LINE *line = lwn_alloc_line (ln->Points, srid, has_z);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  double x;
	  double y;
	  double z;
	  double m;
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
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  line->x[iv] = x;
	  line->y[iv] = y;
	  if (has_z)
	      line->z[iv] = z;
      }
    return line;
}

static int
do_create_node (sqlite3 * handle, const char *network_name, int srid, int has_z)
{
/* attempting to create the Network Node table */
    char *sql;
    char *table;
    char *xtable;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_node", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tnode_id INTEGER PRIMARY KEY AUTOINCREMENT)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE network-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_node_ins" trigger */
    trigger = sqlite3_mprintf ("%s_node_next_ins", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_node", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE networks SET next_node_id = NEW.node_id + 1 "
			   "WHERE Lower(network_name) = Lower(%Q) AND next_node_id < NEW.node_id + 1;\n"
			   "END", xtrigger, xtable, network_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-NODE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_node_upd" trigger */
    trigger = sqlite3_mprintf ("%s_node_next_upd", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_node", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF node_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE networks SET next_node_id = NEW.node_id + 1 "
	 "WHERE Lower(network_name) = Lower(%Q) AND next_node_id < NEW.node_id + 1;\n"
	 "END", xtrigger, xtable, network_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-NODE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Node Geometry */
    table = sqlite3_mprintf ("%s_node", network_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', %d, 'POINT', %Q)", table,
	 srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn network-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Node Geometry */
    table = sqlite3_mprintf ("%s_node", network_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex network-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_link (sqlite3 * handle, const char *network_name, int srid, int has_z)
{
/* attempting to create the Network Link table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xnodes;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link_node_start_fk", network_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link_node_end_fk", network_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", network_name);
    xnodes = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tlink_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tstart_node INTEGER NOT NULL,\n"
			   "\tend_node INTEGER NOT NULL,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (start_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (end_node) "
			   "REFERENCES \"%s\" (node_id))",
			   xtable, xconstraint1, xnodes, xconstraint2, xnodes);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xnodes);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE network-LINK - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_link_ins" trigger */
    trigger = sqlite3_mprintf ("%s_link_next_ins", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE networks SET next_link_id = NEW.link_id + 1 "
			   "WHERE Lower(network_name) = Lower(%Q) AND next_link_id < NEW.link_id + 1;\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE link_id = NEW.link_id;"
			   "END", xtrigger, xtable, network_name, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-LINK next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "link_update" trigger */
    trigger = sqlite3_mprintf ("%s_link_update", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE link_id = NEW.link_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-LINK next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_link_upd" trigger */
    trigger = sqlite3_mprintf ("%s_link_next_upd", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF link_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE networks SET next_link_id = NEW.link_id + 1 "
	 "WHERE Lower(network_name) = Lower(%Q) AND next_link_id < NEW.link_id + 1;\n"
	 "END", xtrigger, xtable, network_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-LINK next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Link Geometry */
    table = sqlite3_mprintf ("%s_link", network_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', %d, 'LINESTRING', %Q)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn network-LINK - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Link Geometry */
    table = sqlite3_mprintf ("%s_link", network_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex network-LINK - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "start_node" */
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_start_node", network_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (start_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX link-startnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "end_node" */
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_end_node", network_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (end_node)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX link-endnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_timestamp", network_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (timestamp)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX link-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_seeds (sqlite3 * handle, const char *network_name, int srid,
		 int has_z)
{
/* attempting to create the Network Seeds table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint;
    char *xlinks;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_seeds", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds_link_fk", network_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link", network_name);
    xlinks = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tseed_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tlink_id INTEGER NOT NULL,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (link_id) "
			   "REFERENCES \"%s\" (link_id) ON DELETE CASCADE)",
			   xtable, xconstraint, xlinks);
    free (xtable);
    free (xconstraint);
    free (xlinks);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE network-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_ins" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_ins", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE seed_id = NEW.seed_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-SEEDS next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_update" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_update", network_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE seed_id = NEW.seed_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER network-SEED next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", network_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geometry', %d, 'POINT', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn network-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", network_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geometry')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex network-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "link_id" */
    table = sqlite3_mprintf ("%s_seeds", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_link", network_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (link_id)",
			 xconstraint, xtable);
    free (xtable);
    free (xconstraint);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-link - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_seeds", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_seeds_timestamp", network_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (timestamp)",
			 xconstraint, xtable);
    free (xtable);
    free (xconstraint);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

GAIANET_DECLARE int
gaiaNetworkCreate (sqlite3 * handle, const char *network_name, int spatial,
		   int srid, int has_z, int allow_coincident)
{
/* attempting to create a new Network */
    int ret;
    char *sql;

/* creating the Networks table (just in case) */
    if (!do_create_networks (handle))
	return 0;

/* testing for forbidding objects */
    if (!check_new_network (handle, network_name))
	return 0;

/* creating the Network own Tables */
    if (!do_create_node (handle, network_name, srid, has_z))
	goto error;
    if (!do_create_link (handle, network_name, srid, has_z))
	goto error;
    if (!do_create_seeds (handle, network_name, srid, has_z))
	goto error;

/* registering the Network */
    sql = sqlite3_mprintf ("INSERT INTO MAIN.networks (network_name, "
			   "spatial, srid, has_z, allow_coincident) VALUES (Lower(%Q), %d, %d, %d, %d)",
			   network_name, spatial, srid, has_z,
			   allow_coincident);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

static int
check_existing_network (sqlite3 * handle, const char *network_name,
			int full_check)
{
/* testing if a Network is already defined */
    char *sql;
    char *prev;
    char *table;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;

/* testing if the Network is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.networks WHERE "
			   "Lower(network_name) = Lower(%Q)", network_name);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 1)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    if (!full_check)
	return 1;

/* testing if all table/geom are correctly defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", network_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geometry')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_link", network_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geometry')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 2)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if all tables are already defined */
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM sqlite_master WHERE type = 'table' AND (");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", network_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_link", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geometry", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_link_geometry", network_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q))", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) != 4)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_drop_network_table (sqlite3 * handle, const char *network_name,
		       const char *which)
{
/* attempting to drop some Network table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* disabling the corresponding Spatial Index */
    table = sqlite3_mprintf ("%s_%s", network_name, which);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'geometry')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableSpatialIndex network-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* discarding the Geometry column */
    table = sqlite3_mprintf ("%s_%s", network_name, which);
    sql =
	sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'geometry')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableGeometryColumn network-%s - error: %s\n", which,
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_%s", network_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP network-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the corresponding Spatial Index */
    table = sqlite3_mprintf ("idx_%s_%s_geometry", network_name, which);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DROP SpatialIndex network-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_get_network (sqlite3 * handle, const char *net_name, char **network_name,
		int *spatial, int *srid, int *has_z, int *allow_coincident)
{
/* retrieving a Network configuration */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xnetwork_name = NULL;
    int xsrid;
    int xhas_z;
    int xspatial;
    int xallow_coincident;

/* preparing the SQL query */
    sql =
	sqlite3_mprintf
	("SELECT network_name, spatial, srid, has_z, allow_coincident "
	 "FROM MAIN.networks WHERE Lower(network_name) = Lower(%Q)", net_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM networks error: \"%s\"\n",
			sqlite3_errmsg (handle));
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
		int ok_name = 0;
		int ok_srid = 0;
		int ok_z = 0;
		int ok_spatial = 0;
		int ok_allow_coincident = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *str =
			  (const char *) sqlite3_column_text (stmt, 0);
		      if (xnetwork_name != NULL)
			  free (xnetwork_name);
		      xnetwork_name = malloc (strlen (str) + 1);
		      strcpy (xnetwork_name, str);
		      ok_name = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		  {
		      xspatial = sqlite3_column_int (stmt, 1);
		      ok_spatial = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_INTEGER)
		  {
		      xsrid = sqlite3_column_int (stmt, 2);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		  {
		      xhas_z = sqlite3_column_int (stmt, 3);
		      ok_z = 1;
		  }
		if (sqlite3_column_type (stmt, 4) == SQLITE_INTEGER)
		  {
		      xallow_coincident = sqlite3_column_int (stmt, 4);
		      ok_allow_coincident = 1;
		  }
		if (ok_name && ok_spatial && ok_srid && ok_z
		    && ok_allow_coincident)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  else
	    {
		spatialite_e
		    ("step: SELECT FROM networks error: \"%s\"\n",
		     sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		return 0;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *network_name = xnetwork_name;
	  *srid = xsrid;
	  *has_z = xhas_z;
	  *spatial = xspatial;
	  *allow_coincident = xallow_coincident;
	  return 1;
      }

    if (xnetwork_name != NULL)
	free (xnetwork_name);
    return 0;
}

GAIANET_DECLARE GaiaNetworkAccessorPtr
gaiaGetNetwork (sqlite3 * handle, const void *cache, const char *network_name)
{
/* attempting to get a reference to some Network Accessor Object */
    GaiaNetworkAccessorPtr accessor;

/* attempting to retrieve an alredy cached definition */
    accessor = gaiaNetworkFromCache (cache, network_name);
    if (accessor != NULL)
	return accessor;

/* attempting to create a new Network Accessor */
    accessor = gaiaNetworkFromDBMS (handle, cache, network_name);
    return accessor;
}

GAIANET_DECLARE GaiaNetworkAccessorPtr
gaiaNetworkFromCache (const void *p_cache, const char *network_name)
{
/* attempting to retrieve an already defined Network Accessor Object from the Connection Cache */
    struct gaia_network *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

    ptr = (struct gaia_network *) (cache->firstNetwork);
    while (ptr != NULL)
      {
	  /* checking for an already registered Network */
	  if (strcasecmp (network_name, ptr->network_name) == 0)
	      return (GaiaNetworkAccessorPtr) ptr;
	  ptr = ptr->next;
      }
    return NULL;
}

GAIANET_DECLARE int
gaiaReadNetworkFromDBMS (sqlite3 *
			 handle,
			 const char
			 *net_name, char **network_name, int *spatial,
			 int *srid, int *has_z, int *allow_coincident)
{
/* testing for existing DBMS objects */
    if (!check_existing_network (handle, net_name, 1))
	return 0;

/* retrieving the Network configuration */
    if (!do_get_network
	(handle, net_name, network_name, spatial, srid, has_z,
	 allow_coincident))
	return 0;
    return 1;
}

GAIANET_DECLARE GaiaNetworkAccessorPtr
gaiaNetworkFromDBMS (sqlite3 * handle, const void *p_cache,
		     const char *network_name)
{
/* attempting to create a Network Accessor Object into the Connection Cache */
    const RTCTX *ctx = NULL;
    LWN_BE_CALLBACKS *callbacks;
    struct gaia_network *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;

/* allocating and initializing the opaque object */
    ptr = malloc (sizeof (struct gaia_network));
    ptr->db_handle = handle;
    ptr->cache = cache;
    ptr->network_name = NULL;
    ptr->srid = -1;
    ptr->has_z = 0;
    ptr->spatial = 0;
    ptr->allow_coincident = 0;
    ptr->last_error_message = NULL;
    ptr->lwn_iface = lwn_CreateBackendIface (ctx, (const LWN_BE_DATA *) ptr);
    ptr->prev = cache->lastNetwork;
    ptr->next = NULL;

    callbacks = malloc (sizeof (LWN_BE_CALLBACKS));
    callbacks->netGetSRID = netcallback_netGetSRID;
    callbacks->netHasZ = netcallback_netHasZ;
    callbacks->netIsSpatial = netcallback_netIsSpatial;
    callbacks->netAllowCoincident = netcallback_netAllowCoincident;
    callbacks->netGetGEOS = netcallback_netGetGEOS;
    callbacks->createNetwork = NULL;
    callbacks->loadNetworkByName = netcallback_loadNetworkByName;
    callbacks->freeNetwork = netcallback_freeNetwork;
    callbacks->getNetNodeWithinDistance2D =
	netcallback_getNetNodeWithinDistance2D;
    callbacks->getLinkWithinDistance2D = netcallback_getLinkWithinDistance2D;
    callbacks->insertNetNodes = netcallback_insertNetNodes;
    callbacks->getNetNodeById = netcallback_getNetNodeById;
    callbacks->updateNetNodesById = netcallback_updateNetNodesById;
    callbacks->deleteNetNodesById = netcallback_deleteNetNodesById;
    callbacks->getLinkByNetNode = netcallback_getLinkByNetNode;
    callbacks->getNextLinkId = netcallback_getNextLinkId;
    callbacks->getNetNodeWithinBox2D = netcallback_getNetNodeWithinBox2D;
    callbacks->getNextLinkId = netcallback_getNextLinkId;
    callbacks->insertLinks = netcallback_insertLinks;
    callbacks->updateLinksById = netcallback_updateLinksById;
    callbacks->getLinkById = netcallback_getLinkById;
    callbacks->deleteLinksById = netcallback_deleteLinksById;
    ptr->callbacks = callbacks;

    lwn_BackendIfaceRegisterCallbacks (ptr->lwn_iface, callbacks);
    ptr->lwn_network = lwn_LoadNetwork (ptr->lwn_iface, network_name);

    ptr->stmt_getNetNodeWithinDistance2D = NULL;
    ptr->stmt_getLinkWithinDistance2D = NULL;
    ptr->stmt_insertNetNodes = NULL;
    ptr->stmt_deleteNetNodesById = NULL;
    ptr->stmt_getNetNodeWithinBox2D = NULL;
    ptr->stmt_getNextLinkId = NULL;
    ptr->stmt_setNextLinkId = NULL;
    ptr->stmt_insertLinks = NULL;
    ptr->stmt_deleteLinksById = NULL;
    if (ptr->lwn_network == NULL)
	goto invalid;

/* creating the SQL prepared statements */
    create_toponet_prepared_stmts ((GaiaNetworkAccessorPtr) ptr);
    return (GaiaNetworkAccessorPtr) ptr;

  invalid:
    gaiaNetworkDestroy ((GaiaNetworkAccessorPtr) ptr);
    return NULL;
}

GAIANET_DECLARE void
gaiaNetworkDestroy (GaiaNetworkAccessorPtr net_ptr)
{
/* destroying a Network Accessor Object */
    struct gaia_network *prev;
    struct gaia_network *next;
    struct splite_internal_cache *cache;
    struct gaia_network *ptr = (struct gaia_network *) net_ptr;
    if (ptr == NULL)
	return;

    prev = ptr->prev;
    next = ptr->next;
    cache = (struct splite_internal_cache *) (ptr->cache);
    if (ptr->lwn_network != NULL)
	lwn_FreeNetwork ((LWN_NETWORK *) (ptr->lwn_network));
    if (ptr->lwn_iface != NULL)
	lwn_FreeBackendIface ((LWN_BE_IFACE *) (ptr->lwn_iface));
    if (ptr->callbacks != NULL)
	free (ptr->callbacks);
    if (ptr->network_name != NULL)
	free (ptr->network_name);
    if (ptr->last_error_message != NULL)
	free (ptr->last_error_message);

    finalize_toponet_prepared_stmts (net_ptr);
    free (ptr);

/* unregistering from the Internal Cache double linked list */
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;
    if (cache->firstNetwork == ptr)
	cache->firstNetwork = next;
    if (cache->lastNetwork == ptr)
	cache->lastNetwork = prev;
}

NETWORK_PRIVATE void
finalize_toponet_prepared_stmts (GaiaNetworkAccessorPtr accessor)
{
/* finalizing the SQL prepared statements */
    struct gaia_network *ptr = (struct gaia_network *) accessor;
    if (ptr->stmt_getNetNodeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getNetNodeWithinDistance2D);
    if (ptr->stmt_getLinkWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getLinkWithinDistance2D);
    if (ptr->stmt_insertNetNodes != NULL)
	sqlite3_finalize (ptr->stmt_insertNetNodes);
    if (ptr->stmt_deleteNetNodesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteNetNodesById);
    if (ptr->stmt_getNetNodeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getNetNodeWithinBox2D);
    if (ptr->stmt_getNextLinkId != NULL)
	sqlite3_finalize (ptr->stmt_getNextLinkId);
    if (ptr->stmt_setNextLinkId != NULL)
	sqlite3_finalize (ptr->stmt_setNextLinkId);
    if (ptr->stmt_insertLinks != NULL)
	sqlite3_finalize (ptr->stmt_insertLinks);
    if (ptr->stmt_deleteLinksById != NULL)
	sqlite3_finalize (ptr->stmt_deleteLinksById);
    ptr->stmt_getNetNodeWithinDistance2D = NULL;
    ptr->stmt_getLinkWithinDistance2D = NULL;
    ptr->stmt_insertNetNodes = NULL;
    ptr->stmt_deleteNetNodesById = NULL;
    ptr->stmt_getNetNodeWithinBox2D = NULL;
    ptr->stmt_getNextLinkId = NULL;
    ptr->stmt_setNextLinkId = NULL;
    ptr->stmt_insertLinks = NULL;
    ptr->stmt_deleteLinksById = NULL;
}

NETWORK_PRIVATE void
create_toponet_prepared_stmts (GaiaNetworkAccessorPtr accessor)
{
/* creating the SQL prepared statements */
    struct gaia_network *ptr = (struct gaia_network *) accessor;
    finalize_toponet_prepared_stmts (accessor);
    ptr->stmt_getNetNodeWithinDistance2D =
	do_create_stmt_getNetNodeWithinDistance2D (accessor);
    ptr->stmt_getLinkWithinDistance2D =
	do_create_stmt_getLinkWithinDistance2D (accessor);
    ptr->stmt_deleteNetNodesById = do_create_stmt_deleteNetNodesById (accessor);
    ptr->stmt_insertNetNodes = do_create_stmt_insertNetNodes (accessor);
    ptr->stmt_getNetNodeWithinBox2D =
	do_create_stmt_getNetNodeWithinBox2D (accessor);
    ptr->stmt_getNextLinkId = do_create_stmt_getNextLinkId (accessor);
    ptr->stmt_setNextLinkId = do_create_stmt_setNextLinkId (accessor);
    ptr->stmt_insertLinks = do_create_stmt_insertLinks (accessor);
    ptr->stmt_deleteLinksById = do_create_stmt_deleteLinksById (accessor);
}

NETWORK_PRIVATE void
gaianet_reset_last_error_msg (GaiaNetworkAccessorPtr accessor)
{
/* resets the last Network error message */
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net == NULL)
	return;

    if (net->last_error_message != NULL)
	free (net->last_error_message);
    net->last_error_message = NULL;
}

NETWORK_PRIVATE void
gaianet_set_last_error_msg (GaiaNetworkAccessorPtr accessor, const char *msg)
{
/* sets the last Network error message */
    int len;
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (msg == NULL)
	msg = "no message available";

    spatialite_e ("%s\n", msg);
    if (net == NULL)
	return;

    if (net->last_error_message != NULL)
	return;

    len = strlen (msg);
    net->last_error_message = malloc (len + 1);
    strcpy (net->last_error_message, msg);
}

NETWORK_PRIVATE const char *
gaianet_get_last_exception (GaiaNetworkAccessorPtr accessor)
{
/* returns the last Network error message */
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net == NULL)
	return NULL;

    return net->last_error_message;
}

GAIANET_DECLARE int
gaiaNetworkDrop (sqlite3 * handle, const char *network_name)
{
/* attempting to drop an already existing Network */
    int ret;
    char *sql;

/* creating the Networks table (just in case) */
    if (!do_create_networks (handle))
	return 0;

/* testing for existing DBMS objects */
    if (!check_existing_network (handle, network_name, 0))
	return 0;

/* dropping the Network own Tables */
    if (!do_drop_network_table (handle, network_name, "seeds"))
	goto error;
    if (!do_drop_network_table (handle, network_name, "link"))
	goto error;
    if (!do_drop_network_table (handle, network_name, "node"))
	goto error;

/* unregistering the Network */
    sql =
	sqlite3_mprintf
	("DELETE FROM MAIN.networks WHERE Lower(network_name) = Lower(%Q)",
	 network_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

GAIANET_DECLARE sqlite3_int64
gaiaAddIsoNetNode (GaiaNetworkAccessorPtr accessor, gaiaPointPtr pt)
{
/* LWN wrapper - AddIsoNetNode */
    sqlite3_int64 ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);
    ret = lwn_AddIsoNetNode ((LWN_NETWORK *) (network->lwn_network), point);
    lwn_free_point (point);

    return ret;
}

GAIANET_DECLARE int
gaiaMoveIsoNetNode (GaiaNetworkAccessorPtr accessor,
		    sqlite3_int64 node, gaiaPointPtr pt)
{
/* LWN wrapper - MoveIsoNetNode */
    int ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_MoveIsoNetNode ((LWN_NETWORK *) (network->lwn_network), node,
			    point);
    lwn_free_point (point);

    if (ret == 0)
	return 1;
    return 0;
}

GAIANET_DECLARE int
gaiaRemIsoNetNode (GaiaNetworkAccessorPtr accessor, sqlite3_int64 node)
{
/* LWN wrapper - RemIsoNetNode */
    int ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret = lwn_RemIsoNetNode ((LWN_NETWORK *) (network->lwn_network), node);

    if (ret == 0)
	return 1;
    return 0;
}

GAIANET_DECLARE sqlite3_int64
gaiaAddLink (GaiaNetworkAccessorPtr accessor,
	     sqlite3_int64 start_node, sqlite3_int64 end_node,
	     gaiaLinestringPtr ln)
{
/* LWN wrapper - AddLink */
    sqlite3_int64 ret;
    LWN_LINE *lwn_line = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (ln != NULL)
      {
	  lwn_line =
	      gaianet_convert_linestring_to_lwnline (ln, network->srid,
						     network->has_z);
      }

    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_AddLink ((LWN_NETWORK *) (network->lwn_network), start_node,
		     end_node, lwn_line);

    lwn_free_line (lwn_line);
    return ret;
}

GAIANET_DECLARE int
gaiaChangeLinkGeom (GaiaNetworkAccessorPtr accessor,
		    sqlite3_int64 link_id, gaiaLinestringPtr ln)
{
/* LWN wrapper - ChangeLinkGeom */
    int ret;
    LWN_LINE *lwn_line = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (ln != NULL)
      {
	  lwn_line =
	      gaianet_convert_linestring_to_lwnline (ln, network->srid,
						     network->has_z);
      }

    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_ChangeLinkGeom ((LWN_NETWORK *) (network->lwn_network), link_id,
			    lwn_line);
    lwn_free_line (lwn_line);

    if (ret == 0)
	return 1;
    return 0;
}

GAIANET_DECLARE int
gaiaRemoveLink (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link)
{
/* LWN wrapper - RemoveLink */
    int ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret = lwn_RemoveLink ((LWN_NETWORK *) (network->lwn_network), link);

    if (ret == 0)
	return 1;
    return 0;
}

GAIANET_DECLARE sqlite3_int64
gaiaNewLogLinkSplit (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link)
{
/* LWN wrapper - NewLogLinkSplit  */
    sqlite3_int64 ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret = lwn_NewLogLinkSplit ((LWN_NETWORK *) (network->lwn_network), link);
    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaModLogLinkSplit (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link)
{
/* LWN wrapper - ModLogLinkSplit  */
    sqlite3_int64 ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret = lwn_ModLogLinkSplit ((LWN_NETWORK *) (network->lwn_network), link);
    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaNewGeoLinkSplit (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link,
		     gaiaPointPtr pt)
{
/* LWN wrapper - NewGeoLinkSplit  */
    sqlite3_int64 ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_NewGeoLinkSplit ((LWN_NETWORK *) (network->lwn_network), link,
			     point);
    lwn_free_point (point);
    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaModGeoLinkSplit (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link,
		     gaiaPointPtr pt)
{
/* LWN wrapper - ModGeoLinkSplit  */
    sqlite3_int64 ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_ModGeoLinkSplit ((LWN_NETWORK *) (network->lwn_network), link,
			     point);
    lwn_free_point (point);
    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaNewLinkHeal (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link,
		 sqlite3_int64 anotherlink)
{
/* LWN wrapper - NewLinkHeal  */
    sqlite3_int64 ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_NewLinkHeal ((LWN_NETWORK *) (network->lwn_network), link,
			 anotherlink);

    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaModLinkHeal (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link,
		 sqlite3_int64 anotherlink)
{
/* LWN wrapper - ModLinkHeal  */
    sqlite3_int64 ret;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    lwn_ResetErrorMsg (network->lwn_iface);
    ret =
	lwn_ModLinkHeal ((LWN_NETWORK *) (network->lwn_network), link,
			 anotherlink);

    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaGetNetNodeByPoint (GaiaNetworkAccessorPtr accessor, gaiaPointPtr pt,
		       double tolerance)
{
/* LWN wrapper - GetNetNodeByPoint */
    sqlite3_int64 ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);

    ret =
	lwn_GetNetNodeByPoint ((LWN_NETWORK *) (network->lwn_network), point,
			       tolerance);

    lwn_free_point (point);
    return ret;
}

GAIANET_DECLARE sqlite3_int64
gaiaGetLinkByPoint (GaiaNetworkAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* LWN wrapper - GetLinkByPoint */
    sqlite3_int64 ret;
    LWN_POINT *point = NULL;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    if (pt != NULL)
      {
	  if (pt->DimensionModel == GAIA_XY_Z
	      || pt->DimensionModel == GAIA_XY_Z_M)
	      point = lwn_create_point3d (network->srid, pt->X, pt->Y, pt->Z);
	  else
	      point = lwn_create_point2d (network->srid, pt->X, pt->Y);
      }
    lwn_ResetErrorMsg (network->lwn_iface);

    ret =
	lwn_GetLinkByPoint ((LWN_NETWORK *) (network->lwn_network), point,
			    tolerance);

    lwn_free_point (point);
    return ret;
}

static int
do_check_create_valid_logicalnet_table (GaiaNetworkAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *errMsg = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (net->cache);

/* attempting to drop the table (just in case if it already exists) */
    table = sqlite3_mprintf ("%s_valid_logicalnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    create_all_topo_prepared_stmts (net->cache);	/* recreating prepared stsms */
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidLogicalNet exception: %s", errMsg);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

/* attempting to create the table */
    table = sqlite3_mprintf ("%s_valid_logicalnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\terror TEXT,\n"
	 "\tprimitive1 INTEGER,\n\tprimitive2 INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidLogicalNet exception: %s", errMsg);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_loginet_check_nodes (GaiaNetworkAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for nodes with geometry */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT node_id FROM MAIN.\"%s\" WHERE geometry IS NOT NULL", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidLogicalNet() - Nodes error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "node has geometry", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidLogicalNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidLogicalNet() - Nodes step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_loginet_check_links (GaiaNetworkAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for links with geometry */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT link_id FROM MAIN.\"%s\" WHERE geometry IS NOT NULL", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidLogicalNet() - Links error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "link has geometry", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, link_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidLogicalNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidLogicalNet() - Links step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

GAIANET_DECLARE int
gaiaValidLogicalNet (GaiaNetworkAccessorPtr accessor)
{
/* generating a validity report for a given Logical Network */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net == NULL)
	return 0;

    if (!do_check_create_valid_logicalnet_table (accessor))
	return 0;

    table = sqlite3_mprintf ("%s_valid_logicalnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (error, primitive1, primitive2) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_ValidLogicalNet error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (!do_loginet_check_nodes (accessor, stmt))
	goto error;

    if (!do_loginet_check_links (accessor, stmt))
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_check_create_valid_spatialnet_table (GaiaNetworkAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *errMsg;
    struct gaia_network *net = (struct gaia_network *) accessor;

/* attempting to drop the table (just in case if it already exists) */
    table = sqlite3_mprintf ("%s_valid_spatialnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS temp.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet exception: %s", errMsg);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

/* attempting to create the table */
    table = sqlite3_mprintf ("%s_valid_spatialnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\terror TEXT,\n"
	 "\tprimitive1 INTEGER,\n\tprimitive2 INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet exception: %s", errMsg);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_spatnet_check_nodes (GaiaNetworkAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for nodes without geometry */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT node_id FROM MAIN.\"%s\" WHERE geometry IS NULL", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet() - Nodes error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "missing node geometry", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidSpatialNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidSpatialNet() - Nodes step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_spatnet_check_links (GaiaNetworkAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for links without geometry */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT link_id FROM MAIN.\"%s\" WHERE geometry IS NULL", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet() - Links error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "missing link geometry", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, link_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidSpatialNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidSpatialNet() - Links step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_spatnet_check_start_nodes (GaiaNetworkAccessorPtr accessor,
			      sqlite3_stmt * stmt)
{
/* checking for links mismatching start nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT l.link_id, l.start_node FROM MAIN.\"%s\" AS l "
			 "JOIN MAIN.\"%s\" AS n ON (l.start_node = n.node_id) "
			 "WHERE ST_Disjoint(ST_StartPoint(l.geometry), n.geometry) = 1",
			 xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidSpatialNet() - StartNodes error: \"%s\"",
	       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry start mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, link_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidSpatialNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidSpatialNet() - StartNodes step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

static int
do_spatnet_check_end_nodes (GaiaNetworkAccessorPtr accessor,
			    sqlite3_stmt * stmt)
{
/* checking for links mismatching end nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;

    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT l.link_id, l.end_node FROM MAIN.\"%s\" AS l "
			   "JOIN MAIN.\"%s\" AS n ON (l.end_node = n.node_id) "
			   "WHERE ST_Disjoint(ST_EndPoint(l.geometry), n.geometry) = 1",
			   xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet() - EndNodes error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry end mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, link_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidSpatialNet() insert error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidSpatialNet() - EndNodes step error: %s",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    return 0;
}

GAIANET_DECLARE int
gaiaValidSpatialNet (GaiaNetworkAccessorPtr accessor)
{
/* generating a validity report for a given Spatial Network */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net == NULL)
	return 0;

    if (!do_check_create_valid_spatialnet_table (accessor))
	return 0;

    table = sqlite3_mprintf ("%s_valid_spatialnet", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (error, primitive1, primitive2) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_ValidSpatialNet error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (!do_spatnet_check_nodes (accessor, stmt))
	goto error;

    if (!do_spatnet_check_links (accessor, stmt))
	goto error;

    if (!do_spatnet_check_start_nodes (accessor, stmt))
	goto error;

    if (!do_spatnet_check_end_nodes (accessor, stmt))
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

NETWORK_PRIVATE int
auxnet_insert_into_network (GaiaNetworkAccessorPtr accessor,
			    gaiaGeomCollPtr geom)
{
/* processing all individual geometry items */
    sqlite3_int64 ret;
    gaiaLinestringPtr ln;
    gaiaPoint pt;
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network == NULL)
	return 0;

    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on Linestrings items */
	  int last = ln->Points - 1;
	  double x;
	  double y;
	  double z = 0.0;
	  double m = 0.0;
	  sqlite3_int64 start_node;
	  sqlite3_int64 end_node;
	  LWN_LINE *lwn_line;
	  lwn_ResetErrorMsg (network->lwn_iface);

	  /* attempting to retrieve or insert the Start Node */
	  if (geom->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, 0, &x, &y, &z);
	    }
	  else if (geom->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, 0, &x, &y, &z, &m);
	    }
	  else if (geom->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, 0, &x, &y, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, 0, &x, &y);
	    }
	  if (network->has_z)
	    {
		pt.DimensionModel = GAIA_XY_Z;
		pt.X = x;
		pt.Y = y;
		pt.Z = z;
	    }
	  else
	    {
		pt.DimensionModel = GAIA_XY;
		pt.X = x;
		pt.Y = y;
	    }
	  start_node = gaiaGetNetNodeByPoint (accessor, &pt, 0.0);
	  if (start_node < 0)
	      start_node = gaiaAddIsoNetNode (accessor, &pt);
	  if (start_node < 0)
	    {
		const char *msg = lwn_GetErrorMsg (network->lwn_iface);
		gaianet_set_last_error_msg (accessor, msg);
		goto error;
	    }

	  /* attempting to retrieve or insert the End Node */
	  if (geom->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, last, &x, &y, &z);
	    }
	  else if (geom->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, last, &x, &y, &z, &m);
	    }
	  else if (geom->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, last, &x, &y, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, last, &x, &y);
	    }
	  if (network->has_z)
	    {
		pt.DimensionModel = GAIA_XY_Z;
		pt.X = x;
		pt.Y = y;
		pt.Z = z;
	    }
	  else
	    {
		pt.DimensionModel = GAIA_XY;
		pt.X = x;
		pt.Y = y;
	    }
	  end_node = gaiaGetNetNodeByPoint (accessor, &pt, 0.0);
	  if (end_node < 0)
	      end_node = gaiaAddIsoNetNode (accessor, &pt);
	  if (end_node < 0)
	    {
		const char *msg = lwn_GetErrorMsg (network->lwn_iface);
		gaianet_set_last_error_msg (accessor, msg);
		goto error;
	    }

	  lwn_line = gaianet_convert_linestring_to_lwnline (ln, network->srid,
							    network->has_z);
	  ret =
	      lwn_AddLink ((LWN_NETWORK *) (network->lwn_network), start_node,
			   end_node, lwn_line);
	  lwn_free_line (lwn_line);
	  if (ret <= 0)
	    {
		const char *msg = lwn_GetErrorMsg (network->lwn_iface);
		gaianet_set_last_error_msg (accessor, msg);
		goto error;
	    }

	  ln = ln->Next;
      }

    return 1;

  error:
    return 0;
}

GAIANET_DECLARE int
gaiaTopoNet_FromGeoTable (GaiaNetworkAccessorPtr accessor,
			  const char *db_prefix, const char *table,
			  const char *column)
{
/* attempting to import a whole GeoTable into a Topoology-Network */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (net == NULL)
	return 0;
    if (net->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (net->cache);
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
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_FromGeoTable error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
			    if (!auxnet_insert_into_network (accessor, geom))
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
				("TopoNet_FromGeoTable error: Invalid Geometry");
			    gaianet_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoNet_FromGeoTable error: not a BLOB value");
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoNet_FromGeoTable error: \"%s\"",
				     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg (accessor, msg);
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

GAIANET_DECLARE gaiaGeomCollPtr
gaiaGetLinkSeed (GaiaNetworkAccessorPtr accessor, sqlite3_int64 link)
{
/* attempting to get a Point (seed) identifying a Network Link */
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    gaiaGeomCollPtr point = NULL;
    if (net == NULL)
	return NULL;

/* building the SQL statement */
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT geometry FROM MAIN.\"%s\" WHERE link_id = ?",
			 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("GetLinkSeed error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, link);

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
				      ("TopoNet_GetLinkSeed error: Invalid Geometry");
				  gaianet_set_last_error_msg (accessor, msg);
				  sqlite3_free (msg);
				  gaiaFreeGeomColl (geom);
				  goto error;
			      }
			    iv = ln->Points / 2;
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
			    if (net->has_z)
			      {
				  point = gaiaAllocGeomCollXYZ ();
				  gaiaAddPointToGeomCollXYZ (point, x, y, z);
			      }
			    else
			      {
				  point = gaiaAllocGeomColl ();
				  gaiaAddPointToGeomColl (point, x, y);
			      }
			    point->Srid = net->srid;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("TopoNet_GetLinkSeed error: Invalid Geometry");
			    gaianet_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("TopoNet_GetLinkSeed error: not a BLOB value");
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoNet_GetLinkSeed error: \"%s\"",
				     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg (accessor, msg);
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
delete_all_seeds (struct gaia_network *net)
{
/* deleting all existing Seeds */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;

    table = sqlite3_mprintf ("%s_seeds", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

static int
update_outdated_link_seeds (struct gaia_network *net)
{
/* updating all outdated Link Seeds */
    char *table;
    char *xseeds;
    char *xlinks;
    char *sql;
    int ret;
    sqlite3_stmt *stmt_out;
    sqlite3_stmt *stmt_in;

/* preparing the UPDATE statement */
    table = sqlite3_mprintf ("%s_seeds", net->network_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET geometry = "
			   "TopoNet_GetLinkSeed(%Q, link_id) WHERE link_id = ?",
			   xseeds, net->network_name);
    free (xseeds);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the SELECT statement */
    table = sqlite3_mprintf ("%s_seeds", net->network_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xlinks = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT s.link_id FROM MAIN.\"%s\" AS s "
			   "JOIN MAIN.\"%s\" AS l ON (l.link_id = s.link_id) "
			   "WHERE s.link_id IS NOT NULL AND l.timestamp > s.timestamp",
			   xseeds, xlinks);
    free (xseeds);
    free (xlinks);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
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
			  ("TopoNet_UpdateSeeds() error: \"%s\"",
			   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr)
						  net, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"",
				     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
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

GAIANET_DECLARE int
gaiaTopoNetUpdateSeeds (GaiaNetworkAccessorPtr accessor, int incremental_mode)
{
/* updating all TopoNet Seeds */
    char *table;
    char *xseeds;
    char *xlinks;
    char *sql;
    char *errMsg;
    int ret;
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net == NULL)
	return 0;

    if (!incremental_mode)
      {
	  /* deleting all existing Seeds */
	  if (!delete_all_seeds (net))
	      return 0;
      }

/* paranoid precaution: deleting all orphan Link Seeds */
    table = sqlite3_mprintf ("%s_seeds", net->network_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xlinks = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM MAIN.\"%s\" WHERE link_id IN ("
			   "SELECT s.link_id FROM MAIN.\"%s\" AS s "
			   "LEFT JOIN MAIN.\"%s\" AS l ON (s.link_id = l.link_id) "
			   "WHERE l.link_id IS NULL)", xseeds, xseeds, xlinks);
    free (xseeds);
    free (xlinks);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* updating all outdated Link Seeds */
    if (!update_outdated_link_seeds (net))
	return 0;

/* inserting all missing Link Seeds */
    table = sqlite3_mprintf ("%s_seeds", net->network_name);
    xseeds = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xlinks = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (seed_id, link_id, geometry) "
	 "SELECT NULL, l.link_id, TopoNet_GetLinkSeed(%Q, l.link_id) "
	 "FROM MAIN.\"%s\" AS l "
	 "LEFT JOIN MAIN.\"%s\" AS s ON (l.link_id = s.link_id) WHERE s.link_id IS NULL",
	 xseeds, net->network_name, xlinks, xseeds);
    free (xseeds);
    free (xlinks);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_UpdateSeeds() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
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

GAIANET_DECLARE int
gaiaTopoNet_DisambiguateSegmentLinks (GaiaNetworkAccessorPtr accessor)
{
/*
/ Ensures that all Links on a Topology-Network will have not less
/ than three vertices; for all Links found beign simple two-points 
/ segments a third intermediate point will be interpolated.
*/
    struct gaia_network *net = (struct gaia_network *) accessor;
    int ret;
    char *sql;
    char *link;
    char *xlink;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int count = 0;
    if (net == NULL)
	return -1;

/* preparing the SQL query identifying all two-points Links */
    link = sqlite3_mprintf ("%s_link", net->network_name);
    xlink = gaiaDoubleQuotedSql (link);
    sqlite3_free (link);
    sql =
	sqlite3_mprintf
	("SELECT link_id, geometry FROM \"%s\" WHERE ST_NumPoints(geometry) = 2 "
	 "ORDER BY link_id", xlink);
    free (xlink);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_DisambiguateSegmentLinks error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the UPDATE SQL query */
    sql =
	sqlite3_mprintf ("SELECT ST_ChangeLinkGeom(%Q, ?, ?)",
			 net->network_name);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_DisambiguateSegmentLinks error: \"%s\"",
			       sqlite3_errmsg (net->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_in, 0);
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
				  sqlite3_bind_int64 (stmt_out, 1, link_id);
				  gaiaToSpatiaLiteBlobWkb (newg, &outblob,
							   &outblob_size);
				  gaiaFreeGeomColl (newg);
				  if (blob == NULL)
				      continue;
				  else
				      sqlite3_bind_blob (stmt_out, 2, outblob,
							 outblob_size, free);
				  /* updating the Links table */
				  ret = sqlite3_step (stmt_out);
				  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
				      count++;
				  else
				    {
					char *msg =
					    sqlite3_mprintf
					    ("TopoNet_DisambiguateSegmentLinks() error: \"%s\"",
					     sqlite3_errmsg (net->db_handle));
					gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
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
		    ("TopoNet_DisambiguateSegmentLinks error: \"%s\"",
		     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg (accessor, msg);
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

static void
do_eval_toponet_point (struct gaia_network *net, gaiaGeomCollPtr result,
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
			    if (net->has_z)
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
		char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_collect_topo_links (struct gaia_network *net, gaiaGeomCollPtr sparse,
		       sqlite3_stmt * stmt_link, sqlite3_int64 link_id)
{
/* collecting Link Geometries one by one */
    int ret;

/* initializing the Topo-Link query */
    sqlite3_reset (stmt_link);
    sqlite3_clear_bindings (stmt_link);
    sqlite3_bind_int64 (stmt_link, 1, link_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_link);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt_link, 0);
		int blob_sz = sqlite3_column_bytes (stmt_link, 0);
		gaiaGeomCollPtr geom =
		    gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      gaiaLinestringPtr ln = geom->FirstLinestring;
		      while (ln != NULL)
			{
			    if (net->has_z)
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
		char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		return;
	    }
      }
}

static void
do_eval_toponet_line (struct gaia_network *net, gaiaGeomCollPtr result,
		      gaiaGeomCollPtr reference, sqlite3_stmt * stmt_seed_link,
		      sqlite3_stmt * stmt_link)
{
/* retrieving Linestrings from Topology */
    int ret;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr sparse;
    gaiaGeomCollPtr rearranged;
    gaiaLinestringPtr ln;

    if (net->has_z)
	sparse = gaiaAllocGeomCollXYZ ();
    else
	sparse = gaiaAllocGeomColl ();
    sparse->Srid = net->srid;

/* initializing the Topo-Seed-Link query */
    gaiaToSpatiaLiteBlobWkb (reference, &p_blob, &n_bytes);
    sqlite3_reset (stmt_seed_link);
    sqlite3_clear_bindings (stmt_seed_link);
    sqlite3_bind_blob (stmt_seed_link, 1, p_blob, n_bytes, SQLITE_TRANSIENT);
    sqlite3_bind_blob (stmt_seed_link, 2, p_blob, n_bytes, SQLITE_TRANSIENT);
    free (p_blob);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_seed_link);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 link_id =
		    sqlite3_column_int64 (stmt_seed_link, 0);
		do_collect_topo_links (net, sparse, stmt_link, link_id);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable error: \"%s\"",
					     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		gaiaFreeGeomColl (sparse);
		return;
	    }
      }

/* attempting to rearrange sparse lines */
    rearranged = gaiaLineMerge_r (net->cache, sparse);
    gaiaFreeGeomColl (sparse);
    if (rearranged == NULL)
	return;
    ln = rearranged->FirstLinestring;
    while (ln != NULL)
      {
	  if (net->has_z)
	      auxtopo_copy_linestring3d (ln, result);
	  else
	      auxtopo_copy_linestring (ln, result);
	  ln = ln->Next;
      }
    gaiaFreeGeomColl (rearranged);
}

static gaiaGeomCollPtr
do_eval_toponet_geom (struct gaia_network *net, gaiaGeomCollPtr geom,
		      sqlite3_stmt * stmt_seed_link,
		      sqlite3_stmt * stmt_node, sqlite3_stmt * stmt_link,
		      int out_type)
{
/* retrieving Topology-Network geometries via matching Seeds */
    gaiaGeomCollPtr result;

    if (net->has_z)
	result = gaiaAllocGeomCollXYZ ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = net->srid;
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
		    auxtopo_make_geom_from_point (net->srid, net->has_z, pt);
		do_eval_toponet_point (net, result, reference, stmt_node);
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
		    auxtopo_make_geom_from_line (net->srid, ln);
		do_eval_toponet_line (net, result, reference, stmt_seed_link,
				      stmt_link);
		auxtopo_destroy_geom_from (reference);
		ln->Next = next;
		ln = ln->Next;
	    }
      }

    if (out_type == GAIA_MULTIPOLYGON || out_type == GAIA_GEOMETRYCOLLECTION
	|| out_type == GAIA_UNKNOWN)
      {
	  /* processing all Polygons */
	  if (geom->FirstPolygon != NULL)
	      goto error;
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
do_eval_toponet_seeds (struct gaia_network *net, sqlite3_stmt * stmt_ref,
		       int ref_geom_col, sqlite3_stmt * stmt_ins,
		       sqlite3_stmt * stmt_seed_link, sqlite3_stmt * stmt_node,
		       sqlite3_stmt * stmt_link, int out_type)
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
				  if (net->cache != NULL)
				    {
					struct splite_internal_cache *cache =
					    (struct splite_internal_cache
					     *) (net->cache);
					gpkg_mode = cache->gpkg_mode;
					tiny_point = cache->tinyPointEnabled;
				    }
				  result =
				      do_eval_toponet_geom (net, geom,
							    stmt_seed_link,
							    stmt_node,
							    stmt_link,
							    out_type);
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
			  sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
					   sqlite3_errmsg (net->db_handle));
		      gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr)
						  net, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				     sqlite3_errmsg (net->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

GAIANET_DECLARE int
gaiaTopoNet_ToGeoTableGeneralize (GaiaNetworkAccessorPtr accessor,
				  const char *db_prefix, const char *ref_table,
				  const char *ref_column, const char *out_table,
				  double tolerance, int with_spatial_index)
{
/* 
/ attempting to create and populate a new GeoTable out from a Topology-Network
/ (simplified/generalized version)
*/
    struct gaia_network *net = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt_ref = NULL;
    sqlite3_stmt *stmt_ins = NULL;
    sqlite3_stmt *stmt_seed_link = NULL;
    sqlite3_stmt *stmt_node = NULL;
    sqlite3_stmt *stmt_link = NULL;
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
    if (net == NULL)
	return 0;

/* incrementally updating all Topology Seeds */
    if (!gaiaTopoNetUpdateSeeds (accessor, 1))
	return 0;

/* composing the CREATE TABLE output-table statement */
    if (!auxtopo_create_togeotable_sql
	(net->db_handle, db_prefix, ref_table, ref_column, out_table, &create,
	 &select, &insert, &ref_geom_col))
	goto error;

/* creating the output-table */
    ret = sqlite3_exec (net->db_handle, create, NULL, NULL, &errMsg);
    sqlite3_free (create);
    create = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* checking the Geometry Type */
    if (!auxtopo_retrieve_geometry_type
	(net->db_handle, db_prefix, ref_table, ref_column, &ref_type))
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
	 out_table, ref_column, net->srid, type,
	 (net->has_z == 0) ? "XY" : "XYZ");
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
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
	  ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_ToGeoTable() error: \"%s\"",
				     errMsg);
		sqlite3_free (errMsg);
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

/* preparing the "SELECT * FROM ref-table" query */
    ret =
	sqlite3_prepare_v2 (net->db_handle, select, strlen (select), &stmt_ref,
			    NULL);
    sqlite3_free (select);
    select = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the "INSERT INTO out-table" query */
    ret =
	sqlite3_prepare_v2 (net->db_handle, insert, strlen (insert), &stmt_ins,
			    NULL);
    sqlite3_free (insert);
    insert = NULL;
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Seed-Links query */
    xprefix = sqlite3_mprintf ("%s_seeds", net->network_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT link_id FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geometry, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_seed_link,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Nodes query */
    xprefix = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sql = sqlite3_mprintf ("SELECT geometry FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(geometry, ?) = 1 AND ROWID IN ("
			   "SELECT ROWID FROM SpatialIndex WHERE f_table_name = %Q AND search_frame = ?)",
			   xtable, xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_node,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the Topo-Links query */
    xprefix = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (xprefix);
    if (tolerance > 0.0)
	sql =
	    sqlite3_mprintf
	    ("SELECT ST_SimplifyPreserveTopology(geometry, %1.6f) FROM MAIN.\"%s\" WHERE link_id = ?",
	     tolerance, xtable, xprefix);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT geometry FROM MAIN.\"%s\" WHERE link_id = ?", xtable,
	     xprefix);
    free (xtable);
    sqlite3_free (xprefix);
    ret =
	sqlite3_prepare_v2 (net->db_handle, sql, strlen (sql), &stmt_link,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("TopoNet_ToGeoTable() error: \"%s\"",
				       sqlite3_errmsg (net->db_handle));
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* evaluating feature/topology matching via coincident topo-seeds */
    if (!do_eval_toponet_seeds
	(net, stmt_ref, ref_geom_col, stmt_ins, stmt_seed_link, stmt_node,
	 stmt_link, out_type))
	goto error;

    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_ins);
    sqlite3_finalize (stmt_seed_link);
    sqlite3_finalize (stmt_node);
    sqlite3_finalize (stmt_link);
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
    if (stmt_seed_link != NULL)
	sqlite3_finalize (stmt_seed_link);
    if (stmt_node != NULL)
	sqlite3_finalize (stmt_node);
    if (stmt_link != NULL)
	sqlite3_finalize (stmt_link);
    return 0;
}

GAIANET_DECLARE int
gaiaTopoNet_ToGeoTable (GaiaNetworkAccessorPtr accessor,
			const char *db_prefix, const char *ref_table,
			const char *ref_column, const char *out_table,
			int with_spatial_index)
{
/* attempting to create and populate a new GeoTable out from a Topology-Network */
    return gaiaTopoNet_ToGeoTableGeneralize (accessor, db_prefix, ref_table,
					     ref_column, out_table, -1.0,
					     with_spatial_index);
}

static int
find_linelink_matches (struct gaia_network *network,
		       sqlite3_stmt * stmt_ref, sqlite3_stmt * stmt_ins,
		       sqlite3_int64 link_id, const unsigned char *blob,
		       int blob_sz)
{
/* retrieving LineLink relationships */
    int ret;
    int count = 0;
    char direction[2];
    strcpy (direction, "?");

    sqlite3_reset (stmt_ref);
    sqlite3_clear_bindings (stmt_ref);
    sqlite3_bind_int64 (stmt_ref, 1, link_id);

    while (1)
      {
	  /* scrolling the result set rows - Spatial Relationships */
	  ret = sqlite3_step (stmt_ref);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_ref, 0);

		if (sqlite3_column_type (stmt_ref, 1) == SQLITE_BLOB)
		  {
		      /* testing directions */
		      gaiaGeomCollPtr geom_link = NULL;
		      gaiaGeomCollPtr geom_line = NULL;
		      const unsigned char *blob2 =
			  sqlite3_column_blob (stmt_ref, 1);
		      int blob2_sz = sqlite3_column_bytes (stmt_ref, 1);
		      geom_link = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      geom_line = gaiaFromSpatiaLiteBlobWkb (blob2, blob2_sz);
		      if (geom_link != NULL && geom_line != NULL)
			  gaia_do_check_direction (geom_link, geom_line,
						   direction);
		      if (geom_link != NULL)
			  gaiaFreeGeomColl (geom_link);
		      if (geom_line != NULL)
			  gaiaFreeGeomColl (geom_line);
		  }

		sqlite3_reset (stmt_ins);
		sqlite3_clear_bindings (stmt_ins);
		sqlite3_bind_int64 (stmt_ins, 1, link_id);
		sqlite3_bind_int64 (stmt_ins, 2, rowid);
		sqlite3_bind_text (stmt_ins, 3, direction, 1, SQLITE_STATIC);
		/* inserting a row into the output table */
		ret = sqlite3_step (stmt_ins);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    count++;
		else
		  {
		      char *msg =
			  sqlite3_mprintf ("LineLinksList error: \"%s\"",
					   sqlite3_errmsg (network->db_handle));
		      gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr)
						  network, msg);
		      sqlite3_free (msg);
		      return 0;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
					     sqlite3_errmsg
					     (network->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) network,
					    msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    if (count == 0)
      {
	  /* unrelated Link */
	  sqlite3_reset (stmt_ins);
	  sqlite3_clear_bindings (stmt_ins);
	  sqlite3_bind_int64 (stmt_ins, 1, link_id);
	  sqlite3_bind_null (stmt_ins, 2);
	  sqlite3_bind_null (stmt_ins, 3);
	  /* inserting a row into the output table */
	  ret = sqlite3_step (stmt_ins);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
					     sqlite3_errmsg
					     (network->db_handle));
		gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) network,
					    msg);
		sqlite3_free (msg);
		return 0;
	    }
      }
    return 1;
}

static int
insert_linelink_reverse (struct gaia_network *network, sqlite3_stmt * stmt_ins,
			 sqlite3_int64 polygon_id)
{
/* found a mismatching RefLinestring - inserting into the output table */
    int ret;

    sqlite3_reset (stmt_ins);
    sqlite3_clear_bindings (stmt_ins);
    sqlite3_bind_null (stmt_ins, 1);
    sqlite3_bind_int64 (stmt_ins, 2, polygon_id);
    sqlite3_bind_null (stmt_ins, 3);
    /* inserting a row into the output table */
    ret = sqlite3_step (stmt_ins);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) network, msg);
	  sqlite3_free (msg);
	  return 0;
      }
    return 1;
}

GAIANET_DECLARE int
gaiaTopoNet_LineLinksList (GaiaNetworkAccessorPtr accessor,
			   const char *db_prefix, const char *ref_table,
			   const char *ref_column, const char *out_table)
{
/* creating and populating a new Table reporting about Links/Linestring correspondencies */
    struct gaia_network *network = (struct gaia_network *) accessor;
    sqlite3_stmt *stmt_links = NULL;
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
    if (network == NULL)
	return 0;

/* attempting to build the output table */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("CREATE TABLE main.\"%s\" (\n"
			   "\tid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tlink_id INTEGER,\n"
			   "\tref_rowid INTEGER,\n"
			   "\tdirection TEXT)", xtable);
    free (xtable);
    ret = sqlite3_exec (network->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    idx_name = sqlite3_mprintf ("idx_%s_link_id", out_table);
    xidx_name = gaiaDoubleQuotedSql (idx_name);
    sqlite3_free (idx_name);
    xtable = gaiaDoubleQuotedSql (out_table);
    sql =
	sqlite3_mprintf
	("CREATE INDEX main.\"%s\" ON \"%s\" (link_id, ref_rowid)", xidx_name,
	 xtable);
    free (xidx_name);
    free (xtable);
    ret = sqlite3_exec (network->db_handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Links SQL statement */
    table = sqlite3_mprintf ("%s_link", network->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT link_id, geometry FROM main.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (network->db_handle, sql, strlen (sql), &stmt_links,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the RefTable SQL statement */
    seeds = sqlite3_mprintf ("%s_seeds", network->network_name);
    rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix, ref_table);
    ref_has_spatial_index =
	gaia_check_spatial_index (network->db_handle, db_prefix, ref_table,
				  ref_column);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    xcolumn = gaiaDoubleQuotedSql (ref_column);
    xseeds = gaiaDoubleQuotedSql (seeds);
    if (ref_has_spatial_index)
	sql =
	    sqlite3_mprintf
	    ("SELECT r.rowid, r.\"%s\" FROM MAIN.\"%s\" AS s, \"%s\".\"%s\" AS r "
	     "WHERE ST_Intersects(r.\"%s\", s.geometry) == 1 AND s.link_id = ? "
	     "AND r.rowid IN (SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q "
	     "AND f_geometry_column = %Q AND search_frame = s.geometry)",
	     xcolumn, xseeds, xprefix, xtable, xcolumn, rtree_name, xcolumn);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT r.rowid, r.\"%s\" FROM MAIN.\"%s\" AS s, \"%s\".\"%s\" AS r "
	     "WHERE  ST_Intersects(r.\"%s\", s.geometry) == 1 AND s.link_id = ?",
	     xcolumn, xseeds, xprefix, xtable, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    free (xseeds);
    sqlite3_free (rtree_name);
    sqlite3_free (seeds);
    ret =
	sqlite3_prepare_v2 (network->db_handle, sql, strlen (sql), &stmt_ref,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Reverse RefTable SQL statement */
    seeds = sqlite3_mprintf ("%s_seeds", network->network_name);
    rtree_name = sqlite3_mprintf ("DB=%s.%s", db_prefix, ref_table);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (ref_table);
    xcolumn = gaiaDoubleQuotedSql (ref_column);
    xseeds = gaiaDoubleQuotedSql (seeds);
    sql = sqlite3_mprintf ("SELECT r.rowid FROM \"%s\".\"%s\" AS r "
			   "LEFT JOIN MAIN.\"%s\" AS s ON (ST_Intersects(r.\"%s\", s.geometry) = 1 "
			   "AND s.link_id IS NOT NULL AND s.rowid IN (SELECT rowid FROM SpatialIndex "
			   "WHERE f_table_name = %Q AND search_frame = r.\"%s\")) "
			   "WHERE s.link_id IS NULL", xprefix, xtable, xseeds,
			   xcolumn, rtree_name, xcolumn);
    free (xprefix);
    free (xtable);
    free (xcolumn);
    free (xseeds);
    sqlite3_free (rtree_name);
    sqlite3_free (seeds);
    ret =
	sqlite3_prepare_v2 (network->db_handle, sql, strlen (sql), &stmt_rev,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("PolyFacesList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* building the Insert SQL statement */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO main.\"%s\" (id, link_id, ref_rowid, "
			   "direction) VALUES (NULL, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (network->db_handle, sql, strlen (sql), &stmt_ins,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
				       sqlite3_errmsg (network->db_handle));
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - Links */
	  ret = sqlite3_step (stmt_links);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt_links, 0);
		if (sqlite3_column_type (stmt_links, 1) == SQLITE_BLOB)
		  {
		      if (!find_linelink_matches
			  (network, stmt_ref, stmt_ins, link_id,
			   sqlite3_column_blob (stmt_links, 1),
			   sqlite3_column_bytes (stmt_links, 1)))
			  goto error;
		  }
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("LineLinksList error: Link not a BLOB value");
		      gaianet_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
					     sqlite3_errmsg
					     (network->db_handle));
		gaianet_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    while (1)
      {
	  /* scrolling the Reverse result set rows - Linestrings */
	  ret = sqlite3_step (stmt_rev);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 line_id = sqlite3_column_int64 (stmt_rev, 0);
		if (!insert_linelink_reverse (network, stmt_ins, line_id))
		    goto error;
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("LineLinksList error: \"%s\"",
					     sqlite3_errmsg
					     (network->db_handle));
		gaianet_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt_links);
    sqlite3_finalize (stmt_ref);
    sqlite3_finalize (stmt_rev);
    sqlite3_finalize (stmt_ins);
    return 1;

  error:
    if (stmt_links != NULL)
	sqlite3_finalize (stmt_links);
    if (stmt_ref != NULL)
	sqlite3_finalize (stmt_ref);
    if (stmt_rev != NULL)
	sqlite3_finalize (stmt_rev);
    if (stmt_ins != NULL)
	sqlite3_finalize (stmt_ins);
    return 0;
}

#endif /* end RTTOPO conditionals */
