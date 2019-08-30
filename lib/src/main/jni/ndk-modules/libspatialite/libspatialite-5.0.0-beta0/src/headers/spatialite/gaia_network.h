/*
 gaia_network.h -- Gaia common support for Topology-Network
  
 version 4.3, 2015 August 11

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
 \file gaia_network.h

 Topology-Network handling functions and constants 
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* stdio.h included for FILE objects. */
#include <stdio.h>
#ifdef DLL_EXPORT
#define GAIANET_DECLARE __declspec(dllexport)
#else
#define GAIANET_DECLARE extern
#endif
#endif

#ifndef _GAIANET_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GAIANET_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 Typedef for Topology-Network Accessor Object (opaque, hidden)

 \sa GaiaNetworkAccessorPtr
 */
    typedef struct gaia_network_accessor GaiaNetworkAccessor;
/**
 Typedef for Network Accessor Object pointer (opaque, hidden)

 \sa GaiaNetworkAccessor
 */
    typedef struct gaia_network_accessor *GaiaNetworkAccessorPtr;

/**
 returns the last Topology-Network exception (if any)

 \param network_name unique name identifying the Topology-Network.
 
 \return pointer to the last Topology-Network error message: may be NULL if
 no Topology error is currently pending.
 */
    GAIANET_DECLARE const char *gaiaNetworkGetLastException (const char
							     *network_name);

/**
 creates a new Topology-Network and all related DB objects

 \param handle pointer to the current DB connection.
 \param network_name unique name identifying the Topology-Network.
 \param spatial boolean: if TRUE this Topology-Network will be assumed
 to be of the Spatial type, otherwise a Logical Network will be assumed.
 \param srid a spatial reference identifier (only applies to Spatial
 Networks).
 \param has_z boolean: if TRUE this Topology-Network supports 3D (XYZ) 
 (only applies to Spatial Networks).
 \param allow_coincident boolean: if TRUE coincident Nodes will be
 tolerated, otherwise a coincident Node condition will raise an exception.
 This argument only applies to Spatial Networks and is meaningless for
 Logical Networks.

 \return 0 on failure: any other value on success.

 \sa gaiaNetworkDrop
 */
    GAIANET_DECLARE int gaiaNetworkCreate (sqlite3 * handle,
					   const char *network_name,
					   int spatial, int srid, int has_z,
					   int allow_coincident);

/**
 completely drops an already existing Topology-Network and removes all related DB objects

 \param handle pointer to the current DB connection.
 \param network_name unique name identifying the Topology-Network.

 \return 0 on failure: any other value on success.

 \sa gaiaNetworkCreate
 */
    GAIANET_DECLARE int gaiaNetworkDrop (sqlite3 * handle,
					 const char *network_name);

/**
 creates an opaque Topology-Network Accessor object starting from its DB configuration

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param network_name unique name identifying the Topology-Network.

 \return the pointer to newly created Topology-Network Accessor Object: 
 NULL on failure.

 \sa gaiaNetworkCreate, gaiaNetworkDestroy, gaiaNetworkFromCache, 
 gaiaGetNetwork
 
 \note you are responsible to destroy (before or after) any allocated 
 Topology-Network Accessor Object. The Topology-Network Accessor once 
 created will be preserved within the internal connection cache for 
 future references.
 */
    GAIANET_DECLARE GaiaNetworkAccessorPtr gaiaNetworkFromDBMS (sqlite3 *
								handle,
								const void
								*cache,
								const char
								*network_name);

/**
 retrieves a Topology configuration from DB

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param net_name unique name identifying the Topology-Network.
 \param network_name on completion will point to the real Topology-Network name.
 \param spatial on completion will report if the Topology-Network is of
 the Spatial or Logical type.
 \param srid on completion will contain the Topology-Network SRID.
 \param has_z on completion will report if the Topology-Network is of the 3D type. 
 \param allow_coincident on completion will report if the Topology-Network
 tolerates a Coindident Nodes condition without raising an exception. 

 \return 1 on success: NULL on failure.

 \sa gaiaNetworkCreate, gaiaNetworkDestroy, gaiaNetworkFromCache, 
 gaiaGetNetwork
 */
    GAIANET_DECLARE int gaiaReadNetworkFromDBMS (sqlite3 *
						 handle,
						 const char
						 *net_name,
						 char **network_name,
						 int *spatial, int *srid,
						 int *has_z,
						 int *allow_coincident);

/**
 retrieves an already defined opaque Topology-Network Accessor object from the
 internal connection cache

 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param network_name unique name identifying the Topology-Network.

 \return pointer to an already existing Topology-Network Accessor Object: NULL on failure.

 \sa gaiaNetworkCreate, gaiaNetworkDestroy, gaiaNetworkFromDBMS, 
 gaiaGetNetwork
 */
    GAIANET_DECLARE GaiaNetworkAccessorPtr gaiaNetworkFromCache (const void
								 *cache,
								 const char
								 *network_name);

/**
 will attempt to return a reference to a Topology-Network Accessor object

 \param handle pointer to the current DB connection.
 \param cache pointer to the opaque Cache Object supporting the DB connection
 \param network_name unique name identifying the Topology-Network.

 \return pointer to Topology-Network Accessor Object: NULL on failure.

 \sa gaiaNetworkCreate, gaiaNetworkDestroy, gaiaNetworkFromCache,
 gaiaNetworkFromDBMS
 
 \note if a corresponding Topology-Network Accessor Object is already defined
 will return a pointer to such Objet. Otherwise an attempt will be made
 in order to create a new Topology-Network Accessor object starting from 
 its DB configuration.
 */
    GAIANET_DECLARE GaiaNetworkAccessorPtr gaiaGetNetwork (sqlite3 *
							   handle,
							   const void
							   *cache,
							   const char
							   *network_name);

/**
 destroys a Topology-Network Accessor object and any related memory allocation

 \param ptr pointer to the Topology-Network Accessor Object to be destroyed.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE void gaiaNetworkDestroy (GaiaNetworkAccessorPtr ptr);

/**
 Adds an isolated node into the Topology-Network

 \param ptr pointer to the Topology-Network Accessor Object.
 \param pt pointer to the Node Geometry.

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaMoveIsoNetNode, gaiaRemIsoNetNode,
 gaiaAddLink
 */
    GAIANET_DECLARE sqlite3_int64 gaiaAddIsoNetNode (GaiaNetworkAccessorPtr
						     ptr, gaiaPointPtr pt);

/**
 Moves an isolated node in a Topology-Network from one point to another

 \param ptr pointer to the Topology-Network Accessor Object.
 \param node the unique identifier of node.
 \param pt pointer to the Node Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddIsoNetNode, gaiaRemIsoNetNode
 */
    GAIANET_DECLARE int gaiaMoveIsoNetNode (GaiaNetworkAccessorPtr ptr,
					    sqlite3_int64 node,
					    gaiaPointPtr pt);

/**
 Removes an isolated node from a Topology-Network

 \param ptr pointer to the Topology-Network Accessor Object.
 \param node the unique identifier of node.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddIsoNetNode, gaiaMoveIsoNetNode
 */
    GAIANET_DECLARE int gaiaRemIsoNetNode (GaiaNetworkAccessorPtr ptr,
					   sqlite3_int64 node);

/**
 Adds a link into the Topology-Network

 \param ptr pointer to the Topology-Network Accessor Object.
 \param start_node the Start Node's unique identifier.
 \param end_node the End Node unique identifier.
 \param ln pointer to the Link Geometry.

 \return the ID of the inserted Link; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddIsoNetNode, gaiaChangeLinkGeom,
 gaiaRemoveLink, gaiaNewLogLinkSplit, gaiaModLogLinkSplit, 
 gaiaNewGeoLinkSplit, gaiaModGeoLinkSplit, gaiaNewLinkHeal,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaAddLink (GaiaNetworkAccessorPtr ptr,
					       sqlite3_int64 start_node,
					       sqlite3_int64 end_node,
					       gaiaLinestringPtr ln);

/**
 Changes the shape of a Link without affecting the Topology-Network structure

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link_id the Link unique identifier.
 \param ln pointer to the Link Geometry.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaRemoveLink
 */
    GAIANET_DECLARE int gaiaChangeLinkGeom (GaiaNetworkAccessorPtr ptr,
					    sqlite3_int64 link_id,
					    gaiaLinestringPtr ln);

/**
 Removes a Link from a Topology-Network

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of link.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink
 */
    GAIANET_DECLARE int gaiaRemoveLink (GaiaNetworkAccessorPtr ptr,
					sqlite3_int64 link);

/**
 Split a logical link, replacing it with two new links.

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the link to be split.

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaModLogLinkSplit, 
 gaiaNewGeoLinkSplit, gaiaModGeoLinkSplit, gaiaNewLinkHeal,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaNewLogLinkSplit (GaiaNetworkAccessorPtr
						       ptr, sqlite3_int64 link);

/**
 Split a logical link, modifying the original link and adding a new one.

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the link to be split.

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaNewLogLinkSplit, 
 gaiaNewGeoLinkSplit, gaiaModGeoLinkSplit, gaiaNewLinkHeal,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaModLogLinkSplit (GaiaNetworkAccessorPtr
						       ptr, sqlite3_int64 link);

/**
 Split a spatial link by a node, replacing it with two new links.

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the link to be split.
 \param pt pointer to the Node Geometry.

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaNewLogLinkSplit, 
 gaiaModLogLinkSplit, gaiaModGeoLinkSplit, gaiaNewLinkHeal,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaNewGeoLinkSplit (GaiaNetworkAccessorPtr
						       ptr,
						       sqlite3_int64 link,
						       gaiaPointPtr pt);

/**
 Split a spatial link by a node, modifying the original link and adding
 a new one.

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the link to be split.
 \param pt pointer to the Node Geometry.

 \return the ID of the inserted Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaNewLogLinkSplit, 
 gaiaModLogLinkSplit, gaiaNewGeoLinkSplit, gaiaNewLinkHeal,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaModGeoLinkSplit (GaiaNetworkAccessorPtr
						       ptr,
						       sqlite3_int64 link,
						       gaiaPointPtr pt);

/**
 Heal two links by deleting the node connecting them, deleting both links,
 and replacing them with a new link whose direction is the same as the
 first link provided.

 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the first link to be healed.
 \param anotherlink the unique identifier of the second link to be healed.

 \return the ID of the removed Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaNewLogLinkSplit, 
 gaiaModLogLinkSplit, gaiaNewGeoLinkSplit, gaiaModGeoLinkSplit,
 gaiaModLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaNewLinkHeal (GaiaNetworkAccessorPtr
						   ptr, sqlite3_int64 link,
						   sqlite3_int64 anotherlink);

/**
 Heal two links by deleting the node connecting them, modfying the first
 link provided, and deleting the second link.
 * 
 \param ptr pointer to the Topology-Network Accessor Object.
 \param link the unique identifier of the first link to be healed.
 \param anotherlink the unique identifier of the second link to be healed.

 \return the ID of the removed Node; a negative number on failure.

 \sa gaiaNetworkFromDBMS, gaiaAddLink, gaiaNewLogLinkSplit, 
 gaiaModLogLinkSplit, gaiaNewGeoLinkSplit, gaiaModGeoLinkSplit,
 gaiaNewLinkHeal
 */
    GAIANET_DECLARE sqlite3_int64 gaiaModLinkHeal (GaiaNetworkAccessorPtr
						   ptr, sqlite3_int64 link,
						   sqlite3_int64 anotherlink);

/**
 Creates a temporary table containing a validation report for a given 
 Logical TopoNet.

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int gaiaValidLogicalNet (GaiaNetworkAccessorPtr ptr);

/**
 Creates a temporary table containing a validation report for a given 
 Spatial TopoNet.

 \param ptr pointer to the Topology Accessor Object.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int gaiaValidSpatialNet (GaiaNetworkAccessorPtr ptr);

/**
 Find the ID of a NetNode at a Point location

 \param ptr pointer to the Topology-Network Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of a NetNode; -1 on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE sqlite3_int64
	gaiaGetNetNodeByPoint (GaiaNetworkAccessorPtr ptr, gaiaPointPtr pt,
			       double tolerance);

/**
 Find the ID of a Link at a Point location

 \param ptr pointer to the Topology-Network Accessor Object.
 \param pt pointer to the Point Geometry.
 \param tolerance approximation factor.

 \return the ID of a Link; -1 on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE sqlite3_int64
	gaiaGetLinkByPoint (GaiaNetworkAccessorPtr ptr, gaiaPointPtr pt,
			    double tolerance);

/**
 Populates a Network by importing a whole GeoTable

 \param ptr pointer to the Network Accessor Object.
 \param db-prefix prefix of the DB containing the input GeoTable.
 If NULL the "main" DB will be intended by default.
 \param table name of the input GeoTable.
 \param column name of the input Geometry Column.
 Could be NULL is the input table has just a single Geometry Column.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNet_FromGeoTable (GaiaNetworkAccessorPtr ptr,
				  const char *db_prefix, const char *table,
				  const char *column);

/**
 Return a Point geometry (seed) identifying a Network Link

 \param ptr pointer to the Network Accessor Object.
 \param link the unique identifier of the link.

 \return pointer to Geomtry (point); NULL on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE gaiaGeomCollPtr
	gaiaGetLinkSeed (GaiaNetworkAccessorPtr ptr, sqlite3_int64 link);

/**
 Ensures that all Links on a Topology-Network will have not less
 than three vertices; for all Links found being simple two-points 
 segments a third intermediate point will be interpolated.

 \param ptr pointer to the Network Accessor Object.

 \return the total number of changed Links; a negativa number on error

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNet_DisambiguateSegmentLinks (GaiaNetworkAccessorPtr ptr);

/**
 Will update all Seeds for a Topology-Network

 \param ptr pointer to the Network Accessor Object.
 \param mode if set to 0 a full update of all Seeds will be performed,
 otherwise an incremental update will happen.

 \return 1 on success; 0 on failure.

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNetUpdateSeeds (GaiaNetworkAccessorPtr ptr, int mode);

/**
 Extracts a Simple Features Table out from a Network by matching
 Network Seeds to a given reference Table.

 \param ptr pointer to the Network Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output table to be created and populated.
 \param with_spatial_index boolean flag: if set to TRUE (non ZERO) a Spatial
 Index supporting the output table will be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNet_ToGeoTable (GaiaNetworkAccessorPtr ptr,
				const char *db_prefix, const char *ref_table,
				const char *ref_column, const char *out_table,
				int with_spatial_index);

/**
 Extracts a simplified/generalized Simple Features Table out from a Network 
 by matching Network Seeds to a given reference Table.

 \param ptr pointer to the Network Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output table to be created and populated.
 \param tolerance approximation radius required by the Douglar-Peucker
 simplification algorithm.
 \param with_spatial_index boolean flag: if set to TRUE (non ZERO) a Spatial
 Index supporting the output table will be created.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNet_ToGeoTableGeneralize (GaiaNetworkAccessorPtr ptr,
					  const char *db_prefix,
					  const char *ref_table,
					  const char *ref_column,
					  const char *out_table,
					  double tolerance,
					  int with_spatial_index);

/**
 Creates and populates a Table containing a comprehensive report
 about all intesections between the Links of some Network and
 a given reference Table of the Linestring/Multilinestring type.

 \param ptr pointer to the Network Accessor Object.
 \param db-prefix prefix of the DB containing the reference GeoTable.
 If NULL the "main" DB will be intended by default.
 \param ref_table name of the reference GeoTable.
 \param ref_column name of the reference Geometry Column.
 Could be NULL is the reference table has just a single Geometry Column.
 \param out_table name of the output output table to be created and populated.

 \return 1 on success; -1 on failure (will raise an exception).

 \sa gaiaNetworkFromDBMS
 */
    GAIANET_DECLARE int
	gaiaTopoNet_LineLinksList (GaiaNetworkAccessorPtr ptr,
				   const char *db_prefix, const char *ref_table,
				   const char *ref_column,
				   const char *out_table);


#ifdef __cplusplus
}
#endif


#endif				/* _GAIANET_H */
