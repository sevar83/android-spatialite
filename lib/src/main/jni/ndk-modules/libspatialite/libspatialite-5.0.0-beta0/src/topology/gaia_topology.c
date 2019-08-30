/*

 gaia_topology.c -- implementation of Topology SQL functions
    
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

UPDATE: supporting the new RTTOTPO 1.1 API was funded by Regione Toscana
CIG: 644544015A Linea A

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
#include <spatialite/gaia_topology.h>
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <librttopo.h>

#include "topology_private.h"

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

#define GAIA_UNUSED() if (argc || argv) argc = argc;

struct pk_item
{
/* an helper struct for a primary key column */
    char *name;
    char *type;
    int notnull;
    int pk;
    struct pk_item *next;
};

struct pk_struct
{
/* an helper struct for cloning dustbin primary keys */
    struct pk_item *first;
    struct pk_item *last;
    int count;
};

static struct pk_struct *
create_pk_dictionary (void)
{
/* creating an empty PK dictionary */
    struct pk_struct *pk = malloc (sizeof (struct pk_struct));
    pk->first = NULL;
    pk->last = NULL;
    pk->count = 0;
    return pk;
}

static void
free_pk_dictionary (struct pk_struct *pk)
{
/* memory cleanup - freeing a PK dictionary */
    struct pk_item *pI;
    struct pk_item *pIn;
    if (pk == NULL)
	return;
    pI = pk->first;
    while (pI != NULL)
      {
	  pIn = pI->next;
	  if (pI->name != NULL)
	      free (pI->name);
	  if (pI->type != NULL)
	      free (pI->type);
	  free (pI);
	  pI = pIn;
      }
    free (pk);
}

static void
add_pk_column (struct pk_struct *pk, const char *name, const char *type,
	       int notnull, int pk_pos)
{
/* adding a PK column into a dictionary */
    int len;
    struct pk_item *pI;
    if (pk == NULL)
	return;
    if (name == NULL || type == NULL)
	return;
    pI = malloc (sizeof (struct pk_item));
    len = strlen (name);
    pI->name = malloc (len + 1);
    strcpy (pI->name, name);
    len = strlen (type);
    pI->type = malloc (len + 1);
    strcpy (pI->type, type);
    pI->notnull = notnull;
    pI->pk = pk_pos;
    pI->next = NULL;
/* inserting into the PK dictionary linked list */
    if (pk->first == NULL)
	pk->first = pI;
    if (pk->last != NULL)
	pk->last->next = pI;
    pk->last = pI;
    pk->count += 1;
}

static struct splite_savepoint *
push_topo_savepoint (struct splite_internal_cache *cache)
{
/* adding a new SavePoint to the Topology stack */
    struct splite_savepoint *p_svpt = malloc (sizeof (struct splite_savepoint));
    p_svpt->savepoint_name = NULL;
    p_svpt->prev = cache->last_topo_svpt;
    p_svpt->next = NULL;
    if (cache->first_topo_svpt == NULL)
	cache->first_topo_svpt = p_svpt;
    if (cache->last_topo_svpt != NULL)
	cache->last_topo_svpt->next = p_svpt;
    cache->last_topo_svpt = p_svpt;
    return p_svpt;
}

static void
pop_topo_savepoint (struct splite_internal_cache *cache)
{
/* removing a SavePoint from the Topology stack */
    struct splite_savepoint *p_svpt = cache->last_topo_svpt;
    if (p_svpt->prev != NULL)
	p_svpt->prev->next = NULL;
    cache->last_topo_svpt = p_svpt->prev;
    if (cache->first_topo_svpt == p_svpt)
	cache->first_topo_svpt = NULL;
    if (p_svpt->savepoint_name != NULL)
	sqlite3_free (p_svpt->savepoint_name);
    free (p_svpt);
}

SPATIALITE_PRIVATE void
start_topo_savepoint (const void *handle, const void *data)
{
/* starting a new SAVEPOINT */
    char *sql;
    int ret;
    char *err_msg;
    struct splite_savepoint *p_svpt;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;

/* creating an unique SavePoint name */
    p_svpt = push_topo_savepoint (cache);
    p_svpt->savepoint_name =
	sqlite3_mprintf ("toposvpt%04x", cache->next_topo_savepoint);
    if (cache->next_topo_savepoint >= 0xffffffffu)
	cache->next_topo_savepoint = 0;
    else
	cache->next_topo_savepoint += 1;

/* starting a SavePoint */
    sql = sqlite3_mprintf ("SAVEPOINT %s", p_svpt->savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
}

SPATIALITE_PRIVATE void
release_topo_savepoint (const void *handle, const void *data)
{
/* releasing the current SAVEPOINT (if any) */
    char *sql;
    int ret;
    char *err_msg;
    struct splite_savepoint *p_svpt;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;
    p_svpt = cache->last_topo_svpt;
    if (p_svpt == NULL)
	return;
    if (p_svpt->savepoint_name == NULL)
	return;

/* releasing the current SavePoint */
    sql = sqlite3_mprintf ("RELEASE SAVEPOINT %s", p_svpt->savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
    pop_topo_savepoint (cache);
}

SPATIALITE_PRIVATE void
rollback_topo_savepoint (const void *handle, const void *data)
{
/* rolling back the current SAVEPOINT (if any) */
    char *sql;
    int ret;
    char *err_msg;
    struct splite_savepoint *p_svpt;
    sqlite3 *sqlite = (sqlite3 *) handle;
    struct splite_internal_cache *cache = (struct splite_internal_cache *) data;
    if (sqlite == NULL || cache == NULL)
	return;
    p_svpt = cache->last_topo_svpt;
    if (p_svpt == NULL)
	return;
    if (p_svpt->savepoint_name == NULL)
	return;

/* rolling back the current SavePoint */
    sql = sqlite3_mprintf ("ROLLBACK TO SAVEPOINT %s", p_svpt->savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
/* releasing the current SavePoint */
    sql = sqlite3_mprintf ("RELEASE SAVEPOINT %s", p_svpt->savepoint_name);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("%s - error: %s\n", sql, err_msg);
	  sqlite3_free (err_msg);
      }
    sqlite3_free (sql);
    pop_topo_savepoint (cache);
}

SPATIALITE_PRIVATE void
fnctaux_GetLastTopologyException (const void *xcontext, int argc,
				  const void *xargv)
{
/* SQL function:
/ GetLastTopologyException  ( text topology-name )
/
/ returns: the more recent exception raised by given Topology
/ NULL on invalid args (or when there is no pending exception)
*/
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_null (context);
	  return;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }

    sqlite3_result_text (context, gaiatopo_get_last_exception (accessor), -1,
			 SQLITE_STATIC);
}

SPATIALITE_PRIVATE void
fnctaux_CreateTopoTables (const void *xcontext, int argc, const void *argv)
{
/* SQL function:
/ CreateTopoTables()
/
/ creates both TOPOLOGIES and NETWORKS tables
/ returns 1 on success
/ 0 on failure, -1 on invalid arguments
*/
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    int topogeo;
    int toponet;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */

    topogeo = do_create_topologies (sqlite);
    toponet = do_create_networks (sqlite);
    if (topogeo || toponet)
	sqlite3_result_int (context, 1);
    else
	sqlite3_result_int (context, 0);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_CreateTopology (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_InitTopoGeo ( text topology-name )
/ CreateTopology ( text topology-name )
/ CreateTopology ( text topology-name, int srid )
/ CreateTopology ( text topology-name, int srid, bool hasZ )
/ CreateTopology ( text topology-name, int srid, bool hasZ,
/                  double tolerance )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
/ excèption on NEGATIVE tolerance
*/
    const char *msg;
    int ret;
    const char *topo_name;
    int srid = -1;
    int has_z = 0;
    double tolerance = 0.0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }
    if (argc >= 2)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	      srid = sqlite3_value_int (argv[1]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	      has_z = sqlite3_value_int (argv[2]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (argc >= 4)
      {
	  if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[3]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[3]);
	  else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	    {
		int tol = sqlite3_value_int (argv[3]);
		tolerance = tol;
	    }
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }

/* raising an exception on NEGATIVE tolerance */
    if (tolerance < 0.0)
	goto negative_tolerance;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyCreate (sqlite, topo_name, srid, tolerance, has_z);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
    return;

  negative_tolerance:
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_DropTopology (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ DropTopology ( text topology-name )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
*/
    int ret;
    const char *topo_name;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GaiaTopologyAccessorPtr accessor;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor != NULL)
	gaiaTopologyDestroy (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyDrop (sqlite, topo_name);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
}

static int
check_matching_srid_dims (GaiaTopologyAccessorPtr accessor, int srid, int dims)
{
/* checking for matching SRID and DIMs */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo->srid != srid)
	return 0;
    if (topo->has_z)
      {
	  if (dims == GAIA_XY_Z || dims == GAIA_XY_Z_M)
	      ;
	  else
	      return 0;
      }
    else
      {
	  if (dims == GAIA_XY_Z || dims == GAIA_XY_Z_M)
	      return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_AddIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddIsoNode ( text topology-name, int face_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	face_id = -1;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  face_id = sqlite3_value_int64 (argv[1]);
	  if (face_id <= 0)
	      face_id = -1;
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddIsoNode (accessor, face_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_MoveIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_MoveIsoNode ( text topology-name, int node_id, Geometry point )
/
/ returns: TEXT (description of new location)
/ raises an exception on failure
*/
    const char *msg;
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;
    sprintf (xid, "%lld", node_id);
    newpos =
	sqlite3_mprintf ("Isolated Node %s moved to location %f,%f", xid, pt->X,
			 pt->Y);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaMoveIsoNode (accessor, node_id, pt);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemIsoNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemIsoNode ( text topology-name, int node_id )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    const char *msg;
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 node_id;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    sprintf (xid, "%lld", node_id);
    newpos = sqlite3_mprintf ("Isolated Node %s removed", xid);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemIsoNode (accessor, node_id);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddIsoEdge (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddIsoEdge ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success, 0 on failure
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddIsoEdge (accessor, start_node_id, end_node_id, ln);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemIsoEdge (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemIsoEdge ( text topology-name, int edfe_id )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    const char *msg;
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    sprintf (xid, "%lld", edge_id);
    newpos = sqlite3_mprintf ("Isolated Edge %s removed", xid);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemIsoEdge (accessor, edge_id);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemEdgeModFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemEdgeModFace ( text topology-name, int edge_id )
/
/ returns: ID of the Face that takes up the space previously occupied by the removed edge
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemEdgeModFace (accessor, edge_id);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemEdgeNewFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemEdgeNewFace ( text topology-name, int edge_id )
/
/ returns: ID of the created Face 
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaRemEdgeNewFace (accessor, edge_id);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ChangeEdgeGeom (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ChangeEdgeGeom ( text topology-name, int edge_id, Geometry linestring )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    const char *msg;
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;
    sprintf (xid, "%lld", edge_id);
    newpos = sqlite3_mprintf ("Edge %s changed", xid);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaChangeEdgeGeom (accessor, edge_id, ln);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_topo:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModEdgeSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModEdgeSplit ( text topology-name, int edge_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaModEdgeSplit (accessor, edge_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewEdgesSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewEdgesSplit ( text topology-name, int edge_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);
      }
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
      {
	  goto invalid_arg;
	  return;
      }
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaNewEdgesSplit (accessor, edge_id, pt, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddEdgeModFace (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddEdgeModFace ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddEdgeModFace (accessor, start_node_id, end_node_id, ln, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddEdgeNewFaces (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddEdgeNewFaces ( text topology-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Edge on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	start_node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	end_node_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);
      }
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    line =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!line)
	goto invalid_arg;
    if (line->FirstPoint != NULL)
	invalid = 1;
    if (line->FirstPolygon != NULL)
	invalid = 1;
    if (line->FirstLinestring != line->LastLinestring
	|| line->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, line->Srid, line->DimensionModel))
	goto invalid_geom;
    ln = line->FirstLinestring;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaAddEdgeNewFaces (accessor, start_node_id, end_node_id, ln, 0);
    if (ret <= 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModEdgeHeal ( text topology-name, int edge_id1, int edge_id2 )
/
/ returns: ID of the removed Node
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id1;
    sqlite3_int64 edge_id2;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id1 = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	edge_id2 = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaModEdgeHeal (accessor, edge_id1, edge_id2);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewEdgeHeal ( text topology-name, int edge_id1, int edge_id2 )
/
/ returns: ID of the removed Node
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    sqlite3_int64 edge_id1;
    sqlite3_int64 edge_id2;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id1 = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	edge_id2 = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaNewEdgeHeal (accessor, edge_id1, edge_id2);
    if (ret < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceGeometry (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_GetFaceGeometry ( text topology-name, int face_id )
/
/ returns: the Face's geometry (Polygon)
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    geom = gaiaGetFaceGeometry (accessor, face_id);
    if (geom == NULL)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode, tiny_point);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceEdges (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_GetFaceEdges ( text topology-name, int face_id )
/
/ create/update a table containing an ordered list of EdgeIDs
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    sqlite3_int64 face_id;
    int ret;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    start_topo_savepoint (sqlite, cache);
    ret = gaiaGetFaceEdges (accessor, face_id);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
check_empty_topology (struct gaia_topology *topo)
{
/* checking for an empty Topology */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int already_populated = 0;

/* testing NODE */
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
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
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

/* testing EDGE */
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
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
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

/* testing FACE */
    table = sqlite3_mprintf ("%s_face", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\" WHERE face_id <> 0",
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
	  if (atoi (results[(i * columns) + 0]) > 0)
	      already_populated = 1;
      }
    sqlite3_free_table (results);
    if (already_populated)
	return 0;

    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_ValidateTopoGeo (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ValidateTopoGeo ( text topology-name )
/
/ create/update a table containing an validation report for a given TopoGeo
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    int ret;
    GaiaTopologyAccessorPtr accessor = NULL;
    struct gaia_topology *topo;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    topo = (struct gaia_topology *) accessor;
    if (check_empty_topology (topo))
	goto empty;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaValidateTopoGeo (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  empty:
    msg = "SQL/MM Spatial exception - empty topology.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_CreateTopoGeo (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_CreateTopoGeo ( text topology-name , blob geom-collection )
/
/ creates and populates an empty Topology by importing a Geometry-collection
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    int ret;
    const unsigned char *blob;
    int blob_sz;
    gaiaGeomCollPtr geom = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    struct gaia_topology *topo;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  blob = sqlite3_value_blob (argv[1]);
	  blob_sz = sqlite3_value_bytes (argv[1]);
	  geom =
	      gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz, gpkg_mode,
					   gpkg_amphibious);
      }
    else
	goto invalid_arg;
    if (geom == NULL)
	goto not_geom;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    topo = (struct gaia_topology *) accessor;
    if (!check_empty_topology (topo))
	goto not_empty;
    if (!check_matching_srid_dims (accessor, geom->Srid, geom->DimensionModel))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);
    ret =
	auxtopo_insert_into_topology (accessor, geom, 0.0, -1, -1,
				      GAIA_MODE_TOPO_FACE, NULL);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    gaiaFreeGeomColl (geom);
    return;

  no_topo:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  not_empty:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - non-empty topology.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  not_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - not a Geometry.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg =
	"SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetNodeByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetNodeByPoint ( text topology-name, Geometry point )
/ GetNodeByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Node on success, 0 if no Node was found
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = -1;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    pt = point->FirstPoint;

    ret = gaiaGetNodeByPoint (accessor, pt, tolerance);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetEdgeByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetEdgeByPoint ( text topology-name, Geometry point )
/ GetEdgeByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Edge on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = -1;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    pt = point->FirstPoint;

    ret = gaiaGetEdgeByPoint (accessor, pt, tolerance);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetFaceByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetFaceByPoint ( text topology-name, Geometry point )
/ GetFaceByPoint ( text topology-name, Geometry point, double tolerance )
/
/ returns: the ID of some Face on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 ret;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = -1;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint != point->LastPoint || point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    pt = point->FirstPoint;

    ret = gaiaGetFaceByPoint (accessor, pt, tolerance);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_AddPoint ( text topology-name, Geometry (multi)point )
/ TopoGeo_AddPoint ( text topology-name, Geometry (multi)point, double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Nodes on success
/ raises an exception on failure
*/
    const char *msg;
    sqlite3_int64 node_id;
    char xnode_id[64];
    char *retlist = NULL;
    char *savelist;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = -1;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Point Geometry */
    point =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!point)
	goto invalid_arg;
    if (point->FirstLinestring != NULL)
	invalid = 1;
    if (point->FirstPolygon != NULL)
	invalid = 1;
    if (point->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);
    pt = point->FirstPoint;
    while (pt != NULL)
      {
	  /* looping on elementary Points */
	  node_id = gaiaTopoGeo_AddPoint (accessor, pt, tolerance);
	  if (node_id < 0)
	      break;
	  sprintf (xnode_id, "%lld", node_id);
	  if (retlist == NULL)
	      retlist = sqlite3_mprintf ("%s", xnode_id);
	  else
	    {
		savelist = retlist;
		retlist = sqlite3_mprintf ("%s, %s", savelist, xnode_id);
		sqlite3_free (savelist);
	    }
	  pt = pt->Next;
      }

    if (node_id < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (node_id < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (retlist != NULL)
	      sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddLineString (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_AddLineString ( text topology-name, Geometry (multi)linestring )
/ TopoGeo_AddLineString ( text topology-name, Geometry (multi)linestring, double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Edges on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    char xedge_id[64];
    sqlite3_int64 *edge_ids = NULL;
    int ids_count = 0;
    char *retlist = NULL;
    char *savelist;
    int i;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr linestring = NULL;
    gaiaLinestringPtr ln;
    double tolerance = -1;
    int invalid = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Linestring Geometry */
    linestring =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!linestring)
	goto invalid_arg;
    if (linestring->FirstPoint != NULL)
	invalid = 1;
    if (linestring->FirstPolygon != NULL)
	invalid = 1;
    if (linestring->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims
	(accessor, linestring->Srid, linestring->DimensionModel))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);
    ln = linestring->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on individual Linestrings */
	  ret =
	      gaiaTopoGeo_AddLineString (accessor, ln, tolerance, &edge_ids,
					 &ids_count);
	  if (!ret)
	      break;
	  for (i = 0; i < ids_count; i++)
	    {
		sprintf (xedge_id, "%lld", edge_ids[i]);
		if (retlist == NULL)
		    retlist = sqlite3_mprintf ("%s", xedge_id);
		else
		  {
		      savelist = retlist;
		      retlist = sqlite3_mprintf ("%s, %s", savelist, xedge_id);
		      sqlite3_free (savelist);
		  }
	    }
	  free (edge_ids);
	  ln = ln->Next;
      }

    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (linestring);
    linestring = NULL;
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
kill_all_existing_faces (sqlite3 * sqlite, char *toponame)
{
/* to be executed before invoking any NO FACE function */
    char *sql;
    char *table;
    char *xtable;
    int ret;
    char *errMsg = NULL;

/* invalidating all relationships between Edges and Faces */
    table = sqlite3_mprintf ("%s_edge", toponame);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET left_face = NULL, right_face = NULL "
	 "WHERE left_face IS NOT NULL OR right_face IS NOT NULL", xtable);
    free (xtable);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("NoFace invalidate Edge/Face: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }

/* invalidating all relationships between Nodes and Faces */
    table = sqlite3_mprintf ("%s_node", toponame);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("UPDATE \"%s\" SET containing_face = NULL "
	 "WHERE containing_face IS NOT NULL", xtable);
    free (xtable);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("NoFace invalidate Node/Face: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }

/* removing all Faces except the Universe */
    table = sqlite3_mprintf ("%s_face", toponame);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("DELETE FROM \"%s\" WHERE face_id <> 0", xtable);
    free (xtable);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("cazzo NoFace remove Faces: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_AddLineStringNoFace (const void *xcontext, int argc,
				     const void *xargv)
{
/* SQL function:
/ TopoGeo_AddLineStringNoFace ( text topology-name, Geometry (multi)linestring )
/ TopoGeo_AddLineStringNoFace ( text topology-name, Geometry (multi)linestring, 
/                               double tolerance )
/
/ returns: a comma separated list of all IDs of corresponding Edges on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    char xedge_id[64];
    sqlite3_int64 *edge_ids = NULL;
    int ids_count = 0;
    char *retlist = NULL;
    char *savelist;
    int i;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr linestring = NULL;
    gaiaLinestringPtr ln;
    double tolerance = -1;
    int invalid = 0;
    struct gaia_topology *topo;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Linestring Geometry */
    linestring =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!linestring)
	goto invalid_arg;
    if (linestring->FirstPoint != NULL)
	invalid = 1;
    if (linestring->FirstPolygon != NULL)
	invalid = 1;
    if (linestring->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    topo = (struct gaia_topology *) accessor;
    if (!check_matching_srid_dims
	(accessor, linestring->Srid, linestring->DimensionModel))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);

/* removing any existing Face except the Universal one */
    if (kill_all_existing_faces (sqlite, topo->topology_name) == 0)
      {
	  msg = "TopoGeo_AddLineStringNoFace: unable to remove existing Faces";
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }

    ln = linestring->FirstLinestring;
    while (ln != NULL)
      {
	  /* looping on individual Linestrings */
	  ret =
	      gaiaTopoGeo_AddLineStringNoFace (accessor, ln, tolerance,
					       &edge_ids, &ids_count);
	  if (!ret)
	      break;
	  for (i = 0; i < ids_count; i++)
	    {
		sprintf (xedge_id, "%lld", edge_ids[i]);
		if (retlist == NULL)
		    retlist = sqlite3_mprintf ("%s", xedge_id);
		else
		  {
		      savelist = retlist;
		      retlist = sqlite3_mprintf ("%s, %s", savelist, xedge_id);
		      sqlite3_free (savelist);
		  }
	    }
	  free (edge_ids);
	  ln = ln->Next;
      }

    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    gaiaFreeGeomColl (linestring);
    linestring = NULL;
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  sqlite3_free (retlist);
	  return;
      }
    sqlite3_result_text (context, retlist, strlen (retlist), sqlite3_free);
    return;

  no_topo:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg =
	"SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (linestring != NULL)
	gaiaFreeGeomColl (linestring);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_Polygonize (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_Polygonize ( text topology-name )
/
/ TopoGeo_Polygonize ( text topology-name , int force-rebuild )
*/
    int edgesCount = 0;
    const char *msg;
    int ret;
    const char *topo_name;
    int force_rebuild = 0;
    struct gaia_topology *topo;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (argc >= 2)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	      force_rebuild = sqlite3_value_int (argv[1]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    topo = (struct gaia_topology *) accessor;

/* testing if there are unreferenced Edges */
    edgesCount = test_inconsistent_topology (accessor);
    if (edgesCount < 0)
      {
	  msg = "TopoGeo_Polygonize: unable to check Topology consistency";
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    if (!edgesCount)
      {
	  if (!force_rebuild)
	    {
		sqlite3_result_null (context);
		return;
	    }
      }

    start_topo_savepoint (sqlite, cache);

/* removing any existing Face except the Universal one */
    if (kill_all_existing_faces (sqlite, topo->topology_name) == 0)
      {
	  msg = "TopoGeo_Polygonize: unable to remove existing Faces";
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }

    ret = gaiaTopoGeo_Polygonize (accessor);

    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_TopoSnap (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_TopoSnap ( text topology-name, Geometry geom, int iterate )
/ TopoGeo_TopoSnap ( text topology-name, Geometry geom, double tolerance_snap,
/                    double tolerance_removal, int iterate )
/
/ returns: the snapped Geometry
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom = NULL;
    gaiaGeomCollPtr g2;
    int iterate;
    double tolerance_snap = -1;
    double tolerance_removal = -1;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);
      }
    else
	goto invalid_arg;
    if (argc == 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	      iterate = sqlite3_value_int (argv[2]);
	  else
	      goto invalid_arg;
      }
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[2]);
		tolerance_snap = t;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      tolerance_snap = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
	  if (tolerance_snap < 0.0)
	      goto negative_tolerance;
	  if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	      goto skip_negative;
	  else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[3]);
		tolerance_removal = t;
	    }
	  else if (sqlite3_value_type (argv[3]) == SQLITE_FLOAT)
	      tolerance_removal = sqlite3_value_double (argv[3]);
	  else
	      goto invalid_arg;
	  if (tolerance_removal < 0.0)
	      goto negative_tolerance;
	skip_negative:
	  if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	      iterate = sqlite3_value_int (argv[4]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Geometry */
    geom =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!geom)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    g2 = gaiaTopoSnap (accessor, geom, tolerance_snap, tolerance_removal,
		       iterate);
    gaiaFreeGeomColl (geom);
    if (g2 == NULL)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (g2, &p_blob, &n_bytes, gpkg_mode, tiny_point);
    gaiaFreeGeomColl (g2);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
check_input_geo_table (sqlite3 * sqlite, const char *db_prefix,
		       const char *table, const char *column, char **xtable,
		       char **xcolumn, int *srid, int *family, int *dims)
{
/* checking if an input GeoTable do really exist */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int len;
    int count = 0;
    char *xx_table = NULL;
    char *xx_column = NULL;
    char *ztable;
    int xtype;
    int xfamily;
    int xdims;
    int xsrid;

    *xtable = NULL;
    *xcolumn = NULL;
    *srid = -1;
    *dims = GAIA_XY;

/* querying GEOMETRY_COLUMNS */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    if (column == NULL)
	sql =
	    sqlite3_mprintf
	    ("SELECT f_table_name, f_geometry_column, geometry_type, srid "
	     "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
	     xprefix, table);
    else
	sql =
	    sqlite3_mprintf
	    ("SELECT f_table_name, f_geometry_column, geometry_type, srid "
	     "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) AND "
	     "Lower(f_geometry_column) = Lower(%Q)", xprefix, table, column);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *table_name = results[(i * columns) + 0];
	  const char *column_name = results[(i * columns) + 1];
	  xtype = atoi (results[(i * columns) + 2]);
	  xsrid = atoi (results[(i * columns) + 3]);
	  len = strlen (table_name);
	  if (xx_table != NULL)
	      free (xx_table);
	  xx_table = malloc (len + 1);
	  strcpy (xx_table, table_name);
	  len = strlen (column_name);
	  if (xx_column != NULL)
	      free (xx_column);
	  xx_column = malloc (len + 1);
	  strcpy (xx_column, column_name);
	  count++;
      }
    sqlite3_free_table (results);

    if (count != 1)
      {
	  if (xx_table != NULL)
	      free (xx_table);
	  if (xx_column != NULL)
	      free (xx_column);
	  return 0;
      }

/* testing if the GeoTable do really exist */
    count = 0;
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    ztable = gaiaDoubleQuotedSql (xx_table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, ztable);
    free (xprefix);
    free (ztable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *column_name = results[(i * columns) + 1];
	  if (strcasecmp (column_name, xx_column) == 0)
	      count++;
      }
    sqlite3_free_table (results);

    if (count != 1)
      {
	  if (xx_table != NULL)
	      free (xx_table);
	  if (xx_column != NULL)
	      free (xx_column);
	  return 0;
      }

    switch (xtype)
      {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
	  xdims = GAIA_XY;
	  break;
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005:
      case 1006:
      case 1007:
	  xdims = GAIA_XY_Z;
	  break;
      case 2000:
      case 2001:
      case 2002:
      case 2003:
      case 2004:
      case 2005:
      case 2006:
      case 2007:
	  xdims = GAIA_XY_M;
	  break;
      case 3000:
      case 3001:
      case 3002:
      case 3003:
      case 3004:
      case 3005:
      case 3006:
      case 3007:
	  xdims = GAIA_XY_Z_M;
	  break;
      };
    switch (xtype)
      {
      case 1:
      case 1001:
      case 2001:
      case 3001:
      case 4:
      case 1004:
      case 2004:
      case 3004:
	  xfamily = GAIA_TYPE_POINT;
	  break;
      case 2:
      case 1002:
      case 2002:
      case 3002:
      case 5:
      case 1005:
      case 2005:
      case 3005:
	  xfamily = GAIA_TYPE_LINESTRING;
	  break;
      case 3:
      case 1003:
      case 2003:
      case 3003:
      case 6:
      case 1006:
      case 2006:
      case 3006:
	  xfamily = GAIA_TYPE_POLYGON;
	  break;
      default:
	  xfamily = GAIA_TYPE_NONE;
	  break;
      };
    *xtable = xx_table;
    *xcolumn = xx_column;
    *srid = xsrid;
    *family = xfamily;
    *dims = xdims;
    return 1;
}

static int
check_output_geo_table (sqlite3 * sqlite, const char *table)
{
/* checking if an output GeoTable do already exist */
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    int count = 0;
    char *ztable;

/* querying GEOMETRY_COLUMNS */
    sql =
	sqlite3_mprintf
	("SELECT f_table_name, f_geometry_column "
	 "FROM MAIN.geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
	 table);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);

    if (count != 0)
	return 0;

/* testing if the Table already exist */
    count = 0;
    ztable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", ztable);
    free (ztable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);

    if (count != 0)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE int
gaia_check_output_table (const void *handle, const char *table)
{
/* checking if an output Table do already exist */
    sqlite3 *sqlite = (sqlite3 *) handle;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    int count = 0;
    char *ztable;

/* testing if the Table already exist */
    ztable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA MAIN.table_info(\"%s\")", ztable);
    free (ztable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);

    if (count != 0)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_SnappedGeoTable (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_SnappedGeoTable ( text topology-name, text db-prefix, text table,
/                           text column, text outtable, int iterate )
/ TopoGeo_SnappedGeoTable ( text topology-name, text db-prefix, text table,
/                           text column, text outtable, double tolerance_snap,
/                           double tolerance_removal, int iterate )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *msg;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    const char *outtable;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int iterate;
    double tolerance_snap = -1;
    double tolerance_removal = -1;
    int srid;
    int family;
    int dims;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	outtable = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (argc == 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
	      iterate = sqlite3_value_int (argv[5]);
	  else
	      goto invalid_arg;
      }
    if (argc >= 8)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[5]);
		tolerance_snap = t;
	    }
	  else if (sqlite3_value_type (argv[5]) == SQLITE_FLOAT)
	      tolerance_snap = sqlite3_value_double (argv[5]);
	  else
	      goto invalid_arg;
	  if (tolerance_snap < 0.0)
	      goto negative_tolerance;
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      goto negative_skip;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[6]);
		tolerance_removal = t;
	    }
	  else if (sqlite3_value_type (argv[6]) == SQLITE_FLOAT)
	      tolerance_removal = sqlite3_value_double (argv[6]);
	  else
	      goto invalid_arg;
	  if (tolerance_removal < 0.0)
	      goto negative_tolerance;
	negative_skip:
	  if (sqlite3_value_type (argv[7]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
	      iterate = sqlite3_value_int (argv[7]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &family,
	 &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

/* checking the output GeoTable */
    if (!check_output_geo_table (sqlite, outtable))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_SnappedGeoTable (accessor, db_prefix, xtable, xcolumn,
				     outtable, tolerance_snap,
				     tolerance_removal, iterate);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xtable);
    free (xcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid input GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "TopoGeo_SnappedGeoTable: output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg =
	"SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_SubdivideLines (const void *xcontext, int argc,
				const void *xargv)
{
/* SQL function:
/ TopoGeo_SubdivideLines ( Geometry geom, int line_max_points )
/ TopoGeo_SubdivideLines ( Geometry geom, int line_max_points,
/                          double max_length )
/
/ returns: a MultiLinestring 
/ raises an exception on failure
*/
    const char *msg;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr result;
    int line_max_points = -1;
    double max_length = -1.0;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
	  n_bytes = sqlite3_value_bytes (argv[0]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  line_max_points = sqlite3_value_int (argv[1]);
	  if (line_max_points < 2)
	      goto illegal_max_points;
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      ;
	  else
	    {
		if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[2]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_int (argv[2]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }

/* attempting to get a Geometry */
    geom =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!geom)
	goto invalid_geom;

/* splitting the geometry */
    result = gaiaTopoGeo_SubdivideLines (geom, line_max_points, max_length);
    gaiaFreeGeomColl (geom);
    if (result == NULL)
	goto invalid_geom;
    gaiaToSpatiaLiteBlobWkbEx2 (result, &p_blob, &n_bytes, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (result);
    if (p_blob == NULL)
	goto invalid_geom;
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    msg = "SQL/MM Spatial exception - invalid Geometry.";
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_FromGeoTable (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column )
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points )
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points, double max_length )
/ TopoGeo_FromGeoTable ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points, double max_length, 
/                        double tolerance )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int family;
    int dims;
    int line_max_points = -1;
    double max_length = -1.0;
    double tolerance = -1;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	    {
		line_max_points = sqlite3_value_int (argv[4]);
		if (line_max_points < 2)
		    goto illegal_max_points;
	    }
	  else
	      goto invalid_arg;
      }
    if (argc >= 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      ;
	  else
	    {
		if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[5]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[5]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[5]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[6]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[6]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[6]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &family,
	 &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_FromGeoTable (accessor, db_prefix, xtable, xcolumn,
				  tolerance, line_max_points, max_length);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xtable);
    free (xcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid input GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg =
	"SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_FromGeoTableNoFace (const void *xcontext, int argc,
				    const void *xargv)
{
/* SQL function:
/ TopoGeo_FromGeoTableNoFace ( text topology-name, text db-prefix, text table,
/                        text column )
/ TopoGeo_FromGeoTableNoFace ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points )
/ TopoGeo_FromGeoTableNoFace ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points, double max_length )
/ TopoGeo_FromGeoTableNoFace ( text topology-name, text db-prefix, text table,
/                        text column, int line_max_points, double max_length, 
/                        double tolerance )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int family;
    int dims;
    int line_max_points = -1;
    double max_length = -1.0;
    double tolerance = -1;
    struct gaia_topology *topo;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	    {
		line_max_points = sqlite3_value_int (argv[4]);
		if (line_max_points < 2)
		    goto illegal_max_points;
	    }
	  else
	      goto invalid_arg;
      }
    if (argc >= 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      ;
	  else
	    {
		if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[5]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[5]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[5]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[6]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[6]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[6]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    topo = (struct gaia_topology *) accessor;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &family,
	 &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);

/* removing any existing Face except the Universal one */
    if (kill_all_existing_faces (sqlite, topo->topology_name) == 0)
      {
	  msg = "TopoGeo_FromGeoTableNoFace: unable to remove existing Faces";
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }

    ret =
	gaiaTopoGeo_FromGeoTableNoFace (accessor, db_prefix, xtable, xcolumn,
					tolerance, line_max_points, max_length);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xtable);
    free (xcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid input GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg =
	"SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
create_dustbin_table (sqlite3 * sqlite, const char *db_prefix,
		      const char *table, const char *dustbin_table)
{
/* attempting to create a dustbin table */
    char *xprefix;
    char *xtable;
    char *sql;
    char *prev_sql;
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;
    struct pk_struct *pk_dictionary = NULL;
    struct pk_item *pI;

/* checking if the target table already exists */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".sqlite_master WHERE Lower(name) = Lower(%Q)",
	 xprefix, dustbin_table);
    free (xprefix);
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
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
      {
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: dustbin-table \"%s\" already exists\n",
	       dustbin_table);
	  return 0;
      }

/* identifying all Primary Key columns */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    pk_dictionary = create_pk_dictionary ();
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 1];
		const char *type = results[(i * columns) + 2];
		int notnull = atoi (results[(i * columns) + 3]);
		int pk = atoi (results[(i * columns) + 5]);
		if (pk > 0)
		    add_pk_column (pk_dictionary, name, type, notnull, pk);
	    }
      }
    sqlite3_free_table (results);
    if (pk_dictionary->count <= 0)
      {
	  free_pk_dictionary (pk_dictionary);
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: the input table \"%s\" has no Primary Key\n",
	       table);
	  return 0;
      }

/* going to create the dustbin table */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (dustbin_table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\".\"%s\" (\n", xprefix, xtable);
    free (xprefix);
    free (xtable);
    prev_sql = sql;
    pI = pk_dictionary->first;
    while (pI != NULL)
      {
	  char *xcolumn = gaiaDoubleQuotedSql (pI->name);
	  if (pI->notnull)
	      sql =
		  sqlite3_mprintf ("%s\t\"%s\" %s NOT NULL,\n", prev_sql,
				   xcolumn, pI->type);
	  else
	      sql =
		  sqlite3_mprintf ("%s\t\"%s\" %s,\n", prev_sql, xcolumn,
				   pI->type);
	  free (xcolumn);
	  sqlite3_free (prev_sql);
	  prev_sql = sql;
	  pI = pI->next;
      }
    xprefix = sqlite3_mprintf ("pk_%s", dustbin_table);
    xtable = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s\tmessage TEXT,\n\ttolerance DOUBLE NOT NULL,\n"
			 "\tfailing_geometry BLOB\n,\tCONSTRAINT \"%s\" PRIMARY KEY (",
			 prev_sql, xtable);
    sqlite3_free (prev_sql);
    free (xtable);
    prev_sql = sql;
    for (i = 1; i <= pk_dictionary->count; i++)
      {
	  pI = pk_dictionary->first;
	  while (pI != NULL)
	    {
		if (pI->pk == i)
		  {
		      char *xcolumn = gaiaDoubleQuotedSql (pI->name);
		      if (i == 1)
			  sql = sqlite3_mprintf ("%s\"%s\"", prev_sql, xcolumn);
		      else
			  sql =
			      sqlite3_mprintf ("%s, \"%s\"", prev_sql, xcolumn);
		      sqlite3_free (prev_sql);
		      free (xcolumn);
		      prev_sql = sql;
		  }
		pI = pI->next;
	    }
      }
    sql = sqlite3_mprintf ("%s))", prev_sql);
    sqlite3_free (prev_sql);
    free_pk_dictionary (pk_dictionary);

    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: unable to create dustbin-table \"%s\": %s\n",
	       dustbin_table, err_msg);
	  sqlite3_free (err_msg);
	  return 0;
      }
    return 1;
}

static int
create_dustbin_view (sqlite3 * sqlite, const char *db_prefix, const char *table,
		     const char *column, const char *dustbin_table,
		     const char *dustbin_view, char **sql_in, char **sql_out,
		     char **sql_in2)
{
/* attempting to create a dustbin view */
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev_sql;
    char *sql2;
    int ret;
    char *err_msg = NULL;
    int i;
    char **results;
    int rows;
    int columns;
    const char *value;
    int error = 0;
    struct pk_struct *pk_dictionary = NULL;
    struct pk_item *pI;
    int first;

    *sql_in = NULL;
    *sql_out = NULL;
    *sql_in2 = NULL;
/* checking if the target view already exists */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".sqlite_master WHERE Lower(name) = Lower(%Q)",
	 xprefix, dustbin_view);
    free (xprefix);
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
		value = results[(i * columns) + 0];
		if (atoi (value) != 0)
		    error = 1;
	    }
      }
    sqlite3_free_table (results);
    if (error)
	return 0;

/* identifying all main table's columns */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    pk_dictionary = create_pk_dictionary ();
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *name = results[(i * columns) + 1];
		const char *type = results[(i * columns) + 2];
		int notnull = atoi (results[(i * columns) + 3]);
		int pk = atoi (results[(i * columns) + 5]);
		add_pk_column (pk_dictionary, name, type, notnull, pk);
	    }
      }
    sqlite3_free_table (results);
    if (pk_dictionary->count <= 0)
      {
	  free_pk_dictionary (pk_dictionary);
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: unable to retrieve \"%s\" columns\n",
	       table);
	  return 0;
      }

/* going to create the dustbin view */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (dustbin_view);
    sql = sqlite3_mprintf ("CREATE VIEW \"%s\".\"%s\" AS\n"
			   "SELECT a.ROWID AS rowid", xprefix, xtable);
    free (xprefix);
    free (xtable);
    prev_sql = sql;
    pI = pk_dictionary->first;
    while (pI != NULL)
      {
	  char *xcolumn = gaiaDoubleQuotedSql (pI->name);
	  sql =
	      sqlite3_mprintf ("%s, a.\"%s\" AS \"%s\"", prev_sql, xcolumn,
			       xcolumn);
	  free (xcolumn);
	  sqlite3_free (prev_sql);
	  prev_sql = sql;
	  pI = pI->next;
      }
    xtable = gaiaDoubleQuotedSql (table);
    xprefix = gaiaDoubleQuotedSql (dustbin_table);
    sql =
	sqlite3_mprintf
	("%s, b.message AS message, b.tolerance AS tolerance, "
	 "b.failing_geometry AS failing_geometry "
	 "FROM \"%s\" AS a, \"%s\" AS b\nWHERE ", prev_sql, xtable, xprefix);
    sqlite3_free (prev_sql);
    free (xtable);
    free (xprefix);
    prev_sql = sql;
    pI = pk_dictionary->first;
    first = 1;
    while (pI != NULL)
      {
	  if (pI->pk > 0)
	    {
		char *xcolumn = gaiaDoubleQuotedSql (pI->name);
		if (first)
		    sql =
			sqlite3_mprintf ("%sa.\"%s\" = b.\"%s\"", prev_sql,
					 xcolumn, xcolumn);
		else
		    sql =
			sqlite3_mprintf ("%s AND a.\"%s\" = b.\"%s\"", prev_sql,
					 xcolumn, xcolumn);
		first = 0;
		sqlite3_free (prev_sql);
		free (xcolumn);
		prev_sql = sql;
	    }
	  pI = pI->next;
      }
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: unable to create dustbin-view \"%s\": %s\n",
	       dustbin_table, err_msg);
	  sqlite3_free (err_msg);
	  free_pk_dictionary (pk_dictionary);
	  return 0;
      }

/* registering the Spatial View */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("INSERT INTO \"%s\".views_geometry_columns (view_name, "
	 "view_geometry, view_rowid, f_table_name, f_geometry_column, read_only) "
	 "VALUES (%Q, %Q, 'rowid',  %Q, %Q, 1)", xprefix, dustbin_view, column,
	 table, column);
    free (xprefix);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e
	      ("TopoGeo_FromGeoTableExt: unable to register the dustbin-view \"%s\": %s\n",
	       dustbin_table, err_msg);
	  sqlite3_free (err_msg);
	  free_pk_dictionary (pk_dictionary);
	  return 0;
      }

/* constructing the input SQL statement */
    sql = sqlite3_mprintf ("SELECT ROWID");
    prev_sql = sql;
    pI = pk_dictionary->first;
    while (pI != NULL)
      {
	  if (pI->pk > 0)
	    {
		char *xcolumn = gaiaDoubleQuotedSql (pI->name);
		sql = sqlite3_mprintf ("%s, \"%s\"", prev_sql, xcolumn);
		sqlite3_free (prev_sql);
		free (xcolumn);
		prev_sql = sql;
	    }
	  pI = pI->next;
      }
    xcolumn = gaiaDoubleQuotedSql (column);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("%s, \"%s\" FROM \"%s\".\"%s\" "
			 "WHERE ROWID > ? ORDER BY ROWID", prev_sql, xcolumn,
			 xprefix, xtable);
    sql2 =
	sqlite3_mprintf ("%s, \"%s\" FROM \"%s\".\"%s\" WHERE ROWID = ?",
			 prev_sql, xcolumn, xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev_sql);
    *sql_in = sql;
    *sql_in2 = sql2;

/* constructing the output SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (dustbin_table);
    sql = sqlite3_mprintf ("INSERT INTO \"%s\".\"%s\" (", xprefix, xtable);
    prev_sql = sql;
    free (xprefix);
    free (xtable);
    pI = pk_dictionary->first;
    first = 1;
    while (pI != NULL)
      {
	  if (pI->pk > 0)
	    {
		char *xcolumn = gaiaDoubleQuotedSql (pI->name);
		if (first)
		    sql = sqlite3_mprintf ("%s\"%s\"", prev_sql, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev_sql, xcolumn);
		first = 0;
		sqlite3_free (prev_sql);
		free (xcolumn);
		prev_sql = sql;
	    }
	  pI = pI->next;
      }
    sql =
	sqlite3_mprintf ("%s, message, tolerance, failing_geometry) VALUES (",
			 prev_sql);
    sqlite3_free (prev_sql);
    prev_sql = sql;
    pI = pk_dictionary->first;
    first = 1;
    while (pI != NULL)
      {
	  if (pI->pk > 0)
	    {
		if (first)
		    sql = sqlite3_mprintf ("%s?", prev_sql);
		else
		    sql = sqlite3_mprintf ("%s, ?", prev_sql);
		first = 0;
		sqlite3_free (prev_sql);
		prev_sql = sql;
	    }
	  pI = pI->next;
      }
    sql = sqlite3_mprintf ("%s, ?, ?, ?)", prev_sql);
    sqlite3_free (prev_sql);
    *sql_out = sql;

    free_pk_dictionary (pk_dictionary);
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_FromGeoTableExt (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_FromGeoTableExt ( text topology-name, text db-prefix, text table,
/                           text column, text dustbin-table, text dustbin-view )
/ TopoGeo_FromGeoTableExt ( text topology-name, text db-prefix, text table,
/                           text column, text dustbin-table, text dustbin-view,
/                           int line_max_points )
/ TopoGeo_FromGeoTableExt ( text topology-name, text db-prefix, text table,
/                           text column, text dustbin-table, text dustbin-view,
/                           int line_max_points, double max_length )
/ TopoGeo_FromGeoTableExt ( text topology-name, text db-prefix, text table,
/                           text column, text dustbin-table, text dustbin-view,
/                           int line_max_points, double max_length , 
/                           double tolerance )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int family;
    int dims;
    const char *dustbin_table;
    const char *dustbin_view;
    int line_max_points = -1;
    double max_length = -1.0;
    double tolerance = -1;
    char *sql_in = NULL;
    char *sql_out = NULL;
    char *sql_in2 = NULL;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	dustbin_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[5]) == SQLITE_TEXT)
	dustbin_view = (const char *) sqlite3_value_text (argv[5]);
    else
	goto invalid_arg;
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		line_max_points = sqlite3_value_int (argv[6]);
		if (line_max_points < 2)
		    goto illegal_max_points;
	    }
	  else
	      goto invalid_arg;
      }
    if (argc >= 8)
      {
	  if (sqlite3_value_type (argv[7]) == SQLITE_NULL)
	      ;
	  else
	    {
		if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[7]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[7]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[7]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }
    if (argc >= 9)
      {
	  if (sqlite3_value_type (argv[8]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[8]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[8]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[8]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &family,
	 &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

/* attempting to create the dustbin table and view */
    start_topo_savepoint (sqlite, cache);
    if (!create_dustbin_table (sqlite, db_prefix, xtable, dustbin_table))
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_dustbin_table;
      }
    if (!create_dustbin_view
	(sqlite, db_prefix, xtable, xcolumn, dustbin_table, dustbin_view,
	 &sql_in, &sql_out, &sql_in2))
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_dustbin_view;
      }
    release_topo_savepoint (sqlite, cache);

    ret =
	gaiaTopoGeo_FromGeoTableExtended (accessor, sql_in, sql_out, sql_in2,
					  tolerance, line_max_points,
					  max_length);
    free (xtable);
    free (xcolumn);
    sqlite3_free (sql_in);
    sqlite3_free (sql_out);
    sqlite3_free (sql_in2);
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid input GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg =
	"SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_dustbin_table:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    msg = "SQL/MM Spatial exception - unable to create the dustbin table.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_dustbin_view:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - unable to create the dustbin view.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_FromGeoTableNoFaceExt (const void *xcontext, int argc,
				       const void *xargv)
{
/* SQL function:
/ TopoGeo_FromGeoTableNoFaceExt ( text topology-name, text db-prefix, 
/                                 text table, text column, text dustbin-table,
/                                 text dustbin-view )
/ TopoGeo_FromGeoTableNoFaceExt ( text topology-name, text db-prefix, 
/                                 text table, text column, text dustbin-table, 
/                                 text dustbin-view, int line_max_points )
/ TopoGeo_FromGeoTableNoFaceExt ( text topology-name, text db-prefix, 
/                                 text table, text column, text dustbin-table, 
/                                 text dustbin-view, int line_max_points, 
/                                 double max_length )
/ TopoGeo_FromGeoTableNoFaceExt ( text topology-name, text db-prefix, 
/                                 text table, text column, text dustbin-table, 
/                                 text dustbin-view, int line_max_points, 
/                                 double max_length, double tolerance )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int family;
    int dims;
    const char *dustbin_table;
    const char *dustbin_view;
    int line_max_points = -1;
    double max_length = -1.0;
    double tolerance = -1;
    char *sql_in = NULL;
    char *sql_out = NULL;
    char *sql_in2 = NULL;
    struct gaia_topology *topo;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	dustbin_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[5]) == SQLITE_TEXT)
	dustbin_view = (const char *) sqlite3_value_text (argv[5]);
    else
	goto invalid_arg;
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	    {
		line_max_points = sqlite3_value_int (argv[6]);
		if (line_max_points < 2)
		    goto illegal_max_points;
	    }
	  else
	      goto invalid_arg;
      }
    if (argc >= 8)
      {
	  if (sqlite3_value_type (argv[7]) == SQLITE_NULL)
	      ;
	  else
	    {
		if (sqlite3_value_type (argv[7]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[7]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[7]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[7]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }
    if (argc >= 9)
      {
	  if (sqlite3_value_type (argv[8]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[8]) == SQLITE_INTEGER)
	    {
		int t = sqlite3_value_int (argv[8]);
		tolerance = t;
	    }
	  else if (sqlite3_value_type (argv[8]) == SQLITE_FLOAT)
	      tolerance = sqlite3_value_double (argv[8]);
	  else
	      goto invalid_arg;
	  if (tolerance < 0.0)
	      goto negative_tolerance;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    topo = (struct gaia_topology *) accessor;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input GeoTable */
    if (!check_input_geo_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &family,
	 &dims))
	goto no_input;
    if (!check_matching_srid_dims (accessor, srid, dims))
	goto invalid_geom;

    start_topo_savepoint (sqlite, cache);

/* removing any existing Face except the Universal one */
    if (kill_all_existing_faces (sqlite, topo->topology_name) == 0)
      {
	  msg =
	      "TopoGeo_FromGeoTableNoFaceExt: unable to remove existing Faces";
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }

/* attempting to create the dustbin table and view */
    if (!create_dustbin_table (sqlite, db_prefix, xtable, dustbin_table))
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_dustbin_table;
      }
    if (!create_dustbin_view
	(sqlite, db_prefix, xtable, xcolumn, dustbin_table, dustbin_view,
	 &sql_in, &sql_out, &sql_in2))
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_dustbin_view;
      }
    release_topo_savepoint (sqlite, cache);

    ret =
	gaiaTopoGeo_FromGeoTableNoFaceExtended (accessor, sql_in, sql_out,
						sql_in2, tolerance,
						line_max_points, max_length);
    free (xtable);
    free (xcolumn);
    sqlite3_free (sql_in);
    sqlite3_free (sql_out);
    sqlite3_free (sql_in2);
    sqlite3_result_int (context, ret);
    return;

  no_topo:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid input GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg =
	"SQL/MM Spatial exception - invalid GeoTable (mismatching SRID or dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_dustbin_table:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    msg = "SQL/MM Spatial exception - unable to create the dustbin table.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_dustbin_view:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - unable to create the dustbin view.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  negative_tolerance:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - illegal negative tolerance.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    if (sql_in != NULL)
	sqlite3_free (sql_in);
    if (sql_out != NULL)
	sqlite3_free (sql_out);
    if (sql_in2 != NULL)
	sqlite3_free (sql_in2);
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
check_matching_srid (GaiaTopologyAccessorPtr accessor, int srid)
{
/* checking for matching SRID */
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo->srid != srid)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE int
gaia_check_reference_geo_table (const void *handle, const char *db_prefix,
				const char *table, const char *column,
				char **xtable, char **xcolumn, int *srid,
				int *family)
{
    sqlite3 *sqlite = (sqlite3 *) handle;
    int dims;
    return check_input_geo_table (sqlite, db_prefix, table, column, xtable,
				  xcolumn, srid, family, &dims);
}

static int
check_reference_table (sqlite3 * sqlite, const char *db_prefix,
		       const char *table)
{
/* checking if an input GeoTable do really exist */

    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *sql;
    char *xprefix;
    int count = 0;
    char *xtable;

/* testing if the Table do really exist */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  return 0;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);

    if (count < 1)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_ToGeoTable (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_ToGeoTable ( text topology-name, text db-prefix, text ref_table,
/                      text ref_column, text out_table )
/ TopoGeo_ToGeoTable ( text topology-name, text db-prefix, text ref_table,
/                      text ref_column, text out_table, int with-spatial-index )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    int with_spatial_index = 0;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	ref_column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	ref_column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (argc >= 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
	      with_spatial_index = sqlite3_value_int (argv[5]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the reference GeoTable */
    if (!gaia_check_reference_geo_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &family))
	goto no_reference;
    if (!check_matching_srid (accessor, srid))
	goto invalid_geom;

/* checking the output GeoTable */
    if (!check_output_geo_table (sqlite, out_table))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_ToGeoTable (accessor, db_prefix, xreftable, xrefcolumn,
				out_table, with_spatial_index);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_ToGeoTable: invalid reference GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_ToGeoTable: output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_PolyFacesList (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_PolyFacesList ( text topology-name, text db-prefix, text ref_table,
/                         text ref_column, text out_table )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	ref_column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	ref_column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the reference GeoTable */
    if (!gaia_check_reference_geo_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &family))
	goto no_reference;
    if (!check_matching_srid (accessor, srid))
	goto invalid_geom;
    if (family != GAIA_TYPE_POLYGON)
	goto not_polygon;

/* checking the output Table */
    if (!gaia_check_output_table (sqlite, out_table))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_PolyFacesList (accessor, db_prefix, xreftable, xrefcolumn,
				   out_table);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_PolyFacesList: invalid reference GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_PolyFacesList: output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  not_polygon:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (not of the [MULTI]POLYGON type).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_LineEdgesList (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_LineEdgesList ( text topology-name, text db-prefix, text ref_table,
/                         text ref_column, text out_table )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	ref_column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	ref_column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the reference GeoTable */
    if (!gaia_check_reference_geo_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &family))
	goto no_reference;
    if (!check_matching_srid (accessor, srid))
	goto invalid_geom;
    if (family != GAIA_TYPE_LINESTRING)
	goto not_linestring;

/* checking the output Table */
    if (!gaia_check_output_table (sqlite, out_table))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_LineEdgesList (accessor, db_prefix, xreftable, xrefcolumn,
				   out_table);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_LineEdgesList: invalid reference GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_LineEdgesList: output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  not_linestring:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (not of the [MULTI]LINESTRING type).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_ToGeoTableGeneralize (const void *xcontext, int argc,
				      const void *xargv)
{
/* SQL function:
/ TopoGeo_ToGeoTableGeneralize ( text topology-name, text db-prefix,
/                                text ref_table, text ref_column,
/                                text out_table, double tolerance )
/ TopoGeo_ToGeoTableGeneralize ( text topology-name, text db-prefix,
/                                text ref_table, text ref_column,
/                                text out_table, double tolerance,
/                                int with-spatial-index )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    double tolerance = 0.0;
    int with_spatial_index = 0;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	ref_column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	ref_column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
      {
	  int val = sqlite3_value_int (argv[5]);
	  tolerance = val;
      }
    else if (sqlite3_value_type (argv[5]) == SQLITE_FLOAT)
	tolerance = sqlite3_value_double (argv[5]);
    else
	goto invalid_arg;
    if (argc >= 7)
      {
	  if (sqlite3_value_type (argv[6]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[6]) == SQLITE_INTEGER)
	      with_spatial_index = sqlite3_value_int (argv[6]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the reference GeoTable */
    if (!gaia_check_reference_geo_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &family))
	goto no_reference;
    if (!check_matching_srid (accessor, srid))
	goto invalid_geom;

/* checking the output GeoTable */
    if (!check_output_geo_table (sqlite, out_table))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_ToGeoTableGeneralize (accessor, db_prefix, xreftable,
					  xrefcolumn, out_table, tolerance,
					  with_spatial_index);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_ToGeoTableGeneralize: invalid reference GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_ToGeoTableGeneralize: output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_RemoveSmallFaces (const void *xcontext, int argc,
				  const void *xargv)
{
/* SQL function:
/ TopoGeo_RemoveSmallFaces ( text topology-name, double min-circularity )
/ TopoGeo_RemoveSmallFaces ( text topology-name, double min-circularity,
/                            double min-area )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    double min_circularity;
    double min_area = 0.0;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  int val = sqlite3_value_int (argv[1]);
	  min_circularity = val;
      }
    else if (sqlite3_value_type (argv[1]) == SQLITE_FLOAT)
	min_circularity = sqlite3_value_double (argv[1]);
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	    {
		int val = sqlite3_value_int (argv[2]);
		min_area = val;
	    }
	  else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	      min_area = sqlite3_value_double (argv[2]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_RemoveSmallFaces (accessor, min_circularity, min_area);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_RemoveDanglingEdges (const void *xcontext, int argc,
				     const void *xargv)
{
/* SQL function:
/ TopoGeo_RemoveDanglingEdges ( text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_RemoveDanglingEdges (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_RemoveDanglingNodes (const void *xcontext, int argc,
				     const void *xargv)
{
/* SQL function:
/ TopoGeo_RemoveDanglingNodes ( text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_RemoveDanglingNodes (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_NewEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_NewEdgeHeal ( text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    if (test_inconsistent_topology (accessor) != 0)
	goto inconsistent_topology;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_NewEdgeHeal (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  inconsistent_topology:
    msg =
	"TopoGeo_NewEdgeHeal exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_ModEdgeHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_ModEdgeHeal ( text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    if (test_inconsistent_topology (accessor) != 0)
	goto inconsistent_topology;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_ModEdgeHeal (accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  inconsistent_topology:
    msg =
	"TopoGeo_ModEdgeHeal exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_NewEdgesSplit (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_NewEdgesSplit ( text topology-name, int line_max_points )
/ TopoGeo_NewEdgesSplit ( text topology-name, int line_max_points, 
/                         double max_length )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    int line_max_points = -1;
    double max_length = -1.0;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  line_max_points = sqlite3_value_int (argv[1]);
	  if (line_max_points < 2)
	      goto illegal_max_points;
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else
	    {
		if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[2]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[2]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    if (test_inconsistent_topology (accessor) != 0)
	goto inconsistent_topology;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_NewEdgesSplit (accessor, line_max_points, max_length);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  inconsistent_topology:
    msg =
	"TopoGeo_NewEdgesSplit exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_ModEdgeSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_ModEdgeSplit ( text topology-name, int line_max_points )
/ TopoGeo_ModEdgeSplit ( text topology-name, int line_max_points, 
/                        double max_length )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    int line_max_points = -1;
    double max_length = -1.0;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
      {
	  line_max_points = sqlite3_value_int (argv[1]);
	  if (line_max_points < 2)
	      goto illegal_max_points;
      }
    else
	goto invalid_arg;
    if (argc >= 3)
      {
	  if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	      goto null_arg;
	  else
	    {
		if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
		  {
		      int max = sqlite3_value_int (argv[2]);
		      max_length = max;
		  }
		else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
		    max_length = sqlite3_value_double (argv[2]);
		else
		    goto invalid_arg;
		if (max_length <= 0.0)
		    goto nonpositive_max_length;
	    }
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    if (test_inconsistent_topology (accessor) != 0)
	goto inconsistent_topology;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_ModEdgeSplit (accessor, line_max_points, max_length);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  inconsistent_topology:
    msg =
	"TopoGeo_ModEdgeSplit exception - inconsisten Topology; try executing TopoGeo_Polygonize to recover.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  illegal_max_points:
    msg = "SQL/MM Spatial exception - max_points should be >= 2.";
    sqlite3_result_error (context, msg, -1);
    return;

  nonpositive_max_length:
    msg = "SQL/MM Spatial exception - max_length should be > 0.0.";
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
do_clone_face (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning FACE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_face", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT face_id, mbr FROM \"%s\".\"%s\" WHERE face_id <> 0",
	 xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"face\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_face", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (face_id, mbr) VALUES (?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"face\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
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
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 2);
		else if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 2,
				       sqlite3_column_blob (stmt_in, 1),
				       sqlite3_column_bytes (stmt_in, 1),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"face\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"face\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"face\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_node (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning NODE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_node", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT node_id, containing_face, geom FROM \"%s\".\"%s\"", xprefix,
	 xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"node\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_node", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (node_id, containing_face, geom) "
	 "VALUES (?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"node\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
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
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 2);
		else if (sqlite3_column_type (stmt_in, 1) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 2,
					sqlite3_column_int64 (stmt_in, 1));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 3,
				       sqlite3_column_blob (stmt_in, 2),
				       sqlite3_column_bytes (stmt_in, 2),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"node\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"node\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"node\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_edge (const char *db_prefix, const char *in_topology_name,
	       struct gaia_topology *topo_out)
{
/* cloning EDGE */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_edge", in_topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT edge_id, start_node, end_node, next_left_edge, "
	 "next_right_edge, left_face, right_face, geom FROM \"%s\".\"%s\"",
	 xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"edge\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_edge", topo_out->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (edge_id, start_node, end_node, "
	 "next_left_edge, next_right_edge, left_face, right_face, geom) "
	 "VALUES (?, ?, ?, ?, ?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (topo_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"edge\" error: \"%s\"",
			sqlite3_errmsg (topo_out->db_handle));
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
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 1,
					sqlite3_column_int64 (stmt_in, 0));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 2,
					sqlite3_column_int64 (stmt_in, 1));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 3,
					sqlite3_column_int64 (stmt_in, 2));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 3) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 4,
					sqlite3_column_int64 (stmt_in, 3));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 4) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 5,
					sqlite3_column_int64 (stmt_in, 4));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 5) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 6,
					sqlite3_column_int64 (stmt_in, 5));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 6) == SQLITE_INTEGER)
		    sqlite3_bind_int64 (stmt_out, 7,
					sqlite3_column_int64 (stmt_in, 6));
		else
		    goto invalid_value;
		if (sqlite3_column_type (stmt_in, 7) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 8,
				       sqlite3_column_blob (stmt_in, 7),
				       sqlite3_column_bytes (stmt_in, 7),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"edge\" step error: \"%s\"",
				    sqlite3_errmsg (topo_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"edge\" step error: %s",
			      sqlite3_errmsg (topo_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"edge\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_topology (const char *db_prefix, const char *in_topology,
		   GaiaTopologyAccessorPtr accessor)
{
/* cloning a full Topology */
    struct gaia_topology *topo_out = (struct gaia_topology *) accessor;

/* cloning FACE */
    if (!do_clone_face (db_prefix, in_topology, topo_out))
	return 0;

/* cloning NODE */
    if (!do_clone_node (db_prefix, in_topology, topo_out))
	return 0;

/* cloning EDGE */
    if (!do_clone_edge (db_prefix, in_topology, topo_out))
	return 0;

    return 1;
}

static char *
gaiaGetAttachedTopology (sqlite3 * handle, const char *db_prefix,
			 const char *topo_name, int *srid, double *tolerance,
			 int *has_z)
{
/* attempting to retrieve the Input Topology for TopoGeo_Clone */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xprefix;
    char *xtopology_name = NULL;
    int xsrid;
    double xtolerance;
    int xhas_z;

/* preparing the SQL query */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT topology_name, srid, tolerance, has_z FROM \"%s\".topologies WHERE "
	 "Lower(topology_name) = Lower(%Q)", xprefix, topo_name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM topologys error: \"%s\"\n",
			sqlite3_errmsg (handle));
	  return NULL;
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
		return NULL;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *srid = xsrid;
	  *tolerance = xtolerance;
	  *has_z = xhas_z;
	  return xtopology_name;
      }

    if (xtopology_name != NULL)
	free (xtopology_name);
    return NULL;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_Clone (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_Clone ( text db-prefix, text in-topology-name, text out-topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *db_prefix = "MAIN";
    const char *in_topo_name;
    const char *out_topo_name;
    char *input_topo_name = NULL;
    int srid;
    double tolerance;
    int has_z;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	in_topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	out_topo_name = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;

/* checking the origin Topology */
    input_topo_name =
	gaiaGetAttachedTopology (sqlite, db_prefix, in_topo_name, &srid,
				 &tolerance, &has_z);
    if (input_topo_name == NULL)
	goto no_topo;

/* attempting to create the destination Topology */
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopologyCreate (sqlite, out_topo_name, srid, tolerance, has_z);
    if (!ret)
      {
	  rollback_topo_savepoint (sqlite, cache);
	  goto no_topo2;
      }

/* attempting to get a Topology Accessor (destination) */
    accessor = gaiaGetTopology (sqlite, cache, out_topo_name);
    if (accessor == NULL)
	goto no_topo2;
    gaiatopo_reset_last_error_msg (accessor);

/* cloning Topology */
    ret = do_clone_topology (db_prefix, input_topo_name, accessor);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  sqlite3_result_error (context, "Clone Topology failure", -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    if (input_topo_name != NULL)
	free (input_topo_name);
    return;

  no_topo:
    if (input_topo_name != NULL)
	free (input_topo_name);
    msg = "SQL/MM Spatial exception - invalid topology name (origin).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_topo2:
    if (input_topo_name != NULL)
	free (input_topo_name);
    msg = "SQL/MM Spatial exception - invalid topology name (destination).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (input_topo_name != NULL)
	free (input_topo_name);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (input_topo_name != NULL)
	free (input_topo_name);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_GetEdgeSeed (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_GetEdgeSeed ( text topology-name, int edge_id )
/
/ returns: a Point (seed) identifying the Edge
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    sqlite3_int64 edge_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	edge_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    geom = gaiaGetEdgeSeed (accessor, edge_id);
    if (geom == NULL)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode, tiny_point);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_GetFaceSeed (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_GetFaceSeed ( text topology-name, int face_id )
/
/ returns: a Point (seed) identifying the Edge
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    sqlite3_int64 face_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaTopologyAccessorPtr accessor = NULL;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	face_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    geom = gaiaGetFaceSeed (accessor, face_id);
    if (geom == NULL)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode, tiny_point);
    gaiaFreeGeomColl (geom);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_DisambiguateSegmentEdges (const void *xcontext, int argc,
					  const void *xargv)
{
/* SQL function:
/ TopoGeo_DisambiguateSegmentEdges ( text topology-name )
/
/ returns: the total number of changed Edges.
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    int changed_edges = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    changed_edges = gaiaTopoGeo_DisambiguateSegmentEdges (accessor);
    if (changed_edges < 0)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (changed_edges < 0)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    sqlite3_result_int (context, changed_edges);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_UpdateSeeds (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoGeo_UpdateSeeds ( text topology-name )
/ TopoGeo_UpdateSeeds ( text topology-name, int incremental_mode )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    int incremental_mode = 1;
    int ret;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (argc >= 2)
      {
	  if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	      incremental_mode = sqlite3_value_int (argv[1]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;

    gaiatopo_reset_last_error_msg (accessor);
    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeoUpdateSeeds (accessor, incremental_mode);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  if (msg != NULL)
	    {
		gaiatopo_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_SnapPointToSeed (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_SnapPointToSeed ( geometry point, text topology-name, double distance )
/
/ returns: a snapped point geometry
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    gaiaGeomCollPtr geom = NULL;
    gaiaGeomCollPtr result = NULL;
    double dist;
    unsigned char *p_blob;
    int n_bytes;
    int invalid = 0;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */

    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
	  n_bytes = sqlite3_value_bytes (argv[0]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int intval = sqlite3_value_int (argv[2]);
	  dist = intval;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	dist = sqlite3_value_double (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Point Geometry */
    geom =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!geom)
	goto invalid_arg;
    if (geom->FirstLinestring != NULL)
	invalid = 1;
    if (geom->FirstPolygon != NULL)
	invalid = 1;
    if (geom->FirstPoint != geom->LastPoint || geom->FirstPoint == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, geom->Srid, geom->DimensionModel))
	goto invalid_geom;

    result = gaiaTopoGeoSnapPointToSeed (accessor, geom, dist);
    if (result == NULL)
      {
	  gaiaFreeGeomColl (geom);
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (result, &p_blob, &n_bytes, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (geom);
    gaiaFreeGeomColl (result);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg =
	"SQL/MM Spatial exception - invalid Point (mismatching SRID od dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_SnapLineToSeed (const void *xcontext, int argc,
				const void *xargv)
{
/* SQL function:
/ TopoGeo_SnapLineToSeed ( geometry line, text topology-name, double distance )
/
/ returns: a snapped linestring geometry
/ raises an exception on failure
*/
    const char *msg;
    const char *topo_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    gaiaGeomCollPtr geom = NULL;
    gaiaGeomCollPtr result = NULL;
    double dist;
    unsigned char *p_blob;
    int n_bytes;
    int invalid = 0;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */

    if (cache != NULL)
      {
	  gpkg_amphibious = cache->gpkg_amphibious_mode;
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    if (sqlite3_value_type (argv[0]) == SQLITE_BLOB)
      {
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[0]);
	  n_bytes = sqlite3_value_bytes (argv[0]);
      }
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
      {
	  int intval = sqlite3_value_int (argv[2]);
	  dist = intval;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_FLOAT)
	dist = sqlite3_value_double (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Linestring Geometry */
    geom =
	gaiaFromSpatiaLiteBlobWkbEx (p_blob, n_bytes, gpkg_mode,
				     gpkg_amphibious);
    if (!geom)
	goto invalid_arg;
    if (geom->FirstPoint != NULL)
	invalid = 1;
    if (geom->FirstPolygon != NULL)
	invalid = 1;
    if (geom->FirstLinestring != geom->LastLinestring
	|| geom->FirstLinestring == NULL)
	invalid = 1;
    if (invalid)
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);
    if (!check_matching_srid_dims (accessor, geom->Srid, geom->DimensionModel))
	goto invalid_geom;

    result = gaiaTopoGeoSnapLinestringToSeed (accessor, geom, dist);
    if (result == NULL)
      {
	  gaiaFreeGeomColl (geom);
	  sqlite3_result_null (context);
	  return;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (result, &p_blob, &n_bytes, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (geom);
    gaiaFreeGeomColl (result);
    if (p_blob == NULL)
	sqlite3_result_null (context);
    else
	sqlite3_result_blob (context, p_blob, n_bytes, free);
    return;

  no_topo:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    if (result != NULL)
	gaiaFreeGeomColl (result);
    msg =
	"SQL/MM Spatial exception - invalid Line (mismatching SRID od dimensions).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

static int
topolayer_exists (GaiaTopologyAccessorPtr accessor, const char *topolayer_name)
{
/* checking if a TopoLayer is already defined */
    char *table;
    char *xtable;
    char *sql;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    struct gaia_topology *topo = (struct gaia_topology *) accessor;
    if (topo == NULL)
	return 0;

    table = sqlite3_mprintf ("%s_topolayers", topo->topology_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM MAIN.\"%s\" WHERE topolayer_name = Lower(%Q)",
	 xtable, topolayer_name);
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

    if (count == 0)
	return 0;
    return 1;
}

static int
check_view (struct gaia_topology *topo, const char *db_prefix,
	    const char *table, const char *column)
{
/* checking a candidate View (or unregistered Table) for valid Geoms */
    char *sql;
    char *xcolumn;
    char *xprefix;
    char *xtable;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int nulls = 0;
    int others = 0;
    int geoms = 0;
    int wrong_srids = 0;

    xcolumn = gaiaDoubleQuotedSql (column);
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\"", xcolumn, xprefix,
			 xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    ret = sqlite3_prepare_v2 (topo->db_handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("TopoGeo_CreateTopoLayer() error: \"%s\"",
			       sqlite3_errmsg (topo->db_handle));
	  gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo, msg);
	  sqlite3_free (msg);
	  goto error;
      }

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
		gaiaGeomCollPtr geom = NULL;
		const unsigned char *blob;
		int blob_sz;
		if (sqlite3_column_type (stmt, 0) == SQLITE_NULL)
		    nulls++;
		else if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt, 0);
		      blob_sz = sqlite3_column_bytes (stmt, 0);
		      geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		      if (geom)
			{
			    if (geom->Srid != topo->srid)
				wrong_srids++;
			    gaiaFreeGeomColl (geom);
			    geoms++;
			}
		      else
			  others++;
		  }
		else
		    others++;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("TopoGeo_CreateTopoLayer step error: %s",
				     sqlite3_errmsg (topo->db_handle));
		gaiatopo_set_last_error_msg ((GaiaTopologyAccessorPtr) topo,
					     msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    sqlite3_finalize (stmt);
    if (geoms == 0)
	return 0;
    if (others != 0)
	return 0;
    if (wrong_srids != 0)
	return 0;
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_CreateTopoLayer (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_CreateTopoLayer ( text topology-name, text db-prefix, text ref_table,
/                           text ref_column, text topolayer_name )
/ TopoGeo_CreateTopoLayer ( text topology-name, text db-prefix, text ref_table,
/                           text ref_column, text topolayer_name, 
/                           boolean is_view )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *topolayer_name;
    int is_view = 0;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	ref_column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	ref_column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[4]) == SQLITE_TEXT)
	topolayer_name = (const char *) sqlite3_value_text (argv[4]);
    else
	goto invalid_arg;
    if (argc >= 6)
      {
	  if (sqlite3_value_type (argv[5]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[5]) == SQLITE_INTEGER)
	      is_view = sqlite3_value_int (argv[5]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

    if (is_view)
      {
	  /* checking a View (or an unregistered geo-table) */
	  struct gaia_topology *topo = (struct gaia_topology *) accessor;
	  if (ref_column == NULL)
	      goto null_view_geom;
	  if (!check_view (topo, db_prefix, ref_table, ref_column))
	      goto invalid_view;
	  xreftable = malloc (strlen (ref_table) + 1);
	  strcpy (xreftable, ref_table);
	  xrefcolumn = malloc (strlen (ref_column) + 1);
	  strcpy (xrefcolumn, ref_column);
      }
    else
      {
	  /* checking the reference GeoTable */
	  if (!gaia_check_reference_geo_table
	      (sqlite, db_prefix, ref_table, ref_column, &xreftable,
	       &xrefcolumn, &srid, &family))
	      goto no_reference;
	  if (!check_matching_srid (accessor, srid))
	      goto invalid_geom;
      }

/* checking the output TopoLayer */
    if (topolayer_exists (accessor, topolayer_name))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_CreateTopoLayer (accessor, db_prefix, xreftable, xrefcolumn,
				     topolayer_name);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_CreateTopoLayer: invalid reference GeoTable.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_view_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"TopoGeo_CreateTopoLayer: IsView requires an explicit Geometry column-name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_view:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_CreateTopoLayer: invalid reference View (invalid Geometry).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"TopoGeo_CreateTopoLayer: a TopoLayer of the same name already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_InitTopoLayer (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoGeo_InitTopoLayer ( text topology-name, text db-prefix, text ref_table,
/                         text topolayer_name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *db_prefix;
    const char *ref_table;
    const char *topolayer_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	ref_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	topolayer_name = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the reference Table */
    if (!check_reference_table (sqlite, db_prefix, ref_table))
	goto no_reference;

/* checking the output TopoLayer */
    if (topolayer_exists (accessor, topolayer_name))
	goto err_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_InitTopoLayer (accessor, db_prefix, ref_table,
				   topolayer_name);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    msg = "TopoGeo_InitTopoLayer: invalid reference Table.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    msg = "TopoGeo_InitTopoLayer: a TopoLayer of the same name already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_RemoveTopoLayer (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_RemoveTopoLayer ( text topology-name, text topolayer_name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *topolayer_name;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topolayer_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the TopoLayer */
    if (!topolayer_exists (accessor, topolayer_name))
	goto err_topolayer;

    start_topo_savepoint (sqlite, cache);
    ret = gaiaTopoGeo_RemoveTopoLayer (accessor, topolayer_name);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_topolayer:
    msg = "TopoGeo_RemoveTopoLayer: not existing TopoLayer.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_ExportTopoLayer (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ TopoGeo_ExportTopoLayer ( text topology-name, text topolayer_name,
/                           text out_table )
/ TopoGeo_ExportTopoLayer ( text topology-name, text topolayer_name,
/                           text out_table, integer with-spatial-index )
/ TopoGeo_ExportTopoLayer ( text topology-name, text topolayer_name,
/                           text out_table, integer with-spatial-index,
/                           integer create-only )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *topolayer_name;
    const char *out_table;
    int with_spatial_index = 0;
    int create_only = 0;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topolayer_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (argc >= 4)
      {
	  if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	      with_spatial_index = sqlite3_value_int (argv[3]);
	  else
	      goto invalid_arg;
      }
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	      goto null_arg;
	  else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	      create_only = sqlite3_value_int (argv[4]);
	  else
	      goto invalid_arg;
      }

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input TopoLayer */
    if (!topolayer_exists (accessor, topolayer_name))
	goto err_topolayer;

/* checking the output GeoTable */
    if (!check_output_geo_table (sqlite, out_table))
	goto invalid_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_ExportTopoLayer (accessor, topolayer_name, out_table,
				     with_spatial_index, create_only);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_output:
    msg = "TopoGeo_ExportTopoLayer: the output GeoTable already exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_topolayer:
    sqlite3_result_error (context,
			  "TopoGeo_ExportTopoLayer: not existing TopoLayer.",
			  -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoGeo_InsertFeatureFromTopoLayer (const void *xcontext, int argc,
					    const void *xargv)
{
/* SQL function:
/ TopoGeo_InsertFeatureFromTopoLayer ( text topology-name, 
/                                      text topolayer_name,
/                                      text out_table, integer fid )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *topo_name;
    const char *topolayer_name;
    const char *out_table;
    sqlite3_int64 fid;
    GaiaTopologyAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topolayer_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	out_table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	fid = sqlite3_value_int64 (argv[3]);
    else
	goto invalid_arg;

/* attempting to get a Topology Accessor */
    accessor = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor == NULL)
	goto no_topo;
    gaiatopo_reset_last_error_msg (accessor);

/* checking the input TopoLayer */
    if (!topolayer_exists (accessor, topolayer_name))
	goto err_topolayer;

/* checking the output GeoTable */
    if (check_output_geo_table (sqlite, out_table))
	goto invalid_output;

    start_topo_savepoint (sqlite, cache);
    ret =
	gaiaTopoGeo_InsertFeatureFromTopoLayer (accessor, topolayer_name,
						out_table, fid);
    if (!ret)
	rollback_topo_savepoint (sqlite, cache);
    else
	release_topo_savepoint (sqlite, cache);
    if (!ret)
      {
	  msg = gaiaGetRtTopoErrorMsg (cache);
	  gaiatopo_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_topo:
    msg = "SQL/MM Spatial exception - invalid topology name.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_output:
    msg =
	"TopoGeo_InsertFeatureFromTopoLayer: the output GeoTable does not exists.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_topolayer:
    msg = "TopoGeo_InsertFeatureFromTopoLayer: non-existing TopoLayer.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    msg = "SQL/MM Spatial exception - null argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaiatopo_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

#endif /* end RTTOPO conditionals */
