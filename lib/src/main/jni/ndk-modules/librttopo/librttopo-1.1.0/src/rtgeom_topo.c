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
 * Copyright (C) 2015-2016 Sandro Santilli <strk@kbt.io>
 *
 **********************************************************************
 *
 * Last port: lwgeom_topo.c r14853
 *
 **********************************************************************/




#include "rttopo_config.h"

/*#define RTGEOM_DEBUG_LEVEL 1*/
#include "rtgeom_log.h"

#include "librttopo_geom_internal.h"
#include "librttopo_internal.h"
#include "rtgeom_geos.h"

#include <stdio.h>
#include <inttypes.h> /* for PRId64 */
#include <errno.h>
#include <math.h>

#ifdef WIN32
# define RTTFMT_ELEMID "lld"
#else
# define RTTFMT_ELEMID PRId64
#endif

/* TODO: move this to rtgeom_log.h */
#define RTDEBUGG(ctx, level, geom, msg) \
  if (RTGEOM_DEBUG_LEVEL >= level) \
  do { \
    size_t sz; \
    char *wkt1 = rtgeom_to_wkt(ctx, geom, RTWKT_EXTENDED, 15, &sz); \
    /* char *wkt1 = rtgeom_to_hexwkb(iface->ctx, geom, RTWKT_EXTENDED, &sz); */ \
    RTDEBUGF(ctx, level, msg ": %s", wkt1); \
    rtfree(ctx, wkt1); \
  } while (0);


/*********************************************************************
 *
 * Backend iface
 *
 ********************************************************************/

RTT_BE_IFACE* rtt_CreateBackendIface(const RTCTX *ctx, const RTT_BE_DATA *data)
{
  RTT_BE_IFACE *iface = rtalloc(ctx, sizeof(RTT_BE_IFACE));
  iface->data = data;
  iface->cb = NULL;
  iface->ctx = ctx;
  return iface;
}

void rtt_BackendIfaceRegisterCallbacks(RTT_BE_IFACE *iface,
                                       const RTT_BE_CALLBACKS* cb)
{
  iface->cb = cb;
}

void rtt_FreeBackendIface(RTT_BE_IFACE* iface)
{
  rtfree(iface->ctx, iface);
}

static void _rtt_EnsureGeos(const RTCTX *ctx)
{
  rtgeom_geos_ensure_init(ctx);
}

/*********************************************************************
 *
 * Backend wrappers
 *
 ********************************************************************/

#define CHECKCB(be, method) do { \
  if ( ! (be)->cb || ! (be)->cb->method ) \
  rterror((be)->ctx, "Callback " # method " not registered by backend"); \
} while (0)

#define CB0(be, method) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data)

#define CB1(be, method, a1) \
  CHECKCB(be, method);\
  return (be)->cb->method((be)->data, a1)

#define CBT0(to, method) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo)

#define CBT1(to, method, a1) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1)

#define CBT2(to, method, a1, a2) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2)

#define CBT3(to, method, a1, a2, a3) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3)

#define CBT4(to, method, a1, a2, a3, a4) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4)

#define CBT5(to, method, a1, a2, a3, a4, a5) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5)

#define CBT6(to, method, a1, a2, a3, a4, a5, a6) \
  CHECKCB((to)->be_iface, method);\
  return (to)->be_iface->cb->method((to)->be_topo, a1, a2, a3, a4, a5, a6)

const char *
rtt_be_lastErrorMessage(const RTT_BE_IFACE* be)
{
  CB0(be, lastErrorMessage);
}

RTT_BE_TOPOLOGY *
rtt_be_loadTopologyByName(RTT_BE_IFACE *be, const char *name)
{
  CB1(be, loadTopologyByName, name);
}

static int
rtt_be_topoGetSRID(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetSRID);
}

static double
rtt_be_topoGetPrecision(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoGetPrecision);
}

static int
rtt_be_topoHasZ(RTT_TOPOLOGY *topo)
{
  CBT0(topo, topoHasZ);
}

int
rtt_be_freeTopology(RTT_TOPOLOGY *topo)
{
  CBT0(topo, freeTopology);
}

RTT_ISO_NODE*
rtt_be_getNodeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getNodeById, ids, numelems, fields);
}

RTT_ISO_NODE*
rtt_be_getNodeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getNodeWithinDistance2D, pt, dist, numelems, fields, limit);
}

static RTT_ISO_NODE*
rtt_be_getNodeWithinBox2D( const RTT_TOPOLOGY* topo,
                           const RTGBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getNodeWithinBox2D, box, numelems, fields, limit);
}

RTT_ISO_EDGE*
rtt_be_getEdgeWithinBox2D( const RTT_TOPOLOGY* topo,
                           const RTGBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getEdgeWithinBox2D, box, numelems, fields, limit);
}

static RTT_ISO_FACE*
rtt_be_getFaceWithinBox2D( const RTT_TOPOLOGY* topo,
                           const RTGBOX* box, int* numelems, int fields,
                           int limit )
{
  CBT4(topo, getFaceWithinBox2D, box, numelems, fields, limit);
}

int
rtt_be_insertNodes(RTT_TOPOLOGY* topo, RTT_ISO_NODE* node, int numelems)
{
  CBT2(topo, insertNodes, node, numelems);
}

static int
rtt_be_insertFaces(RTT_TOPOLOGY* topo, RTT_ISO_FACE* face, int numelems)
{
  CBT2(topo, insertFaces, face, numelems);
}

static int
rtt_be_deleteFacesById(const RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int numelems)
{
  CBT2(topo, deleteFacesById, ids, numelems);
}

static int
rtt_be_deleteNodesById(const RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int numelems)
{
  CBT2(topo, deleteNodesById, ids, numelems);
}

RTT_ELEMID
rtt_be_getNextEdgeId(RTT_TOPOLOGY* topo)
{
  CBT0(topo, getNextEdgeId);
}

RTT_ISO_EDGE*
rtt_be_getEdgeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeById, ids, numelems, fields);
}

static RTT_ISO_FACE*
rtt_be_getFaceById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getFaceById, ids, numelems, fields);
}

static RTT_ISO_EDGE*
rtt_be_getEdgeByNode(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields)
{
  CBT3(topo, getEdgeByNode, ids, numelems, fields);
}

static RTT_ISO_EDGE*
rtt_be_getEdgeByFace(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields, const RTGBOX *box)
{
  CBT4(topo, getEdgeByFace, ids, numelems, fields, box);
}

static RTT_ISO_NODE*
rtt_be_getNodeByFace(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                   int* numelems, int fields, const RTGBOX *box)
{
  CBT4(topo, getNodeByFace, ids, numelems, fields, box);
}

RTT_ISO_EDGE*
rtt_be_getEdgeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit)
{
  CBT5(topo, getEdgeWithinDistance2D, pt, dist, numelems, fields, limit);
}

int
rtt_be_insertEdges(RTT_TOPOLOGY* topo, RTT_ISO_EDGE* edge, int numelems)
{
  CBT2(topo, insertEdges, edge, numelems);
}

int
rtt_be_updateEdges(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* sel_edge, int sel_fields,
  const RTT_ISO_EDGE* upd_edge, int upd_fields,
  const RTT_ISO_EDGE* exc_edge, int exc_fields
)
{
  CBT6(topo, updateEdges, sel_edge, sel_fields,
                          upd_edge, upd_fields,
                          exc_edge, exc_fields);
}

static int
rtt_be_updateNodes(RTT_TOPOLOGY* topo,
  const RTT_ISO_NODE* sel_node, int sel_fields,
  const RTT_ISO_NODE* upd_node, int upd_fields,
  const RTT_ISO_NODE* exc_node, int exc_fields
)
{
  CBT6(topo, updateNodes, sel_node, sel_fields,
                          upd_node, upd_fields,
                          exc_node, exc_fields);
}

static int
rtt_be_updateFacesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_FACE* faces, int numfaces
)
{
  CBT2(topo, updateFacesById, faces, numfaces);
}

static int
rtt_be_updateEdgesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* edges, int numedges, int upd_fields
)
{
  CBT3(topo, updateEdgesById, edges, numedges, upd_fields);
}

static int
rtt_be_updateNodesById(RTT_TOPOLOGY* topo,
  const RTT_ISO_NODE* nodes, int numnodes, int upd_fields
)
{
  CBT3(topo, updateNodesById, nodes, numnodes, upd_fields);
}

int
rtt_be_deleteEdges(RTT_TOPOLOGY* topo,
  const RTT_ISO_EDGE* sel_edge, int sel_fields
)
{
  CBT2(topo, deleteEdges, sel_edge, sel_fields);
}

RTT_ELEMID
rtt_be_getFaceContainingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  CBT1(topo, getFaceContainingPoint, pt);
}


int
rtt_be_updateTopoGeomEdgeSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_edge, RTT_ELEMID new_edge1, RTT_ELEMID new_edge2)
{
  CBT3(topo, updateTopoGeomEdgeSplit, split_edge, new_edge1, new_edge2);
}

static int
rtt_be_updateTopoGeomFaceSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_face,
                               RTT_ELEMID new_face1, RTT_ELEMID new_face2)
{
  CBT3(topo, updateTopoGeomFaceSplit, split_face, new_face1, new_face2);
}

static int
rtt_be_checkTopoGeomRemEdge(RTT_TOPOLOGY* topo, RTT_ELEMID edge_id,
                            RTT_ELEMID face_left, RTT_ELEMID face_right)
{
  CBT3(topo, checkTopoGeomRemEdge, edge_id, face_left, face_right);
}

static int
rtt_be_checkTopoGeomRemNode(RTT_TOPOLOGY* topo, RTT_ELEMID node_id,
                            RTT_ELEMID eid1, RTT_ELEMID eid2)
{
  CBT3(topo, checkTopoGeomRemNode, node_id, eid1, eid2);
}

static int
rtt_be_updateTopoGeomFaceHeal(RTT_TOPOLOGY* topo,
                             RTT_ELEMID face1, RTT_ELEMID face2,
                             RTT_ELEMID newface)
{
  CBT3(topo, updateTopoGeomFaceHeal, face1, face2, newface);
}

static int
rtt_be_updateTopoGeomEdgeHeal(RTT_TOPOLOGY* topo,
                             RTT_ELEMID edge1, RTT_ELEMID edge2,
                             RTT_ELEMID newedge)
{
  CBT3(topo, updateTopoGeomEdgeHeal, edge1, edge2, newedge);
}

static RTT_ELEMID*
rtt_be_getRingEdges( RTT_TOPOLOGY* topo,
                     RTT_ELEMID edge, int *numedges, int limit )
{
  CBT3(topo, getRingEdges, edge, numedges, limit);
}


/* wrappers of backend wrappers... */

int
rtt_be_ExistsCoincidentNode(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  int exists = 0;
  rtt_be_getNodeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    rterror(topo->be_iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

int
rtt_be_ExistsEdgeIntersectingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt)
{
  int exists = 0;
  rtt_be_getEdgeWithinDistance2D(topo, pt, 0, &exists, 0, -1);
  if ( exists == -1 ) {
    rterror(topo->be_iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  return exists;
}

/************************************************************************
 *
 * Utility functions
 *
 ************************************************************************/

static RTGEOM *
_rtt_toposnap(const RTCTX *ctx, RTGEOM *src, RTGEOM *tgt, double tol)
{
  RTGEOM *tmp = src;
  RTGEOM *tmp2;
  int changed;
  int iterations = 0;

  int maxiterations = rtgeom_count_vertices(ctx, tgt);

  /* GEOS snapping can be unstable */
  /* See https://trac.osgeo.org/geos/ticket/760 */
  do {
    RTGEOM *tmp3;
    tmp2 = rtgeom_snap(ctx, tmp, tgt, tol);
    ++iterations;
    changed = ( rtgeom_count_vertices(ctx, tmp2) != rtgeom_count_vertices(ctx, tmp) );
#if GEOS_NUMERIC_VERSION < 30309
    /* Up to GEOS-3.3.8, snapping could duplicate points */
    if ( changed ) {
      tmp3 = rtgeom_remove_repeated_points(ctx, tmp2, 0 );
      rtgeom_free(ctx, tmp2);
      tmp2 = tmp3;
      changed = ( rtgeom_count_vertices(ctx, tmp2) != rtgeom_count_vertices(ctx, tmp) );
    }
#endif /* GEOS_NUMERIC_VERSION < 30309 */
    RTDEBUGF(ctx, 2, "After iteration %d, geometry changed ? %d (%d vs %d vertices)", iterations, changed, rtgeom_count_vertices(ctx, tmp2), rtgeom_count_vertices(ctx, tmp));
    if ( tmp != src ) rtgeom_free(ctx, tmp);
    tmp = tmp2;
  } while ( changed && iterations <= maxiterations );

  RTDEBUGF(ctx, 1, "It took %d/%d iterations to properly snap",
              iterations, maxiterations);

  return tmp;
}

static void
_rtt_release_faces(const RTCTX *ctx, RTT_ISO_FACE *faces, int num_faces)
{
  int i;
  for ( i=0; i<num_faces; ++i ) {
    if ( faces[i].mbr ) rtfree(ctx, faces[i].mbr);
  }
  rtfree(ctx, faces);
}

void
rtt_release_edges(const RTCTX *ctx, RTT_ISO_EDGE *edges, int num_edges)
{
  int i;
  for ( i=0; i<num_edges; ++i ) {
    if ( edges[i].geom ) rtline_free(ctx, edges[i].geom);
  }
  rtfree(ctx, edges);
}

static void
_rtt_release_nodes(const RTCTX *ctx, RTT_ISO_NODE *nodes, int num_nodes)
{
  int i;
  for ( i=0; i<num_nodes; ++i ) {
    if ( nodes[i].geom ) rtpoint_free(ctx, nodes[i].geom);
  }
  rtfree(ctx, nodes);
}

/************************************************************************
 *
 * API implementation
 *
 ************************************************************************/

RTT_TOPOLOGY *
rtt_LoadTopology( RTT_BE_IFACE *iface, const char *name )
{
  RTT_BE_TOPOLOGY* be_topo;
  RTT_TOPOLOGY* topo;

  be_topo = rtt_be_loadTopologyByName(iface, name);
  if ( ! be_topo ) {
    //rterror(iface->ctx, "Could not load topology from backend: %s",
    rterror(iface->ctx, "%s", rtt_be_lastErrorMessage(iface));
    return NULL;
  }
  topo = rtalloc(iface->ctx, sizeof(RTT_TOPOLOGY));
  topo->be_iface = iface;
  topo->be_topo = be_topo;
  topo->srid = rtt_be_topoGetSRID(topo);
  topo->hasZ = rtt_be_topoHasZ(topo);
  topo->precision = rtt_be_topoGetPrecision(topo);

  return topo;
}

void
rtt_FreeTopology( RTT_TOPOLOGY* topo )
{
  const RTT_BE_IFACE *iface = topo->be_iface;

  if ( ! rtt_be_freeTopology(topo) ) {
    rtnotice(topo->be_iface->ctx, "Could not release backend topology memory: %s",
            rtt_be_lastErrorMessage(topo->be_iface));
  }
  rtfree(iface->ctx, topo);
}

/**
 * @param checkFace if non zero will check the given face
 *        for really containing the point or determine the
 *        face when given face is -1. Use 0 to simply use
 *        the given face value, no matter what (effectively
 *        allowing adding a non-isolated point when used
 *        with face=-1).
 */
static RTT_ELEMID
_rtt_AddIsoNode( RTT_TOPOLOGY* topo, RTT_ELEMID face,
                RTPOINT* pt, int skipISOChecks, int checkFace )
{
  RTT_ELEMID foundInFace = -1;
  const RTT_BE_IFACE *iface = topo->be_iface;

  if ( ! skipISOChecks )
  {
    if ( rtt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - coincident node");
      return -1;
    }
    if ( rtt_be_ExistsEdgeIntersectingPoint(topo, pt) ) /*x*/
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - edge crosses node.");
      return -1;
    }
  }

  if ( checkFace && ( face == -1 || ! skipISOChecks ) )
  {
    foundInFace = rtt_be_getFaceContainingPoint(topo, pt); /*x*/
    if ( foundInFace == -2 ) {
      rterror(topo->be_iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( foundInFace == -1 ) foundInFace = 0;
  }

  if ( face == -1 ) {
    face = foundInFace;
  }
  else if ( ! skipISOChecks && foundInFace != face ) {
#if 0
    rterror(iface->ctx, "SQL/MM Spatial exception - within face %d (not %d)",
            foundInFace, face);
#else
    rterror(topo->be_iface->ctx, "SQL/MM Spatial exception - not within face");
#endif
    return -1;
  }

  RTT_ISO_NODE node;
  node.node_id = -1;
  node.containing_face = face;
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    rterror(topo->be_iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return node.node_id;
}

RTT_ELEMID
rtt_AddIsoNode( RTT_TOPOLOGY* topo, RTT_ELEMID face,
                RTPOINT* pt, int skipISOChecks )
{
  return _rtt_AddIsoNode( topo, face, pt, skipISOChecks, 1 );
}

/* Check that an edge does not cross an existing node or edge
 *
 * @param myself the id of an edge to skip, if any
 *               (for ChangeEdgeGeom). Can use 0 for none.
 *
 * Return -1 on cross or error, 0 if everything is fine.
 * Note that before returning -1, rterror is invoked...
 */
static int
_rtt_CheckEdgeCrossing( RTT_TOPOLOGY* topo,
                        RTT_ELEMID start_node, RTT_ELEMID end_node,
                        const RTLINE *geom, RTT_ELEMID myself )
{
  int i, num_nodes, num_edges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_NODE *nodes;
  const RTGBOX *edgebox;
  GEOSGeometry *edgegg;
  const GEOSPreparedGeometry* prepared_edge;
  const RTT_BE_IFACE *iface = topo->be_iface;

  _rtt_EnsureGeos(iface->ctx);

  edgegg = RTGEOM2GEOS(iface->ctx,  rtline_as_rtgeom(iface->ctx, geom), 0);
  if ( ! edgegg ) {
    rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
    return -1;
  }
  prepared_edge = GEOSPrepare_r(iface->ctx->gctx,  edgegg );
  if ( ! prepared_edge ) {
    rterror(iface->ctx, "Could not prepare edge geometry: %s", rtgeom_get_last_geos_error(iface->ctx));
    return -1;
  }
  edgebox = rtgeom_get_bbox(iface->ctx,  rtline_as_rtgeom(iface->ctx, geom) );

  /* loop over each node within the edge's gbox */
  nodes = rtt_be_getNodeWithinBox2D( topo, edgebox, &num_nodes,
                                            RTT_COL_NODE_ALL, 0 );
  RTDEBUGF(iface->ctx, 1, "rtt_be_getNodeWithinBox2D returned %d nodes", num_nodes);
  if ( num_nodes == -1 ) {
    GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
    GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    RTT_ISO_NODE* node = &(nodes[i]);
    GEOSGeometry *nodegg;
    int contains;
    if ( node->node_id == start_node ) continue;
    if ( node->node_id == end_node ) continue;
    /* check if the edge contains this node (not on boundary) */
    nodegg = RTGEOM2GEOS(iface->ctx,  rtpoint_as_rtgeom(iface->ctx, node->geom) , 0);
    /* ST_RelateMatch(rec.relate, 'T********') */
    contains = GEOSPreparedContains_r(iface->ctx->gctx,  prepared_edge, nodegg );
    GEOSGeom_destroy_r(iface->ctx->gctx, nodegg);
    if (contains == 2)
    {
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      _rtt_release_nodes(iface->ctx, nodes, num_nodes);
      rterror(iface->ctx, "GEOS exception on PreparedContains: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }
    if ( contains )
    {
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      _rtt_release_nodes(iface->ctx, nodes, num_nodes);
      rterror(iface->ctx, "SQL/MM Spatial exception - geometry crosses a node");
      return -1;
    }
  }
  if ( nodes ) _rtt_release_nodes(iface->ctx, nodes, num_nodes);
               /* may be NULL if num_nodes == 0 */

  /* loop over each edge within the edge's gbox */
  edges = rtt_be_getEdgeWithinBox2D( topo, edgebox, &num_edges, RTT_COL_EDGE_ALL, 0 );
  RTDEBUGF(iface->ctx, 1, "rtt_be_getEdgeWithinBox2D returned %d edges", num_edges);
  if ( num_edges == -1 ) {
    GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
    GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_edges; ++i )
  {
    RTT_ISO_EDGE* edge = &(edges[i]);
    RTT_ELEMID edge_id = edge->edge_id;
    GEOSGeometry *eegg;
    char *relate;
    int match;

    if ( edge_id == myself ) continue;

    if ( ! edge->geom ) {
      rtt_release_edges(iface->ctx, edges, num_edges);
      rterror(iface->ctx, "Edge %d has NULL geometry!", edge_id);
      return -1;
    }

    eegg = RTGEOM2GEOS(iface->ctx,  rtline_as_rtgeom(iface->ctx, edge->geom), 0 );
    if ( ! eegg ) {
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      rtt_release_edges(iface->ctx, edges, num_edges);
      rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }

    RTDEBUGF(iface->ctx, 2, "Edge %d converted to GEOS", edge_id);

    /* check if the edge crosses our edge (not boundary-boundary) */

    relate = GEOSRelateBoundaryNodeRule_r(iface->ctx->gctx, eegg, edgegg, 2);
    if ( ! relate ) {
      GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      rtt_release_edges(iface->ctx, edges, num_edges);
      rterror(iface->ctx, "GEOSRelateBoundaryNodeRule error: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }

    RTDEBUGF(iface->ctx, 2, "Edge %d relate pattern is %s", edge_id, relate);

    match = GEOSRelatePatternMatch_r(iface->ctx->gctx, relate, "F********");
    if ( match ) {
      /* error or no interior intersection */
      GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
      GEOSFree_r(iface->ctx->gctx, relate);
      if ( match == 2 ) {
        rtt_release_edges(iface->ctx, edges, num_edges);
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
        GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
        rterror(iface->ctx, "GEOSRelatePatternMatch error: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -1;
      }
      else continue; /* no interior intersection */
    }

    match = GEOSRelatePatternMatch_r(iface->ctx->gctx, relate, "1FFF*FFF2");
    if ( match ) {
      rtt_release_edges(iface->ctx, edges, num_edges);
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
      GEOSFree_r(iface->ctx->gctx, relate);
      if ( match == 2 ) {
        rterror(iface->ctx, "GEOSRelatePatternMatch error: %s", rtgeom_get_last_geos_error(iface->ctx));
      } else {
        rterror(iface->ctx, "SQL/MM Spatial exception - coincident edge %" RTTFMT_ELEMID,
                edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch_r(iface->ctx->gctx, relate, "1********");
    if ( match ) {
      rtt_release_edges(iface->ctx, edges, num_edges);
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
      GEOSFree_r(iface->ctx->gctx, relate);
      if ( match == 2 ) {
        rterror(iface->ctx, "GEOSRelatePatternMatch error: %s", rtgeom_get_last_geos_error(iface->ctx));
      } else {
        rterror(iface->ctx, "Spatial exception - geometry intersects edge %"
                RTTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    match = GEOSRelatePatternMatch_r(iface->ctx->gctx, relate, "T********");
    if ( match ) {
      rtt_release_edges(iface->ctx, edges, num_edges);
      GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
      GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);
      GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
      GEOSFree_r(iface->ctx->gctx, relate);
      if ( match == 2 ) {
        rterror(iface->ctx, "GEOSRelatePatternMatch error: %s", rtgeom_get_last_geos_error(iface->ctx));
      } else {
        rterror(iface->ctx, "SQL/MM Spatial exception - geometry crosses edge %"
                RTTFMT_ELEMID, edge_id);
      }
      return -1;
    }

    RTDEBUGF(iface->ctx, 2, "Edge %d analisys completed, it does no harm", edge_id);

    GEOSFree_r(iface->ctx->gctx, relate);
    GEOSGeom_destroy_r(iface->ctx->gctx, eegg);
  }
  if ( edges ) rtt_release_edges(iface->ctx, edges, num_edges);
              /* would be NULL if num_edges was 0 */

  GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepared_edge);
  GEOSGeom_destroy_r(iface->ctx->gctx, edgegg);

  return 0;
}


RTT_ELEMID
rtt_AddIsoEdge( RTT_TOPOLOGY* topo, RTT_ELEMID startNode,
                RTT_ELEMID endNode, const RTLINE* geom )
{
  int num_nodes;
  int i;
  RTT_ISO_EDGE newedge;
  RTT_ISO_NODE *endpoints;
  RTT_ELEMID containing_face = -1;
  RTT_ELEMID node_ids[2];
  RTT_ISO_NODE updated_nodes[2];
  int skipISOChecks = 0;
  RTPOINT2D p1, p2;
  const RTT_BE_IFACE *iface = topo->be_iface;

  /* NOT IN THE SPECS:
   * A closed edge is never isolated (as it forms a face)
   */
  if ( startNode == endNode )
  {
    rterror(iface->ctx, "Closed edges would not be isolated, try rtt_AddEdgeNewFaces");
    return -1;
  }

  if ( ! skipISOChecks )
  {
    /* Acurve must be simple */
    if ( ! rtgeom_is_simple(iface->ctx, rtline_as_rtgeom(iface->ctx, geom)) )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  /*
   * Check for:
   *    existence of nodes
   *    nodes faces match
   * Extract:
   *    nodes face id
   *    nodes geoms
   */
  num_nodes = 2;
  node_ids[0] = startNode;
  node_ids[1] = endNode;
  endpoints = rtt_be_getNodeById( topo, node_ids, &num_nodes,
                                             RTT_COL_NODE_ALL );
  if ( num_nodes < 0 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  else if ( num_nodes < 2 )
  {
    if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
    rterror(iface->ctx, "SQL/MM Spatial exception - non-existent node");
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    const RTT_ISO_NODE *n = &(endpoints[i]);
    if ( n->containing_face == -1 )
    {
      _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
      rterror(iface->ctx, "SQL/MM Spatial exception - not isolated node");
      return -1;
    }
    if ( containing_face == -1 ) containing_face = n->containing_face;
    else if ( containing_face != n->containing_face )
    {
      _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
      rterror(iface->ctx, "SQL/MM Spatial exception - nodes in different faces");
      return -1;
    }

    if ( ! skipISOChecks )
    {
      if ( n->node_id == startNode )
      {
        /* l) Check that start point of acurve match start node geoms. */
        rt_getPoint2d_p(iface->ctx, geom->points, 0, &p1);
        rt_getPoint2d_p(iface->ctx, n->geom->point, 0, &p2);
        if ( ! p2d_same(iface->ctx, &p1, &p2) )
        {
          _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
          rterror(iface->ctx, "SQL/MM Spatial exception - "
                  "start node not geometry start point.");
          return -1;
        }
      }
      else
      {
        /* m) Check that end point of acurve match end node geoms. */
        rt_getPoint2d_p(iface->ctx, geom->points, geom->points->npoints-1, &p1);
        rt_getPoint2d_p(iface->ctx, n->geom->point, 0, &p2);
        if ( ! p2d_same(iface->ctx, &p1, &p2) )
        {
          _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
          rterror(iface->ctx, "SQL/MM Spatial exception - "
                  "end node not geometry end point.");
          return -1;
        }
      }
    }
  }

  if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);

  if ( ! skipISOChecks )
  {
    if ( _rtt_CheckEdgeCrossing( topo, startNode, endNode, geom, 0 ) )
    {
      /* would have called rterror already, leaking :( */
      return -1;
    }
  }

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = rtt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* TODO: this should likely be an exception instead ! */
  if ( containing_face == -1 ) containing_face = 0;

  newedge.start_node = startNode;
  newedge.end_node = endNode;
  newedge.face_left = newedge.face_right = containing_face;
  newedge.next_left = -newedge.edge_id;
  newedge.next_right = newedge.edge_id;
  newedge.geom = (RTLINE *)geom; /* const cast.. */

  int ret = rtt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rterror(iface->ctx, "Insertion of split edge failed (no reason)");
    return -1;
  }

  /*
   * Update Node containing_face values
   *
   * the nodes anode and anothernode are no more isolated
   * because now there is an edge connecting them
   */
  updated_nodes[0].node_id = startNode;
  updated_nodes[0].containing_face = -1;
  updated_nodes[1].node_id = endNode;
  updated_nodes[1].containing_face = -1;
  ret = rtt_be_updateNodesById(topo, updated_nodes, 2,
                               RTT_COL_NODE_CONTAINING_FACE);
  if ( ret == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return newedge.edge_id;
}

static RTCOLLECTION *
_rtt_EdgeSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge, RTPOINT* pt, int skipISOChecks, RTT_ISO_EDGE** oldedge )
{
  RTGEOM *split;
  RTCOLLECTION *split_col;
  int i;
  const RTT_BE_IFACE *iface = topo->be_iface;

  /* Get edge */
  i = 1;
  RTDEBUG(iface->ctx, 1, "calling rtt_be_getEdgeById");
  *oldedge = rtt_be_getEdgeById(topo, &edge, &i, RTT_COL_EDGE_ALL);
  RTDEBUGF(iface->ctx, 1, "rtt_be_getEdgeById returned %p", *oldedge);
  if ( ! *oldedge )
  {
    RTDEBUGF(iface->ctx, 1, "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    else if ( i == 0 )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge");
      return NULL;
    }
    else
    {
      rterror(iface->ctx, "Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return NULL;
    }
  }


  /*
   *  - check if a coincident node already exists
   */
  if ( ! skipISOChecks )
  {
    RTDEBUG(iface->ctx, 1, "calling rtt_be_ExistsCoincidentNode");
    if ( rtt_be_ExistsCoincidentNode(topo, pt) ) /*x*/
    {
      RTDEBUG(iface->ctx, 1, "rtt_be_ExistsCoincidentNode returned");
      rtt_release_edges(iface->ctx, *oldedge, 1);
      rterror(iface->ctx, "SQL/MM Spatial exception - coincident node");
      return NULL;
    }
    RTDEBUG(iface->ctx, 1, "rtt_be_ExistsCoincidentNode returned");
  }

  /* Split edge */
  split = rtgeom_split(iface->ctx, (RTGEOM*)(*oldedge)->geom, (RTGEOM*)pt);
  if ( ! split )
  {
    rtt_release_edges(iface->ctx, *oldedge, 1);
    rterror(iface->ctx, "could not split edge by point ?");
    return NULL;
  }
  split_col = rtgeom_as_rtcollection(iface->ctx, split);
  if ( ! split_col ) {
    rtt_release_edges(iface->ctx, *oldedge, 1);
    rtgeom_free(iface->ctx, split);
    rterror(iface->ctx, "rtgeom_as_rtcollection returned NULL");
    return NULL;
  }
  if (split_col->ngeoms < 2) {
    rtt_release_edges(iface->ctx, *oldedge, 1);
    rtgeom_free(iface->ctx, split);
    rterror(iface->ctx, "SQL/MM Spatial exception - point not on edge");
    return NULL;
  }

#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt(iface->ctx, (RTGEOM*)split_col, RTWKT_EXTENDED, 2, &sz);
  RTDEBUGF(iface->ctx, 1, "returning split col: %s", wkt);
  rtfree(iface->ctx, wkt);
  }
#endif
  return split_col;
}

RTT_ELEMID
rtt_ModEdgeSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                  RTPOINT* pt, int skipISOChecks )
{
  RTT_ISO_NODE node;
  RTT_ISO_EDGE* oldedge = NULL;
  RTCOLLECTION *split_col;
  const RTGEOM *oldedge_geom;
  const RTGEOM *newedge_geom;
  RTT_ISO_EDGE newedge1;
  RTT_ISO_EDGE seledge, updedge, excedge;
  int ret;
  const RTT_BE_IFACE *iface = topo->be_iface;

  split_col = _rtt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];
  /* Make sure the SRID is set on the subgeom */
  ((RTGEOM*)oldedge_geom)->srid = split_col->srid;
  ((RTGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    /* should have been set by backend */
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Insert the new edge */
  newedge1.edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedge1.edge_id == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedge1.start_node = node.node_id;
  newedge1.end_node = oldedge->end_node;
  newedge1.face_left = oldedge->face_left;
  newedge1.face_right = oldedge->face_right;
  newedge1.next_left = oldedge->next_left == -oldedge->edge_id ?
      -newedge1.edge_id : oldedge->next_left;
  newedge1.next_right = -oldedge->edge_id;
  newedge1.geom = rtgeom_as_rtline(iface->ctx, newedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedge1.geom ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "first geometry in rtgeom_split output is not a line");
    return -1;
  }
  ret = rtt_be_insertEdges(topo, &newedge1, 1);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update the old edge */
  updedge.geom = rtgeom_as_rtline(iface->ctx, oldedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! updedge.geom ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "second geometry in rtgeom_split output is not a line");
    return -1;
  }
  updedge.next_left = newedge1.edge_id;
  updedge.end_node = node.node_id;
  ret = rtt_be_updateEdges(topo,
      oldedge, RTT_COL_EDGE_EDGE_ID,
      &updedge, RTT_COL_EDGE_GEOM|RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      NULL, 0);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Edge being split (%d) disappeared during operations?", oldedge->edge_id);
    return -1;
  } else if ( ret > 1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "More than a single edge found with id %d !", oldedge->edge_id);
    return -1;
  }

  /* Update all next edge references to match new layout (ST_ModEdgeSplit) */

  updedge.next_right = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_right = -oldedge->edge_id;
  seledge.start_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      &excedge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedge1.edge_id;
  excedge.edge_id = newedge1.edge_id;
  seledge.next_left = -oldedge->edge_id;
  seledge.end_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      &excedge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = rtt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedge1.edge_id, -1);
  if ( ! ret ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  rtt_release_edges(iface->ctx, oldedge, 1);
  rtcollection_free(iface->ctx, split_col);

  /* return new node id */
  return node.node_id;
}

RTT_ELEMID
rtt_NewEdgesSplit( RTT_TOPOLOGY* topo, RTT_ELEMID edge,
                   RTPOINT* pt, int skipISOChecks )
{
  RTT_ISO_NODE node;
  RTT_ISO_EDGE* oldedge = NULL;
  RTCOLLECTION *split_col;
  const RTGEOM *oldedge_geom;
  const RTGEOM *newedge_geom;
  RTT_ISO_EDGE newedges[2];
  RTT_ISO_EDGE seledge, updedge;
  int ret;
  const RTT_BE_IFACE *iface = topo->be_iface;

  split_col = _rtt_EdgeSplit( topo, edge, pt, skipISOChecks, &oldedge );
  if ( ! split_col ) return -1; /* should have raised an exception */
  oldedge_geom = split_col->geoms[0];
  newedge_geom = split_col->geoms[1];
  /* Make sure the SRID is set on the subgeom */
  ((RTGEOM*)oldedge_geom)->srid = split_col->srid;
  ((RTGEOM*)newedge_geom)->srid = split_col->srid;

  /* Add new node, getting new id back */
  node.node_id = -1;
  node.containing_face = -1; /* means not-isolated */
  node.geom = pt;
  if ( ! rtt_be_insertNodes(topo, &node, 1) )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if (node.node_id == -1) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    /* should have been set by backend */
    rterror(iface->ctx, "Backend coding error: "
            "insertNodes callback did not return node_id");
    return -1;
  }

  /* Delete the old edge */
  seledge.edge_id = edge;
  ret = rtt_be_deleteEdges(topo, &seledge, RTT_COL_EDGE_EDGE_ID);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Get new edges identifiers */
  newedges[0].edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedges[0].edge_id == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  newedges[1].edge_id = rtt_be_getNextEdgeId(topo);
  if ( newedges[1].edge_id == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Define the first new edge (to new node) */
  newedges[0].start_node = oldedge->start_node;
  newedges[0].end_node = node.node_id;
  newedges[0].face_left = oldedge->face_left;
  newedges[0].face_right = oldedge->face_right;
  newedges[0].next_left = newedges[1].edge_id;
  if ( oldedge->next_right == edge )
    newedges[0].next_right = newedges[0].edge_id;
  else if ( oldedge->next_right == -edge )
    newedges[0].next_right = -newedges[1].edge_id;
  else
    newedges[0].next_right = oldedge->next_right;
  newedges[0].geom = rtgeom_as_rtline(iface->ctx, oldedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedges[0].geom ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "first geometry in rtgeom_split output is not a line");
    return -1;
  }

  /* Define the second new edge (from new node) */
  newedges[1].start_node = node.node_id;
  newedges[1].end_node = oldedge->end_node;
  newedges[1].face_left = oldedge->face_left;
  newedges[1].face_right = oldedge->face_right;
  newedges[1].next_right = -newedges[0].edge_id;
  if ( oldedge->next_left == -edge )
    newedges[1].next_left = -newedges[1].edge_id;
  else if ( oldedge->next_left == edge )
    newedges[1].next_left = newedges[0].edge_id;
  else
    newedges[1].next_left = oldedge->next_left;
  newedges[1].geom = rtgeom_as_rtline(iface->ctx, newedge_geom);
  /* rtgeom_split of a line should only return lines ... */
  if ( ! newedges[1].geom ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "second geometry in rtgeom_split output is not a line");
    return -1;
  }

  /* Insert both new edges */
  ret = rtt_be_insertEdges(topo, newedges, 2);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Insertion of split edge failed (no reason)");
    return -1;
  }

  /* Update all next edge references pointing to old edge id */

  updedge.next_right = newedges[1].edge_id;
  seledge.next_right = edge;
  seledge.start_node = oldedge->start_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_right = -newedges[0].edge_id;
  seledge.next_right = -edge;
  seledge.start_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_RIGHT|RTT_COL_EDGE_START_NODE,
      &updedge, RTT_COL_EDGE_NEXT_RIGHT,
      NULL, 0);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = newedges[0].edge_id;
  seledge.next_left = edge;
  seledge.end_node = oldedge->start_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  updedge.next_left = -newedges[1].edge_id;
  seledge.next_left = -edge;
  seledge.end_node = oldedge->end_node;
  ret = rtt_be_updateEdges(topo,
      &seledge, RTT_COL_EDGE_NEXT_LEFT|RTT_COL_EDGE_END_NODE,
      &updedge, RTT_COL_EDGE_NEXT_LEFT,
      NULL, 0);
  if ( ret == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_release(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Update TopoGeometries composition */
  ret = rtt_be_updateTopoGeomEdgeSplit(topo, oldedge->edge_id, newedges[0].edge_id, newedges[1].edge_id);
  if ( ! ret ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rtcollection_free(iface->ctx, split_col);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  rtt_release_edges(iface->ctx, oldedge, 1);
  rtcollection_free(iface->ctx, split_col);

  /* return new node id */
  return node.node_id;
}

/* Data structure used by AddEdgeX functions */
typedef struct edgeend_t {
  /* Signed identifier of next clockwise edge (+outgoing,-incoming) */
  RTT_ELEMID nextCW;
  /* Identifier of face between myaz and next CW edge */
  RTT_ELEMID cwFace;
  /* Signed identifier of next counterclockwise edge (+outgoing,-incoming) */
  RTT_ELEMID nextCCW;
  /* Identifier of face between myaz and next CCW edge */
  RTT_ELEMID ccwFace;
  int was_isolated;
  double myaz; /* azimuth of edgeend geometry */
} edgeend;

/*
 * Get first distinct vertex from endpoint
 * @param pa the pointarray to seek points in
 * @param ref the point we want to search a distinct one
 * @param from vertex index to start from
 * @param dir  1 to go forward
 *            -1 to go backward
 * @return 0 if edge is collapsed (no distinct points)
 */
static int
_rtt_FirstDistinctVertex2D(const RTCTX *ctx, const RTPOINTARRAY* pa, RTPOINT2D *ref, int from, int dir, RTPOINT2D *op)
{
  int i, toofar, inc;
  RTPOINT2D fp;

  if ( dir > 0 )
  {
    toofar = pa->npoints;
    inc = 1;
  }
  else
  {
    toofar = -1;
    inc = -1;
  }

  RTDEBUGF(ctx, 1, "first point is index %d", from);
  fp = *ref; /* rt_getPoint2d_p(ctx, pa, from, &fp); */
  for ( i = from+inc; i != toofar; i += inc )
  {
    RTDEBUGF(ctx, 1, "testing point %d", i);
    rt_getPoint2d_p(ctx, pa, i, op); /* pick next point */
    if ( p2d_same(ctx, op, &fp) ) continue; /* equal to startpoint */
    /* this is a good one, neither same of start nor of end point */
    return 1; /* found */
  }

  /* no distinct vertices found */
  return 0;
}


/*
 * Return non-zero on failure (rterror is invoked in that case)
 * Possible failures:
 *  -1 no two distinct vertices exist
 *  -2 azimuth computation failed for first edge end
 */
static int
_rtt_InitEdgeEndByLine(const RTCTX *ctx, edgeend *fee, edgeend *lee, RTLINE *edge,
                                            RTPOINT2D *fp, RTPOINT2D *lp)
{
  RTPOINTARRAY *pa = edge->points;
  RTPOINT2D pt;

  fee->nextCW = fee->nextCCW =
  lee->nextCW = lee->nextCCW = 0;
  fee->cwFace = fee->ccwFace =
  lee->cwFace = lee->ccwFace = -1;

  /* Compute azimuth of first edge end */
  RTDEBUG(ctx, 1, "computing azimuth of first edge end");
  if ( ! _rtt_FirstDistinctVertex2D(ctx, pa, fp, 0, 1, &pt) )
  {
    rterror(ctx, "Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(ctx, fp, &pt, &(fee->myaz)) ) {
    rterror(ctx, "error computing azimuth of first edgeend"
            " [%.15g %.15g,%.15g %.15g]",
            fp->x, fp->y, pt.x, pt.y);
    return -2;
  }
  RTDEBUGF(ctx, 1, "azimuth of first edge end"
            " [%.15g %.15g,%.15g %.15g] is %.15g",
            fp->x, fp->y, pt.x, pt.y, fee->myaz);

  /* Compute azimuth of second edge end */
  RTDEBUG(ctx, 1, "computing azimuth of second edge end");
  if ( ! _rtt_FirstDistinctVertex2D(ctx, pa, lp, pa->npoints-1, -1, &pt) )
  {
    rterror(ctx, "Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(ctx, lp, &pt, &(lee->myaz)) ) {
    rterror(ctx, "error computing azimuth of last segment"
            " [%.15g %.15g,%.15g %.15g]",
            lp->x, lp->y, pt.x, pt.y);
    return -2;
  }
  RTDEBUGF(ctx, 1, "azimuth of last edge end"
            " [%.15g %.15g,%.15g %.15g] is %.15g",
            lp->x, lp->y, pt.x, pt.y, lee->myaz);

  return 0;
}

/*
 * Find the first edges encountered going clockwise and counterclockwise
 * around a node, starting from the given azimuth, and take
 * note of the face on the both sides.
 *
 * @param topo the topology to act upon
 * @param node the identifier of the node to analyze
 * @param data input (myaz) / output (nextCW, nextCCW) parameter
 * @param other edgeend, if also incident to given node (closed edge).
 * @param myedge_id identifier of the edge id that data->myaz belongs to
 * @return number of incident edges found
 *
 */
static int
_rtt_FindAdjacentEdges( RTT_TOPOLOGY* topo, RTT_ELEMID node, edgeend *data,
                        edgeend *other, int myedge_id )
{
  RTT_ISO_EDGE *edges;
  int numedges = 1;
  int i;
  double minaz, maxaz;
  double az, azdif;
  const RTT_BE_IFACE *iface = topo->be_iface;

  data->nextCW = data->nextCCW = 0;
  data->cwFace = data->ccwFace = -1;

  if ( other ) {
    azdif = other->myaz - data->myaz;
    if ( azdif < 0 ) azdif += 2 * M_PI;
    minaz = maxaz = azdif;
    /* TODO: set nextCW/nextCCW/cwFace/ccwFace to other->something ? */
    RTDEBUGF(iface->ctx, 1, "Other edge end has cwFace=%d and ccwFace=%d",
                other->cwFace, other->ccwFace);
  } else {
    minaz = maxaz = -1;
  }

  RTDEBUGF(iface->ctx, 1, "Looking for edges incident to node %" RTTFMT_ELEMID
              " and adjacent to azimuth %g", node, data->myaz);

  /* Get incident edges */
  edges = rtt_be_getEdgeByNode( topo, &node, &numedges, RTT_COL_EDGE_ALL );
  if ( numedges == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }

  RTDEBUGF(iface->ctx, 1, "getEdgeByNode returned %d edges, minaz=%g, maxaz=%g",
              numedges, minaz, maxaz);

  /* For each incident edge-end (1 or 2): */
  for ( i = 0; i < numedges; ++i )
  {
    RTT_ISO_EDGE *edge;
    RTGEOM *g;
    RTGEOM *cleangeom;
    RTPOINT2D p1, p2;
    RTPOINTARRAY *pa;

    edge = &(edges[i]);

    if ( edge->edge_id == myedge_id ) continue;

    g = rtline_as_rtgeom(iface->ctx, edge->geom);
    /* NOTE: remove_repeated_points call could be replaced by
     * some other mean to pick two distinct points for endpoints */
    cleangeom = rtgeom_remove_repeated_points(iface->ctx,  g, 0 );
    pa = rtgeom_as_rtline(iface->ctx, cleangeom)->points;

    if ( pa->npoints < 2 ) {{
      RTT_ELEMID id = edge->edge_id;
      rtt_release_edges(iface->ctx, edges, numedges);
      rtgeom_free(iface->ctx, cleangeom);
      rterror(iface->ctx, "corrupted topology: edge %" RTTFMT_ELEMID
              " does not have two distinct points", id);
      return -1;
    }}

    if ( edge->start_node == node ) {
      rt_getPoint2d_p(iface->ctx, pa, 0, &p1);
      rt_getPoint2d_p(iface->ctx, pa, 1, &p2);
      RTDEBUGF(iface->ctx, 1, "edge %" RTTFMT_ELEMID
                  " starts on node %" RTTFMT_ELEMID
                  ", edgeend is %.15g %.15g,%.15g %.15g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(iface->ctx, &p1, &p2, &az) ) {{
        RTT_ELEMID id = edge->edge_id;
        rtt_release_edges(iface->ctx, edges, numedges);
        rtgeom_free(iface->ctx, cleangeom);
        rterror(iface->ctx, "error computing azimuth of edge %d first segment"
                " [%.15g %.15g,%.15g,%.15g]",
                id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      RTDEBUGF(iface->ctx, 1, "azimuth of edge %" RTTFMT_ELEMID
                  ": %g (diff: %g)", edge->edge_id, az, azdif);

      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = edge->edge_id; /* outgoing */
        data->cwFace = edge->face_left;
        data->ccwFace = edge->face_right;
        RTDEBUGF(iface->ctx, 1, "new nextCW and nextCCW edge is %" RTTFMT_ELEMID
                    ", outgoing, "
                    "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                    " (face_right is new ccwFace, face_left is new cwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = edge->edge_id; /* outgoing */
          data->cwFace = edge->face_left;
          RTDEBUGF(iface->ctx, 1, "new nextCW edge is %" RTTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had minaz=%g, face_left is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = edge->edge_id; /* outgoing */
          data->ccwFace = edge->face_right;
          RTDEBUGF(iface->ctx, 1, "new nextCCW edge is %" RTTFMT_ELEMID
                      ", outgoing, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had maxaz=%g, face_right is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    if ( edge->end_node == node ) {
      rt_getPoint2d_p(iface->ctx, pa, pa->npoints-1, &p1);
      rt_getPoint2d_p(iface->ctx, pa, pa->npoints-2, &p2);
      RTDEBUGF(iface->ctx, 1, "edge %" RTTFMT_ELEMID
                  " ends on node %" RTTFMT_ELEMID
                  ", edgeend is %.15g %.15g,%.15g %.15g",
                  edge->edge_id, node, p1.x, p1.y, p2.x, p2.y);
      if ( ! azimuth_pt_pt(iface->ctx, &p1, &p2, &az) ) {{
        RTT_ELEMID id = edge->edge_id;
        rtt_release_edges(iface->ctx, edges, numedges);
        rtgeom_free(iface->ctx, cleangeom);
        rterror(iface->ctx, "error computing azimuth of edge %d last segment"
                " [%.15g %.15g,%.15g %.15g]",
                id, p1.x, p1.y, p2.x, p2.y);
        return -1;
      }}
      azdif = az - data->myaz;
      RTDEBUGF(iface->ctx, 1, "azimuth of edge %" RTTFMT_ELEMID
                  ": %g (diff: %g)", edge->edge_id, az, azdif);
      if ( azdif < 0 ) azdif += 2 * M_PI;
      if ( minaz == -1 ) {
        minaz = maxaz = azdif;
        data->nextCW = data->nextCCW = -edge->edge_id; /* incoming */
        data->cwFace = edge->face_right;
        data->ccwFace = edge->face_left;
        RTDEBUGF(iface->ctx, 1, "new nextCW and nextCCW edge is %" RTTFMT_ELEMID
                    ", incoming, "
                    "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                    " (face_right is new cwFace, face_left is new ccwFace)",
                    edge->edge_id, edge->face_left,
                    edge->face_right);
      } else {
        if ( azdif < minaz ) {
          data->nextCW = -edge->edge_id; /* incoming */
          data->cwFace = edge->face_right;
          RTDEBUGF(iface->ctx, 1, "new nextCW edge is %" RTTFMT_ELEMID
                      ", incoming, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had minaz=%g, face_right is new cwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, minaz);
          minaz = azdif;
        }
        else if ( azdif > maxaz ) {
          data->nextCCW = -edge->edge_id; /* incoming */
          data->ccwFace = edge->face_left;
          RTDEBUGF(iface->ctx, 1, "new nextCCW edge is %" RTTFMT_ELEMID
                      ", outgoing, from start point, "
                      "with face_left %" RTTFMT_ELEMID " and face_right %" RTTFMT_ELEMID
                      " (previous had maxaz=%g, face_left is new ccwFace)",
                      edge->edge_id, edge->face_left,
                      edge->face_right, maxaz);
          maxaz = azdif;
        }
      }
    }

    rtgeom_free(iface->ctx, cleangeom);
  }
  if ( numedges ) rtt_release_edges(iface->ctx, edges, numedges);

  RTDEBUGF(iface->ctx, 1, "edges adjacent to azimuth %g"
              " (incident to node %" RTTFMT_ELEMID ")"
              ": CW:%" RTTFMT_ELEMID "(%g) CCW:%" RTTFMT_ELEMID "(%g)",
              data->myaz, node, data->nextCW, minaz,
              data->nextCCW, maxaz);

  if ( myedge_id < 1 && numedges && data->cwFace != data->ccwFace )
  {
    if ( data->cwFace != -1 && data->ccwFace != -1 ) {
      rterror(iface->ctx, "Corrupted topology: adjacent edges %" RTTFMT_ELEMID " and %" RTTFMT_ELEMID
              " bind different face (%" RTTFMT_ELEMID " and %" RTTFMT_ELEMID ")",
              data->nextCW, data->nextCCW,
              data->cwFace, data->ccwFace);
      return -1;
    }
  }

  /* Return number of incident edges found */
  return numedges;
}

/*
 * Get a point internal to the line and write it into the "ip"
 * parameter
 *
 * return 0 on failure (line is empty or collapsed), 1 otherwise
 */
static int
_rtt_GetInteriorEdgePoint(const RTCTX *ctx, const RTLINE* edge, RTPOINT2D* ip)
{
  int i;
  RTPOINT2D fp, lp, tp;
  RTPOINTARRAY *pa = edge->points;

  if ( pa->npoints < 2 ) return 0; /* empty or structurally collapsed */

  rt_getPoint2d_p(ctx, pa, 0, &fp); /* save first point */
  rt_getPoint2d_p(ctx, pa, pa->npoints-1, &lp); /* save last point */
  for (i=1; i<pa->npoints-1; ++i)
  {
    rt_getPoint2d_p(ctx, pa, i, &tp); /* pick next point */
    if ( p2d_same(ctx, &tp, &fp) ) continue; /* equal to startpoint */
    if ( p2d_same(ctx, &tp, &lp) ) continue; /* equal to endpoint */
    /* this is a good one, neither same of start nor of end point */
    *ip = tp;
    return 1; /* found */
  }

  /* no distinct vertex found */

  /* interpolate if start point != end point */

  if ( p2d_same(ctx, &fp, &lp) ) return 0; /* no distinct points in edge */

  ip->x = fp.x + ( (lp.x - fp.x) * 0.5 );
  ip->y = fp.y + ( (lp.y - fp.y) * 0.5 );

  return 1;
}

/*
 * Add a split face by walking on the edge side.
 *
 * @param topo the topology to act upon
 * @param sedge edge id and walking side and direction
 *              (forward,left:positive backward,right:negative)
 * @param face the face in which the edge identifier is known to be
 * @param mbr_only do not create a new face but update MBR of the current
 *
 * @return:
 *    -1: if mbr_only was requested
 *     0: if the edge does not form a ring
 *    -1: if it is impossible to create a face on the requested side
 *        ( new face on the side is the universe )
 *    -2: error
 *   >0 : id of newly added face
 */
static RTT_ELEMID
_rtt_AddFaceSplit( RTT_TOPOLOGY* topo,
                   RTT_ELEMID sedge, RTT_ELEMID face,
                   int mbr_only )
{
  int numedges, numfaceedges, i, j;
  int newface_outside;
  int num_signed_edge_ids;
  RTT_ELEMID *signed_edge_ids;
  RTT_ELEMID *edge_ids;
  RTT_ISO_EDGE *edges;
  RTT_ISO_EDGE *ring_edges;
  RTT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  RTT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;
  const RTT_BE_IFACE *iface = topo->be_iface;

  signed_edge_ids = rtt_be_getRingEdges(topo, sedge,
                                        &num_signed_edge_ids, 0);
  if ( ! signed_edge_ids ) {
    rterror(iface->ctx, "Backend error (no ring edges for edge %" RTTFMT_ELEMID "): %s",
            sedge, rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  RTDEBUGF(iface->ctx, 1, "getRingEdges returned %d edges", num_signed_edge_ids);

  /* You can't get to the other side of an edge forming a ring */
  for (i=0; i<num_signed_edge_ids; ++i) {
    if ( signed_edge_ids[i] == -sedge ) {
      /* No split here */
      RTDEBUG(iface->ctx, 1, "not a ring");
      rtfree(iface->ctx,  signed_edge_ids );
      return 0;
    }
  }

  RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID " split face %" RTTFMT_ELEMID " (mbr_only:%d)",
           sedge, face, mbr_only);

  /* Construct a polygon using edges of the ring */
  numedges = 0;
  edge_ids = rtalloc(iface->ctx, sizeof(RTT_ELEMID)*num_signed_edge_ids);
  for (i=0; i<num_signed_edge_ids; ++i) {
    int absid = llabs(signed_edge_ids[i]);
    int found = 0;
    /* Do not add the same edge twice */
    for (j=0; j<numedges; ++j) {
      if ( edge_ids[j] == absid ) {
        found = 1;
        break;
      }
    }
    if ( ! found ) edge_ids[numedges++] = absid;
  }
  i = numedges;
  ring_edges = rtt_be_getEdgeById(topo, edge_ids, &i,
                                  RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM);
  rtfree(iface->ctx,  edge_ids );
  if ( i == -1 )
  {
    rtfree(iface->ctx,  signed_edge_ids );
    /* ring_edges should be NULL */
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  else if ( i != numedges )
  {
    rtfree(iface->ctx,  signed_edge_ids );
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rterror(iface->ctx, "Unexpected error: %d edges found when expecting %d", i, numedges);
    return -2;
  }

  /* Should now build a polygon with those edges, in the order
   * given by GetRingEdges.
   */
  RTPOINTARRAY *pa = NULL;
  for ( i=0; i<num_signed_edge_ids; ++i )
  {
    RTT_ELEMID eid = signed_edge_ids[i];
    RTDEBUGF(iface->ctx, 1, "Edge %d in ring of edge %" RTTFMT_ELEMID " is edge %" RTTFMT_ELEMID,
                i, sedge, eid);
    RTT_ISO_EDGE *edge = NULL;
    RTPOINTARRAY *epa;
    for ( j=0; j<numedges; ++j )
    {
      if ( ring_edges[j].edge_id == llabs(eid) )
      {
        edge = &(ring_edges[j]);
        break;
      }
    }
    if ( edge == NULL )
    {
      rtfree(iface->ctx,  signed_edge_ids );
      rtt_release_edges(iface->ctx, ring_edges, numedges);
      rterror(iface->ctx, "missing edge that was found in ring edges loop");
      return -2;
    }

    if ( pa == NULL )
    {
      pa = ptarray_clone_deep(iface->ctx, edge->geom->points);
      if ( eid < 0 ) ptarray_reverse(iface->ctx, pa);
    }
    else
    {
      if ( eid < 0 )
      {
        epa = ptarray_clone_deep(iface->ctx, edge->geom->points);
        ptarray_reverse(iface->ctx, epa);
        ptarray_append_ptarray(iface->ctx, pa, epa, 0);
        ptarray_free(iface->ctx, epa);
      }
      else
      {
        /* avoid a clone here */
        ptarray_append_ptarray(iface->ctx, pa, edge->geom->points, 0);
      }
    }
  }
  RTPOINTARRAY **points = rtalloc(iface->ctx, sizeof(RTPOINTARRAY*));
  points[0] = pa;
  /* NOTE: the ring may very well have collapsed components,
   *       which would make it topologically invalid
   */
  RTPOLY* shell = rtpoly_construct(iface->ctx, 0, 0, 1, points);

  int isccw = ptarray_isccw(iface->ctx, pa);
  RTDEBUGF(iface->ctx, 1, "Ring of edge %" RTTFMT_ELEMID " is %sclockwise",
              sedge, isccw ? "counter" : "");
  const RTGBOX* shellbox = rtgeom_get_bbox(iface->ctx, rtpoly_as_rtgeom(iface->ctx, shell));

  if ( face == 0 )
  {
    /* Edge split the universe face */
    if ( ! isccw )
    {
      rtpoly_free(iface->ctx, shell);
      rtfree(iface->ctx,  signed_edge_ids );
      rtt_release_edges(iface->ctx, ring_edges, numedges);
      /* Face on the left side of this ring is the universe face.
       * Next call (for the other side) should create the split face
       */
      RTDEBUG(iface->ctx, 1, "The left face of this clockwise ring is the universe, "
                 "won't create a new face there");
      return -1;
    }
  }

  if ( mbr_only && face != 0 )
  {
    if ( isccw )
    {{
      RTT_ISO_FACE updface;
      updface.face_id = face;
      updface.mbr = (RTGBOX *)shellbox; /* const cast, we won't free it, later */
      int ret = rtt_be_updateFacesById( topo, &updface, 1 );
      if ( ret == -1 )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rtt_release_edges(iface->ctx, ring_edges, numedges);
        rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox above */
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != 1 )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rtt_release_edges(iface->ctx, ring_edges, numedges);
        rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox above */
        rterror(iface->ctx, "Unexpected error: %d faces found when expecting 1", ret);
        return -2;
      }
    }}
    rtfree(iface->ctx,  signed_edge_ids );
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox above */
    return -1; /* mbr only was requested */
  }

  RTT_ISO_FACE *oldface = NULL;
  RTT_ISO_FACE newface;
  newface.face_id = -1;
  if ( face != 0 && ! isccw)
  {{
    /* Face created an hole in an outer face */
    int nfaces = 1;
    oldface = rtt_be_getFaceById(topo, &face, &nfaces, RTT_COL_FACE_ALL);
    if ( nfaces == -1 )
    {
      rtfree(iface->ctx,  signed_edge_ids );
      rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox */
      rtt_release_edges(iface->ctx, ring_edges, numedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -2;
    }
    if ( nfaces != 1 )
    {
      rtfree(iface->ctx,  signed_edge_ids );
      rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox */
      rtt_release_edges(iface->ctx, ring_edges, numedges);
      rterror(iface->ctx, "Unexpected error: %d faces found when expecting 1", nfaces);
      return -2;
    }
    newface.mbr = oldface->mbr;
  }}
  else
  {
    newface.mbr = (RTGBOX *)shellbox; /* const cast, we won't free it, later */
  }

  /* Insert the new face */
  int ret = rtt_be_insertFaces( topo, &newface, 1 );
  if ( ret == -1 )
  {
    rtfree(iface->ctx,  signed_edge_ids );
    rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox */
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( ret != 1 )
  {
    rtfree(iface->ctx,  signed_edge_ids );
    rtpoly_free(iface->ctx, shell); /* NOTE: owns shellbox */
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rterror(iface->ctx, "Unexpected error: %d faces inserted when expecting 1", ret);
    return -2;
  }
  if ( oldface ) {
    newface.mbr = NULL; /* it is a reference to oldface mbr... */
    _rtt_release_faces(iface->ctx, oldface, 1);
  }

  /* Update side location of new face edges */

  /* We want the new face to be on the left, if possible */
  if ( face != 0 && ! isccw ) { /* ring is clockwise in a real face */
    /* face shrinked, must update all non-contained edges and nodes */
    RTDEBUG(iface->ctx, 1, "New face is on the outside of the ring, updating rings in former shell");
    newface_outside = 1;
    /* newface is outside */
  } else {
    RTDEBUG(iface->ctx, 1, "New face is on the inside of the ring, updating forward edges in new ring");
    newface_outside = 0;
    /* newface is inside */
  }

  /* Update edges bounding the old face */
  /* (1) fetch all edges where left_face or right_face is = oldface */
  int fields = RTT_COL_EDGE_EDGE_ID |
               RTT_COL_EDGE_FACE_LEFT |
               RTT_COL_EDGE_FACE_RIGHT |
               RTT_COL_EDGE_GEOM
               ;
  numfaceedges = 1;
  edges = rtt_be_getEdgeByFace( topo, &face, &numfaceedges, fields, newface.mbr );
  if ( numfaceedges == -1 ) {
    rtfree(iface->ctx,  signed_edge_ids );
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  RTDEBUGF(iface->ctx, 1, "rtt_be_getEdgeByFace returned %d edges", numfaceedges);
  GEOSGeometry *shellgg = 0;
  const GEOSPreparedGeometry* prepshell = 0;
  shellgg = RTGEOM2GEOS(iface->ctx,  rtpoly_as_rtgeom(iface->ctx, shell), 0);
  if ( ! shellgg ) {
    rtpoly_free(iface->ctx, shell);
    rtfree(iface->ctx, signed_edge_ids);
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rtt_release_edges(iface->ctx, edges, numfaceedges);
    rterror(iface->ctx, "Could not convert shell geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
    return -2;
  }
  prepshell = GEOSPrepare_r(iface->ctx->gctx,  shellgg );
  if ( ! prepshell ) {
    GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
    rtpoly_free(iface->ctx, shell);
    rtfree(iface->ctx, signed_edge_ids);
    rtt_release_edges(iface->ctx, ring_edges, numedges);
    rtt_release_edges(iface->ctx, edges, numfaceedges);
    rterror(iface->ctx, "Could not prepare shell geometry: %s", rtgeom_get_last_geos_error(iface->ctx));
    return -2;
  }

  if ( numfaceedges )
  {
    forward_edges = rtalloc(iface->ctx, sizeof(RTT_ISO_EDGE)*numfaceedges);
    forward_edges_count = 0;
    backward_edges = rtalloc(iface->ctx, sizeof(RTT_ISO_EDGE)*numfaceedges);
    backward_edges_count = 0;

    /* (2) loop over the results and: */
    for ( i=0; i<numfaceedges; ++i )
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      int found = 0;
      int contains;
      GEOSGeometry *egg;
      RTPOINT *epgeom;
      RTPOINT2D ep;

      /* (2.1) skip edges whose ID is in the list of boundary edges ? */
      for ( j=0; j<num_signed_edge_ids; ++j )
      {
        int seid = signed_edge_ids[j];
        if ( seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          RTDEBUGF(iface->ctx, 1, "Edge %d is a forward edge of the new ring", e->edge_id);
          forward_edges[forward_edges_count].edge_id = e->edge_id;
          forward_edges[forward_edges_count++].face_left = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
        else if ( -seid == e->edge_id )
        {
          /* IDEA: remove entry from signed_edge_ids ? */
          RTDEBUGF(iface->ctx, 1, "Edge %d is a backward edge of the new ring", e->edge_id);
          backward_edges[backward_edges_count].edge_id = e->edge_id;
          backward_edges[backward_edges_count++].face_right = newface.face_id;
          found++;
          if ( found == 2 ) break;
        }
      }
      if ( found ) continue;

      /* We need to check only a single point
       * (to avoid collapsed elements of the shell polygon
       * giving false positive).
       * The point but must not be an endpoint.
       */
      if ( ! _rtt_GetInteriorEdgePoint(iface->ctx, e->geom, &ep) )
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
        GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
        rtfree(iface->ctx, signed_edge_ids);
        rtpoly_free(iface->ctx, shell);
        rtfree(iface->ctx, forward_edges); /* contents owned by ring_edges */
        rtfree(iface->ctx, backward_edges); /* contents owned by ring_edges */
        rtt_release_edges(iface->ctx, ring_edges, numedges);
        rtt_release_edges(iface->ctx, edges, numfaceedges);
        rterror(iface->ctx, "Could not find interior point for edge %d: %s",
                e->edge_id, rtgeom_get_last_geos_error(iface->ctx));
        return -2;
      }

      epgeom = rtpoint_make2d(iface->ctx, 0, ep.x, ep.y);
      egg = RTGEOM2GEOS(iface->ctx,  rtpoint_as_rtgeom(iface->ctx, epgeom) , 0);
      rtpoint_free(iface->ctx, epgeom);
      if ( ! egg ) {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
        GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
        rtfree(iface->ctx, signed_edge_ids);
        rtpoly_free(iface->ctx, shell);
        rtfree(iface->ctx, forward_edges); /* contents owned by ring_edges */
        rtfree(iface->ctx, backward_edges); /* contents owned by ring_edges */
        rtt_release_edges(iface->ctx, ring_edges, numedges);
        rtt_release_edges(iface->ctx, edges, numfaceedges);
        rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s",
                rtgeom_get_last_geos_error(iface->ctx));
        return -2;
      }
      /* IDEA: can be optimized by computing this on our side rather
       *       than on GEOS _r(iface->ctx->gctx, saves conversion of big edges) */
      /* IDEA: check that bounding box shortcut is taken, or use
       *       shellbox to do it here */
      contains = GEOSPreparedContains_r(iface->ctx->gctx,  prepshell, egg );
      GEOSGeom_destroy_r(iface->ctx->gctx, egg);
      if ( contains == 2 )
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
        GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
        rtfree(iface->ctx, signed_edge_ids);
        rtpoly_free(iface->ctx, shell);
        rtfree(iface->ctx, forward_edges); /* contents owned by ring_edges */
        rtfree(iface->ctx, backward_edges); /* contents owned by ring_edges */
        rtt_release_edges(iface->ctx, ring_edges, numedges);
        rtt_release_edges(iface->ctx, edges, numfaceedges);
        rterror(iface->ctx, "GEOS exception on PreparedContains: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -2;
      }
      RTDEBUGF(iface->ctx, 1, "Edge %d %scontained in new ring", e->edge_id,
                  (contains?"":"not "));

      /* (2.2) skip edges (NOT, if newface_outside) contained in shell */
      if ( newface_outside )
      {
        if ( contains )
        {
          RTDEBUGF(iface->ctx, 1, "Edge %d contained in an hole of the new face",
                      e->edge_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          RTDEBUGF(iface->ctx, 1, "Edge %d not contained in the face shell",
                      e->edge_id);
          continue;
        }
      }

      /* (2.3) push to forward_edges if left_face = oface */
      if ( e->face_left == face )
      {
        RTDEBUGF(iface->ctx, 1, "Edge %d has new face on the left side", e->edge_id);
        forward_edges[forward_edges_count].edge_id = e->edge_id;
        forward_edges[forward_edges_count++].face_left = newface.face_id;
      }

      /* (2.4) push to backward_edges if right_face = oface */
      if ( e->face_right == face )
      {
        RTDEBUGF(iface->ctx, 1, "Edge %d has new face on the right side", e->edge_id);
        backward_edges[backward_edges_count].edge_id = e->edge_id;
        backward_edges[backward_edges_count++].face_right = newface.face_id;
      }
    }

    /* Update forward edges */
    if ( forward_edges_count )
    {
      ret = rtt_be_updateEdgesById(topo, forward_edges,
                                   forward_edges_count,
                                   RTT_COL_EDGE_FACE_LEFT);
      if ( ret == -1 )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != forward_edges_count )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rterror(iface->ctx, "Unexpected error: %d edges updated when expecting %d",
                ret, forward_edges_count);
        return -2;
      }
    }

    /* Update backward edges */
    if ( backward_edges_count )
    {
      ret = rtt_be_updateEdgesById(topo, backward_edges,
                                   backward_edges_count,
                                   RTT_COL_EDGE_FACE_RIGHT);
      if ( ret == -1 )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
      if ( ret != backward_edges_count )
      {
        rtfree(iface->ctx,  signed_edge_ids );
        rterror(iface->ctx, "Unexpected error: %d edges updated when expecting %d",
                ret, backward_edges_count);
        return -2;
      }
    }

    rtfree(iface->ctx, forward_edges);
    rtfree(iface->ctx, backward_edges);

  }

  rtt_release_edges(iface->ctx, ring_edges, numedges);
  rtt_release_edges(iface->ctx, edges, numfaceedges);

  /* Update isolated nodes which are now in new face */
  int numisonodes = 1;
  fields = RTT_COL_NODE_NODE_ID | RTT_COL_NODE_GEOM;
  RTT_ISO_NODE *nodes = rtt_be_getNodeByFace(topo, &face,
                                             &numisonodes, fields, newface.mbr);
  if ( numisonodes == -1 ) {
    rtfree(iface->ctx,  signed_edge_ids );
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -2;
  }
  if ( numisonodes ) {
    RTT_ISO_NODE *updated_nodes = rtalloc(iface->ctx, sizeof(RTT_ISO_NODE)*numisonodes);
    int nodes_to_update = 0;
    for (i=0; i<numisonodes; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      GEOSGeometry *ngg;
      ngg = RTGEOM2GEOS(iface->ctx,  rtpoint_as_rtgeom(iface->ctx, n->geom), 0 );
      int contains;
      if ( ! ngg ) {
        _rtt_release_nodes(iface->ctx, nodes, numisonodes);
        if ( prepshell ) GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
        if ( shellgg ) GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
        rtfree(iface->ctx, signed_edge_ids);
        rtpoly_free(iface->ctx, shell);
        rterror(iface->ctx, "Could not convert node geometry to GEOS: %s",
                rtgeom_get_last_geos_error(iface->ctx));
        return -2;
      }
      contains = GEOSPreparedContains_r(iface->ctx->gctx,  prepshell, ngg );
      GEOSGeom_destroy_r(iface->ctx->gctx, ngg);
      if ( contains == 2 )
      {
        _rtt_release_nodes(iface->ctx, nodes, numisonodes);
        if ( prepshell ) GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
        if ( shellgg ) GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
        rtfree(iface->ctx, signed_edge_ids);
        rtpoly_free(iface->ctx, shell);
        rterror(iface->ctx, "GEOS exception on PreparedContains: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -2;
      }
      RTDEBUGF(iface->ctx, 1, "Node %d is %scontained in new ring, newface is %s",
                  n->node_id, contains ? "" : "not ",
                  newface_outside ? "outside" : "inside" );
      if ( newface_outside )
      {
        if ( contains )
        {
          RTDEBUGF(iface->ctx, 1, "Node %d contained in an hole of the new face",
                      n->node_id);
          continue;
        }
      }
      else
      {
        if ( ! contains )
        {
          RTDEBUGF(iface->ctx, 1, "Node %d not contained in the face shell",
                      n->node_id);
          continue;
        }
      }
      updated_nodes[nodes_to_update].node_id = n->node_id;
      updated_nodes[nodes_to_update++].containing_face =
                                       newface.face_id;
      RTDEBUGF(iface->ctx, 1, "Node %d will be updated", n->node_id);
    }
    _rtt_release_nodes(iface->ctx, nodes, numisonodes);
    if ( nodes_to_update )
    {
      int ret = rtt_be_updateNodesById(topo, updated_nodes,
                                       nodes_to_update,
                                       RTT_COL_NODE_CONTAINING_FACE);
      if ( ret == -1 ) {
        rtfree(iface->ctx,  signed_edge_ids );
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -2;
      }
    }
    rtfree(iface->ctx, updated_nodes);
  }

  GEOSPreparedGeom_destroy_r(iface->ctx->gctx, prepshell);
  GEOSGeom_destroy_r(iface->ctx->gctx, shellgg);
  rtfree(iface->ctx, signed_edge_ids);
  rtpoly_free(iface->ctx, shell);

  return newface.face_id;
}

/**
 * @param modFace can be
 *    0 - have two new faces replace a splitted face
 *    1 - modify a splitted face, adding a new one
 *   -1 - do not check at all for face splitting
 *
 */
static RTT_ELEMID
_rtt_AddEdge( RTT_TOPOLOGY* topo,
              RTT_ELEMID start_node, RTT_ELEMID end_node,
              RTLINE *geom, int skipChecks, int modFace )
{
  RTT_ISO_EDGE newedge;
  RTGEOM *cleangeom;
  edgeend span; /* start point analisys */
  edgeend epan; /* end point analisys */
  RTPOINT2D p1, pn, p2;
  RTPOINTARRAY *pa;
  RTT_ELEMID node_ids[2];
  const RTPOINT *start_node_geom = NULL;
  const RTPOINT *end_node_geom = NULL;
  int num_nodes;
  RTT_ISO_NODE *endpoints;
  int i;
  int prev_left;
  int prev_right;
  RTT_ISO_EDGE seledge;
  RTT_ISO_EDGE updedge;
  const RTT_BE_IFACE *iface = topo->be_iface;

  if ( ! skipChecks )
  {
    /* curve must be simple */
    if ( ! rtgeom_is_simple(iface->ctx, rtline_as_rtgeom(iface->ctx, geom)) )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - curve not simple");
      return -1;
    }
  }

  newedge.start_node = start_node;
  newedge.end_node = end_node;
  newedge.geom = geom;
  newedge.face_left = -1;
  newedge.face_right = -1;
  cleangeom = rtgeom_remove_repeated_points(iface->ctx,  rtline_as_rtgeom(iface->ctx, geom), 0 );

  pa = rtgeom_as_rtline(iface->ctx, cleangeom)->points;
  if ( pa->npoints < 2 ) {
    rtgeom_free(iface->ctx, cleangeom);
    rterror(iface->ctx, "Invalid edge (no two distinct vertices exist)");
    return -1;
  }

  /* Initialize endpoint info (some of that ) */
  span.cwFace = span.ccwFace =
  epan.cwFace = epan.ccwFace = -1;

  /* Compute azimut of first edge end on start node */
  rt_getPoint2d_p(iface->ctx, pa, 0, &p1);
  rt_getPoint2d_p(iface->ctx, pa, 1, &pn);
  if ( p2d_same(iface->ctx, &p1, &pn) ) {
    rtgeom_free(iface->ctx, cleangeom);
    /* Can still happen, for 2-point lines */
    rterror(iface->ctx, "Invalid edge (no two distinct vertices exist)");
    return -1;
  }
  if ( ! azimuth_pt_pt(iface->ctx, &p1, &pn, &span.myaz) ) {
    rtgeom_free(iface->ctx, cleangeom);
    rterror(iface->ctx, "error computing azimuth of first segment"
            " [%.15g %.15g,%.15g %.15g]",
            p1.x, p1.y, pn.x, pn.y);
    return -1;
  }
  RTDEBUGF(iface->ctx, 1, "edge's start node is %g,%g", p1.x, p1.y);

  /* Compute azimuth of last edge end on end node */
  rt_getPoint2d_p(iface->ctx, pa, pa->npoints-1, &p2);
  rt_getPoint2d_p(iface->ctx, pa, pa->npoints-2, &pn);
  rtgeom_free(iface->ctx, cleangeom);
  if ( ! azimuth_pt_pt(iface->ctx, &p2, &pn, &epan.myaz) ) {
    rterror(iface->ctx, "error computing azimuth of last segment"
            " [%.15g %.15g,%.15g %.15g]",
            p2.x, p2.y, pn.x, pn.y);
    return -1;
  }
  RTDEBUGF(iface->ctx, 1, "edge's end node is %g,%g", p2.x, p2.y);

  /*
   * Check endpoints existance, match with Curve geometry
   * and get face information (if any)
   */

  if ( start_node != end_node ) {
    num_nodes = 2;
    node_ids[0] = start_node;
    node_ids[1] = end_node;
  } else {
    num_nodes = 1;
    node_ids[0] = start_node;
  }

  endpoints = rtt_be_getNodeById( topo, node_ids, &num_nodes, RTT_COL_NODE_ALL );
  if ( num_nodes < 0 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<num_nodes; ++i )
  {
    RTT_ISO_NODE* node = &(endpoints[i]);
    if ( node->containing_face != -1 )
    {
      if ( newedge.face_left == -1 )
      {
        newedge.face_left = newedge.face_right = node->containing_face;
      }
      else if ( newedge.face_left != node->containing_face )
      {
        rterror(iface->ctx, "SQL/MM Spatial exception - geometry crosses an edge"
                " (endnodes in faces %" RTTFMT_ELEMID " and %" RTTFMT_ELEMID ")",
                newedge.face_left, node->containing_face);
        _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
        return -1;
      }
    }

    RTDEBUGF(iface->ctx, 1, "Node %d, with geom %p (looking for %d and %d)",
             node->node_id, node->geom, start_node, end_node);
    if ( node->node_id == start_node ) {
      start_node_geom = node->geom;
    }
    if ( node->node_id == end_node ) {
      end_node_geom = node->geom;
    }
  }

  if ( ! skipChecks )
  {
    if ( ! start_node_geom )
    {
      if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = start_node_geom->point;
      rt_getPoint2d_p(iface->ctx, pa, 0, &pn);
      if ( ! p2d_same(iface->ctx, &pn, &p1) )
      {
        if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
        rterror(iface->ctx, "SQL/MM Spatial exception"
                " - start node not geometry start point."
                //" - start node not geometry start point (%g,%g != %g,%g).", pn.x, pn.y, p1.x, p1.y
        );
        return -1;
      }
    }

    if ( ! end_node_geom )
    {
      if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent node");
      return -1;
    }
    else
    {
      pa = end_node_geom->point;
      rt_getPoint2d_p(iface->ctx, pa, 0, &pn);
      if ( ! p2d_same(iface->ctx, &pn, &p2) )
      {
        if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);
        rterror(iface->ctx, "SQL/MM Spatial exception"
                " - end node not geometry end point."
                //" - end node not geometry end point (%g,%g != %g,%g).", pn.x, pn.y, p2.x, p2.y
        );
        return -1;
      }
    }

    if ( num_nodes ) _rtt_release_nodes(iface->ctx, endpoints, num_nodes);

    if ( _rtt_CheckEdgeCrossing( topo, start_node, end_node, geom, 0 ) )
      return -1;

  } /* ! skipChecks */

  /*
   * All checks passed, time to prepare the new edge
   */

  newedge.edge_id = rtt_be_getNextEdgeId( topo );
  if ( newedge.edge_id == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Find adjacent edges to each endpoint */
  int isclosed = start_node == end_node;
  int found;
  found = _rtt_FindAdjacentEdges( topo, start_node, &span,
                                  isclosed ? &epan : NULL, -1 );
  if ( found ) {
    span.was_isolated = 0;
    newedge.next_right = span.nextCW ? span.nextCW : -newedge.edge_id;
    prev_left = span.nextCCW ? -span.nextCCW : newedge.edge_id;
    RTDEBUGF(iface->ctx, 1, "New edge %d is connected on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.cwFace;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.ccwFace;
    }
  } else {
    span.was_isolated = 1;
    newedge.next_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    prev_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    RTDEBUGF(iface->ctx, 1, "New edge %d is isolated on start node, "
                "next_right is %d, prev_left is %d",
                newedge.edge_id, newedge.next_right, prev_left);
  }

  found = _rtt_FindAdjacentEdges( topo, end_node, &epan,
                                  isclosed ? &span : NULL, -1 );
  if ( found ) {
    epan.was_isolated = 0;
    newedge.next_left = epan.nextCW ? epan.nextCW : newedge.edge_id;
    prev_right = epan.nextCCW ? -epan.nextCCW : -newedge.edge_id;
    RTDEBUGF(iface->ctx, 1, "New edge %d is connected on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
    if ( newedge.face_right == -1 ) {
      newedge.face_right = span.ccwFace;
    } else if ( modFace != -1 && newedge.face_right != epan.ccwFace ) {
      /* side-location conflict */
      rterror(iface->ctx, "Side-location conflict: "
              "new edge starts in face"
               " %" RTTFMT_ELEMID " and ends in face"
               " %" RTTFMT_ELEMID,
              newedge.face_right, epan.ccwFace
      );
      return -1;
    }
    if ( newedge.face_left == -1 ) {
      newedge.face_left = span.cwFace;
    } else if ( modFace != -1 && newedge.face_left != epan.cwFace ) {
      /* side-location conflict */
      rterror(iface->ctx, "Side-location conflict: "
              "new edge starts in face"
               " %" RTTFMT_ELEMID " and ends in face"
               " %" RTTFMT_ELEMID,
              newedge.face_left, epan.cwFace
      );
      return -1;
    }
  } else {
    epan.was_isolated = 1;
    newedge.next_left = isclosed ? newedge.edge_id : -newedge.edge_id;
    prev_right = isclosed ? -newedge.edge_id : newedge.edge_id;
    RTDEBUGF(iface->ctx, 1, "New edge %d is isolated on end node, "
                "next_left is %d, prev_right is %d",
                newedge.edge_id, newedge.next_left, prev_right);
  }

  /*
   * If we don't have faces setup by now we must have encountered
   * a malformed topology (no containing_face on isolated nodes, no
   * left/right faces on adjacent edges or mismatching values)
   */
  if ( newedge.face_left != newedge.face_right )
  {
    rterror(iface->ctx, "Left(%" RTTFMT_ELEMID ")/right(%" RTTFMT_ELEMID ")"
            "faces mismatch: invalid topology ?",
            newedge.face_left, newedge.face_right);
    return -1;
  }
  else if ( newedge.face_left == -1 && modFace > -1 )
  {
    rterror(iface->ctx, "Could not derive edge face from linked primitives:"
            " invalid topology ?");
    return -1;
  }

  /*
   * Insert the new edge, and update all linking
   */

  int ret = rtt_be_insertEdges(topo, &newedge, 1);
  if ( ret == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  } else if ( ret == 0 ) {
    rterror(iface->ctx, "Insertion of split edge failed (no reason)");
    return -1;
  }

  int updfields;

  /* Link prev_left to us
   * (if it's not us already) */
  if ( llabs(prev_left) != newedge.edge_id )
  {
    if ( prev_left > 0 )
    {
      /* its next_left_edge is us */
      updfields = RTT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = newedge.edge_id;
      seledge.edge_id = prev_left;
    }
    else
    {
      /* its next_right_edge is us */
      updfields = RTT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = newedge.edge_id;
      seledge.edge_id = -prev_left;
    }

    ret = rtt_be_updateEdges(topo,
        &seledge, RTT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* Link prev_right to us
   * (if it's not us already) */
  if ( llabs(prev_right) != newedge.edge_id )
  {
    if ( prev_right > 0 )
    {
      /* its next_left_edge is -us */
      updfields = RTT_COL_EDGE_NEXT_LEFT;
      updedge.next_left = -newedge.edge_id;
      seledge.edge_id = prev_right;
    }
    else
    {
      /* its next_right_edge is -us */
      updfields = RTT_COL_EDGE_NEXT_RIGHT;
      updedge.next_right = -newedge.edge_id;
      seledge.edge_id = -prev_right;
    }

    ret = rtt_be_updateEdges(topo,
        &seledge, RTT_COL_EDGE_EDGE_ID,
        &updedge, updfields,
        NULL, 0);
    if ( ret == -1 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* NOT IN THE SPECS...
   * set containing_face = null for start_node and end_node
   * if they where isolated
   *
   */
  RTT_ISO_NODE updnode, selnode;
  updnode.containing_face = -1;
  if ( span.was_isolated )
  {
    selnode.node_id = start_node;
    ret = rtt_be_updateNodes(topo,
        &selnode, RTT_COL_NODE_NODE_ID,
        &updnode, RTT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  if ( epan.was_isolated )
  {
    selnode.node_id = end_node;
    ret = rtt_be_updateNodes(topo,
        &selnode, RTT_COL_NODE_NODE_ID,
        &updnode, RTT_COL_NODE_CONTAINING_FACE,
        NULL, 0);
    if ( ret == -1 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* Check face splitting, if required */

  if ( modFace > -1 ) {

  if ( ! isclosed && ( epan.was_isolated || span.was_isolated ) )
  {
    RTDEBUG(iface->ctx, 1, "New edge is dangling, so it cannot split any face");
    return newedge.edge_id; /* no split */
  }

  int newface1 = -1;

  /* IDEA: avoid building edge ring if input is closed, which means we
   *       know in advance it splits a face */

  if ( ! modFace )
  {
    newface1 = _rtt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 0 );
    if ( newface1 == 0 ) {
      RTDEBUG(iface->ctx, 1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }
  }

  int newface = _rtt_AddFaceSplit( topo, newedge.edge_id,
                                   newedge.face_left, 0 );
  if ( modFace )
  {
    if ( newface == 0 ) {
      RTDEBUG(iface->ctx, 1, "New edge does not split any face");
      return newedge.edge_id; /* no split */
    }

    if ( newface < 0 )
    {
      /* face on the left is the universe face */
      /* must be forming a maximal ring in universal face */
      newface = _rtt_AddFaceSplit( topo, -newedge.edge_id,
                                   newedge.face_left, 0 );
      if ( newface < 0 ) return newedge.edge_id; /* no split */
    }
    else
    {
      _rtt_AddFaceSplit( topo, -newedge.edge_id, newedge.face_left, 1 );
    }
  }

  /*
   * Update topogeometries, if needed
   */
  if ( newedge.face_left != 0 )
  {
    ret = rtt_be_updateTopoGeomFaceSplit(topo, newedge.face_left,
                                         newface, newface1);
    if ( ret == 0 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    if ( ! modFace )
    {
      /* drop old face from the face table */
      ret = rtt_be_deleteFacesById(topo, &(newedge.face_left), 1);
      if ( ret == -1 ) {
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }
  }

  } // end of face split checking

  return newedge.edge_id;
}

RTT_ELEMID
rtt_AddEdgeModFace( RTT_TOPOLOGY* topo,
                    RTT_ELEMID start_node, RTT_ELEMID end_node,
                    RTLINE *geom, int skipChecks )
{
  return _rtt_AddEdge( topo, start_node, end_node, geom, skipChecks, 1 );
}

RTT_ELEMID
rtt_AddEdgeNewFaces( RTT_TOPOLOGY* topo,
                    RTT_ELEMID start_node, RTT_ELEMID end_node,
                    RTLINE *geom, int skipChecks )
{
  return _rtt_AddEdge( topo, start_node, end_node, geom, skipChecks, 0 );
}

static RTGEOM *
_rtt_FaceByEdges(RTT_TOPOLOGY *topo, RTT_ISO_EDGE *edges, int numfaceedges)
{
  RTGEOM *outg;
  RTCOLLECTION *bounds;
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTGEOM **geoms = rtalloc(iface->ctx,  sizeof(RTGEOM*) * numfaceedges );
  int i, validedges = 0;

  for ( i=0; i<numfaceedges; ++i )
  {
    /* NOTE: skipping edges with same face on both sides, although
     *       correct, results in a failure to build faces from
     *       invalid topologies as expected by legacy tests.
     * TODO: update legacy tests expectances/unleash this skipping ?
     */
    /* if ( edges[i].face_left == edges[i].face_right ) continue; */
    geoms[validedges++] = rtline_as_rtgeom(iface->ctx, edges[i].geom);
  }
  if ( ! validedges )
  {
    /* Face has no valid boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    if ( numfaceedges ) rtfree(iface->ctx, geoms);
    RTDEBUG(iface->ctx, 1, "_rtt_FaceByEdges returning empty polygon");
    return rtpoly_as_rtgeom(iface->ctx,
            rtpoly_construct_empty(iface->ctx, topo->srid, topo->hasZ, 0)
           );
  }
  bounds = rtcollection_construct(iface->ctx, RTMULTILINETYPE,
                                  topo->srid,
                                  NULL, /* gbox */
                                  validedges,
                                  geoms);
  outg = rtgeom_buildarea(iface->ctx,  rtcollection_as_rtgeom(iface->ctx, bounds) );
  rtcollection_release(iface->ctx, bounds);
  rtfree(iface->ctx, geoms);
#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt(iface->ctx, outg, RTWKT_EXTENDED, 2, &sz);
  RTDEBUGF(iface->ctx, 1, "_rtt_FaceByEdges returning area: %s", wkt);
  rtfree(iface->ctx, wkt);
  }
#endif
  return outg;
}

RTGEOM*
rtt_GetFaceGeometry(RTT_TOPOLOGY* topo, RTT_ELEMID faceid)
{
  int numfaceedges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_FACE *face;
  RTPOLY *out;
  RTGEOM *outg;
  int i;
  int fields;
  const RTT_BE_IFACE *iface = topo->be_iface;

  if ( faceid == 0 )
  {
    rterror(iface->ctx, "SQL/MM Spatial exception - universal face has no geometry");
    return NULL;
  }

  /* Construct the face geometry */
  numfaceedges = 1;
  fields = RTT_COL_EDGE_GEOM |
           RTT_COL_EDGE_FACE_LEFT |
           RTT_COL_EDGE_FACE_RIGHT
           ;
  edges = rtt_be_getEdgeByFace( topo, &faceid, &numfaceedges, fields, NULL );
  if ( numfaceedges == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }

  if ( numfaceedges == 0 )
  {
    i = 1;
    face = rtt_be_getFaceById(topo, &faceid, &i, RTT_COL_FACE_FACE_ID);
    if ( i == -1 ) {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return NULL;
    }
    if ( i == 0 ) {
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent face.");
      return NULL;
    }
    rtfree(iface->ctx,  face );
    if ( i > 1 ) {
      rterror(iface->ctx, "Corrupted topology: multiple face records have face_id=%"
              RTTFMT_ELEMID, faceid);
      return NULL;
    }
    /* Face has no boundary edges, we'll return EMPTY, see
     * https://trac.osgeo.org/postgis/ticket/3221 */
    out = rtpoly_construct_empty(iface->ctx, topo->srid, topo->hasZ, 0);
    return rtpoly_as_rtgeom(iface->ctx, out);
  }

  outg = _rtt_FaceByEdges( topo, edges, numfaceedges );
  rtt_release_edges(iface->ctx, edges, numfaceedges);

  return outg;
}

/* Find which edge from the "edges" set defines the next
 * portion of the given "ring".
 *
 * The edge might be either forward or backward.
 *
 * @param ring The ring to find definition of.
 *             It is assumed it does not contain duplicated vertices.
 * @param from offset of the ring point to start looking from
 * @param edges array of edges to search into
 * @param numedges number of edges in the edges array
 *
 * @return index of the edge defining the next ring portion or
 *               -1 if no edge was found to be part of the ring
 */
static int
_rtt_FindNextRingEdge(const RTCTX *ctx, const RTPOINTARRAY *ring, int from,
                      const RTT_ISO_EDGE *edges, int numedges)
{
  int i;
  RTPOINT2D p1;

  /* Get starting ring point */
  rt_getPoint2d_p(ctx, ring, from, &p1);

  RTDEBUGF(ctx, 1, "Ring's 'from' point (%d) is %g,%g", from, p1.x, p1.y);

  /* find the edges defining the next portion of ring starting from
   * vertex "from" */
  for ( i=0; i<numedges; ++i )
  {
    const RTT_ISO_EDGE *isoe = &(edges[i]);
    RTLINE *edge = isoe->geom;
    RTPOINTARRAY *epa = edge->points;
    RTPOINT2D p2, pt;
    int match = 0;
    int j;

    /* Skip if the edge is a dangling one */
    if ( isoe->face_left == isoe->face_right )
    {
      RTDEBUGF(ctx, 3, "_rtt_FindNextRingEdge: edge %" RTTFMT_ELEMID
                  " has same face (%" RTTFMT_ELEMID
                  ") on both sides, skipping",
                  isoe->edge_id, isoe->face_left);
      continue;
    }

#if 0
    size_t sz;
    RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " is %s",
                isoe->edge_id,
                rtgeom_to_wkt(ctx, rtline_as_rtgeom(ctx, edge), RTWKT_EXTENDED, 2, &sz));
#endif

    /* ptarray_remove_repeated_points ? */

    rt_getPoint2d_p(ctx, epa, 0, &p2);
    RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " 'first' point is %g,%g",
                isoe->edge_id, p2.x, p2.y);
    RTDEBUGF(ctx, 1, "Rings's 'from' point is still %g,%g", p1.x, p1.y);
    if ( p2d_same(ctx, &p1, &p2) )
    {
      RTDEBUG(ctx, 1, "p2d_same(ctx, p1,p2) returned true");
      RTDEBUGF(ctx, 1, "First point of edge %" RTTFMT_ELEMID
                  " matches ring vertex %d", isoe->edge_id, from);
      /* first point matches, let's check next non-equal one */
      for ( j=1; j<epa->npoints; ++j )
      {
        rt_getPoint2d_p(ctx, epa, j, &p2);
        RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " 'next' point %d is %g,%g",
                    isoe->edge_id, j, p2.x, p2.y);
        /* we won't check duplicated edge points */
        if ( p2d_same(ctx, &p1, &p2) ) continue;
        /* we assume there are no duplicated points in ring */
        rt_getPoint2d_p(ctx, ring, from+1, &pt);
        RTDEBUGF(ctx, 1, "Ring's point %d is %g,%g",
                    from+1, pt.x, pt.y);
        match = p2d_same(ctx, &pt, &p2);
        break; /* we want to check a single non-equal next vertex */
      }
#if RTGEOM_DEBUG_LEVEL > 0
      if ( match ) {
        RTDEBUGF(ctx, 1, "Prev point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        RTDEBUGF(ctx, 1, "Prev point of edge %" RTTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( ! match )
    {
      RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " did not match as forward",
                 isoe->edge_id);
      rt_getPoint2d_p(ctx, epa, epa->npoints-1, &p2);
      RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " 'last' point is %g,%g",
                  isoe->edge_id, p2.x, p2.y);
      if ( p2d_same(ctx, &p1, &p2) )
      {
        RTDEBUGF(ctx, 1, "Last point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from);
        /* last point matches, let's check next non-equal one */
        for ( j=epa->npoints-2; j>=0; --j )
        {
          rt_getPoint2d_p(ctx, epa, j, &p2);
          RTDEBUGF(ctx, 1, "Edge %" RTTFMT_ELEMID " 'prev' point %d is %g,%g",
                      isoe->edge_id, j, p2.x, p2.y);
          /* we won't check duplicated edge points */
          if ( p2d_same(ctx, &p1, &p2) ) continue;
          /* we assume there are no duplicated points in ring */
          rt_getPoint2d_p(ctx, ring, from+1, &pt);
          RTDEBUGF(ctx, 1, "Ring's point %d is %g,%g",
                      from+1, pt.x, pt.y);
          match = p2d_same(ctx, &pt, &p2);
          break; /* we want to check a single non-equal next vertex */
        }
      }
#if RTGEOM_DEBUG_LEVEL > 0
      if ( match ) {
        RTDEBUGF(ctx, 1, "Prev point of edge %" RTTFMT_ELEMID
                    " matches ring vertex %d", isoe->edge_id, from+1);
      } else {
        RTDEBUGF(ctx, 1, "Prev point of edge %" RTTFMT_ELEMID
                    " does not match ring vertex %d", isoe->edge_id, from+1);
      }
#endif
    }

    if ( match ) return i;

  }

  return -1;
}

/* Reverse values in array between "from" (inclusive)
 * and "to" (exclusive) indexes */
static void
_rtt_ReverseElemidArray(RTT_ELEMID *ary, int from, int to)
{
  RTT_ELEMID t;
  while (from < to)
  {
    t = ary[from];
    ary[from++] = ary[to];
    ary[to--] = t;
  }
}

/* Rotate values in array between "from" (inclusive)
 * and "to" (exclusive) indexes, so that "rotidx" is
 * the new value at "from" */
static void
_rtt_RotateElemidArray(RTT_ELEMID *ary, int from, int to, int rotidx)
{
  _rtt_ReverseElemidArray(ary, from, rotidx-1);
  _rtt_ReverseElemidArray(ary, rotidx, to-1);
  _rtt_ReverseElemidArray(ary, from, to-1);
}


int
rtt_GetFaceEdges(RTT_TOPOLOGY* topo, RTT_ELEMID face_id, RTT_ELEMID **out )
{
  RTGEOM *face;
  RTPOLY *facepoly;
  RTT_ISO_EDGE *edges;
  int numfaceedges;
  int fields, i;
  int nseid = 0; /* number of signed edge ids */
  int prevseid;
  RTT_ELEMID *seid; /* signed edge ids */
  const RTT_BE_IFACE *iface = topo->be_iface;

  /* Get list of face edges */
  numfaceedges = 1;
  fields = RTT_COL_EDGE_EDGE_ID |
           RTT_COL_EDGE_GEOM |
           RTT_COL_EDGE_FACE_LEFT |
           RTT_COL_EDGE_FACE_RIGHT
           ;
  edges = rtt_be_getEdgeByFace( topo, &face_id, &numfaceedges, fields, NULL );
  if ( numfaceedges == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! numfaceedges ) return 0; /* no edges in output */

  /* order edges by occurrence in face */

  face = _rtt_FaceByEdges(topo, edges, numfaceedges);
  if ( ! face )
  {
    /* _rtt_FaceByEdges should have already invoked rterror in this case */
    rtt_release_edges(iface->ctx, edges, numfaceedges);
    return -1;
  }

  if ( rtgeom_is_empty(iface->ctx, face) )
  {
    /* no edges in output */
    rtt_release_edges(iface->ctx, edges, numfaceedges);
    rtgeom_free(iface->ctx, face);
    return 0;
  }

  /* force_lhr, if the face is not the universe */
  /* _rtt_FaceByEdges seems to guaranteed RHR */
  /* rtgeom_force_clockwise(iface->ctx, face); */
  if ( face_id ) rtgeom_reverse(iface->ctx, face);

#if 0
  {
  size_t sz;
  char *wkt = rtgeom_to_wkt(iface->ctx, face, RTWKT_EXTENDED, 6, &sz);
  RTDEBUGF(ctx, 1, "Geometry of face %" RTTFMT_ELEMID " is: %s",
              face_id, wkt);
  rtfree(iface->ctx, wkt);
  }
#endif

  facepoly = rtgeom_as_rtpoly(iface->ctx, face);
  if ( ! facepoly )
  {
    rtt_release_edges(iface->ctx, edges, numfaceedges);
    rtgeom_free(iface->ctx, face);
    rterror(iface->ctx, "Geometry of face %" RTTFMT_ELEMID " is not a polygon", face_id);
    return -1;
  }

  nseid = prevseid = 0;
  seid = rtalloc(iface->ctx,  sizeof(RTT_ELEMID) * numfaceedges );

  /* for each ring of the face polygon... */
  for ( i=0; i<facepoly->nrings; ++i )
  {
    const RTPOINTARRAY *ring = facepoly->rings[i];
    int j = 0;
    RTT_ISO_EDGE *nextedge;
    RTLINE *nextline;

    RTDEBUGF(iface->ctx, 1, "Ring %d has %d points", i, ring->npoints);

    while ( j < ring->npoints-1 )
    {
      RTDEBUGF(iface->ctx, 1, "Looking for edge covering ring %d from vertex %d",
                  i, j);

      int edgeno = _rtt_FindNextRingEdge(iface->ctx, ring, j, edges, numfaceedges);
      if ( edgeno == -1 )
      {
        /* should never happen */
        rtt_release_edges(iface->ctx, edges, numfaceedges);
        rtgeom_free(iface->ctx, face);
        rtfree(iface->ctx, seid);
        rterror(iface->ctx, "No edge (among %d) found to be defining geometry of face %"
                RTTFMT_ELEMID, numfaceedges, face_id);
        return -1;
      }

      nextedge = &(edges[edgeno]);
      nextline = nextedge->geom;

      RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID
                  " covers ring %d from vertex %d to %d",
                  nextedge->edge_id, i, j, j + nextline->points->npoints - 1);

#if 0
      {
      size_t sz;
      char *wkt = rtgeom_to_wkt(iface->ctx, rtline_as_rtgeom(iface->ctx, nextline), RTWKT_EXTENDED, 6, &sz);
      RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID " is %s",
                  nextedge->edge_id, wkt);
      rtfree(iface->ctx, wkt);
      }
#endif

      j += nextline->points->npoints - 1;

      /* Add next edge to the output array */
      seid[nseid++] = nextedge->face_left == face_id ?
                          nextedge->edge_id :
                         -nextedge->edge_id;

      /* avoid checking again on next time turn */
      nextedge->face_left = nextedge->face_right = -1;
    }

    /* now "scroll" the list of edges so that the one
     * with smaller absolute edge_id is first */
    /* Range is: [prevseid, nseid) -- [inclusive, exclusive) */
    if ( (nseid - prevseid) > 1 )
    {{
      RTT_ELEMID minid = 0;
      int minidx = 0;
      RTDEBUGF(iface->ctx, 1, "Looking for smallest id among the %d edges "
                  "composing ring %d", (nseid-prevseid), i);
      for ( j=prevseid; j<nseid; ++j )
      {
        RTT_ELEMID id = llabs(seid[j]);
        RTDEBUGF(iface->ctx, 1, "Abs id of edge in pos %d is %" RTTFMT_ELEMID, j, id);
        if ( ! minid || id < minid )
        {
          minid = id;
          minidx = j;
        }
      }
      RTDEBUGF(iface->ctx, 1, "Smallest id is %" RTTFMT_ELEMID
                  " at position %d", minid, minidx);
      if ( minidx != prevseid )
      {
        _rtt_RotateElemidArray(seid, prevseid, nseid, minidx);
      }
    }}

    prevseid = nseid;
  }

  rtgeom_free(iface->ctx, face);
  rtt_release_edges(iface->ctx, edges, numfaceedges);

  *out = seid;
  return nseid;
}

static GEOSGeometry *
_rtt_EdgeMotionArea(const RTCTX *ctx, RTLINE *geom, int isclosed)
{
  GEOSGeometry *gg;
  RTPOINT4D p4d;
  RTPOINTARRAY *pa;
  RTPOINTARRAY **pas;
  RTPOLY *poly;
  RTGEOM *g;

  pas = rtalloc(ctx, sizeof(RTPOINTARRAY*));

  _rtt_EnsureGeos(ctx);

  if ( isclosed )
  {
    pas[0] = ptarray_clone_deep(ctx,  geom->points );
    poly = rtpoly_construct(ctx, 0, 0, 1, pas);
    gg = RTGEOM2GEOS(ctx,  rtpoly_as_rtgeom(ctx, poly), 0 );
    rtpoly_free(ctx, poly); /* should also delete the pointarrays */
  }
  else
  {
    pa = geom->points;
    rt_getPoint4d_p(ctx, pa, 0, &p4d);
    pas[0] = ptarray_clone_deep(ctx,  pa );
    /* don't bother dup check */
    if ( RT_FAILURE == ptarray_append_point(ctx, pas[0], &p4d, RT_TRUE) )
    {
      ptarray_free(ctx, pas[0]);
      rtfree(ctx, pas);
      rterror(ctx, "Could not append point to pointarray");
      return NULL;
    }
    poly = rtpoly_construct(ctx, 0, NULL, 1, pas);
    /* make valid, in case the edge self-intersects on its first-last
     * vertex segment */
    g = rtgeom_make_valid(ctx, rtpoly_as_rtgeom(ctx, poly));
    rtpoly_free(ctx, poly); /* should also delete the pointarrays */
    if ( ! g )
    {
      rterror(ctx, "Could not make edge motion area valid");
      return NULL;
    }
    gg = RTGEOM2GEOS(ctx, g, 0);
    rtgeom_free(ctx, g);
  }
  if ( ! gg )
  {
    rterror(ctx, "Could not convert old edge area geometry to GEOS: %s",
            rtgeom_get_last_geos_error(ctx));
    return NULL;
  }
  return gg;
}

int
rtt_ChangeEdgeGeom(RTT_TOPOLOGY* topo, RTT_ELEMID edge_id, RTLINE *geom)
{
  RTT_ISO_EDGE *oldedge;
  RTT_ISO_EDGE newedge;
  RTPOINT2D p1, p2, pt;
  int i;
  int isclosed = 0;
  const RTT_BE_IFACE *iface = topo->be_iface;

  /* curve must be simple */
  if ( ! rtgeom_is_simple(iface->ctx, rtline_as_rtgeom(iface->ctx, geom)) )
  {
    rterror(iface->ctx, "SQL/MM Spatial exception - curve not simple");
    return -1;
  }

  i = 1;
  oldedge = rtt_be_getEdgeById(topo, &edge_id, &i, RTT_COL_EDGE_ALL);
  if ( ! oldedge )
  {
    RTDEBUGF(iface->ctx, 1, "rtt_ChangeEdgeGeom: "
                "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i == 0 )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge %"
              RTTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      rterror(iface->ctx, "Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return -1;
    }
  }

  RTDEBUGF(iface->ctx, 1, "rtt_ChangeEdgeGeom: "
              "old edge has %d points, new edge has %d points",
              oldedge->geom->points->npoints, geom->points->npoints);

  /*
   * e) Check StartPoint consistency
   */
  rt_getPoint2d_p(iface->ctx, oldedge->geom->points, 0, &p1);
  rt_getPoint2d_p(iface->ctx, geom->points, 0, &pt);
  if ( ! p2d_same(iface->ctx, &p1, &pt) )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "SQL/MM Spatial exception - "
            "start node not geometry start point.");
    return -1;
  }

  /*
   * f) Check EndPoint consistency
   */
  if ( oldedge->geom->points->npoints < 2 )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Corrupted topology: edge %" RTTFMT_ELEMID
            " has less than 2 vertices", oldedge->edge_id);
    return -1;
  }
  rt_getPoint2d_p(iface->ctx, oldedge->geom->points, oldedge->geom->points->npoints-1, &p2);
  if ( geom->points->npoints < 2 )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Invalid edge: less than 2 vertices");
    return -1;
  }
  rt_getPoint2d_p(iface->ctx, geom->points, geom->points->npoints-1, &pt);
  if ( ! p2d_same(iface->ctx, &pt, &p2) )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "SQL/MM Spatial exception - "
            "end node not geometry end point.");
    return -1;
  }

  /* Not in the specs:
   * if the edge is closed, check we didn't change winding !
   *       (should be part of isomorphism checking)
   */
  if ( oldedge->start_node == oldedge->end_node )
  {
    isclosed = 1;
#if 1 /* TODO: this is actually bogus as a test */
    /* check for valid edge (distinct vertices must exist) */
    if ( ! _rtt_GetInteriorEdgePoint(iface->ctx, geom, &pt) )
    {
      rtt_release_edges(iface->ctx, oldedge, 1);
      rterror(iface->ctx, "Invalid edge (no two distinct vertices exist)");
      return -1;
    }
#endif

    if ( ptarray_isccw(iface->ctx, oldedge->geom->points) !=
         ptarray_isccw(iface->ctx, geom->points) )
    {
      rtt_release_edges(iface->ctx, oldedge, 1);
      rterror(iface->ctx, "Edge twist at node POINT(%g %g)", p1.x, p1.y);
      return -1;
    }
  }

  if ( _rtt_CheckEdgeCrossing(topo, oldedge->start_node,
                                    oldedge->end_node, geom, edge_id ) )
  {
    /* would have called rterror already, leaking :( */
    rtt_release_edges(iface->ctx, oldedge, 1);
    return -1;
  }

  RTDEBUG(iface->ctx, 1, "rtt_ChangeEdgeGeom: "
             "edge crossing check passed ");

  /*
   * Not in the specs:
   * Check topological isomorphism
   */

  /* Check that the "motion range" doesn't include any node */
  // 1. compute combined bbox of old and new edge
  RTGBOX mbox; /* motion box */
  rtgeom_add_bbox(iface->ctx, (RTGEOM*)oldedge->geom); /* just in case */
  rtgeom_add_bbox(iface->ctx, (RTGEOM*)geom); /* just in case */
  gbox_union(iface->ctx, oldedge->geom->bbox, geom->bbox, &mbox);
  // 2. fetch all nodes in the combined box
  RTT_ISO_NODE *nodes;
  int numnodes;
  nodes = rtt_be_getNodeWithinBox2D(topo, &mbox, &numnodes,
                                          RTT_COL_NODE_ALL, 0);
  RTDEBUGF(iface->ctx, 1, "rtt_be_getNodeWithinBox2D returned %d nodes", numnodes);
  if ( numnodes == -1 ) {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  // 3. if any node beside endnodes are found:
  if ( numnodes > ( 1 + isclosed ? 0 : 1 ) )
  {{
    GEOSGeometry *oarea, *narea;
    const GEOSPreparedGeometry *oareap, *nareap;

    _rtt_EnsureGeos(iface->ctx);

    oarea = _rtt_EdgeMotionArea(iface->ctx, oldedge->geom, isclosed);
    if ( ! oarea )
    {
      rtt_release_edges(iface->ctx, oldedge, 1);
      rterror(iface->ctx, "Could not compute edge motion area for old edge");
      return -1;
    }

    narea = _rtt_EdgeMotionArea(iface->ctx, geom, isclosed);
    if ( ! narea )
    {
      GEOSGeom_destroy_r(iface->ctx->gctx, oarea);
      rtt_release_edges(iface->ctx, oldedge, 1);
      rterror(iface->ctx, "Could not compute edge motion area for new edge");
      return -1;
    }

    //   3.2. bail out if any node is in one and not the other
    oareap = GEOSPrepare_r(iface->ctx->gctx,  oarea );
    nareap = GEOSPrepare_r(iface->ctx->gctx,  narea );
    for (i=0; i<numnodes; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      GEOSGeometry *ngg;
      int ocont, ncont;
      size_t sz;
      char *wkt;
      if ( n->node_id == oldedge->start_node ) continue;
      if ( n->node_id == oldedge->end_node ) continue;
      ngg = RTGEOM2GEOS(iface->ctx,  rtpoint_as_rtgeom(iface->ctx, n->geom) , 0);
      ocont = GEOSPreparedContains_r(iface->ctx->gctx,  oareap, ngg );
      ncont = GEOSPreparedContains_r(iface->ctx->gctx,  nareap, ngg );
      GEOSGeom_destroy_r(iface->ctx->gctx, ngg);
      if (ocont == 2 || ncont == 2)
      {
        _rtt_release_nodes(iface->ctx, nodes, numnodes);
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, oareap);
        GEOSGeom_destroy_r(iface->ctx->gctx, oarea);
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, nareap);
        GEOSGeom_destroy_r(iface->ctx->gctx, narea);
        rterror(iface->ctx, "GEOS exception on PreparedContains: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -1;
      }
      if (ocont != ncont)
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, oareap);
        GEOSGeom_destroy_r(iface->ctx->gctx, oarea);
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, nareap);
        GEOSGeom_destroy_r(iface->ctx->gctx, narea);
        wkt = rtgeom_to_wkt(iface->ctx, rtpoint_as_rtgeom(iface->ctx, n->geom), RTWKT_ISO, 15, &sz);
        _rtt_release_nodes(iface->ctx, nodes, numnodes);
        rterror(iface->ctx, "Edge motion collision at %s", wkt);
        rtfree(iface->ctx, wkt); /* would not necessarely reach this point */
        return -1;
      }
    }
    GEOSPreparedGeom_destroy_r(iface->ctx->gctx, oareap);
    GEOSGeom_destroy_r(iface->ctx->gctx, oarea);
    GEOSPreparedGeom_destroy_r(iface->ctx->gctx, nareap);
    GEOSGeom_destroy_r(iface->ctx->gctx, narea);
  }}
  if ( numnodes ) _rtt_release_nodes(iface->ctx, nodes, numnodes);

  RTDEBUG(iface->ctx, 1, "nodes containment check passed");

  /*
   * Check edge adjacency before
   * TODO: can be optimized to gather azimuths of all edge ends once
   */

  edgeend span_pre, epan_pre;
  /* initialize span_pre.myaz and epan_pre.myaz with existing edge */
  i = _rtt_InitEdgeEndByLine(iface->ctx, &span_pre, &epan_pre,
                             oldedge->geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  _rtt_FindAdjacentEdges( topo, oldedge->start_node, &span_pre,
                                  isclosed ? &epan_pre : NULL, edge_id );
  _rtt_FindAdjacentEdges( topo, oldedge->end_node, &epan_pre,
                                  isclosed ? &span_pre : NULL, edge_id );

  RTDEBUGF(iface->ctx, 1, "edges adjacent to old edge are %" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID " (first point), %" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID " (last point)" RTTFMT_ELEMID,
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);

  /* update edge geometry */
  newedge.edge_id = edge_id;
  newedge.geom = geom;
  i = rtt_be_updateEdgesById(topo, &newedge, 1, RTT_COL_EDGE_GEOM);
  if ( i == -1 )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! i )
  {
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Unexpected error: %d edges updated when expecting 1", i);
    return -1;
  }

  /*
   * Check edge adjacency after
   */
  edgeend span_post, epan_post;
  i = _rtt_InitEdgeEndByLine(iface->ctx, &span_post, &epan_post, geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  /* initialize epan_post.myaz and epan_post.myaz */
  i = _rtt_InitEdgeEndByLine(iface->ctx, &span_post, &epan_post,
                             geom, &p1, &p2);
  if ( i ) return -1; /* rterror should have been raised */
  _rtt_FindAdjacentEdges( topo, oldedge->start_node, &span_post,
                          isclosed ? &epan_post : NULL, edge_id );
  _rtt_FindAdjacentEdges( topo, oldedge->end_node, &epan_post,
                          isclosed ? &span_post : NULL, edge_id );

  RTDEBUGF(iface->ctx, 1, "edges adjacent to new edge are %" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID " (first point), %" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID " (last point)" RTTFMT_ELEMID,
              span_pre.nextCW, span_pre.nextCCW,
              epan_pre.nextCW, epan_pre.nextCCW);


  /* Bail out if next CW or CCW edge on start node changed */
  if ( span_pre.nextCW != span_post.nextCW ||
       span_pre.nextCCW != span_post.nextCCW )
  {{
    RTT_ELEMID nid = oldedge->start_node;
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Edge changed disposition around start node %"
            RTTFMT_ELEMID, nid);
    return -1;
  }}

  /* Bail out if next CW or CCW edge on end node changed */
  if ( epan_pre.nextCW != epan_post.nextCW ||
       epan_pre.nextCCW != epan_post.nextCCW )
  {{
    RTT_ELEMID nid = oldedge->end_node;
    rtt_release_edges(iface->ctx, oldedge, 1);
    rterror(iface->ctx, "Edge changed disposition around end node %"
            RTTFMT_ELEMID, nid);
    return -1;
  }}

  /*
  -- Update faces MBR of left and right faces
  -- TODO: think about ways to optimize this part, like see if
  --       the old edge geometry partecipated in the definition
  --       of the current MBR (for shrinking) or the new edge MBR
  --       would be larger than the old face MBR...
  --
  */
  int facestoupdate = 0;
  RTT_ISO_FACE faces[2];
  RTGEOM *nface1 = NULL;
  RTGEOM *nface2 = NULL;
  if ( oldedge->face_left > 0 )
  {
    nface1 = rtt_GetFaceGeometry(topo, oldedge->face_left);
    if ( ! nface1 )
    {
      rterror(iface->ctx, "lwt_ChangeEdgeGeom could not construct face %"
                 RTTFMT_ELEMID ", on the left of edge %" RTTFMT_ELEMID,
                oldedge->face_left, edge_id);
      return -1;
    }
#if 0
    {
    size_t sz;
    char *wkt = rtgeom_to_wkt(iface->ctx, nface1, RTWKT_EXTENDED, 2, &sz);
    RTDEBUGF(iface->ctx, 1, "new geometry of face left (%d): %s", (int)oldedge->face_left, wkt);
    rtfree(iface->ctx, wkt);
    }
#endif
    rtgeom_add_bbox(iface->ctx, nface1);
    faces[facestoupdate].face_id = oldedge->face_left;
    /* ownership left to nface */
    faces[facestoupdate++].mbr = nface1->bbox;
  }
  if ( oldedge->face_right > 0
       /* no need to update twice the same face.. */
       && oldedge->face_right != oldedge->face_left )
  {
    nface2 = rtt_GetFaceGeometry(topo, oldedge->face_right);
    if ( ! nface2 )
    {
      rterror(iface->ctx, "lwt_ChangeEdgeGeom could not construct face %"
                 RTTFMT_ELEMID ", on the right of edge %" RTTFMT_ELEMID,
                oldedge->face_right, edge_id);
      return -1;
    }
#if 0
    {
    size_t sz;
    char *wkt = rtgeom_to_wkt(iface->ctx, nface2, RTWKT_EXTENDED, 2, &sz);
    RTDEBUGF(iface->ctx, 1, "new geometry of face right (%d): %s", (int)oldedge->face_right, wkt);
    rtfree(iface->ctx, wkt);
    }
#endif
    rtgeom_add_bbox(iface->ctx, nface2);
    faces[facestoupdate].face_id = oldedge->face_right;
    faces[facestoupdate++].mbr = nface2->bbox; /* ownership left to nface */
  }
  RTDEBUGF(iface->ctx, 1, "%d faces to update", facestoupdate);
  if ( facestoupdate )
  {
    i = rtt_be_updateFacesById( topo, &(faces[0]), facestoupdate );
    if ( i != facestoupdate )
    {
      if ( nface1 ) rtgeom_free(iface->ctx, nface1);
      if ( nface2 ) rtgeom_free(iface->ctx, nface2);
      rtt_release_edges(iface->ctx, oldedge, 1);
      if ( i == -1 )
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      else
        rterror(iface->ctx, "Unexpected error: %d faces found when expecting 1", i);
      return -1;
    }
  }
  if ( nface1 ) rtgeom_free(iface->ctx, nface1);
  if ( nface2 ) rtgeom_free(iface->ctx, nface2);

  RTDEBUG(iface->ctx, 1, "all done, cleaning up edges");

  rtt_release_edges(iface->ctx, oldedge, 1);
  return 0; /* success */
}

/* Only return CONTAINING_FACE in the node object */
static RTT_ISO_NODE *
_rtt_GetIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid)
{
  RTT_ISO_NODE *node;
  int n = 1;
  const RTT_BE_IFACE *iface = topo->be_iface;

  node = rtt_be_getNodeById( topo, &nid, &n, RTT_COL_NODE_CONTAINING_FACE );
  if ( n < 0 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return 0;
  }
  if ( n < 1 ) {
    rterror(iface->ctx, "SQL/MM Spatial exception - non-existent node");
    return 0;
  }
  if ( node->containing_face == -1 )
  {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "SQL/MM Spatial exception - not isolated node");
    return 0;
  }

  return node;
}

int
rtt_MoveIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid, RTPOINT *pt)
{
  RTT_ISO_NODE *node;
  int ret;
  const RTT_BE_IFACE *iface = topo->be_iface;

  node = _rtt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  if ( rtt_be_ExistsCoincidentNode(topo, pt) )
  {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "SQL/MM Spatial exception - coincident node");
    return -1;
  }

  if ( rtt_be_ExistsEdgeIntersectingPoint(topo, pt) )
  {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "SQL/MM Spatial exception - edge crosses node.");
    return -1;
  }

  /* TODO: check that the new point is in the same containing face !
   * See https://trac.osgeo.org/postgis/ticket/3232
   */

  node->node_id = nid;
  node->geom = pt;
  ret = rtt_be_updateNodesById(topo, node, 1,
                               RTT_COL_NODE_GEOM);
  if ( ret == -1 ) {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  rtfree(iface->ctx, node);
  return 0;
}

int
rtt_RemoveIsoNode(RTT_TOPOLOGY* topo, RTT_ELEMID nid)
{
  RTT_ISO_NODE *node;
  int n = 1;
  const RTT_BE_IFACE *iface = topo->be_iface;

  node = _rtt_GetIsoNode( topo, nid );
  if ( ! node ) return -1;

  n = rtt_be_deleteNodesById( topo, &nid, n );
  if ( n == -1 )
  {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( n != 1 )
  {
    rtfree(iface->ctx, node);
    rterror(iface->ctx, "Unexpected error: %d nodes deleted when expecting 1", n);
    return -1;
  }

  /* TODO: notify to caller about node being removed ?
   * See https://trac.osgeo.org/postgis/ticket/3231
   */

  rtfree(iface->ctx, node);
  return 0; /* success */
}

int
rtt_RemIsoEdge(RTT_TOPOLOGY* topo, RTT_ELEMID id)
{
  RTT_ISO_EDGE deledge;
  RTT_ISO_EDGE *edge;
  RTT_ELEMID nid[2];
  RTT_ISO_NODE upd_node[2];
  RTT_ELEMID containing_face;
  int n = 1;
  int i;
  const RTT_BE_IFACE *iface = topo->be_iface;

  edge = rtt_be_getEdgeById( topo, &id, &n, RTT_COL_EDGE_START_NODE|
                                            RTT_COL_EDGE_END_NODE |
                                            RTT_COL_EDGE_FACE_LEFT |
                                            RTT_COL_EDGE_FACE_RIGHT );
  if ( ! edge )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! n )
  {
    rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge");
    return -1;
  }
  if ( n > 1 )
  {
    rtfree(iface->ctx, edge);
    rterror(iface->ctx, "Corrupted topology: more than a single edge have id %"
            RTTFMT_ELEMID, id);
    return -1;
  }

  if ( edge[0].face_left != edge[0].face_right )
  {
    rtfree(iface->ctx, edge);
    rterror(iface->ctx, "SQL/MM Spatial exception - not isolated edge");
    return -1;
  }
  containing_face = edge[0].face_left;

  nid[0] = edge[0].start_node;
  nid[1] = edge[0].end_node;
  rtfree(iface->ctx, edge);

  n = 2;
  edge = rtt_be_getEdgeByNode( topo, nid, &n, RTT_COL_EDGE_EDGE_ID );
  if ( n == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<n; ++i )
  {
    if ( edge[i].edge_id == id ) continue;
    rtfree(iface->ctx, edge);
    rterror(iface->ctx, "SQL/MM Spatial exception - not isolated edge");
    return -1;
  }
  if ( edge ) rtfree(iface->ctx, edge);

  deledge.edge_id = id;
  n = rtt_be_deleteEdges( topo, &deledge, RTT_COL_EDGE_EDGE_ID );
  if ( n == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( n != 1 )
  {
    rterror(iface->ctx, "Unexpected error: %d edges deleted when expecting 1", n);
    return -1;
  }

  upd_node[0].node_id = nid[0];
  upd_node[0].containing_face = containing_face;
  n = 1;
  if ( nid[1] != nid[0] ) {
    upd_node[1].node_id = nid[1];
    upd_node[1].containing_face = containing_face;
    ++n;
  }
  n = rtt_be_updateNodesById(topo, upd_node, n,
                               RTT_COL_NODE_CONTAINING_FACE);
  if ( n == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* TODO: notify to caller about edge being removed ?
   * See https://trac.osgeo.org/postgis/ticket/3248
   */

  return 0; /* success */
}

/* Used by _rtt_RemEdge to update edge face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_rtt_UpdateEdgeFaceRef( RTT_TOPOLOGY *topo, RTT_ELEMID of, RTT_ELEMID nf)
{
  RTT_ISO_EDGE sel_edge, upd_edge;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel_edge.face_left = of;
  upd_edge.face_left = nf;
  ret = rtt_be_updateEdges(topo, &sel_edge, RTT_COL_EDGE_FACE_LEFT,
                                 &upd_edge, RTT_COL_EDGE_FACE_LEFT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  /* Update face_right for all edges still referencing old face */
  sel_edge.face_right = of;
  upd_edge.face_right = nf;
  ret = rtt_be_updateEdges(topo, &sel_edge, RTT_COL_EDGE_FACE_RIGHT,
                                 &upd_edge, RTT_COL_EDGE_FACE_RIGHT,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by _rtt_RemEdge to update node face ref on healing
 *
 * @param of old face id (never 0 as you cannot remove face 0)
 * @param nf new face id
 * @return 0 on success, -1 on backend error
 */
static int
_rtt_UpdateNodeFaceRef( RTT_TOPOLOGY *topo, RTT_ELEMID of, RTT_ELEMID nf)
{
  RTT_ISO_NODE sel, upd;
  int ret;

  assert( of != 0 );

  /* Update face_left for all edges still referencing old face */
  sel.containing_face = of;
  upd.containing_face = nf;
  ret = rtt_be_updateNodes(topo, &sel, RTT_COL_NODE_CONTAINING_FACE,
                                 &upd, RTT_COL_NODE_CONTAINING_FACE,
                                 NULL, 0);
  if ( ret == -1 ) return -1;

  return 0;
}

/* Used by rtt_RemEdgeModFace and rtt_RemEdgeNewFaces
 *
 * Returns -1 on error, identifier of the face that takes up the space
 * previously occupied by the removed edge if modFace is 1, identifier of
 * the created face (0 if none) if modFace is 0.
 */
static RTT_ELEMID
_rtt_RemEdge(RTT_TOPOLOGY* topo, RTT_ELEMID edge_id, int modFace )
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  int i, nedges, nfaces, fields;
  RTT_ISO_EDGE *edge = NULL;
  RTT_ISO_EDGE *upd_edge = NULL;
  RTT_ISO_EDGE upd_edge_left[2];
  int nedge_left = 0;
  RTT_ISO_EDGE upd_edge_right[2];
  int nedge_right = 0;
  RTT_ISO_NODE upd_node[2];
  int nnode = 0;
  RTT_ISO_FACE *faces = NULL;
  RTT_ISO_FACE newface;
  RTT_ELEMID node_ids[2];
  RTT_ELEMID face_ids[2];
  int fnode_edges = 0; /* number of edges on the first node (excluded
                        * the one being removed ) */
  int lnode_edges = 0; /* number of edges on the last node (excluded
                        * the one being removed ) */

  newface.face_id = 0;

  i = 1;
  edge = rtt_be_getEdgeById(topo, &edge_id, &i, RTT_COL_EDGE_ALL);
  if ( ! edge )
  {
    RTDEBUGF(iface->ctx, 1, "rtt_be_getEdgeById returned NULL and set i=%d", i);
    if ( i == -1 )
    {
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i == 0 )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge %"
              RTTFMT_ELEMID, edge_id);
      return -1;
    }
    else
    {
      rterror(iface->ctx, "Backend coding error: getEdgeById callback returned NULL "
              "but numelements output parameter has value %d "
              "(expected 0 or 1)", i);
      return -1;
    }
  }

  if ( ! rtt_be_checkTopoGeomRemEdge(topo, edge_id,
                                     edge->face_left, edge->face_right) )
  {
    rterror(iface->ctx, "%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  RTDEBUG(iface->ctx, 1, "Updating next_{right,left}_face of ring edges...");

  /* Update edge linking */

  nedges = 0;
  node_ids[nedges++] = edge->start_node;
  if ( edge->end_node != edge->start_node )
  {
    node_ids[nedges++] = edge->end_node;
  }
  fields = RTT_COL_EDGE_EDGE_ID | RTT_COL_EDGE_START_NODE |
           RTT_COL_EDGE_END_NODE | RTT_COL_EDGE_NEXT_LEFT |
           RTT_COL_EDGE_NEXT_RIGHT;
  upd_edge = rtt_be_getEdgeByNode( topo, &(node_ids[0]), &nedges, fields );
  if ( nedges == -1 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  nedge_left = nedge_right = 0;
  for ( i=0; i<nedges; ++i )
  {
    RTT_ISO_EDGE *e = &(upd_edge[i]);
    if ( e->edge_id == edge_id ) continue;
    if ( e->start_node == edge->start_node || e->end_node == edge->start_node )
    {
      ++fnode_edges;
    }
    if ( e->start_node == edge->end_node || e->end_node == edge->end_node )
    {
      ++lnode_edges;
    }
    if ( e->next_left == -edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_left == edge_id )
    {
      upd_edge_left[nedge_left].edge_id = e->edge_id;
      upd_edge_left[nedge_left++].next_left =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }

    if ( e->next_right == -edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_left != edge_id ? edge->next_left : edge->next_right;
    }
    else if ( e->next_right == edge_id )
    {
      upd_edge_right[nedge_right].edge_id = e->edge_id;
      upd_edge_right[nedge_right++].next_right =
        edge->next_right != -edge_id ? edge->next_right : edge->next_left;
    }
  }

  if ( nedge_left )
  {
    RTDEBUGF(iface->ctx, 1, "updating %d 'next_left' edges", nedge_left);
    /* update edges in upd_edge_left set next_left */
    i = rtt_be_updateEdgesById(topo, &(upd_edge_left[0]), nedge_left,
                               RTT_COL_EDGE_NEXT_LEFT);
    if ( i == -1 )
    {
      rtt_release_edges(iface->ctx, edge, 1);
      rtfree(iface->ctx, upd_edge);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  if ( nedge_right )
  {
    RTDEBUGF(iface->ctx, 1, "updating %d 'next_right' edges", nedge_right);
    /* update edges in upd_edge_right set next_right */
    i = rtt_be_updateEdgesById(topo, &(upd_edge_right[0]), nedge_right,
                               RTT_COL_EDGE_NEXT_RIGHT);
    if ( i == -1 )
    {
      rtt_release_edges(iface->ctx, edge, 1);
      rtfree(iface->ctx, upd_edge);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }
  RTDEBUGF(iface->ctx, 1, "releasing %d updateable edges in %p", nedges, upd_edge);
  rtfree(iface->ctx, upd_edge);

  /* Id of face that will take up all the space previously
   * taken by left and right faces of the edge */
  RTT_ELEMID floodface;

  /* Find floodface, and update its mbr if != 0 */
  if ( edge->face_left == edge->face_right )
  {
    floodface = edge->face_right;
  }
  else
  {
    /* Two faces healed */
    if ( edge->face_left == 0 || edge->face_right == 0 )
    {
      floodface = 0;
      RTDEBUG(iface->ctx, 1, "floodface is universe");
    }
    else
    {
      /* we choose right face as the face that will remain
       * to be symmetric with ST_AddEdgeModFace */
      floodface = edge->face_right;
      RTDEBUGF(iface->ctx, 1, "floodface is %" RTTFMT_ELEMID, floodface);
      /* update mbr of floodface as union of mbr of both faces */
      face_ids[0] = edge->face_left;
      face_ids[1] = edge->face_right;
      nfaces = 2;
      fields = RTT_COL_FACE_ALL;
      faces = rtt_be_getFaceById(topo, face_ids, &nfaces, fields);
      if ( nfaces == -1 ) {
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      RTGBOX *box1=NULL;
      RTGBOX *box2=NULL;
      for ( i=0; i<nfaces; ++i )
      {
        if ( faces[i].face_id == edge->face_left )
        {
          if ( ! box1 ) box1 = faces[i].mbr;
          else
          {
            i = edge->face_left;
            rtt_release_edges(iface->ctx, edge, 1);
            _rtt_release_faces(iface->ctx, faces, nfaces);
            rterror(iface->ctx, "corrupted topology: more than 1 face have face_id=%"
                    RTTFMT_ELEMID, i);
            return -1;
          }
        }
        else if ( faces[i].face_id == edge->face_right )
        {
          if ( ! box2 ) box2 = faces[i].mbr;
          else
          {
            i = edge->face_right;
            rtt_release_edges(iface->ctx, edge, 1);
            _rtt_release_faces(iface->ctx, faces, nfaces);
            rterror(iface->ctx, "corrupted topology: more than 1 face have face_id=%"
                    RTTFMT_ELEMID, i);
            return -1;
          }
        }
        else
        {
          i = faces[i].face_id;
          rtt_release_edges(iface->ctx, edge, 1);
          _rtt_release_faces(iface->ctx, faces, nfaces);
          rterror(iface->ctx, "Backend coding error: getFaceById returned face "
                  "with non-requested id %" RTTFMT_ELEMID, i);
          return -1;
        }
      }
      if ( ! box1 ) {
        i = edge->face_left;
        rtt_release_edges(iface->ctx, edge, 1);
        _rtt_release_faces(iface->ctx, faces, nfaces);
        rterror(iface->ctx, "corrupted topology: no face have face_id=%"
                RTTFMT_ELEMID " (left face for edge %"
                RTTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      if ( ! box2 ) {
        i = edge->face_right;
        rtt_release_edges(iface->ctx, edge, 1);
        _rtt_release_faces(iface->ctx, faces, nfaces);
        rterror(iface->ctx, "corrupted topology: no face have face_id=%"
                RTTFMT_ELEMID " (right face for edge %"
                RTTFMT_ELEMID ")", i, edge_id);
        return -1;
      }
      gbox_merge(iface->ctx, box2, box1); /* box1 is now the union of the two */
      newface.mbr = box1;
      if ( modFace )
      {
        newface.face_id = floodface;
        i = rtt_be_updateFacesById( topo, &newface, 1 );
        _rtt_release_faces(iface->ctx, faces, 2);
        if ( i == -1 )
        {
          rtt_release_edges(iface->ctx, edge, 1);
          rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
          return -1;
        }
        if ( i != 1 )
        {
          rtt_release_edges(iface->ctx, edge, 1);
          rterror(iface->ctx, "Unexpected error: %d faces updated when expecting 1", i);
          return -1;
        }
      }
      else
      {
        /* New face replaces the old two faces */
        newface.face_id = -1;
        i = rtt_be_insertFaces( topo, &newface, 1 );
        _rtt_release_faces(iface->ctx, faces, 2);
        if ( i == -1 )
        {
          rtt_release_edges(iface->ctx, edge, 1);
          rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
          return -1;
        }
        if ( i != 1 )
        {
          rtt_release_edges(iface->ctx, edge, 1);
          rterror(iface->ctx, "Unexpected error: %d faces inserted when expecting 1", i);
          return -1;
        }
        floodface = newface.face_id;
      }
    }

    /* Update face references for edges and nodes still referencing
     * the removed face(s) */

    if ( edge->face_left != floodface )
    {
      if ( -1 == _rtt_UpdateEdgeFaceRef(topo, edge->face_left, floodface) )
      {
        rtt_release_edges(iface->ctx, edge, 1);
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      if ( -1 == _rtt_UpdateNodeFaceRef(topo, edge->face_left, floodface) )
      {
        rtt_release_edges(iface->ctx, edge, 1);
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }

    if ( edge->face_right != floodface )
    {
      if ( -1 == _rtt_UpdateEdgeFaceRef(topo, edge->face_right, floodface) )
      {
        rtt_release_edges(iface->ctx, edge, 1);
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      if ( -1 == _rtt_UpdateNodeFaceRef(topo, edge->face_right, floodface) )
      {
        rtt_release_edges(iface->ctx, edge, 1);
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
    }

    /* Update topogeoms on heal */
    if ( ! rtt_be_updateTopoGeomFaceHeal(topo,
                                  edge->face_right, edge->face_left,
                                  floodface) )
    {
      rtt_release_edges(iface->ctx, edge, 1);
      rterror(iface->ctx, "%s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  } /* two faces healed */

  /* Delete the edge */
  i = rtt_be_deleteEdges(topo, edge, RTT_COL_EDGE_EDGE_ID);
  if ( i == -1 ) {
    rtt_release_edges(iface->ctx, edge, 1);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* If any of the edge nodes remained isolated, set
   * containing_face = floodface
   */
  if ( ! fnode_edges )
  {
    upd_node[nnode].node_id = edge->start_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( edge->end_node != edge->start_node && ! lnode_edges )
  {
    upd_node[nnode].node_id = edge->end_node;
    upd_node[nnode].containing_face = floodface;
    ++nnode;
  }
  if ( nnode )
  {
    i = rtt_be_updateNodesById(topo, upd_node, nnode,
                               RTT_COL_NODE_CONTAINING_FACE);
    if ( i == -1 ) {
      rtt_release_edges(iface->ctx, edge, 1);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  if ( edge->face_left != edge->face_right )
  /* or there'd be no face to remove */
  {
    RTT_ELEMID ids[2];
    int nids = 0;
    if ( edge->face_right != floodface )
      ids[nids++] = edge->face_right;
    if ( edge->face_left != floodface )
      ids[nids++] = edge->face_left;
    i = rtt_be_deleteFacesById(topo, ids, nids);
    if ( i == -1 ) {
      rtt_release_edges(iface->ctx, edge, 1);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  rtt_release_edges(iface->ctx, edge, 1);
  return modFace ? floodface : newface.face_id;
}

RTT_ELEMID
rtt_RemEdgeModFace( RTT_TOPOLOGY* topo, RTT_ELEMID edge_id )
{
  return _rtt_RemEdge( topo, edge_id, 1 );
}

RTT_ELEMID
rtt_RemEdgeNewFace( RTT_TOPOLOGY* topo, RTT_ELEMID edge_id )
{
  return _rtt_RemEdge( topo, edge_id, 0 );
}

static RTT_ELEMID
_rtt_HealEdges( RTT_TOPOLOGY* topo, RTT_ELEMID eid1, RTT_ELEMID eid2,
                int modEdge )
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTT_ELEMID ids[2];
  RTT_ELEMID commonnode = -1;
  int caseno = 0;
  RTT_ISO_EDGE *node_edges;
  int num_node_edges;
  RTT_ISO_EDGE *edges;
  RTT_ISO_EDGE *e1 = NULL;
  RTT_ISO_EDGE *e2 = NULL;
  RTT_ISO_EDGE newedge, updedge, seledge;
  int nedges, i;
  int e1freenode;
  int e2sign, e2freenode;
  RTPOINTARRAY *pa;
  char buf[256];
  char *ptr;
  size_t bufleft = 256;

  ptr = buf;

  /* NOT IN THE SPECS: see if the same edge is given twice.. */
  if ( eid1 == eid2 )
  {
    rterror(iface->ctx, "Cannot heal edge %" RTTFMT_ELEMID
            " with itself, try with another", eid1);
    return -1;
  }
  ids[0] = eid1;
  ids[1] = eid2;
  nedges = 2;
  edges = rtt_be_getEdgeById(topo, ids, &nedges, RTT_COL_EDGE_ALL);
  if ( nedges == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for ( i=0; i<nedges; ++i )
  {
    if ( edges[i].edge_id == eid1 ) {
      if ( e1 ) {
        rtt_release_edges(iface->ctx, edges, nedges);
        rterror(iface->ctx, "Corrupted topology: multiple edges have id %"
                RTTFMT_ELEMID, eid1);
        return -1;
      }
      e1 = &(edges[i]);
    }
    else if ( edges[i].edge_id == eid2 ) {
      if ( e2 ) {
        rtt_release_edges(iface->ctx, edges, nedges);
        rterror(iface->ctx, "Corrupted topology: multiple edges have id %"
                RTTFMT_ELEMID, eid2);
        return -1;
      }
      e2 = &(edges[i]);
    }
  }
  if ( ! e1 )
  {
    if ( edges ) rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge %"
            RTTFMT_ELEMID, eid1);
    return -1;
  }
  if ( ! e2 )
  {
    if ( edges ) rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "SQL/MM Spatial exception - non-existent edge %"
            RTTFMT_ELEMID, eid2);
    return -1;
  }

  /* NOT IN THE SPECS: See if any of the two edges are closed. */
  if ( e1->start_node == e1->end_node )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "Edge %" RTTFMT_ELEMID " is closed, cannot heal to edge %"
            RTTFMT_ELEMID, eid1, eid2);
    return -1;
  }
  if ( e2->start_node == e2->end_node )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "Edge %" RTTFMT_ELEMID " is closed, cannot heal to edge %"
            RTTFMT_ELEMID, eid2, eid1);
    return -1;
  }

  /* Find common node */

  if ( e1->end_node == e2->start_node )
  {
    commonnode = e1->end_node;
    caseno = 1;
  }
  else if ( e1->end_node == e2->end_node )
  {
    commonnode = e1->end_node;
    caseno = 2;
  }
  /* Check if any other edge is connected to the common node, if found */
  if ( commonnode != -1 )
  {
    num_node_edges = 1;
    node_edges = rtt_be_getEdgeByNode( topo, &commonnode,
                                       &num_node_edges, RTT_COL_EDGE_EDGE_ID );
    if ( num_node_edges == -1 ) {
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    for (i=0; i<num_node_edges; ++i)
    {
      int r;
      if ( node_edges[i].edge_id == eid1 ) continue;
      if ( node_edges[i].edge_id == eid2 ) continue;
      commonnode = -1;
      /* append to string, for error message */
      if ( bufleft ) {
        r = snprintf(ptr, bufleft, "%s%" RTTFMT_ELEMID,
                     ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
        if ( r >= bufleft )
        {
          bufleft = 0;
          buf[252] = '.';
          buf[253] = '.';
          buf[254] = '.';
          buf[255] = '\0';
        }
        else
        {
          bufleft -= r;
          ptr += r;
        }
      }
    }
    rtfree(iface->ctx, node_edges);
  }

  if ( commonnode == -1 )
  {
    if ( e1->start_node == e2->start_node )
    {
      commonnode = e1->start_node;
      caseno = 3;
    }
    else if ( e1->start_node == e2->end_node )
    {
      commonnode = e1->start_node;
      caseno = 4;
    }
    /* Check if any other edge is connected to the common node, if found */
    if ( commonnode != -1 )
    {
      num_node_edges = 1;
      node_edges = rtt_be_getEdgeByNode( topo, &commonnode,
                                         &num_node_edges, RTT_COL_EDGE_EDGE_ID );
      if ( num_node_edges == -1 ) {
        rtt_release_edges(iface->ctx, edges, nedges);
        rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
        return -1;
      }
      for (i=0; i<num_node_edges; ++i)
      {
        int r;
        if ( node_edges[i].edge_id == eid1 ) continue;
        if ( node_edges[i].edge_id == eid2 ) continue;
        commonnode = -1;
        /* append to string, for error message */
        if ( bufleft ) {
          r = snprintf(ptr, bufleft, "%s%" RTTFMT_ELEMID,
                       ( ptr==buf ? "" : "," ), node_edges[i].edge_id);
          if ( r >= bufleft )
          {
            bufleft = 0;
            buf[252] = '.';
            buf[253] = '.';
            buf[254] = '.';
            buf[255] = '\0';
          }
          else
          {
            bufleft -= r;
            ptr += r;
          }
        }
      }
      if ( num_node_edges ) rtfree(iface->ctx, node_edges);
    }
  }

  if ( commonnode == -1 )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    if ( ptr != buf )
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - other edges connected (%s)",
              buf);
    }
    else
    {
      rterror(iface->ctx, "SQL/MM Spatial exception - non-connected edges");
    }
    return -1;
  }

  if ( ! rtt_be_checkTopoGeomRemNode(topo, commonnode,
                                     eid1, eid2 ) )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* Construct the geometry of the new edge */
  switch (caseno)
  {
    case 1: /* e1.end = e2.start */
      pa = ptarray_clone_deep(iface->ctx, e1->geom->points);
      //pa = ptarray_merge(iface->ctx, pa, e2->geom->points);
      ptarray_append_ptarray(iface->ctx, pa, e2->geom->points, 0);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->end_node;
      newedge.next_left = e2->next_left;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = -1;
      e2sign = 1;
      break;
    case 2: /* e1.end = e2.end */
    {
      RTPOINTARRAY *pa2;
      pa2 = ptarray_clone_deep(iface->ctx, e2->geom->points);
      ptarray_reverse(iface->ctx, pa2);
      pa = ptarray_clone_deep(iface->ctx, e1->geom->points);
      //pa = ptarray_merge(iface->ctx, e1->geom->points, pa);
      ptarray_append_ptarray(iface->ctx, pa, pa2, 0);
      ptarray_free(iface->ctx, pa2);
      newedge.start_node = e1->start_node;
      newedge.end_node = e2->start_node;
      newedge.next_left = e2->next_right;
      newedge.next_right = e1->next_right;
      e1freenode = 1;
      e2freenode = 1;
      e2sign = -1;
      break;
    }
    case 3: /* e1.start = e2.start */
      pa = ptarray_clone_deep(iface->ctx, e2->geom->points);
      ptarray_reverse(iface->ctx, pa);
      //pa = ptarray_merge(iface->ctx, pa, e1->geom->points);
      ptarray_append_ptarray(iface->ctx, pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->end_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_left;
      e1freenode = -1;
      e2freenode = -1;
      e2sign = -1;
      break;
    case 4: /* e1.start = e2.end */
      pa = ptarray_clone_deep(iface->ctx, e2->geom->points);
      //pa = ptarray_merge(iface->ctx, pa, e1->geom->points);
      ptarray_append_ptarray(iface->ctx, pa, e1->geom->points, 0);
      newedge.end_node = e1->end_node;
      newedge.start_node = e2->start_node;
      newedge.next_left = e1->next_left;
      newedge.next_right = e2->next_right;
      e1freenode = -1;
      e2freenode = 1;
      e2sign = 1;
      break;
    default:
      pa = NULL;
      e1freenode = 0;
      e2freenode = 0;
      e2sign = 0;
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Coding error: caseno=%d should never happen", caseno);
      break;
  }
  newedge.geom = rtline_construct(iface->ctx, topo->srid, NULL, pa);

  if ( modEdge )
  {
    /* Update data of the first edge */
    newedge.edge_id = eid1;
    i = rtt_be_updateEdgesById(topo, &newedge, 1,
                               RTT_COL_EDGE_NEXT_LEFT|
                               RTT_COL_EDGE_NEXT_RIGHT |
                               RTT_COL_EDGE_START_NODE |
                               RTT_COL_EDGE_END_NODE |
                               RTT_COL_EDGE_GEOM);
    if ( i == -1 )
    {
      rtline_free(iface->ctx, newedge.geom);
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    else if ( i != 1 )
    {
      rtline_free(iface->ctx, newedge.geom);
      if ( edges ) rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Unexpected error: %d edges updated when expecting 1", i);
      return -1;
    }
  }
  else
  {
    /* Add new edge */
    newedge.edge_id = -1;
    newedge.face_left = e1->face_left;
    newedge.face_right = e1->face_right;
    i = rtt_be_insertEdges(topo, &newedge, 1);
    if ( i == -1 ) {
      rtline_free(iface->ctx, newedge.geom);
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    } else if ( i == 0 ) {
      rtline_free(iface->ctx, newedge.geom);
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Insertion of split edge failed (no reason)");
      return -1;
    }
  }
  rtline_free(iface->ctx, newedge.geom);

  /*
  -- Update next_left_edge/next_right_edge for
  -- any edge having them still pointing at the edge being removed
  -- (eid2 only when modEdge, or both otherwise)
  --
  -- NOTE:
  -- e#freenode is 1 when edge# end node was the common node
  -- and -1 otherwise. This gives the sign of possibly found references
  -- to its "free" (non connected to other edge) endnode.
  -- e2sign is -1 if edge1 direction is opposite to edge2 direction,
  -- or 1 otherwise.
  --
  */

  /* update edges connected to e2's boundary from their end node */
  seledge.next_left = e2freenode * eid2;
  updedge.next_left = e2freenode * newedge.edge_id * e2sign;
  i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_LEFT,
                               &updedge, RTT_COL_EDGE_NEXT_LEFT,
                               NULL, 0);
  if ( i == -1 )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /* update edges connected to e2's boundary from their start node */
  seledge.next_right = e2freenode * eid2;
  updedge.next_right = e2freenode * newedge.edge_id * e2sign;
  i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_RIGHT,
                               &updedge, RTT_COL_EDGE_NEXT_RIGHT,
                               NULL, 0);
  if ( i == -1 )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  if ( ! modEdge )
  {
    /* update edges connected to e1's boundary from their end node */
    seledge.next_left = e1freenode * eid1;
    updedge.next_left = e1freenode * newedge.edge_id;
    i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_LEFT,
                                 &updedge, RTT_COL_EDGE_NEXT_LEFT,
                                 NULL, 0);
    if ( i == -1 )
    {
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }

    /* update edges connected to e1's boundary from their start node */
    seledge.next_right = e1freenode * eid1;
    updedge.next_right = e1freenode * newedge.edge_id;
    i = rtt_be_updateEdges(topo, &seledge, RTT_COL_EDGE_NEXT_RIGHT,
                                 &updedge, RTT_COL_EDGE_NEXT_RIGHT,
                                 NULL, 0);
    if ( i == -1 )
    {
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  /* delete the edges (only second on modEdge or both) */
  i = rtt_be_deleteEdges(topo, e2, RTT_COL_EDGE_EDGE_ID);
  if ( i == -1 )
  {
    rtt_release_edges(iface->ctx, edges, nedges);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( ! modEdge ) {
    i = rtt_be_deleteEdges(topo, e1, RTT_COL_EDGE_EDGE_ID);
    if ( i == -1 )
    {
      rtt_release_edges(iface->ctx, edges, nedges);
      rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
  }

  rtt_release_edges(iface->ctx, edges, nedges);

  /* delete the common node */
  i = rtt_be_deleteNodesById( topo, &commonnode, 1 );
  if ( i == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  /*
  --
  -- NOT IN THE SPECS:
  -- Drop composition rows involving second
  -- edge, as the first edge took its space,
  -- and all affected TopoGeom have been previously checked
  -- for being composed by both edges.
  */
  if ( ! rtt_be_updateTopoGeomEdgeHeal(topo,
                                eid1, eid2, newedge.edge_id) )
  {
    rterror(iface->ctx, "%s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  return modEdge ? commonnode : newedge.edge_id;
}

RTT_ELEMID
rtt_ModEdgeHeal( RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2 )
{
  return _rtt_HealEdges( topo, e1, e2, 1 );
}

RTT_ELEMID
rtt_NewEdgeHeal( RTT_TOPOLOGY* topo, RTT_ELEMID e1, RTT_ELEMID e2 )
{
  return _rtt_HealEdges( topo, e1, e2, 0 );
}

RTT_ELEMID
rtt_GetNodeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ISO_NODE *elem;
  int num;
  int flds = RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM; /* geom not needed */
  RTT_ELEMID id = 0;
  RTPOINT2D qp; /* query point */
  const RTT_BE_IFACE *iface = topo->be_iface;

  if ( ! rt_getPoint2d_p(iface->ctx, pt->point, 0, &qp) )
  {
    rterror(iface->ctx, "Empty query point");
    return -1;
  }
  elem = rtt_be_getNodeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  else if ( num )
  {
    if ( num > 1 )
    {
      _rtt_release_nodes(iface->ctx, elem, num);
      rterror(iface->ctx, "Two or more nodes found");
      return -1;
    }
    id = elem[0].node_id;
    _rtt_release_nodes(iface->ctx, elem, num);
  }

  return id;
}

RTT_ELEMID
rtt_GetEdgeByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ISO_EDGE *elem;
  int num, i;
  int flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM; /* GEOM is not needed */
  RTT_ELEMID id = 0;
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTGEOM *qp = rtpoint_as_rtgeom(iface->ctx, pt); /* query point */

  if ( rtgeom_is_empty(iface->ctx, qp) )
  {
    rterror(iface->ctx, "Empty query point");
    return -1;
  }
  elem = rtt_be_getEdgeWithinDistance2D(topo, pt, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for (i=0; i<num;++i)
  {
    RTT_ISO_EDGE *e = &(elem[i]);
#if 0
    RTGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      rtt_release_edges(iface->ctx, elem, num);
      rtnotice(iface->ctx, "Corrupted topology: edge %" RTTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* Should we check for intersection not being on an endpoint
     * as documented ? */
    geom = rtline_as_rtgeom(iface->ctx, e->geom);
    dist = rtgeom_mindistance2d_tolerance(iface->ctx, geom, qp, tol);
    if ( dist > tol ) continue;
#endif

    if ( id )
    {
      rtt_release_edges(iface->ctx, elem, num);
      rterror(iface->ctx, "Two or more edges found");
      return -1;
    }
    else id = e->edge_id;
  }

  if ( num ) rtt_release_edges(iface->ctx, elem, num);

  return id;
}

RTT_ELEMID
rtt_GetFaceByPoint(RTT_TOPOLOGY *topo, RTPOINT *pt, double tol)
{
  RTT_ELEMID id = 0;
  RTT_ISO_EDGE *elem;
  int num, i;
  int flds = RTT_COL_EDGE_EDGE_ID |
             RTT_COL_EDGE_GEOM |
             RTT_COL_EDGE_FACE_LEFT |
             RTT_COL_EDGE_FACE_RIGHT;
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTGEOM *qp = rtpoint_as_rtgeom(iface->ctx, pt);

  id = rtt_be_getFaceContainingPoint(topo, pt);
  if ( id == -2 ) {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }

  if ( id > 0 )
  {
    return id;
  }
  id = 0; /* or it'll be -1 for not found */

  RTDEBUG(iface->ctx, 1, "No face properly contains query point,"
             " looking for edges");

  /* Not in a face, may be in universe or on edge, let's check
   * for distance */
  /* NOTE: we never pass a tolerance of 0 to avoid ever using
   * ST_Within, which doesn't include endpoints matches */
  elem = rtt_be_getEdgeWithinDistance2D(topo, pt, tol?tol:1e-5, &num, flds, 0);
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  for (i=0; i<num; ++i)
  {
    RTT_ISO_EDGE *e = &(elem[i]);
    RTT_ELEMID eface = 0;
    RTGEOM* geom;
    double dist;

    if ( ! e->geom )
    {
      rtt_release_edges(iface->ctx, elem, num);
      rtnotice(iface->ctx, "Corrupted topology: edge %" RTTFMT_ELEMID
               " has null geometry", e->edge_id);
      continue;
    }

    /* don't consider dangling edges */
    if ( e->face_left == e->face_right )
    {
      RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID
                  " is dangling, won't consider it", e->edge_id);
      continue;
    }

    geom = rtline_as_rtgeom(iface->ctx, e->geom);
    dist = rtgeom_mindistance2d_tolerance(iface->ctx, geom, qp, tol);

    RTDEBUGF(iface->ctx, 1, "Distance from edge %" RTTFMT_ELEMID
                " is %g (tol=%g)", e->edge_id, dist, tol);

    /* we won't consider edges too far */
    if ( dist > tol ) continue;
    if ( e->face_left == 0 ) {
      eface = e->face_right;
    }
    else if ( e->face_right == 0 ) {
      eface = e->face_left;
    }
    else {
      rtt_release_edges(iface->ctx, elem, num);
      rterror(iface->ctx, "Two or more faces found");
      return -1;
    }

    if ( id && id != eface )
    {
      rtt_release_edges(iface->ctx, elem, num);
      rterror(iface->ctx, "Two or more faces found"
#if 0 /* debugging */
              " (%" RTTFMT_ELEMID
              " and %" RTTFMT_ELEMID ")", id, eface
#endif
             );
      return -1;
    }
    else id = eface;
  }
  if ( num ) rtt_release_edges(iface->ctx, elem, num);

  return id;
}

/* Return the smallest delta that can perturbate
 * the maximum absolute value of a geometry ordinate
 */
static double
_rtt_minTolerance(const RTCTX *ctx,  RTGEOM *g )
{
  const RTGBOX* gbox;
  double max;
  double ret;

  gbox = rtgeom_get_bbox(ctx, g);
  if ( ! gbox ) return 0; /* empty */
  max = FP_ABS(gbox->xmin);
  if ( max < FP_ABS(gbox->xmax) ) max = FP_ABS(gbox->xmax);
  if ( max < FP_ABS(gbox->ymin) ) max = FP_ABS(gbox->ymin);
  if ( max < FP_ABS(gbox->ymax) ) max = FP_ABS(gbox->ymax);

  ret = 3.6 * pow(10,  - ( 15 - log10(max?max:1.0) ) );

  return ret;
}

#define _RTT_MINTOLERANCE( topo, geom ) ( \
  topo->precision >= 0 ?  topo->precision : _rtt_minTolerance(topo->be_iface->ctx, geom) )

typedef struct scored_pointer_t {
  void *ptr;
  double score;
} scored_pointer;

static int
compare_scored_pointer(const void *si1, const void *si2)
{
  double a = ((scored_pointer *)si1)->score;
  double b = ((scored_pointer *)si2)->score;
  if ( a < b )
    return -1;
  else if ( a > b )
    return 1;
  else
    return 0;
}

/*
 * @param findFace if non-zero the code will determine which face
 *        contains the given point (unless it is known to be NOT
 *        isolated)
 */
static RTT_ELEMID
_rtt_AddPoint(RTT_TOPOLOGY* topo, RTPOINT* point, double tol, int findFace)
{
  int num, i;
  double mindist = FLT_MAX;
  RTT_ISO_NODE *nodes, *nodes2;
  RTT_ISO_EDGE *edges, *edges2;
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTGEOM *pt = rtpoint_as_rtgeom(iface->ctx, point);
  int flds;
  RTT_ELEMID id = 0;
  scored_pointer *sorted;

  /* Get tolerance, if -1 was given */
  if ( tol == -1 ) tol = _RTT_MINTOLERANCE( topo, pt );

  RTDEBUGG(iface->ctx, 1, pt, "Adding point");

  /*
  -- 1. Check if any existing node is closer than the given precision
  --    and if so pick the closest
  TODO: use WithinBox2D
  */
  flds = RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM;
  nodes = rtt_be_getNodeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
    RTDEBUGF(iface->ctx, 1, "New point is within %.15g units of %d nodes", tol, num);
    /* Order by distance if there are more than a single return */
    if ( num > 1 )
    {{
      sorted= rtalloc(iface->ctx, sizeof(scored_pointer)*num);
      for (i=0; i<num; ++i)
      {
        sorted[i].ptr = nodes+i;
        sorted[i].score = rtgeom_mindistance2d(iface->ctx, rtpoint_as_rtgeom(iface->ctx, nodes[i].geom), pt);
        RTDEBUGF(iface->ctx, 1, "Node %" RTTFMT_ELEMID " distance: %.15g",
          ((RTT_ISO_NODE*)(sorted[i].ptr))->node_id, sorted[i].score);
      }
      qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
      nodes2 = rtalloc(iface->ctx, sizeof(RTT_ISO_NODE)*num);
      for (i=0; i<num; ++i)
      {
        nodes2[i] = *((RTT_ISO_NODE*)sorted[i].ptr);
      }
      rtfree(iface->ctx, sorted);
      rtfree(iface->ctx, nodes);
      nodes = nodes2;
    }}

    for ( i=0; i<num; ++i )
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      RTGEOM *g = rtpoint_as_rtgeom(iface->ctx, n->geom);
      double dist = rtgeom_mindistance2d(iface->ctx, g, pt);
      /* TODO: move this check in the previous sort scan ... */
      /* must be closer than tolerated, unless distance is zero */
      if ( dist && dist >= tol ) continue;
      if ( ! id || dist < mindist )
      {
        id = n->node_id;
        mindist = dist;
      }
    }
    if ( id )
    {
      /* found an existing node */
      if ( nodes ) _rtt_release_nodes(iface->ctx, nodes, num);
      return id;
    }
  }

  _rtt_EnsureGeos(iface->ctx);

  /*
  -- 2. Check if any existing edge falls within tolerance
  --    and if so split it by a point projected on it
  TODO: use WithinBox2D
  */
  flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM;
  edges = rtt_be_getEdgeWithinDistance2D(topo, point, tol, &num, flds, 0);
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
  RTDEBUGF(iface->ctx, 1, "New point is within %.15g units of %d edges", tol, num);

  /* Order by distance if there are more than a single return */
  if ( num > 1 )
  {{
    int j;
    sorted = rtalloc(iface->ctx, sizeof(scored_pointer)*num);
    for (i=0; i<num; ++i)
    {
      sorted[i].ptr = edges+i;
      sorted[i].score = rtgeom_mindistance2d(iface->ctx, rtline_as_rtgeom(iface->ctx, edges[i].geom), pt);
      RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID " distance: %.15g",
        ((RTT_ISO_EDGE*)(sorted[i].ptr))->edge_id, sorted[i].score);
    }
    qsort(sorted, num, sizeof(scored_pointer), compare_scored_pointer);
    edges2 = rtalloc(iface->ctx, sizeof(RTT_ISO_EDGE)*num);
    for (j=0, i=0; i<num; ++i)
    {
      if ( sorted[i].score == sorted[0].score )
      {
        edges2[j++] = *((RTT_ISO_EDGE*)sorted[i].ptr);
      }
      else
      {
        rtline_free(iface->ctx, ((RTT_ISO_EDGE*)sorted[i].ptr)->geom);
      }
    }
    num = j;
    rtfree(iface->ctx, sorted);
    rtfree(iface->ctx, edges);
    edges = edges2;
  }}

  for (i=0; i<num; ++i)
  {
    /* The point is on or near an edge, split the edge */
    RTT_ISO_EDGE *e = &(edges[i]);
    RTGEOM *g = rtline_as_rtgeom(iface->ctx, e->geom);
    RTGEOM *prj;
    int contains;
    GEOSGeometry *prjg, *gg;
    RTT_ELEMID edge_id = e->edge_id;

    RTDEBUGF(iface->ctx, 1, "Splitting edge %" RTTFMT_ELEMID, edge_id);

    /* project point to line, split edge by point */
    prj = rtgeom_closest_point(iface->ctx, g, pt);
    if ( rtgeom_has_z(iface->ctx, pt) )
    {{
      /*
      -- This is a workaround for ClosestPoint lack of Z support:
      -- http://trac.osgeo.org/postgis/ticket/2033
      */
      RTGEOM *tmp;
      double z;
      RTPOINT4D p4d;
      RTPOINT *prjpt;
      /* add Z to "prj" */
      tmp = rtgeom_force_3dz(iface->ctx, prj);
      prjpt = rtgeom_as_rtpoint(iface->ctx, tmp);
      rt_getPoint4d_p(iface->ctx, point->point, 0, &p4d);
      z = p4d.z;
      rt_getPoint4d_p(iface->ctx, prjpt->point, 0, &p4d);
      p4d.z = z;
      ptarray_set_point4d(iface->ctx, prjpt->point, 0, &p4d);
      rtgeom_free(iface->ctx, prj);
      prj = tmp;
    }}
    prjg = RTGEOM2GEOS(iface->ctx, prj, 0);
    if ( ! prjg ) {
      rtgeom_free(iface->ctx, prj);
      rtt_release_edges(iface->ctx, edges, num);
      rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }
    gg = RTGEOM2GEOS(iface->ctx, g, 0);
    if ( ! gg ) {
      rtgeom_free(iface->ctx, prj);
      rtt_release_edges(iface->ctx, edges, num);
      GEOSGeom_destroy_r(iface->ctx->gctx, prjg);
      rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }
    contains = GEOSContains_r(iface->ctx->gctx, gg, prjg);
    GEOSGeom_destroy_r(iface->ctx->gctx, prjg);
    GEOSGeom_destroy_r(iface->ctx->gctx, gg);
    if ( contains == 2 )
    {
      rtgeom_free(iface->ctx, prj);
      rtt_release_edges(iface->ctx, edges, num);
      rterror(iface->ctx, "GEOS exception on Contains: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }
    if ( ! contains )
    {{
      double snaptol;
      RTGEOM *snapedge;
      RTLINE *snapline;
      RTPOINT4D p1, p2;

      RTDEBUGF(iface->ctx, 1, "Edge %" RTTFMT_ELEMID
                  " does not contain projected point to it",
                  edge_id);

      /* In order to reduce the robustness issues, we'll pick
       * an edge that contains the projected point, if possible */
      if ( i+1 < num )
      {
        RTDEBUG(iface->ctx, 1, "But there's another to check");
        rtgeom_free(iface->ctx, prj);
        continue;
      }

      /*
      -- The tolerance must be big enough for snapping to happen
      -- and small enough to snap only to the projected point.
      -- Unfortunately ST_Distance returns 0 because it also uses
      -- a projected point internally, so we need another way.
      */
      snaptol = _rtt_minTolerance(iface->ctx, prj);
      snapedge = _rtt_toposnap(iface->ctx, g, prj, snaptol);
      snapline = rtgeom_as_rtline(iface->ctx, snapedge);

      RTDEBUGF(iface->ctx, 1, "Edge snapped with tolerance %g", snaptol);

      /* TODO: check if snapping did anything ? */
#if RTGEOM_DEBUG_LEVEL > 0
      {
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(iface->ctx, g, RTWKT_EXTENDED, 15, &sz);
      char *wkt2 = rtgeom_to_wkt(iface->ctx, snapedge, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(iface->ctx, 1, "Edge %s snapped became %s", wkt1, wkt2);
      rtfree(iface->ctx, wkt1);
      rtfree(iface->ctx, wkt2);
      }
#endif


      /*
      -- Snapping currently snaps the first point below tolerance
      -- so may possibly move first point. See ticket #1631
      */
      rt_getPoint4d_p(iface->ctx, e->geom->points, 0, &p1);
      rt_getPoint4d_p(iface->ctx, snapline->points, 0, &p2);
      RTDEBUGF(iface->ctx, 1, "Edge first point is %g %g, "
                  "snapline first point is %g %g",
                  p1.x, p1.y, p2.x, p2.y);
      if ( p1.x != p2.x || p1.y != p2.y )
      {
        RTDEBUG(iface->ctx, 1, "Snapping moved first point, re-adding it");
        if ( RT_SUCCESS != ptarray_insert_point(iface->ctx, snapline->points, &p1, 0) )
        {
          rtgeom_free(iface->ctx, prj);
          rtgeom_free(iface->ctx, snapedge);
          rtt_release_edges(iface->ctx, edges, num);
          rterror(iface->ctx, "GEOS exception on Contains: %s", rtgeom_get_last_geos_error(iface->ctx));
          return -1;
        }
#if RTGEOM_DEBUG_LEVEL > 0
        {
        size_t sz;
        char *wkt1 = rtgeom_to_wkt(iface->ctx, g, RTWKT_EXTENDED, 15, &sz);
        RTDEBUGF(iface->ctx, 1, "Tweaked snapline became %s", wkt1);
        rtfree(iface->ctx, wkt1);
        }
#endif
      }
#if RTGEOM_DEBUG_LEVEL > 0
      else {
        RTDEBUG(iface->ctx, 1, "Snapping did not move first point");
      }
#endif

      if ( -1 == rtt_ChangeEdgeGeom( topo, edge_id, snapline ) )
      {
        /* TODO: should have invoked rterror already, leaking memory */
        rtgeom_free(iface->ctx, prj);
        rtgeom_free(iface->ctx, snapedge);
        rtt_release_edges(iface->ctx, edges, num);
        rterror(iface->ctx, "rtt_ChangeEdgeGeom failed");
        return -1;
      }
      rtgeom_free(iface->ctx, snapedge);
    }}
#if RTGEOM_DEBUG_LEVEL > 0
    else
    {{
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(iface->ctx, g, RTWKT_EXTENDED, 15, &sz);
      char *wkt2 = rtgeom_to_wkt(iface->ctx, prj, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(iface->ctx, 1, "Edge %s contains projected point %s", wkt1, wkt2);
      rtfree(iface->ctx, wkt1);
      rtfree(iface->ctx, wkt2);
    }}
#endif

    /* TODO: pass 1 as last argument (skipChecks) ? */
    id = rtt_ModEdgeSplit( topo, edge_id, rtgeom_as_rtpoint(iface->ctx, prj), 0 );
    if ( -1 == id )
    {
      /* TODO: should have invoked rterror already, leaking memory */
      rtgeom_free(iface->ctx, prj);
      rtt_release_edges(iface->ctx, edges, num);
      rterror(iface->ctx, "rtt_ModEdgeSplit failed");
      return -1;
    }

    rtgeom_free(iface->ctx, prj);

    /*
     * TODO: decimate the two new edges with the given tolerance ?
     *
     * the edge identifiers to decimate would be: edge_id and "id"
     * The problem here is that decimation of existing edges
     * may introduce intersections or topological inconsistencies,
     * for example:
     *
     *  - A node may end up falling on the other side of the edge
     *  - The decimated edge might intersect another existing edge
     *
     */

    break; /* we only want to snap a single edge */
  }
  rtt_release_edges(iface->ctx, edges, num);
  }
  else
  {
    /* The point is isolated, add it as such */
    /* TODO: pass 1 as last argument (skipChecks) ? */
    id = _rtt_AddIsoNode(topo, -1, point, 0, findFace);
    if ( -1 == id )
    {
      /* should have invoked rterror already, leaking memory */
      rterror(iface->ctx, "rtt_AddIsoNode failed");
      return -1;
    }
  }

  return id;
}

RTT_ELEMID
rtt_AddPoint(RTT_TOPOLOGY* topo, RTPOINT* point, double tol)
{
  return _rtt_AddPoint( topo, point, tol, 1 );
}

/* Return identifier of an equal edge, 0 if none or -1 on error
 * (and rterror gets called on error)
 */
static RTT_ELEMID
_rtt_GetEqualEdge( RTT_TOPOLOGY *topo, RTLINE *edge )
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTT_ELEMID id;
  RTT_ISO_EDGE *edges;
  int num, i;
  const RTGBOX *qbox = rtgeom_get_bbox(iface->ctx,  rtline_as_rtgeom(iface->ctx, edge) );
  GEOSGeometry *edgeg;
  const int flds = RTT_COL_EDGE_EDGE_ID|RTT_COL_EDGE_GEOM;

  edges = rtt_be_getEdgeWithinBox2D( topo, qbox, &num, flds, 0 );
  if ( num == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( num )
  {
    _rtt_EnsureGeos(iface->ctx);

    edgeg = RTGEOM2GEOS(iface->ctx,  rtline_as_rtgeom(iface->ctx, edge), 0 );
    if ( ! edgeg )
    {
      rtt_release_edges(iface->ctx, edges, num);
      rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
      return -1;
    }
    for (i=0; i<num; ++i)
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      RTGEOM *g = rtline_as_rtgeom(iface->ctx, e->geom);
      GEOSGeometry *gg;
      int equals;
      gg = RTGEOM2GEOS(iface->ctx,  g, 0 );
      if ( ! gg )
      {
        GEOSGeom_destroy_r(iface->ctx->gctx, edgeg);
        rtt_release_edges(iface->ctx, edges, num);
        rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -1;
      }
      equals = GEOSEquals_r(iface->ctx->gctx, gg, edgeg);
      GEOSGeom_destroy_r(iface->ctx->gctx, gg);
      if ( equals == 2 )
      {
        GEOSGeom_destroy_r(iface->ctx->gctx, edgeg);
        rtt_release_edges(iface->ctx, edges, num);
        rterror(iface->ctx, "GEOSEquals exception: %s", rtgeom_get_last_geos_error(iface->ctx));
        return -1;
      }
      if ( equals )
      {
        id = e->edge_id;
        GEOSGeom_destroy_r(iface->ctx->gctx, edgeg);
        rtt_release_edges(iface->ctx, edges, num);
        return id;
      }
    }
    GEOSGeom_destroy_r(iface->ctx->gctx, edgeg);
    rtt_release_edges(iface->ctx, edges, num);
  }

  return 0;
}

/*
 * Add a pre-noded pre-split line edge. Used by rtt_AddLine
 * Return edge id, 0 if none added (empty edge), -1 on error
 *
 * @param handleFaceSplit if non-zero the code will check
 *        if the newly added edge would split a face and if so
 *        would create new faces accordingly. Otherwise it will
 *        set left_face and right_face to null (-1)
 *
 */
static RTT_ELEMID
_rtt_AddLineEdge( RTT_TOPOLOGY* topo, RTLINE* edge, double tol,
                  int handleFaceSplit )
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTCOLLECTION *col;
  RTPOINT *start_point, *end_point;
  RTGEOM *tmp, *tmp2;
  RTT_ISO_NODE *node;
  RTT_ELEMID nid[2]; /* start_node, end_node */
  RTT_ELEMID id; /* edge id */
  RTPOINT4D p4d;
  int nn, i;

  RTDEBUGG(iface->ctx, 1, rtline_as_rtgeom(iface->ctx, edge), "_lwtAddLineEdge");
  RTDEBUGF(iface->ctx, 1, "_lwtAddLineEdge with tolerance %g", tol);

  start_point = rtline_get_rtpoint(iface->ctx, edge, 0);
  if ( ! start_point )
  {
    rtnotice(iface->ctx, "Empty component of noded line");
    return 0; /* must be empty */
  }
  nid[0] = _rtt_AddPoint( topo, start_point, tol, handleFaceSplit );
  rtpoint_free(iface->ctx, start_point); /* too late if rtt_AddPoint calls rterror */
  if ( nid[0] == -1 ) return -1; /* rterror should have been called */

  end_point = rtline_get_rtpoint(iface->ctx, edge, edge->points->npoints-1);
  if ( ! end_point )
  {
    rterror(iface->ctx, "could not get last point of line "
            "after successfully getting first point !?");
    return -1;
  }
  nid[1] = _rtt_AddPoint( topo, end_point, tol, handleFaceSplit );
  rtpoint_free(iface->ctx, end_point); /* too late if rtt_AddPoint calls rterror */
  if ( nid[1] == -1 ) return -1; /* rterror should have been called */

  /*
    -- Added endpoints may have drifted due to tolerance, so
    -- we need to re-snap the edge to the new nodes before adding it
  */

  nn = nid[0] == nid[1] ? 1 : 2;
  node = rtt_be_getNodeById( topo, nid, &nn,
                             RTT_COL_NODE_NODE_ID|RTT_COL_NODE_GEOM );
  if ( nn == -1 )
  {
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  start_point = NULL; end_point = NULL;
  for (i=0; i<nn; ++i)
  {
    if ( node[i].node_id == nid[0] ) start_point = node[i].geom;
    if ( node[i].node_id == nid[1] ) end_point = node[i].geom;
  }
  if ( ! start_point  || ! end_point )
  {
    if ( nn ) _rtt_release_nodes(iface->ctx, node, nn);
    rterror(iface->ctx, "Could not find just-added nodes % " RTTFMT_ELEMID
            " and %" RTTFMT_ELEMID, nid[0], nid[1]);
    return -1;
  }

  /* snap */

  rt_getPoint4d_p(iface->ctx,  start_point->point, 0, &p4d );
  rtline_setPoint4d(iface->ctx, edge, 0, &p4d);

  rt_getPoint4d_p(iface->ctx,  end_point->point, 0, &p4d );
  rtline_setPoint4d(iface->ctx, edge, edge->points->npoints-1, &p4d);

  if ( nn ) _rtt_release_nodes(iface->ctx, node, nn);

  /* make valid, after snap (to handle collapses) */
  tmp = rtgeom_make_valid(iface->ctx, rtline_as_rtgeom(iface->ctx, edge));

  col = rtgeom_as_rtcollection(iface->ctx, tmp);
  if ( col )
  {{

    col = rtcollection_extract(iface->ctx, col, RTLINETYPE);

    /* Check if the so-snapped edge collapsed (see #1650) */
    if ( col->ngeoms == 0 )
    {
      rtcollection_free(iface->ctx, col);
      rtgeom_free(iface->ctx, tmp);
      RTDEBUG(iface->ctx, 1, "Made-valid snapped edge collapsed");
      return 0;
    }

    tmp2 = rtgeom_clone_deep(iface->ctx,  col->geoms[0] );
    rtgeom_free(iface->ctx, tmp);
    tmp = tmp2;
    edge = rtgeom_as_rtline(iface->ctx, tmp);
    rtcollection_free(iface->ctx, col);
    if ( ! edge )
    {
      /* should never happen */
      rterror(iface->ctx, "rtcollection_extract(iface->ctx, RTLINETYPE) returned a non-line?");
      return -1;
    }
  }}
  else
  {
    edge = rtgeom_as_rtline(iface->ctx, tmp);
    if ( ! edge )
    {
      RTDEBUGF(iface->ctx, 1, "Made-valid snapped edge collapsed to %s",
                  rttype_name(iface->ctx, rtgeom_get_type(iface->ctx, tmp)));
      rtgeom_free(iface->ctx, tmp);
      return 0;
    }
  }

  /* check if the so-snapped edge _now_ exists */
  id = _rtt_GetEqualEdge ( topo, edge );
  RTDEBUGF(iface->ctx, 1, "_rtt_GetEqualEdge returned %" RTTFMT_ELEMID, id);
  if ( id == -1 )
  {
    rtgeom_free(iface->ctx, tmp); /* probably too late, due to internal rterror */
    return -1;
  }
  if ( id )
  {
    rtgeom_free(iface->ctx, tmp); /* possibly takes "edge" down with it */
    return id;
  }

  /* No previously existing edge was found, we'll add one */

  /* Remove consecutive vertices below given tolerance
   * on edge addition */
  if ( tol )
  {{
    tmp2 = rtline_remove_repeated_points(iface->ctx, edge, tol);
    RTDEBUGG(iface->ctx, 1, tmp2, "Repeated-point removed");
    edge = rtgeom_as_rtline(iface->ctx, tmp2);
    rtgeom_free(iface->ctx, tmp);
    tmp = tmp2;

    /* check if the so-decimated edge _now_ exists */
    id = _rtt_GetEqualEdge ( topo, edge );
    RTDEBUGF(iface->ctx, 1, "_rtt_GetEqualEdge returned %" RTTFMT_ELEMID, id);
    if ( id == -1 )
    {
      rtgeom_free(iface->ctx, tmp); /* probably too late, due to internal lwerror */
      return -1;
    }
    if ( id )
    {
      rtgeom_free(iface->ctx, tmp); /* takes "edge" down with it */
      return id;
    }
  }}

  /* TODO: skip checks ? */
  id = _rtt_AddEdge( topo, nid[0], nid[1], edge, 0, handleFaceSplit ?  1 : -1 );
  RTDEBUGF(iface->ctx, 1, "rtt_AddEdgeModFace returned %" RTTFMT_ELEMID, id);
  if ( id == -1 )
  {
    rtgeom_free(iface->ctx, tmp); /* probably too late, due to internal rterror */
    return -1;
  }
  rtgeom_free(iface->ctx, tmp); /* possibly takes "edge" down with it */

  return id;
}

/* Simulate split-loop as it was implemented in pl/pgsql version
 * of TopoGeo_addLinestring */
static RTGEOM *
_rtt_split_by_nodes(const RTCTX *ctx, const RTGEOM *g, const RTGEOM *nodes)
{
  RTCOLLECTION *col = rtgeom_as_rtcollection(ctx, nodes);
  int i;
  RTGEOM *bg;

  bg = rtgeom_clone_deep(ctx, g);
  if ( ! col->ngeoms ) return bg;

  for (i=0; i<col->ngeoms; ++i)
  {
    RTGEOM *g2;
    g2 = rtgeom_split(ctx, bg, col->geoms[i]);
    rtgeom_free(ctx, bg);
    bg = g2;
  }
  bg->srid = nodes->srid;

  return bg;
}

/*
 * @param handleFaceSplit if non-zero the code will check
 *        if the newly added edge would split a face and if so
 *        would create new faces accordingly. Otherwise it will
 *        set left_face and right_face to null (-1)
 */
static RTT_ELEMID*
_rtt_AddLine(RTT_TOPOLOGY* topo, RTLINE* line, double tol, int* nedges,
             int handleFaceSplit)
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  RTGEOM *geomsbuf[1];
  RTGEOM **geoms;
  int ngeoms;
  RTGEOM *noded, *tmp;
  RTCOLLECTION *col;
  RTT_ELEMID *ids;
  RTT_ISO_EDGE *edges;
  RTT_ISO_NODE *nodes;
  int num;
  int i;
  RTGBOX qbox;

  *nedges = -1; /* error condition, by default */

  /* Get tolerance, if -1 was given */
  if ( tol == -1 ) tol = _RTT_MINTOLERANCE( topo, (RTGEOM*)line );
  RTDEBUGF(iface->ctx, 1, "Working tolerance:%.15g", tol);
  RTDEBUGF(iface->ctx, 1, "Input line has srid=%d", line->srid);

  /* Remove consecutive vertices below given tolerance upfront */
  if ( tol )
  {{
    RTLINE *clean = rtgeom_as_rtline(iface->ctx, rtline_remove_repeated_points(iface->ctx, line, tol));
    tmp = rtline_as_rtgeom(iface->ctx, clean); /* NOTE: might collapse to non-simple */
    RTDEBUGG(iface->ctx, 1, tmp, "Repeated-point removed");
  }} else tmp=(RTGEOM*)line;

  /* 1. Self-node */
  noded = rtgeom_node(iface->ctx, (RTGEOM*)tmp);
  if ( tmp != (RTGEOM*)line ) rtgeom_free(iface->ctx, tmp);
  if ( ! noded ) return NULL; /* should have called rterror already */
  RTDEBUGG(iface->ctx, 1, noded, "Noded");

  qbox = *rtgeom_get_bbox(iface->ctx,  rtline_as_rtgeom(iface->ctx, line) );
  RTDEBUGF(iface->ctx, 1, "Line BOX is %.15g %.15g, %.15g %.15g", qbox.xmin, qbox.ymin,
                                          qbox.xmax, qbox.ymax);
  gbox_expand(iface->ctx, &qbox, tol);
  RTDEBUGF(iface->ctx, 1, "BOX expanded by %g is %.15g %.15g, %.15g %.15g",
              tol, qbox.xmin, qbox.ymin, qbox.xmax, qbox.ymax);

  /* 2. Node to edges falling within tol distance */
  edges = rtt_be_getEdgeWithinBox2D( topo, &qbox, &num, RTT_COL_EDGE_ALL, 0 );
  if ( num == -1 )
  {
    rtgeom_free(iface->ctx, noded);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }
  RTDEBUGF(iface->ctx, 1, "Line bbox intersects %d edges bboxes", num);
  if ( num )
  {{
    /* collect those whose distance from us is < tol */
    RTGEOM **nearby = rtalloc(iface->ctx, sizeof(RTGEOM *)*num);
    int nn=0;
    for (i=0; i<num; ++i)
    {
      RTT_ISO_EDGE *e = &(edges[i]);
      RTGEOM *g = rtline_as_rtgeom(iface->ctx, e->geom);
      double dist = rtgeom_mindistance2d(iface->ctx, g, noded);
      /* must be closer than tolerated, unless distance is zero */
      if ( dist && dist >= tol ) continue;
      nearby[nn++] = g;
    }
    if ( nn )
    {{
      RTCOLLECTION *col;
      RTGEOM *iedges; /* just an alias for col */
      RTGEOM *snapped;
      RTGEOM *set1, *set2;

      RTDEBUGF(iface->ctx, 1, "Line intersects %d edges", nn);

      col = rtcollection_construct(iface->ctx, RTCOLLECTIONTYPE, topo->srid,
                                   NULL, nn, nearby);
      iedges = rtcollection_as_rtgeom(iface->ctx, col);
      RTDEBUGG(iface->ctx, 1, iedges, "Collected edges");
      RTDEBUGF(iface->ctx, 1, "Snapping noded, with srid=%d "
                  "to interesecting edges, with srid=%d",
                  noded->srid, iedges->srid);
      snapped = _rtt_toposnap(iface->ctx, noded, iedges, tol);
      rtgeom_free(iface->ctx, noded);
      RTDEBUGG(iface->ctx, 1, snapped, "Snapped");
      RTDEBUGF(iface->ctx, 1, "Diffing snapped, with srid=%d "
                  "and interesecting edges, with srid=%d",
                  snapped->srid, iedges->srid);
      noded = rtgeom_difference(iface->ctx, snapped, iedges);
      RTDEBUGG(iface->ctx, 1, noded, "Differenced");
      RTDEBUGF(iface->ctx, 1, "Intersecting snapped, with srid=%d "
                  "and interesecting edges, with srid=%d",
                  snapped->srid, iedges->srid);
      set1 = rtgeom_intersection(iface->ctx, snapped, iedges);
      RTDEBUGG(iface->ctx, 1, set1, "Intersected");
      rtgeom_free(iface->ctx, snapped);
      RTDEBUGF(iface->ctx, 1, "Linemerging set1, with srid=%d", set1->srid);
      set2 = rtgeom_linemerge(iface->ctx, set1);
      RTDEBUGG(iface->ctx, 1, set2, "Linemerged");
      RTDEBUGG(iface->ctx, 1, noded, "Noded");
      rtgeom_free(iface->ctx, set1);
      RTDEBUGF(iface->ctx, 1, "Unioning noded, with srid=%d "
                  "and set2, with srid=%d", noded->srid, set2->srid);
      set1 = rtgeom_union(iface->ctx, noded, set2);
      rtgeom_free(iface->ctx, set2);
      rtgeom_free(iface->ctx, noded);
      noded = set1;
      RTDEBUGG(iface->ctx, 1, set1, "Unioned");

      /* will not release the geoms array */
      rtcollection_release(iface->ctx, col);
    }}
    rtfree(iface->ctx, nearby);
    rtt_release_edges(iface->ctx, edges, num);
  }}

  /* 2.1. Node with existing nodes within tol
   * TODO: check if we should be only considering _isolated_ nodes! */
  nodes = rtt_be_getNodeWithinBox2D( topo, &qbox, &num, RTT_COL_NODE_ALL, 0 );
  if ( num == -1 )
  {
    rtgeom_free(iface->ctx, noded);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }
  RTDEBUGF(iface->ctx, 1, "Line bbox intersects %d nodes bboxes", num);
  if ( num )
  {{
    /* collect those whose distance from us is < tol */
    RTGEOM **nearby = rtalloc(iface->ctx, sizeof(RTGEOM *)*num);
    int nn=0;
    for (i=0; i<num; ++i)
    {
      RTT_ISO_NODE *n = &(nodes[i]);
      RTGEOM *g = rtpoint_as_rtgeom(iface->ctx, n->geom);
      double dist = rtgeom_mindistance2d(iface->ctx, g, noded);
      /* must be closer than tolerated, unless distance is zero */
      if ( dist && dist >= tol ) continue;
      nearby[nn++] = g;
    }
    if ( nn )
    {{
      RTCOLLECTION *col;
      RTGEOM *inodes; /* just an alias for col */
      RTGEOM *tmp;

      RTDEBUGF(iface->ctx, 1, "Line intersects %d nodes", nn);

      col = rtcollection_construct(iface->ctx, RTMULTIPOINTTYPE, topo->srid,
                                   NULL, nn, nearby);
      inodes = rtcollection_as_rtgeom(iface->ctx, col);

      RTDEBUGG(iface->ctx, 1, inodes, "Collected nodes");

      /* TODO: consider snapping once against all elements
       *      (rather than once with edges and once with nodes) */
      tmp = _rtt_toposnap(iface->ctx, noded, inodes, tol);
      rtgeom_free(iface->ctx, noded);
      noded = tmp;
      RTDEBUGG(iface->ctx, 1, noded, "Node-snapped");

      tmp = _rtt_split_by_nodes(iface->ctx, noded, inodes);
          /* rtgeom_split(iface->ctx, noded, inodes); */
      rtgeom_free(iface->ctx, noded);
      noded = tmp;
      RTDEBUGG(iface->ctx, 1, noded, "Node-split");

      /* will not release the geoms array */
      rtcollection_release(iface->ctx, col);

      /*
      -- re-node to account for ST_Snap introduced self-intersections
      -- See http://trac.osgeo.org/postgis/ticket/1714
      -- TODO: consider running UnaryUnion once after all noding
      */
      tmp = rtgeom_unaryunion(iface->ctx, noded);
      rtgeom_free(iface->ctx, noded);
      noded = tmp;
      RTDEBUGG(iface->ctx, 1, noded, "Unary-unioned");

    }}
    rtfree(iface->ctx, nearby);
    _rtt_release_nodes(iface->ctx, nodes, num);
  }}

  RTDEBUGG(iface->ctx, 1, noded, "Finally-noded");

  /* 3. For each (now-noded) segment, insert an edge */
  col = rtgeom_as_rtcollection(iface->ctx, noded);
  if ( col )
  {
    RTDEBUG(iface->ctx, 1, "Noded line was a collection");
    geoms = col->geoms;
    ngeoms = col->ngeoms;
  }
  else
  {
    RTDEBUG(iface->ctx, 1, "Noded line was a single geom");
    geomsbuf[0] = noded;
    geoms = geomsbuf;
    ngeoms = 1;
  }

  RTDEBUGF(iface->ctx, 1, "Line was split into %d edges", ngeoms);

  /* TODO: refactor to first add all nodes (re-snapping edges if
   * needed) and then check all edges for existing already
   * ( so to save a DB scan for each edge to be added )
   */
  ids = rtalloc(iface->ctx, sizeof(RTT_ELEMID)*ngeoms);
  num = 0;
  for ( i=0; i<ngeoms; ++i )
  {
    RTT_ELEMID id;
    RTGEOM *g = geoms[i];
    g->srid = noded->srid;

#if RTGEOM_DEBUG_LEVEL > 0
    {
      size_t sz;
      char *wkt1 = rtgeom_to_wkt(iface->ctx, g, RTWKT_EXTENDED, 15, &sz);
      RTDEBUGF(iface->ctx, 1, "Component %d of split line is: %s", i, wkt1);
      rtfree(iface->ctx, wkt1);
    }
#endif

    id = _rtt_AddLineEdge( topo, rtgeom_as_rtline(iface->ctx, g), tol, handleFaceSplit );
    RTDEBUGF(iface->ctx, 1, "_rtt_AddLineEdge returned %" RTTFMT_ELEMID, id);
    if ( id < 0 )
    {
      rtgeom_free(iface->ctx, noded);
      rtfree(iface->ctx, ids);
      return NULL;
    }
    if ( ! id )
    {
      RTDEBUGF(iface->ctx, 1, "Component %d of split line collapsed", i);
      continue;
    }

    RTDEBUGF(iface->ctx, 1, "Component %d of split line is edge %" RTTFMT_ELEMID,
                  i, id);
    ids[num++] = id; /* TODO: skip duplicates */
  }

  RTDEBUGG(iface->ctx, 1, noded, "Noded before free");
  rtgeom_free(iface->ctx, noded);

  /* TODO: XXX remove duplicated ids if not done before */

  *nedges = num;
  return ids;
}

RTT_ELEMID*
rtt_AddLine(RTT_TOPOLOGY* topo, RTLINE* line, double tol, int* nedges)
{
  return _rtt_AddLine(topo, line, tol, nedges, 1);
}

/*
 * @return -1 on error (and report error),
 *          1 if faces beside the universal one exist
 *          0 otherwise
 */
static int
_rtt_CheckFacesExist(RTT_TOPOLOGY *topo)
{
  RTT_ISO_FACE *faces;
  int fields = RTT_COL_FACE_FACE_ID;
  int nelems = 1;
  RTGBOX qbox;
  const RTCTX *ctx = topo->be_iface->ctx;

  qbox.xmin = qbox.ymin = -DBL_MAX;
  qbox.xmax = qbox.ymax = DBL_MAX;
  faces = rtt_be_getFaceWithinBox2D( topo, &qbox, &nelems, fields, 1);
  if ( nelems == -1 ) {
    rterror(ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return -1;
  }
  if ( faces ) _rtt_release_faces(ctx, faces, nelems);
  return nelems;
}

RTT_ELEMID*
rtt_AddLineNoFace(RTT_TOPOLOGY* topo, RTLINE* line, double tol, int* nedges)
{
  /*
   Check if Topology already contains some Face
   (ignoring the Universal Face)
  */
  const RTT_BE_IFACE *iface = topo->be_iface;
  int numfaces = _rtt_CheckFacesExist(topo);
  if ( numfaces != 0 ) {
    if ( numfaces > 0 ) {
      /* Faces exist */
      rterror(iface->ctx, "rtt_AddLineNoFace - table <topo>Face is not empty.");
    }
    /* Backend error, message should have been printed already */
    return NULL;
  }

  return _rtt_AddLine(topo, line, tol, nedges, 0);
}

RTT_ELEMID*
rtt_AddPolygon(RTT_TOPOLOGY* topo, RTPOLY* poly, double tol, int* nfaces)
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  int i;
  *nfaces = -1; /* error condition, by default */
  int num;
  RTT_ISO_FACE *faces;
  int nfacesinbox;
  RTT_ELEMID *ids = NULL;
  RTGBOX qbox;
  const GEOSPreparedGeometry *ppoly;
  GEOSGeometry *polyg;

  /* Get tolerance, if -1 was given */
  if ( tol == -1 ) tol = _RTT_MINTOLERANCE( topo, (RTGEOM*)poly );
  RTDEBUGF(iface->ctx, 1, "Working tolerance:%.15g", tol);

  /* Add each ring as an edge */
  for ( i=0; i<poly->nrings; ++i )
  {
    RTLINE *line;
    RTPOINTARRAY *pa;
    RTT_ELEMID *eids;
    int nedges;

    pa = ptarray_clone(iface->ctx, poly->rings[i]);
    line = rtline_construct(iface->ctx, topo->srid, NULL, pa);
    eids = rtt_AddLine( topo, line, tol, &nedges );
    if ( nedges < 0 ) {
      /* probably too late as rtt_AddLine invoked rterror */
      rtline_free(iface->ctx, line);
      rterror(iface->ctx, "Error adding ring %d of polygon", i);
      return NULL;
    }
    rtline_free(iface->ctx, line);
    rtfree(iface->ctx, eids);
  }

  /*
  -- Find faces covered by input polygon
  -- NOTE: potential snapping changed polygon edges
  */
  qbox = *rtgeom_get_bbox(iface->ctx,  rtpoly_as_rtgeom(iface->ctx, poly) );
  gbox_expand(iface->ctx, &qbox, tol);
  faces = rtt_be_getFaceWithinBox2D( topo, &qbox, &nfacesinbox,
                                     RTT_COL_FACE_ALL, 0 );
  if ( nfacesinbox == -1 )
  {
    rtfree(iface->ctx, ids);
    rterror(iface->ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }

  num = 0;
  if ( nfacesinbox )
  {
    polyg = RTGEOM2GEOS(iface->ctx, rtpoly_as_rtgeom(iface->ctx, poly), 0);
    if ( ! polyg )
    {
      _rtt_release_faces(iface->ctx, faces, nfacesinbox);
      rterror(iface->ctx, "Could not convert poly geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
      return NULL;
    }
    ppoly = GEOSPrepare_r(iface->ctx->gctx, polyg);
    ids = rtalloc(iface->ctx, sizeof(RTT_ELEMID)*nfacesinbox);
    for ( i=0; i<nfacesinbox; ++i )
    {
      RTT_ISO_FACE *f = &(faces[i]);
      RTGEOM *fg;
      GEOSGeometry *fgg, *sp;
      int covers;

      /* check if a point on this face surface is covered by our polygon */
      fg = rtt_GetFaceGeometry( topo, f->face_id );
      if ( ! fg )
      {
        i = f->face_id; /* so we can destroy faces */
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, ppoly);
        GEOSGeom_destroy_r(iface->ctx->gctx, polyg);
        rtfree(iface->ctx, ids);
        _rtt_release_faces(iface->ctx, faces, nfacesinbox);
        rterror(iface->ctx, "Could not get geometry of face %" RTTFMT_ELEMID, i);
        return NULL;
      }
      /* check if a point on this face's surface is covered by our polygon */
      fgg = RTGEOM2GEOS(iface->ctx, fg, 0);
      rtgeom_free(iface->ctx, fg);
      if ( ! fgg )
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, ppoly);
        GEOSGeom_destroy_r(iface->ctx->gctx, polyg);
        _rtt_release_faces(iface->ctx, faces, nfacesinbox);
        rterror(iface->ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(iface->ctx));
        return NULL;
      }
      sp = GEOSPointOnSurface_r(iface->ctx->gctx, fgg);
      GEOSGeom_destroy_r(iface->ctx->gctx, fgg);
      if ( ! sp )
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, ppoly);
        GEOSGeom_destroy_r(iface->ctx->gctx, polyg);
        _rtt_release_faces(iface->ctx, faces, nfacesinbox);
        rterror(iface->ctx, "Could not find point on face surface: %s", rtgeom_get_last_geos_error(iface->ctx));
        return NULL;
      }
      covers = GEOSPreparedCovers_r(iface->ctx->gctx,  ppoly, sp );
      GEOSGeom_destroy_r(iface->ctx->gctx, sp);
      if (covers == 2)
      {
        GEOSPreparedGeom_destroy_r(iface->ctx->gctx, ppoly);
        GEOSGeom_destroy_r(iface->ctx->gctx, polyg);
        _rtt_release_faces(iface->ctx, faces, nfacesinbox);
        rterror(iface->ctx, "PreparedCovers error: %s", rtgeom_get_last_geos_error(iface->ctx));
        return NULL;
      }
      if ( ! covers )
      {
        continue; /* we're not composed by this face */
      }

      /* TODO: avoid duplicates ? */
      ids[num++] = f->face_id;
    }
    GEOSPreparedGeom_destroy_r(iface->ctx->gctx, ppoly);
    GEOSGeom_destroy_r(iface->ctx->gctx, polyg);
    _rtt_release_faces(iface->ctx, faces, nfacesinbox);
  }

  /* possibly 0 if non face's surface point was found
   * to be covered by input polygon */
  *nfaces = num;

  return ids;
}

/*
 *---- polygonizer
 */

/* An array of pointers to EDGERING structures */
typedef struct RTT_ISO_EDGE_TABLE_T {
  RTT_ISO_EDGE *edges;
  int size;
} RTT_ISO_EDGE_TABLE;

static int
compare_iso_edges_by_id(const void *si1, const void *si2)
{
	int a = ((RTT_ISO_EDGE *)si1)->edge_id;
	int b = ((RTT_ISO_EDGE *)si2)->edge_id;
	if ( a < b )
		return -1;
	else if ( a > b )
		return 1;
	else
		return 0;
}

static RTT_ISO_EDGE *
_rtt_getIsoEdgeById(RTT_ISO_EDGE_TABLE *tab, RTT_ELEMID id)
{
  RTT_ISO_EDGE key;
  key.edge_id = id;

  void *match = bsearch( &key, tab->edges, tab->size,
                     sizeof(RTT_ISO_EDGE),
                     compare_iso_edges_by_id);
  return match;
}

typedef struct RTT_EDGERING_ELEM_T {
  /* externally owned */
  RTT_ISO_EDGE *edge;
  /* 0 if false, 1 if true */
  int left;
} RTT_EDGERING_ELEM;

/* A ring of edges */
typedef struct RTT_EDGERING_T {
  /* Signed edge identifiers
   * positive ones are walked in their direction, negative ones
   * in the opposite direction */
  RTT_EDGERING_ELEM **elems;
  /* Number of edges in the ring */
  int size;
  int capacity;
  /* Bounding box of the ring */
  RTGBOX *env;
  /* Bounding box of the ring in GEOS format (for STRTree) */
  GEOSGeometry *genv;
} RTT_EDGERING;

#define RTT_EDGERING_INIT(c, a) { \
  (a)->size = 0; \
  (a)->capacity = 1; \
  (a)->elems = rtalloc((c), sizeof(RTT_EDGERING_ELEM *) * (a)->capacity); \
  (a)->env = NULL; \
  (a)->genv = NULL; \
}

#define RTT_EDGERING_PUSH(c, a, r) { \
  if ( (a)->size + 1 > (a)->capacity ) { \
    (a)->capacity *= 2; \
    (a)->elems = rtrealloc((c), (a)->elems, sizeof(RTT_EDGERING_ELEM *) * (a)->capacity); \
  } \
  /* rtdebug(1, "adding elem %d (%p) of edgering %p", (a)->size, (r), (a)); */ \
  (a)->elems[(a)->size++] = (r); \
}

#define RTT_EDGERING_CLEAN(c, a) { \
  int i; for (i=0; i<(a)->size; ++i) { \
    if ( (a)->elems[i] ) { \
      /* rtdebug(1, "freeing elem %d (%p) of edgering %p", i, (a)->elems[i], (a)); */ \
      rtfree(ctx,(a)->elems[i]); \
    } \
  } \
  if ( (a)->elems ) { rtfree(ctx,(a)->elems); (a)->elems = NULL; } \
  (a)->size = 0; \
  (a)->capacity = 0; \
  if ( (a)->env ) { rtfree(ctx,(a)->env); (a)->env = NULL; } \
  if ( (a)->genv ) { GEOSGeom_destroy_r( (c)->gctx, (a)->genv); (a)->genv = NULL; } \
}

/* An array of pointers to EDGERING structures */
typedef struct RTT_EDGERING_ARRAY_T {
  RTT_EDGERING **rings;
  int size;
  int capacity;
  GEOSSTRtree* tree;
} RTT_EDGERING_ARRAY;

#define RTT_EDGERING_ARRAY_INIT(c, a) { \
  (a)->size = 0; \
  (a)->capacity = 1; \
  (a)->rings = rtalloc((c), sizeof(RTT_EDGERING *) * (a)->capacity); \
  (a)->tree = NULL; \
}

/* WARNING: use of 'j' is intentional, no to clash with
 * 'i' used in RTT_EDGERING_CLEAN */
#define RTT_EDGERING_ARRAY_CLEAN(c, a) { \
  int j; for (j=0; j<(a)->size; ++j) { \
    RTT_EDGERING_CLEAN((c), (a)->rings[j]); \
    rtfree( (c), (a)->rings[j]); \
  } \
  if ( (a)->capacity ) rtfree(ctx,(a)->rings); \
  if ( (a)->tree ) { \
    GEOSSTRtree_destroy_r( (c)->gctx, (a)->tree ); \
    (a)->tree = NULL; \
  } \
}

#define RTT_EDGERING_ARRAY_PUSH(c, a, r) { \
  if ( (a)->size + 1 > (a)->capacity ) { \
    (a)->capacity *= 2; \
    (a)->rings = rtrealloc((c), (a)->rings, sizeof(RTT_EDGERING *) * (a)->capacity); \
  } \
  (a)->rings[(a)->size++] = (r); \
}

typedef struct RTT_EDGERING_POINT_ITERATOR_T {
  RTT_EDGERING *ring;
  RTT_EDGERING_ELEM *curelem;
  int curelemidx;
  int curidx;
} RTT_EDGERING_POINT_ITERATOR;

static int
_rtt_EdgeRingIterator_next(const RTCTX *ctx, RTT_EDGERING_POINT_ITERATOR *it, RTPOINT2D *pt)
{
  RTT_EDGERING_ELEM *el = it->curelem;
  RTPOINTARRAY *pa;

  if ( ! el ) return 0; /* finished */

  pa = el->edge->geom->points;

  int tonext = 0;
  RTDEBUGF(ctx, 3, "iterator fetching idx %d from pa of %d points", it->curidx, pa->npoints);
  rt_getPoint2d_p(ctx,pa, it->curidx, pt);
  if ( el->left ) {
    it->curidx++;
    if ( it->curidx >= pa->npoints ) tonext = 1;
  } else {
    it->curidx--;
    if ( it->curidx < 0 ) tonext = 1;
  }

  if ( tonext )
  {
    RTDEBUG(ctx, 3, "iterator moving to next element");
    it->curelemidx++;
    if ( it->curelemidx < it->ring->size )
    {
      el = it->curelem = it->ring->elems[it->curelemidx];
      it->curidx = el->left ? 0 : el->edge->geom->points->npoints - 1;
    }
    else
    {
      it->curelem = NULL;
    }
  }

  return 1;
}

/* Release return with rtfree */
static RTT_EDGERING_POINT_ITERATOR *
_rtt_EdgeRingIterator_begin(const RTCTX *ctx, RTT_EDGERING *er)
{
  RTT_EDGERING_POINT_ITERATOR *ret = rtalloc(ctx, sizeof(RTT_EDGERING_POINT_ITERATOR));
  ret->ring = er;
  if ( er->size ) ret->curelem = er->elems[0];
  else ret->curelem = NULL;
  ret->curelemidx = 0;
  ret->curidx = ret->curelem->left ? 0 : ret->curelem->edge->geom->points->npoints - 1;
  return ret;
}

/* Identifier for a placeholder face that will be
 * used to mark hole rings */
#define RTT_HOLES_FACE_PLACEHOLDER INT32_MIN

static int
_rtt_FetchNextUnvisitedEdge(RTT_TOPOLOGY *topo, RTT_ISO_EDGE_TABLE *etab, int from)
{
  while (
    from < etab->size &&
    etab->edges[from].face_left != -1 &&
    etab->edges[from].face_right != -1
  ) from++;
  return from < etab->size ? from : -1;
}

static RTT_ISO_EDGE *
_rtt_FetchAllEdges(RTT_TOPOLOGY *topo, int *numedges)
{
  RTT_ISO_EDGE *edge;
  int fields = RTT_COL_EDGE_ALL;
  int nelems = 1;
  const RTCTX *ctx = topo->be_iface->ctx;

  edge = rtt_be_getEdgeWithinBox2D( topo, NULL, &nelems, fields, 0);
  *numedges = nelems;
  if ( nelems == -1 ) {
    rterror(ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
    return NULL;
  }
  return edge;
}

/* Update the side face of given ring edges
 *
 * Edge identifiers are signed, those with negative identifier
 * need to be updated their right_face, those with positive
 * identifier need to be updated their left_face.
 *
 * @param face identifier of the face bound by the ring
 * @return 0 on success, -1 on error
 */
static int
_rtt_UpdateEdgeRingSideFace(RTT_TOPOLOGY *topo, RTT_EDGERING *ring,
                            RTT_ELEMID face)
{
  RTT_ISO_EDGE *forward_edges = NULL;
  int forward_edges_count = 0;
  RTT_ISO_EDGE *backward_edges = NULL;
  int backward_edges_count = 0;
  int i, ret;
  const RTCTX *ctx = topo->be_iface->ctx;

  /* Make a list of forward_edges and backward_edges */

  forward_edges = rtalloc(ctx, sizeof(RTT_ISO_EDGE) * ring->size);
  forward_edges_count = 0;
  backward_edges = rtalloc(ctx, sizeof(RTT_ISO_EDGE) * ring->size);
  backward_edges_count = 0;

  for ( i=0; i<ring->size; ++i )
  {
    RTT_EDGERING_ELEM *elem = ring->elems[i];
    RTT_ISO_EDGE *edge = elem->edge;
    RTT_ELEMID id = edge->edge_id;
    if ( elem->left )
    {
      RTDEBUGF(ctx, 3, "Forward edge %d is %d", forward_edges_count, id);
      forward_edges[forward_edges_count].edge_id = id;
      forward_edges[forward_edges_count++].face_left = face;
      edge->face_left = face;
    }
    else
    {
      RTDEBUGF(ctx, 3, "Backward edge %d is %d", forward_edges_count, id);
      backward_edges[backward_edges_count].edge_id = id;
      backward_edges[backward_edges_count++].face_right = face;
      edge->face_right = face;
    }
  }

  /* Update forward edges */
  if ( forward_edges_count )
  {
    ret = rtt_be_updateEdgesById(topo, forward_edges,
                                 forward_edges_count,
                                 RTT_COL_EDGE_FACE_LEFT);
    if ( ret == -1 )
    {
      rtfree(ctx, forward_edges );
      rtfree(ctx, backward_edges );
      rterror(ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( ret != forward_edges_count )
    {
      rtfree(ctx, forward_edges );
      rtfree(ctx, backward_edges );
      rterror(ctx, "Unexpected error: %d edges updated when expecting %d (forward)",
              ret, forward_edges_count);
      return -1;
    }
  }

  /* Update backward edges */
  if ( backward_edges_count )
  {
    ret = rtt_be_updateEdgesById(topo, backward_edges,
                                 backward_edges_count,
                                 RTT_COL_EDGE_FACE_RIGHT);
    if ( ret == -1 )
    {
      rtfree(ctx, forward_edges );
      rtfree(ctx, backward_edges );
      rterror(ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( ret != backward_edges_count )
    {
      rtfree(ctx, forward_edges );
      rtfree(ctx, backward_edges );
      rterror(ctx, "Unexpected error: %d edges updated when expecting %d (backward)",
              ret, backward_edges_count);
      return -1;
    }
  }

  rtfree(ctx, forward_edges );
  rtfree(ctx, backward_edges );

  return 0;
}

/*
 * @param side 1 for left side, -1 for right side
 */
static RTT_EDGERING *
_rtt_BuildEdgeRing(RTT_TOPOLOGY *topo, RTT_ISO_EDGE_TABLE *edges,
                   RTT_ISO_EDGE *edge, int side)
{
  RTT_EDGERING *ring;
  RTT_EDGERING_ELEM *elem;
  RTT_ISO_EDGE *cur;
  int curside;
  const RTCTX *ctx = topo->be_iface->ctx;

  ring = rtalloc(ctx, sizeof(RTT_EDGERING));
  RTT_EDGERING_INIT(ctx, ring);

  cur = edge;
  curside = side;

  RTDEBUGF(ctx, 2, "Building rings for edge %d (side %d)", cur->edge_id, side);

  do {
    RTT_ELEMID next;

    elem = rtalloc(ctx, sizeof(RTT_EDGERING_ELEM));
    elem->edge = cur;
    elem->left = ( curside == 1 );

    /* Mark edge as "visited" */
    if ( elem->left ) cur->face_left = RTT_HOLES_FACE_PLACEHOLDER;
    else cur->face_right = RTT_HOLES_FACE_PLACEHOLDER;

    RTT_EDGERING_PUSH(ctx, ring, elem);
    next = elem->left ? cur->next_left : cur->next_right;

    RTDEBUGF(ctx, 3, " next edge is %d", next);

    if ( next > 0 ) curside = 1;
    else { curside = -1; next = -next; }
    cur = _rtt_getIsoEdgeById(edges, next);
    if ( ! cur )
    {
      RTT_EDGERING_CLEAN(ctx, ring);
      rterror(ctx, "Could not find edge with id %d", next);
      break;
    }
  } while (cur != edge || curside != side);

  RTDEBUGF(ctx, 1, "Ring for edge %d has %d elems", edge->edge_id*side, ring->size);

  return ring;
}

static double
_rtt_EdgeRingSignedArea(const RTCTX *ctx, RTT_EDGERING_POINT_ITERATOR *it)
{
	RTPOINT2D P1;
	RTPOINT2D P2;
	RTPOINT2D P3;
	double sum = 0.0;
	double x0, x, y1, y2;

  if ( ! _rtt_EdgeRingIterator_next(ctx, it, &P1) ) return 0.0;
  if ( ! _rtt_EdgeRingIterator_next(ctx, it, &P2) ) return 0.0;

  RTDEBUG(ctx, 2, "_rtt_EdgeRingSignedArea");

  x0 = P1.x;
  while ( _rtt_EdgeRingIterator_next(ctx, it, &P3)  )
  {
    x = P2.x - x0;
    y1 = P3.y;
    y2 = P1.y;
    sum += x * (y2-y1);

    /* Move forwards! */
    P1 = P2;
    P2 = P3;
  }

	return sum / 2.0;
}


/* Return 1 for true, 0 for false */
static int
_rtt_EdgeRingIsCCW(const RTCTX *ctx, RTT_EDGERING *ring)
{
  double sa;

  RTDEBUGF(ctx, 2, "_rtt_EdgeRingIsCCW, ring has %d elems", ring->size);
  RTT_EDGERING_POINT_ITERATOR *it = _rtt_EdgeRingIterator_begin(ctx, ring);
  sa = _rtt_EdgeRingSignedArea(ctx, it);
  RTDEBUGF(ctx, 2, "_rtt_EdgeRingIsCCW, signed area is %g", sa);
  rtfree(ctx,it);
  if ( sa >= 0 ) return 0;
  else return 1;
}

static int
_rtt_EdgeRingCrossingCount(const RTCTX *ctx, const RTPOINT2D *p, RTT_EDGERING_POINT_ITERATOR *it)
{
	int cn = 0;    /* the crossing number counter */
	RTPOINT2D v1, v2;
#ifndef RELAX
  RTPOINT2D v0;
#endif

  if ( ! _rtt_EdgeRingIterator_next(ctx, it, &v1) ) return cn;
  v0 = v1;
	while ( _rtt_EdgeRingIterator_next(ctx, it, &v2) )
	{
		double vt;

		/* edge from vertex i to vertex i+1 */
		if
		(
		    /* an upward crossing */
		    ((v1.y <= p->y) && (v2.y > p->y))
		    /* a downward crossing */
		    || ((v1.y > p->y) && (v2.y <= p->y))
		)
		{

			vt = (double)(p->y - v1.y) / (v2.y - v1.y);

			/* P->x <intersect */
			if (p->x < v1.x + vt * (v2.x - v1.x))
			{
				/* a valid crossing of y=p->y right of p->x */
				++cn;
			}
		}
		v1 = v2;
	}

	RTDEBUGF(ctx, 3, "_rtt_EdgeRingCrossingCount returning %d", cn);

#ifndef RELAX
  if ( memcmp(&v1, &v0, sizeof(RTPOINT2D)) )
  {
    rterror(ctx, "_rtt_EdgeRingCrossingCount: V[n] != V[0] (%g %g != %g %g)",
      v1.x, v1.y, v0.x, v0.y);
    return -1;
  }
#endif

  return cn;
}

/* Return 1 for true, 0 for false */
static int
_rtt_EdgeRingContainsPoint(const RTCTX *ctx, RTT_EDGERING *ring, RTPOINT2D *p)
{
  int cn = 0;

  RTT_EDGERING_POINT_ITERATOR *it = _rtt_EdgeRingIterator_begin(ctx, ring);
  cn = _rtt_EdgeRingCrossingCount(ctx, p, it);
  rtfree(ctx, it);
	return (cn&1);    /* 0 if even (out), and 1 if odd (in) */
}

static RTGBOX *
_rtt_EdgeRingGetBbox(const RTCTX *ctx, RTT_EDGERING *ring)
{
  int i;

  if ( ! ring->env )
  {
    RTDEBUGF(ctx, 2, "Computing RTGBOX for ring %p", ring);
    for (i=0; i<ring->size; ++i)
    {
      RTT_EDGERING_ELEM *elem = ring->elems[i];
      RTLINE *g = elem->edge->geom;
      const RTGBOX *newbox = rtgeom_get_bbox(ctx, rtline_as_rtgeom(ctx, g));
      if ( ! i ) ring->env = gbox_clone( ctx, newbox );
      else gbox_merge( ctx, newbox, ring->env );
    }
  }

  return ring->env;
}

static RTT_ELEMID
_rtt_EdgeRingGetFace(RTT_EDGERING *ring)
{
  RTT_EDGERING_ELEM *el = ring->elems[0];
  return el->left ? el->edge->face_left : el->edge->face_right;
}


/*
 * Register a face on an edge side
 *
 * Create and register face to shell (CCW) walks,
 * register arbitrary negative face_id to CW rings.
 *
 * Push CCW rings to shells, CW rings to holes.
 *
 * The ownership of the "geom" and "ids" members of the
 * RTT_EDGERING pushed to the given RTT_EDGERING_ARRAYS
 * are transferred to caller.
 *
 * @param side 1 for left side, -1 for right side
 *
 * @param holes an array where holes will be pushed
 *
 * @param shells an array where shells will be pushed
 *
 * @param registered id of registered face. It will be a negative number
 *  for holes or isolated edge strips (still registered in the face
 *  table, but only temporary).
 *
 * @return 0 on success, -1 on error.
 *
 */
static int
_rtt_RegisterFaceOnEdgeSide(RTT_TOPOLOGY *topo, RTT_ISO_EDGE *edge,
                            int side, RTT_ISO_EDGE_TABLE *edges,
                            RTT_EDGERING_ARRAY *holes,
                            RTT_EDGERING_ARRAY *shells,
                            RTT_ELEMID *registered)
{
  const RTT_BE_IFACE *iface = topo->be_iface;
  int sedge = edge->edge_id * side;
  /* this is arbitrary, could be taken as parameter */
  static const int placeholder_faceid = RTT_HOLES_FACE_PLACEHOLDER;
  RTT_EDGERING *ring;
  const RTCTX *ctx = topo->be_iface->ctx;

  /* Get edge ring */
  ring = _rtt_BuildEdgeRing(topo, edges, edge, side);

	RTDEBUG(ctx, 2, "Ring built, calling EdgeRingIsCCW");

  /* Compute winding (CW or CCW?) */
  int isccw = _rtt_EdgeRingIsCCW(ctx, ring);

  if ( isccw )
  {
    /* Create new face */
    RTT_ISO_FACE newface;

    RTDEBUGF(ctx, 1, "Ring of edge %d is a shell (shell %d)", sedge, shells->size);

    newface.mbr = _rtt_EdgeRingGetBbox(ctx, ring);

    newface.face_id = -1;
    /* Insert the new face */
    int ret = rtt_be_insertFaces( topo, &newface, 1 );
    newface.mbr = NULL;
    if ( ret == -1 )
    {
      RTT_EDGERING_CLEAN(ctx, ring);
      rterror(ctx, "Backend error: %s", rtt_be_lastErrorMessage(topo->be_iface));
      return -1;
    }
    if ( ret != 1 )
    {
      RTT_EDGERING_CLEAN(ctx, ring);
      rterror(ctx, "Unexpected error: %d faces inserted when expecting 1", ret);
      return -1;
    }
    /* return new face_id */
    *registered = newface.face_id;
    RTT_EDGERING_ARRAY_PUSH(ctx, shells, ring);

    /* update ring edges set new face_id on resp. side to *registered */
    ret = _rtt_UpdateEdgeRingSideFace(topo, ring, *registered);
    if ( ret )
    {
        rterror(ctx, "Errors updating edgering side face: %s",
                rtt_be_lastErrorMessage(iface));
        return -1;
    }

  }
  else /* cw, so is an hole */
  {
    RTDEBUGF(ctx, 1, "Ring of edge %d is a hole (hole %d)", sedge, holes->size);
    *registered = placeholder_faceid;
    RTT_EDGERING_ARRAY_PUSH(ctx, holes, ring);
  }

  return 0;
}

typedef struct RING_ACCUMULATOR_T {
  RTT_EDGERING_ARRAY *target;
  const RTCTX *ctx;
} RING_ACCUMULATOR;

static void
_rtt_AccumulateCanditates(void* item, void* userdata)
{
  RING_ACCUMULATOR *acc = userdata;
  RTT_EDGERING_ARRAY *candidates = acc->target;
  const RTCTX *ctx = acc->ctx;
  RTT_EDGERING *sring = item;
  RTT_EDGERING_ARRAY_PUSH(ctx, candidates, sring);
}

static RTT_ELEMID
_rtt_FindFaceContainingRing(RTT_TOPOLOGY* topo, RTT_EDGERING *ring,
                            RTT_EDGERING_ARRAY *shells)
{
  RTT_ELEMID foundInFace = -1;
  int i;
  const RTGBOX *minenv = NULL;
  RTPOINT2D pt;
  const RTGBOX *testbox;
  GEOSGeometry *ghole;
  const RTCTX *ctx = topo->be_iface->ctx;

  rt_getPoint2d_p(ctx, ring->elems[0]->edge->geom->points, 0, &pt );

  testbox = _rtt_EdgeRingGetBbox(ctx, ring);

  /* Create a GEOS Point from a vertex of the hole ring */
  {
    RTPOINT *point = rtpoint_make2d(ctx, topo->srid, pt.x, pt.y);
    ghole = RTGEOM2GEOS(ctx,  rtpoint_as_rtgeom(ctx, point), 1 );
    rtpoint_free(ctx, point);
    if ( ! ghole ) {
      rterror(ctx, "Could not convert edge geometry to GEOS: %s", rtgeom_get_last_geos_error(ctx));
      return -1;
    }
  }

  /* Build STRtree of shell envelopes */
  if ( ! shells->tree )
  {
    static const int STRTREE_NODE_CAPACITY = 10;
    RTDEBUG(ctx, 1, "Building STRtree");
	  shells->tree = GEOSSTRtree_create_r(ctx->gctx, STRTREE_NODE_CAPACITY);
    if (shells->tree == NULL)
    {
      rterror(ctx, "Could not create GEOS STRTree: %s", rtgeom_get_last_geos_error(ctx));
      return -1;
    }
    for (i=0; i<shells->size; ++i)
    {
      RTT_EDGERING *sring = shells->rings[i];
      const RTGBOX* shellbox = _rtt_EdgeRingGetBbox(ctx, sring);
      RTDEBUGF(ctx, 2, "RTGBOX of shell %p for edge %d is %g %g,%g %g",
        sring, sring->elems[0]->edge->edge_id, shellbox->xmin,
        shellbox->ymin, shellbox->xmax, shellbox->ymax);
      RTPOINTARRAY *pa = ptarray_construct(ctx, 0, 0, 2);
      RTPOINT4D pt;
      RTLINE *diag;
      pt.x = shellbox->xmin;
      pt.y = shellbox->ymin;
      ptarray_set_point4d(ctx, pa, 0, &pt);
      pt.x = shellbox->xmax;
      pt.y = shellbox->ymax;
      ptarray_set_point4d(ctx, pa, 1, &pt);
      diag = rtline_construct(ctx, topo->srid, NULL, pa);
      /* Record just envelope in ggeom */
      /* making valid, probably not needed */
      sring->genv = RTGEOM2GEOS(ctx, rtline_as_rtgeom(ctx, diag), 1 );
      rtline_free(ctx, diag);
      GEOSSTRtree_insert_r(ctx->gctx, shells->tree, sring->genv, sring);
    }
    RTDEBUG(ctx, 1, "STRtree build completed");
  }

  RTT_EDGERING_ARRAY candidates;
  RING_ACCUMULATOR accumulator;
  accumulator.target = &candidates;
  accumulator.ctx = ctx;
  RTT_EDGERING_ARRAY_INIT(ctx, &candidates);
	GEOSSTRtree_query_r(ctx->gctx, shells->tree, ghole, &_rtt_AccumulateCanditates, &accumulator);
  RTDEBUGF(ctx, 1, "Found %d candidate shells containing first point of ring's originating edge %d",
          candidates.size, ring->elems[0]->edge->edge_id * ( ring->elems[0]->left ? 1 : -1 ) );

  /* TODO: sort candidates by bounding box size */

  for (i=0; i<candidates.size; ++i)
  {
    RTT_EDGERING *sring = candidates.rings[i];
    const RTGBOX* shellbox = _rtt_EdgeRingGetBbox(ctx, sring);
    int contains = 0;

    if ( sring->elems[0]->edge->edge_id == ring->elems[0]->edge->edge_id )
    {
      RTDEBUGF(ctx, 1, "Shell %d is on other side of ring",
               _rtt_EdgeRingGetFace(sring));
      continue;
    }

    /* The hole envelope cannot equal the shell envelope */
    if ( gbox_same(ctx, shellbox, testbox) )
    {
      RTDEBUGF(ctx, 1, "Bbox of shell %d equals that of hole ring",
               _rtt_EdgeRingGetFace(sring));
      continue;
    }

    /* Skip if ring box is not in shell box */
    if ( ! gbox_contains_2d(ctx, shellbox, testbox) )
    {
      RTDEBUGF(ctx, 1, "Bbox of shell %d does not contain bbox of ring point",
               _rtt_EdgeRingGetFace(sring));
      continue;
    }

    /* Skip test if a containing shell was already found
     * and this shell's bbox is not contained in the other */
    if ( minenv && ! gbox_contains_2d(ctx, minenv, shellbox) )
    {
      RTDEBUGF(ctx, 2, "Bbox of shell %d (face %d) not contained by bbox "
                  "of last shell found to contain the point",
                  i, _rtt_EdgeRingGetFace(sring));
      continue;
    }

    contains = _rtt_EdgeRingContainsPoint(ctx, sring, &pt);
    if ( contains )
    {
      /* Continue until all shells are tested, as we want to
       * use the one with the smallest bounding box */
      /* IDEA: sort shells by bbox size, stopping on first match */
      RTDEBUGF(ctx, 1, "Shell %d contains hole of edge %d",
               _rtt_EdgeRingGetFace(sring),
               ring->elems[0]->edge->edge_id);
      minenv = shellbox;
      foundInFace = _rtt_EdgeRingGetFace(sring);
    }
  }
  if ( foundInFace == -1 ) foundInFace = 0;

  candidates.size = 0; /* Avoid destroying the actual shell rings */
  RTT_EDGERING_ARRAY_CLEAN(ctx, &candidates);

  GEOSGeom_destroy_r(ctx->gctx, ghole);

  return foundInFace;
}

/*
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
int
rtt_Polygonize(RTT_TOPOLOGY* topo)
{
  /*
     Iteratively:
       Fetch next edge with null left-or-right face
       For each NULL side:
         Find ring on its side
         If ring is CCW:
            create a new face, assign to the ring edges' appropriate side
         If ring is CW (face needs to be same of external):
            assign a negative face_id the ring edges' appropriate side
     Now for each edge with a negative face_id on the side:
       Find containing face (mbr cache and all)
       Update with id of containing face
   */

  const RTT_BE_IFACE *iface = topo->be_iface;
  RTT_ISO_EDGE *edge;
  int numfaces = -1;
  RTT_ISO_EDGE_TABLE edgetable;
  RTT_EDGERING_ARRAY holes, shells;
  int i;
  int err = 0;
  const RTCTX *ctx = iface->ctx;

  _rtt_EnsureGeos(iface->ctx);

  RTT_EDGERING_ARRAY_INIT(ctx, &holes);
  RTT_EDGERING_ARRAY_INIT(ctx, &shells);

  /*
   Check if Topology already contains some Face
   (ignoring the Universal Face)
  */
  numfaces = _rtt_CheckFacesExist(topo);
  if ( numfaces != 0 ) {
    if ( numfaces > 0 ) {
      /* Faces exist */
      rterror(iface->ctx, "rtt_Polygonize exception - table <topo>Face is not empty.");
    }
    /* Backend error, message should have been printed already */
    return -1;
  }

  edgetable.edges = _rtt_FetchAllEdges(topo, &(edgetable.size));
  if ( ! edgetable.edges ) {
    if (edgetable.size == 0) {
      /* not an error: no Edges */
      return 0;
    }
    /* error should have been printed already */
    return -1;
  }

  /* Sort edges by ID (to allow btree searches) */
  qsort(edgetable.edges, edgetable.size, sizeof(RTT_ISO_EDGE), compare_iso_edges_by_id);

  /* Mark all edges as unvisited */
  for (i=0; i<edgetable.size; ++i)
    edgetable.edges[i].face_left = edgetable.edges[i].face_right = -1;

  i = 0;
  while (1)
  {
    i = _rtt_FetchNextUnvisitedEdge(topo, &edgetable, i);
    if ( i < 0 ) break; /* end of unvisited */
    edge = &(edgetable.edges[i]);

    RTT_ELEMID newface = -1;

    RTDEBUGF(ctx, 1, "Next face-missing edge has id:%d, face_left:%d, face_right:%d",
               edge->edge_id, edge->face_left, edge->face_right);
    if ( edge->face_left == -1 )
    {
      err = _rtt_RegisterFaceOnEdgeSide(topo, edge, 1, &edgetable,
                                        &holes, &shells, &newface);
      if ( err ) break;
      RTDEBUGF(ctx, 1, "New face on the left of edge %d is %d",
                 edge->edge_id, newface);
      edge->face_left = newface;
    }
    if ( edge->face_right == -1 )
    {
      err = _rtt_RegisterFaceOnEdgeSide(topo, edge, -1, &edgetable,
                                        &holes, &shells, &newface);
      if ( err ) break;
      RTDEBUGF(ctx, 1, "New face on the right of edge %d is %d",
                 edge->edge_id, newface);
      edge->face_right = newface;
    }
  }

  if ( err )
  {
      rtt_release_edges(ctx, edgetable.edges, edgetable.size);
      RTT_EDGERING_ARRAY_CLEAN( ctx, &holes );
      RTT_EDGERING_ARRAY_CLEAN( ctx, &shells );
      rterror(ctx, "Errors fetching or registering face-missing edges: %s",
              rtt_be_lastErrorMessage(iface));
      return -1;
  }

  RTDEBUGF(ctx, 1, "Found %d holes and %d shells", holes.size, shells.size);

  /* TODO: sort holes by pt.x, sort shells by bbox.xmin */

  /* Assign shells to holes */
  for (i=0; i<holes.size; ++i)
  {
    RTT_ELEMID containing_face;
    RTT_EDGERING *ring = holes.rings[i];

    containing_face = _rtt_FindFaceContainingRing(topo, ring, &shells);
    RTDEBUGF(ctx, 1, "Ring %d contained by face %" RTTFMT_ELEMID, i, containing_face);
    if ( containing_face == -1 )
    {
      rtt_release_edges(ctx, edgetable.edges, edgetable.size);
      RTT_EDGERING_ARRAY_CLEAN( ctx, &holes );
      RTT_EDGERING_ARRAY_CLEAN( ctx, &shells );
      rterror(ctx, "Errors finding face containing ring: %s",
              rtt_be_lastErrorMessage(iface));
      return -1;
    }
    int ret = _rtt_UpdateEdgeRingSideFace(topo, holes.rings[i], containing_face);
    if ( ret )
    {
      rtt_release_edges(ctx, edgetable.edges, edgetable.size);
      RTT_EDGERING_ARRAY_CLEAN( ctx, &holes );
      RTT_EDGERING_ARRAY_CLEAN( ctx, &shells );
      rterror(ctx, "Errors updating edgering side face: %s",
              rtt_be_lastErrorMessage(iface));
      return -1;
    }
  }

  RTDEBUG(ctx, 1, "All holes assigned, cleaning up");

  rtt_release_edges(ctx, edgetable.edges, edgetable.size);

  /* delete all shell and hole EDGERINGS */
  RTT_EDGERING_ARRAY_CLEAN( ctx, &holes );
  RTT_EDGERING_ARRAY_CLEAN( ctx, &shells );

  return 0;
}
