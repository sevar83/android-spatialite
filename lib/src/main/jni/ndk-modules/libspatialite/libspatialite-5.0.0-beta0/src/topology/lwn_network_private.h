/*
 lwn_network_private.h -- private members and methods
 Topology-Network abstract multi-backend interface
  
 version 4.3, 2015 August 13

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

#ifndef LWN_NETWORK_PRIVATE_H
#define LWN_NETWORK_PRIVATE_H 1


/************************************************************************
 *
 * Generic SQL handler
 *
 ************************************************************************/

struct LWN_BE_IFACE_T
{
    const RTCTX *ctx;
    const LWN_BE_DATA *data;
    const LWN_BE_CALLBACKS *cb;
    char *errorMsg;
};

/************************************************************************
 *
 * Internal objects
 *
 ************************************************************************/

struct LWN_NETWORK_T
{
    LWN_BE_IFACE *be_iface;
    LWN_BE_NETWORK *be_net;
    int srid;
    int hasZ;
    int spatial;
    int allowCoincident;
    const void *geos_handle;
};


int lwn_be_existsCoincidentNode (const LWN_NETWORK * net, const LWN_POINT * pt);

int lwn_be_existsLinkIntersectingPoint (const LWN_NETWORK * net,
					const LWN_POINT * pt);

#endif /* LWN_NETWORK_PRIVATE_H */
