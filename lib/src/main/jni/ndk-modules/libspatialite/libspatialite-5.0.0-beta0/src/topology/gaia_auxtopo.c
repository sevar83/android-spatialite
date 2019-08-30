/*

 gaia_auxtopo.c -- implementation of the Topology module methods
    
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
#include "process.h"
#else
#include "unistd.h"
#endif

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

SPATIALITE_PRIVATE void
free_internal_cache_topologies (void *firstTopology)
{
/* destroying all Topologies registered into the Internal Connection Cache */
    struct gaia_topology *p_topo = (struct gaia_topology *) firstTopology;
    struct gaia_topology *p_topo_n;

    while (p_topo != NULL)
      {
	  p_topo_n = p_topo->next;
	  gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo_n;
      }
}

SPATIALITE_PRIVATE int
do_create_topologies (void *sqlite_handle)
{
/* attempting to create the Topologies table (if not already existing) */
    const char *sql;
    char *err_msg = NULL;
    int ret;
    sqlite3 *handle = (sqlite3 *) sqlite_handle;

    sql = "CREATE TABLE IF NOT EXISTS topologies (\n"
	"\ttopology_name TEXT NOT NULL PRIMARY KEY,\n"
	"\tsrid INTEGER NOT NULL,\n"
	"\ttolerance DOUBLE NOT NULL,\n"
	"\thas_z INTEGER NOT NULL,\n"
	"\tnext_edge_id INTEGER NOT NULL DEFAULT 1,\n"
	"\tCONSTRAINT topo_srid_fk FOREIGN KEY (srid) "
	"REFERENCES spatial_ref_sys (srid))";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topologies - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating Topologies triggers */
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_insert\n"
	"BEFORE INSERT ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'insert on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    sql = "CREATE TRIGGER IF NOT EXISTS topology_name_update\n"
	"BEFORE UPDATE OF 'topology_name' ON 'topologies'\nFOR EACH ROW BEGIN\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a single quote')\n"
	"WHERE NEW.topology_name LIKE ('%''%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must not contain a double quote')\n"
	"WHERE NEW.topology_name LIKE ('%\"%');\n"
	"SELECT RAISE(ABORT,'update on topologies violates constraint: "
	"topology_name value must be lower case')\n"
	"WHERE NEW.topology_name <> lower(NEW.topology_name);\nEND";
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
check_new_topology (sqlite3 * handle, const char *topo_name)
{
/* testing if some already defined DB object forbids creating the new Topology */
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

/* testing if the same Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
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
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
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

/* testing if some Spatial View is already defined in views_geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM views_geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
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
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_seeds_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
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

static int
do_create_face (sqlite3 * handle, const char *topo_name, int srid)
{
/* attempting to create the Topology Face table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tface_id INTEGER PRIMARY KEY AUTOINCREMENT)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Face BBOX Geometry */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'mbr', %d, 'POLYGON', 'XY')",
	 table, srid);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Face Geometry */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-FACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* inserting the default World Face */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" VALUES (0, NULL)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT WorldFACE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_node (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Node table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint;
    char *xmother;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node_face_fk", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xmother = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tnode_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tcontaining_face INTEGER,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (containing_face) "
			   "REFERENCES \"%s\" (face_id))", xtable, xconstraint,
			   xmother);
    free (xtable);
    free (xconstraint);
    free (xmother);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'POINT', %Q, 1)", table,
	 srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Node Geometry */
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-NODE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "containing_face" */
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_node_contface", topo_name);
    xconstraint = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (containing_face)",
			 xconstraint, xtable);
    free (xtable);
    free (xconstraint);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX node-contface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_edge (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Edge table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xconstraint3;
    char *xconstraint4;
    char *xnodes;
    char *xfaces;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_start_fk", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_node_end_fk", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_left_fk", topo_name);
    xconstraint3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge_face_right_fk", topo_name);
    xconstraint4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xnodes = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tedge_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tstart_node INTEGER NOT NULL,\n"
			   "\tend_node INTEGER NOT NULL,\n"
			   "\tnext_left_edge INTEGER NOT NULL,\n"
			   "\tnext_right_edge INTEGER NOT NULL,\n"
			   "\tleft_face INTEGER,\n"
			   "\tright_face INTEGER,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (start_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (end_node) "
			   "REFERENCES \"%s\" (node_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (left_face) "
			   "REFERENCES \"%s\" (face_id),\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (right_face) "
			   "REFERENCES \"%s\" (face_id))",
			   xtable, xconstraint1, xnodes, xconstraint2, xnodes,
			   xconstraint3, xfaces, xconstraint4, xfaces);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xconstraint3);
    free (xconstraint4);
    free (xnodes);
    free (xfaces);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_ins" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER INSERT ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
			   "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE edge_id = NEW.edge_id;"
			   "END", xtrigger, xtable, topo_name, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "edge_update" trigger */
    trigger = sqlite3_mprintf ("%s_edge_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER \"%s\" AFTER UPDATE ON \"%s\"\n"
			   "FOR EACH ROW BEGIN\n"
			   "\tUPDATE \"%s\" SET timestamp = strftime('%%Y-%%m-%%dT%%H:%%M:%%fZ', 'now') "
			   "WHERE edge_id = NEW.edge_id;"
			   "END", xtrigger, xtable, xtable);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "next_edge_upd" trigger */
    trigger = sqlite3_mprintf ("%s_edge_next_upd", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TRIGGER \"%s\" AFTER UPDATE OF edge_id ON \"%s\"\n"
	 "FOR EACH ROW BEGIN\n"
	 "\tUPDATE topologies SET next_edge_id = NEW.edge_id + 1 "
	 "WHERE Lower(topology_name) = Lower(%Q) AND next_edge_id < NEW.edge_id + 1;\n"
	 "END", xtrigger, xtable, topo_name);
    free (xtrigger);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CREATE TRIGGER topology-EDGE next UPDATE - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'LINESTRING', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Edge Geometry */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-EDGE - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "start_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_start_node", topo_name);
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
	  spatialite_e ("CREATE INDEX edge-startnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "end_node" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_end_node", topo_name);
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
	  spatialite_e ("CREATE INDEX edge-endnode - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "left_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_leftface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (left_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-leftface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "right_face" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_edge_rightface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (right_face)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX edge-rightface - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_timestamp", topo_name);
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
	  spatialite_e ("CREATE INDEX edge-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_seeds (sqlite3 * handle, const char *topo_name, int srid, int has_z)
{
/* attempting to create the Topology Seeds table */
    char *sql;
    char *table;
    char *xtable;
    char *xconstraint1;
    char *xconstraint2;
    char *xedges;
    char *xfaces;
    char *trigger;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds_edge_fk", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds_face_fk", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xedges = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xfaces = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tseed_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tedge_id INTEGER,\n"
			   "\tface_id INTEGER,\n"
			   "\ttimestamp DATETIME,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (edge_id) "
			   "REFERENCES \"%s\" (edge_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (face_id) "
			   "REFERENCES \"%s\" (face_id) ON DELETE CASCADE)",
			   xtable, xconstraint1, xedges, xconstraint2, xfaces);
    free (xtable);
    free (xconstraint1);
    free (xconstraint2);
    free (xedges);
    free (xfaces);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_ins" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_ins", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
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
	      ("CREATE TRIGGER topology-SEEDS next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* adding the "seeds_update" trigger */
    trigger = sqlite3_mprintf ("%s_seeds_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (trigger);
    sqlite3_free (trigger);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
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
	      ("CREATE TRIGGER topology-SEED next INSERT - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating the Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(%Q, 'geom', %d, 'POINT', %Q, 1)",
	 table, srid, has_z ? "XYZ" : "XY");
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("AddGeometryColumn topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating a Spatial Index supporting Seeds Geometry */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, 'geom')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("CreateSpatialIndex topology-SEEDS - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "edge_id" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_sdedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (edge_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-edge - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "face_id" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_sdface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (face_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX seeds-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "timestamp" */
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_seeds_timestamp", topo_name);
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
	  spatialite_e ("CREATE INDEX seeds-timestamps - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_edge_seeds (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Edge Seeds view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT seed_id AS rowid, edge_id AS edge_id, geom AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE edge_id IS NOT NULL", xview, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-EDGE-SEEDS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    xtable = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'geom', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-EDGE-SEEDS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_face_seeds (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Face Seeds view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_seeds", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT seed_id AS rowid, face_id AS face_id, geom AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE face_id IS NOT NULL", xview, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-FACE-SEEDS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_face_seeds", topo_name);
    xtable = sqlite3_mprintf ("%s_seeds", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'geom', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-FACE-SEEDS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_face_geoms (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the Face Geoms view */
    char *sql;
    char *table;
    char *xtable;
    char *xview;
    char *err_msg = NULL;
    int ret;

/* creating the view */
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    xview = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\" AS\n"
			   "SELECT face_id AS rowid, ST_GetFaceGeometry(%Q, face_id) AS geom\n"
			   "FROM \"%s\"\n"
			   "WHERE face_id <> 0", xview, topo_name, xtable);
    free (xtable);
    free (xview);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE VIEW topology-FACE-GEOMS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* registering a Spatial View */
    xview = sqlite3_mprintf ("%s_face_geoms", topo_name);
    xtable = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("INSERT INTO views_geometry_columns (view_name, "
			   "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
			   "VALUES (Lower(%Q), 'geom', 'rowid', Lower(%Q), 'mbr', 1)",
			   xview, xtable);
    sqlite3_free (xview);
    sqlite3_free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("Registering Spatial VIEW topology-FACE-GEOMS - error: %s\n",
	       err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_topolayers (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the TopoLayers table */
    char *sql;
    char *table;
    char *xtable;
    char *xtrigger;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\ttopolayer_id INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\ttopolayer_name NOT NULL UNIQUE)", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-TOPOLAYERS - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating TopoLayers triggers */
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayer_name_insert", topo_name);
    xtrigger = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER IF NOT EXISTS \"%s\"\n"
			   "BEFORE INSERT ON \"%s\"\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayer_name value must not contain a single quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%''%%');\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayers_name value must not contain a double quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%\"%%');\n"
			   "SELECT RAISE(ABORT,'insert on topolayers violates constraint: "
			   "topolayer_name value must be lower case')\n"
			   "WHERE NEW.topolayer_name <> lower(NEW.topolayer_name);\nEND",
			   xtrigger, xtable);
    free (xtable);
    free (xtrigger);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayer_name_update", topo_name);
    xtrigger = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TRIGGER IF NOT EXISTS \"%s\"\n"
			   "BEFORE UPDATE OF 'topolayer_name' ON \"%s\"\nFOR EACH ROW BEGIN\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must not contain a single quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%''%%');\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must not contain a double quote')\n"
			   "WHERE NEW.topolayer_name LIKE ('%%\"%%');\n"
			   "SELECT RAISE(ABORT,'update on topolayers violates constraint: "
			   "topolayer_name value must be lower case')\n"
			   "WHERE NEW.topolayer_name <> lower(NEW.topolayer_name);\nEND",
			   xtrigger, xtable);
    free (xtable);
    free (xtrigger);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SQL error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_create_topofeatures (sqlite3 * handle, const char *topo_name)
{
/* attempting to create the TopoFeatures table */
    char *sql;
    char *table;
    char *xtable;
    char *xtable1;
    char *xtable2;
    char *xtable3;
    char *xtable4;
    char *xconstraint1;
    char *xconstraint2;
    char *xconstraint3;
    char *xconstraint4;
    char *err_msg = NULL;
    int ret;

/* creating the main table */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftnode", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftedge", topo_name);
    xconstraint2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_ftface", topo_name);
    xconstraint3 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("fk_%s_topolayer", topo_name);
    xconstraint4 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (\n"
			   "\tuid INTEGER PRIMARY KEY AUTOINCREMENT,\n"
			   "\tnode_id INTEGER,\n\tedge_id INTEGER,\n"
			   "\tface_id INTEGER,\n\ttopolayer_id INTEGER NOT NULL,\n"
			   "\tfid INTEGER NOT NULL,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (node_id) "
			   "REFERENCES \"%s\" (node_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (edge_id) "
			   "REFERENCES \"%s\" (edge_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (face_id) "
			   "REFERENCES \"%s\" (face_id) ON DELETE CASCADE,\n"
			   "\tCONSTRAINT \"%s\" FOREIGN KEY (topolayer_id) "
			   "REFERENCES \"%s\" (topolayer_id) ON DELETE CASCADE)",
			   xtable, xconstraint1, xtable1, xconstraint2, xtable2,
			   xconstraint3, xtable3, xconstraint4, xtable4);
    free (xtable);
    free (xtable1);
    free (xtable2);
    free (xtable3);
    free (xtable4);
    free (xconstraint1);
    free (xconstraint2);
    free (xconstraint3);
    free (xconstraint4);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE TABLE topology-TOPOFEATURES - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "node_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftnode", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (node_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-node - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "edge_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftedge", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (edge_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-edge - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "face_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_ftface", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (face_id)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-face - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* creating an Index supporting "topolayers_id" */
    table = sqlite3_mprintf ("%s_topofeatures", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("idx_%s_fttopolayers", topo_name);
    xconstraint1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("CREATE INDEX \"%s\" ON \"%s\" (topolayer_id, fid)",
			 xconstraint1, xtable);
    free (xtable);
    free (xconstraint1);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("CREATE INDEX topofeatures-topolayers - error: %s\n",
			err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaTopologyCreate (sqlite3 * handle, const char *topo_name, int srid,
		    double tolerance, int has_z)
{
/* attempting to create a new Topology */
    int ret;
    char *sql;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for forbidding objects */
    if (!check_new_topology (handle, topo_name))
	return 0;

/* creating the Topology own Tables */
    if (!do_create_face (handle, topo_name, srid))
	goto error;
    if (!do_create_node (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_edge (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_seeds (handle, topo_name, srid, has_z))
	goto error;
    if (!do_create_edge_seeds (handle, topo_name))
	goto error;
    if (!do_create_face_seeds (handle, topo_name))
	goto error;
    if (!do_create_face_geoms (handle, topo_name))
	goto error;
    if (!do_create_topolayers (handle, topo_name))
	goto error;
    if (!do_create_topofeatures (handle, topo_name))
	goto error;

/* registering the Topology */
    sql = sqlite3_mprintf ("INSERT INTO MAIN.topologies (topology_name, "
			   "srid, tolerance, has_z) VALUES (Lower(%Q), %d, %f, %d)",
			   topo_name, srid, tolerance, has_z);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

static int
check_existing_topology (sqlite3 * handle, const char *topo_name,
			 int full_check)
{
/* testing if a Topology is already defined */
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

/* testing if the Topology is already defined */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.topologies WHERE "
			   "Lower(topology_name) = Lower(%Q)", topo_name);
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
    sql = sqlite3_mprintf ("SELECT Count(*) FROM geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(f_table_name) = Lower(%Q) AND f_geometry_column = 'mbr')",
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
		if (atoi (value) != 3)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* testing if all Spatial Views are correctly defined in geometry_columns */
    sql = sqlite3_mprintf ("SELECT Count(*) FROM views_geometry_columns WHERE");
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
	 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql =
	sqlite3_mprintf
	("%s OR (Lower(view_name) = Lower(%Q) AND view_geometry = 'geom')",
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
		if (atoi (value) != 3)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;


/* testing if all tables are already defined */
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM sqlite_master WHERE (type = 'table' AND (");
    prev = sql;
    table = sqlite3_mprintf ("%s_node", topo_name);
    sql = sqlite3_mprintf ("%s Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_node_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_edge_geom", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)))", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_edge_seeds", topo_name);
    sql =
	sqlite3_mprintf ("%s OR (type = 'view' AND (Lower(name) = Lower(%Q)",
			 prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_seeds", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)", prev, table);
    sqlite3_free (table);
    sqlite3_free (prev);
    prev = sql;
    table = sqlite3_mprintf ("%s_face_geoms", topo_name);
    sql = sqlite3_mprintf ("%s OR Lower(name) = Lower(%Q)))", prev, table);
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
		if (atoi (value) != 9)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

    return 1;
}

static int
do_drop_topo_face (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop the Topology-Face table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* disabling the corresponding Spatial Index */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableSpatialIndex topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* discarding the Geometry column */
    table = sqlite3_mprintf ("%s_face", topo_name);
    sql = sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'mbr')", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DisableGeometryColumn topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_face", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

/* dropping the corresponding Spatial Index */
    table = sqlite3_mprintf ("idx_%s_face_mbr", topo_name);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS \"%s\"", table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (table);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("DROP SpatialIndex topology-face - error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_drop_topo_table (sqlite3 * handle, const char *topo_name, const char *which,
		    int spatial)
{
/* attempting to drop some Topology table */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

    if (strcmp (which, "face") == 0)
	return do_drop_topo_face (handle, topo_name);

    if (spatial)
      {
	  /* disabling the corresponding Spatial Index */
	  table = sqlite3_mprintf ("%s_%s", topo_name, which);
	  sql =
	      sqlite3_mprintf ("SELECT DisableSpatialIndex(%Q, 'geom')", table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DisableSpatialIndex topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
	  /* discarding the Geometry column */
	  table = sqlite3_mprintf ("%s_%s", topo_name, which);
	  sql =
	      sqlite3_mprintf ("SELECT DiscardGeometryColumn(%Q, 'geom')",
			       table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DisableGeometryColumn topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
      }

/* dropping the main table */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    if (spatial)
      {
	  /* dropping the corresponding Spatial Index */
	  table = sqlite3_mprintf ("idx_%s_%s_geom", topo_name, which);
	  sql = sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"", table);
	  ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
	  sqlite3_free (table);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		spatialite_e
		    ("DROP SpatialIndex topology-%s - error: %s\n", which,
		     err_msg);
		sqlite3_free (err_msg);
		return 0;
	    }
      }

    return 1;
}

static int
do_drop_topo_view (sqlite3 * handle, const char *topo_name, const char *which)
{
/* attempting to drop some Topology view */
    char *sql;
    char *table;
    char *xtable;
    char *err_msg = NULL;
    int ret;

/* unregistering the Spatial View */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    sql =
	sqlite3_mprintf
	("DELETE FROM views_geometry_columns WHERE view_name = Lower(%Q)",
	 table);
    sqlite3_free (table);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("Unregister Spatial View -%s - error: %s\n", which,
			err_msg);
	  sqlite3_free (err_msg);
      }

/* dropping the view */
    table = sqlite3_mprintf ("%s_%s", topo_name, which);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP VIEW IF EXISTS MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("DROP topology-%s - error: %s\n", which, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }

    return 1;
}

static int
do_drop_topofeature_tables (sqlite3 * handle, const char *topo_name)
{
/* dropping any eventual topofeatures table */
    char *table;
    char *xtable;
    char *sql;
    char *err_msg = NULL;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;

    table = sqlite3_mprintf ("%s_topolayers", topo_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT topolayer_id FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *id = results[(i * columns) + 0];
		table = sqlite3_mprintf ("%s_topofeatures_%s", topo_name, id);
		xtable = gaiaDoubleQuotedSql (table);
		sqlite3_free (table);
		sql =
		    sqlite3_mprintf ("DROP TABLE IF EXISTS MAIN.\"%s\"",
				     xtable);
		free (xtable);
		ret = sqlite3_exec (handle, sql, NULL, NULL, &err_msg);
		sqlite3_free (sql);
		if (ret != SQLITE_OK)
		  {
		      spatialite_e ("DROP topology-features (%s) - error: %s\n",
				    id, err_msg);
		      sqlite3_free (err_msg);
		      return 0;
		  }
	    }
      }
    sqlite3_free_table (results);
    return 1;
}

static int
do_get_topology (sqlite3 * handle, const char *topo_name, char **topology_name,
		 int *srid, double *tolerance, int *has_z)
{
/* retrieving a Topology configuration */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xtopology_name = NULL;
    int xsrid;
    double xtolerance;
    int xhas_z;

/* preparing the SQL query */
    sql =
	sqlite3_mprintf
	("SELECT topology_name, srid, tolerance, has_z FROM MAIN.topologies WHERE "
	 "Lower(topology_name) = Lower(%Q)", topo_name);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM topologys error: \"%s\"\n",
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
		int ok_tolerance = 0;
		int ok_z = 0;
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		  {
		      const char *str =
			  (const char *) sqlite3_column_text (stmt, 0);
		      if (xtopology_name != NULL)
			  free (xtopology_name);
		      xtopology_name = malloc (strlen (str) + 1);
		      strcpy (xtopology_name, str);
		      ok_name = 1;
		  }
		if (sqlite3_column_type (stmt, 1) == SQLITE_INTEGER)
		  {
		      xsrid = sqlite3_column_int (stmt, 1);
		      ok_srid = 1;
		  }
		if (sqlite3_column_type (stmt, 2) == SQLITE_FLOAT)
		  {
		      xtolerance = sqlite3_column_double (stmt, 2);
		      ok_tolerance = 1;
		  }
		if (sqlite3_column_type (stmt, 3) == SQLITE_INTEGER)
		  {
		      xhas_z = sqlite3_column_int (stmt, 3);
		      ok_z = 1;
		  }
		if (ok_name && ok_srid && ok_tolerance && ok_z)
		  {
		      ok = 1;
		      break;
		  }
	    }
	  else
	    {
		spatialite_e
		    ("step: SELECT FROM topologies error: \"%s\"\n",
		     sqlite3_errmsg (handle));
		sqlite3_finalize (stmt);
		return 0;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *topology_name = xtopology_name;
	  *srid = xsrid;
	  *tolerance = xtolerance;
	  *has_z = xhas_z;
	  return 1;
      }

    if (xtopology_name != NULL)
	free (xtopology_name);
    return 0;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaGetTopology (sqlite3 * handle, const void *cache, const char *topo_name)
{
/* attempting to get a reference to some Topology Accessor Object */
    GaiaTopologyAccessorPtr accessor;

/* attempting to retrieve an alredy cached definition */
    accessor = gaiaTopologyFromCache (cache, topo_name);
    if (accessor != NULL)
	return accessor;

/* attempting to create a new Topology Accessor */
    accessor = gaiaTopologyFromDBMS (handle, cache, topo_name);
    return accessor;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromCache (const void *p_cache, const char *topo_name)
{
/* attempting to retrieve an already defined Topology Accessor Object from the Connection Cache */
    struct gaia_topology *ptr;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == 0)
	return NULL;

    ptr = (struct gaia_topology *) (cache->firstTopology);
    while (ptr != NULL)
      {
	  /* checking for an already registered Topology */
	  if (strcasecmp (topo_name, ptr->topology_name) == 0)
	      return (GaiaTopologyAccessorPtr) ptr;
	  ptr = ptr->next;
      }
    return NULL;
}

GAIATOPO_DECLARE int
gaiaReadTopologyFromDBMS (sqlite3 *
			  handle,
			  const char
			  *topo_name, char **topology_name, int *srid,
			  double *tolerance, int *has_z)
{
/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 1))
	return 0;

/* retrieving the Topology configuration */
    if (!do_get_topology
	(handle, topo_name, topology_name, srid, tolerance, has_z))
	return 0;
    return 1;
}

GAIATOPO_DECLARE GaiaTopologyAccessorPtr
gaiaTopologyFromDBMS (sqlite3 * handle, const void *p_cache,
		      const char *topo_name)
{
/* attempting to create a Topology Accessor Object into the Connection Cache */
    const RTCTX *ctx = NULL;
    RTT_BE_CALLBACKS *callbacks;
    struct gaia_topology *ptr;
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
    ptr = malloc (sizeof (struct gaia_topology));
    ptr->db_handle = handle;
    ptr->cache = cache;
    ptr->topology_name = NULL;
    ptr->srid = -1;
    ptr->tolerance = 0;
    ptr->has_z = 0;
    ptr->last_error_message = NULL;
    ptr->rtt_iface = rtt_CreateBackendIface (ctx, (const RTT_BE_DATA *) ptr);
    ptr->prev = cache->lastTopology;
    ptr->next = NULL;

    callbacks = malloc (sizeof (RTT_BE_CALLBACKS));
    callbacks->lastErrorMessage = callback_lastErrorMessage;
    callbacks->topoGetSRID = callback_topoGetSRID;
    callbacks->topoGetPrecision = callback_topoGetPrecision;
    callbacks->topoHasZ = callback_topoHasZ;
    callbacks->createTopology = NULL;
    callbacks->loadTopologyByName = callback_loadTopologyByName;
    callbacks->freeTopology = callback_freeTopology;
    callbacks->getNodeById = callback_getNodeById;
    callbacks->getNodeWithinDistance2D = callback_getNodeWithinDistance2D;
    callbacks->insertNodes = callback_insertNodes;
    callbacks->getEdgeById = callback_getEdgeById;
    callbacks->getEdgeWithinDistance2D = callback_getEdgeWithinDistance2D;
    callbacks->getNextEdgeId = callback_getNextEdgeId;
    callbacks->insertEdges = callback_insertEdges;
    callbacks->updateEdges = callback_updateEdges;
    callbacks->getFaceById = callback_getFaceById;
    callbacks->getFaceContainingPoint = callback_getFaceContainingPoint;
    callbacks->deleteEdges = callback_deleteEdges;
    callbacks->getNodeWithinBox2D = callback_getNodeWithinBox2D;
    callbacks->getEdgeWithinBox2D = callback_getEdgeWithinBox2D;
    callbacks->getEdgeByNode = callback_getEdgeByNode;
    callbacks->updateNodes = callback_updateNodes;
    callbacks->insertFaces = callback_insertFaces;
    callbacks->updateFacesById = callback_updateFacesById;
    callbacks->deleteFacesById = callback_deleteFacesById;
    callbacks->getRingEdges = callback_getRingEdges;
    callbacks->updateEdgesById = callback_updateEdgesById;
    callbacks->getEdgeByFace = callback_getEdgeByFace;
    callbacks->getNodeByFace = callback_getNodeByFace;
    callbacks->updateNodesById = callback_updateNodesById;
    callbacks->deleteNodesById = callback_deleteNodesById;
    callbacks->updateTopoGeomEdgeSplit = callback_updateTopoGeomEdgeSplit;
    callbacks->updateTopoGeomFaceSplit = callback_updateTopoGeomFaceSplit;
    callbacks->checkTopoGeomRemEdge = callback_checkTopoGeomRemEdge;
    callbacks->updateTopoGeomFaceHeal = callback_updateTopoGeomFaceHeal;
    callbacks->checkTopoGeomRemNode = callback_checkTopoGeomRemNode;
    callbacks->updateTopoGeomEdgeHeal = callback_updateTopoGeomEdgeHeal;
    callbacks->getFaceWithinBox2D = callback_getFaceWithinBox2D;
    ptr->callbacks = callbacks;

    rtt_BackendIfaceRegisterCallbacks (ptr->rtt_iface, callbacks);
    ptr->rtt_topology = rtt_LoadTopology (ptr->rtt_iface, topo_name);

    ptr->stmt_getNodeWithinDistance2D = NULL;
    ptr->stmt_insertNodes = NULL;
    ptr->stmt_getEdgeWithinDistance2D = NULL;
    ptr->stmt_getNextEdgeId = NULL;
    ptr->stmt_setNextEdgeId = NULL;
    ptr->stmt_insertEdges = NULL;
    ptr->stmt_getFaceContainingPoint_1 = NULL;
    ptr->stmt_getFaceContainingPoint_2 = NULL;
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = NULL;
    ptr->stmt_getEdgeWithinBox2D = NULL;
    ptr->stmt_getFaceWithinBox2D = NULL;
    ptr->stmt_getAllEdges = NULL;
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = NULL;
    ptr->stmt_updateFacesById = NULL;
    ptr->stmt_deleteFacesById = NULL;
    ptr->stmt_deleteNodesById = NULL;
    ptr->stmt_getRingEdges = NULL;
    if (ptr->rtt_topology == NULL)
      {
	  char *msg =
	      sqlite3_mprintf ("Topology \"%s\" is undefined !!!", topo_name);
	  gaiaSetRtTopoErrorMsg (p_cache, msg);
	  sqlite3_free (msg);
	  goto invalid;
      }

/* creating the SQL prepared statements */
    create_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) ptr);

    return (GaiaTopologyAccessorPtr) ptr;

  invalid:
    gaiaTopologyDestroy ((GaiaTopologyAccessorPtr) ptr);
    return NULL;
}

GAIATOPO_DECLARE void
gaiaTopologyDestroy (GaiaTopologyAccessorPtr topo_ptr)
{
/* destroying a Topology Accessor Object */
    struct gaia_topology *prev;
    struct gaia_topology *next;
    struct splite_internal_cache *cache;
    struct gaia_topology *ptr = (struct gaia_topology *) topo_ptr;
    if (ptr == NULL)
	return;

    prev = ptr->prev;
    next = ptr->next;
    cache = (struct splite_internal_cache *) (ptr->cache);
    if (ptr->rtt_topology != NULL)
	rtt_FreeTopology ((RTT_TOPOLOGY *) (ptr->rtt_topology));
    if (ptr->rtt_iface != NULL)
	rtt_FreeBackendIface ((RTT_BE_IFACE *) (ptr->rtt_iface));
    if (ptr->callbacks != NULL)
	free (ptr->callbacks);
    if (ptr->topology_name != NULL)
	free (ptr->topology_name);
    if (ptr->last_error_message != NULL)
	free (ptr->last_error_message);

    finalize_topogeo_prepared_stmts (topo_ptr);
    free (ptr);

/* unregistering from the Internal Cache double linked list */
    if (prev != NULL)
	prev->next = next;
    if (next != NULL)
	next->prev = prev;
    if (cache->firstTopology == ptr)
	cache->firstTopology = next;
    if (cache->lastTopology == ptr)
	cache->lastTopology = prev;
}

TOPOLOGY_PRIVATE void
finalize_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor)
{
/* finalizing the SQL prepared statements */
    struct gaia_topology *ptr = (struct gaia_topology *) accessor;
    if (ptr->stmt_getNodeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinDistance2D);
    if (ptr->stmt_insertNodes != NULL)
	sqlite3_finalize (ptr->stmt_insertNodes);
    if (ptr->stmt_getEdgeWithinDistance2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinDistance2D);
    if (ptr->stmt_getNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_getNextEdgeId);
    if (ptr->stmt_setNextEdgeId != NULL)
	sqlite3_finalize (ptr->stmt_setNextEdgeId);
    if (ptr->stmt_insertEdges != NULL)
	sqlite3_finalize (ptr->stmt_insertEdges);
    if (ptr->stmt_getFaceContainingPoint_1 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_1);
    if (ptr->stmt_getFaceContainingPoint_2 != NULL)
	sqlite3_finalize (ptr->stmt_getFaceContainingPoint_2);
    if (ptr->stmt_deleteEdges != NULL)
	sqlite3_finalize (ptr->stmt_deleteEdges);
    if (ptr->stmt_getNodeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getNodeWithinBox2D);
    if (ptr->stmt_getEdgeWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getEdgeWithinBox2D);
    if (ptr->stmt_getFaceWithinBox2D != NULL)
	sqlite3_finalize (ptr->stmt_getFaceWithinBox2D);
    if (ptr->stmt_getAllEdges != NULL)
	sqlite3_finalize (ptr->stmt_getAllEdges);
    if (ptr->stmt_updateNodes != NULL)
	sqlite3_finalize (ptr->stmt_updateNodes);
    if (ptr->stmt_insertFaces != NULL)
	sqlite3_finalize (ptr->stmt_insertFaces);
    if (ptr->stmt_updateFacesById != NULL)
	sqlite3_finalize (ptr->stmt_updateFacesById);
    if (ptr->stmt_deleteFacesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteFacesById);
    if (ptr->stmt_deleteNodesById != NULL)
	sqlite3_finalize (ptr->stmt_deleteNodesById);
    if (ptr->stmt_getRingEdges != NULL)
	sqlite3_finalize (ptr->stmt_getRingEdges);
    ptr->stmt_getNodeWithinDistance2D = NULL;
    ptr->stmt_insertNodes = NULL;
    ptr->stmt_getEdgeWithinDistance2D = NULL;
    ptr->stmt_getNextEdgeId = NULL;
    ptr->stmt_setNextEdgeId = NULL;
    ptr->stmt_insertEdges = NULL;
    ptr->stmt_getFaceContainingPoint_1 = NULL;
    ptr->stmt_getFaceContainingPoint_2 = NULL;
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = NULL;
    ptr->stmt_getEdgeWithinBox2D = NULL;
    ptr->stmt_getFaceWithinBox2D = NULL;
    ptr->stmt_getAllEdges = NULL;
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = NULL;
    ptr->stmt_updateFacesById = NULL;
    ptr->stmt_deleteFacesById = NULL;
    ptr->stmt_deleteNodesById = NULL;
    ptr->stmt_getRingEdges = NULL;
}

TOPOLOGY_PRIVATE void
create_topogeo_prepared_stmts (GaiaTopologyAccessorPtr accessor)
{
/* creating the SQL prepared statements */
    struct gaia_topology *ptr = (struct gaia_topology *) accessor;
    finalize_topogeo_prepared_stmts (accessor);
    ptr->stmt_getNodeWithinDistance2D =
	do_create_stmt_getNodeWithinDistance2D (accessor);
    ptr->stmt_insertNodes = do_create_stmt_insertNodes (accessor);
    ptr->stmt_getEdgeWithinDistance2D =
	do_create_stmt_getEdgeWithinDistance2D (accessor);
    ptr->stmt_getNextEdgeId = do_create_stmt_getNextEdgeId (accessor);
    ptr->stmt_setNextEdgeId = do_create_stmt_setNextEdgeId (accessor);
    ptr->stmt_insertEdges = do_create_stmt_insertEdges (accessor);
    ptr->stmt_getFaceContainingPoint_1 =
	do_create_stmt_getFaceContainingPoint_1 (accessor);
    ptr->stmt_getFaceContainingPoint_2 =
	do_create_stmt_getFaceContainingPoint_2 (accessor);
    ptr->stmt_deleteEdges = NULL;
    ptr->stmt_getNodeWithinBox2D = do_create_stmt_getNodeWithinBox2D (accessor);
    ptr->stmt_getEdgeWithinBox2D = do_create_stmt_getEdgeWithinBox2D (accessor);
    ptr->stmt_getFaceWithinBox2D = do_create_stmt_getFaceWithinBox2D (accessor);
    ptr->stmt_getAllEdges = do_create_stmt_getAllEdges (accessor);
    ptr->stmt_updateNodes = NULL;
    ptr->stmt_insertFaces = do_create_stmt_insertFaces (accessor);
    ptr->stmt_updateFacesById = do_create_stmt_updateFacesById (accessor);
    ptr->stmt_deleteFacesById = do_create_stmt_deleteFacesById (accessor);
    ptr->stmt_deleteNodesById = do_create_stmt_deleteNodesById (accessor);
    ptr->stmt_getRingEdges = do_create_stmt_getRingEdges (accessor);
}

TOPOLOGY_PRIVATE void
finalize_all_topo_prepared_stmts (const void *p_cache)
{
/* finalizing all Topology-related prepared Stms */
    struct gaia_topology *p_topo;
    struct gaia_network *p_network;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    p_topo = (struct gaia_topology *) cache->firstTopology;
    while (p_topo != NULL)
      {
	  finalize_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo->next;
      }

    p_network = (struct gaia_network *) cache->firstNetwork;
    while (p_network != NULL)
      {
	  finalize_toponet_prepared_stmts ((GaiaNetworkAccessorPtr) p_network);
	  p_network = p_network->next;
      }
}

TOPOLOGY_PRIVATE void
create_all_topo_prepared_stmts (const void *p_cache)
{
/* (re)creating all Topology-related prepared Stms */
    struct gaia_topology *p_topo;
    struct gaia_network *p_network;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) p_cache;
    if (cache == NULL)
	return;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return;

    p_topo = (struct gaia_topology *) cache->firstTopology;
    while (p_topo != NULL)
      {
	  create_topogeo_prepared_stmts ((GaiaTopologyAccessorPtr) p_topo);
	  p_topo = p_topo->next;
      }

    p_network = (struct gaia_network *) cache->firstNetwork;
    while (p_network != NULL)
      {
	  create_toponet_prepared_stmts ((GaiaNetworkAccessorPtr) p_network);
	  p_network = p_network->next;
      }
}

TOPOLOGY_PRIVATE void
gaiatopo_reset_last_error_msg (GaiaTopologyAccessorPtr accessor)
{
/* resets the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return;

    if (topo->cache != NULL)
	gaiaResetRtTopoMsg (topo->cache);
    if (topo->last_error_message != NULL)
	free (topo->last_error_message);
    topo->last_error_message = NULL;
}

TOPOLOGY_PRIVATE void
gaiatopo_set_last_error_msg (GaiaTopologyAccessorPtr accessor, const char *msg)
{
/* sets the last Topology error message */
    int len;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (msg == NULL)
	msg = "no message available";

    spatialite_e ("%s\n", msg);
    if (topo == NULL)
	return;

    if (topo->last_error_message != NULL)
	return;

    len = strlen (msg);
    topo->last_error_message = malloc (len + 1);
    strcpy (topo->last_error_message, msg);
}

TOPOLOGY_PRIVATE const char *
gaiatopo_get_last_exception (GaiaTopologyAccessorPtr accessor)
{
/* returns the last Topology error message */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return NULL;

    return topo->last_error_message;
}

GAIATOPO_DECLARE int
gaiaTopologyDrop (sqlite3 * handle, const char *topo_name)
{
/* attempting to drop an already existing Topology */
    int ret;
    char *sql;

/* creating the Topologies table (just in case) */
    if (!do_create_topologies (handle))
	return 0;

/* testing for existing DBMS objects */
    if (!check_existing_topology (handle, topo_name, 0))
	return 0;

/* dropping all topofeature tables (if any) */
    if (!do_drop_topofeature_tables (handle, topo_name))
	goto error;

/* dropping the Topology own Tables */
    if (!do_drop_topo_view (handle, topo_name, "edge_seeds"))
	goto error;
    if (!do_drop_topo_view (handle, topo_name, "face_seeds"))
	goto error;
    if (!do_drop_topo_view (handle, topo_name, "face_geoms"))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "topofeatures", 0))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "topolayers", 0))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "seeds", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "edge", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "node", 1))
	goto error;
    if (!do_drop_topo_table (handle, topo_name, "face", 1))
	goto error;

/* unregistering the Topology */
    sql =
	sqlite3_mprintf
	("DELETE FROM MAIN.topologies WHERE Lower(topology_name) = Lower(%Q)",
	 topo_name);
    ret = sqlite3_exec (handle, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    return 1;

  error:
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoNode (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 face, gaiaPointPtr pt, int skip_checks)
{
/* RTT wrapper - AddIsoNode */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_AddIsoNode ((RTT_TOPOLOGY *) (topo->rtt_topology), face, rt_pt,
			skip_checks);

    rtpoint_free (ctx, rt_pt);
    return ret;
}

GAIATOPO_DECLARE int
gaiaMoveIsoNode (GaiaTopologyAccessorPtr accessor,
		 sqlite3_int64 node, gaiaPointPtr pt)
{
/* RTT wrapper - MoveIsoNode */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    int ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    gaiaResetRtTopoMsg (cache);
    ret = rtt_MoveIsoNode ((RTT_TOPOLOGY *) (topo->rtt_topology), node, rt_pt);

    rtpoint_free (ctx, rt_pt);
    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE int
gaiaRemIsoNode (GaiaTopologyAccessorPtr accessor, sqlite3_int64 node)
{
/* RTT wrapper - RemIsoNode */
    struct splite_internal_cache *cache = NULL;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret = rtt_RemoveIsoNode ((RTT_TOPOLOGY *) (topo->rtt_topology), node);

    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddIsoEdge (GaiaTopologyAccessorPtr accessor,
		sqlite3_int64 start_node, sqlite3_int64 end_node,
		gaiaLinestringPtr ln)
{
/* RTT wrapper - AddIsoEdge */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_AddIsoEdge ((RTT_TOPOLOGY *) (topo->rtt_topology), start_node,
			end_node, rt_line);

    rtline_free (ctx, rt_line);
    return ret;
}

GAIATOPO_DECLARE int
gaiaRemIsoEdge (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge)
{
/* RTT wrapper - RemIsoEdge */
    struct splite_internal_cache *cache = NULL;
    int ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret = rtt_RemIsoEdge ((RTT_TOPOLOGY *) (topo->rtt_topology), edge);

    if (ret == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE int
gaiaChangeEdgeGeom (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 edge_id, gaiaLinestringPtr ln)
{
/* RTT wrapper - ChangeEdgeGeom  */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    int ret;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_ChangeEdgeGeom ((RTT_TOPOLOGY *) (topo->rtt_topology), edge_id,
			    rt_line);

    rtline_free (ctx, rt_line);
    if (ret == 0)
	return 1;
    return 0;
}

static void
do_check_mod_split_edge3d (struct gaia_topology *topo, gaiaPointPtr pt,
			   sqlite3_int64 old_edge)
{
/*
/ defensive programming: carefully ensuring that lwgeom could
/ never shift Edges' start/end points after computing ModSplit
/ 3D topology
*/
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    double x1s;
    double y1s;
    double z1s;
    double x1e;
    double y1e;
    double z1e;
    double x2s;
    double y2s;
    double z2s;
    double x2e;
    double y2e;
    double z2e;
    sqlite3_int64 new_edge = sqlite3_last_insert_rowid (topo->db_handle);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("SELECT ST_X(ST_StartPoint(geom)), ST_Y(ST_StartPoint(geom)), "
	 "ST_Z(ST_StartPoint(geom)), ST_X(ST_EndPoint(geom)), ST_Y(ST_EndPoint(geom)), "
	 "ST_Z(ST_EndPoint(geom)) FROM \"%s\" WHERE edge_id = ?", xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, old_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x1s = sqlite3_column_double (stmt, 0);
		y1s = sqlite3_column_double (stmt, 1);
		z1s = sqlite3_column_double (stmt, 2);
		x1e = sqlite3_column_double (stmt, 3);
		y1e = sqlite3_column_double (stmt, 4);
		z1e = sqlite3_column_double (stmt, 5);
	    }
	  else
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, new_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x2s = sqlite3_column_double (stmt, 0);
		y2s = sqlite3_column_double (stmt, 1);
		z2s = sqlite3_column_double (stmt, 2);
		x2e = sqlite3_column_double (stmt, 3);
		y2e = sqlite3_column_double (stmt, 4);
		z2e = sqlite3_column_double (stmt, 5);
	    }
	  else
	      goto error;
      }
    if (x1s == x2e && y1s == y2e && z1s == z2e)
      {
	  /* just silencing stupid compiler warnings */
	  ;
      }
    if (x1e == x2s && y1e == y2s && z1e == z2s)
      {
	  if (x1e != pt->X || y1e != pt->Y || z1e != pt->Z)
	      goto fixme;
      }
  error:
    sqlite3_finalize (stmt);
    return;

  fixme:
    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetEndPoint(geom, MakePointZ(?, ?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_double (stmt, 3, pt->Z);
    sqlite3_bind_int64 (stmt, 4, old_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetStartPoint(geom, MakePointZ(?, ?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_double (stmt, 3, pt->Z);
    sqlite3_bind_int64 (stmt, 4, new_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

  error2:
    sqlite3_finalize (stmt);
    return;
}

static void
do_check_mod_split_edge (struct gaia_topology *topo, gaiaPointPtr pt,
			 sqlite3_int64 old_edge)
{
/*
/ defensive programming: carefully ensuring that lwgeom could
/ never shift Edges' start/end points after computing ModSplit
/ 2D topology
*/
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *table;
    char *xtable;
    double x1s;
    double y1s;
    double x1e;
    double y1e;
    double x2s;
    double y2s;
    double x2e;
    double y2e;
    sqlite3_int64 new_edge;
    if (topo->has_z)
      {
	  do_check_mod_split_edge3d (topo, pt, old_edge);
	  return;
      }

    new_edge = sqlite3_last_insert_rowid (topo->db_handle);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("SELECT ST_X(ST_StartPoint(geom)), ST_Y(ST_StartPoint(geom)), "
	 "ST_X(ST_EndPoint(geom)), ST_Y(ST_EndPoint(geom)) FROM \"%s\" WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, old_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x1s = sqlite3_column_double (stmt, 0);
		y1s = sqlite3_column_double (stmt, 1);
		x1e = sqlite3_column_double (stmt, 2);
		y1e = sqlite3_column_double (stmt, 3);
	    }
	  else
	      goto error;
      }

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, new_edge);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		x2s = sqlite3_column_double (stmt, 0);
		y2s = sqlite3_column_double (stmt, 1);
		x2e = sqlite3_column_double (stmt, 2);
		y2e = sqlite3_column_double (stmt, 3);
	    }
	  else
	      goto error;
      }
    if (x1s == x2e && y1s == y2e)
      {
	  /* just silencing stupid compiler warnings */
	  ;
      }
    if (x1e == x2s && y1e == y2s)
      {
	  if (x1e != pt->X || y1e != pt->Y)
	      goto fixme;
      }
  error:
    sqlite3_finalize (stmt);
    return;

  fixme:
    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetEndPoint(geom, MakePoint(?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_int64 (stmt, 3, old_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

    sqlite3_finalize (stmt);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET geom = ST_SetStartPoint(geom, MakePoint(?, ?)) WHERE edge_id = ?",
	 xtable);
    free (xtable);
    sqlite3_free (table);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->X);
    sqlite3_bind_double (stmt, 2, pt->Y);
    sqlite3_bind_int64 (stmt, 3, new_edge);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	goto error2;

  error2:
    sqlite3_finalize (stmt);
    return;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeSplit (GaiaTopologyAccessorPtr accessor,
		  sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* RTT wrapper - ModEdgeSplit */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_ModEdgeSplit ((RTT_TOPOLOGY *) (topo->rtt_topology), edge, rt_pt,
			  skip_checks);

    rtpoint_free (ctx, rt_pt);

    if (ret > 0)
	do_check_mod_split_edge (topo, pt, edge);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgesSplit (GaiaTopologyAccessorPtr accessor,
		   sqlite3_int64 edge, gaiaPointPtr pt, int skip_checks)
{
/* RTT wrapper - NewEdgesSplit */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_NewEdgesSplit ((RTT_TOPOLOGY *) (topo->rtt_topology), edge, rt_pt,
			   skip_checks);

    rtpoint_free (ctx, rt_pt);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeModFace (GaiaTopologyAccessorPtr accessor,
		    sqlite3_int64 start_node, sqlite3_int64 end_node,
		    gaiaLinestringPtr ln, int skip_checks)
{
/* RTT wrapper - AddEdgeModFace */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_AddEdgeModFace ((RTT_TOPOLOGY *) (topo->rtt_topology), start_node,
			    end_node, rt_line, skip_checks);

    rtline_free (ctx, rt_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaAddEdgeNewFaces (GaiaTopologyAccessorPtr accessor,
		     sqlite3_int64 start_node, sqlite3_int64 end_node,
		     gaiaLinestringPtr ln, int skip_checks)
{
/* RTT wrapper - AddEdgeNewFaces */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_AddEdgeNewFaces ((RTT_TOPOLOGY *) (topo->rtt_topology), start_node,
			     end_node, rt_line, skip_checks);

    rtline_free (ctx, rt_line);
    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeNewFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* RTT wrapper - RemEdgeNewFace */
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret = rtt_RemEdgeNewFace ((RTT_TOPOLOGY *) (topo->rtt_topology), edge_id);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaRemEdgeModFace (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id)
{
/* RTT wrapper - RemEdgeModFace */
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret = rtt_RemEdgeModFace ((RTT_TOPOLOGY *) (topo->rtt_topology), edge_id);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaNewEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* RTT wrapper - NewEdgeHeal */
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_NewEdgeHeal ((RTT_TOPOLOGY *) (topo->rtt_topology), edge_id1,
			 edge_id2);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaModEdgeHeal (GaiaTopologyAccessorPtr accessor, sqlite3_int64 edge_id1,
		 sqlite3_int64 edge_id2)
{
/* RTT wrapper - ModEdgeHeal */
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_ModEdgeHeal ((RTT_TOPOLOGY *) (topo->rtt_topology), edge_id1,
			 edge_id2);

    return ret;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaGetFaceGeometry (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* RTT wrapper - GetFaceGeometry */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    RTGEOM *result = NULL;
    RTPOLY *rtpoly;
    int has_z = 0;
    RTPOINTARRAY *pa;
    RTPOINT4D pt4d;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    gaiaGeomCollPtr geom;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int dimension_model;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    result = rtt_GetFaceGeometry ((RTT_TOPOLOGY *) (topo->rtt_topology), face);
    if (result == NULL)
	return NULL;

/* converting the result as a Gaia Geometry */
    rtpoly = (RTPOLY *) result;
    if (rtpoly->nrings <= 0)
      {
	  /* empty geometry */
	  rtgeom_free (ctx, result);
	  return NULL;
      }
    pa = rtpoly->rings[0];
    if (pa->npoints <= 0)
      {
	  /* empty geometry */
	  rtgeom_free (ctx, result);
	  return NULL;
      }
    if (RTFLAGS_GET_Z (pa->flags))
	has_z = 1;
    if (has_z)
      {
	  dimension_model = GAIA_XY_Z;
	  geom = gaiaAllocGeomCollXYZ ();
      }
    else
      {
	  dimension_model = GAIA_XY;
	  geom = gaiaAllocGeomColl ();
      }
    pg = gaiaAddPolygonToGeomColl (geom, pa->npoints, rtpoly->nrings - 1);
    rng = pg->Exterior;
    for (iv = 0; iv < pa->npoints; iv++)
      {
	  /* copying Exterior Ring vertices */
	  rt_getPoint4d_p (ctx, pa, iv, &pt4d);
	  x = pt4d.x;
	  y = pt4d.y;
	  if (has_z)
	      z = pt4d.z;
	  else
	      z = 0.0;
	  if (dimension_model == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (rng->Coords, iv, x, y);
	    }
      }
    for (ib = 1; ib < rtpoly->nrings; ib++)
      {
	  has_z = 0;
	  pa = rtpoly->rings[ib];
	  if (RTFLAGS_GET_Z (pa->flags))
	      has_z = 1;
	  rng = gaiaAddInteriorRing (pg, ib - 1, pa->npoints);
	  for (iv = 0; iv < pa->npoints; iv++)
	    {
		/* copying Exterior Ring vertices */
		rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		x = pt4d.x;
		y = pt4d.y;
		if (has_z)
		    z = pt4d.z;
		else
		    z = 0.0;
		if (dimension_model == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (rng->Coords, iv, x, y, z);
		  }
		else
		  {
		      gaiaSetPoint (rng->Coords, iv, x, y);
		  }
	    }
      }
    rtgeom_free (ctx, result);
    geom->DeclaredType = GAIA_POLYGON;
    geom->Srid = topo->srid;
    return geom;
}

static int
do_check_create_faceedges_table (GaiaTopologyAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int exists = 0;
    int ok_face_id = 0;
    int ok_sequence = 0;
    int ok_edge_id = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* testing for an already existing table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("PRAGMA TEMP.table_info(\"%s\")", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *type = results[(i * columns) + 2];
	  const char *notnull = results[(i * columns) + 3];
	  const char *dflt_value = results[(i * columns) + 4];
	  const char *pk = results[(i * columns) + 5];
	  if (strcmp (name, "face_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "1") == 0)
	      ok_face_id = 1;
	  if (strcmp (name, "sequence") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "2") == 0)
	      ok_sequence = 1;
	  if (strcmp (name, "edge_id") == 0 && strcmp (type, "INTEGER") == 0
	      && strcmp (notnull, "1") == 0 && dflt_value == NULL
	      && strcmp (pk, "0") == 0)
	      ok_edge_id = 1;
	  exists = 1;
      }
    sqlite3_free_table (results);
    if (ok_face_id && ok_sequence && ok_edge_id)
	return 1;		/* already existing and valid */

    if (exists)
	return 0;		/* already existing but invalid */

/* attempting to create the table */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\tface_id INTEGER NOT NULL,\n"
	 "\tsequence INTEGER NOT NULL,\n\tedge_id INTEGER NOT NULL,\n"
	 "\tCONSTRAINT pk_topo_facee_edges PRIMARY KEY (face_id, sequence))",
	 xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_populate_faceedges_table (GaiaTopologyAccessorPtr accessor,
			     sqlite3_int64 face, RTT_ELEMID * edges,
			     int num_edges)
{
/* populating the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    sqlite3_stmt *stmt = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* deleting all rows belonging to Face (if any) */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM TEMP.\"%s\" WHERE face_id = ?", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, face);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

/* preparing the INSERT statement */
    table = sqlite3_mprintf ("%s_face_edges_temp", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (face_id, sequence, edge_id) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    for (i = 0; i < num_edges; i++)
      {
	  /* inserting all Face/Edges */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, face);
	  sqlite3_bind_int (stmt, 2, i + 1);
	  sqlite3_bind_int64 (stmt, 3, *(edges + i));
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg = sqlite3_mprintf ("ST_GetFaceEdges exception: %s",
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
gaiaGetFaceEdges (GaiaTopologyAccessorPtr accessor, sqlite3_int64 face)
{
/* RTT wrapper - GetFaceEdges */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    RTT_ELEMID *edges = NULL;
    int num_edges;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    gaiaResetRtTopoMsg (cache);

    num_edges =
	rtt_GetFaceEdges ((RTT_TOPOLOGY *) (topo->rtt_topology), face, &edges);

    if (num_edges < 0)
	return 0;

    if (num_edges > 0)
      {
	  /* attemtping to create or validate the target table */
	  if (!do_check_create_faceedges_table (accessor))
	    {
		rtfree (ctx, edges);
		return 0;
	    }

	  /* populating the target table */
	  if (!do_populate_faceedges_table (accessor, face, edges, num_edges))
	    {
		rtfree (ctx, edges);
		return 0;
	    }
      }
    rtfree (ctx, edges);
    return 1;
}

static int
do_check_create_validate_topogeo_table (GaiaTopologyAccessorPtr accessor)
{
/* attemtping to create or validate the target table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *errMsg = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* attempting to drop the table (just in case if it already exists) */
    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE IF EXISTS temp.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidSpatialNet exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }
/* attempting to create the table */
    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("CREATE TEMP TABLE \"%s\" (\n\terror TEXT,\n"
	 "\tprimitive1 INTEGER,\n\tprimitive2 INTEGER)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo exception: %s", errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_topo_check_coincident_nodes (GaiaTopologyAccessorPtr accessor,
				sqlite3_stmt * stmt)
{
/* checking for coincident nodes */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT n1.node_id, n2.node_id FROM MAIN.\"%s\" AS n1 "
			 "JOIN MAIN.\"%s\" AS n2 ON (n1.node_id <> n2.node_id AND "
			 "ST_Equals(n1.geom, n2.geom) = 1 AND n2.node_id IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = n1.geom))",
			 xtable, xtable, table);
    sqlite3_free (table);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - CoicidentNodes error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 node_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "coincident nodes", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id1);
		sqlite3_bind_int64 (stmt, 3, node_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #1 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - CoicidentNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_edge_node (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edge-node crossing */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("SELECT e.edge_id, n.node_id FROM MAIN.\"%s\" AS e "
			   "JOIN MAIN.\"%s\" AS n ON (ST_Distance(e.geom, n.geom) <= 0 "
			   "AND ST_Disjoint(ST_StartPoint(e.geom), n.geom) = 1 AND "
			   "ST_Disjoint(ST_EndPoint(e.geom), n.geom) = 1 AND n.node_id IN "
			   "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			   "f_geometry_column = 'geom' AND search_frame = e.geom))",
			   xtable1, xtable2, table);
    sqlite3_free (table);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - EdgeCrossedNode error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge crosses node", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, node_id);
		sqlite3_bind_int64 (stmt, 3, edge_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #2 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EdgeCrossedNode step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_non_simple (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for non-simple edges */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT edge_id FROM MAIN.\"%s\" WHERE ST_IsSimple(geom) = 0", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - NonSimpleEdge error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge not simple", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #3 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - NonSimpleEdge step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_edge_edge (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edge-edge crossing */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
/*
    sql =
	sqlite3_mprintf ("SELECT e1.edge_id, e2.edge_id FROM MAIN.\"%s\" AS e1 "
			 "JOIN MAIN.\"%s\" AS e2 ON (e1.edge_id <> e2.edge_id AND "
			 "ST_Crosses(e1.geom, e2.geom) = 1 AND e2.edge_id IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = e1.geom))",
			 xtable, xtable, table);
*/
    sql =
	sqlite3_mprintf ("SELECT e1.edge_id, e2.edge_id FROM MAIN.\"%s\" AS e1 "
			 "JOIN MAIN.\"%s\" AS e2 ON (e1.edge_id <> e2.edge_id AND "
			 "ST_RelateMatch(ST_Relate(e1.geom, e2.geom), '0******0*') = 1 AND e2.edge_id IN "
			 "(SELECT rowid FROM SpatialIndex WHERE f_table_name = %Q AND "
			 "f_geometry_column = 'geom' AND search_frame = e1.geom))",
			 xtable, xtable, table);
    sqlite3_free (table);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - EdgeCrossesEdge error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 edge_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 edge_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "edge crosses edge", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id1);
		sqlite3_bind_int64 (stmt, 3, edge_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #4 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EdgeCrossesEdge step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_start_nodes (GaiaTopologyAccessorPtr accessor,
			   sqlite3_stmt * stmt)
{
/* checking for edges mismatching start nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT e.edge_id, e.start_node FROM MAIN.\"%s\" AS e "
			 "JOIN MAIN.\"%s\" AS n ON (e.start_node = n.node_id) "
			 "WHERE ST_Disjoint(ST_StartPoint(e.geom), n.geom) = 1",
			 xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - StartNodes error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry start mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #5 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - StartNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_end_nodes (GaiaTopologyAccessorPtr accessor, sqlite3_stmt * stmt)
{
/* checking for edges mismatching end nodes */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT e.edge_id, e.end_node FROM MAIN.\"%s\" AS e "
			   "JOIN MAIN.\"%s\" AS n ON (e.end_node = n.node_id) "
			   "WHERE ST_Disjoint(ST_EndPoint(e.geom), n.geom) = 1",
			   xtable1, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo() - EndNodes error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 edge_id = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "geometry end mismatch", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, edge_id);
		sqlite3_bind_int64 (stmt, 3, node_id);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #6 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - EndNodes step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_face_no_edges (GaiaTopologyAccessorPtr accessor,
			     sqlite3_stmt * stmt)
{
/* checking for faces with no edges */
    char *sql;
    char *table;
    char *xtable1;
    char *xtable2;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT f.face_id, Count(e1.edge_id) AS cnt1, "
			   "Count(e2.edge_id) AS cnt2 FROM MAIN.\"%s\" AS f "
			   "LEFT JOIN MAIN.\"%s\" AS e1 ON (f.face_id = e1.left_face) "
			   "LEFT JOIN MAIN.\"%s\" AS e2 ON (f.face_id = e2.right_face) "
			   "GROUP BY f.face_id HAVING cnt1 = 0 AND cnt2 = 0",
			   xtable1, xtable2, xtable2);
    free (xtable1);
    free (xtable2);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - FaceNoEdges error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_in, 0);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face without edges", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id);
		sqlite3_bind_null (stmt, 3);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #7 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - FaceNoEdges step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_no_universal_face (GaiaTopologyAccessorPtr accessor,
				 sqlite3_stmt * stmt)
{
/* checking for missing universal face */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\" WHERE face_id = 0",
			 xtable);
    free (xtable);
    ret =
	sqlite3_get_table (topo->db_handle, sql, &results, &rows, &columns,
			   &errMsg);
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

    if (count <= 0)
      {
	  /* reporting the error */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, "no universal face", -1, SQLITE_STATIC);
	  sqlite3_bind_null (stmt, 2);
	  sqlite3_bind_null (stmt, 3);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() insert #8 error: \"%s\"",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_free (msg);
		return 0;
	    }
      }

    return 1;
}

static int
do_topo_check_create_aux_faces (GaiaTopologyAccessorPtr accessor)
{
/* creating the aux-Face temp table */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* creating the aux-face Temp Table */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE \"%s\" (\n"
			   "\tface_id INTEGER PRIMARY KEY,\n\tgeom BLOB)",
			   xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("CREATE TEMPORARY TABLE aux_face - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* creating the exotic spatial index */
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("CREATE VIRTUAL TABLE temp.\"%s\" USING RTree "
			   "(id_face, x_min, x_max, y_min, y_max)", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("CREATE TEMPORARY TABLE aux_face - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

static int
do_topo_check_build_aux_faces (GaiaTopologyAccessorPtr accessor,
			       sqlite3_stmt * stmt)
{
/* populating the aux-face Temp Table */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    sqlite3_stmt *stmt_rtree = NULL;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* preparing the input SQL statement */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT face_id, ST_GetFaceGeometry(%Q, face_id) "
			   "FROM MAIN.\"%s\" WHERE face_id <> 0",
			   topo->topology_name, xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - GetFaceGeometry error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (face_id, geom) VALUES (?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_ValidateTopoGeo() - AuxFace error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* preparing the RTree SQL statement */
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" "
			   "(id_face, x_min, x_max, y_min, y_max) VALUES (?, ?, ?, ?, ?)",
			   xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_rtree,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - AuxFaceRTree error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		gaiaGeomCollPtr geom = NULL;
		const unsigned char *blob;
		int blob_sz;
		sqlite3_int64 face_id = sqlite3_column_int64 (stmt_in, 0);
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, 1);
		      blob_sz = sqlite3_column_bytes (stmt_in, 1);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		  }
		if (geom == NULL)
		  {
		      /* reporting the error */
		      sqlite3_reset (stmt);
		      sqlite3_clear_bindings (stmt);
		      sqlite3_bind_text (stmt, 1, "invalid face geometry", -1,
					 SQLITE_STATIC);
		      sqlite3_bind_int64 (stmt, 2, face_id);
		      sqlite3_bind_null (stmt, 3);
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #9 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		else
		  {
		      double xmin = geom->MinX;
		      double xmax = geom->MaxX;
		      double ymin = geom->MinY;
		      double ymax = geom->MaxY;
		      gaiaFreeGeomColl (geom);
		      /* inserting into AuxFace */
		      sqlite3_reset (stmt_out);
		      sqlite3_clear_bindings (stmt_out);
		      sqlite3_bind_int64 (stmt_out, 1, face_id);
		      sqlite3_bind_blob (stmt_out, 2, blob, blob_sz,
					 SQLITE_STATIC);
		      ret = sqlite3_step (stmt_out);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #10 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		      /* updating the AuxFaceRTree */
		      sqlite3_reset (stmt_rtree);
		      sqlite3_clear_bindings (stmt_rtree);
		      sqlite3_bind_int64 (stmt_rtree, 1, face_id);
		      sqlite3_bind_double (stmt_rtree, 2, xmin);
		      sqlite3_bind_double (stmt_rtree, 3, xmax);
		      sqlite3_bind_double (stmt_rtree, 4, ymin);
		      sqlite3_bind_double (stmt_rtree, 5, ymax);
		      ret = sqlite3_step (stmt_rtree);
		      if (ret == SQLITE_DONE || ret == SQLITE_ROW)
			  ;
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("ST_ValidateTopoGeo() insert #11 error: \"%s\"",
				 sqlite3_errmsg (topo->db_handle));
			    gaiatopo_set_last_error_msg (accessor, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - GetFaceGeometry step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    sqlite3_finalize (stmt_rtree);

    return 1;

  error:
    if (stmt_in == NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out == NULL)
	sqlite3_finalize (stmt_out);
    if (stmt_rtree == NULL)
	sqlite3_finalize (stmt_rtree);
    return 0;
}

static int
do_topo_check_overlapping_faces (GaiaTopologyAccessorPtr accessor,
				 sqlite3_stmt * stmt)
{
/* checking for overlapping faces */
    char *sql;
    char *table;
    char *xtable;
    char *rtree;
    int ret;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;


#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name, pid);
    rtree = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT a.face_id, b.face_id FROM TEMP.\"%s\" AS a, TEMP.\"%s\" AS b "
	 "WHERE a.geom IS NOT NULL AND a.face_id <> b.face_id AND ST_Overlaps(a.geom, b.geom) = 1 "
	 "AND b.face_id IN (SELECT id_face FROM TEMP.\"%s\" WHERE x_min <= MbrMaxX(a.geom) "
	 "AND x_max >= MbrMinX(a.geom) AND y_min <= MbrMaxY(a.geom) AND y_max >= MbrMinY(a.geom))",
	 xtable, xtable, rtree);
    free (xtable);
    free (rtree);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - OverlappingFaces error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 face_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 face_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face overlaps face", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id1);
		sqlite3_bind_int64 (stmt, 3, face_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #12 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - OverlappingFaces step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_face_within_face (GaiaTopologyAccessorPtr accessor,
				sqlite3_stmt * stmt)
{
/* checking for face-within-face */
    char *sql;
    char *table;
    char *xtable;
    char *rtree;
    int ret;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    sqlite3_stmt *stmt_in = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;


#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name, pid);
    rtree = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT a.face_id, b.face_id FROM TEMP.\"%s\" AS a, TEMP.\"%s\" AS b "
	 "WHERE a.geom IS NOT NULL AND a.face_id <> b.face_id AND ST_Within(a.geom, b.geom) = 1 "
	 "AND b.face_id IN (SELECT id_face FROM TEMP.\"%s\" WHERE x_min <= MbrMaxX(a.geom) "
	 "AND x_max >= MbrMinX(a.geom) AND y_min <= MbrMaxY(a.geom) AND y_max >= MbrMinY(a.geom))",
	 xtable, xtable, rtree);
    free (xtable);
    free (rtree);
    ret =
	sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf
	      ("ST_ValidateTopoGeo() - FaceWithinFace error: \"%s\"",
	       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
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
		sqlite3_int64 face_id1 = sqlite3_column_int64 (stmt_in, 0);
		sqlite3_int64 face_id2 = sqlite3_column_int64 (stmt_in, 1);
		/* reporting the error */
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_text (stmt, 1, "face within face", -1,
				   SQLITE_STATIC);
		sqlite3_bind_int64 (stmt, 2, face_id1);
		sqlite3_bind_int64 (stmt, 3, face_id2);
		ret = sqlite3_step (stmt);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("ST_ValidateTopoGeo() insert #13 error: \"%s\"",
			   sqlite3_errmsg (topo->db_handle));
		      gaiatopo_set_last_error_msg (accessor, msg);
		      sqlite3_free (msg);
		      goto error;
		  }
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("ST_ValidateTopoGeo() - FaceWithinFace step error: %s",
		     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
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
do_topo_check_drop_aux_faces (GaiaTopologyAccessorPtr accessor)
{
/* dropping the aux-Face temp table */
    char *table;
    char *xtable;
    char *sql;
    char *errMsg;
    int ret;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

/* finalizing all prepared Statements */
    finalize_all_topo_prepared_stmts (topo->cache);

/* dropping the aux-face Temp Table */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    table = sqlite3_mprintf ("%s_aux_face_%d", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    create_all_topo_prepared_stmts (topo->cache);	/* recreating prepared stsms */
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("DROP TABLE temp.aux_face - error: %s\n",
				       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* dropping the aux-face Temp RTree */
    table = sqlite3_mprintf ("%s_aux_face_%d_rtree", topo->topology_name, pid);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (topo->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("DROP TABLE temp.aux_face_rtree - error: %s\n",
			       errMsg);
	  sqlite3_free (errMsg);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

GAIATOPO_DECLARE int
gaiaValidateTopoGeo (GaiaTopologyAccessorPtr accessor)
{
/* generating a validity report for a given Topology */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    if (!do_check_create_validate_topogeo_table (accessor))
	return 0;

    table = sqlite3_mprintf ("%s_validate_topogeo", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (error, primitive1, primitive2) VALUES (?, ?, ?)",
	 xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("ST_ValidateTopoGeo error: \"%s\"",
				       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (!do_topo_check_coincident_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_edge_node (accessor, stmt))
	goto error;

    if (!do_topo_check_non_simple (accessor, stmt))
	goto error;

    if (!do_topo_check_edge_edge (accessor, stmt))
	goto error;

    if (!do_topo_check_start_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_end_nodes (accessor, stmt))
	goto error;

    if (!do_topo_check_face_no_edges (accessor, stmt))
	goto error;

    if (!do_topo_check_no_universal_face (accessor, stmt))
	goto error;

    if (!do_topo_check_create_aux_faces (accessor))
	goto error;

    if (!do_topo_check_build_aux_faces (accessor, stmt))
	goto error;

    if (!do_topo_check_overlapping_faces (accessor, stmt))
	goto error;

    if (!do_topo_check_face_within_face (accessor, stmt))
	goto error;

    if (!do_topo_check_drop_aux_faces (accessor))
	goto error;

    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetNodeByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* RTT wrapper - GetNodeByPoint */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_GetNodeByPoint ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_pt,
			    tolerance);
    rtpoint_free (ctx, rt_pt);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetEdgeByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* RTT wrapper - GetEdgeByPoint */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_GetEdgeByPoint ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_pt,
			    tolerance);
    rtpoint_free (ctx, rt_pt);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaGetFaceByPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		    double tolerance)
{
/* RTT wrapper - GetFaceByPoint */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_GetFaceByPoint ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_pt,
			    tolerance);
    rtpoint_free (ctx, rt_pt);

    return ret;
}

GAIATOPO_DECLARE sqlite3_int64
gaiaTopoGeo_AddPoint (GaiaTopologyAccessorPtr accessor, gaiaPointPtr pt,
		      double tolerance)
{
/* RTT wrapper - AddPoint */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    sqlite3_int64 ret;
    int has_z = 0;
    RTPOINT *rt_pt;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
	has_z = 1;
    pa = ptarray_construct (ctx, has_z, 0, 1);
    point.x = pt->X;
    point.y = pt->Y;
    if (has_z)
	point.z = pt->Z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rt_pt = rtpoint_construct (ctx, topo->srid, NULL, pa);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    ret =
	rtt_AddPoint ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_pt, tolerance);
    rtpoint_free (ctx, rt_pt);

    return ret;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_AddLineString (GaiaTopologyAccessorPtr accessor,
			   gaiaLinestringPtr ln, double tolerance,
			   sqlite3_int64 ** edge_ids, int *ids_count)
{
/* RTT wrapper - AddLinestring */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    int ret = 0;
    RTT_ELEMID *edgeids;
    int nedges;
    int i;
    sqlite3_int64 *ids;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    *edge_ids = NULL;
    *ids_count = 0;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    edgeids =
	rtt_AddLine ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_line, tolerance,
		     &nedges);

    rtline_free (ctx, rt_line);
    if (edgeids != NULL)
      {
	  ids = malloc (sizeof (sqlite3_int64) * nedges);
	  for (i = 0; i < nedges; i++)
	      ids[i] = edgeids[i];
	  *edge_ids = ids;
	  *ids_count = nedges;
	  ret = 1;
	  rtfree (ctx, edgeids);
      }
    return ret;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_AddLineStringNoFace (GaiaTopologyAccessorPtr accessor,
				 gaiaLinestringPtr ln, double tolerance,
				 sqlite3_int64 ** edge_ids, int *ids_count)
{
/* RTT wrapper - AddLinestring NO FACE */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    int ret = 0;
    RTT_ELEMID *edgeids;
    int nedges;
    int i;
    sqlite3_int64 *ids;
    RTLINE *rt_line;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    *edge_ids = NULL;
    *ids_count = 0;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_line =
	gaia_convert_linestring_to_rtline (ctx, ln, topo->srid, topo->has_z);

    if (tolerance < 0.0)
	tolerance = topo->tolerance;	/* using the standard tolerance */

    gaiaResetRtTopoMsg (cache);
    edgeids =
	rtt_AddLineNoFace ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_line,
			   tolerance, &nedges);

    rtline_free (ctx, rt_line);
    if (edgeids != NULL)
      {
	  ids = malloc (sizeof (sqlite3_int64) * nedges);
	  for (i = 0; i < nedges; i++)
	      ids[i] = edgeids[i];
	  *edge_ids = ids;
	  *ids_count = nedges;
	  ret = 1;
	  rtfree (ctx, edgeids);
      }
    return ret;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_Polygonize (GaiaTopologyAccessorPtr accessor)
{
/* RTT wrapper - Determine and register all topology faces */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    gaiaResetRtTopoMsg (cache);
    if (rtt_Polygonize ((RTT_TOPOLOGY *) (topo->rtt_topology)) == 0)
	return 1;
    return 0;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoSnap (GaiaTopologyAccessorPtr accessor, gaiaGeomCollPtr geom,
	      double tolerance_snap, double tolerance_removal, int iterate)
{
/* RTT wrapper - TopoSnap */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    RTGEOM *input = NULL;
    RTGEOM *result = NULL;
    gaiaGeomCollPtr output;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return NULL;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return NULL;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return NULL;
    if (!geom)
	return NULL;

    input = toRTGeom (ctx, geom);
    if (!input)
	return NULL;

    if (tolerance_snap < 0.0)
	tolerance_snap = topo->tolerance;

    result =
	rtt_tpsnap ((RTT_TOPOLOGY *) (topo->rtt_topology), input,
		    tolerance_snap, tolerance_removal, iterate);
    rtgeom_free (ctx, input);
    if (result == NULL)
	return NULL;

/* converting the result as a Gaia Geometry */
    output = fromRTGeom (ctx, result, geom->DimensionModel, geom->DeclaredType);
    output->Srid = geom->Srid;
    rtgeom_free (ctx, result);
    return output;
}

GAIATOPO_DECLARE int
gaiaTopoGeo_AddPolygon (GaiaTopologyAccessorPtr accessor, gaiaPolygonPtr pg,
			double tolerance, sqlite3_int64 ** face_ids,
			int *ids_count)
{
/* RTT wrapper - AddPolygon */
    const RTCTX *ctx = NULL;
    struct splite_internal_cache *cache = NULL;
    int ret = 0;
    RTT_ELEMID *faceids;
    int nfaces;
    int i;
    sqlite3_int64 *ids;
    RTPOLY *rt_polyg;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    cache = (struct splite_internal_cache *) topo->cache;
    if (cache == NULL)
	return 0;
    if (cache->magic1 != SPATIALITE_CACHE_MAGIC1
	|| cache->magic2 != SPATIALITE_CACHE_MAGIC2)
	return 0;
    ctx = cache->RTTOPO_handle;
    if (ctx == NULL)
	return 0;

    rt_polyg =
	gaia_convert_polygon_to_rtpoly (ctx, pg, topo->srid, topo->has_z);

    gaiaResetRtTopoMsg (cache);
    faceids =
	rtt_AddPolygon ((RTT_TOPOLOGY *) (topo->rtt_topology), rt_polyg,
			tolerance, &nfaces);

    rtpoly_free (ctx, rt_polyg);
    if (faceids != NULL)
      {
	  ids = malloc (sizeof (sqlite3_int64) * nfaces);
	  for (i = 0; i < nfaces; i++)
	      ids[i] = faceids[i];
	  *face_ids = ids;
	  *ids_count = nfaces;
	  ret = 1;
	  rtfree (ctx, faceids);
      }
    return ret;
}

static void
do_split_line (gaiaGeomCollPtr geom, gaiaDynamicLinePtr dyn)
{
/* inserting a new linestring into the collection of split lines */
    int points = 0;
    int iv = 0;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;

    pt = dyn->First;
    while (pt != NULL)
      {
	  /* counting how many points */
	  points++;
	  pt = pt->Next;
      }

    ln = gaiaAddLinestringToGeomColl (geom, points);
    pt = dyn->First;
    while (pt != NULL)
      {
	  /* copying all points */
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (ln->Coords, iv, pt->X, pt->Y, pt->Z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaSetPointXYM (ln->Coords, iv, pt->X, pt->Y, pt->M);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaSetPointXYZM (ln->Coords, iv, pt->X, pt->Y, pt->Z, pt->M);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, iv, pt->X, pt->Y);
	    }
	  iv++;
	  pt = pt->Next;
      }
}

static void
do_geom_split_line (gaiaGeomCollPtr geom, gaiaLinestringPtr in,
		    int line_max_points, double max_length)
{
/* splitting a Linestring into a collection of shorter Linestrings */
    int iv;
    int count = 0;
    int split = 0;
    double tot_length = 0.0;
    gaiaDynamicLinePtr dyn = gaiaAllocDynamicLine ();

    for (iv = 0; iv < in->Points; iv++)
      {
	  /* consuming all Points from the input Linestring */
	  double ox;
	  double oy;
	  double x;
	  double y;
	  double z = 0.0;
	  double m = 0.0;
	  if (in->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (in->Coords, iv, &x, &y, &z);
	    }
	  else if (in->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (in->Coords, iv, &x, &y, &m);
	    }
	  else if (in->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (in->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (in->Coords, iv, &x, &y);
	    }

	  split = 0;
	  if (max_length > 0.0)
	    {
		if (tot_length > max_length)
		    split = 1;
	    }
	  if (line_max_points > 0)
	    {
		if (count == line_max_points)
		    split = 1;
	    }
	  if (split && count >= 2)
	    {
		/* line break */
		double oz;
		double om;
		ox = dyn->Last->X;
		oy = dyn->Last->Y;
		if (in->DimensionModel == GAIA_XY_Z
		    || in->DimensionModel == GAIA_XY_Z_M)
		    oz = dyn->Last->Z;
		if (in->DimensionModel == GAIA_XY_M
		    || in->DimensionModel == GAIA_XY_Z_M)
		    om = dyn->Last->M;
		do_split_line (geom, dyn);
		gaiaFreeDynamicLine (dyn);
		dyn = gaiaAllocDynamicLine ();
		/* reinserting the last point */
		if (in->DimensionModel == GAIA_XY_Z)
		    gaiaAppendPointZToDynamicLine (dyn, ox, oy, oz);
		else if (in->DimensionModel == GAIA_XY_M)
		    gaiaAppendPointMToDynamicLine (dyn, ox, oy, om);
		else if (in->DimensionModel == GAIA_XY_Z_M)
		    gaiaAppendPointZMToDynamicLine (dyn, ox, oy, oz, om);
		else
		    gaiaAppendPointToDynamicLine (dyn, ox, oy);
		count = 1;
		tot_length = 0.0;
	    }

	  /* inserting a point */
	  if (in->DimensionModel == GAIA_XY_Z)
	      gaiaAppendPointZToDynamicLine (dyn, x, y, z);
	  else if (in->DimensionModel == GAIA_XY_M)
	      gaiaAppendPointMToDynamicLine (dyn, x, y, m);
	  else if (in->DimensionModel == GAIA_XY_Z_M)
	      gaiaAppendPointZMToDynamicLine (dyn, x, y, z, m);
	  else
	      gaiaAppendPointToDynamicLine (dyn, x, y);
	  if (count > 0)
	    {
		if (max_length > 0.0)
		  {
		      double dist =
			  sqrt (((ox - x) * (ox - x)) + ((oy - y) * (oy - y)));
		      tot_length += dist;
		  }
	    }
	  ox = x;
	  oy = y;
	  count++;
      }

    if (dyn->First != NULL)
      {
	  /* flushing the last Line */
	  do_split_line (geom, dyn);
      }
    gaiaFreeDynamicLine (dyn);
}

static gaiaGeomCollPtr
do_linearize (gaiaGeomCollPtr geom)
{
/* attempts to transform Polygon Rings into a (multi)linestring */
    gaiaGeomCollPtr result;
    gaiaLinestringPtr new_ln;
    gaiaPolygonPtr pg;
    gaiaRingPtr rng;
    int iv;
    int ib;
    double x;
    double y;
    double m;
    double z;
    if (!geom)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;

    pg = geom->FirstPolygon;
    while (pg)
      {
	  /* dissolving any POLYGON as simple LINESTRINGs (rings) */
	  rng = pg->Exterior;
	  new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		/* copying the EXTERIOR RING as LINESTRING */
		if (geom->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		      gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
		  }
		else if (geom->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		      gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
		  }
		else if (geom->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		      gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		      gaiaSetPoint (new_ln->Coords, iv, x, y);
		  }
	    }
	  for (ib = 0; ib < pg->NumInteriors; ib++)
	    {
		rng = pg->Interiors + ib;
		new_ln = gaiaAddLinestringToGeomColl (result, rng->Points);
		for (iv = 0; iv < rng->Points; iv++)
		  {
		      /* copying an INTERIOR RING as LINESTRING */
		      if (geom->DimensionModel == GAIA_XY_Z_M)
			{
			    gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
			    gaiaSetPointXYZM (new_ln->Coords, iv, x, y, z, m);
			}
		      else if (geom->DimensionModel == GAIA_XY_Z)
			{
			    gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
			    gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
			}
		      else if (geom->DimensionModel == GAIA_XY_M)
			{
			    gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
			    gaiaSetPointXYM (new_ln->Coords, iv, x, y, m);
			}
		      else
			{
			    gaiaGetPoint (rng->Coords, iv, &x, &y);
			    gaiaSetPoint (new_ln->Coords, iv, x, y);
			}
		  }
	    }
	  pg = pg->Next;
      }
    if (result->FirstLinestring == NULL)
      {
	  gaiaFreeGeomColl (result);
	  return NULL;
      }
    return result;
}

GAIATOPO_DECLARE gaiaGeomCollPtr
gaiaTopoGeo_SubdivideLines (gaiaGeomCollPtr geom, int line_max_points,
			    double max_length)
{
/* subdividing a (multi)Linestring into a collection of simpler Linestrings */
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr result;

    if (geom == NULL)
	return NULL;

    if (geom->FirstPoint != NULL)
	return NULL;
    if (geom->FirstLinestring == NULL && geom->FirstPolygon != NULL)
	return NULL;

    if (geom->DimensionModel == GAIA_XY_Z)
	result = gaiaAllocGeomCollXYZ ();
    else if (geom->DimensionModel == GAIA_XY_M)
	result = gaiaAllocGeomCollXYM ();
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	result = gaiaAllocGeomCollXYZM ();
    else
	result = gaiaAllocGeomColl ();
    result->Srid = geom->Srid;
    result->DeclaredType = GAIA_MULTILINESTRING;
    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  do_geom_split_line (result, ln, line_max_points, max_length);
	  ln = ln->Next;
      }

    if (geom->FirstPolygon != NULL)
      {
	  /* transforming all Polygon Rings into Linestrings */
	  gaiaGeomCollPtr pg_rings = do_linearize (geom);
	  if (pg_rings != NULL)
	    {
		ln = pg_rings->FirstLinestring;
		while (ln != NULL)
		  {
		      do_geom_split_line (result, ln, line_max_points,
					  max_length);
		      ln = ln->Next;
		  }
		gaiaFreeGeomColl (pg_rings);
	    }
      }
    return result;
}

static gaiaGeomCollPtr
do_build_failing_point (int srid, int dims, gaiaPointPtr pt)
{
/* building a Point geometry */
    gaiaGeomCollPtr geom;
    if (dims == GAIA_XY_Z)
	geom = gaiaAllocGeomCollXYZ ();
    else if (dims == GAIA_XY_M)
	geom = gaiaAllocGeomCollXYM ();
    else if (dims == GAIA_XY_Z_M)
	geom = gaiaAllocGeomCollXYZM ();
    else
	geom = gaiaAllocGeomColl ();
    geom->Srid = srid;
    if (dims == GAIA_XY_Z)
	gaiaAddPointToGeomCollXYZ (geom, pt->X, pt->Y, pt->Z);
    else if (geom->DimensionModel == GAIA_XY_M)
	gaiaAddPointToGeomCollXYM (geom, pt->X, pt->Y, pt->M);
    else if (geom->DimensionModel == GAIA_XY_Z_M)
	gaiaAddPointToGeomCollXYZM (geom, pt->X, pt->Y, pt->Z, pt->M);
    else
	gaiaAddPointToGeomColl (geom, pt->X, pt->Y);
    return geom;
}

static gaiaGeomCollPtr
do_build_failing_line (int srid, int dims, gaiaLinestringPtr line)
{
/* building a Linestring geometry */
    gaiaGeomCollPtr geom;
    gaiaLinestringPtr ln;
    if (dims == GAIA_XY_Z)
	geom = gaiaAllocGeomCollXYZ ();
    else if (dims == GAIA_XY_M)
	geom = gaiaAllocGeomCollXYM ();
    else if (dims == GAIA_XY_Z_M)
	geom = gaiaAllocGeomCollXYZM ();
    else
	geom = gaiaAllocGeomColl ();
    geom->Srid = srid;
    ln = gaiaAddLinestringToGeomColl (geom, line->Points);
    gaiaCopyLinestringCoords (ln, line);
    return geom;
}

TOPOLOGY_PRIVATE int
auxtopo_insert_into_topology (GaiaTopologyAccessorPtr accessor,
			      gaiaGeomCollPtr geom, double tolerance,
			      int line_max_points, double max_length, int mode,
			      gaiaGeomCollPtr * failing_geometry)
{
/* processing all individual geometry items */
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr g;
    gaiaGeomCollPtr split = NULL;
    gaiaGeomCollPtr pg_rings;
    sqlite3_int64 *ids = NULL;
    int ids_count;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;

    if (failing_geometry != NULL)
	*failing_geometry = NULL;
    if (topo == NULL)
	return 0;

    pt = geom->FirstPoint;
    while (pt != NULL)
      {
	  /* looping on Point items */
	  if (gaiaTopoGeo_AddPoint (accessor, pt, tolerance) < 0)
	    {
		if (failing_geometry != NULL)
		    *failing_geometry =
			do_build_failing_point (geom->Srid,
						geom->DimensionModel, pt);
		return 0;
	    }
	  pt = pt->Next;
      }

    if (line_max_points <= 0 && max_length <= 0.0)
	g = geom;
    else
      {
	  /* subdividing Linestrings */
	  split =
	      gaiaTopoGeo_SubdivideLines (geom, line_max_points, max_length);
	  if (split != NULL)
	      g = split;
	  else
	      g = geom;
      }
    ln = g->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on Linestrings items */
	  int ret;
	  if (mode == GAIA_MODE_TOPO_NO_FACE)
	      ret = gaiaTopoGeo_AddLineStringNoFace
		  (accessor, ln, tolerance, &ids, &ids_count);
	  else
	      ret = gaiaTopoGeo_AddLineString
		  (accessor, ln, tolerance, &ids, &ids_count);
	  if (ret == 0)
	    {
		if (failing_geometry != NULL)
		    *failing_geometry =
			do_build_failing_line (geom->Srid, geom->DimensionModel,
					       ln);
		if (ids != NULL)
		    free (ids);
		if (split != NULL)
		    gaiaFreeGeomColl (split);
		return 0;
	    }
	  if (ids != NULL)
	      free (ids);
	  ln = ln->Next;
      }
    if (split != NULL)
	gaiaFreeGeomColl (split);
    split = NULL;

/* transforming all Polygon Rings into Linestrings */
    pg_rings = do_linearize (geom);
    if (pg_rings != NULL)
      {
	  if (line_max_points <= 0 && max_length <= 0.0)
	      g = pg_rings;
	  else
	    {
		/* subdividing Linestrings */
		split =
		    gaiaTopoGeo_SubdivideLines (pg_rings, line_max_points,
						max_length);
		if (split != NULL)
		    g = split;
		else
		    g = pg_rings;
	    }
	  ln = g->FirstLinestring;
	  while (ln != NULL)
	    {
		/* looping on Linestrings items */
		int ret;
		if (mode == GAIA_MODE_TOPO_NO_FACE)
		    ret = gaiaTopoGeo_AddLineStringNoFace
			(accessor, ln, tolerance, &ids, &ids_count);
		else
		    ret = gaiaTopoGeo_AddLineString
			(accessor, ln, tolerance, &ids, &ids_count);
		if (ret == 0)
		  {
		      if (failing_geometry != NULL)
			  *failing_geometry =
			      do_build_failing_line (geom->Srid,
						     geom->DimensionModel, ln);
		      if (ids != NULL)
			  free (ids);
		      gaiaFreeGeomColl (pg_rings);
		      if (split != NULL)
			  gaiaFreeGeomColl (split);
		      return 0;
		  }
		if (ids != NULL)
		    free (ids);
		ln = ln->Next;
	    }
	  gaiaFreeGeomColl (pg_rings);
	  if (split != NULL)
	      gaiaFreeGeomColl (split);
	  split = NULL;
      }

    return 1;
}

#endif /* end ENABLE_RTTOPO conditionals */
