/*

 gaia_network.c -- implementation of Topology-Network SQL functions
    
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

static struct splite_savepoint *
push_net_savepoint (struct splite_internal_cache *cache)
{
/* adding a new SavePoint to the Network stack */
    struct splite_savepoint *p_svpt = malloc (sizeof (struct splite_savepoint));
    p_svpt->savepoint_name = NULL;
    p_svpt->prev = cache->last_net_svpt;
    p_svpt->next = NULL;
    if (cache->first_net_svpt == NULL)
	cache->first_net_svpt = p_svpt;
    if (cache->last_net_svpt != NULL)
	cache->last_net_svpt->next = p_svpt;
    cache->last_net_svpt = p_svpt;
    return p_svpt;
}

static void
pop_net_savepoint (struct splite_internal_cache *cache)
{
/* removing a SavePoint from the Network stack */
    struct splite_savepoint *p_svpt = cache->last_net_svpt;
    if (p_svpt->prev != NULL)
	p_svpt->prev->next = NULL;
    cache->last_net_svpt = p_svpt->prev;
    if (cache->first_net_svpt == p_svpt)
	cache->first_net_svpt = NULL;
    if (p_svpt->savepoint_name != NULL)
	sqlite3_free (p_svpt->savepoint_name);
    free (p_svpt);
}

SPATIALITE_PRIVATE void
start_net_savepoint (const void *handle, const void *data)
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
    p_svpt = push_net_savepoint (cache);
    p_svpt->savepoint_name =
	sqlite3_mprintf ("netsvpt%04x", cache->next_network_savepoint);
    if (cache->next_network_savepoint >= 0xffffffffu)
	cache->next_network_savepoint = 0;
    else
	cache->next_network_savepoint += 1;

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
release_net_savepoint (const void *handle, const void *data)
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
    p_svpt = cache->last_net_svpt;
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
    pop_net_savepoint (cache);
}

SPATIALITE_PRIVATE void
rollback_net_savepoint (const void *handle, const void *data)
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
    p_svpt = cache->last_net_svpt;
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
    pop_net_savepoint (cache);
}

SPATIALITE_PRIVATE void
fnctaux_GetLastNetworkException (const void *xcontext, int argc,
				 const void *xargv)
{
/* SQL function:
/ GetLastNetworkException  ( text network-name )
/
/ returns: the more recent exception raised by given Topology-Network
/ NULL on invalid args (or when there is no pending exception)
*/
    const char *network_name;
    GaiaNetworkAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_null (context);
	  return;
      }

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
      {
	  sqlite3_result_null (context);
	  return;
      }

    sqlite3_result_text (context, gaianet_get_last_exception (accessor), -1,
			 SQLITE_STATIC);
}

SPATIALITE_PRIVATE void
fnctaux_CreateNetwork (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_InitTopoNet ( text network-name )
/ CreateNetwork ( text network-name )
/ CreateNetwork ( text network-name, bool spatial )
/ CreateNetwork ( text network-name, bool spatial, int srid )
/ CreateNetwork ( text network-name, bool spatial, int srid, bool hasZ )
/ CreateNetwork ( text network-name, bool spatial, int srid, bool hasZ,
/                 bool allow_coincident )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
*/
    int ret;
    const char *network_name;
    int srid = -1;
    int has_z = 0;
    int spatial = 0;
    int allow_coincident = 1;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
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
	      spatial = sqlite3_value_int (argv[1]);
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
	      srid = sqlite3_value_int (argv[2]);
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
	  else if (sqlite3_value_type (argv[3]) == SQLITE_INTEGER)
	      has_z = sqlite3_value_int (argv[3]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }
    if (argc >= 5)
      {
	  if (sqlite3_value_type (argv[4]) == SQLITE_NULL)
	      ;
	  else if (sqlite3_value_type (argv[4]) == SQLITE_INTEGER)
	      allow_coincident = sqlite3_value_int (argv[4]);
	  else
	    {
		sqlite3_result_int (context, -1);
		return;
	    }
      }

    start_net_savepoint (sqlite, cache);
    ret =
	gaiaNetworkCreate (sqlite, network_name, spatial, srid, has_z,
			   allow_coincident);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
}

SPATIALITE_PRIVATE void
fnctaux_DropNetwork (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ DropNetwork ( text network-name )
/
/ returns: 1 on success, 0 on failure
/ -1 on invalid args
*/
    int ret;
    const char *network_name;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GaiaNetworkAccessorPtr accessor;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
      {
	  sqlite3_result_int (context, -1);
	  return;
      }

    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor != NULL)
	gaiaNetworkDestroy (accessor);

    start_net_savepoint (sqlite, cache);
    ret = gaiaNetworkDrop (sqlite, network_name);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    sqlite3_result_int (context, ret);
}

static int
check_matching_srid_dims (GaiaNetworkAccessorPtr accessor, int srid, int dims)
{
/* checking for matching SRID and DIMs */
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net->srid != srid)
	return 0;
    if (net->has_z)
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
fnctaux_AddIsoNetNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddIsoNetNode ( text network-name, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt = NULL;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
      {
	  if (net->spatial)
	      goto spatial_err;
      }
    else if (sqlite3_value_type (argv[1]) == SQLITE_BLOB)
      {
	  if (net->spatial == 0)
	      goto logical_err;
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[1]);
	  n_bytes = sqlite3_value_bytes (argv[1]);

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
	  if (point->FirstPoint != point->LastPoint
	      || point->FirstPoint == NULL)
	      invalid = 1;
	  if (invalid)
	      goto invalid_arg;

	  if (!check_matching_srid_dims
	      (accessor, point->Srid, point->DimensionModel))
	      goto invalid_geom;
	  pt = point->FirstPoint;
      }
    else
	goto invalid_arg;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaAddIsoNetNode (accessor, pt);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (point != NULL)
      {
	  gaiaFreeGeomColl (point);
	  point = NULL;
      }
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;

  spatial_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Logical Network can't accept not null geometry.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_MoveIsoNetNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_MoveIsoNetNode ( text network-name, int node_id, Geometry point )
/
/ returns: TEXT (description of new location)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *net_name;
    sqlite3_int64 node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt = NULL;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	net_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, net_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
      {
	  if (net->spatial)
	      goto spatial_err;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  if (net->spatial == 0)
	      goto logical_err;
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);

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
	  if (point->FirstPoint != point->LastPoint
	      || point->FirstPoint == NULL)
	      invalid = 1;
	  if (invalid)
	      goto invalid_arg;
	  if (!check_matching_srid_dims
	      (accessor, point->Srid, point->DimensionModel))
	      goto invalid_geom;
	  pt = point->FirstPoint;
      }
    else
	goto invalid_arg;
    sprintf (xid, "%lld", node_id);
    if (pt == NULL)
	newpos =
	    sqlite3_mprintf ("Isolated Node %s moved to NULL location", xid);
    else
	newpos =
	    sqlite3_mprintf ("Isolated Node %s moved to location %f,%f", xid,
			     pt->X, pt->Y);

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaMoveIsoNetNode (accessor, node_id, pt);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    point = NULL;
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_net:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;

  spatial_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Logical Network can't accept not null geometry.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemIsoNetNode (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemIsoNetNode ( text network-name, int node_id )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *network_name;
    sqlite3_int64 node_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	node_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    sprintf (xid, "%lld", node_id);
    newpos = sqlite3_mprintf ("Isolated NetNode %s removed", xid);

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaRemIsoNetNode (accessor, node_id);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_net:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_AddLink (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_AddLink ( text network-name, int start_node_id, int end_node_id, Geometry linestring )
/
/ returns: the ID of the inserted Link on success, 0 on failure
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 start_node_id;
    sqlite3_int64 end_node_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln = NULL;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
      {
	  if (net->spatial)
	      goto spatial_err;
      }
    else if (sqlite3_value_type (argv[3]) == SQLITE_BLOB)
      {
	  if (net->spatial == 0)
	      goto logical_err;
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[3]);
	  n_bytes = sqlite3_value_bytes (argv[3]);

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
	  if (!check_matching_srid_dims
	      (accessor, line->Srid, line->DimensionModel))
	      goto invalid_geom;
	  ln = line->FirstLinestring;
      }
    else
	goto invalid_arg;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaAddLink (accessor, start_node_id, end_node_id, ln);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    line = NULL;
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  spatial_err:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Logical Network can't accept not null geometry.",
			  -1);
    return;

  invalid_geom:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ChangeLinkGeom (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ChangeLinkGeom ( text network-name, int link_id, Geometry linestring )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *network_name;
    sqlite3_int64 link_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr line = NULL;
    gaiaLinestringPtr ln = NULL;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
      {
	  if (net->spatial)
	      goto spatial_err;
      }
    else if (sqlite3_value_type (argv[2]) == SQLITE_BLOB)
      {
	  if (net->spatial == 0)
	      goto logical_err;
	  p_blob = (unsigned char *) sqlite3_value_blob (argv[2]);
	  n_bytes = sqlite3_value_bytes (argv[2]);

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

	  if (!check_matching_srid_dims
	      (accessor, line->Srid, line->DimensionModel))
	      goto invalid_geom;
	  ln = line->FirstLinestring;
      }
    else
	goto invalid_arg;
    sprintf (xid, "%lld", link_id);
    newpos = sqlite3_mprintf ("Link %s changed", xid);

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaChangeLinkGeom (accessor, link_id, ln);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    line = NULL;
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_net:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (newpos != NULL)
	sqlite3_free (newpos);
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;

  spatial_err:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (line != NULL)
	gaiaFreeGeomColl (line);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Logical Network can't accept not null geometry.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_RemoveLink (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_RemoveLink ( text network-name, int link_id )
/
/ returns: TEXT (description of operation)
/ raises an exception on failure
*/
    char xid[80];
    char *newpos = NULL;
    int ret;
    const char *network_name;
    sqlite3_int64 link_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    sprintf (xid, "%lld", link_id);
    newpos = sqlite3_mprintf ("Link %s removed", xid);

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaRemoveLink (accessor, link_id);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  if (newpos != NULL)
	      sqlite3_free (newpos);
	  return;
      }
    sqlite3_result_text (context, newpos, strlen (newpos), sqlite3_free);
    return;

  no_net:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (newpos != NULL)
	sqlite3_free (newpos);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewLogLinkSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewLogLinkSplit ( text network-name, int link_id )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial)
	goto spatial_err;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaNewLogLinkSplit (accessor, link_id);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  spatial_err:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - ST_NewLogLinkSplit can't support Spatial Network; try using ST_NewGeoLinkSplit.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModLogLinkSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModLogLinkSplit ( text network-name, int link_id )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial)
	goto spatial_err;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaModLogLinkSplit (accessor, link_id);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  spatial_err:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - ST_ModLogLinkSplit can't support Spatial Network; try using ST_ModGeoLinkSplit.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewGeoLinkSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewGeoLinkSplit ( text network-name, int link_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto spatial_err;
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
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaNewGeoLinkSplit (accessor, link_id, pt);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;

  spatial_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - ST_NewGeoLinkSplit can't support Logical Network; try using ST_NewLogLinkSplit.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModGeoLinkSplit (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModGeoLinkSplit ( text network-name, int link_id, Geometry point )
/
/ returns: the ID of the inserted Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto spatial_err;
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
    if (!check_matching_srid_dims
	(accessor, point->Srid, point->DimensionModel))
	goto invalid_geom;
    pt = point->FirstPoint;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaModGeoLinkSplit (accessor, link_id, pt);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid geometry (mismatching SRID or dimensions).",
			  -1);
    return;

  spatial_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - Spatial Network can't accept null geometry.",
			  -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - ST_ModGeoLinkSplit can't support Logical Network; try using ST_ModLogLinkSplit.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_NewLinkHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_NewLinkHeal ( text network-name, int link_id, int anotherlink_id )
/
/ returns: the ID of the removed Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    sqlite3_int64 anotherlink_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	anotherlink_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaNewLinkHeal (accessor, link_id, anotherlink_id);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ModLinkHeal (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ModLinkHeal ( text network-name, int link_id )
/
/ returns: the ID of the removed Node on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    sqlite3_int64 link_id;
    sqlite3_int64 anotherlink_id;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_INTEGER)
	anotherlink_id = sqlite3_value_int64 (argv[2]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaModLinkHeal (accessor, link_id, anotherlink_id);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

static int
check_empty_network (struct gaia_network *net)
{
/* checking for an empty Network */
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
    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (net->db_handle, sql, &results, &rows, &columns,
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

/* testing LINK */
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("SELECT Count(*) FROM MAIN.\"%s\"", xtable);
    free (xtable);
    ret =
	sqlite3_get_table (net->db_handle, sql, &results, &rows, &columns,
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

static int
do_loginet_from_tgeo (struct gaia_network *net, struct gaia_topology *topo)
{
/* populating a Logical Network starting from an existing Topology */
    char *table;
    char *xtable1;
    char *xtable2;
    char *sql;
    char *errMsg;
    int ret;

/* preparing the SQL statement - NODE */
    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO \"%s\" (node_id, geometry) "
			   "SELECT node_id, NULL FROM MAIN.\"%s\"", xtable1,
			   xtable2);
    free (xtable1);
    free (xtable2);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_LogiNetFromTGeo() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* preparing the SQL statement - LINK */
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (link_id, start_node, end_node, geometry) "
	 "SELECT edge_id, start_node, end_node, NULL FROM MAIN.\"%s\"", xtable1,
	 xtable2);
    free (xtable1);
    free (xtable2);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_LogiNetFromTGeo() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_LogiNetFromTGeo (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_LogiNetFromTGeo ( text network-name, text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *network_name;
    const char *topo_name;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    GaiaTopologyAccessorPtr accessor2;
    struct gaia_topology *topo;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial)
	goto spatial_err;
    if (!check_empty_network (net))
	goto non_empty;

/* attempting to get a Topology Accessor */
    accessor2 = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor2 == NULL)
	goto no_topo;
    topo = (struct gaia_topology *) accessor2;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = do_loginet_from_tgeo (net, topo);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  spatial_err:
    sqlite3_result_error (context,
			  "ST_LogiNetFromTGeo() cannot be applied to Spatial Network.",
			  -1);
    return;

  non_empty:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - non-empty network.", -1);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

static int
check_matching_topo_net (struct gaia_network *net, struct gaia_topology *topo)
{
/* checking for matching SRID and DIMs */
    if (net->srid != topo->srid)
	return 0;
    if (net->has_z != topo->has_z)
	return 0;
    return 1;
}

static int
do_spatnet_from_tgeo (struct gaia_network *net, struct gaia_topology *topo)
{
/* populating a Spatial Network starting from an existing Topology */
    char *table;
    char *xtable1;
    char *xtable2;
    char *sql;
    char *errMsg;
    int ret;

/* preparing the SQL statement - NODE */
    table = sqlite3_mprintf ("%s_node", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_node", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (node_id, geometry) "
			   "SELECT node_id, geom FROM MAIN.\"%s\"", xtable1,
			   xtable2);
    free (xtable1);
    free (xtable2);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_SpatNetFromTGeo() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

/* preparing the SQL statement - LINK */
    table = sqlite3_mprintf ("%s_link", net->network_name);
    xtable1 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    table = sqlite3_mprintf ("%s_edge", topo->topology_name);
    xtable2 = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (link_id, start_node, end_node, geometry) "
	 "SELECT edge_id, start_node, end_node, geom FROM MAIN.\"%s\"", xtable1,
	 xtable2);
    free (xtable1);
    free (xtable2);
    ret = sqlite3_exec (net->db_handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("ST_SpatNetFromTGeo() error: \"%s\"", errMsg);
	  sqlite3_free (errMsg);
	  gaianet_set_last_error_msg ((GaiaNetworkAccessorPtr) net, msg);
	  sqlite3_free (msg);
	  return 0;
      }

    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_SpatNetFromTGeo (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_SpatNetFromTGeo ( text network-name, text topology-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *network_name;
    const char *topo_name;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    GaiaTopologyAccessorPtr accessor2;
    struct gaia_topology *topo;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	topo_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;
    if (!check_empty_network (net))
	goto non_empty;

/* attempting to get a Topology Accessor */
    accessor2 = gaiaGetTopology (sqlite, cache, topo_name);
    if (accessor2 == NULL)
	goto no_topo;
    topo = (struct gaia_topology *) accessor2;
    if (!check_matching_topo_net (net, topo))
	goto mismatching;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = do_spatnet_from_tgeo (net, topo);
    if (ret <= 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (ret <= 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "ST_SpatNetFromTGeo() cannot be applied to Logical Network.",
			  -1);
    return;

  non_empty:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - non-empty network.", -1);
    return;

  no_topo:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid topology name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  mismatching:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - mismatching SRID or dimensions.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ValidLogicalNet (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ValidLogicalNet ( text network-name )
/
/ create/update a table containing an validation report for a given
/ Logical Network
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *network_name;
    int ret;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial)
	goto spatial_err;
    if (check_empty_network (net))
	goto empty;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaValidLogicalNet (accessor);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  spatial_err:
    sqlite3_result_error (context,
			  "ST_ValidLogicalNet() cannot be applied to Spatial Network.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  empty:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - empty network.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_ValidSpatialNet (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_ValidSpatialNet ( text network-name )
/
/ create/update a table containing an validation report for a given
/ Spatial Network
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *network_name;
    int ret;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;
    if (check_empty_network (net))
	goto empty;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaValidSpatialNet (accessor);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "ST_ValidSpatialNet() cannot be applied to Logical Network.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  empty:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - empty network.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_SpatNetFromGeom (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ ST_SpatNetFromGeom ( text network-name , blob geom-collection )
/
/ creates and populates an empty Network by importing a Geometry-collection
/
/ returns NULL on success
/ raises an exception on failure
*/
    const char *network_name;
    int ret;
    const unsigned char *blob;
    int blob_sz;
    gaiaGeomCollPtr geom = NULL;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;
    if (!check_empty_network (net))
	goto not_empty;
    if (!check_matching_srid_dims (accessor, geom->Srid, geom->DimensionModel))
	goto invalid_geom;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = auxnet_insert_into_network (accessor, geom);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_null (context);
    gaiaFreeGeomColl (geom);
    return;

  no_net:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "ST_ValidSpatialNet() cannot be applied to Logical Network.",
			  -1);
    return;

  null_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  not_empty:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - non-empty network.", -1);
    return;

  not_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - not a Geometry.", -1);
    return;

  invalid_geom:
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid Geometry (mismatching SRID or dimensions).",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetNetNodeByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetNetNodeByPoint ( text network-name, Geometry point )
/ GetNetNodeByPoint ( text network-name, Geometry point, double tolerance )
/
/ returns: the ID of some Node on success, 0 if no Node was found
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = 0.0;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;
    pt = point->FirstPoint;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaGetNetNodeByPoint (accessor, pt, tolerance);
    if (ret < 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "GetNetNodekByPoint() cannot be applied to Logical Network.",
			  -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - illegal negative tolerance.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_GetLinkByPoint (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ GetLinkByPoint ( text network-name, Geometry point )
/ GetLinkByPoint ( text network-name, Geometry point, double tolerance )
/
/ returns: the ID of some Link on success
/ raises an exception on failure
*/
    sqlite3_int64 ret;
    const char *network_name;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr point = NULL;
    gaiaPointPtr pt;
    double tolerance = 0;
    int invalid = 0;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
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
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;
    pt = point->FirstPoint;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaGetLinkByPoint (accessor, pt, tolerance);
    if (ret < 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    gaiaFreeGeomColl (point);
    point = NULL;
    if (ret < 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int64 (context, ret);
    return;

  no_net:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  logical_err:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "GetLinkByPoint() cannot be applied to Logical Network.",
			  -1);
    return;

  negative_tolerance:
    if (point != NULL)
	gaiaFreeGeomColl (point);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - illegal negative tolerance.",
			  -1);
    return;
}

static int
check_matching_srid_dims_class (GaiaNetworkAccessorPtr accessor, int srid,
				int dims, int linear)
{
/* checking for matching SRID and DIMs */
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net->srid != srid)
	return 0;
    if (!linear)
	return 0;
    if (net->has_z)
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

static int
check_input_geonet_table (sqlite3 * sqlite, const char *db_prefix,
			  const char *table, const char *column, char **xtable,
			  char **xcolumn, int *srid, int *dims, int *linear)
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
    int xdims;
    int xsrid;

    *xtable = NULL;
    *xcolumn = NULL;
    *srid = -1;
    *dims = GAIA_XY;
    *linear = 1;

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
      case 2:
      case 5:
	  xdims = GAIA_XY;
	  break;
      case 1002:
      case 1005:
	  xdims = GAIA_XY_Z;
	  break;
      case 2002:
      case 2005:
	  xdims = GAIA_XY_M;
	  break;
      case 3002:
      case 3005:
	  xdims = GAIA_XY_Z_M;
	  break;
      default:
	  *linear = 0;
	  break;
      };
    *xtable = xx_table;
    *xcolumn = xx_column;
    *srid = xsrid;
    *dims = xdims;
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_FromGeoTable (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoNet_FromGeoTable ( text network-name, text db-prefix, text table,
/                        text column )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *network_name;
    const char *db_prefix;
    const char *table;
    const char *column;
    char *xtable = NULL;
    char *xcolumn = NULL;
    int srid;
    int dims;
    int linear;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	db_prefix = "main";
    else if (sqlite3_value_type (argv[1]) == SQLITE_TEXT)
	db_prefix = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	table = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[3]) == SQLITE_NULL)
	column = NULL;
    else if (sqlite3_value_type (argv[3]) == SQLITE_TEXT)
	column = (const char *) sqlite3_value_text (argv[3]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

/* checking the input GeoTable */
    if (!check_input_geonet_table
	(sqlite, db_prefix, table, column, &xtable, &xcolumn, &srid, &dims,
	 &linear))
	goto no_input;
    if (!check_matching_srid_dims_class (accessor, srid, dims, linear))
	goto invalid_geom;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaTopoNet_FromGeoTable (accessor, db_prefix, xtable, xcolumn);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    free (xtable);
    free (xcolumn);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  no_input:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid input GeoTable.",
			  -1);
    return;

  null_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid GeoTable (mismatching SRID, dimensions or class).",
			  -1);
    return;

  logical_err:
    if (xtable != NULL)
	free (xtable);
    if (xcolumn != NULL)
	free (xcolumn);
    sqlite3_result_error (context,
			  "FromGeoTable() cannot be applied to Logical Network.",
			  -1);
    return;
}

static int
check_reference_geonet_table (sqlite3 * sqlite, const char *db_prefix,
			      const char *table, const char *column,
			      char **xtable, char **xcolumn, int *srid,
			      int *linear)
{
    int dims;
    return check_input_geonet_table (sqlite, db_prefix, table, column, xtable,
				     xcolumn, srid, &dims, linear);
}

static int
check_matching_srid_class (GaiaNetworkAccessorPtr accessor, int srid,
			   int linear)
{
/* checking for matching SRID */
    struct gaia_network *net = (struct gaia_network *) accessor;
    if (net->srid != srid)
	return 0;
    if (!linear)
	return 0;
    return 1;
}

static int
check_output_geonet_table (sqlite3 * sqlite, const char *table)
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

SPATIALITE_PRIVATE void
fnctaux_TopoNet_ToGeoTable (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoNet_ToGeoTable ( text network-name, text db-prefix, text ref_table,
/                      text ref_column, text out_table )
/ TopoNet_ToGeoTable ( text network-name, text db-prefix, text ref_table,
/                      text ref_column, text out_table, int with_spatial_index )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *network_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    int with_spatial_index = 0;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int linear;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

/* checking the reference GeoTable */
    if (!check_reference_geonet_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &linear))
	goto no_reference;
    if (!check_matching_srid_class (accessor, srid, linear))
	goto invalid_geom;

/* checking the output GeoTable */
    if (!check_output_geonet_table (sqlite, out_table))
	goto err_output;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret =
	gaiaTopoNet_ToGeoTable (accessor, db_prefix, xreftable, xrefcolumn,
				out_table, with_spatial_index);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTable: invalid reference GeoTable.",
			  -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTable: output GeoTable already exists.",
			  -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).",
			  -1);
    return;

  logical_err:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTable() cannot be applied to Logical Network.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_ToGeoTableGeneralize (const void *xcontext, int argc,
				      const void *xargv)
{
/* SQL function:
/ TopoNet_ToGeoTableGeneralize ( text network-name, text db-prefix, 
/                                text ref_table, text ref_column,
/                                text out_table, int tolerance, 
/                                int with_spatial_index )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *network_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    double tolerance = 0.0;
    int with_spatial_index = 0;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int linear;
    GaiaNetworkAccessorPtr accessor;
    struct gaia_network *net;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

/* checking the reference GeoTable */
    if (!check_reference_geonet_table
	(sqlite, db_prefix, ref_table, ref_column, &xreftable, &xrefcolumn,
	 &srid, &linear))
	goto no_reference;
    if (!check_matching_srid_class (accessor, srid, linear))
	goto invalid_geom;

/* checking the output GeoTable */
    if (!check_output_geonet_table (sqlite, out_table))
	goto err_output;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret =
	gaiaTopoNet_ToGeoTableGeneralize (accessor, db_prefix, xreftable,
					  xrefcolumn, out_table, tolerance,
					  with_spatial_index);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    free (xreftable);
    free (xrefcolumn);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTableGeneralize: invalid reference GeoTable.",
			  -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTableGeneralize: output GeoTable already exists.",
			  -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID or class).",
			  -1);
    return;

  logical_err:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    sqlite3_result_error (context,
			  "TopoNet_ToGeoTableGeneralize() cannot be applied to Logical Network.",
			  -1);
    return;
}

static int
do_clone_netnode (const char *db_prefix, const char *in_network,
		  struct gaia_network *net_out)
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
    table = sqlite3_mprintf ("%s_node", in_network);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("SELECT node_id, geometry FROM \"%s\".\"%s\"", xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"node\" error: \"%s\"",
			sqlite3_errmsg (net_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_node", net_out->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" (node_id, geometry) "
			   "VALUES (?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"node\" error: \"%s\"",
			sqlite3_errmsg (net_out->db_handle));
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
		      spatialite_e ("INSERT INTO \"node\" step error: \"%s\"",
				    sqlite3_errmsg (net_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"node\" step error: %s",
			      sqlite3_errmsg (net_out->db_handle));
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
do_clone_link (const char *db_prefix, const char *in_network,
	       struct gaia_network *net_out)
{
/* cloning LINK */
    char *sql;
    char *table;
    char *xprefix;
    char *xtable;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;

/* preparing the input SQL statement */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    table = sqlite3_mprintf ("%s_link", in_network);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("SELECT link_id, start_node, end_node, geometry FROM \"%s\".\"%s\"",
	 xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net_out->db_handle, sql, strlen (sql), &stmt_in,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM \"link\" error: \"%s\"",
			sqlite3_errmsg (net_out->db_handle));
	  goto error;
      }

/* preparing the output SQL statement */
    table = sqlite3_mprintf ("%s_link", net_out->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("INSERT INTO MAIN.\"%s\" (link_id, start_node, end_node, "
	 "geometry) VALUES (?, ?, ?, ?)", xtable);
    free (xtable);
    ret =
	sqlite3_prepare_v2 (net_out->db_handle, sql, strlen (sql), &stmt_out,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("INSERT INTO \"link\" error: \"%s\"",
			sqlite3_errmsg (net_out->db_handle));
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
		if (sqlite3_column_type (stmt_in, 3) == SQLITE_NULL)
		    sqlite3_bind_null (stmt_out, 4);
		else if (sqlite3_column_type (stmt_in, 3) == SQLITE_BLOB)
		    sqlite3_bind_blob (stmt_out, 4,
				       sqlite3_column_blob (stmt_in, 3),
				       sqlite3_column_bytes (stmt_in, 3),
				       SQLITE_STATIC);
		else
		    goto invalid_value;
		/* inserting into the output table */
		ret = sqlite3_step (stmt_out);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		  {
		      spatialite_e ("INSERT INTO \"link\" step error: \"%s\"",
				    sqlite3_errmsg (net_out->db_handle));
		      goto error;
		  }
	    }
	  else
	    {
		spatialite_e ("SELECT FROM \"link\" step error: %s",
			      sqlite3_errmsg (net_out->db_handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  invalid_value:
    spatialite_e ("SELECT FROM \"link\": found an invalid value");

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static int
do_clone_network (const char *db_prefix, const char *in_network,
		  GaiaNetworkAccessorPtr accessor)
{
/* cloning a full Network */
    struct gaia_network *net_out = (struct gaia_network *) accessor;

/* cloning NODE */
    if (!do_clone_netnode (db_prefix, in_network, net_out))
	return 0;

/* cloning LINK */
    if (!do_clone_link (db_prefix, in_network, net_out))
	return 0;

    return 1;
}

static char *
gaiaGetAttachedNetwork (sqlite3 * handle, const char *db_prefix,
			const char *network_name, int *spatial, int *srid,
			int *has_z, int *allow_coincident)
{
/* attempting to retrieve the Input Network for TopoNet_Clone */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    int ok = 0;
    char *xprefix;
    char *xnetwork_name = NULL;
    int xspatial;
    int xsrid;
    int xhas_z;
    int xallow_coincident;

/* preparing the SQL query */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT network_name, spatial, srid, has_z, allow_coincident "
	 "FROM \"%s\".networks WHERE Lower(network_name) = Lower(%Q)", xprefix,
	 network_name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  spatialite_e ("SELECT FROM networks error: \"%s\"\n",
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
		return NULL;
	    }
      }
    sqlite3_finalize (stmt);

    if (ok)
      {
	  *spatial = xspatial;
	  *srid = xsrid;
	  *has_z = xhas_z;
	  *allow_coincident = xallow_coincident;
	  return xnetwork_name;
      }

    if (xnetwork_name != NULL)
	free (xnetwork_name);
    return NULL;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_Clone (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoNet_Clone ( text db-prefix, text in-network-name, text out-network-name )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    int ret;
    const char *db_prefix = "MAIN";
    const char *in_network_name;
    const char *out_network_name;
    char *input_network_name = NULL;
    int spatial;
    int srid;
    int has_z;
    int allow_coincident;
    GaiaNetworkAccessorPtr accessor;
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
	in_network_name = (const char *) sqlite3_value_text (argv[1]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[2]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[2]) == SQLITE_TEXT)
	out_network_name = (const char *) sqlite3_value_text (argv[2]);
    else
	goto invalid_arg;

/* checking the origin Network */
    input_network_name =
	gaiaGetAttachedNetwork (sqlite, db_prefix, in_network_name, &spatial,
				&srid, &has_z, &allow_coincident);
    if (input_network_name == NULL)
	goto no_net;

/* attempting to create the destination Network */
    start_net_savepoint (sqlite, cache);
    ret =
	gaiaNetworkCreate (sqlite, out_network_name, spatial, srid,
			   has_z, allow_coincident);
    if (!ret)
      {
	  rollback_net_savepoint (sqlite, cache);
	  goto no_net2;
      }

/* attempting to get a Network Accessor (destination) */
    accessor = gaiaGetNetwork (sqlite, cache, out_network_name);
    if (accessor == NULL)
      {
	  rollback_net_savepoint (sqlite, cache);
	  goto no_net2;
      }

/* cloning Network */
    ret = do_clone_network (db_prefix, input_network_name, accessor);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  sqlite3_result_error (context, "Clone Network failure", -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    free (input_network_name);
    return;

  no_net:
    if (input_network_name != NULL)
	free (input_network_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name (origin).",
			  -1);
    return;

  no_net2:
    if (input_network_name != NULL)
	free (input_network_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name (destination).",
			  -1);
    return;

  null_arg:
    if (input_network_name != NULL)
	free (input_network_name);
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    if (input_network_name != NULL)
	free (input_network_name);
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_GetLinkSeed (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoNet_GetLinkSeed ( text network-name, int link_id )
/
/ returns: a Point (seed) identifying the Link
/ raises an exception on failure
*/
    const char *network_name;
    sqlite3_int64 link_id;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom;
    GaiaNetworkAccessorPtr accessor;
    int gpkg_mode = 0;
    int tiny_point = 0;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    struct gaia_network *net;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (cache != NULL)
      {
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;
    if (sqlite3_value_type (argv[1]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[1]) == SQLITE_INTEGER)
	link_id = sqlite3_value_int64 (argv[1]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

    gaianet_reset_last_error_msg (accessor);
    geom = gaiaGetLinkSeed (accessor, link_id);
    if (geom == NULL)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  if (msg != NULL)
	    {
		gaianet_set_last_error_msg (accessor, msg);
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

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "TopoNet_GetLinkSeed() cannot be applied to Logical Network.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_UpdateSeeds (const void *xcontext, int argc, const void *xargv)
{
/* SQL function:
/ TopoNet_UpdateSeeds ( text network-name )
/ TopoNet_UpdateSeeds ( text network-name, int incremental_mode )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *network_name;
    int incremental_mode = 1;
    int ret;
    GaiaNetworkAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    struct gaia_network *net;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    ret = gaiaTopoNetUpdateSeeds (accessor, incremental_mode);
    if (!ret)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (!ret)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  if (msg != NULL)
	    {
		gaianet_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "TopoNet_UpdateSeeds() cannot be applied to Logical Network.",
			  -1);
    return;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_DisambiguateSegmentLinks (const void *xcontext, int argc,
					  const void *xargv)
{
/* SQL function:
/ TopoNet_DisambiguateSegmentLinks ( text network-name )
/
/ returns: the total number of changed Links.
/ raises an exception on failure
*/
    const char *network_name;
    int changed_links = 0;
    GaiaNetworkAccessorPtr accessor;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    struct gaia_network *net;
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
    else
	goto invalid_arg;

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    net = (struct gaia_network *) accessor;
    if (net->spatial == 0)
	goto logical_err;

    gaianet_reset_last_error_msg (accessor);
    start_net_savepoint (sqlite, cache);
    changed_links = gaiaTopoNet_DisambiguateSegmentLinks (accessor);
    if (changed_links < 0)
	rollback_net_savepoint (sqlite, cache);
    else
	release_net_savepoint (sqlite, cache);
    if (changed_links < 0)
      {
	  const char *msg = lwn_GetErrorMsg (net->lwn_iface);
	  if (msg != NULL)
	    {
		gaianet_set_last_error_msg (accessor, msg);
		sqlite3_result_error (context, msg, -1);
		return;
	    }
	  sqlite3_result_null (context);
	  return;
      }
    sqlite3_result_int (context, changed_links);
    return;

  no_net:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid network name.",
			  -1);
    return;

  null_arg:
    sqlite3_result_error (context, "SQL/MM Spatial exception - null argument.",
			  -1);
    return;

  invalid_arg:
    sqlite3_result_error (context,
			  "SQL/MM Spatial exception - invalid argument.", -1);
    return;

  logical_err:
    sqlite3_result_error (context,
			  "TopoNet_UpdateSeeds() cannot be applied to Logical Network.",
			  -1);
    return;
}

static int
check_matching_srid (GaiaNetworkAccessorPtr accessor, int srid)
{
/* checking for matching SRID */
    struct gaia_network *network = (struct gaia_network *) accessor;
    if (network->srid != srid)
	return 0;
    return 1;
}

SPATIALITE_PRIVATE void
fnctaux_TopoNet_LineLinksList (const void *xcontext, int argc,
			       const void *xargv)
{
/* SQL function:
/ TopoNet_LineLinksList ( text network-name, text db-prefix, text ref_table,
/                         text ref_column, text out_table )
/
/ returns: 1 on success
/ raises an exception on failure
*/
    const char *msg;
    int ret;
    const char *network_name;
    const char *db_prefix;
    const char *ref_table;
    const char *ref_column;
    const char *out_table;
    char *xreftable = NULL;
    char *xrefcolumn = NULL;
    int srid;
    int family;
    GaiaNetworkAccessorPtr accessor = NULL;
    sqlite3_context *context = (sqlite3_context *) xcontext;
    sqlite3_value **argv = (sqlite3_value **) xargv;
    sqlite3 *sqlite = sqlite3_context_db_handle (context);
    struct splite_internal_cache *cache = sqlite3_user_data (context);
    GAIA_UNUSED ();		/* LCOV_EXCL_LINE */
    if (sqlite3_value_type (argv[0]) == SQLITE_NULL)
	goto null_arg;
    else if (sqlite3_value_type (argv[0]) == SQLITE_TEXT)
	network_name = (const char *) sqlite3_value_text (argv[0]);
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

/* attempting to get a Network Accessor */
    accessor = gaiaGetNetwork (sqlite, cache, network_name);
    if (accessor == NULL)
	goto no_net;
    gaianet_reset_last_error_msg (accessor);

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
	gaiaTopoNet_LineLinksList (accessor, db_prefix, xreftable, xrefcolumn,
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
	  gaianet_set_last_error_msg (accessor, msg);
	  sqlite3_result_error (context, msg, -1);
	  return;
      }
    sqlite3_result_int (context, 1);
    return;

  no_net:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid network name.";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  no_reference:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoGeo_LineLinksList: invalid reference GeoTable.";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  err_output:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "TopoNet_LineLinksList: output GeoTable already exists.";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  null_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - null argument.";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_arg:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg = "SQL/MM Spatial exception - invalid argument.";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  invalid_geom:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (mismatching SRID).";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;

  not_linestring:
    if (xreftable != NULL)
	free (xreftable);
    if (xrefcolumn != NULL)
	free (xrefcolumn);
    msg =
	"SQL/MM Spatial exception - invalid reference GeoTable (not of the [MULTI]LINESTRING type).";
    gaianet_set_last_error_msg (accessor, msg);
    sqlite3_result_error (context, msg, -1);
    return;
}

#endif /* end RTTOPO conditionals */
