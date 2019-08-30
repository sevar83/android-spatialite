/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 *
 * rttopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rttopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rttopo.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright (C) 2015 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************/



#ifndef LIBRTGEOM_TOPO_H
#define LIBRTGEOM_TOPO_H 1

#include "librttopo_geom.h"

/* INT64 */
typedef int64_t RTT_INT64;

/** Identifier of topology element */
typedef RTT_INT64 RTT_ELEMID;

/*
 * ISO primitive elements
 */

/** NODE */
typedef struct
{
  RTT_ELEMID node_id;
  RTT_ELEMID containing_face; /* -1 if not isolated */
  RTPOINT *geom;
}
RTT_ISO_NODE;

void rtt_iso_node_release(RTT_ISO_NODE* node);

/** Node fields */
#define RTT_COL_NODE_NODE_ID         1<<0
#define RTT_COL_NODE_CONTAINING_FACE 1<<1
#define RTT_COL_NODE_GEOM            1<<2
#define RTT_COL_NODE_ALL            (1<<3)-1

/** EDGE */
typedef struct
{
  RTT_ELEMID edge_id;
  RTT_ELEMID start_node;
  RTT_ELEMID end_node;
  RTT_ELEMID face_left;
  RTT_ELEMID face_right;
  RTT_ELEMID next_left;
  RTT_ELEMID next_right;
  RTLINE *geom;
}
RTT_ISO_EDGE;

/** Edge fields */
#define RTT_COL_EDGE_EDGE_ID         1<<0
#define RTT_COL_EDGE_START_NODE      1<<1
#define RTT_COL_EDGE_END_NODE        1<<2
#define RTT_COL_EDGE_FACE_LEFT       1<<3
#define RTT_COL_EDGE_FACE_RIGHT      1<<4
#define RTT_COL_EDGE_NEXT_LEFT       1<<5
#define RTT_COL_EDGE_NEXT_RIGHT      1<<6
#define RTT_COL_EDGE_GEOM            1<<7
#define RTT_COL_EDGE_ALL            (1<<8)-1

/** FACE */
typedef struct
{
  RTT_ELEMID face_id;
  RTGBOX *mbr;
}
RTT_ISO_FACE;

/** Face fields */
#define RTT_COL_FACE_FACE_ID         1<<0
#define RTT_COL_FACE_MBR             1<<1
#define RTT_COL_FACE_ALL            (1<<2)-1

typedef enum RTT_SPATIALTYPE_T {
  RTT_PUNTAL = 0,
  RTT_LINEAL = 1,
  RTT_AREAL = 2,
  RTT_COLLECTION = 3
} RTT_SPATIALTYPE;

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
typedef struct RTT_BE_DATA_T RTT_BE_DATA;

/**
 * Backend interface handler
 *
 * Embeds all registered backend callbacks and private data pointer.
 * Will need to be passed (directly or indirectly) to al public facing
 * APIs of this library.
 */
typedef struct RTT_BE_IFACE_T RTT_BE_IFACE;

/**
 * Topology handler.
 *
 * Embeds backend interface handler.
 * Will need to be passed to all topology manipulation APIs
 * of this library.
 */
typedef struct RTT_BE_TOPOLOGY_T RTT_BE_TOPOLOGY;

/**
 * Structure containing base backend callbacks
 *
 * Used for registering into the backend iface
 */
typedef struct RTT_BE_CALLBACKS_T {

  /**
   * Read last error message from backend
   *
   * @return NULL-terminated error string
   */
  const char* (*lastErrorMessage) (const RTT_BE_DATA* be);

  /**
   * Create a new topology in the backend
   *
   * @param name the topology name
   * @param srid the topology SRID
   * @param precision the topology precision/tolerance
   * @param hasZ non-zero if topology primitives should have a Z ordinate
   * @return a topology handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
  RTT_BE_TOPOLOGY* (*createTopology) (
    const RTT_BE_DATA* be,
    const char* name, int srid, double precision, int hasZ
  );

  /**
   * Load a topology from the backend
   *
   * @param name the topology name
   * @return a topology handler, which embeds the backend data/params
   *         or NULL on error (@see lastErrorMessage)
   */
  RTT_BE_TOPOLOGY* (*loadTopologyByName) (
    const RTT_BE_DATA* be,
    const char* name
  );

  /**
   * Release memory associated to a backend topology
   *
   * @param topo the backend topology handler
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
  int (*freeTopology) (RTT_BE_TOPOLOGY* topo);

  /**
   * Get nodes by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of node in output array.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_NODE_* macros
   *
   * @return an array of nodes
   *         or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   *
   */
  RTT_ISO_NODE* (*getNodeById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids, int* numelems, int fields
  );

  /**
   * Get nodes within distance by point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  RTT_ISO_NODE* (*getNodeWithinDistance2D) (
      const RTT_BE_TOPOLOGY* topo,
      const RTPOINT* pt, double dist, int* numelems,
      int fields, int limit
  );

  /**
   * Insert nodes
   *
   * Insert node primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param nodes the nodes to insert. Those with a node_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the nodes array
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   */
  int (*insertNodes) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ISO_NODE* nodes,
      int numelems
  );

  /**
   * Get edge by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of edge identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_EDGE_* macros
   *
   * @return an array of edges or NULL in the following cases:
   *         - none found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  RTT_ISO_EDGE* (*getEdgeById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids, int* numelems, int fields
  );

  /**
   * Get edges within distance by point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   * @param dist the distance
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_EDGE_* macros
   * @param limit max number of edges to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of edges or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  RTT_ISO_EDGE* (*getEdgeWithinDistance2D) (
      const RTT_BE_TOPOLOGY* topo,
      const RTPOINT* pt, double dist, int* numelems,
      int fields, int limit
  );

  /**
   * Get next available edge identifier
   *
   * Identifiers returned by this function should not be considered
   * available anymore.
   *
   * @param topo the topology to act upon
   *
   * @return next available edge identifier or -1 on error
   */
  RTT_ELEMID (*getNextEdgeId) (
      const RTT_BE_TOPOLOGY* topo
  );

  /**
   * Insert edges
   *
   * Insert edge primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param edges the edges to insert. Those with a edge_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the edges array
   *
   * @return number of inserted edges, or -1 (@see lastErrorMessage)
   */
  int (*insertEdges) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ISO_EDGE* edges,
      int numelems
  );

  /**
   * Update edges selected by fields match/mismatch
   *
   * @param topo the topology to act upon
   * @param sel_edge an RTT_ISO_EDGE object with selecting fields set.
   * @param sel_fields fields used to select edges to be updated,
   *                   see RTT_COL_EDGE_* macros
   * @param upd_edge an RTT_ISO_EDGE object with updated fields set.
   * @param upd_fields fields to be updated for the selected edges,
   *                   see RTT_COL_EDGE_* macros
   * @param exc_edge an RTT_ISO_EDGE object with exclusion fields set,
   *                 can be NULL if no exlusion condition exists.
   * @param exc_fields fields used for excluding edges from the update,
   *                   see RTT_COL_EDGE_* macros
   *
   * @return number of edges being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateEdges) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_EDGE* sel_edge, int sel_fields,
      const RTT_ISO_EDGE* upd_edge, int upd_fields,
      const RTT_ISO_EDGE* exc_edge, int exc_fields
  );

  /**
   * Get faces by id
   *
   * @param topo the topology to act upon
   * @param ids an array of element identifiers
   * @param numelems input/output parameter, pass number of edge identifiers
   *                 in the input array, gets number of node in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_FACE_* macros
   *
   * @return an array of faces or NULL in the following cases:
   *         - none found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  RTT_ISO_FACE* (*getFaceById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids, int* numelems, int fields
  );

  /**
   * Get face containing point
   *
   * @param topo the topology to act upon
   * @param pt the query point
   *
   * @return a face identifier, -1 if no face contains the point
   *         (could be in universe face or on an edge)
   *         or -2 on error (@see lastErrorMessage)
   */
  RTT_ELEMID (*getFaceContainingPoint) (
      const RTT_BE_TOPOLOGY* topo,
      const RTPOINT* pt
  );

  /**
   * Update TopoGeometry objects after an edge split event
   *
   * @param topo the topology to act upon
   * @param split_edge identifier of the edge that was split.
   * @param new_edge1 identifier of the first new edge that was created
   *        as a result of edge splitting.
   * @param new_edge2 identifier of the second new edge that was created
   *        as a result of edge splitting, or -1 if the old edge was
   *        modified rather than replaced.
   *
   * @return 1 on success, 0 on error
   *
   * @note on splitting an edge, the new edges both have the
   *       same direction as the original one. If a second new edge was
   *       created, its start node will be equal to the first new edge
   *       end node.
   */
  int (*updateTopoGeomEdgeSplit) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID split_edge, RTT_ELEMID new_edge1, RTT_ELEMID new_edge2
  );

  /**
   * Delete edges
   *
   * @param topo the topology to act upon
   * @param sel_edge an RTT_ISO_EDGE object with selecting fields set.
   * @param sel_fields fields used to select edges to be deleted,
   *                   see RTT_COL_EDGE_* macros
   *
   * @return number of edges being deleted or -1 on error
   *         (@see lastErroMessage)
   */
  int (*deleteEdges) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_EDGE* sel_edge, int sel_fields
  );

  /**
   * Get edges whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_NODE_* macros
   * @param limit max number of nodes to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of nodes or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  RTT_ISO_NODE* (*getNodeWithinBox2D) (
      const RTT_BE_TOPOLOGY* topo,
      const RTGBOX* box,
      int* numelems, int fields, int limit
  );

  /**
   * Get edges whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box, to be considered infinite if NULL
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_EDGE_* macros
   * @param limit max number of edges to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of edges or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  RTT_ISO_EDGE* (*getEdgeWithinBox2D) (
      const RTT_BE_TOPOLOGY* topo,
      const RTGBOX* box,
      int* numelems, int fields, int limit
  );

  /**
   * Get edges that start or end on any of the given node identifiers
   *
   * @param topo the topology to act upon
   * @param ids an array of node identifiers
   * @param numelems input/output parameter, pass number of node identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_EDGE_* macros
   *
   * @return an array of edges that are incident to a node
   *         or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *           (@see lastErrorMessage)
   */
  RTT_ISO_EDGE* (*getEdgeByNode) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids, int* numelems, int fields
  );

  /**
   * Update nodes selected by fields match/mismatch
   *
   * @param topo the topology to act upon
   * @param sel_node an RTT_ISO_NODE object with selecting fields set.
   * @param sel_fields fields used to select nodes to be updated,
   *                   see RTT_COL_NODE_* macros
   * @param upd_node an RTT_ISO_NODE object with updated fields set.
   * @param upd_fields fields to be updated for the selected nodes,
   *                   see RTT_COL_NODE_* macros
   * @param exc_node an RTT_ISO_NODE object with exclusion fields set,
   *                 can be NULL if no exlusion condition exists.
   * @param exc_fields fields used for excluding nodes from the update,
   *                   see RTT_COL_NODE_* macros
   *
   * @return number of nodes being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateNodes) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_NODE* sel_node, int sel_fields,
      const RTT_ISO_NODE* upd_node, int upd_fields,
      const RTT_ISO_NODE* exc_node, int exc_fields
  );

  /**
   * Update TopoGeometry objects after a face split event
   *
   * @param topo the topology to act upon
   * @param split_face identifier of the face that was split.
   * @param new_face1 identifier of the first new face that was created
   *        as a result of face splitting.
   * @param new_face2 identifier of the second new face that was created
   *        as a result of face splitting, or -1 if the old face was
   *        modified rather than replaced.
   *
   * @return 1 on success, 0 on error (@see lastErroMessage)
   *
   */
  int (*updateTopoGeomFaceSplit) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID split_face, RTT_ELEMID new_face1, RTT_ELEMID new_face2
  );

  /**
   * Insert faces
   *
   * Insert face primitives in the topology, performing no
   * consistency checks.
   *
   * @param topo the topology to act upon
   * @param faces the faces to insert. Those with a node_id set to -1
   *              it will be replaced to an automatically assigned identifier
   * @param nelems number of elements in the faces array
   *
   * @return number of inserted faces, or -1 (@see lastErrorMessage)
   */
  int (*insertFaces) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ISO_FACE* faces,
      int numelems
  );

  /**
   * Update faces by id
   *
   * @param topo the topology to act upon
   * @param faces an array of RTT_ISO_FACE object with selecting id
   *              and setting mbr.
   * @param numfaces number of faces in the "faces" array
   *
   * @return number of faces being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateFacesById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_FACE* faces, int numfaces
  );

  /*
   * Get the ordered list edge visited by a side walk
   *
   * The walk starts from the side of an edge and stops when
   * either the max number of visited edges OR the starting
   * position is reached. The output list never includes a
   * duplicated signed edge identifier.
   *
   * It is expected that the walk uses the "next_left" and "next_right"
   * attributes of ISO edges to perform the walk (rather than recomputing
   * the turns at each node).
   *
   * @param topo the topology to operate on
   * @param edge walk start position and direction:
   *             abs value identifies the edge, sign expresses
   *             side (left if positive, right if negative)
   *             and direction (forward if positive, backward if negative).
   * @param numedges output parameter, gets the number of edges visited
   * @param limit max edges to return (to avoid an infinite loop in case
   *              of a corrupted topology). 0 is for unlimited.
   *              The function is expected to error out if the limit is hit.
   *
   * @return an array of signed edge identifiers (positive edges being
   *         walked in their direction, negative ones in opposite) or
   *         NULL on error (@see lastErroMessage)
   */
  RTT_ELEMID* (*getRingEdges) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID edge, int *numedges, int limit
  );

  /**
   * Update edges by id
   *
   * @param topo the topology to act upon
   * @param edges an array of RTT_ISO_EDGE object with selecting id
   *              and updating fields.
   * @param numedges number of edges in the "edges" array
   * @param upd_fields fields to be updated for the selected edges,
   *                   see RTT_COL_EDGE_* macros
   *
   * @return number of edges being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateEdgesById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_EDGE* edges, int numedges,
      int upd_fields
  );

  /**
   * \brief
   * Get edges that have any of the given faces on the left or right side
   * and optionally whose bounding box overlaps the given one.
   *
   * @param topo the topology to act upon
   * @param ids an array of face identifiers
   * @param numelems input/output parameter, pass number of face identifiers
   *                 in the input array, gets number of edges in output array
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_EDGE_* macros
   * @param box optional bounding box to further restrict matches, use
   *            NULL for no further restriction.
   *
   * @return an array of edges identifiers or NULL in the following cases:
   *         - no edge found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   */
  RTT_ISO_EDGE* (*getEdgeByFace) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids, int* numelems, int fields,
      const RTGBOX *box
  );

  /**
   * Get isolated nodes contained in any of the given faces
   *
   * @param topo the topology to act upon
   * @param faces an array of face identifiers
   * @param numelems input/output parameter, pass number of face
   *                 identifiers in the input array, gets number of
   *                 nodes in output array if the return is not null,
   *                 otherwise see @return section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_NODE_* macros
   * @param box optional bounding box to further restrict matches, use
   *            NULL for no further restriction.
   *
   * @return an array of nodes or NULL in the following cases:
   *         - no nod found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1, @see lastErrorMessage)
   */
  RTT_ISO_NODE* (*getNodeByFace) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* faces, int* numelems, int fields,
      const RTGBOX *box
  );

  /**
   * Update nodes by id
   *
   * @param topo the topology to act upon
   * @param nodes an array of RTT_ISO_EDGE objects with selecting id
   *              and updating fields.
   * @param numnodes number of nodes in the "nodes" array
   * @param upd_fields fields to be updated for the selected edges,
   *                   see RTT_COL_NODE_* macros
   *
   * @return number of nodes being updated or -1 on error
   *         (@see lastErroMessage)
   */
  int (*updateNodesById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ISO_NODE* nodes, int numnodes,
      int upd_fields
  );

  /**
   * Delete faces by id
   *
   * @param topo the topology to act upon
   * @param ids an array of face identifiers
   * @param numelems number of face identifiers in the ids array
   *
   * @return number of faces being deleted or -1 on error
   *         (@see lastErroMessage)
   */
  int (*deleteFacesById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids,
      int numelems
  );

  /**
   * Get topology SRID
   * @return 0 for unknown
   */
  int (*topoGetSRID) (
      const RTT_BE_TOPOLOGY* topo
  );

  /**
   * Get topology precision
   */
  double (*topoGetPrecision) (
      const RTT_BE_TOPOLOGY* topo
  );

  /**
   * Get topology Z flag
   * @return 1 if topology elements do have Z value, 0 otherwise
   */
  int (*topoHasZ) (
      const RTT_BE_TOPOLOGY* topo
  );

  /**
   * Delete nodes by id
   *
   * @param topo the topology to act upon
   * @param ids an array of node identifiers
   * @param numelems number of node identifiers in the ids array
   *
   * @return number of nodes being deleted or -1 on error
   *         (@see lastErroMessage)
   */
  int (*deleteNodesById) (
      const RTT_BE_TOPOLOGY* topo,
      const RTT_ELEMID* ids,
      int numelems
  );

  /**
   * Check TopoGeometry objects before an edge removal event
   *
   * @param topo the topology to act upon
   * @param rem_edge identifier of the edge that's been removed
   * @param face_left identifier of the face on the edge's left side
   * @param face_right identifier of the face on the edge's right side
   *
   * @return 1 to allow, 0 to forbid the operation
   *         (reporting reason via lastErrorMessage)
   *
   */
  int (*checkTopoGeomRemEdge) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID rem_edge,
      RTT_ELEMID face_left,
      RTT_ELEMID face_right
  );

  /**
   * Update TopoGeometry objects after healing two faces
   *
   * @param topo the topology to act upon
   * @param face1 identifier of the first face
   * @param face2 identifier of the second face
   * @param newface identifier of the new face
   *
   * @note that newface may or may not be equal to face1 or face2,
   *       while face1 should never be the same as face2.
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   *
   */
  int (*updateTopoGeomFaceHeal) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID face1, RTT_ELEMID face2, RTT_ELEMID newface
  );

  /**
   * Check TopoGeometry objects before a node removal event
   *
   * @param topo the topology to act upon
   * @param rem_node identifier of the node that's been removed
   * @param e1 identifier of the first connected edge
   * @param e2 identifier of the second connected edge
   *
   * The operation should be forbidden if any TopoGeometry object
   * exists which contains only one of the two healed edges.
   *
   * The operation should also be forbidden if the removed node
   * takes part in the definition of a TopoGeometry, although
   * this wasn't the case yet as of PostGIS version 2.1.8:
   * https://trac.osgeo.org/postgis/ticket/3239
   *
   * @return 1 to allow, 0 to forbid the operation
   *         (reporting reason via lastErrorMessage)
   *
   */
  int (*checkTopoGeomRemNode) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID rem_node,
      RTT_ELEMID e1,
      RTT_ELEMID e2
  );

  /**
   * Update TopoGeometry objects after healing two edges
   *
   * @param topo the topology to act upon
   * @param edge1 identifier of the first edge
   * @param edge2 identifier of the second edge
   * @param newedge identifier of the new edge, taking the space
   *                previously occupied by both original edges
   *
   * @note that newedge may or may not be equal to edge1 or edge2,
   *       while edge1 should never be the same as edge2.
   *
   * @return 1 on success, 0 on error (@see lastErrorMessage)
   *
   */
  int (*updateTopoGeomEdgeHeal) (
      const RTT_BE_TOPOLOGY* topo,
      RTT_ELEMID edge1, RTT_ELEMID edge2, RTT_ELEMID newedge
  );

  /**
   * Get faces whose bounding box overlaps a given 2D bounding box
   *
   * @param topo the topology to act upon
   * @param box the query box
   * @param numelems output parameter, gets number of elements found
   *                 if the return is not null, otherwise see @return
   *                 section for semantic.
   * @param fields fields to be filled in the returned structure, see
   *               RTT_COL_FACE_* macros
   * @param limit max number of faces to return, 0 for no limit, -1
   *              to only check for existance if a matching row.
   *
   * @return an array of faces or null in the following cases:
   *         - limit=-1 ("numelems" is set to 1 if found, 0 otherwise)
   *         - limit>0 and no records found ("numelems" is set to 0)
   *         - error ("numelems" is set to -1)
   *
   */
  RTT_ISO_FACE* (*getFaceWithinBox2D) (
      const RTT_BE_TOPOLOGY* topo,
      const RTGBOX* box,
      int* numelems, int fields, int limit
  );

} RTT_BE_CALLBACKS;


/**
 * Create a new backend interface
 *
 * Ownership to caller delete with rtt_FreeBackendIface
 *
 * @param ctx librtgeom context, create with rtgeom_init
 * @param data Backend data, passed as first parameter
 *             to all callback functions
 */
RTT_BE_IFACE* rtt_CreateBackendIface(const RTCTX* ctx,
                                     const RTT_BE_DATA* data);

/**
 * Register backend callbacks into the opaque iface handler
 *
 * @param iface the backend interface handler (see rtt_CreateBackendIface)
 * @param cb a pointer to the callbacks structure; ownership left to caller.
 */
void rtt_BackendIfaceRegisterCallbacks(RTT_BE_IFACE* iface,
                                       const RTT_BE_CALLBACKS* cb);

/** Release memory associated with an RTT_BE_IFACE */
void rtt_FreeBackendIface(RTT_BE_IFACE* iface);

/********************************************************************
 *
 * End of BE interface
 *
 *******************************************************************/

/**
 * Topology errors type
 */
typedef enum RTT_TOPOERR_TYPE_T {
  RTT_TOPOERR_EDGE_CROSSES_NODE,
  RTT_TOPOERR_EDGE_INVALID,
  RTT_TOPOERR_EDGE_NOT_SIMPLE,
  RTT_TOPOERR_EDGE_CROSSES_EDGE,
  RTT_TOPOERR_EDGE_STARTNODE_MISMATCH,
  RTT_TOPOERR_EDGE_ENDNODE_MISMATCH,
  RTT_TOPOERR_FACE_WITHOUT_EDGES,
  RTT_TOPOERR_FACE_HAS_NO_RINGS,
  RTT_TOPOERR_FACE_OVERLAPS_FACE,
  RTT_TOPOERR_FACE_WITHIN_FACE
} RTT_TOPOERR_TYPE;

/** Topology error */
typedef struct RTT_TOPOERR_T {
  /** Type of error */
  RTT_TOPOERR_TYPE err;
  /** Identifier of first affected element */
  RTT_ELEMID elem1;
  /** Identifier of second affected element (0 if inapplicable) */
  RTT_ELEMID elem2;
} RTT_TOPOERR;

/*
 * Topology functions
 */

/** Opaque topology structure
 *
 * Embeds backend interface and topology
 */
typedef struct RTT_TOPOLOGY_T RTT_TOPOLOGY;


/*******************************************************************
 *
 * Non-ISO signatures here
 *
 *******************************************************************/

/**
 * Initializes a new topology
 *
 * @param iface the backend interface handler (see rtt_CreateBackendIface)
 * @param name name of the new topology
 * @param srid the topology SRID
 * @param prec the topology precision/tolerance
 * @param hasz non-zero if topology primitives should have a Z ordinate
 *
 * @return the handler of the topology, or NULL on error
 *         (librtgeom error handler will be invoked with error message)
 */
RTT_TOPOLOGY *rtt_CreateTopology(RTT_BE_IFACE *iface, const char *name,
                        int srid, double prec, int hasz);

/**
 * Loads an existing topology by name from the database
 *
 * @param iface the backend interface handler (see rtt_CreateBackendIface)
 * @param name name of the topology to load
 *
 * @return the handler of the topology, or NULL on error
 *         (librtgeom error handler will be invoked with error message)
 */
RTT_TOPOLOGY *rtt_LoadTopology(RTT_BE_IFACE *iface, const char *name);

/**
 * Drop a topology and all its associated objects from the database
 *
 * @param topo the topology to drop
 */
void rtt_DropTopology(RTT_TOPOLOGY* topo);

/** Release memory associated with an RTT_TOPOLOGY
 *
 * @param topo the topology to release (it's not removed from db)
 */
void rtt_FreeTopology(RTT_TOPOLOGY* topo);

/**
 * Retrieve the id of a node at a point location
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for a node
 * @return a node identifier if one is found, 0 if none is found, -1
 *         on error (multiple nodes within distance).
 *         The librtgeom error handler will be invoked in case of error.
 */
RTT_ELEMID rtt_GetNodeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol);

/**
 * Find the edge-id of an edge that intersects a given point
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for an
 *            intersecting edge
 * @return an edge identifier if one is found, 0 if none is found, -1
 *         on error (multiple edges within distance).
 *         The librtgeom error handler will be invoked in case of error.
 */
RTT_ELEMID rtt_GetEdgeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol);

/**
 * Find the face-id of a face containing a given point
 *
 * @param topo the topology to operate on
 * @param point the point to use for query
 * @param tol max distance around the given point to look for a
 *            containing face
 * @return a face identifier if one is found (0 if universe), -1
 *         on error (multiple faces within distance or point on node
 *         or edge).
 *         The librtgeom error handler will be invoked in case of error.
 */
RTT_ELEMID rtt_GetFaceByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol);


/*******************************************************************
 *
 * Topology population (non-ISO)
 *
 *******************************************************************/

/**
 * Adds a point to the topology
 *
 * The given point will snap to existing nodes or edges within given
 * tolerance. An existing edge may be split by the point.
 *
 * @param topo the topology to operate on
 * @param point the point to add
 * @param tol snap tolerance, the topology tolerance will be used if -1
 *
 * @return identifier of added (or pre-existing) node or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 */
RTT_ELEMID rtt_AddPoint(RTT_TOPOLOGY* topo, RTPOINT* point, double tol);

/**
 * Adds a linestring to the topology
 *
 * The given line will snap to existing nodes or edges within given
 * tolerance. Existing edges or faces may be split by the line.
 *
 * @param topo the topology to operate on
 * @param line the line to add
 * @param tol snap tolerance, the topology tolerance will be used if -1
 * @param nedges output parameter, will be set to number of edges the
 *               line was split into, or -1 on error
 *               (librtgeom error handler will be invoked with error message)
 *
 * @return an array of <nedges> edge identifiers that sewed togheter
 *         will build up the input linestring (after snapping). Caller
 *         will need to free the array using rtfree(const RTCTX *ctx),
 *         if not null.
 */
RTT_ELEMID* rtt_AddLine(RTT_TOPOLOGY* topo, RTLINE* line, double tol,
                        int* nedges);

/**
 * Adds a linestring to the topology without determining generated faces
 *
 * The given line will snap to existing nodes or edges within given
 * tolerance. Existing edges or faces may be split by the line.
 * Side faces for the new edges will not be determined and no new
 * faces will be created, effectively leaving the topology in an
 * invalid state (WARNING!)
 *
 * @param topo the topology to operate on
 * @param line the line to add
 * @param tol snap tolerance, the topology tolerance will be used if -1
 * @param nedges output parameter, will be set to number of edges the
 *               line was split into, or -1 on error
 *               (librtgeom error handler will be invoked with error message)
 *
 * @return an array of <nedges> edge identifiers that sewed togheter
 *         will build up the input linestring (after snapping). Caller
 *         will need to free the array using rtfree(const RTCTX *ctx),
 *         if not null.
 */
RTT_ELEMID* rtt_AddLineNoFace(RTT_TOPOLOGY* topo, RTLINE* line, double tol,
                        int* nedges);

/**
 * Determine and register all topology faces:
 *
 *  - Determines which faces are generated by existing
 *    edges.
 *  - Creates face records with correct mbr
 *  - Update edge left/right face attributes
 *
 *  Precondition:
 *     - the topology edges are correctly linked
 *
 *  Postconditions:
 *     - all left/right face attributes of edges
 *       reference faces with correct mbr.
 *
 *  Notes:
 *     - does not attempt to assign isolated nodes to their
 *       containing faces
 *     - does not remove existing face records
 *     - loads in memory all the topology edges
 *
 * @param topo the topology to operate on
 *
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error message)
 */
int rtt_Polygonize(RTT_TOPOLOGY* topo);

/**
 * Adds a polygon to the topology
 *
 * The boundary of the given polygon will snap to existing nodes or
 * edges within given tolerance.
 * Existing edges or faces may be split by the boundary of the polygon.
 *
 * @param topo the topology to operate on
 * @param poly the polygon to add
 * @param tol snap tolerance, the topology tolerance will be used if -1
 * @param nfaces output parameter, will be set to number of faces the
 *               polygon was split into, or -1 on error
 *               (librtgeom error handler will be invoked with error message)
 *
 * @return an array of <nfaces> face identifiers that sewed togheter
 *         will build up the input polygon (after snapping). Caller
 *         will need to free the array using rtfree(const RTCTX *ctx),
 *         if not null.
 */
RTT_ELEMID* rtt_AddPolygon(RTT_TOPOLOGY* topo, RTPOLY* poly, double tol,
                        int* nfaces);

/*******************************************************************
 *
 * ISO signatures here
 *
 *******************************************************************/

/**
 * Populate an empty topology with data from a simple geometry
 *
 * For ST_CreateTopoGeo
 *
 * @param topo the topology to operate on
 * @param geom the geometry to import
 *
 */
void rtt_CreateTopoGeo(RTT_TOPOLOGY* topo, RTGEOM *geom);

/**
 * Add an isolated node
 *
 * For ST_AddIsoNode
 *
 * @param topo the topology to operate on
 * @param face the identifier of containing face or -1 for "unknown"
 * @param pt the node position
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident nodes, crossing edges,
 *                    actual face containement)
 *
 * @return ID of the newly added node, or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_AddIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID face,
                          RTPOINT* pt, int skipChecks);

/**
 * Move an isolated node
 *
 * For ST_MoveIsoNode
 *
 * @param topo the topology to operate on
 * @param node the identifier of the nod to be moved
 * @param pt the new node position
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
int rtt_MoveIsoNode(RTT_TOPOLOGY* topo,
                    RTT_ELEMID node, RTPOINT* pt);

/**
 * Remove an isolated node
 *
 * For ST_RemoveIsoNode
 *
 * @param topo the topology to operate on
 * @param node the identifier of the node to be moved
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
int rtt_RemoveIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID node);

/**
 * Remove an isolated edge
 *
 * For ST_RemIsoEdge
 *
 * @param topo the topology to operate on
 * @param edge the identifier of the edge to be moved
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
int rtt_RemIsoEdge(RTT_TOPOLOGY* topo, RTT_ELEMID edge);

/**
 * Add an isolated edge connecting two existing isolated nodes
 *
 * For ST_AddIsoEdge
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @return ID of the newly added edge, or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_AddIsoEdge(RTT_TOPOLOGY* topo,
                          RTT_ELEMID startNode, RTT_ELEMID endNode,
                          const RTLINE *geom);

/**
 * Add a new edge possibly splitting a face (modifying it)
 *
 * For ST_AddEdgeModFace
 *
 * If the new edge splits a face, the face is shrinked and a new one
 * is created. Unless the face being split is the Universal Face, the
 * new face will be on the right side of the newly added edge.
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @param skipChecks if non-zero skips consistency checks
 *                   (curve being simple and valid, start/end nodes
 *                    consistency actual face containement)
 *
 * @return ID of the newly added edge or null on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_AddEdgeModFace(RTT_TOPOLOGY* topo,
                              RTT_ELEMID start_node, RTT_ELEMID end_node,
                              RTLINE *geom, int skipChecks);

/**
 * Add a new edge possibly splitting a face (replacing with two new faces)
 *
 * For ST_AddEdgeNewFaces
 *
 * If the new edge splits a face, the face is replaced by two new faces.
 *
 * @param topo the topology to operate on
 * @param start_node identifier of the starting node
 * @param end_node identifier of the ending node
 * @param geom the edge geometry
 * @param skipChecks if non-zero skips consistency checks
 *                   (curve being simple and valid, start/end nodes
 *                    consistency actual face containement)
 * @return ID of the newly added edge
 *
 */
RTT_ELEMID rtt_AddEdgeNewFaces(RTT_TOPOLOGY* topo,
                              RTT_ELEMID start_node, RTT_ELEMID end_node,
                              RTLINE *geom, int skipChecks);

/**
 * Remove an edge, possibly merging two faces (replacing both with a new one)
 *
 * For ST_RemEdgeNewFace
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be removed
 * @return the id of newly created face, 0 if no new face was created
 *         or -1 on error
 *
 */
RTT_ELEMID rtt_RemEdgeNewFace(RTT_TOPOLOGY* topo, RTT_ELEMID edge);

/**
 * Remove an edge, possibly merging two faces (replacing one with the other)
 *
 * For ST_RemEdgeModFace
 *
 * Preferentially keep the face on the right, to be symmetric with
 * rtt_AddEdgeModFace.
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be removed
 * @return the id of the face that takes the space previously occupied
 *         by the removed edge, or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_RemEdgeModFace(RTT_TOPOLOGY* topo, RTT_ELEMID edge);

/**
 * Changes the shape of an edge without affecting the topology structure.
 *
 * For ST_ChangeEdgeGeom
 *
 * @param topo the topology to operate on
 * @param curve the edge geometry
 * @return 0 on success, -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
int rtt_ChangeEdgeGeom(RTT_TOPOLOGY* topo, RTT_ELEMID edge, RTLINE* curve);

/**
 * Split an edge by a node, modifying the original edge and adding a new one.
 *
 * For ST_ModEdgeSplit
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be split
 * @param pt geometry of the new node
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident node, point not on edge...)
 * @return the id of newly created node, or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_ModEdgeSplit(RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                            RTPOINT* pt, int skipChecks);

/**
 * Split an edge by a node, replacing it with two new edges
 *
 * For ST_NewEdgesSplit
 *
 * @param topo the topology to operate on
 * @param edge identifier of the edge to be split
 * @param pt geometry of the new node
 * @param skipChecks if non-zero skips consistency checks
 *                   (coincident node, point not on edge...)
 * @return the id of newly created node
 *
 */
RTT_ELEMID rtt_NewEdgesSplit(RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                             RTPOINT* pt, int skipChecks);

/**
 * Merge two edges, modifying the first and deleting the second
 *
 * For ST_ModEdgeHeal
 *
 * @param topo the topology to operate on
 * @param e1 identifier of first edge
 * @param e2 identifier of second edge
 * @return the id of the removed node or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_ModEdgeHeal(RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2);

/**
 * Merge two edges, replacing both with a new one
 *
 * For ST_NewEdgeHeal
 *
 * @param topo the topology to operate on
 * @param e1 identifier of first edge
 * @param e2 identifier of second edge
 * @return the id of the new edge or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
RTT_ELEMID rtt_NewEdgeHeal(RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2);

/**
 * Return the list of directed edges bounding a face
 *
 * For ST_GetFaceEdges
 *
 * @param topo the topology to operate on
 * @param face identifier of the face
 * @param edges will be set to an array of signed edge identifiers, will
 *              need to be released with rtfree
 * @return the number of edges in the edges array, or -1 on error
 *         (librtgeom error handler will be invoked with error message)
 *
 */
int rtt_GetFaceEdges(RTT_TOPOLOGY* topo, RTT_ELEMID face, RTT_ELEMID **edges);

/**
 * Return the geometry of a face
 *
 * For ST_GetFaceGeometry
 *
 * @param topo the topology to operate on
 * @param face identifier of the face
 * @return a polygon geometry representing the face, ownership to caller,
 *         to be released with rtgeom_release, or NULL on error
 *         (librtgeom error handler will be invoked with error message)
 */
RTGEOM* rtt_GetFaceGeometry(RTT_TOPOLOGY* topo, RTT_ELEMID face);

/*******************************************************************
 *
 * Utility functions
 *
 *******************************************************************/


/*
 * rtt_tpsnap - snap geometry to topology
 *
 * Uses Trevisani-Peri algorithm version 13 as reported here:
 * https://git.osgeo.org/gitea/rttopo/librttopo/wiki/SnapToTopo-algorithm
 *
 * @param topo the reference topology
 * @param gin the input geometry
 * @param tolerance_snap snap tolerance
 * @param tolerance_removal removal tolerance (use -1 to skip removal phase)
 * @param iterate if non zero, allows snapping to more than a single vertex,
 *                iteratively
 *
 * @return a new geometry, or NULL on error
 *
 */
RTGEOM* rtt_tpsnap(RTT_TOPOLOGY *topo, const RTGEOM *gin,
                   double tolerance_snap,
                   double tolerance_removal,
                   int iterate);

#endif /* LIBRTGEOM_TOPO_H */
