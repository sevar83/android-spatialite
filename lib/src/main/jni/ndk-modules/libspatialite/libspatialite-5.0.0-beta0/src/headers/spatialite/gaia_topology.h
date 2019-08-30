/*
 gaia_topology.h -- Gaia common support for Topology
  
 version 4.3, 2015 July 15

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


/**
 \file gaia_topology.h

 Topology handling functions and constants 
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* stdio.h included for FILE objects. */
#include <stdio.h>
#ifdef DLL_EXPORT
#define GAIATOPO_DECLARE __declspec(dllexport)
#else
#define GAIATOPO_DECLARE extern
#endif
#endif

#ifndef _GAIATOPO_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIATOPO_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Typedef for Topology Accessor Object (opaque, hidden)

 \sa GaiaTopologyAccessorPtr
 */
    typedef struct gaia_topology_accessor GaiaTopologyAccessor;
/**
 Typedef for Topology Accessor Object pointer (opaque, hidden)

 \sa GaiaTopologyAccessor
 */
    typedef struct gaia_topology_accessor *GaiaTopologyAccessorPtr;

/**
 returns the last Topology exception (if any)

 \param topo_name unique name identifying the Topology.
 
 \return pointer to the last Topology error message: may be NULL if
 no Topology error is currently pending.
 */
    GAIATOPO_DECLARE const char *gaiaTopologyGetLastException (const char
							       *topo_name);

/**
 creates a new Topology and all related DB objects

 \param handle pointer to the current DB connection.
 \param topo_name unique name identifying the Topology.
 \param srid a spatial reference identifier.
 \param tolerance a tolerance factor measuren in the same units defined
        by the associated SRID.
 \param has_z boolean: if TRUE this Topology supports 3D (XYZ).

 \return 0 on failure: any other value on success.

 \sa gaiaTopologyDrop
 */
    GAIATOPO_DECLARE int gaiaTopologyCreate (sqlite3 * handle,
					     const char *topo_name, int srid,
					     double tolerance, int has_z);

/**
 completely drops an already existing Topology and removes all related DB objects

 \param handle pointer to the current DB connection.
 \param topo_name unique name identifying the Topology.

 \return 0 on failure: any other value on success.

 \sa gaiaTopologyCreate
 */
    GAIATOPO_DECLARE int gaiaTopologyDrop (sqlite3 * handle,
					   const char *topo_name);

/**
 creates an opaque Topology Accessor object starting from its DB configuration

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return the pointer to newly created Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache, 
 gaiaGetTopology
 
 \note you are responsible to destroy (before or after) any allocated 
 Topology Accessor Object. The Topology Accessor once created will be
 preserved within the internal connection cache for future references.
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaTopologyFromDBMS (sqlite3 *
								   handle,
								   const void
								   *cache,
								   const char
								   *topo_name);

/**
 retrieves a Topology configuration from DB

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.
 \param topology_name on completion will point to the real Topology name.
 \param srid on completion will contain the Topology SRID.
 \param tolerance on completion will contain the tolerance argument.
 \param has_z on completion will report if the Topology is of the 3D type. 

 \return 1 on success: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache, 
 gaiaGetTopology
 */
    GAIATOPO_DECLARE int gaiaReadTopologyFromDBMS (sqlite3 *
						   handle,
						   const char
						   *topo_name,
						   char **topology_name,
						   int *srid,
						   double *tolerance,
						   int *has_z);

/**
 retrieves an already defined opaque Topology Accessor object from the
 internal connection cache

 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return pointer to an already existing Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromDBMS, 
 gaiaGetTopology
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaTopologyFromCache (const void
								    *cache,
								    const char
								    *topo_name);

/**
 will attempt to return a reference to a Topology Accessor object

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param topo_name unique name identifying the Topology.

 \return pointer to Topology Accessor Object: NULL on failure.

 \sa gaiaTopologyCreate, gaiaTopologyDestroy, gaiaTopologyFromCache,
 gaiaTopologyFromDBMS
 
 \note if a corresponding Topology Accessor Object is already defined
 will return a pointer to such Objet. Otherwise an attempt will be made
 in order to create a new Topology Accessor object starting from its DB 
 configuration.
 */
    GAIATOPO_DECLARE GaiaTopologyAccessorPtr gaiaGetTopology (sqlite3 *
							      handle,
							      const void
							      *cache,
							      const char
							      *topo_name);

/**
 destroys a Topology Accessor object and any related memory allocation

 \param ptr pointer to the Topology Accessor Object to be destroyed.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE void gaiaTopologyDestroy (GaiaTopologyAccessorPtr ptr);

/**
 Adds an isolated node into the Topology

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of containing face or -1 for "unknown".
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident nodes, crossing edges, actual face containement)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddIsoNode (GaiaTopologyAccessorPtr
						   ptr, sqlite3_int64 face,
						   gaiaPointPtr pt,
						   int skip_checks);

/**
 Moves an isolated node in a Topology from one point to another

 \param ptr pointer to the Topology Accessor Object.
 \param node the unique identifier of node.
 \param pt pointer to the Node Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaMoveIsoNode (GaiaTopologyAccessorPtr ptr,
					  sqlite3_int64 node, gaiaPointPtr pt);

/**
 Removes an isolated node from a Topology

 \param ptr pointer to the Topology Accessor Object.
 \param node the unique identifier of node.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaRemIsoNode (GaiaTopologyAccessorPtr ptr,
					 sqlite3_int64 node);

/**
 Adds an isolated edge into the Topology

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the Start Node's unique identifier.
 \param end_node the End Node unique identifier.
 \param ln pointer to the Edge Geometry.

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddIsoEdge (GaiaTopologyAccessorPtr
						   ptr,
						   sqlite3_int64 start_node,
						   sqlite3_int64 end_node,
						   gaiaLinestringPtr ln);

/**
 Removes an isolated edge from a Topology

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of edge.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaRemIsoEdge (GaiaTopologyAccessorPtr ptr,
					 sqlite3_int64 edge);

/**
 Changes the shape of an Edge without affecting the Topology structure

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the Edge unique identifier.
 \param ln pointer to the Edge Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaChangeEdgeGeom (GaiaTopologyAccessorPtr ptr,
					     sqlite3_int64 edge_id,
					     gaiaLinestringPtr ln);

/**
 Split an edge by a node, modifying the original edge and adding a new one.

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of the edge to be split.
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident node, point not on edge...)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaModEdgeSplit (GaiaTopologyAccessorPtr
						     ptr, sqlite3_int64 edge,
						     gaiaPointPtr pt,
						     int skip_checks);

/**
 Split an edge by a node, replacing it with two new edges.

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of the edge to be split.
 \param pt pointer to the Node Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (coincident node, point not on edge...)

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaNewEdgesSplit (GaiaTopologyAccessorPtr
						      ptr, sqlite3_int64 edge,
						      gaiaPointPtr pt,
						      int skip_checks);

/**
 Adds a new edge possibly splitting and modifying a face.

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the unique identifier of the starting node.
 \param end_node the unique identifier of the ending node.
 \param ln pointer to the Edge Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (curve being simple and valid, start/end nodes, consistency 
 actual face containement)

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaAddEdgeModFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64
						       start_node,
						       sqlite3_int64 end_node,
						       gaiaLinestringPtr ln,
						       int skip_checks);

/**
 Adds a new edge possibly splitting a face (replacing with two new faces).

 \param ptr pointer to the Topology Accessor Object.
 \param start_node the unique identifier of the starting node.
 \param end_node the unique identifier of the ending node.
 \param ln pointer to the Edge Geometry.
 \param skip_checks boolean: if TRUE skips consistency checks
 (curve being simple and valid, start/end nodes, consistency 
 actual face containement)

 \return the ID of the inserted Edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64
	gaiaAddEdgeNewFaces (GaiaTopologyAccessorPtr ptr,
			     sqlite3_int64 start_node, sqlite3_int64 end_node,
			     gaiaLinestringPtr ln, int skip_checks);

/**
 Removes an edge, and if the removed edge separated two faces, delete one
 of them and modify the other to take the space of both.

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the unique identifier of the edge to be removed.

 \return the ID of face that takes up the space previously occupied 
 by the removed edge; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaRemEdgeModFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64 edge_id);

/**
 Removes an edge, and if the removed edge separated two faces, delete the
 original faces and replace them with a new face.

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id the unique identifier of the edge to be removed.

 \return the ID of the created face; a negative number on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaRemEdgeNewFace (GaiaTopologyAccessorPtr
						       ptr,
						       sqlite3_int64 edge_id);

/**
 Heal two edges by removing the node connecting them, modifying the
 first edge and removing the second edge

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id1 the unique identifier of the first edge to be healed.
 \param edge_id2 the unique identifier of the second edge to be healed.

 \return the ID of the removed node.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaModEdgeHeal (GaiaTopologyAccessorPtr
						    ptr,
						    sqlite3_int64 edge_id1,
						    sqlite3_int64 edge_id2);

/**
 Heal two edges by removing the node connecting them, deleting both edges
 and replacing them with an edge whose orientation is the same of the
 first edge provided

 \param ptr pointer to the Topology Accessor Object.
 \param edge_id1 the unique identifier of the first edge to be healed.
 \param edge_id2 the unique identifier of the second edge to be healed.

 \return the ID of the removed node.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64 gaiaNewEdgeHeal (GaiaTopologyAccessorPtr
						    ptr,
						    sqlite3_int64 edge_id1,
						    sqlite3_int64 edge_id2);

/**
 Return the geometry of a Topology Face

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of the face.

 \return pointer to Geometry (polygon); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaGetFaceGeometry (GaiaTopologyAccessorPtr ptr, sqlite3_int64 face);

/**
 Updates a temporary table containing the ordered list of Edges
 (in counterclockwise order) for every Face.
 EdgeIDs are signed value; a negative ID intends reverse direction.

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of the face.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaGetFaceEdges (GaiaTopologyAccessorPtr ptr, sqlite3_int64 face);

/**
 Find the ID of a Node at a Point location

 \param ptr pointer to the Topology Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of a Node; -1 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64
	gaiaGetNodeByPoint (GaiaTopologyAccessorPtr ptr, gaiaPointPtr pt,
			    double tolerance);

/**
 Find the ID of an Edge at a Point location

 \param ptr pointer to the Topology Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of an Edge; -1 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64
	gaiaGetEdgeByPoint (GaiaTopologyAccessorPtr ptr, gaiaPointPtr pt,
			    double tolerance);

/**
 Find the ID of a Face at a Point location

 \param ptr pointer to the Topology Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of a Face; -1 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64
	gaiaGetFaceByPoint (GaiaTopologyAccessorPtr ptr, gaiaPointPtr pt,
			    double tolerance);

/**
 Transforms a (multi)Linestring or (multi)Polygon into an new MultiLinestring.

 \param geom pointer to the input Geometry (Linestring, MultiLinestring,
 Polygon or MultiPolygon).
 \param line_max_points if set to a positive number all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having no more than
 this maximum number of points. 
 \param max_length if set to a positive value all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having a length
 not exceeding this threshold. If both line_max_points and max_legth
 are set as the same time the first condition occurring will cause
 a new Line to be started.

 \return a MultiLinestring Geometry; NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaTopoGeo_SubdivideLines (gaiaGeomCollPtr geom, int line_max_points,
				    double max_length);

/**
 Adds a Point to an existing Topology and possibly splitting an Edge.

 \param ptr pointer to the Topology Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of the Node; -1 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE sqlite3_int64
	gaiaTopoGeo_AddPoint (GaiaTopologyAccessorPtr ptr, gaiaPointPtr pt,
			      double tolerance);

/**
 Adds a Linestring to an existing Topology without determining generated faces..

 \param ptr pointer to the Topology Accessor Object.
 \param ln pointer to the Linestring Geometry.
 \param tolerance approximation factor.
 \param edge_ids on success will point to an array of Edge IDs
 \param ids_count on success will report the number of Edge IDs in the
 above array

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_AddLineStringNoFace
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_AddLineString (GaiaTopologyAccessorPtr ptr,
				   gaiaLinestringPtr pt, double tolerance,
				   sqlite3_int64 ** edge_ids, int *ids_count);

/**
 Adds a Linestring to an existing Topology and possibly splitting Edges/Faces.

 \param ptr pointer to the Topology Accessor Object.
 \param ln pointer to the Linestring Geometry.
 \param tolerance approximation factor.
 \param edge_ids on success will point to an array of Edge IDs
 \param ids_count on success will report the number of Edge IDs in the
 above array

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_AddLineString, gaiaTopoGeo_Polygonize
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_AddLineStringNoFace (GaiaTopologyAccessorPtr ptr,
					 gaiaLinestringPtr pt,
					 double tolerance,
					 sqlite3_int64 ** edge_ids,
					 int *ids_count);

/**
 Determine and register all topology faces.

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_AddLineStringNoFace, 
 gaiaTopoGeo_FromGeoTableNoFace, FromGeoTableNoFaceExt
 */
    GAIATOPO_DECLARE int gaiaTopoGeo_Polygonize (GaiaTopologyAccessorPtr ptr);

/**
 Snap Geometry to Topology.

 \param ptr pointer to the Topology Accessor Object.
 \param geom pointer to the input Geometry.
 \param tolerance_snap snap tolerance.
 \param tolerance_removal removal tolerance (use -1 to skip removal phase).
 \param iterate if non zero, allows snapping to more than a single 
 vertex, iteratively.

 \return pointer to the snapped Geometry; NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaTopoSnap (GaiaTopologyAccessorPtr ptr, gaiaGeomCollPtr geom,
		      double tolerance_snap, double tolerance_removal,
		      int iterate);

/**
 Adds a Polygon to an existing Topology and possibly splitting Edges/Faces.

 \param ptr pointer to the Topology Accessor Object.
 \param pg pointer to the Polygon Geometry.
 \param tolerance approximation factor.
 \param face_ids on success will point to an array of Face IDs
 \param ids_count on success will report the number of Face IDs in the
 above array

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_AddPolygon (GaiaTopologyAccessorPtr ptr,
				gaiaPolygonPtr pg, double tolerance,
				sqlite3_int64 ** face_ids, int *ids_count);

/**
 Populates a Topology by importing a whole GeoTable

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the input GeoTable.
 If NULL the "main" DB will be intended by default.
 \param table name of the input GeoTable.
 \param column name of the input Geometry Column.
 Could be NULL is the input table has just a single Geometry Column.
 \param tolerance approximation factor.
 \param line_max_points if set to a positive number all input Linestrings
 and/or Polygon Rings will be split into simpler Linestrings having no more 
 than this maximum number of points. 
 \param max_length if set to a positive value all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having a length
 not exceeding this threshold. If both line_max_points and max_legth
 are set as the same time the first condition occurring will cause
 a new Line to be started. 

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_FromGeoTableNoFace
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_FromGeoTable (GaiaTopologyAccessorPtr ptr,
				  const char *db_prefix, const char *table,
				  const char *column, double tolerance,
				  int line_max_points, double max_length);

/**
 Populates a Topology by importing a whole GeoTable without 
 determining generated faces

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the input GeoTable.
 If NULL the "main" DB will be intended by default.
 \param table name of the input GeoTable.
 \param column name of the input Geometry Column.
 Could be NULL is the input table has just a single Geometry Column.
 \param tolerance approximation factor.
 \param line_max_points if set to a positive number all input Linestrings
 and/or Polygon Rings will be split into simpler Linestrings having no more 
 than this maximum number of points. 
 \param max_length if set to a positive value all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having a length
 not exceeding this threshold. If both line_max_points and max_legth
 are set as the same time the first condition occurring will cause
 a new Line to be started. 

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_FromGeoTable, gaiaTopoGeo_Polygonize
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_FromGeoTableNoFace (GaiaTopologyAccessorPtr ptr,
					const char *db_prefix,
					const char *table, const char *column,
					double tolerance, int line_max_points,
					double max_length);

/**
 Populates a Topology by importing a whole GeoTable - Extended mode

 \param ptr pointer to the Topology Accessor Object.
 \param sql_in an SQL statement (SELECT) returning input features
 \param sql_out a second SQL statement (INSERT INTO) intended to
 store failing features references into the "dustbin" table.
 \param sql_in2 an SQL statement (SELECT) returning a single input 
 feature (used for retrying to insert a failing feature)
 \param tolerance approximation factor.
 \param line_max_points if set to a positive number all input Linestrings
 and/or Polygon Rings will be split into simpler Linestrings having no more 
 than this maximum number of points. 
 \param max_length if set to a positive value all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having a length
 not exceeding this threshold. If both line_max_points and max_legth
 are set as the same time the first condition occurring will cause
 a new Line to be started. 

 \return 0 if all input features were succesfully importer, or a
 positive number (total count of failing features raising an exception
 and referenced by the "dustbin" table); -1 if some unexpected
 error occurred.

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_FromGeoTableNoFaceExtended
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_FromGeoTableExtended (GaiaTopologyAccessorPtr ptr,
					  const char *sql_in,
					  const char *sql_out,
					  const char *sql_in2,
					  double tolerance,
					  int line_max_points,
					  double max_length);

/**
 Populates a Topology by importing a whole GeoTable without 
 determining generated faces - Extended mode

 \param ptr pointer to the Topology Accessor Object.
 \param sql_in an SQL statement (SELECT) returning input features
 \param sql_out a second SQL statement (INSERT INTO) intended to
 store failing features references into the "dustbin" table.
 \param sql_in2 an SQL statement (SELECT) returning a single input 
 feature (used for retrying to insert a failing feature)
 \param tolerance approximation factor.
 \param line_max_points if set to a positive number all input Linestrings
 and/or Polygon Rings will be split into simpler Linestrings having no more 
 than this maximum number of points. 
 \param max_length if set to a positive value all input Linestrings 
 and/or Polygon Rings will be split into simpler Lines having a length
 not exceeding this threshold. If both line_max_points and max_legth
 are set as the same time the first condition occurring will cause
 a new Line to be started. 

 \return 0 if all input features were succesfully importer, or a
 positive number (total count of failing features raising an exception
 and referenced by the "dustbin" table); -1 if some unexpected
 error occurred.

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_FromGeoTableExtended, 
 gaiaTopoGeo_Polygonize
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_FromGeoTableNoFaceExtended (GaiaTopologyAccessorPtr ptr,
						const char *sql_in,
						const char *sql_out,
						const char *sql_in2,
						double tolerance,
						int line_max_points,
						double max_length);

/**
 Creates and populates a new GeoTable by snapping all Geometries
 contained into another GeoTable against a given Topology

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the input GeoTable.
 If NULL the "main" DB will be intended by default.
 \param table name of the input GeoTable.
 \param column name of the input Geometry Column.
 Could be NULL is the input table has just a single Geometry Column.
 \param outtable name of the output GeoTable.
 \param tolerance_snap snap tolerance.
 \param tolerance_removal removal tolerance (use -1 to skip removal phase).
 \param iterate if non zero, allows snapping to more than a single 
 vertex, iteratively.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_SnappedGeoTable (GaiaTopologyAccessorPtr ptr,
				     const char *db_prefix, const char *table,
				     const char *column, const char *outtable,
				     double tolerance_snap,
				     double tolerance_removal, int iterate);

/**
 Creates a temporary table containing a validation report for a given TopoGeo.

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int gaiaValidateTopoGeo (GaiaTopologyAccessorPtr ptr);

/**
 Return a Point geometry (seed) identifying a Topology Edge

 \param ptr pointer to the Topology Accessor Object.
 \param edge the unique identifier of the edge.

 \return pointer to Geometry (point); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaGetEdgeSeed (GaiaTopologyAccessorPtr ptr, sqlite3_int64 edge);

/**
 Return a Point geometry (seed) identifying a Topology Face

 \param ptr pointer to the Topology Accessor Object.
 \param face the unique identifier of the face.

 \return pointer to Geometry (point); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaGetFaceSeed (GaiaTopologyAccessorPtr ptr, sqlite3_int64 face);

/**
 Ensures that all Edges on a Topology-Geometry will have not less
 than three vertices; for all Edges found being simple two-points 
 segments a third intermediate point will be interpolated.

 \param ptr pointer to the Topology Accessor Object.

 \return the total number of changed Edges; a negativa number on error

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_DisambiguateSegmentEdges (GaiaTopologyAccessorPtr ptr);

/**
 Will update all Seeds for a Topology-Geometry

 \param ptr pointer to the Topology Accessor Object.
 \param mode if set to 0 a full update of all Seeds will be performed,
 otherwise an incremental update will happen.

 \return 1 on success; 0 on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeoUpdateSeeds (GaiaTopologyAccessorPtr ptr, int mode);

/**
 Will snap a Point geometry to TopoSeeds

 \param ptr pointer to the Topology Accessor Object.
 \param pt pointer to the Point Geometry.
 \param distance tolerance approximation factor.

 \return pointer to Geometry (point); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaTopoGeoSnapPointToSeed (GaiaTopologyAccessorPtr ptr,
				    gaiaGeomCollPtr pt, double distance);

/**
 Will snap a Linestring geometry to TopoSeeds

 \param ptr pointer to the Topology Accessor Object.
 \param ln pointer to the Linestring Geometry.
 \param distance tolerance approximation factor.

 \return pointer to Geometry (linestring); NULL on failure.

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE gaiaGeomCollPtr
	gaiaTopoGeoSnapLinestringToSeed (GaiaTopologyAccessorPtr ptr,
					 gaiaGeomCollPtr ln, double distance);

/**
 Extracts a Simple Features Table out from a Topology by matching
 Topology Seeds to a given reference Table.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output output table to be created and populated.
 \param with_spatial_index boolean flag: if set to TRUE (non ZERO) a Spatial
 Index supporting the output table will be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_ToGeoTable (GaiaTopologyAccessorPtr ptr,
				const char *db_prefix, const char *ref_table,
				const char *ref_column, const char *out_table,
				int with_spatial_index);

/**
 Creates and populates a Table containing a comprehensive report
 about all intesections between the Faces of some Topology and
 a given reference Table of the Polygon/Multipolygon type.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output output table to be created and populated.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_PolyFacesList (GaiaTopologyAccessorPtr ptr,
				   const char *db_prefix, const char *ref_table,
				   const char *ref_column,
				   const char *out_table);

/**
 Creates and populates a Table containing a comprehensive report
 about all intesections between the Edges of some Topology and
 a given reference Table of the Linestring/Multilinestring type.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output output table to be created and populated.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_LineEdgesList (GaiaTopologyAccessorPtr ptr,
				   const char *db_prefix, const char *ref_table,
				   const char *ref_column,
				   const char *out_table);

/**
 Extracts a simplified/generalized Simple Features Table out from a Topology 
 by matching Topology Seeds to a given reference Table.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output output table to be created and populated.
 \param tolerance approximation radius required by the Douglar-Peucker
 simplification algorithm.
 \param with_spatial_index boolean flag: if set to TRUE (non ZERO) a Spatial
 Index supporting the output table will be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_ToGeoTableGeneralize (GaiaTopologyAccessorPtr ptr,
					  const char *db_prefix,
					  const char *ref_table,
					  const char *ref_column,
					  const char *out_table,
					  double tolerance,
					  int with_spatial_index);

/**
 Removes all small Faces from a Topology

 \param ptr pointer to the Topology Accessor Object.
 \param min_circularity threshold Circularity-index identifying threadlike Faces.
 \param min_area threshold area to identify small Faces.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_RemoveSmallFaces (GaiaTopologyAccessorPtr ptr,
				      double min_circularity, double min_area);

/**
 Removes all dangling Edges from a Topology

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_RemoveDanglingEdges (GaiaTopologyAccessorPtr ptr);

/**
 Removes all dangling Nodes from a Topology

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_RemoveDanglingNodes (GaiaTopologyAccessorPtr ptr);

/**
 Removes all useless Nodes from a Topology by calling ST_NewEdgeHeal

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_ModEdgeHeal
 */
    GAIATOPO_DECLARE int gaiaTopoGeo_NewEdgeHeal (GaiaTopologyAccessorPtr ptr);

/**
 Removes all useless Nodes from a Topology by calling ST_ModEdgeHeal

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_NewEdgeHeal
 */
    GAIATOPO_DECLARE int gaiaTopoGeo_ModEdgeHeal (GaiaTopologyAccessorPtr ptr);

/**
 Removes all useless Nodes from a Topology by calling ST_NewEdgeHeal

 \param ptr pointer to the Topology Accessor Object.
 \param line_max_points if set to a positive number all input Edges
 will be split into simpler Edges having no more than this maximum 
 number of points. 
 \param max_length if set to a positive value all input Edges will 
 be split into simpler Edges having a length not exceeding this 
 threshold. If both line_max_points and max_legth are set as the 
 same time the first condition occurring will cause an Edge to be split
 in two halves. 

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_ModEdgeHeal
 */
    GAIATOPO_DECLARE int gaiaTopoGeo_NewEdgesSplit (GaiaTopologyAccessorPtr ptr,
						    int line_max_points,
						    double max_length);

/**
 Removes all useless Nodes from a Topology by calling ST_ModEdgeHeal

 \param ptr pointer to the Topology Accessor Object.
 \param line_max_points if set to a positive number all input Edges
 will be split into simpler Edges having no more than this maximum 
 number of points. 
 \param max_length if set to a positive value all input Edges will 
 be split into simpler Edges having a length not exceeding this 
 threshold. If both line_max_points and max_legth are set as the 
 same time the first condition occurring will cause an Edge to be split
 in two halves. 

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS, gaiaTopoGeo_NewEdgeHeal
 */
    GAIATOPO_DECLARE int gaiaTopoGeo_ModEdgeSplit (GaiaTopologyAccessorPtr ptr,
						   int line_max_points,
						   double max_length);

/**
 creates a TopoLayer and its corresponding Feature relations for a given 
 Topology by matching Topology Seeds to a given reference Table.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param topolayer_name name of the TopoLayer to be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_CreateTopoLayer (GaiaTopologyAccessorPtr ptr,
				     const char *db_prefix,
				     const char *ref_table,
				     const char *ref_column,
				     const char *topolayer_name);

/**
 initializes a TopoLayer (laking all corresponding Feature relations) for a given 
 Topology from a given reference Table.

 \param ptr pointer to the Topology Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param topolayer_name name of the TopoLayer to be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_InitTopoLayer (GaiaTopologyAccessorPtr ptr,
				   const char *db_prefix,
				   const char *ref_table,
				   const char *topolayer_name);

/**
 completely removes a TopoLayer from a Topology.

 \param ptr pointer to the Topology Accessor Object.
 \param topolayer_name name of the TopoLayer to be removed.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_RemoveTopoLayer (GaiaTopologyAccessorPtr ptr,
				     const char *topolayer_name);

/**
 creates a GeoTable by exporting Features out from a given TopoLayer.

 \param ptr pointer to the Topology Accessor Object.
 \param topolayer_name name of an existing TopoLayer.
 \param out_table name of the output GeoTable to be created.
 \param with_spatial_index boolean flag: if set to TRUE (non ZERO) a Spatial
 Index supporting the output table will be created.
 \param create_only boolean flag; is set to any value != 0 (TRUE)
 just the output table will be creayed but no TopoFeature will
 be actually inserted.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_ExportTopoLayer (GaiaTopologyAccessorPtr ptr,
				     const char *topolayer_name,
				     const char *out_table,
				     int with_spatial_index, int create_only);

/**
 inserts into the output GeoTable a single Features extracted out 
 from a given TopoLayer.

 \param ptr pointer to the Topology Accessor Object.
 \param topolayer_name name of an existing TopoLayer.
 \param out_table name of the target GeoTable.
 \param fid unique identifier of the TopoLayer's Feature to
 be inserted.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaTopologyFromDBMS
 */
    GAIATOPO_DECLARE int
	gaiaTopoGeo_InsertFeatureFromTopoLayer (GaiaTopologyAccessorPtr ptr,
						const char *topolayer_name,
						const char *out_table,
						sqlite3_int64 fid);

#ifdef __cplusplus
}
#endif


#endif				/* _GAIATOPO_H */
