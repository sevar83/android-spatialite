/*

 net_callbacks.c -- implementation of Topology-Network callback functions
    
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
#include <spatialite/gaiaaux.h>

#include <spatialite.h>
#include <spatialite_private.h>

#include <librttopo.h>
#include <lwn_network.h>

#include "network_private.h"

struct net_node
{
/* a struct wrapping a Network Node */
    sqlite3_int64 node_id;
    double x;
    double y;
    double z;
    int has_z;
    int is_null;
    struct net_node *next;
};

struct net_nodes_list
{
/* a struct wrapping a list of Network Nodes */
    struct net_node *first;
    struct net_node *last;
    int count;
};

struct net_link
{
/* a struct wrapping a Network Link */
    sqlite3_int64 link_id;
    sqlite3_int64 start_node;
    sqlite3_int64 end_node;
    gaiaLinestringPtr geom;
    struct net_link *next;
};

struct net_links_list
{
/* a struct wrapping a list of Network Links */
    struct net_link *first;
    struct net_link *last;
    int count;
};

static struct net_node *
create_net_node (sqlite3_int64 node_id, double x, double y, double z, int has_z)
{
/* creating a Network Node */
    struct net_node *ptr = malloc (sizeof (struct net_node));
    ptr->node_id = node_id;
    ptr->x = x;
    ptr->y = y;
    ptr->z = z;
    ptr->has_z = has_z;
    ptr->is_null = 0;
    ptr->next = NULL;
    return ptr;
}

static struct net_node *
create_net_node_null (sqlite3_int64 node_id)
{
/* creating a Network Node - NULL */
    struct net_node *ptr = malloc (sizeof (struct net_node));
    ptr->node_id = node_id;
    ptr->is_null = 1;
    ptr->next = NULL;
    return ptr;
}

static void
destroy_net_node (struct net_node *ptr)
{
/* destroying a Network Node */
    if (ptr == NULL)
	return;
    free (ptr);
}

static struct net_link *
create_net_link (sqlite3_int64 link_id, sqlite3_int64 start_node,
		 sqlite3_int64 end_node, gaiaLinestringPtr ln)
{
/* creating a Network Link */
    struct net_link *ptr = malloc (sizeof (struct net_link));
    ptr->link_id = link_id;
    ptr->start_node = start_node;
    ptr->end_node = end_node;
    ptr->geom = ln;
    ptr->next = NULL;
    return ptr;
}

static void
destroy_net_link (struct net_link *ptr)
{
/* destroying a Network Link */
    if (ptr == NULL)
	return;
    if (ptr->geom != NULL)
	gaiaFreeLinestring (ptr->geom);
    free (ptr);
}

static struct net_nodes_list *
create_nodes_list (void)
{
/* creating an empty list of Network Nodes */
    struct net_nodes_list *ptr = malloc (sizeof (struct net_nodes_list));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    return ptr;
}

static void
destroy_net_nodes_list (struct net_nodes_list *ptr)
{
/* destroying a list of Network Nodes */
    struct net_node *p;
    struct net_node *pn;
    if (ptr == NULL)
	return;

    p = ptr->first;
    while (p != NULL)
      {
	  pn = p->next;
	  destroy_net_node (p);
	  p = pn;
      }
    free (ptr);
}

static void
add_node_2D (struct net_nodes_list *list, sqlite3_int64 node_id,
	     double x, double y)
{
/* inserting a Network Node 2D into the list */
    struct net_node *ptr;
    if (list == NULL)
	return;

    ptr = create_net_node (node_id, x, y, 0.0, 0);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static void
add_node_3D (struct net_nodes_list *list, sqlite3_int64 node_id,
	     double x, double y, double z)
{
/* inserting a Network Node 3D into the list */
    struct net_node *ptr;
    if (list == NULL)
	return;

    ptr = create_net_node (node_id, x, y, z, 1);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static void
add_node_null (struct net_nodes_list *list, sqlite3_int64 node_id)
{
/* inserting a Network Node (NULL) into the list */
    struct net_node *ptr;
    if (list == NULL)
	return;

    ptr = create_net_node_null (node_id);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static struct net_links_list *
create_links_list (void)
{
/* creating an empty list of Network Links */
    struct net_links_list *ptr = malloc (sizeof (struct net_links_list));
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->count = 0;
    return ptr;
}

static void
destroy_links_list (struct net_links_list *ptr)
{
/* destroying a list of Network Links */
    struct net_link *p;
    struct net_link *pn;
    if (ptr == NULL)
	return;

    p = ptr->first;
    while (p != NULL)
      {
	  pn = p->next;
	  destroy_net_link (p);
	  p = pn;
      }
    free (ptr);
}

static void
add_link (struct net_links_list *list, sqlite3_int64 link_id,
	  sqlite3_int64 start_node, sqlite3_int64 end_node,
	  gaiaLinestringPtr ln)
{
/* inserting a Network Link into the list */
    struct net_link *ptr;
    if (list == NULL)
	return;

    ptr = create_net_link (link_id, start_node, end_node, ln);
    if (list->first == NULL)
	list->first = ptr;
    if (list->last != NULL)
	list->last->next = ptr;
    list->last = ptr;
    list->count++;
}

static char *
do_prepare_read_net_node (const char *network_name, int fields, int spatial,
			  int has_z)
{
/* preparing the auxiliary "read_node" SQL statement */
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;

    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & LWN_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_NODE_GEOM && spatial)
      {
	  if (comma)
	      sql =
		  sqlite3_mprintf ("%s, ST_X(geometry), ST_Y(geometry)", prev);
	  else
	      sql = sqlite3_mprintf ("%s ST_X(geometry), ST_Y(geometry)", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
	  if (has_z)
	    {
		sql = sqlite3_mprintf ("%s, ST_Z(geometry)", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
      }
    table = sqlite3_mprintf ("%s_node", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE node_id = ?", prev, xtable);
    sqlite3_free (prev);
    free (xtable);
    return sql;
}

static int
do_read_net_node (sqlite3_stmt * stmt, struct net_nodes_list *list,
		  sqlite3_int64 id, int fields, int spatial, int has_z,
		  const char *callback_name, char **errmsg)
{
/* reading Nodes out from the DBMS */
    int icol = 0;
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		int ok_id = 0;
		int ok_x = 0;
		int ok_y = 0;
		int ok_z = 0;
		sqlite3_int64 node_id = -1;
		double x = 0.0;
		double y = 0.0;
		double z = 0.0;
		if (fields & LWN_COL_NODE_NODE_ID)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
			{
			    node_id = sqlite3_column_int64 (stmt, icol);
			    ok_id = 1;
			}
		      icol++;
		  }
		else
		    ok_id = 1;
		if (fields & LWN_COL_NODE_GEOM && spatial)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    x = sqlite3_column_double (stmt, icol);
			    ok_x = 1;
			}
		      icol++;
		      if (sqlite3_column_type (stmt, icol) == SQLITE_FLOAT)
			{
			    y = sqlite3_column_double (stmt, icol);
			    ok_y = 1;
			}
		      icol++;
		      if (has_z)
			{
			    if (sqlite3_column_type (stmt, icol) ==
				SQLITE_FLOAT)
			      {
				  z = sqlite3_column_double (stmt, icol);
				  ok_z = 1;
			      }
			}
		  }
		else
		  {
		      ok_x = 1;
		      ok_y = 1;
		      ok_z = 1;
		  }
		if (!spatial)
		  {
		      add_node_null (list, node_id);
		      *errmsg = NULL;
		      sqlite3_reset (stmt);
		      return 1;
		  }
		else
		  {
		      if (has_z)
			{
			    if (ok_id && ok_x && ok_y && ok_z)
			      {
				  add_node_3D (list, node_id, x, y, z);
				  *errmsg = NULL;
				  sqlite3_reset (stmt);
				  return 1;
			      }
			}
		      else
			{
			    if (ok_id && ok_x && ok_y)
			      {
				  add_node_2D (list, node_id, x, y);
				  *errmsg = NULL;
				  sqlite3_reset (stmt);
				  return 1;
			      }
			}
		  }
		/* an invalid Node has been found */
		*errmsg =
		    sqlite3_mprintf
		    ("%s: found an invalid Node \"%lld\"", callback_name,
		     node_id);
		sqlite3_reset (stmt);
		return 0;
	    }
      }
    *errmsg = NULL;
    sqlite3_reset (stmt);
    return 1;
}

static char *
do_prepare_read_link (const char *network_name, int fields)
{
/* preparing the auxiliary "read_link" SQL statement */
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;

    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & LWN_COL_LINK_LINK_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, link_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s link_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geometry", prev);
	  else
	      sql = sqlite3_mprintf ("%s geometry", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_link", network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf ("%s FROM MAIN.\"%s\" WHERE link_id = ?", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    return sql;
}

static int
do_read_link_row (sqlite3_stmt * stmt, struct net_links_list *list, int fields,
		  const char *callback_name, char **errmsg)
{
/* reading a Link Row out from the resultset */
    int icol = 0;

    int ok_id = 0;
    int ok_start = 0;
    int ok_end = 0;
    int ok_geom = 0;
    sqlite3_int64 link_id = -1;
    sqlite3_int64 start_node = -1;
    sqlite3_int64 end_node = -1;
    gaiaGeomCollPtr geom = NULL;
    gaiaLinestringPtr ln = NULL;
    if (fields & LWN_COL_LINK_LINK_ID)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		link_id = sqlite3_column_int64 (stmt, icol);
		ok_id = 1;
	    }
	  icol++;
      }
    else
	ok_id = 1;
    if (fields & LWN_COL_LINK_START_NODE)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		start_node = sqlite3_column_int64 (stmt, icol);
		ok_start = 1;
	    }
	  icol++;
      }
    else
	ok_start = 1;
    if (fields & LWN_COL_LINK_END_NODE)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_INTEGER)
	    {
		end_node = sqlite3_column_int64 (stmt, icol);
		ok_end = 1;
	    }
	  icol++;
      }
    else
	ok_end = 1;
    if (fields & LWN_COL_LINK_GEOM)
      {
	  if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
	    {
		ln = NULL;
		ok_geom = 1;
	    }
	  if (sqlite3_column_type (stmt, icol) == SQLITE_BLOB)
	    {
		const unsigned char *blob = sqlite3_column_blob (stmt, icol);
		int blob_sz = sqlite3_column_bytes (stmt, icol);
		geom = gaiaFromSpatiaLiteBlobWkb (blob, blob_sz);
		if (geom != NULL)
		  {
		      if (geom->FirstPoint == NULL
			  && geom->FirstPolygon == NULL
			  && geom->FirstLinestring ==
			  geom->LastLinestring && geom->FirstLinestring != NULL)
			{
			    ln = geom->FirstLinestring;
			    ok_geom = 1;
			    /* releasing ownership on Linestring */
			    geom->FirstLinestring = NULL;
			    geom->LastLinestring = NULL;
			}
		      gaiaFreeGeomColl (geom);
		  }
	    }
	  icol++;
      }
    else
	ok_geom = 1;
    if (ok_id && ok_start && ok_end && ok_geom)
      {
	  add_link (list, link_id, start_node, end_node, ln);
	  *errmsg = NULL;
	  return 1;
      }
/* an invalid Link has been found */
    if (geom != NULL)
	gaiaFreeGeomColl (geom);
    *errmsg =
	sqlite3_mprintf
	("%s: found an invalid Link \"%lld\"", callback_name, link_id);
    return 0;
}

static int
do_read_link (sqlite3_stmt * stmt, struct net_links_list *list,
	      sqlite3_int64 link_id, int fields, const char *callback_name,
	      char **errmsg)
{
/* reading a single Link out from the DBMS */
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, link_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (!do_read_link_row
		    (stmt, list, fields, callback_name, errmsg))
		  {
		      sqlite3_reset (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_reset (stmt);
    return 1;
}

static int
do_read_link_by_net_node (sqlite3_stmt * stmt, struct net_links_list *list,
			  sqlite3_int64 node_id, int fields,
			  const char *callback_name, char **errmsg)
{
/* reading a single Link out from the DBMS */
    int ret;

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_int64 (stmt, 1, node_id);
    sqlite3_bind_int64 (stmt, 2, node_id);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (!do_read_link_row
		    (stmt, list, fields, callback_name, errmsg))
		  {
		      sqlite3_reset (stmt);
		      return 0;
		  }
	    }
      }
    sqlite3_reset (stmt);
    return 1;
}

const char *
netcallback_lastErrorMessage (const LWN_BE_DATA * be)
{
    return gaianet_get_last_exception ((GaiaNetworkAccessorPtr) be);
}

LWN_BE_NETWORK *
netcallback_loadNetworkByName (const LWN_BE_DATA * be, const char *name)
{
/* callback function: loadNetworkByName */
    struct gaia_network *ptr = (struct gaia_network *) be;
    char *network_name;
    int spatial;
    int srid;
    int has_z;
    int allow_coincident;
    struct splite_internal_cache *cache =
	(struct splite_internal_cache *) ptr->cache;

    if (gaiaReadNetworkFromDBMS
	(ptr->db_handle, name, &network_name, &spatial, &srid, &has_z,
	 &allow_coincident))
      {
	  ptr->network_name = network_name;
	  ptr->srid = srid;
	  ptr->has_z = has_z;
	  ptr->spatial = spatial;
	  ptr->allow_coincident = allow_coincident;
	  /* registering into the Internal Cache double linked list */
	  if (cache->firstNetwork == NULL)
	      cache->firstNetwork = ptr;
	  if (cache->lastNetwork != NULL)
	    {
		struct gaia_network *p2 =
		    (struct gaia_network *) (cache->lastNetwork);
		p2->next = ptr;
	    }
	  cache->lastNetwork = ptr;
	  return (LWN_BE_NETWORK *) ptr;
      }
    else
	return NULL;
}

int
netcallback_freeNetwork (LWN_BE_NETWORK * lwn_net)
{
/* callback function: freeNetwork - does nothing */
    if (lwn_net != NULL)
	lwn_net = NULL;		/* silencing stupid compiler warnings on unuse args */
    return 1;
}

static gaiaGeomCollPtr
do_convert_lwnline_to_geom (LWN_LINE * line, int srid)
{
/* converting an LWN_LINE into a Linestring  */
    int iv;
    double ox;
    double oy;
    int normalized_points = 0;
    gaiaLinestringPtr ln;
    gaiaGeomCollPtr geom;
    if (line->has_z)
	geom = gaiaAllocGeomCollXYZ ();
    else
	geom = gaiaAllocGeomColl ();
    for (iv = 0; iv < line->points; iv++)
      {
	  /* counting how many duplicate points are there */
	  double x = line->x[iv];
	  double y = line->y[iv];
	  if (iv == 0)
	      normalized_points++;
	  else
	    {
		if (x == ox && y == oy)
		    ;
		else
		    normalized_points++;
	    }
	  ox = x;
	  oy = y;
      }
    ln = gaiaAddLinestringToGeomColl (geom, normalized_points);
    normalized_points = 0;
    for (iv = 0; iv < line->points; iv++)
      {
	  double x = line->x[iv];
	  double y = line->y[iv];
	  double z;
	  if (iv == 0)
	      ;
	  else
	    {
		if (x == ox && y == oy)
		    continue;	/* discarding duplicate points */
	    }
	  ox = x;
	  oy = y;
	  if (line->has_z)
	      z = line->z[iv];
	  if (line->has_z)
	    {
		gaiaSetPointXYZ (ln->Coords, normalized_points, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (ln->Coords, normalized_points, x, y);
	    }
	  normalized_points++;
      }
    geom->DeclaredType = GAIA_LINESTRING;
    geom->Srid = srid;

    return geom;
}

LWN_NET_NODE *
netcallback_getNetNodeWithinDistance2D (const LWN_BE_NETWORK * lwn_net,
					const LWN_POINT * pt, double dist,
					int *numelems, int fields, int limit)
{
/* callback function: getNodeWithinDistance2D */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt;
    int ret;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct net_nodes_list *list = NULL;
    LWN_NET_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }
    if (pt == NULL)
      {
	  *numelems = 0;
	  return NULL;
      }

    stmt = accessor->stmt_getNetNodeWithinDistance2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql =
	      do_prepare_read_net_node (accessor->network_name, fields,
					accessor->spatial, accessor->has_z);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Prepare_getNetNodeWithinDistance2D AUX error: \"%s\"",
		     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->x);
    sqlite3_bind_double (stmt, 2, pt->y);
    sqlite3_bind_double (stmt, 3, dist);
    sqlite3_bind_double (stmt, 4, pt->x);
    sqlite3_bind_double (stmt, 5, pt->y);
    sqlite3_bind_double (stmt, 6, dist);
    list = create_nodes_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_net_node
			  (stmt_aux, list, node_id, fields, accessor->spatial,
			   accessor->has_z,
			   "netcallback_getNetNodeWithinDistance2D", &msg))
			{
			    gaianet_set_last_error_msg (net, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_getNodeWithinDistance2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct net_node *p_nd;
		result = malloc (sizeof (LWN_NET_NODE) * list->count);
		p_nd = list->first;
		while (p_nd != NULL)
		  {
		      LWN_NET_NODE *nd = result + i;
		      nd->geom = NULL;
		      if (fields & LWN_COL_NODE_NODE_ID)
			  nd->node_id = p_nd->node_id;
		      if (fields & LWN_COL_NODE_GEOM)
			{
			    if (p_nd->is_null)
				;
			    else
			      {
				  if (accessor->has_z)
				      nd->geom =
					  lwn_create_point3d (accessor->srid,
							      p_nd->x, p_nd->y,
							      p_nd->z);
				  else
				      nd->geom =
					  lwn_create_point2d (accessor->srid,
							      p_nd->x, p_nd->y);
			      }
			}
		      i++;
		      p_nd = p_nd->next;
		  }
		*numelems = list->count;
	    }
      }

    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_net_nodes_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_net_nodes_list (list);
    *numelems = -1;
    sqlite3_reset (stmt);
    return NULL;
}

LWN_NET_NODE *
netcallback_getNetNodeById (const LWN_BE_NETWORK * lwn_net,
			    const LWN_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getNetNodeById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt_aux = NULL;
    int ret;
    int i;
    char *sql;
    struct net_nodes_list *list = NULL;
    LWN_NET_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    /* preparing the SQL statement */
    sql =
	do_prepare_read_net_node (accessor->network_name, fields,
				  accessor->spatial, accessor->has_z);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt_aux,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getNetNodeById AUX error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_nodes_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_net_node
	      (stmt_aux, list, *(ids + i), fields, accessor->spatial,
	       accessor->has_z, "netcallback_getNetNodeById", &msg))
	    {
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no node was found */
	  *numelems = list->count;
      }
    else
      {
	  struct net_node *p_nd;
	  result = malloc (sizeof (LWN_NET_NODE) * list->count);
	  p_nd = list->first;
	  i = 0;
	  while (p_nd != NULL)
	    {
		LWN_NET_NODE *nd = result + i;
		nd->geom = NULL;
		if (fields & LWN_COL_NODE_NODE_ID)
		    nd->node_id = p_nd->node_id;
		if (fields & LWN_COL_NODE_GEOM)
		  {
		      if (p_nd->is_null)
			  ;
		      else
			{
			    if (accessor->has_z)
				nd->geom =
				    lwn_create_point3d (accessor->srid, p_nd->x,
							p_nd->y, p_nd->z);
			    else
				nd->geom =
				    lwn_create_point2d (accessor->srid, p_nd->x,
							p_nd->y);
			}
		  }
		i++;
		p_nd = p_nd->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_net_nodes_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_net_nodes_list (list);
    *numelems = -1;
    return NULL;
}

LWN_LINK *
netcallback_getLinkWithinDistance2D (const LWN_BE_NETWORK * lwn_net,
				     const LWN_POINT * pt, double dist,
				     int *numelems, int fields, int limit)
{
/* callback function: getLinkWithinDistance2D */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt;
    int ret;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct net_links_list *list = NULL;
    LWN_LINK *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }
    if (pt == NULL)
      {
	  *numelems = 0;
	  return NULL;
      }

    stmt = accessor->stmt_getLinkWithinDistance2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql = do_prepare_read_link (accessor->network_name, fields);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf ("Prepare_getLinkById AUX error: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, pt->x);
    sqlite3_bind_double (stmt, 2, pt->y);
    sqlite3_bind_double (stmt, 3, dist);
    sqlite3_bind_double (stmt, 4, pt->x);
    sqlite3_bind_double (stmt, 5, pt->y);
    sqlite3_bind_double (stmt, 6, dist);
    list = create_links_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 link_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_link
			  (stmt_aux, list, link_id, fields,
			   "netcallback_getLinkWithinDistance2D", &msg))
			{
			    gaianet_set_last_error_msg (net, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_getLinkWithinDistance2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct net_link *p_lnk;
		result = malloc (sizeof (LWN_LINK) * list->count);
		p_lnk = list->first;
		while (p_lnk != NULL)
		  {
		      LWN_LINK *lnk = result + i;
		      if (fields & LWN_COL_LINK_LINK_ID)
			  lnk->link_id = p_lnk->link_id;
		      if (fields & LWN_COL_LINK_START_NODE)
			  lnk->start_node = p_lnk->start_node;
		      if (fields & LWN_COL_LINK_END_NODE)
			  lnk->end_node = p_lnk->end_node;
		      if (fields & LWN_COL_LINK_GEOM)
			  lnk->geom =
			      gaianet_convert_linestring_to_lwnline
			      (p_lnk->geom, accessor->srid, accessor->has_z);
		      else
			  lnk->geom = NULL;
		      i++;
		      p_lnk = p_lnk->next;
		  }
		*numelems = list->count;
	    }
      }
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_links_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_links_list (list);
    *numelems = -1;
    sqlite3_reset (stmt);
    return NULL;
}

int
netcallback_insertNetNodes (const LWN_BE_NETWORK * lwn_net,
			    LWN_NET_NODE * nodes, int numelems)
{
/* callback function: insertNetNodes */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt;
    int ret;
    int i;
    unsigned char *p_blob;
    int n_bytes;
    gaiaGeomCollPtr geom = NULL;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return 0;

    stmt = accessor->stmt_insertNetNodes;
    if (stmt == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    for (i = 0; i < numelems; i++)
      {
	  LWN_NET_NODE *nd = nodes + i;
	  /* setting up the prepared statement */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (nd->node_id <= 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, nd->node_id);
	  if (nd->geom == NULL)
	      sqlite3_bind_null (stmt, 2);
	  else
	    {
		if (accessor->has_z)
		    geom = gaiaAllocGeomCollXYZ ();
		else
		    geom = gaiaAllocGeomColl ();
		if (accessor->has_z)
		    gaiaAddPointToGeomCollXYZ (geom, nd->geom->x, nd->geom->y,
					       nd->geom->z);
		else
		    gaiaAddPointToGeomColl (geom, nd->geom->x, nd->geom->y);
		geom->Srid = accessor->srid;
		geom->DeclaredType = GAIA_POINT;
		gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
					    tiny_point);
		gaiaFreeGeomColl (geom);
		sqlite3_bind_blob (stmt, 2, p_blob, n_bytes, free);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		/* retrieving the PK value */
		nd->node_id = sqlite3_last_insert_rowid (accessor->db_handle);
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_insertNetNodes: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return 1;

  error:
    sqlite3_reset (stmt);
    return 0;
}

int
netcallback_updateNetNodesById (const LWN_BE_NETWORK * lwn_net,
				const LWN_NET_NODE * nodes, int numnodes,
				int upd_fields)
{
/* callback function: updateNetNodesById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int icol = 1;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_node", accessor->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & LWN_COL_NODE_NODE_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, node_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s node_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & LWN_COL_NODE_GEOM)
      {
	  if (accessor->has_z)
	    {
		if (comma)
		    sql =
			sqlite3_mprintf
			("%s, geometry = MakePointZ(?, ?. ?, %d)", prev,
			 accessor->srid);
		else
		    sql =
			sqlite3_mprintf
			("%s geometry = MakePointZ(?, ?, ?, %d)", prev,
			 accessor->srid);
	    }
	  else
	    {
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, geometry = MakePoint(?, ?, %d)",
					 prev, accessor->srid);
		else
		    sql =
			sqlite3_mprintf ("%s geometry = MakePoint(?, ?, %d)",
					 prev, accessor->srid);
	    }
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    sql = sqlite3_mprintf ("%s WHERE node_id = ?", prev);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_updateNetNodesById error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  return -1;
      }

    for (i = 0; i < numnodes; i++)
      {
	  /* parameter binding */
	  const LWN_NET_NODE *nd = nodes + i;
	  icol = 1;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (upd_fields & LWN_COL_NODE_NODE_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, nd->node_id);
		icol++;
	    }
	  if (upd_fields & LWN_COL_NODE_GEOM)
	    {
		if (accessor->spatial)
		  {
		      sqlite3_bind_double (stmt, icol, nd->geom->x);
		      icol++;
		      sqlite3_bind_double (stmt, icol, nd->geom->y);
		      icol++;
		      if (accessor->has_z)
			{
			    sqlite3_bind_double (stmt, icol, nd->geom->z);
			    icol++;
			}
		  }
		else
		  {
		      icol += 2;
		      if (accessor->has_z)
			  icol++;
		  }
	    }
	  sqlite3_bind_int64 (stmt, icol, nd->node_id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      changed += sqlite3_changes (accessor->db_handle);
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_updateNetNodesById: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

int
netcallback_insertLinks (const LWN_BE_NETWORK * lwn_net, LWN_LINK * links,
			 int numelems)
{
/* callback function: insertLinks */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt;
    int ret;
    int i;
    gaiaGeomCollPtr geom;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return 0;

    stmt = accessor->stmt_insertLinks;
    if (stmt == NULL)
	return 0;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

    for (i = 0; i < numelems; i++)
      {
	  LWN_LINK *lnk = links + i;
	  /* setting up the prepared statement */
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (lnk->link_id <= 0)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_int64 (stmt, 1, lnk->link_id);
	  sqlite3_bind_int64 (stmt, 2, lnk->start_node);
	  sqlite3_bind_int64 (stmt, 3, lnk->end_node);
	  if (lnk->geom == NULL)
	      sqlite3_bind_null (stmt, 4);
	  else
	    {
		/* transforming the LWN_LINE into a Geometry-Linestring */
		geom = do_convert_lwnline_to_geom (lnk->geom, accessor->srid);
		gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes, gpkg_mode,
					    tiny_point);
		gaiaFreeGeomColl (geom);
		sqlite3_bind_blob (stmt, 4, p_blob, n_bytes, free);
	    }
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		/* retrieving the PK value */
		lnk->link_id = sqlite3_last_insert_rowid (accessor->db_handle);
	    }
	  else
	    {
		char *msg = sqlite3_mprintf ("netcallback_inserLinks: \"%s\"",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return 1;

  error:
    sqlite3_reset (stmt);
    return 0;
}

LWN_LINK *
netcallback_getLinkByNetNode (const LWN_BE_NETWORK * lwn_net,
			      const LWN_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getLinkByNetNode */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    int ret;
    char *sql;
    char *prev;
    char *table;
    char *xtable;
    int comma = 0;
    int i;
    sqlite3_stmt *stmt_aux = NULL;
    struct net_links_list *list = NULL;
    LWN_LINK *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    /* preparing the SQL statement */
    sql = sqlite3_mprintf ("SELECT ");
    prev = sql;
    if (fields & LWN_COL_LINK_LINK_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, link_id", prev);
	  else
	      sql = sqlite3_mprintf ("%s link_id", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (fields & LWN_COL_LINK_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geometry", prev);
	  else
	      sql = sqlite3_mprintf ("%s geometry", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    table = sqlite3_mprintf ("%s_link", accessor->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql =
	sqlite3_mprintf
	("%s FROM MAIN.\"%s\" WHERE start_node = ? OR end_node = ?", prev,
	 xtable);
    free (xtable);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
			    &stmt_aux, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg =
	      sqlite3_mprintf ("Prepare_getLinkByNetNode AUX error: \"%s\"",
			       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_links_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_link_by_net_node
	      (stmt_aux, list, *(ids + i), fields,
	       "netcallback_getLinkByNetNode", &msg))
	    {
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no link was found */
	  *numelems = list->count;
      }
    else
      {
	  struct net_link *p_lnk;
	  result = malloc (sizeof (LWN_LINK) * list->count);
	  p_lnk = list->first;
	  i = 0;
	  while (p_lnk != NULL)
	    {
		LWN_LINK *lnk = result + i;
		lnk->geom = NULL;
		if (fields & LWN_COL_LINK_LINK_ID)
		    lnk->link_id = p_lnk->link_id;
		if (fields & LWN_COL_LINK_START_NODE)
		    lnk->start_node = p_lnk->start_node;
		if (fields & LWN_COL_LINK_END_NODE)
		    lnk->end_node = p_lnk->end_node;
		if (fields & LWN_COL_LINK_GEOM)
		    lnk->geom =
			gaianet_convert_linestring_to_lwnline (p_lnk->geom,
							       accessor->srid,
							       accessor->has_z);
		i++;
		p_lnk = p_lnk->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_links_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_links_list (list);
    *numelems = -1;
    return NULL;
}

int
netcallback_deleteNetNodesById (const LWN_BE_NETWORK * lwn_net,
				const LWN_ELEMID * ids, int numelems)
{
/* callback function: deleteNetNodesById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_deleteNetNodesById;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numelems; i++)
      {
	  /* parameter binding */
	  sqlite3_int64 id = *(ids + i);
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		changed += sqlite3_changes (accessor->db_handle);
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_deleteNetNodesById: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return changed;

  error:
    sqlite3_reset (stmt);
    return -1;
}

LWN_NET_NODE *
netcallback_getNetNodeWithinBox2D (const LWN_BE_NETWORK * lwn_net,
				   const LWN_BBOX * box, int *numelems,
				   int fields, int limit)
{
/* callback function: getNetNodeWithinBox2D */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt;
    int ret;
    int count = 0;
    sqlite3_stmt *stmt_aux = NULL;
    char *sql;
    struct net_nodes_list *list = NULL;
    LWN_NET_NODE *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    stmt = accessor->stmt_getNetNodeWithinBox2D;
    if (stmt == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    if (limit >= 0)
      {
	  /* preparing the auxiliary SQL statement */
	  sql =
	      do_prepare_read_net_node (accessor->network_name, fields,
					accessor->spatial, accessor->has_z);
	  ret =
	      sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql),
				  &stmt_aux, NULL);
	  sqlite3_free (sql);
	  if (ret != SQLITE_OK)
	    {
		char *msg =
		    sqlite3_mprintf
		    ("Prepare_getNetNodeWithinBox2D AUX error: \"%s\"",
		     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		*numelems = -1;
		return NULL;
	    }
      }

/* setting up the prepared statement */
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, box->min_x);
    sqlite3_bind_double (stmt, 2, box->min_y);
    sqlite3_bind_double (stmt, 3, box->max_x);
    sqlite3_bind_double (stmt, 4, box->max_y);
    list = create_nodes_list ();

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		sqlite3_int64 node_id = sqlite3_column_int64 (stmt, 0);
		if (stmt_aux != NULL)
		  {
		      char *msg;
		      if (!do_read_net_node
			  (stmt_aux, list, node_id, fields, accessor->spatial,
			   accessor->has_z, "netcallback_getNetNodeWithinBox2D",
			   &msg))
			{
			    gaianet_set_last_error_msg (net, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
		count++;
		if (limit > 0)
		  {
		      if (count > limit)
			  break;
		  }
		if (limit < 0)
		    break;
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_getNetNodeWithinBox2D: %s",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (limit < 0)
      {
	  result = NULL;
	  *numelems = count;
      }
    else
      {
	  if (list->count <= 0)
	    {
		result = NULL;
		*numelems = 0;
	    }
	  else
	    {
		int i = 0;
		struct net_node *p_nd;
		result = malloc (sizeof (LWN_NET_NODE) * list->count);
		p_nd = list->first;
		while (p_nd != NULL)
		  {
		      LWN_NET_NODE *nd = result + i;
		      nd->geom = NULL;
		      if (fields & LWN_COL_NODE_NODE_ID)
			  nd->node_id = p_nd->node_id;
		      if (fields & LWN_COL_NODE_GEOM)
			{
			    if (p_nd->is_null)
				;
			    else
			      {
				  if (accessor->has_z)
				      nd->geom =
					  lwn_create_point3d (accessor->srid,
							      p_nd->x, p_nd->y,
							      p_nd->z);
				  else
				      nd->geom =
					  lwn_create_point2d (accessor->srid,
							      p_nd->x, p_nd->y);
			      }
			}
		      i++;
		      p_nd = p_nd->next;
		  }
		*numelems = list->count;
	    }
      }

    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    destroy_net_nodes_list (list);
    sqlite3_reset (stmt);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_net_nodes_list (list);
    *numelems = 1;
    sqlite3_reset (stmt);
    return NULL;
}

LWN_ELEMID
netcallback_getNextLinkId (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: getNextLinkId */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt_in;
    sqlite3_stmt *stmt_out;
    int ret;
    sqlite3_int64 link_id = -1;
    if (accessor == NULL)
	return -1;

    stmt_in = accessor->stmt_getNextLinkId;
    if (stmt_in == NULL)
	return -1;
    stmt_out = accessor->stmt_setNextLinkId;
    if (stmt_out == NULL)
	return -1;

/* setting up the prepared statement */
    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	      link_id = sqlite3_column_int64 (stmt_in, 0);
	  else
	    {
		char *msg = sqlite3_mprintf ("netcallback_getNextLinkId: %s",
					     sqlite3_errmsg
					     (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto stop;
	    }
      }

/* updating next_link_id */
    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
      {
	  sqlite3_reset (stmt_in);
	  sqlite3_reset (stmt_out);
	  return link_id;
      }
    else
      {
	  char *msg = sqlite3_mprintf ("netcallback_setNextLinkId: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  link_id = -1;
      }
  stop:
    sqlite3_reset (stmt_in);
    sqlite3_reset (stmt_out);
    if (link_id >= 0)
	link_id++;
    return link_id;
}

int
netcallback_updateLinksById (const LWN_BE_NETWORK * lwn_net,
			     const LWN_LINK * links, int numlinks,
			     int upd_fields)
{
/* callback function: updateLinksById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt = NULL;
    gaiaGeomCollPtr geom;
    int ret;
    char *sql;
    char *prev;
    int comma = 0;
    char *table;
    char *xtable;
    int i;
    int changed = 0;
    unsigned char *p_blob;
    int n_bytes;
    int gpkg_mode = 0;
    int tiny_point = 0;
    if (accessor == NULL)
	return -1;

    if (accessor->cache != NULL)
      {
	  struct splite_internal_cache *cache =
	      (struct splite_internal_cache *) (accessor->cache);
	  gpkg_mode = cache->gpkg_mode;
	  tiny_point = cache->tinyPointEnabled;
      }

/* composing the SQL prepared statement */
    table = sqlite3_mprintf ("%s_link", accessor->network_name);
    xtable = gaiaDoubleQuotedSql (table);
    sqlite3_free (table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET", xtable);
    free (xtable);
    prev = sql;
    if (upd_fields & LWN_COL_LINK_LINK_ID)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, link_id = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s link_id = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & LWN_COL_LINK_START_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, start_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s start_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & LWN_COL_LINK_END_NODE)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, end_node = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s end_node = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    if (upd_fields & LWN_COL_LINK_GEOM)
      {
	  if (comma)
	      sql = sqlite3_mprintf ("%s, geometry = ?", prev);
	  else
	      sql = sqlite3_mprintf ("%s geometry = ?", prev);
	  comma = 1;
	  sqlite3_free (prev);
	  prev = sql;
      }
    sql = sqlite3_mprintf ("%s WHERE link_id = ?", prev);
    sqlite3_free (prev);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_updateLinksById error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  return -1;
      }

    for (i = 0; i < numlinks; i++)
      {
	  /* parameter binding */
	  int icol = 1;
	  const LWN_LINK *upd_link = links + i;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (upd_fields & LWN_COL_LINK_LINK_ID)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_link->link_id);
		icol++;
	    }
	  if (upd_fields & LWN_COL_LINK_START_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_link->start_node);
		icol++;
	    }
	  if (upd_fields & LWN_COL_LINK_END_NODE)
	    {
		sqlite3_bind_int64 (stmt, icol, upd_link->end_node);
		icol++;
	    }
	  if (upd_fields & LWN_COL_LINK_GEOM)
	    {
		if (upd_link->geom == NULL)
		    sqlite3_bind_null (stmt, icol);
		else
		  {
		      /* transforming the LWN_LINE into a Geometry-Linestring */
		      geom =
			  do_convert_lwnline_to_geom (upd_link->geom,
						      accessor->srid);
		      gaiaToSpatiaLiteBlobWkbEx2 (geom, &p_blob, &n_bytes,
						  gpkg_mode, tiny_point);
		      gaiaFreeGeomColl (geom);
		      sqlite3_bind_blob (stmt, icol, p_blob, n_bytes, free);
		  }
		icol++;
	    }
	  sqlite3_bind_int64 (stmt, icol, upd_link->link_id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      changed += sqlite3_changes (accessor->db_handle);
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_updateLinksById: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    return changed;

  error:
    sqlite3_finalize (stmt);
    return -1;
}

LWN_LINK *
netcallback_getLinkById (const LWN_BE_NETWORK * lwn_net,
			 const LWN_ELEMID * ids, int *numelems, int fields)
{
/* callback function: getLinkById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt_aux = NULL;
    int ret;
    int i;
    char *sql;
    struct net_links_list *list = NULL;
    LWN_LINK *result = NULL;
    if (accessor == NULL)
      {
	  *numelems = -1;
	  return NULL;
      }

    /* preparing the SQL statement */
    sql = do_prepare_read_link (accessor->network_name, fields);
    ret =
	sqlite3_prepare_v2 (accessor->db_handle, sql, strlen (sql), &stmt_aux,
			    NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  char *msg = sqlite3_mprintf ("Prepare_getLinkById AUX error: \"%s\"",
				       sqlite3_errmsg (accessor->db_handle));
	  gaianet_set_last_error_msg (net, msg);
	  sqlite3_free (msg);
	  *numelems = -1;
	  return NULL;
      }

    list = create_links_list ();
    for (i = 0; i < *numelems; i++)
      {
	  char *msg;
	  if (!do_read_link
	      (stmt_aux, list, *(ids + i), fields, "netcallback_getLinkById",
	       &msg))
	    {
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (list->count == 0)
      {
	  /* no link was found */
	  *numelems = list->count;
      }
    else
      {
	  struct net_link *p_lnk;
	  result = malloc (sizeof (LWN_LINK) * list->count);
	  p_lnk = list->first;
	  i = 0;
	  while (p_lnk != NULL)
	    {
		LWN_LINK *lnk = result + i;
		lnk->geom = NULL;
		if (fields & LWN_COL_LINK_LINK_ID)
		    lnk->link_id = p_lnk->link_id;
		if (fields & LWN_COL_LINK_START_NODE)
		    lnk->start_node = p_lnk->start_node;
		if (fields & LWN_COL_LINK_END_NODE)
		    lnk->end_node = p_lnk->end_node;
		if (fields & LWN_COL_LINK_GEOM)
		  {
		      if (p_lnk->geom == NULL)
			  lnk->geom = NULL;
		      else
			  lnk->geom =
			      gaianet_convert_linestring_to_lwnline
			      (p_lnk->geom, accessor->srid, accessor->has_z);
		  }
		i++;
		p_lnk = p_lnk->next;
	    }
	  *numelems = list->count;
      }
    sqlite3_finalize (stmt_aux);
    destroy_links_list (list);
    return result;

  error:
    if (stmt_aux != NULL)
	sqlite3_finalize (stmt_aux);
    if (list != NULL)
	destroy_links_list (list);
    *numelems = -1;
    return NULL;
}

int
netcallback_deleteLinksById (const LWN_BE_NETWORK * lwn_net,
			     const LWN_ELEMID * ids, int numelems)
{
/* callback function: deleteLinksById */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int i;
    int changed = 0;
    if (accessor == NULL)
	return -1;

    stmt = accessor->stmt_deleteLinksById;
    if (stmt == NULL)
	return -1;

    for (i = 0; i < numelems; i++)
      {
	  /* parameter binding */
	  sqlite3_int64 id = *(ids + i);
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_int64 (stmt, 1, id);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	    {
		changed += sqlite3_changes (accessor->db_handle);
	    }
	  else
	    {
		char *msg =
		    sqlite3_mprintf ("netcallback_deleteLinksById: \"%s\"",
				     sqlite3_errmsg (accessor->db_handle));
		gaianet_set_last_error_msg (net, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }
    sqlite3_reset (stmt);
    return changed;

  error:
    sqlite3_reset (stmt);
    return -1;
}

int
netcallback_netGetSRID (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: netGetSRID */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    if (accessor == NULL)
	return -1;

    return accessor->srid;
}

int
netcallback_netHasZ (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: netHasZ */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    if (accessor == NULL)
	return 0;

    return accessor->has_z;
}

int
netcallback_netIsSpatial (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: netIsSpatial */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    if (accessor == NULL)
	return 0;

    return accessor->spatial;
}

int
netcallback_netAllowCoincident (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: netAllowCoincident */
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    if (accessor == NULL)
	return 0;

    return accessor->allow_coincident;
}

const void *
netcallback_netGetGEOS (const LWN_BE_NETWORK * lwn_net)
{
/* callback function: netGetGEOS */
    const struct splite_internal_cache *cache;
    GaiaNetworkAccessorPtr net = (GaiaNetworkAccessorPtr) lwn_net;
    struct gaia_network *accessor = (struct gaia_network *) net;
    if (accessor == NULL)
	return NULL;
    if (accessor->cache == NULL)
	return NULL;

    cache = (const struct splite_internal_cache *) (accessor->cache);
    return cache->GEOS_handle;
}

#endif /* end RTTOPO conditionals */
