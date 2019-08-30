/*
 lwn_network.h -- Topology-Network abstract multi-backend interface
  
 version 4.3, 2015 August 12

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

#ifndef LWN_NETWORK_H
#define LWN_NETWORK_H 1

#include <stdint.h>

/* INT64 */
typedef int64_t LWN_INT64;

/** Identifier of network element */
typedef LWN_INT64 LWN_ELEMID;

/*
 * Geometry elements
 */

typedef struct
{
    int srid;
    double x;
    double y;
    double z;
    int has_z;
}
LWN_POINT;

typedef struct
{
    int srid;
    int points;
    double *x;
    double *y;
    double *z;
    int has_z;
}
LWN_LINE;

typedef struct
{
    double min_x;
    double min_y;
    double max_x;
    double max_y;
}
LWN_BBOX;

/*
 * ISO primitive elements
 */

/** NODE */
typedef struct
{
    LWN_ELEMID node_id;
    LWN_POINT *geom;
}
LWN_NET_NODE;

/** Node fields */
#define LWN_COL_NODE_NODE_ID         1<<0
#define LWN_COL_NODE_GEOM            1<<1
#define LWN_COL_NODE_ALL            (1<<2)-1

/** LINK */
typedef struct
{
    LWN_ELEMID link_id;
    LWN_ELEMID start_node;
    LWN_ELEMID end_node;
    LWN_LINE *geom;
}
LWN_LINK;

/** Link fields */
#define LWN_COL_LINK_LINK_ID         1<<0
#define LWN_COL_LINK_START_NODE      1<<1
#define LWN_COL_LINK_END_NODE        1<<2
#define LWN_COL_LINK_GEOM            1<<3
#define LWN_COL_LINK_ALL            (1<<4)-1

/*
 * Backend handling functions
 */

/* opaque pointers referencing native backend objects */

/**
 * Backend private data pointer
 *
 * Only the backend handler needs to know what it really is.
 * It will be passed to all registered callback functions.
 */
typedef struct LWN_BE_DATA_T LWN_BE_DATA;

/**
 * Backend interface handler
 *
 * Embeds all registered backend callbacks and private data pointer.
 * Will need to be passed (directly or indirectly) to al public facing
 * APIs of this library.
 */
typedef struct LWN_BE_IFACE_T LWN_BE_IFACE;

/**
 * Network handler.
 *
 * Embeds backend interface handler.
 * Will need to be passed to all network manipulation APIs
 * of this library.
 */
typedef struct LWN_BE_NETWORK_T LWN_BE_NETWORK;

/**
 * Structure containing base backend callbacks
 *
 * Used for registering into the backend iface
 */
typedef struct LWN_BE_CALLBACKS_T
{

  /**
   * Create a new network in the backend
   *
   * @param name the network name
   * @param srid the network SRID
   * @param hasZ non-zero if network primitives should have a Z ordinate
   * @param spatial determines if this is a Logical or SpatialNetwork
   * @param coincident_nodes determines if they should de tolerated or not.
   * @param geos_handle handle to GEOS context.
   * 
   * @return a network handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
    LWN_BE_NETWORK *(*createNetwork) (const LWN_BE_DATA * be,
				      const char *name, int srid, int hasZ,
				      int spatial, int coincident_nodes,
				      const void *geos_handle);

  /**
   * Load a network from the backend
   *
   * @param name the network name
   * @return a network handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
    LWN_BE_NETWORK *(*loadNetworkByName) (const LWN_BE_DATA * be,
					  const char *name);

  /**
   * Release memory associated to a backend network
   *
   * @param net the backend network handler
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
    int (*freeNetwork) (LWN_BE_NETWORK * net);

  /**
   * Get nodes within distance by point
   *
   * @param net the network to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
    LWN_NET_NODE *(*getNetNodeWithinDistance2D) (const LWN_BE_NETWORK * net,
						 const LWN_POINT * pt,
						 double dist, int *numelems,
						 int fields, int limit);

  /**
   * Get links that start or end on any of the given node identifiers
   *
   * @param net the network to act upon
   * @param ids an array of node identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_LINK_* macros
   *
   * @return an array of links that are incident to a node
   *         or NULL in the following cases:
   *         - no link found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   */
    LWN_LINK *(*getLinkByNetNode) (const LWN_BE_NETWORK * net,
				   const LWN_ELEMID * ids, int *numelems,
				   int fields);

  /**
   * Get links within distance by point
   *
   * @param net the network to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_LINK_* macros
   * @param limit max number of links to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of links or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
    LWN_LINK *(*getLinkWithinDistance2D) (const LWN_BE_NETWORK * net,
					  const LWN_POINT * pt, double dist,
					  int *numelems, int fields, int limit);

  /**
   * Insert nodes
   *
   * Insert node primitives in the network
   *
   * @param net the network to act upon
   * @param nodes the nodes to insert. Those with a node_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the nodes array
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
    int (*insertNetNodes) (const LWN_BE_NETWORK * net,
			   LWN_NET_NODE * nodes, int numelems);

  /**
   * Get nodes by id
   *
   * @param net the network to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of node in output array.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_NODE_* macros
   *
   * @return an array of nodes
   *         or NULL in the following cases:
   *         - no node found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   *
   */
    LWN_NET_NODE *(*getNetNodeById) (const LWN_BE_NETWORK * topo,
				     const LWN_ELEMID * ids, int *numelems,
				     int fields);

  /**
   * Update nodes by id
   *
   * @param net the network to act upon
   * @param nodes an array of LWN_NET_NODE objects with selecting id
   *              and updating fields.
   * @param numnodes number of nodes in the "nodes" array
   * @param upd_fields fields to be updated for the selected edges,
   *                   see LWN_COL_NODE_* macros
   *
   * @return number of nodes being updated or -1 on error
   *         (@see lastErroMessage)
   */
    int (*updateNetNodesById) (const LWN_BE_NETWORK * net,
			       const LWN_NET_NODE * nodes, int numnodes,
			       int upd_fields);

  /**
   * Delete nodes by id
   *
   * @param net the network to act upon
   * @param ids an array of node identifiers
   * @param numelems number of node identifiers in the ids array
   *
   * @return number of nodes being deleted or -1 on error
   *         (@see lastErroMessage)
   */
    int (*deleteNetNodesById) (const LWN_BE_NETWORK * net,
			       const LWN_ELEMID * ids, int numelems);

  /**
   * Get next available link identifier
   *
   * Identifiers returned by this function should not be considered
   * available anymore.
   *
   * @param net the network to act upon
   *
   * @return next available link identifier or -1 on error
   */
      LWN_ELEMID (*getNextLinkId) (const LWN_BE_NETWORK * net);

  /**
   * Get nodes within a 2D bounding box
   *
   * @param net the network to act upon
   * @param box the query box
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
    LWN_NET_NODE *(*getNetNodeWithinBox2D) (const LWN_BE_NETWORK * net,
					    const LWN_BBOX * box,
					    int *numelems, int fields,
					    int limit);

  /**
   * Get next available link identifier
   *
   * Identifiers returned by this function should not be considered
   * available anymore.
   *
   * @param net the network to act upon
   *
   * @return next available link identifier or -1 on error
   */
      LWN_ELEMID (*getNexLinkId) (const LWN_BE_NETWORK * net);

  /**
   * Insert links
   *
   * Insert link primitives in the network
   *
   * @param net the network to act upon
   * @param links the links to insert. Those with a link_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the links array
   *
   * @return number of inserted links, or -1 (@see lastErrorMessage)
   */
    int (*insertLinks) (const LWN_BE_NETWORK * net,
			LWN_LINK * links, int numelems);

  /**
   * Update links by id
   *
   * @param net the network to act upon
   * @param links an array of LWN_LINK object with selecting id
   *              and updating fields.
   * @param numedges number of links in the "links" array
   * @param upd_fields fields to be updated for the selected links,
   *                   see LWN_COL_LINK_* macros
   *
   * @return number of links being updated or -1 on error
   *         (@see lastErroMessage)
   */
    int (*updateLinksById) (const LWN_BE_NETWORK * net,
			    const LWN_LINK * links, int numlinks,
			    int upd_fields);

  /**
   * Get link by id
   *
   * @param net the network to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of link identifiers
   *                 in the input array, gets number of links in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               LWN_COL_LINK_* macros
   *
   * @return an array of links or NULL in the following cases:
   *         - none found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
    LWN_LINK *(*getLinkById) (const LWN_BE_NETWORK * net,
			      const LWN_ELEMID * ids, int *numelems,
			      int fields);

  /**
   * Delete links by id
   *
   * @param net the network to act upon
   * @param ids an array of link identifiers
   * @param numelems number of link identifiers in the ids array
   *
   * @return number of links being deleted or -1 on error
   *         (@see lastErroMessage)
   */
    int (*deleteLinksById) (const LWN_BE_NETWORK * net,
			    const LWN_ELEMID * ids, int numelems);


  /**
   * Get network SRID
   * @return 0 for unknown
   */
    int (*netGetSRID) (const LWN_BE_NETWORK * net);

  /**
   * Get network Z flag
   * @return 1 if network elements do have Z value, 0 otherwise
   */
    int (*netHasZ) (const LWN_BE_NETWORK * net);

  /**
   * Get network type flag
   * @return 0 if network is of the Logical Type, 1 otherwise (Spatial)
   */
    int (*netIsSpatial) (const LWN_BE_NETWORK * net);

  /**
   * Get CoincidentNodes flag
   * @return 1 if newtwork tolerates Coincident Nodes, 0 otherwise (Spatial)
   */
    int (*netAllowCoincident) (const LWN_BE_NETWORK * net);

  /**
   * Get GEOS Handle
   * @return current GEOS Handle, NULL otherwise
   */
    const void *(*netGetGEOS) (const LWN_BE_NETWORK * net);


} LWN_BE_CALLBACKS;


/**
 * Create a new backend interface
 *
 * Ownership to caller delete with lwn_FreeBackendIface
 *
 * @param ctx librtgeom context, create with rtgeom_init
 * @param data Backend data, passed as first parameter to all callback functions
 */
LWN_BE_IFACE *lwn_CreateBackendIface (const RTCTX * ctx,
				      const LWN_BE_DATA * data);

/**
 * Register backend callbacks into the opaque iface handler
 *
 * @param iface the backend interface handler (see lwn_CreateBackendIface)
 * @param cb a pointer to the callbacks structure; ownership left to caller.
 */
void lwn_BackendIfaceRegisterCallbacks (LWN_BE_IFACE * iface,
					const LWN_BE_CALLBACKS * cb);

/** Release memory associated with an LWB_BE_IFACE */
void lwn_FreeBackendIface (LWN_BE_IFACE * iface);

/********************************************************************
 *
 * End of BE interface
 *
 *******************************************************************/


/*
 * Network functions
 */

/** Opaque network structure
 *
 * Embeds backend interface and network
 */
typedef struct LWN_NETWORK_T LWN_NETWORK;


/********************************************************************
 *
 * Error handling functions
 *
 *******************************************************************/

/**
 * Reset the last backend error message
 *
 * @oaram iface the backend interface handler (see lwn_CreateBackendIface)
 */
void lwn_ResetErrorMsg (LWN_BE_IFACE * iface);

/**
 * Set the last backend error message
 *
 * @oaram iface the backend interface handler (see lwn_CreateBackendIface)
 * 
 * @return NULL-terminated error string
 */
void lwn_SetErrorMsg (LWN_BE_IFACE * iface, const char *message);

/**
 * Read last error message from backend
 *
 * @oaram iface the backend interface handler (see lwn_CreateBackendIface)
 * 
 * @return NULL-terminated error string
 */
const char *lwn_GetErrorMsg (LWN_BE_IFACE * iface);


/*******************************************************************
 *
 * Backend independent Linestring auxiliary functions
 *
 *******************************************************************/

/**
 * Creates a backend independent 2D Point
 * 
 * @param srid the network SRID
 * @param x the X coordinate
 * @param y the Y coordinate
 * 
 * @return pointer to a new LWN_POINT
 */
LWN_POINT *lwn_create_point2d (int srid, double x, double y);

/**
 * Creates a backend independent 3D Point
 * 
 * @param srid the network SRID
 * @param x the X coordinate
 * @param y the Y coordinate
 * @param z the Z coordinate
 * 
 * @return pointer to a new LWN_POINT
 */
LWN_POINT *lwn_create_point3d (int srid, double x, double y, double z);

/**
 * Free a backend independent Point
 *
 * @param ptr pointer to the LWN_POINT to be freed
 */
void lwn_free_point (LWN_POINT * ptr);

/**
 * Initializes a backend independent Linestring
 *
 * @param points total number of points
 * @param srid the network SRID
 * @param hasz non-zero if network primitives should have a Z ordinate
 *
 * @return pointer to a new LWN_LINE
 */
LWN_LINE *lwn_alloc_line (int points, int srid, int hasz);

/**
 * Free a backend independent Linestring
 *
 * @param ptr pointer to the LWN_LINE to be freed
 */
void lwn_free_line (LWN_LINE * ptr);


/*******************************************************************
 *
 * Non-ISO signatures here
 *
 *******************************************************************/

/**
 * Initializes a new network
 *
 * @param iface the backend interface handler (see lwn_CreateBackendIface)
 * @param name name of the new network
 * @param srid the network SRID
 * @param hasz non-zero if network primitives should have a Z ordinate
 * @param spatial determines if this is a Logical or SpatialNetwork
 * @param coincident_nodes determines if they should de tolerated or not.
 *
 * @return the handler of the network, or NULL on error
 *         (lwn error handler will be invoked with error message)
 */
LWN_NETWORK *lwn_CreateNetwork (LWN_BE_IFACE * iface, const char *name,
				int srid, int hasz, int spatial,
				int coincident_nodes);

/**
 * Loads an existing network by name from the database
 *
 * @param iface the backend interface handler (see lwt_CreateBackendIface)
 * @param name name of the network to load
 *
 * @return the handler of the network, or NULL on error
 *         (lwn error handler will be invoked with error message)
 */
LWN_NETWORK *lwn_LoadNetwork (LWN_BE_IFACE * iface, const char *name);

/**
 * Drop a network and all its associated objects from the database
 *
 * @param net the network to drop
 */
void lwn_DropNetwork (LWN_NETWORK * net);

/** Release memory associated with an LWN_NETWORK
 *
 * @param net the network to release (it's not removed from db)
 */
void lwn_FreeNetwork (LWN_NETWORK * net);


/*******************************************************************
 *
 * ISO signatures here
 *
 *******************************************************************/

/**
 * Add an isolated node
 *
 * For ST_AddIsoNetNode
 *
 * @param net the network to operate on
 * @param pt the node position
 *
 * @return ID of the newly added node
 *
 */
LWN_ELEMID lwn_AddIsoNetNode (LWN_NETWORK * net, LWN_POINT * pt);

/**
 * Move an isolated node
 *
 * For ST_MoveIsoNetNode
 *
 * @param net the network to operate on
 * @param node the identifier of the nod to be moved
 * @param pt the new node position
 * @return 0 on success, -1 on error
 *         (lwn error handler will be invoked with error message)
 *
 */
int lwn_MoveIsoNetNode (LWN_NETWORK * net, LWN_ELEMID node,
			const LWN_POINT * pt);

/**
 * Remove an isolated node
 *
 * For ST_RemIsoNetNode
 *
 * @param net the network to operate on
 * @param node the identifier of the node to be moved
 * @return 0 on success, -1 on error
 *         (lwn error handler will be invoked with error message)
 *
 */
int lwn_RemIsoNetNode (LWN_NETWORK * net, LWN_ELEMID node);

/**
 * Add a link connecting two existing nodes
 *
 * For ST_AddLink
 *
 * @param net the network to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the link geometry
 * @return ID of the newly added link, or -1 on error
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_ELEMID lwn_AddLink (LWN_NETWORK * net,
			LWN_ELEMID startNode, LWN_ELEMID endNode,
			LWN_LINE * geom);

/**
 * Changes the shape of a link without affecting the network structure.
 *
 * For ST_ChangeLinkGeom
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be changed
 * @param curve the link geometry
 * @return 0 on success, -1 on error
 *         (lwn error handler will be invoked with error message)
 *
 */
int lwn_ChangeLinkGeom (LWN_NETWORK * net, LWN_ELEMID link,
			const LWN_LINE * curve);

/**
 * Remove a link.
 *
 * For ST_RemoveLink
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be removed
 * @return 0 on success, -1 on error
 *         (lwn error handler will be invoked with error message)
 *
 */
int lwn_RemoveLink (LWN_NETWORK * net, LWN_ELEMID link);

/**
 * Split a logical link, replacing it with two new links.
 *
 * For ST_NewLogLinkSplit
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be split.
 * @return the ID of the inserted Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_NewLogLinkSplit (LWN_NETWORK * net, LWN_ELEMID link);

/**
 * Split a logical link, modifying the original link and adding a new one.
 * 
 * For ST:ModLogLinkSplit
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be split.
 * @return the ID of the inserted Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_ModLogLinkSplit (LWN_NETWORK * net, LWN_ELEMID link);

/**
 * Split a spatial link by a node, replacing it with two new links.
 *
 * For ST_NewGeoLinkSplit
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be split.
 * @param pt the point geometry
 * @return the ID of the inserted Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_NewGeoLinkSplit (LWN_NETWORK * net, LWN_ELEMID link,
			       const LWN_POINT * pt);

/**
 * Split a spatial link by a node, modifying the original link and adding
 * a new one.
 *
 * For ST_ModGeoLinkSplit
 *
 * @param net the network to operate on
 * @param link the identifier of the link to be split.
 * @param pt the point geometry
 * @return the ID of the inserted Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_ModGeoLinkSplit (LWN_NETWORK * net, LWN_ELEMID link,
			       const LWN_POINT * pt);

/**
 * Heal two links by deleting the node connecting them, deleting both links,
 * and replacing them with a new link whose direction is the same as the
 * first link provided.
 *
 * For ST_NewLinkHeal
 *
 * @param net the network to operate on
 * @param link the identifier of a first link.
 * @param anotherlink the identifier of a second link.
 * @return the ID of the rmove Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_NewLinkHeal (LWN_NETWORK * net, LWN_ELEMID link,
			   LWN_ELEMID anotherlink);

/**
 * Heal two links by deleting the node connecting them, modfying the first
 * link provided, and deleting the second link.
 *
 * For ST_ModLinkHeal
 *
 * @param net the network to operate on
 * @param link the identifier of a first link.
 * @param anotherlink the identifier of a second link.
 * @return the ID of the rmove Node; a negative number on failure.
 *         (lwn error handler will be invoked with error message)
 *
 */
LWN_INT64 lwn_ModLinkHeal (LWN_NETWORK * net, LWN_ELEMID link,
			   LWN_ELEMID anotherlink);

/**
 * Retrieve the id of a net-node at a point location
 *
 * For GetNetNodeByNode
 *
 * @param net the network to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for a net-node
 * @return a net-node identifier if one is found, 0 if none is found, -1
 *         on error (multiple net-nodes within distance).
 *         (lwn error handler will be invoked with error message)
 */
LWN_ELEMID lwn_GetNetNodeByPoint (LWN_NETWORK * net, const LWN_POINT * pt,
				  double tol);

/**
 * Find the edge-id of a link that intersects a given point
 *
 * For GetLinkByPoint
 *
 * @param net the network to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for an
 *            intersecting link
 * @return a link identifier if one is found, 0 if none is found, -1
 *         on error (multiple links within distance).
 *         (lwn error handler will be invoked with error message)
 */
LWN_ELEMID lwn_GetLinkByPoint (LWN_NETWORK * net, const LWN_POINT * pt,
			       double tol);

#endif /* LWN_NETWORK_H */
