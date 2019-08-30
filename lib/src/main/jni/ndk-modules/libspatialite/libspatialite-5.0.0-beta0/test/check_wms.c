/*

 check_wms.c -- SpatiaLite Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

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
 
Portions created by the Initial Developer are Copyright (C) 2016
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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

static int
do_level0_tests (sqlite3 * handle, int *retcode)
{
/* performing Level 0 tests */
    int ret;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;
    const char *url;

/* creating WMS support tables */
    ret =
	sqlite3_get_table (handle, "SELECT WMS_CreateTables()", &results, &rows,
			   &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_CreateTables() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -21;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "WMS_CreateTables() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -22;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_CreateTables() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -23;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a first WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetCapabilities('urlcapab_alpha')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetCapability() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -24;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -25;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -26;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a second WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetCapabilities('urlcapab_beta')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetCapability() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -27;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -28;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -29;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a third WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetCapabilities('urlcapab_gamma', 'title', 'abstract')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetCapability() #3 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -30;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #3 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -31;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetCapability() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -32;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering few WMS GetMap related to the first WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_alpha', 'urlmap_alpha_zero', 'layer', '1.3.0', 'EPSG:3003', 'image/jpeg', 'default', 2, 2)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -33;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -34;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -35;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_alpha', 'urlmap_alpha_one', 'layer', '1.3.0', 'EPSG:3003', 'image/png', 'default', 1, 0)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -36;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -37;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -38;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_alpha', 'urlmap_alpha_two', 'layer', '1.3.0', 'EPSG:3003', 'image/jpeg', 'rgb', 0, 0)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -39;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #3 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -40;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -41;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering a WMS GetMap related to the second WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_beta', 'urlmap_beta', 'layer', 'title', 'abstract', '1.1.1', 'EPSG:4326', 'image/jpeg', 'default', -1, -1, 1, 1, 64, 64, 'ffffff', 1, 'getfeatureinfo')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -42;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #4 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -43;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #4 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -44;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering few WMS GetMap related to the third WMS GetCapabilities */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_gamma', 'urlmap_gamma_zero', 'layer', '1.3.0', 'EPSG:3003', 'image/jpeg', 'default', 0, 0)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -45;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #5 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -46;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #5 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -47;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterGetMap('urlcapab_gamma', 'urlmap_gamma_one', 'layer', '1.3.0', 'EPSG:3003', 'image/png', 'default', 1, 0, 1, -1, 6000, 6000)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #6 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -48;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterGetMap() #6 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -49;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterGetMap() #6 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -50;
	  return 0;
      }
    sqlite3_free_table (results);

/* registering few WMS GetMap Settings */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterSetting('urlmap_gamma_zero', 'layer', 'VERSION', '1.1.0');"
			   "SELECT WMS_RegisterSetting('urlmap_gamma_zero', 'layer', 'FORMAT', 'image/jpeg')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -51;
	  return 0;
      }
    if (rows != 2 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterSetting() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -52;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1 || atoi (*(results + 2)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -53;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterSetting('urlmap_gamma_zero', 'layer', 'FORMAT', 'image/png')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -54;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterSetting() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -55;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -56;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterRefSys('urlmap_gamma_zero', 'layer', 'EPSG:3004', 1000.5, 1001.5, 2000.5, 2001.5);"
			   "SELECT WMS_RegisterRefSys('urlmap_gamma_zero', 'layer', 'EPSG:3003', 2000.5, 2001.5, 3000.5, 3001.5)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterRefSys() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -57;
	  return 0;
      }
    if (rows != 2 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterRefSys() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -58;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1 || atoi (*(results + 2)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterRefSys() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -59;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterRefSys('urlmap_gamma_zero', 'layer', 'EPSG:4326', 30, 60, 35, 65, 1)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterRefSys() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -60;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterRefSys() #2 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -61;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterRefSys() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -62;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_RegisterSetting('urlmap_gamma_zero', 'layer', 'STYLE', 'gray', 3)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #5 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -63;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_RegisterSetting() #5 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -64;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_RegisterSetting() #5 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -65;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing WMS GetCapabilities Info */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetCapabilitiesInfos('urlcapab_alpha', 'new title', 'new abstract')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetCapabilitiesInfos() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -66;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetCapabilitiesInfos() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -67;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetCapabilitiesInfos() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -68;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing WMS GetMap Info */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapInfos('urlmap_alpha_zero', 'layer', 'new title', 'new abstract')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapInfos() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -69;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapInfos() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -70;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapInfos() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -71;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating a WMS GetMap Default Setting */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_DefaultSetting('urlmap_gamma_zero', 'layer', 'FORMAT', 'image/jpeg')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_DefaultSetting() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -72;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_DefaultSetting() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -73;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_DefaultSetting() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -74;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating a WMS GetMap Default Setting */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_DefaultRefSys('urlmap_gamma_zero', 'layer', 'EPSG:3004')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_DefaultRefSys() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -75;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_DefaultRefSys() #1 error: rows=%d columns=%d\n", rows,
		   columns);
	  sqlite3_free_table (results);
	  *retcode = -76;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_DefaultRefSys() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -77;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a WMS GetMap Setting */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_UnRegisterSetting('urlmap_gamma_zero', 'layer', 'FORMAT', 'image/png')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_UnRegisterSetting() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -78;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_UnRegisterSetting() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -79;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_UnRegisterSetting() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -80;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a WMS GetMap SRS */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_UnRegisterRefSys('urlmap_gamma_zero', 'layer', 'EPSG:3003')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_UnRegisterRefSys() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -81;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_UnRegisterRefSys() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -82;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_UnRegisterRefSys() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -83;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a WMS GetMap */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_UnRegisterGetMap('urlmap_alpha_one', 'layer')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_UnRegisterGetMap() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -84;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_UnRegisterGetMap() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -85;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_UnRegisterGetMap() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -86;
	  return 0;
      }
    sqlite3_free_table (results);

/* deleting a WMS GetMap */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_UnRegisterGetCapabilities('urlcapab_beta')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_UnRegisterGetCapabilities() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -87;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_UnRegisterGetCapabilities() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -88;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr,
		   "WMS_UnRegisterGetCapabilities() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -89;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating WMS GetMap Options - BgColor*/
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapOptions('urlmap_alpha_two', 'layer', 'ffffd0')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapOptions() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -91;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -92;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating WMS GetMap Options - Flags*/
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapOptions('urlmap_alpha_two', 'layer', 2, 2)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapOptions() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -91;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -92;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating WMS GetMap Options - Queryable */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapOptions('urlmap_alpha_two', 'layer', 2, 'getfeatureinfo')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapOptions() #3 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -91;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -92;
	  return 0;
      }
    sqlite3_free_table (results);

/* updating WMS GetMap Options - Tiled */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapOptions('urlmap_alpha_two', 'layer', 2, 2, 64, 64)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #4 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -90;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapOptions() #4 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -91;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapOptions() #4 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -92;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing WMS GetMap request URL */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_GetMapRequestURL('urlmap_alpha_two', 'layer', 1024, 1024, 30, 40, 50, 60)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_GetMapRequestURL() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -93;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_GetMapRequestURL() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -94;
	  return 0;
      }
    url = *(results + 1);
    if (url == NULL)
      {
	  fprintf (stderr,
		   "WMS_GetMapRequestURL() #1 error: unexpected NULL\n");
	  *retcode = -95;
	  return 0;
      }
    if (strcmp
	(url,
	 "urlmap_alpha_two?SERVICE=WMS&REQUEST=GetMap&VERSION=1.3.0&LAYERS=layer&CRS=EPSG:3003&BBOX=40.000000,30.000000,60.000000,50.000000&WIDTH=1024&HEIGHT=1024&STYLES=rgb&FORMAT=image/jpeg&TRANSPARENT=TRUE&BGCOLOR=0xffffd0")
	!= 0)
      {
	  fprintf (stderr, "WMS_GetMapRequestURL() #1 unexpected result: %s\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -96;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_GetMapRequestURL('urlmap_gamma_one', 'layer', 1024, 1024, 30.5, 40.5, 50.5, 60.5)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_GetMapRequestURL() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -97;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_GetMapRequestURL() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -98;
	  return 0;
      }
    url = *(results + 1);
    if (url == NULL)
      {
	  fprintf (stderr,
		   "WMS_GetMapRequestURL() #2 error: unexpected NULL\n");
	  *retcode = -99;
	  return 0;
      }
    if (strcmp
	(url,
	 "urlmap_gamma_one?SERVICE=WMS&REQUEST=GetMap&VERSION=1.3.0&LAYERS=layer&CRS=EPSG:3003&BBOX=30.500000,40.500000,50.500000,60.500000&WIDTH=1024&HEIGHT=1024&STYLES=default&FORMAT=image/png&TRANSPARENT=TRUE")
	!= 0)
      {
	  fprintf (stderr, "WMS_GetMapRequestURL() #2 unexpected result: %s\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -100;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing WMS GetFeatureInfo request URL */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_GetFeatureInfoRequestURL('urlmap_alpha_two', 'layer', 1024, 1024, 100, 200, 30, 40, 50, 60, 33)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_GetFeatureInfoRequestURL() #1 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -101;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_GetFeatureInfoRequestURL() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -102;
	  return 0;
      }
    url = *(results + 1);
    if (url == NULL)
      {
	  fprintf (stderr,
		   "WMS_GetFeatureInfoRequestURL() #1 error: unexpected NULL\n");
	  *retcode = -103;
	  return 0;
      }
    if (strcmp
	(url,
	 "getfeatureinfo?SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&QUERY_LAYERS=layer&CRS=EPSG:3003&BBOX=40.000000,30.000000,60.000000,50.000000&WIDTH=1024&HEIGHT=1024&X=100&Y=200&FEATURE_COUNT=33")
	!= 0)
      {
	  fprintf (stderr,
		   "WMS_GetFeatureInfoRequestURL() #1 unexpected result: %s\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -104;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_GetFeatureInfoRequestURL('urlmap_gamma_one', 'layer', 1024, 1024, 100, 200, 30.5, 40.5, 50.5, 60.5)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_GetFeatureInfoRequestURL() #2 error: %s\n",
		   err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -105;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_GetFeatureInfoRequestURL() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -106;
	  return 0;
      }
    url = *(results + 1);
    if (url != NULL)
      {
	  fprintf (stderr,
		   "WMS_GetFeatureInfoRequestURL() #2 unexpected result: %s\n",
		   *(results + 1));
	  sqlite3_free_table (results);
	  *retcode = -107;
	  return 0;
      }
    sqlite3_free_table (results);

/* re-creating WMS support tables */
    ret =
	sqlite3_get_table (handle, "SELECT WMS_CreateTables()", &results, &rows,
			   &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_CreateTables() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -108;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr, "WMS_CreateTables() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -109;
	  return 0;
      }
    if (atoi (*(results + 1)) != 0)
      {
	  fprintf (stderr, "WMS_CreateTables() #2 unexpected success\n");
	  sqlite3_free_table (results);
	  *retcode = -110;
	  return 0;
      }
    sqlite3_free_table (results);

/* testing WMS GetMap Copyright and License */
    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapCopyright('urlmap_alpha_zero', 'layer', 'somebody')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -111;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapCopyright() #1 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -112;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #1 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -113;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapCopyright('urlmap_alpha_zero', 'layer', 'someone else', 'CC BY-SA 3.0')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #2 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -114;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapCopyright() #2 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -115;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #2 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -116;
	  return 0;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT WMS_SetGetMapCopyright('urlmap_alpha_zero', 'layer', NULL, 'CC BY-SA 4.0')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #3 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  *retcode = -117;
	  return 0;
      }
    if (rows != 1 || columns != 1)
      {
	  fprintf (stderr,
		   "WMS_SetGetMapCopyright() #3 error: rows=%d columns=%d\n",
		   rows, columns);
	  sqlite3_free_table (results);
	  *retcode = -118;
	  return 0;
      }
    if (atoi (*(results + 1)) != 1)
      {
	  fprintf (stderr, "WMS_SetGetMapCopyright() #3 unexpected failure\n");
	  sqlite3_free_table (results);
	  *retcode = -119;
	  return 0;
      }
    sqlite3_free_table (results);

    return 1;
}

int
main (int argc, char *argv[])
{
    int retcode = 0;
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open \":memory:\" database: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    ret = sqlite3_exec (handle, "PRAGMA foreign_keys=1", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "PRAGMA foreign_keys=1 error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -2;
      }

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1)", NULL, NULL,
		      &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "InitSpatialMetadata() error: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  sqlite3_close (handle);
	  return -3;
      }

/*tests: level 0 */
    if (!do_level0_tests (handle, &retcode))
	goto end;

  end:
    spatialite_finalize_topologies (cache);
    sqlite3_close (handle);
    spatialite_cleanup_ex (cache);
    spatialite_shutdown ();
    return retcode;
}
