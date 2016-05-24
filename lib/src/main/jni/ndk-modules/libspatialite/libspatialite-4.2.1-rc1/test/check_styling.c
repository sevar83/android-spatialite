
/*

 check_styling.c -- SpatiaLite Test Case

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
 
Portions created by the Initial Developer are Copyright (C) 2013
the Initial Developer. All Rights Reserved.

Contributor(s):
Brad Hards <bradh@frogmouth.net>

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

#include "config.h"

#include "sqlite3.h"
#include "spatialite.h"

static unsigned char *
load_blob (const char *path, int *blob_len)
{
/* loading an external image */
    unsigned char *blob;
    int sz = 0;
    int rd;
    FILE *fl = fopen (path, "rb");
    if (!fl)
      {
	  fprintf (stderr, "cannot open \"%s\"\n", path);
	  return NULL;
      }
    if (fseek (fl, 0, SEEK_END) == 0)
	sz = ftell (fl);
    blob = (unsigned char *) malloc (sz);
    *blob_len = sz;
    rewind (fl);
    rd = fread (blob, 1, sz, fl);
    if (rd != sz)
      {
	  fprintf (stderr, "read error \"%s\"\n", path);
	  return NULL;
      }
    fclose (fl);
    return blob;
}

static unsigned char *
load_xml (const char *path, int *len)
{
/* loading an external XML */
    unsigned char *xml;
    int sz = 0;
    int rd;
    FILE *fl = fopen (path, "rb");
    if (!fl)
      {
	  fprintf (stderr, "cannot open \"%s\"\n", path);
	  return NULL;
      }
    if (fseek (fl, 0, SEEK_END) == 0)
	sz = ftell (fl);
    xml = malloc (sz + 1);
    *len = sz;
    rewind (fl);
    rd = fread (xml, 1, sz, fl);
    if (rd != sz)
      {
	  fprintf (stderr, "read error \"%s\"\n", path);
	  return NULL;
      }
    fclose (fl);
    xml[rd] = '\0';
    return xml;
}

static char *
build_hex_blob (const unsigned char *blob, int blob_len)
{
/* building an HEX blob */
    int i;
    const unsigned char *p_in = blob;
    char *hex = malloc ((blob_len * 2) + 1);
    char *p_out = hex;
    for (i = 0; i < blob_len; i++)
      {
	  sprintf (p_out, "%02x", *p_in);
	  p_in++;
	  p_out += 2;
      }
    return hex;
}

int
main (int argc, char *argv[])
{
    int ret;
    sqlite3 *handle;
    char *err_msg = NULL;
    char **results;
    int rows;
    int columns;
    unsigned char *blob;
    int blob_len;
    char *hexBlob;
    unsigned char *xml;
    int len;
    char *sql;
    void *cache = spatialite_alloc_connection ();

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

    ret =
	sqlite3_open_v2 (":memory:", &handle,
			 SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "cannot open in-memory db: %s\n",
		   sqlite3_errmsg (handle));
	  sqlite3_close (handle);
	  return -1;
      }

    spatialite_init_ex (handle, cache, 0);

    ret =
	sqlite3_exec (handle, "SELECT InitSpatialMetadata(1, 'WGS84')", NULL,
		      NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Unexpected InitSpatialMetadata result: %i, (%s)\n",
		   ret, err_msg);
	  sqlite3_free (err_msg);
	  return -2;
      }

#ifdef ENABLE_LIBXML2		/* only if LIBXML2 is supported */

    ret =
	sqlite3_get_table (handle, "SELECT CreateStylingTables(1)", &results,
			   &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error CreateStylingTables: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -3;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -4;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #0 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -5;
      }
    sqlite3_free_table (results);

    blob = load_blob ("empty.png", &blob_len);
    if (blob == NULL)
	return -6;
    hexBlob = build_hex_blob (blob, blob_len);
    free (blob);
    if (hexBlob == NULL)
	return -7;
    sql =
	sqlite3_mprintf ("SELECT RegisterExternalGraphic('url-A', x%Q)",
			 hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterExternalGraphic #1: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -8;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -9;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #1 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -10;
      }
    sqlite3_free_table (results);

    sql =
	sqlite3_mprintf
	("SELECT RegisterExternalGraphic('url-A', x%Q, 'title', 'abstract', 'file_name')",
	 hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    free (hexBlob);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterExternalGraphic #2: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -11;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -12;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #2 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -13;
      }
    sqlite3_free_table (results);

    xml = load_xml ("thunderstorm_mild.svg", &len);
    if (xml == NULL)
	return -14;
    gaiaXmlToBlob (cache, xml, len, 1, NULL, &blob, &blob_len, NULL, NULL);
    free (xml);
    if (blob == NULL)
      {
	  fprintf (stderr, "this is not a well-formed XML !!!\n");
	  return -15;
      }
    hexBlob = build_hex_blob (blob, blob_len);
    free (blob);
    if (hexBlob == NULL)
	return -16;
    sql =
	sqlite3_mprintf
	("SELECT RegisterExternalGraphic('url-B', x%Q, 'title', 'abstract', 'file_name')",
	 hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterExternalGraphic #3: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -17;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -18;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #3 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -19;
      }
    sqlite3_free_table (results);

    sql =
	sqlite3_mprintf ("SELECT RegisterExternalGraphic('url-B', x%Q)",
			 hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    free (hexBlob);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterExternalGraphic #4: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -20;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -21;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #4 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -22;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_exec (handle,
		      "CREATE TABLE table1 (id INTEGER PRIMARY KEY AUTOINCREMENT)",
		      NULL, NULL, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error Create Table table1: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -23;
      }
    ret =
	sqlite3_get_table (handle,
			   "SELECT AddGeometryColumn('table1', 'geom', 4326, 'POINT', 'XY')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error AddGeometryColumn: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -24;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -25;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #5 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -26;
      }
    sqlite3_free_table (results);

    xml = load_xml ("stazioni_se.xml", &len);
    if (xml == NULL)
	return -27;
    gaiaXmlToBlob (cache, xml, len, 1, NULL, &blob, &blob_len, NULL, NULL);
    free (xml);
    if (blob == NULL)
      {
	  fprintf (stderr, "this is not a well-formed XML !!!\n");
	  return -28;
      }
    hexBlob = build_hex_blob (blob, blob_len);
    free (blob);
    if (hexBlob == NULL)
	return -29;
    sql =
	sqlite3_mprintf
	("SELECT RegisterVectorStyledLayer('table1', 'geom', x%Q)", hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterVectorStyledLayer #6: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -30;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -31;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #6 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -32;
      }
    sqlite3_free_table (results);

    sql =
	sqlite3_mprintf
	("SELECT RegisterVectorStyledLayer('table1', 'geom', 0, x%Q)", hexBlob);
    free (hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterVectorStyledLayer #7: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -33;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -34;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #7 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -35;
      }
    sqlite3_free_table (results);

    xml = load_xml ("raster_se.xml", &len);
    if (xml == NULL)
	return -36;
    gaiaXmlToBlob (cache, xml, len, 1, NULL, &blob, &blob_len, NULL, NULL);
    free (xml);
    if (blob == NULL)
      {
	  fprintf (stderr, "this is not a well-formed XML !!!\n");
	  return -37;
      }
    hexBlob = build_hex_blob (blob, blob_len);
    free (blob);
    if (hexBlob == NULL)
	return -38;
    sql =
	sqlite3_mprintf ("SELECT RegisterRasterStyledLayer('srtm', x%Q)",
			 hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterRasterStyledLayer #8: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -39;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -40;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #8 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -41;
      }
    sqlite3_free_table (results);

    sql =
	sqlite3_mprintf ("SELECT RegisterRasterStyledLayer('srtm', 0, x%Q)",
			 hexBlob);
    free (hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterRasterStyledLayer #9: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -42;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -43;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #9 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -44;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT RegisterStyledGroup('group', 'srtm')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterStyledGroup #10: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -45;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -46;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #10 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -47;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT RegisterStyledGroup('group', 'table1', 'geom')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterStyledGroup #11: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -48;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -49;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #12 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -50;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT RegisterStyledGroup('group', 'srtm', 4)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterStyledGroup #13: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -51;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -52;
      }
    if (strcmp (results[1 * columns + 0], "0") != 0)
      {
	  fprintf (stderr, "Unexpected #13 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -53;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT RegisterStyledGroup('group', 'table1', 'geom', 1)",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterStyledGroup #14: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -54;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -55;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #14 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -56;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT SetStyledGroupInfos('group', 'title', 'abstract')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error SetStyledGroupInfos #15: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -57;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -58;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #15 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -59;
      }
    sqlite3_free_table (results);

    ret =
	sqlite3_get_table (handle,
			   "SELECT SetStyledGroupInfos('group-bis', 'title', 'abstract')",
			   &results, &rows, &columns, &err_msg);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error SetStyledGroupInfos #16: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return 60;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -61;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #16 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -62;
      }
    sqlite3_free_table (results);

    xml = load_xml ("sld_sample.xml", &len);
    if (xml == NULL)
	return -63;
    gaiaXmlToBlob (cache, xml, len, 1, NULL, &blob, &blob_len, NULL, NULL);
    free (xml);
    if (blob == NULL)
      {
	  fprintf (stderr, "this is not a well-formed XML !!!\n");
	  return -64;
      }
    hexBlob = build_hex_blob (blob, blob_len);
    free (blob);
    if (hexBlob == NULL)
	return -65;
    sql = sqlite3_mprintf ("SELECT RegisterGroupStyle('group', x%Q)", hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterGroupStyle #1: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -66;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -67;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #1 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -68;
      }
    sqlite3_free_table (results);

    sql =
	sqlite3_mprintf ("SELECT RegisterGroupStyle('group', 0, x%Q)", hexBlob);
    free (hexBlob);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &err_msg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Error RegisterGroupStyle #2: %s\n", err_msg);
	  sqlite3_free (err_msg);
	  return -69;
      }
    if ((rows != 1) || (columns != 1))
      {
	  sqlite3_free_table (results);
	  fprintf (stderr, "Unexpected row / column count: %i x %i\n", rows,
		   columns);
	  return -70;
      }
    if (strcmp (results[1 * columns + 0], "1") != 0)
      {
	  fprintf (stderr, "Unexpected #2 result (got %s, expected 1)",
		   results[1 * columns + 0]);
	  sqlite3_free_table (results);
	  return -71;
      }
    sqlite3_free_table (results);


#endif

    ret = sqlite3_close (handle);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_close() error: %s\n",
		   sqlite3_errmsg (handle));
	  return -57;
      }

    spatialite_cleanup_ex (cache);

    spatialite_shutdown ();
    return 0;
}
