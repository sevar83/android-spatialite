/*

 lwn_network.c -- Topology-Network abstract multi-backend interface
    
 version 4.3, 2015 August 13

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
#include <float.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifdef ENABLE_RTTOPO		/* only if RTTOPO is enabled */

#include <librttopo_geom.h>

#include "lwn_network.h"
#include "lwn_network_private.h"

#ifdef GEOS_REENTRANT
#ifdef GEOS_ONLY_REENTRANT
#define GEOS_USE_ONLY_R_API	/* only fully thread-safe GEOS API */
#endif
#endif
#include <geos_c.h>

#include <librttopo_geom.h>

#include <spatialite_private.h>

/*********************************************************************
 *
 * Backend iface
 *
 ********************************************************************/

LWN_BE_IFACE *
lwn_CreateBackendIface (const RTCTX * ctx, const LWN_BE_DATA * data)
{
    LWN_BE_IFACE *iface = malloc (sizeof (LWN_BE_IFACE));
    iface->ctx = ctx;
    iface->data = data;
    iface->cb = NULL;
    iface->errorMsg = NULL;
    return iface;
}

void
lwn_BackendIfaceRegisterCallbacks (LWN_BE_IFACE * iface,
				   const LWN_BE_CALLBACKS * cb)
{
    iface->cb = cb;
}

void
lwn_FreeBackendIface (LWN_BE_IFACE * iface)
{
    if (iface == NULL)
	return;
    if (iface->errorMsg != NULL)
	free (iface->errorMsg);
    free (iface);
}

/*********************************************************************
 *
 * Backend wrappers
 *
 ********************************************************************/

#define CHECKCB(be, method) do { \
  if ( ! (be)->cb || ! (be)->cb->method ) \
  lwn_SetErrorMsg(be, "Callback " # method " not registered by backend"); \
} while (0)

#define NETCB0(be, method) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data)

#define NETCB1(be, method, a1) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data, a1)

#define NETCBT0(to, method) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net)

#define NETCBT1(to, method, a1) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net, a1)

#define NETCBT2(to, method, a1, a2) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net, a1, a2)

#define NETCBT3(to, method, a1, a2, a3) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net, a1, a2, a3)

#define NETCBT4(to, method, a1, a2, a3, a4) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net, a1, a2, a3, a4)

#define NETCBT5(to, method, a1, a2, a3, a4, a5) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_net, a1, a2, a3, a4, a5)


static LWN_BE_NETWORK *
lwn_be_loadNetworkByName (LWN_BE_IFACE * be, const char *name)
{
    NETCB1 (be, loadNetworkByName, name);
}

static int
lwn_be_netGetSRID (LWN_NETWORK * net)
{
    NETCBT0 (net, netGetSRID);
}

static int
lwn_be_netHasZ (LWN_NETWORK * net)
{
    NETCBT0 (net, netHasZ);
}

static int
lwn_be_netIsSpatial (LWN_NETWORK * net)
{
    NETCBT0 (net, netIsSpatial);
}

static int
lwn_be_netAllowCoincident (LWN_NETWORK * net)
{
    NETCBT0 (net, netAllowCoincident);
}

static const void *
lwn_be_netGetGEOS (LWN_NETWORK * net)
{
    NETCBT0 (net, netGetGEOS);
}

static int
lwn_be_freeNetwork (LWN_NETWORK * net)
{
    NETCBT0 (net, freeNetwork);
}

static LWN_NET_NODE *
lwn_be_getNetNodeWithinDistance2D (const LWN_NETWORK * net,
				   const LWN_POINT * pt, double dist,
				   int *numelems, int fields, int limit)
{
    NETCBT5 (net, getNetNodeWithinDistance2D, pt, dist, numelems, fields,
	     limit);
}

static LWN_LINK *
lwn_be_getLinkWithinDistance2D (const LWN_NETWORK * net, const LWN_POINT * pt,
				double dist, int *numelems, int fields,
				int limit)
{
    NETCBT5 (net, getLinkWithinDistance2D, pt, dist, numelems, fields, limit);
}

static int
lwn_be_updateNetNodesById (const LWN_NETWORK * net,
			   const LWN_NET_NODE * nodes, int numnodes,
			   int upd_fields)
{
    NETCBT3 (net, updateNetNodesById, nodes, numnodes, upd_fields);
}

static int
lwn_be_insertNetNodes (const LWN_NETWORK * net, LWN_NET_NODE * node,
		       int numelems)
{
    NETCBT2 (net, insertNetNodes, node, numelems);
}

static LWN_NET_NODE *
lwn_be_getNetNodeById (const LWN_NETWORK * net, const LWN_ELEMID * ids,
		       int *numelems, int fields)
{
    NETCBT3 (net, getNetNodeById, ids, numelems, fields);
}

static LWN_LINK *
lwn_be_getLinkByNetNode (const LWN_NETWORK * net, const LWN_ELEMID * ids,
			 int *numelems, int fields)
{
    NETCBT3 (net, getLinkByNetNode, ids, numelems, fields);
}

static int
lwn_be_deleteNetNodesById (const LWN_NETWORK * net, const LWN_ELEMID * ids,
			   int numelems)
{
    NETCBT2 (net, deleteNetNodesById, ids, numelems);
}

static LWN_ELEMID
lwn_be_getNextLinkId (const LWN_NETWORK * net)
{
    NETCBT0 (net, getNextLinkId);
}

static LWN_NET_NODE *
lwn_be_getNetNodeWithinBox2D (const LWN_NETWORK * net,
			      const LWN_BBOX * box, int *numelems, int fields,
			      int limit)
{
    NETCBT4 (net, getNetNodeWithinBox2D, box, numelems, fields, limit);
}

static int
lwn_be_insertLinks (const LWN_NETWORK * net, LWN_LINK * link, int numelems)
{
    NETCBT2 (net, insertLinks, link, numelems);
}

static int
lwn_be_updateLinksById (const LWN_NETWORK * net, LWN_LINK * links, int numlinks,
			int upd_fields)
{
    NETCBT3 (net, updateLinksById, links, numlinks, upd_fields);
}

static LWN_LINK *
lwn_be_getLinkById (const LWN_NETWORK * net, const LWN_ELEMID * ids,
		    int *numelems, int fields)
{
    NETCBT3 (net, getLinkById, ids, numelems, fields);
}

static int
lwn_be_deleteLinksById (const LWN_NETWORK * net, const LWN_ELEMID * ids,
			int numelems)
{
    NETCBT2 (net, deleteLinksById, ids, numelems);
}


/************************************************************************
 *
 * API implementation
 *
 ************************************************************************/

LWN_NETWORK *
lwn_LoadNetwork (LWN_BE_IFACE * iface, const char *name)
{
    LWN_BE_NETWORK *be_net;
    LWN_NETWORK *net;

    be_net = lwn_be_loadNetworkByName (iface, name);
    if (!be_net)
      {
	  lwn_SetErrorMsg (iface, "Could not load network from backend");
	  return NULL;
      }
    net = malloc (sizeof (LWN_NETWORK));
    net->be_iface = iface;
    net->be_net = be_net;
    net->srid = lwn_be_netGetSRID (net);
    net->hasZ = lwn_be_netHasZ (net);
    net->spatial = lwn_be_netIsSpatial (net);
    net->allowCoincident = lwn_be_netAllowCoincident (net);
    net->geos_handle = lwn_be_netGetGEOS (net);
    return net;
}

void
lwn_FreeNetwork (LWN_NETWORK * net)
{
    if (!lwn_be_freeNetwork (net))
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "Could not release backend topology memory");
      }
    free (net);
}

LWN_POINT *
lwn_create_point2d (int srid, double x, double y)
{
    LWN_POINT *point = malloc (sizeof (LWN_POINT));
    point->srid = srid;
    point->has_z = 0;
    point->x = x;
    point->y = y;
    return point;
}

LWN_POINT *
lwn_create_point3d (int srid, double x, double y, double z)
{
    LWN_POINT *point = malloc (sizeof (LWN_POINT));
    point->srid = srid;
    point->has_z = 1;
    point->x = x;
    point->y = y;
    point->z = z;
    return point;
}

void
lwn_free_point (LWN_POINT * point)
{
    if (point == NULL)
	return;
    free (point);
}

LWN_LINE *
lwn_alloc_line (int points, int srid, int hasz)
{
    LWN_LINE *line = malloc (sizeof (LWN_LINE));
    line->points = points;
    line->srid = srid;
    line->has_z = hasz;
    line->x = malloc (sizeof (double) * points);
    line->y = malloc (sizeof (double) * points);
    if (hasz)
	line->z = malloc (sizeof (double) * points);
    else
	line->z = NULL;
    return line;
}

void
lwn_free_line (LWN_LINE * line)
{
    if (line == NULL)
	return;
    if (line->x != NULL)
	free (line->x);
    if (line->y != NULL)
	free (line->y);
    if (line->z != NULL && line->has_z)
	free (line->z);
    free (line);
}

void
lwn_ResetErrorMsg (LWN_BE_IFACE * iface)
{
    if (iface == NULL)
	return;
    if (iface->errorMsg != NULL)
	free (iface->errorMsg);
    iface->errorMsg = NULL;
}

void
lwn_SetErrorMsg (LWN_BE_IFACE * iface, const char *message)
{
    int len;
    if (iface == NULL)
	return;
    if (iface->errorMsg != NULL)
	free (iface->errorMsg);
    iface->errorMsg = NULL;
    if (message == NULL)
	return;
    len = strlen (message);
    iface->errorMsg = malloc (len + 1);
    strcpy (iface->errorMsg, message);
}

const char *
lwn_GetErrorMsg (LWN_BE_IFACE * iface)
{
    if (iface == NULL)
	return NULL;
    return iface->errorMsg;
}

static void
_lwn_release_nodes (LWN_NET_NODE * nodes, int num_nodes)
{
    int i;
    for (i = 0; i < num_nodes; ++i)
      {
	  if (nodes[i].geom != NULL)
	      lwn_free_point (nodes[i].geom);
      }
    free (nodes);
}

static void
_lwn_release_links (LWN_LINK * links, int num_links)
{
    int i;
    for (i = 0; i < num_links; ++i)
      {
	  if (links[i].geom != NULL)
	      lwn_free_line (links[i].geom);
      }
    free (links);
}

LWN_ELEMID
lwn_AddIsoNetNode (LWN_NETWORK * net, LWN_POINT * pt)
{
    LWN_NET_NODE node;

    if (net->spatial && net->allowCoincident == 0)
      {
	  if (lwn_be_existsCoincidentNode (net, pt))
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - coincident node.");
		return -1;
	    }
	  if (lwn_be_existsLinkIntersectingPoint (net, pt))
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - link crosses node.");
		return -1;
	    }
      }

    node.node_id = -1;
    node.geom = pt;
    if (!lwn_be_insertNetNodes (net, &node, 1))
	return -1;

    return node.node_id;
}

static LWN_NET_NODE *
_lwn_GetIsoNetNode (LWN_NETWORK * net, LWN_ELEMID nid)
{
    LWN_NET_NODE *node;
    int n = 1;
    LWN_LINK *links;
    int nlinks;

    node = lwn_be_getNetNodeById (net, &nid, &n, LWN_COL_NODE_NODE_ID);
    if (n < 0)
	return 0;
    if (n < 1)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent node.");
	  return 0;
      }

    nlinks = 1;
    links = lwn_be_getLinkByNetNode (net, &nid, &nlinks, LWN_COL_LINK_LINK_ID);
    if (nlinks < 0)
	return 0;
    if (nlinks != 0)
      {
	  free (node);
	  _lwn_release_links (links, nlinks);
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - not isolated node.");
	  return 0;
      }

    return node;
}

static LWN_LINK *
_lwn_GetLink (LWN_NETWORK * net, LWN_ELEMID link_id)
{
    LWN_LINK *link;
    int n = 1;

    link = lwn_be_getLinkById (net, &link_id, &n, LWN_COL_LINK_LINK_ID);
    if (n < 0)
	return 0;
    if (n < 1)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent link.");
	  return 0;
      }
    return link;
}

static int
line2bbox (const LWN_LINE * line, LWN_BBOX * bbox)
{
    int iv;
    if (line == NULL)
	return 0;

    bbox->min_x = DBL_MAX;
    bbox->min_y = DBL_MAX;
    bbox->max_x = -DBL_MAX;
    bbox->max_y = -DBL_MAX;

    for (iv = 0; iv < line->points; iv++)
      {
	  double x = line->x[iv];
	  double y = line->y[iv];
	  if (x < bbox->min_x)
	      bbox->min_x = x;
	  if (y < bbox->min_y)
	      bbox->min_y = y;
	  if (x > bbox->max_x)
	      bbox->max_x = x;
	  if (y > bbox->max_y)
	      bbox->max_y = y;
      }

    return 1;
}


static GEOSGeometry *
point2geos (GEOSContextHandle_t handle, const LWN_POINT * point)
{
    GEOSGeometry *geos = NULL;
    GEOSCoordSequence *cs;
    cs = GEOSCoordSeq_create_r (handle, 1, 2);
    GEOSCoordSeq_setX_r (handle, cs, 0, point->x);
    GEOSCoordSeq_setY_r (handle, cs, 0, point->y);
    geos = GEOSGeom_createPoint_r (handle, cs);
    return geos;
}

static GEOSGeometry *
line2geos (GEOSContextHandle_t handle, const LWN_LINE * line)
{
    int iv;
    GEOSGeometry *geos = NULL;
    GEOSCoordSequence *cs;
    cs = GEOSCoordSeq_create_r (handle, line->points, 2);
    for (iv = 0; iv < line->points; iv++)
      {
	  GEOSCoordSeq_setX_r (handle, cs, iv, line->x[iv]);
	  GEOSCoordSeq_setY_r (handle, cs, iv, line->y[iv]);
      }
    geos = GEOSGeom_createLineString_r (handle, cs);
    return geos;
}

/* Check that a link does not cross an existing node
 *
 * Return -1 on cross or error, 0 if everything is fine.
 * Note that before returning -1, lwerror is invoked...
 */
static int
_lwn_CheckLinkCrossing (LWN_NETWORK * net,
			LWN_ELEMID start_node, LWN_ELEMID end_node,
			const LWN_LINE * geom)
{
    int i, num_nodes;
    LWN_NET_NODE *nodes;
    LWN_BBOX linkbbox;
    GEOSContextHandle_t handle = (GEOSContextHandle_t) (net->geos_handle);
    GEOSGeometry *linkgg;
    const GEOSPreparedGeometry *prepared_link;

    linkgg = line2geos (handle, geom);
    if (!linkgg)
	return -1;
    prepared_link = GEOSPrepare_r (handle, linkgg);
    if (!prepared_link)
	return -1;
    if (!line2bbox (geom, &linkbbox))
      {
	  GEOSPreparedGeom_destroy_r (handle, prepared_link);
	  GEOSGeom_destroy_r (handle, linkgg);
	  return -1;
      }

    /* loop over each node within the edge's gbox */
    nodes = lwn_be_getNetNodeWithinBox2D (net, &linkbbox, &num_nodes,
					  LWN_COL_NODE_ALL, 0);
    if (num_nodes == -1)
      {
	  GEOSPreparedGeom_destroy_r (handle, prepared_link);
	  GEOSGeom_destroy_r (handle, linkgg);
	  return -1;
      }
    for (i = 0; i < num_nodes; ++i)
      {
	  LWN_NET_NODE *node = &(nodes[i]);
	  GEOSGeometry *nodegg;
	  int contains;
	  if (node->node_id == start_node)
	      continue;
	  if (node->node_id == end_node)
	      continue;
	  /* check if the link contains this node (not on boundary) */
	  nodegg = point2geos (handle, node->geom);
	  /* ST_RelateMatch(rec.relate, 'T********') */
	  contains = GEOSPreparedContains_r (handle, prepared_link, nodegg);
	  GEOSGeom_destroy_r (handle, nodegg);
	  if (contains == 2)
	    {
		GEOSPreparedGeom_destroy_r (handle, prepared_link);
		GEOSGeom_destroy_r (handle, linkgg);
		_lwn_release_nodes (nodes, num_nodes);
		lwn_SetErrorMsg (net->be_iface,
				 "GEOS exception on PreparedContains");
		return -1;
	    }
	  if (contains)
	    {
		GEOSPreparedGeom_destroy_r (handle, prepared_link);
		GEOSGeom_destroy_r (handle, linkgg);
		_lwn_release_nodes (nodes, num_nodes);
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - geometry crosses a node.");
		return -1;
	    }
      }
    if (nodes)
	_lwn_release_nodes (nodes, num_nodes);
    GEOSPreparedGeom_destroy_r (handle, prepared_link);
    GEOSGeom_destroy_r (handle, linkgg);

    return 0;
}

int
lwn_MoveIsoNetNode (LWN_NETWORK * net, LWN_ELEMID nid, const LWN_POINT * pt)
{
    LWN_NET_NODE *node;
    int ret;

    node = _lwn_GetIsoNetNode (net, nid);
    if (!node)
	return -1;

    if (net->spatial && net->allowCoincident == 0)
      {
	  if (lwn_be_existsCoincidentNode (net, pt))
	    {
		_lwn_release_nodes (node, 1);
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - coincident node.");
		return -1;
	    }

	  if (lwn_be_existsLinkIntersectingPoint (net, pt))
	    {
		_lwn_release_nodes (node, 1);
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - link crosses node.");
		return -1;
	    }
      }

    node->node_id = nid;
    if (node->geom)
	lwn_free_point (node->geom);
    node->geom = (LWN_POINT *) pt;
    ret = lwn_be_updateNetNodesById (net, node, 1, LWN_COL_NODE_GEOM);
    node->geom = NULL;
    _lwn_release_nodes (node, 1);
    if (ret == -1)
	return -1;

    return 0;
}

int
lwn_RemIsoNetNode (LWN_NETWORK * net, LWN_ELEMID nid)
{
    LWN_NET_NODE *node;
    int n = 1;

    node = _lwn_GetIsoNetNode (net, nid);
    if (!node)
	return -1;

    n = lwn_be_deleteNetNodesById (net, &nid, n);
    if (n == -1)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - not isolated node.");
	  return -1;
      }
    if (n != 1)
	return -1;

    free (node);
    return 0;
}

static int
getLineFirstPoint (const LWN_LINE * geom, LWN_POINT * pt)
{
    if (geom == NULL)
	return 0;
    pt->srid = geom->srid;
    pt->has_z = geom->has_z;
    pt->x = geom->x[0];
    pt->y = geom->y[0];
    if (geom->has_z)
	pt->z = geom->z[0];
    return 1;
}

static int
getLineLastPoint (const LWN_LINE * geom, LWN_POINT * pt)
{
    int iv;
    if (geom == NULL)
	return 0;
    iv = geom->points - 1;
    pt->srid = geom->srid;
    pt->has_z = geom->has_z;
    pt->x = geom->x[iv];
    pt->y = geom->y[iv];
    if (geom->has_z)
	pt->z = geom->z[iv];
    return 1;
}

static int
point_same_2d (LWN_POINT * pt1, LWN_POINT * pt2)
{
    if (pt1->x == pt2->x && pt1->y == pt2->y)
	return 1;
    return 0;
}

LWN_ELEMID
lwn_AddLink (LWN_NETWORK * net,
	     LWN_ELEMID startNode, LWN_ELEMID endNode, LWN_LINE * geom)
{
    int num_nodes;
    int i;
    LWN_LINK newlink;
    LWN_NET_NODE *endpoints;
    LWN_ELEMID *node_ids;
    LWN_POINT pt;

    /* NOT IN THE SPECS:
     * A closed link makes no sense and should always be forbidden
     */
    if (startNode == endNode)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - self-closed links are forbidden.");
	  return -1;
      }

    /*
     * Check for:
     *    existence of nodes
     * Extract:
     *    nodes geoms
     */
    num_nodes = 2;
    node_ids = malloc (sizeof (LWN_ELEMID) * num_nodes);
    node_ids[0] = startNode;
    node_ids[1] = endNode;
    endpoints = lwn_be_getNetNodeById (net, node_ids, &num_nodes,
				       LWN_COL_NODE_ALL);
    if (num_nodes < 0)
	return -1;
    else if (num_nodes < 2)
      {
	  if (num_nodes)
	      _lwn_release_nodes (endpoints, num_nodes);
	  free (node_ids);
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent node.");
	  return -1;
      }
    for (i = 0; i < num_nodes; ++i)
      {
	  const LWN_NET_NODE *n = &(endpoints[i]);
	  if (net->spatial)
	    {
		if (n->geom == NULL)
		    return -1;
		if (n->node_id == startNode)
		  {
		      /* l) Check that start point of acurve match start node geoms. */
		      if (!getLineFirstPoint (geom, &pt))
			  return -1;
		      if (!point_same_2d (&pt, n->geom))
			{
			    _lwn_release_nodes (endpoints, num_nodes);
			    free (node_ids);
			    lwn_SetErrorMsg (net->be_iface,
					     "SQL/MM Spatial exception - "
					     "start node not geometry start point.");
			    return -1;
			}
		  }
		else
		  {
		      /* m) Check that end point of acurve match end node geoms. */
		      if (!getLineLastPoint (geom, &pt))
			  return -1;
		      if (!point_same_2d (&pt, n->geom))
			{
			    _lwn_release_nodes (endpoints, num_nodes);
			    free (node_ids);
			    lwn_SetErrorMsg (net->be_iface,
					     "SQL/MM Spatial exception - "
					     "end node not geometry end point.");
			    return -1;
			}
		  }
	    }
      }
    _lwn_release_nodes (endpoints, num_nodes);
    free (node_ids);

    if (net->spatial && net->allowCoincident == 0)
      {
	  if (_lwn_CheckLinkCrossing (net, startNode, endNode, geom))
	      return -1;
      }

    /*
     * All checks passed, time to prepare the new link
     */

    newlink.link_id = lwn_be_getNextLinkId (net);
    if (newlink.link_id == -1)
	return -1;

    newlink.start_node = startNode;
    newlink.end_node = endNode;
    newlink.geom = geom;

    if (!lwn_be_insertLinks (net, &newlink, 1))
	return -1;

    return newlink.link_id;
}

int
lwn_ChangeLinkGeom (LWN_NETWORK * net, LWN_ELEMID link, const LWN_LINE * geom)
{
    int num_nodes;
    int i;
    LWN_LINK newlink;
    LWN_LINK *oldlink;
    LWN_NET_NODE *endpoints;
    LWN_ELEMID *node_ids;
    LWN_POINT pt;
    LWN_ELEMID startNode;
    LWN_ELEMID endNode;
    int ret;

    i = 1;
    oldlink =
	lwn_be_getLinkById (net, &link, &i,
			    LWN_COL_LINK_START_NODE | LWN_COL_LINK_END_NODE);
    if (!oldlink)
      {
	  if (i == -1)
	      return -1;
	  else if (i == 0)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-existent link.");
		return -1;
	    }
      }
    startNode = oldlink->start_node;
    endNode = oldlink->end_node;
    _lwn_release_links (oldlink, 1);

    /*
     * Check for:
     *    existence of nodes
     * Extract:
     *    nodes geoms
     */
    num_nodes = 2;
    node_ids = malloc (sizeof (LWN_ELEMID) * num_nodes);
    node_ids[0] = startNode;
    node_ids[1] = endNode;
    endpoints = lwn_be_getNetNodeById (net, node_ids, &num_nodes,
				       LWN_COL_NODE_ALL);
    if (num_nodes < 0)
	return -1;
    else if (num_nodes < 2)
      {
	  if (num_nodes)
	      _lwn_release_nodes (endpoints, num_nodes);
	  free (node_ids);
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent node.");
	  return -1;
      }
    for (i = 0; i < num_nodes; ++i)
      {
	  const LWN_NET_NODE *n = &(endpoints[i]);
	  if (net->spatial)
	    {
		if (n->geom == NULL)
		    return -1;
		if (n->node_id == startNode)
		  {
		      /* l) Check that start point of acurve match start node geoms. */
		      if (!getLineFirstPoint (geom, &pt))
			  return -1;
		      if (!point_same_2d (&pt, n->geom))
			{
			    _lwn_release_nodes (endpoints, num_nodes);
			    free (node_ids);
			    lwn_SetErrorMsg (net->be_iface,
					     "SQL/MM Spatial exception - "
					     "start node not geometry start point.");
			    return -1;
			}
		  }
		else
		  {
		      /* m) Check that end point of acurve match end node geoms. */
		      if (!getLineLastPoint (geom, &pt))
			  return -1;
		      if (!point_same_2d (&pt, n->geom))
			{
			    _lwn_release_nodes (endpoints, num_nodes);
			    free (node_ids);
			    lwn_SetErrorMsg (net->be_iface,
					     "SQL/MM Spatial exception - "
					     "end node not geometry end point.");
			    return -1;
			}
		  }
	    }
      }
    _lwn_release_nodes (endpoints, num_nodes);
    free (node_ids);

    if (net->spatial && net->allowCoincident == 0)
      {
	  if (_lwn_CheckLinkCrossing (net, startNode, endNode, geom))
	      return -1;
      }

    /*
     * All checks passed, time to prepare the new link
     */

    newlink.link_id = link;
    newlink.start_node = startNode;
    newlink.end_node = endNode;
    newlink.geom = (LWN_LINE *) geom;

    ret = lwn_be_updateLinksById (net, &newlink, 1, LWN_COL_LINK_GEOM);
    if (ret == -1)
	return -1;
    else if (ret == 0)
	return -1;

    return 0;
}

int
lwn_RemoveLink (LWN_NETWORK * net, LWN_ELEMID link_id)
{
    LWN_LINK *link;
    int n = 1;

    link = _lwn_GetLink (net, link_id);
    if (!link)
	return -1;

    n = lwn_be_deleteLinksById (net, &link_id, n);
    if (n != 1)
	return -1;

    free (link);
    return 0;
}

LWN_INT64
lwn_NewLogLinkSplit (LWN_NETWORK * net, LWN_ELEMID link)
{
    int i;
    LWN_LINK *oldlink;
    LWN_LINK newlink[2];
    LWN_ELEMID startNode;
    LWN_ELEMID endNode;
    LWN_NET_NODE newnode;

    i = 1;
    oldlink =
	lwn_be_getLinkById (net, &link, &i,
			    LWN_COL_LINK_START_NODE | LWN_COL_LINK_END_NODE);
    if (!oldlink)
      {
	  if (i == -1)
	      return -1;
	  else if (i == 0)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-existent link.");
		return -1;
	    }
      }
    startNode = oldlink->start_node;
    endNode = oldlink->end_node;
    _lwn_release_links (oldlink, 1);

/* inserting a new NetNode */
    newnode.node_id = -1;
    newnode.geom = NULL;
    if (!lwn_be_insertNetNodes (net, &newnode, 1))
	return -1;

/* deleting the original Link */
    i = lwn_be_deleteLinksById (net, &link, 1);
    if (i != 1)
	return -1;

/* inserting two new Links */
    newlink[0].link_id = lwn_be_getNextLinkId (net);
    if (newlink[0].link_id == -1)
	return -1;
    newlink[1].link_id = lwn_be_getNextLinkId (net);
    if (newlink[1].link_id == -1)
	return -1;

    newlink[0].start_node = startNode;
    newlink[0].end_node = newnode.node_id;
    newlink[0].geom = NULL;
    newlink[1].start_node = newnode.node_id;
    newlink[1].end_node = endNode;
    newlink[1].geom = NULL;

    if (!lwn_be_insertLinks (net, newlink, 2))
	return -1;

    return newnode.node_id;
}

LWN_INT64
lwn_ModLogLinkSplit (LWN_NETWORK * net, LWN_ELEMID link)
{
    int i;
    LWN_LINK *oldlink;
    LWN_LINK newlink;
    LWN_ELEMID startNode;
    LWN_ELEMID endNode;
    LWN_NET_NODE newnode;

    i = 1;
    oldlink =
	lwn_be_getLinkById (net, &link, &i,
			    LWN_COL_LINK_START_NODE | LWN_COL_LINK_END_NODE);
    if (!oldlink)
      {
	  if (i == -1)
	      return -1;
	  else if (i == 0)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-existent link.");
		return -1;
	    }
      }
    startNode = oldlink->start_node;
    endNode = oldlink->end_node;
    _lwn_release_links (oldlink, 1);

/* inserting a new NetNode */
    newnode.node_id = -1;
    newnode.geom = NULL;
    if (!lwn_be_insertNetNodes (net, &newnode, 1))
	return -1;

/* update the original Link */
    newlink.link_id = link;
    newlink.start_node = startNode;
    newlink.end_node = newnode.node_id;
    newlink.geom = NULL;
    if (!lwn_be_updateLinksById (net, &newlink, 1, LWN_COL_LINK_END_NODE))
	return -1;

/* inserting a new Link */
    newlink.link_id = lwn_be_getNextLinkId (net);
    if (newlink.link_id == -1)
	return -1;
    newlink.start_node = newnode.node_id;
    newlink.end_node = endNode;
    newlink.geom = NULL;

    if (!lwn_be_insertLinks (net, &newlink, 1))
	return -1;

    return newnode.node_id;
}

static int
geo_link_split (LWN_NETWORK * net, const LWN_LINE * oldline,
		const LWN_POINT * pt, LWN_LINE * newline1, LWN_LINE * newline2)
{
    const RTCTX *ctx = NULL;
    RTPOINTARRAY *pa;
    RTPOINT4D point;
    int iv;
    RTGEOM *rtg_ln;
    RTGEOM *rtg_pt;
    RTGEOM *split;
    RTCOLLECTION *split_col;
    RTGEOM *rtg = NULL;
    RTLINE *rtl = NULL;
    RTPOINT4D pt4d;
    int ret = 0;

    if (net == NULL)
	return 0;
    if (net->be_iface == NULL)
	return 0;
    ctx = net->be_iface->ctx;
    if (ctx == NULL)
	return 0;

/* creating an RTGEOM Linestring from oldline */
    pa = ptarray_construct (ctx, oldline->has_z, 0, oldline->points);
    for (iv = 0; iv < oldline->points; iv++)
      {
	  /* copying vertices */
	  point.x = oldline->x[iv];
	  point.y = oldline->y[iv];
	  if (oldline->has_z)
	      point.z = oldline->z[iv];
	  ptarray_set_point4d (ctx, pa, iv, &point);
      }
    rtg_ln = (RTGEOM *) rtline_construct (ctx, oldline->srid, NULL, pa);

/* creating an RTGEOM Point from pt */
    pa = ptarray_construct (ctx, pt->has_z, 0, 1);
    point.x = pt->x;
    point.y = pt->y;
    if (pt->has_z)
	point.z = pt->z;
    ptarray_set_point4d (ctx, pa, 0, &point);
    rtg_pt = (RTGEOM *) rtpoint_construct (ctx, oldline->srid, NULL, pa);

/* Split link */
    split = rtgeom_split (ctx, rtg_ln, rtg_pt);
    rtgeom_free (ctx, rtg_ln);
    rtgeom_free (ctx, rtg_pt);
    if (!split)
      {
	  lwn_SetErrorMsg (net->be_iface, "could not split link by point ?");
	  goto end;
      }
    split_col = rtgeom_as_rtcollection (ctx, split);
    if (!split_col)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "lwgeom_as_lwcollection returned NULL");
	  goto end;
      }
    if (split_col->ngeoms != 2)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - point not on link.");
	  goto end;
      }

/* retrieving the first half of the split link */
    rtg = split_col->geoms[0];
    if (rtg->type == RTLINETYPE)
      {
	  rtl = (RTLINE *) rtg;
	  pa = rtl->points;
	  newline1->points = pa->npoints;
	  newline1->x = malloc (sizeof (double) * newline1->points);
	  newline1->y = malloc (sizeof (double) * newline1->points);
	  if (newline1->has_z)
	      newline1->z = malloc (sizeof (double) * newline1->points);
	  for (iv = 0; iv < newline1->points; iv++)
	    {
		/* copying LINESTRING vertices */
		rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		newline1->x[iv] = pt4d.x;
		newline1->y[iv] = pt4d.y;
		if (newline1->has_z)
		    newline1->z[iv] = pt4d.z;
	    }
      }
    else
	goto end;

/* retrieving the second half of the split link */
    rtg = split_col->geoms[1];
    if (rtg->type == RTLINETYPE)
      {
	  rtl = (RTLINE *) rtg;
	  pa = rtl->points;
	  newline2->points = pa->npoints;
	  newline2->x = malloc (sizeof (double) * newline2->points);
	  newline2->y = malloc (sizeof (double) * newline2->points);
	  if (newline2->has_z)
	      newline2->z = malloc (sizeof (double) * newline2->points);
	  for (iv = 0; iv < newline2->points; iv++)
	    {
		/* copying LINESTRING vertices */
		rt_getPoint4d_p (ctx, pa, iv, &pt4d);
		newline2->x[iv] = pt4d.x;
		newline2->y[iv] = pt4d.y;
		if (newline2->has_z)
		    newline2->z[iv] = pt4d.z;
	    }
      }
    else
	goto end;

    ret = 1;

  end:
    if (split != NULL)
	rtgeom_free (ctx, split);
    return ret;
}

static void
cleanup_line (LWN_LINE * line)
{
    if (line->x != NULL)
	free (line->x);
    if (line->y != NULL)
	free (line->y);
    if (line->z != NULL)
	free (line->z);
}

LWN_INT64
lwn_NewGeoLinkSplit (LWN_NETWORK * net, LWN_ELEMID link, const LWN_POINT * pt)
{
    int i;
    LWN_LINK *oldlink;
    LWN_LINK newlink[2];
    LWN_ELEMID startNode;
    LWN_ELEMID endNode;
    LWN_NET_NODE newnode;
    LWN_LINE newline1;
    LWN_LINE newline2;

    i = 1;
    oldlink = lwn_be_getLinkById (net, &link, &i, LWN_COL_LINK_ALL);
    if (!oldlink)
      {
	  if (i == -1)
	      return -1;
	  else if (i == 0)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-existent link.");
		return -1;
	    }
      }
    startNode = oldlink->start_node;
    endNode = oldlink->end_node;

    newline1.srid = oldlink->geom->srid;
    newline1.has_z = oldlink->geom->has_z;
    newline1.points = 0;
    newline1.x = NULL;
    newline1.y = NULL;
    newline1.z = NULL;
    newline2.srid = oldlink->geom->srid;
    newline2.has_z = oldlink->geom->has_z;
    newline2.points = 0;
    newline2.x = NULL;
    newline2.y = NULL;
    newline2.z = NULL;
    if (!geo_link_split (net, oldlink->geom, pt, &newline1, &newline2))
      {
	  _lwn_release_links (oldlink, 1);
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }
    _lwn_release_links (oldlink, 1);

    if (net->spatial && net->allowCoincident == 0)
      {
	  /* check if a coincident node already exists */
	  if (lwn_be_existsCoincidentNode (net, pt))
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - coincident node");
		cleanup_line (&newline1);
		cleanup_line (&newline2);
		return -1;
	    }
      }

/* inserting a new NetNode */
    newnode.node_id = -1;
    newnode.geom = (LWN_POINT *) pt;
    if (!lwn_be_insertNetNodes (net, &newnode, 1))
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

/* deleting the original Link */
    i = lwn_be_deleteLinksById (net, &link, 1);
    if (i != 1)
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

/* inserting two new Links */
    newlink[0].link_id = lwn_be_getNextLinkId (net);
    if (newlink[0].link_id == -1)
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }
    newlink[1].link_id = lwn_be_getNextLinkId (net);
    if (newlink[1].link_id == -1)
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

    newlink[0].start_node = startNode;
    newlink[0].end_node = newnode.node_id;
    newlink[0].geom = &newline1;
    newlink[1].start_node = newnode.node_id;
    newlink[1].end_node = endNode;
    newlink[1].geom = &newline2;

    if (!lwn_be_insertLinks (net, newlink, 2))
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

    cleanup_line (&newline1);
    cleanup_line (&newline2);
    return newnode.node_id;
}

LWN_INT64
lwn_ModGeoLinkSplit (LWN_NETWORK * net, LWN_ELEMID link, const LWN_POINT * pt)
{
    int i;
    LWN_LINK *oldlink;
    LWN_LINK newlink;
    LWN_ELEMID startNode;
    LWN_ELEMID endNode;
    LWN_NET_NODE newnode;
    LWN_LINE newline1;
    LWN_LINE newline2;

    i = 1;
    oldlink = lwn_be_getLinkById (net, &link, &i, LWN_COL_LINK_ALL);
    if (!oldlink)
      {
	  if (i == -1)
	      return -1;
	  else if (i == 0)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-existent link.");
		return -1;
	    }
      }
    startNode = oldlink->start_node;
    endNode = oldlink->end_node;

    newline1.srid = oldlink->geom->srid;
    newline1.has_z = oldlink->geom->has_z;
    newline1.points = 0;
    newline1.x = NULL;
    newline1.y = NULL;
    newline1.z = NULL;
    newline2.srid = oldlink->geom->srid;
    newline2.has_z = oldlink->geom->has_z;
    newline2.points = 0;
    newline2.x = NULL;
    newline2.y = NULL;
    newline2.z = NULL;
    if (!geo_link_split (net, oldlink->geom, pt, &newline1, &newline2))
      {
	  _lwn_release_links (oldlink, 1);
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }
    _lwn_release_links (oldlink, 1);

    if (net->spatial && net->allowCoincident == 0)
      {
	  /* check if a coincident node already exists */
	  if (lwn_be_existsCoincidentNode (net, pt))
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - coincident node");
		cleanup_line (&newline1);
		cleanup_line (&newline2);
		return -1;
	    }
      }

/* inserting a new NetNode */
    newnode.node_id = -1;
    newnode.geom = (LWN_POINT *) pt;
    if (!lwn_be_insertNetNodes (net, &newnode, 1))
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

/* update the original Link */
    newlink.link_id = link;
    newlink.start_node = startNode;
    newlink.end_node = newnode.node_id;
    newlink.geom = &newline1;
    if (!lwn_be_updateLinksById
	(net, &newlink, 1, LWN_COL_LINK_END_NODE | LWN_COL_LINK_GEOM))
	return -1;

/* inserting a new Link */
    newlink.link_id = lwn_be_getNextLinkId (net);
    if (newlink.link_id == -1)
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

    newlink.start_node = newnode.node_id;
    newlink.end_node = endNode;
    newlink.geom = &newline2;

    if (!lwn_be_insertLinks (net, &newlink, 1))
      {
	  cleanup_line (&newline1);
	  cleanup_line (&newline2);
	  return -1;
      }

    cleanup_line (&newline1);
    cleanup_line (&newline2);
    return newnode.node_id;
}

static int
_lwn_LinkHeal (LWN_NETWORK * net, LWN_ELEMID eid1, LWN_ELEMID eid2,
	       LWN_ELEMID * node_id, LWN_ELEMID * start_node,
	       LWN_ELEMID * end_node, LWN_LINE * newline)
{
    int caseno = 0;
    LWN_ELEMID ids[2];
    LWN_ELEMID commonnode = -1;
    LWN_LINK *node_links;
    int num_node_links;
    LWN_LINK *links;
    LWN_LINK *e1 = NULL;
    LWN_LINK *e2 = NULL;
    int nlinks, i;
    int otherlinks = 0;

    /* NOT IN THE SPECS: see if the same link is given twice.. */
    if (eid1 == eid2)
      {
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - Cannot heal link with itself.");
	  return 0;
      }
    ids[0] = eid1;
    ids[1] = eid2;
    nlinks = 2;
    links = lwn_be_getLinkById (net, ids, &nlinks, LWN_COL_LINK_ALL);
    if (nlinks == -1)
	return -0;
    for (i = 0; i < nlinks; ++i)
      {
	  if (links[i].link_id == eid1)
	      e1 = &(links[i]);
	  else if (links[i].link_id == eid2)
	      e2 = &(links[i]);
      }
    if (!e1)
      {
	  _lwn_release_links (links, nlinks);
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent first link.");
	  return 0;
      }
    if (!e2)
      {
	  _lwn_release_links (links, nlinks);
	  lwn_SetErrorMsg (net->be_iface,
			   "SQL/MM Spatial exception - non-existent second link.");
	  return 0;
      }

    /* Find common node */

    if (e1->end_node == e2->start_node)
      {
	  commonnode = e1->end_node;
	  *start_node = e1->start_node;
	  *end_node = e2->end_node;
	  caseno = 1;
      }
    else if (e1->end_node == e2->end_node)
      {
	  commonnode = e1->end_node;
	  *start_node = e1->start_node;
	  *end_node = e2->start_node;
	  caseno = 2;
      }
    else if (e1->start_node == e2->start_node)
      {
	  commonnode = e1->start_node;
	  *start_node = e2->end_node;
	  *end_node = e1->end_node;
	  caseno = 3;
      }
    else if (e1->start_node == e2->end_node)
      {
	  commonnode = e1->start_node;
	  *start_node = e2->start_node;
	  *end_node = e1->start_node;
	  caseno = 4;
      }

    if (e1->geom != NULL && e2->geom != NULL)
      {
	  /* reassembling the healed Link geometry */
	  int iv;
	  int iv2 = 0;
	  newline->srid = e1->geom->srid;
	  newline->has_z = e1->geom->has_z;
	  newline->points = e1->geom->points + e2->geom->points - 1;
	  newline->x = malloc (sizeof (double) * newline->points);
	  newline->y = malloc (sizeof (double) * newline->points);
	  if (newline->has_z)
	      newline->z = malloc (sizeof (double) * newline->points);
	  switch (caseno)
	    {
	    case 1:
		for (iv = 0; iv < e1->geom->points; iv++)
		  {
		      newline->x[iv2] = e1->geom->x[iv];
		      newline->y[iv2] = e1->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e1->geom->z[iv];
		      iv2++;
		  }
		for (iv = 1; iv < e2->geom->points; iv++)
		  {
		      newline->x[iv2] = e2->geom->x[iv];
		      newline->y[iv2] = e2->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e2->geom->z[iv];
		      iv2++;
		  }
		break;
	    case 2:
		for (iv = 0; iv < e1->geom->points - 1; iv++)
		  {
		      newline->x[iv2] = e1->geom->x[iv];
		      newline->y[iv2] = e1->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e1->geom->z[iv];
		      iv2++;
		  }
		for (iv = e2->geom->points - 1; iv >= 0; iv--)
		  {
		      newline->x[iv2] = e2->geom->x[iv];
		      newline->y[iv2] = e2->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e2->geom->z[iv];
		      iv2++;
		  }
		break;
	    case 3:
		for (iv = e2->geom->points - 1; iv >= 0; iv--)
		  {
		      newline->x[iv2] = e2->geom->x[iv];
		      newline->y[iv2] = e2->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e2->geom->z[iv];
		      iv2++;
		  }
		for (iv = 1; iv < e1->geom->points; iv++)
		  {
		      newline->x[iv2] = e1->geom->x[iv];
		      newline->y[iv2] = e1->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e1->geom->z[iv];
		      iv2++;
		  }
		break;
	    case 4:
		for (iv = 0; iv < e2->geom->points; iv++)
		  {
		      newline->x[iv2] = e2->geom->x[iv];
		      newline->y[iv2] = e2->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e2->geom->z[iv];
		      iv2++;
		  }
		for (iv = 1; iv < e1->geom->points; iv++)
		  {
		      newline->x[iv2] = e1->geom->x[iv];
		      newline->y[iv2] = e1->geom->y[iv];
		      if (newline->has_z)
			  newline->z[iv2] = e1->geom->z[iv];
		      iv2++;
		  }
		break;
	    };
      }


    /* Check if any other link is connected to the common node, if found */
    if (commonnode != -1)
      {
	  num_node_links = 1;
	  node_links = lwn_be_getLinkByNetNode (net, &commonnode,
						&num_node_links,
						LWN_COL_LINK_LINK_ID);
	  if (num_node_links == -1)
	    {
		_lwn_release_links (links, nlinks);
		return 0;
	    }
	  for (i = 0; i < num_node_links; ++i)
	    {
		if (node_links[i].link_id == eid1)
		    continue;
		if (node_links[i].link_id == eid2)
		    continue;
		commonnode = -1;
		otherlinks++;
	    }
	  free (node_links);
      }

    if (commonnode == -1)
      {
	  _lwn_release_links (links, nlinks);
	  if (otherlinks)
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - other links connected.");
	    }
	  else
	    {
		lwn_SetErrorMsg (net->be_iface,
				 "SQL/MM Spatial exception - non-connected links.");
	    }
	  return 0;
      }
    _lwn_release_links (links, nlinks);

    *node_id = commonnode;
    return 1;
}

LWN_INT64
lwn_NewLinkHeal (LWN_NETWORK * net, LWN_ELEMID link, LWN_ELEMID anotherlink)
{
    LWN_ELEMID node_id;
    LWN_ELEMID start_node;
    LWN_ELEMID end_node;
    LWN_ELEMID linkids[2];
    LWN_LINK newlink;
    LWN_LINE newline;
    int n;

    newline.points = 0;
    newline.x = NULL;
    newline.y = NULL;
    newline.z = NULL;
    if (!_lwn_LinkHeal
	(net, link, anotherlink, &node_id, &start_node, &end_node, &newline))
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* removing both Links */
    linkids[0] = link;
    linkids[1] = anotherlink;
    n = lwn_be_deleteLinksById (net, linkids, 2);
    if (n != 2)
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* removing the common NetNode */
    n = lwn_be_deleteNetNodesById (net, &node_id, 1);
    if (n == -1)
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* inserting a new healed Link */
    newlink.link_id = -1;
    newlink.start_node = start_node;
    newlink.end_node = end_node;
    if (newline.points == 0)
	newlink.geom = NULL;
    else
	newlink.geom = &newline;

    if (!lwn_be_insertLinks (net, &newlink, 1))
      {
	  cleanup_line (&newline);
	  return -1;
      }

    cleanup_line (&newline);
    return node_id;
}

LWN_INT64
lwn_ModLinkHeal (LWN_NETWORK * net, LWN_ELEMID link, LWN_ELEMID anotherlink)
{
    LWN_ELEMID node_id;
    LWN_ELEMID start_node;
    LWN_ELEMID end_node;
    LWN_LINK newlink;
    LWN_LINE newline;
    int n;

    newline.points = 0;
    newline.x = NULL;
    newline.y = NULL;
    newline.z = NULL;
    if (!_lwn_LinkHeal
	(net, link, anotherlink, &node_id, &start_node, &end_node, &newline))
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* removing the second Link */
    n = lwn_be_deleteLinksById (net, &anotherlink, 1);
    if (n != 1)
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* updating the healed link */
    newlink.link_id = link;
    newlink.start_node = start_node;
    newlink.end_node = end_node;
    if (newline.points == 0)
	newlink.geom = NULL;
    else
	newlink.geom = &newline;
    if (!lwn_be_updateLinksById
	(net, &newlink, 1,
	 LWN_COL_LINK_START_NODE | LWN_COL_LINK_END_NODE | LWN_COL_LINK_GEOM))
      {
	  cleanup_line (&newline);
	  return -1;
      }

/* removing the common NetNode */
    n = lwn_be_deleteNetNodesById (net, &node_id, 1);
    if (n == -1)
      {
	  cleanup_line (&newline);
	  return -1;
      }

    cleanup_line (&newline);
    return node_id;
}

LWN_ELEMID
lwn_GetNetNodeByPoint (LWN_NETWORK * net, const LWN_POINT * pt, double tol)
{
    LWN_NET_NODE *elem;
    int num;
    int flds = LWN_COL_NODE_NODE_ID;
    LWN_ELEMID id = 0;

    elem = lwn_be_getNetNodeWithinDistance2D (net, pt, tol, &num, flds, 0);
    if (num <= 0)
	return -1;
    else if (num > 1)
      {
	  _lwn_release_nodes (elem, num);
	  lwn_SetErrorMsg (net->be_iface, "Two or more net-nodes found");
	  return -1;
      }
    id = elem[0].node_id;
    _lwn_release_nodes (elem, num);

    return id;
}

LWN_ELEMID
lwn_GetLinkByPoint (LWN_NETWORK * net, const LWN_POINT * pt, double tol)
{
    LWN_LINK *elem;
    int num, i;
    int flds = LWN_COL_LINK_LINK_ID;
    LWN_ELEMID id = 0;

    elem = lwn_be_getLinkWithinDistance2D (net, pt, tol, &num, flds, 0);
    if (num <= 0)
	return -1;
    for (i = 0; i < num; ++i)
      {
	  LWN_LINK *e = &(elem[i]);

	  if (id)
	    {
		_lwn_release_links (elem, num);
		lwn_SetErrorMsg (net->be_iface, "Two or more links found");
		return -1;
	    }
	  else
	      id = e->link_id;
      }
    _lwn_release_links (elem, num);

    return id;
}

/* wrappers of backend wrappers... */

int
lwn_be_existsCoincidentNode (const LWN_NETWORK * net, const LWN_POINT * pt)
{
    int exists = 0;
    lwn_be_getNetNodeWithinDistance2D (net, pt, 0, &exists, 0, -1);
    if (exists == -1)
	return 0;
    return exists;
}

int
lwn_be_existsLinkIntersectingPoint (const LWN_NETWORK * net,
				    const LWN_POINT * pt)
{
    int exists = 0;
    lwn_be_getLinkWithinDistance2D (net, pt, 0, &exists, 0, -1);
    if (exists == -1)
	return 0;
    return exists;
}

#endif /* end RTTOPO conditionals */
