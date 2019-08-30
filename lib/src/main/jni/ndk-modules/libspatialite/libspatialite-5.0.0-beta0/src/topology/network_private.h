/*
 network_private.h -- Topology-Network opaque definitions
  
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
 \file network_private.h

 SpatiaLite Topology-Network private header file
 */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifdef _WIN32
#ifdef DLL_EXPORT
#define NETWORK_PRIVATE
#else
#define NETWORK_PRIVATE
#endif
#else
#define NETWORK_PRIVATE __attribute__ ((visibility("hidden")))
#endif
#endif

struct gaia_network
{
/* a struct wrapping a Topology-Network Accessor Object */
    const void *cache;
    sqlite3 *db_handle;
    char *network_name;
    int spatial;
    int srid;
    int has_z;
    int allow_coincident;
    char *last_error_message;
    sqlite3_stmt *stmt_getNetNodeWithinDistance2D;
    sqlite3_stmt *stmt_getLinkWithinDistance2D;
    sqlite3_stmt *stmt_insertNetNodes;
    sqlite3_stmt *stmt_deleteNetNodesById;
    sqlite3_stmt *stmt_getNetNodeWithinBox2D;
    sqlite3_stmt *stmt_getNextLinkId;
    sqlite3_stmt *stmt_setNextLinkId;
    sqlite3_stmt *stmt_insertLinks;
    sqlite3_stmt *stmt_deleteLinksById;
    void *callbacks;
    void *lwn_iface;
    void *lwn_network;
    struct gaia_network *prev;
    struct gaia_network *next;
};

/* prototypes for functions handling Topology errors */
NETWORK_PRIVATE void gaianet_reset_last_error_msg (GaiaNetworkAccessorPtr
						   accessor);

NETWORK_PRIVATE void gaianet_set_last_error_msg (GaiaNetworkAccessorPtr
						 accessor, const char *msg);

NETWORK_PRIVATE const char
    *gaianet_get_last_exception (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE int auxnet_insert_into_network (GaiaNetworkAccessorPtr accessor,
						gaiaGeomCollPtr geom);


/* prototypes for functions creating some SQL prepared statement */
NETWORK_PRIVATE sqlite3_stmt
    *
do_create_stmt_getNetNodeWithinDistance2D (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_getLinkWithinDistance2D (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_insertNetNodes (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_deleteNetNodesById (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_getNetNodeWithinBox2D (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_getNextLinkId (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_setNextLinkId (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_insertLinks (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE sqlite3_stmt
    * do_create_stmt_deleteLinksById (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE void
finalize_toponet_prepared_stmts (GaiaNetworkAccessorPtr accessor);

NETWORK_PRIVATE void
create_toponet_prepared_stmts (GaiaNetworkAccessorPtr accessor);


/* common utility */
NETWORK_PRIVATE LWN_LINE
    * gaianet_convert_linestring_to_lwnline (gaiaLinestringPtr ln, int srid,
					     int has_z);


/* callback function prototypes */
const char *netcallback_lastErrorMessage (const LWN_BE_DATA * be);

int netcallback_freeNetwork (LWN_BE_NETWORK * net);

LWN_BE_NETWORK *netcallback_loadNetworkByName (const LWN_BE_DATA * be,
					       const char *name);

LWN_NET_NODE *netcallback_getNetNodeWithinDistance2D (const LWN_BE_NETWORK *
						      lwn_net,
						      const LWN_POINT * pt,
						      double dist,
						      int *numelems, int fields,
						      int limit);

LWN_LINK *netcallback_getLinkWithinDistance2D (const LWN_BE_NETWORK * net,
					       const LWN_POINT * pt,
					       double dist, int *numelems,
					       int fields, int limit);

int netcallback_updateNetNodesById (const LWN_BE_NETWORK * net,
				    const LWN_NET_NODE * nodes, int numnodes,
				    int upd_fields);

int netcallback_deleteNetNodesById (const LWN_BE_NETWORK * net,
				    const LWN_ELEMID * ids, int numnodes);

int netcallback_insertNetNodes (const LWN_BE_NETWORK * net, LWN_NET_NODE * node,
				int numelems);

LWN_NET_NODE *netcallback_getNetNodeById (const LWN_BE_NETWORK * net,
					  const LWN_ELEMID * ids, int *numelems,
					  int fields);

LWN_ELEMID netcallback_getNextLinkId (const LWN_BE_NETWORK * net);

LWN_NET_NODE *netcallback_getNetNodeWithinBox2D (const LWN_BE_NETWORK * net,
						 const LWN_BBOX * box,
						 int *numelems, int fields,
						 int limit);

LWN_ELEMID netcallback_getNextLinkId (const LWN_BE_NETWORK * lwn_net);

int netcallback_insertLinks (const LWN_BE_NETWORK * net,
			     LWN_LINK * links, int numelems);

int netcallback_updateLinksById (const LWN_BE_NETWORK * net,
				 const LWN_LINK * links, int numlinks,
				 int upd_fields);

LWN_LINK *netcallback_getLinkByNetNode (const LWN_BE_NETWORK * net,
					const LWN_ELEMID * ids, int *numelems,
					int fields);

LWN_LINK *netcallback_getLinkById (const LWN_BE_NETWORK * lwn_net,
				   const LWN_ELEMID * ids, int *numelems,
				   int fields);

int netcallback_deleteLinksById (const LWN_BE_NETWORK * net,
				 const LWN_ELEMID * ids, int numnodes);

int netcallback_netGetSRID (const LWN_BE_NETWORK * net);

int netcallback_netHasZ (const LWN_BE_NETWORK * net);

int netcallback_netIsSpatial (const LWN_BE_NETWORK * net);

int netcallback_netAllowCoincident (const LWN_BE_NETWORK * net);

const void *netcallback_netGetGEOS (const LWN_BE_NETWORK * net);
