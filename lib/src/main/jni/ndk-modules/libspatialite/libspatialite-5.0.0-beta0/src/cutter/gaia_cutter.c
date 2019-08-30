/*

 gaia_cutter.c -- implementation of the Cutter module
    
 version 4.3, 2015 July 2

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

/*
 
CREDITS:

this module has been completely funded by:
Regione Toscana - Settore Sistema Informativo Territoriale ed Ambientale
(Cutter module)

CIG: 6038019AE5 

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32) && !defined(__MINGW32__)
#include "process.h"
#else
#include "unistd.h"
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#include <spatialite/sqlite.h>
#include <spatialite/debug.h>

#include <spatialite.h>
#include <spatialite_private.h>
#include <spatialite/gaiaaux.h>

#ifndef OMIT_GEOS		/* only if GEOS is enabled */

#if defined(_WIN32) && !defined(__MINGW32__)
#define strcasecmp    _stricmp
#define strncasecmp    _strnicmp
#define pid_t	int
#endif

#define GAIA_CUTTER_OUTPUT_PK	1
#define GAIA_CUTTER_INPUT_PK	2
#define GAIA_CUTTER_BLADE_PK	3
#define GAIA_CUTTER_NORMAL		4

#define GAIA_CUTTER_POINT		1
#define GAIA_CUTTER_LINESTRING	2
#define GAIA_CUTTER_POLYGON		3

struct output_column
{
/* a struct wrapping an Output Table Column */
    char *base_name;
    char *real_name;
    char *type;
    int notnull;
    int role;
    int order_nr;
    struct output_column *next;
};

struct output_table
{
/* a struct wrapping the Output Table */
    struct output_column *first;
    struct output_column *last;
};

struct multivar
{
/* a struct wrapping a generic SQLite value */
    int progr_id;
    int type;
    union multivalue
    {
	sqlite3_int64 intValue;
	double doubleValue;
	char *textValue;
    } value;
    struct multivar *next;
};

struct temporary_row
{
/* a struct wrapping a row from the TMP #1 resultset */
    struct multivar *first_input;
    struct multivar *last_input;
    struct multivar *first_blade;
    struct multivar *last_blade;
};

struct child_row
{
/* a struct wrapping a resultset row */
    struct multivar *first;
    struct multivar *last;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr geom2;
    struct child_row *next;
};

struct pending_rows
{
/* a struct containing groups of pending rows to be processed */
    struct multivar *first;
    struct multivar *last;
    gaiaGeomCollPtr geom;
    gaiaGeomCollPtr geom2;
    struct child_row *first_child;
    struct child_row *last_child;
};

struct cut_item
{
/* a struct wrapping a Cut Solution Item */
    struct multivar *first;
    struct multivar *last;
    gaiaGeomCollPtr geom;
    struct cut_item *next;
};

struct cut_solution
{
/* a struct containing a Cut Solution */
    struct cut_item *first;
    struct cut_item *last;
};

static struct multivar *
alloc_multivar (void)
{
/* allocating a NULL SQLite value */
    struct multivar *var = malloc (sizeof (struct multivar));
    var->progr_id = 0;
    var->type = SQLITE_NULL;
    var->value.textValue = NULL;
    var->next = NULL;
    return var;
}

static void
destroy_multivar (struct multivar *var)
{
/* destroying a generic SQLite values */
    if (var == NULL)
	return;
    if (var->type == SQLITE_TEXT)
      {
	  if (var->value.textValue != NULL)
	      free (var->value.textValue);
      }
    free (var);
}

static void
reset_temporary_row (struct temporary_row *row)
{
/* memory cleanup - resetting a temporary row */
    struct multivar *var;
    struct multivar *var_n;
    if (row == NULL)
	return;

    var = row->first_input;
    while (var != NULL)
      {
	  var_n = var->next;
	  destroy_multivar (var);
	  var = var_n;
      }
    var = row->first_blade;
    while (var != NULL)
      {
	  var_n = var->next;
	  destroy_multivar (var);
	  var = var_n;
      }
}

static void
do_set_null_blade_columns (struct temporary_row *row)
{
/* setting a NULL value for all Blade PK columns */
    struct multivar *var;
    if (row == NULL)
	return;

    var = row->first_blade;
    while (var != NULL)
      {
	  if (var->type == SQLITE_TEXT)
	    {
		if (var->value.textValue != NULL)
		    free (var->value.textValue);
		var->value.textValue = NULL;
	    }
	  var->type = SQLITE_NULL;
	  var = var->next;
      }
}

static struct multivar *
find_input_pk_value (struct temporary_row *row, int idx)
{
/* finding an Input column value by its relative position */
    int count = 0;
    struct multivar *var;
    if (row == NULL)
	return NULL;

    var = row->first_input;
    while (var != NULL)
      {
	  if (count == idx)
	      return var;
	  count++;
	  var = var->next;
      }
    return NULL;
}

static struct multivar *
find_blade_pk_value (struct temporary_row *row, int idx)
{
/* finding a Bladecolumn value by its relative position */
    int count = 0;
    struct multivar *var;
    if (row == NULL)
	return NULL;

    var = row->first_blade;
    while (var != NULL)
      {
	  if (count == idx)
	      return var;
	  count++;
	  var = var->next;
      }
    return NULL;
}

static void
add_int_pk_value (struct temporary_row *row, char table, int progr_id,
		  sqlite3_int64 value)
{
/* storing an INT-64 value into a ROW object */
    struct multivar *var = alloc_multivar ();
    var->progr_id = progr_id;
    var->type = SQLITE_INTEGER;
    var->value.intValue = value;
    if (table == 'B')
      {
	  /* from the Blade Table */
	  if (row->first_blade == NULL)
	      row->first_blade = var;
	  if (row->last_blade != NULL)
	      row->last_blade->next = var;
	  row->last_blade = var;
      }
    else
      {
	  /* from the Input Table */
	  if (row->first_input == NULL)
	      row->first_input = var;
	  if (row->last_input != NULL)
	      row->last_input->next = var;
	  row->last_input = var;
      }
}

static void
add_double_pk_value (struct temporary_row *row, char table, int progr_id,
		     double value)
{
/* storing a FLOAT DOUBLE value into a ROW object */
    struct multivar *var = alloc_multivar ();
    var->progr_id = progr_id;
    var->type = SQLITE_FLOAT;
    var->value.doubleValue = value;
    if (table == 'B')
      {
	  /* from the Blade Table */
	  if (row->first_blade == NULL)
	      row->first_blade = var;
	  if (row->last_blade != NULL)
	      row->last_blade->next = var;
	  row->last_blade = var;
      }
    else
      {
	  /* from the Input Table */
	  if (row->first_input == NULL)
	      row->first_input = var;
	  if (row->last_input != NULL)
	      row->last_input->next = var;
	  row->last_input = var;
      }
}

static void
add_text_pk_value (struct temporary_row *row, char table, int progr_id,
		   const char *value)
{
/* storing a TEXT value into a ROW object */
    int len;
    struct multivar *var = alloc_multivar ();
    var->progr_id = progr_id;
    var->type = SQLITE_TEXT;
    len = strlen (value);
    var->value.textValue = malloc (len + 1);
    strcpy (var->value.textValue, value);
    if (table == 'B')
      {
	  /* from the Blade Table */
	  if (row->first_blade == NULL)
	      row->first_blade = var;
	  if (row->last_blade != NULL)
	      row->last_blade->next = var;
	  row->last_blade = var;
      }
    else
      {
	  /* from the Input Table */
	  if (row->first_input == NULL)
	      row->first_input = var;
	  if (row->last_input != NULL)
	      row->last_input->next = var;
	  row->last_input = var;
      }
}

static void
add_null_pk_value (struct temporary_row *row, char table, int progr_id)
{
/* storing a NULL value into a ROW object */
    struct multivar *var = alloc_multivar ();
    var->progr_id = progr_id;
    if (table == 'B')
      {
	  /* from the Blade Table */
	  if (row->first_blade == NULL)
	      row->first_blade = var;
	  if (row->last_blade != NULL)
	      row->last_blade->next = var;
	  row->last_blade = var;
      }
    else
      {
	  /* from the Input Table */
	  if (row->first_input == NULL)
	      row->first_input = var;
	  if (row->last_input != NULL)
	      row->last_input->next = var;
	  row->last_input = var;
      }
}

static int
eval_multivar (struct multivar *var1, struct multivar *var2)
{
/* comparing to generic SQLite values */
    if (var1->type != var2->type)
	return 0;
    switch (var1->type)
      {
      case SQLITE_INTEGER:
	  if (var1->value.intValue == var2->value.intValue)
	      return 1;
	  break;
      case SQLITE_FLOAT:
	  if (var1->value.doubleValue == var2->value.doubleValue)
	      return 1;
	  break;
      case SQLITE_TEXT:
	  if (strcmp (var1->value.textValue, var2->value.textValue) == 0)
	      return 1;
	  break;
      default:
	  return 1;
      };
    return 0;
}

static int
check_same_input (struct temporary_row *prev_row, struct temporary_row *row)
{
/* checks if the current row is the same of the previous row - Input PK columns */
    int ok = 1;
    struct multivar *var1;
    struct multivar *var2;

    var1 = prev_row->first_input;
    var2 = row->first_input;
    while (1)
      {
	  if (var1 == NULL)
	    {
		ok = 0;
		break;
	    }
	  if (var2 == NULL)
	    {
		ok = 0;
		break;
	    }
	  if (!eval_multivar (var1, var2))
	    {
		ok = 0;
		break;
	    }
	  var1 = var1->next;
	  var2 = var2->next;
	  if (var1 == NULL && var2 == NULL)
	      break;
      }
    return ok;
}

static void
copy_input_values (struct temporary_row *orig, struct temporary_row *dest)
{
/* copying all values from origin to destination */
    struct multivar *var;
    int pos = 0;

    reset_temporary_row (dest);
    dest->first_input = NULL;
    dest->last_input = NULL;
    dest->first_blade = NULL;
    dest->last_blade = NULL;
    var = orig->first_input;
    while (var)
      {
	  switch (var->type)
	    {
	    case SQLITE_INTEGER:
		add_int_pk_value (dest, 'I', pos, var->value.intValue);
		break;
	    case SQLITE_FLOAT:
		add_double_pk_value (dest, 'I', pos, var->value.doubleValue);
		break;
	    case SQLITE_TEXT:
		add_text_pk_value (dest, 'I', pos, var->value.textValue);
		break;
	    default:
		add_null_pk_value (dest, 'I', pos);
	    };
	  pos++;
	  var = var->next;
      }
}

static struct output_column *
alloc_output_table_column (const char *name, const char *type, int notnull,
			   int role, int order_nr)
{
/* creating an Output Table Column */
    int len;
    struct output_column *col = malloc (sizeof (struct output_column));
    if (col == NULL)
	return NULL;
    len = strlen (name);
    col->base_name = malloc (len + 1);
    strcpy (col->base_name, name);
    col->real_name = NULL;
    len = strlen (type);
    col->type = malloc (len + 1);
    strcpy (col->type, type);
    col->notnull = notnull;
    col->role = role;
    col->order_nr = order_nr;
    col->next = NULL;
    return col;
}

static void
destroy_output_table_column (struct output_column *col)
{
/* destroying an Output Table Column */
    if (col == NULL)
	return;
    if (col->base_name != NULL)
	free (col->base_name);
    if (col->real_name != NULL)
	sqlite3_free (col->real_name);
    if (col->type != NULL)
	free (col->type);
    free (col);
}

static struct output_table *
alloc_output_table (void)
{
/* creating the Output Table struct */
    struct output_table *ptr = malloc (sizeof (struct output_table));
    if (ptr == NULL)
	return NULL;
    ptr->first = NULL;
    ptr->last = NULL;
    return ptr;
}

static void
destroy_output_table (struct output_table *tbl)
{
/* memory cleanup - destroying the Output Table struct */
    struct output_column *col;
    struct output_column *col_n;

    if (tbl == NULL)
	return;
    col = tbl->first;
    while (col != NULL)
      {
	  col_n = col->next;
	  destroy_output_table_column (col);
	  col = col_n;
      }
    free (tbl);
}

static struct output_column *
add_column_to_output_table (struct output_table *tbl, const char *name,
			    const char *type, int notnull, int role,
			    int order_nr)
{
/* adding a column to the Output Table */
    struct output_column *col;

    if (tbl == NULL)
	return NULL;
    col = alloc_output_table_column (name, type, notnull, role, order_nr);
    if (col == NULL)
	return NULL;
    if (tbl->first == NULL)
	tbl->first = col;
    if (tbl->last != NULL)
	tbl->last->next = col;
    tbl->last = col;
    return col;
}

static void
do_reset_message (char **ptr)
{
/* resetting the message string */
    if (ptr == NULL)
	return;
    if (*ptr != NULL)
	sqlite3_free (*ptr);
    *ptr = NULL;
}

static void
do_update_message (char **ptr, const char *message)
{
/* updating the message string */
    if (ptr == NULL)
	return;
    if (*ptr != NULL)
	return;
    *ptr = sqlite3_mprintf ("%s", message);
}

static void
do_update_sql_error (char **ptr, const char *message, const char *sql_err)
{
/* printing some SQL error */
    if (ptr == NULL)
	return;
    if (*ptr != NULL)
	return;
    *ptr = sqlite3_mprintf ("%s %s", message, sql_err);
}

static void
do_print_message2 (char **ptr, const char *message, const char *prefix,
		   const char *table)
{
/* printing the message string - 2 args */
    if (ptr == NULL)
	return;
    if (*ptr != NULL)
	return;
    *ptr = sqlite3_mprintf (message, prefix, table);
}

static void
do_print_message3 (char **ptr, const char *message, const char *prefix,
		   const char *table, const char *geometry)
{
/* printing the message string - 3 args */
    if (ptr == NULL)
	return;
    if (*ptr != NULL)
	return;
    *ptr = sqlite3_mprintf (message, prefix, table, geometry);
}

static int
do_check_input (sqlite3 * handle, const char *db_prefix, const char *table,
		const char *xgeometry, char **geometry, int *srid, int *type,
		char **message)
{
/* checking the input table for validity */
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    char *geom_name = NULL;
    int geom_srid = -1;
    int geom_type = -1;
    int pk = 0;
    int i;
    int ret;

    *geometry = NULL;
    *srid = -1;
    *type = GAIA_UNKNOWN;
    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* checking if the table really exists */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 5];
	  if (atoi (value) > 0)
	      pk = 1;
	  count++;
      }
    sqlite3_free_table (results);
    if (count == 0)
      {
	  do_print_message2 (message, "ERROR: table %s.%s does not exists",
			     db_prefix, table);
	  goto error;
      }
    if (pk == 0)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s lacks any Primary Key",
			     db_prefix, table);
	  goto error;
      }

/* checking geometry_columns */
    count = 0;
    if (xgeometry == NULL)
	sql = sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			       "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
			       xprefix, table, xgeometry);
    else
	sql = sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			       "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) "
			       "AND Lower(f_geometry_column) = Lower(%Q)",
			       xprefix, table, xgeometry);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT geometry_columns", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  if (count == 0)
	    {
		int len = strlen (value);
		geom_name = malloc (len + 1);
		strcpy (geom_name, value);
	    }
	  value = results[(i * columns) + 1];
	  geom_srid = atoi (value);
	  value = results[(i * columns) + 2];
	  geom_type = atoi (value);
	  count++;
      }
    sqlite3_free_table (results);
    if (count == 0)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s lacks any registered Geometry",
			     db_prefix, table);
	  goto error;
      }
    if (count > 1)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s has multiple Geometries and a NULL name was passed",
			     db_prefix, table);
	  goto error;
      }
    switch (geom_type)
      {
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
      case GAIA_MULTIPOINT:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTIPOLYGON:
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_POINTM:
      case GAIA_LINESTRINGM:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTIPOLYGONM:
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
	  break;
      default:
	  do_print_message3 (message,
			     "ERROR: table %s.%s Geometry %s has an invalid Type",
			     db_prefix, table, geom_name);
	  goto error;
      };
    *geometry = geom_name;
    *srid = geom_srid;
    *type = geom_type;

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    if (geom_name != NULL)
	free (geom_name);
    return 0;
}

static int
do_check_blade (sqlite3 * handle, const char *db_prefix, const char *table,
		const char *xgeometry, char **geometry, int *srid,
		char **message)
{
/* checking the blade table for validity */
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    char *geom_name = NULL;
    int geom_srid = -1;
    int geom_type = -1;
    int pk = 0;
    int i;
    int ret;

    *geometry = NULL;
    *srid = -1;
    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* checking if the table really exists */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 5];
	  if (atoi (value) > 0)
	      pk = 1;
	  count++;
      }
    sqlite3_free_table (results);
    if (count == 0)
      {
	  do_print_message2 (message, "ERROR: table %s.%s does not exists",
			     db_prefix, table);
	  goto error;
      }
    if (pk == 0)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s lacks any Primary Key",
			     db_prefix, table);
	  goto error;
      }

/* checking geometry_columns */
    count = 0;
    if (xgeometry == NULL)
	sql = sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			       "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q)",
			       xprefix, table, xgeometry);
    else
	sql = sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			       "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) "
			       "AND Lower(f_geometry_column) = Lower(%Q)",
			       xprefix, table, xgeometry);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT geometry_columns", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  if (count == 0)
	    {
		int len = strlen (value);
		geom_name = malloc (len + 1);
		strcpy (geom_name, value);
	    }
	  value = results[(i * columns) + 1];
	  geom_srid = atoi (value);
	  value = results[(i * columns) + 2];
	  geom_type = atoi (value);
	  count++;
      }
    sqlite3_free_table (results);
    if (count == 0)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s lacks any registered Geometry",
			     db_prefix, table);
	  goto error;
      }
    if (count > 1)
      {
	  do_print_message2 (message,
			     "ERROR: table %s.%s has multiple Geometries and a NULL name was passed",
			     db_prefix, table);
	  goto error;
      }
    switch (geom_type)
      {
      case GAIA_POLYGON:
      case GAIA_MULTIPOLYGON:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGONZM:
	  break;
      default:
	  do_print_message3 (message,
			     "ERROR: table %s.%s Geometry %s isn't of the POLYGON or MULTIPOLYGON Type",
			     db_prefix, table, geom_name);
	  goto error;
      };
    *geometry = geom_name;
    *srid = geom_srid;

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    if (geom_name != NULL)
	free (geom_name);
    return 0;
}

static int
do_check_nulls (sqlite3 * handle, const char *db_prefix, const char *table,
		const char *geom, const char *which, char **message)
{
/* testing for NULL PK values and NULL Geoms */
    int ret;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    sqlite3_stmt *stmt = NULL;
    int icol;
    int count = 0;
    int null_geom = 0;
    int null_pk = 0;

/* composing the SQL query */
    xcolumn = gaiaDoubleQuotedSql (geom);
    sql = sqlite3_mprintf ("SELECT \"%s\"", geom);
    free (xcolumn);
    prev = sql;

/* retrieving all PK columns */
    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xprefix);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *value = results[(i * columns) + 5];
	  if (atoi (value) > 0)
	    {
		/* found a PK column */
		xcolumn = gaiaDoubleQuotedSql (name);
		sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn);
		free (xcolumn);
		sqlite3_free (prev);
		prev = sql;
	    }
      }
    sqlite3_free_table (results);

    xprefix = gaiaDoubleQuotedSql (db_prefix);
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("%s FROM \"%s\".\"%s\"", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);

/* creating the prepared statement */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CHECK NULLS ",
			       sqlite3_errmsg (handle));
	  goto error;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		count++;
		if (sqlite3_column_type (stmt, 0) == SQLITE_NULL)
		    null_geom++;
		for (icol = 1; icol < sqlite3_column_count (stmt); icol++)
		  {
		      if (sqlite3_column_type (stmt, icol) == SQLITE_NULL)
			  null_pk++;
		  }
		if (null_geom || null_pk)
		    break;
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: CHECK NULLS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;

    if (null_geom)
      {
	  sql =
	      sqlite3_mprintf ("Invalid %s: found NULL Geometries !!!", which);
	  do_update_message (message, sql);
	  sqlite3_free (sql);
	  goto error;
      }
    if (null_pk)
      {
	  sql = sqlite3_mprintf ("Invalid %s: found NULL PK Values !!!", which);
	  do_update_message (message, sql);
	  sqlite3_free (sql);
	  goto error;
      }
    if (!count)
      {
	  sql = sqlite3_mprintf ("Invalid %s: empty table !!!", which);
	  do_update_message (message, sql);
	  sqlite3_free (sql);
	  goto error;
      }

    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_check_valid (sqlite3 * handle, const char *out_table, const char *input_geom,
		char **message)
{
/* checking for Invalid Output Geoms */
    int ret;
    char *sql;
    char *xcolumn;
    char *xtable;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    int count = 0;

/* inspecting the table */
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    xtable = gaiaDoubleQuotedSql (out_table);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM MAIN.\"%s\" WHERE ST_IsValid(\"%s\") <> 1",
	 xtable, xcolumn);
    free (xtable);
    free (xcolumn);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 1;
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  count = atoi (value);
      }
    sqlite3_free_table (results);
    if (count > 0)
      {
	  do_update_message (message,
			     "The OUTPUT table contains INVALID Geometries");
	  return 0;
      }
    return 1;
}

static int
check_spatial_index (sqlite3 * handle, const char *db_prefix,
		     const char *idx_name, char **message)
{
/* testing if some Spatial Index do really exists */
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;
    int ok_pkid = 0;
    int ok_xmin = 0;
    int ok_xmax = 0;
    int ok_ymin = 0;
    int ok_ymax = 0;

    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* inspecting the table */
    xtable = gaiaDoubleQuotedSql (idx_name);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  if (strcasecmp (name, "pkid") == 0)
	      ok_pkid = 1;
	  if (strcasecmp (name, "xmin") == 0)
	      ok_xmin = 1;
	  if (strcasecmp (name, "xmax") == 0)
	      ok_xmax = 1;
	  if (strcasecmp (name, "ymin") == 0)
	      ok_ymin = 1;
	  if (strcasecmp (name, "ymax") == 0)
	      ok_ymax = 1;
      }
    sqlite3_free_table (results);
    if (!ok_pkid || !ok_xmin || !ok_xmax || !ok_ymin || !ok_ymax)
	goto error;

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    return 0;
}

static int
do_verify_blade_spatial_index (sqlite3 * handle, const char *db_prefix,
			       const char *table, const char *geometry,
			       char **spatial_index_prefix,
			       char **spatial_index, int *drop_spatial_index,
			       char **message)
{
/* verifying the Spatial Index supporting the Blade Geometry */
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char *xgeometry;
    char *idx_prefix;
    char *idx_name;
    char **results;
    int rows;
    int columns;
    int i;
    int declared = 0;
    char *errMsg = NULL;
    time_t now;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif

    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* inspecting the table */
    sql = sqlite3_mprintf ("SELECT spatial_index_enabled "
			   "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) "
			   "AND Lower(f_geometry_column) = Lower(%Q)",
			   xprefix, table, geometry);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT geometry_columns", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *value = results[(i * columns) + 0];
	  if (atoi (value) == 1)
	      declared = 1;
      }
    sqlite3_free_table (results);

    if (!declared)
	goto create_default;

    idx_name = sqlite3_mprintf ("idx_%s_%s", table, geometry);
    if (check_spatial_index (handle, db_prefix, idx_name, message))
      {
	  idx_prefix = malloc (strlen (db_prefix) + 1);
	  strcpy (idx_prefix, db_prefix);
	  *spatial_index_prefix = idx_prefix;
	  *spatial_index = idx_name;
	  *drop_spatial_index = 0;
	  goto end;
      }
    sqlite3_free (idx_name);

  create_default:
/* creating a transient Spatial Index */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    time (&now);
    idx_name = sqlite3_mprintf ("tmpidx_%u_%u", pid, now);
    xtable = gaiaDoubleQuotedSql (idx_name);
    sql = sqlite3_mprintf ("CREATE VIRTUAL TABLE TEMP.\"%s\" USING "
			   "rtree(pkid, xmin, xmax, ymin, ymax)", xtable);
    free (xtable);

    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CREATE SPATIAL INDEX", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }

/* populating the transient Spatial Index */
    xtable = gaiaDoubleQuotedSql (table);
    xgeometry = gaiaDoubleQuotedSql (geometry);
    sql =
	sqlite3_mprintf
	("INSERT INTO TEMP.\"%s\" (pkid, xmin, xmax, ymin, ymax) "
	 "SELECT ROWID, MbrMinX(\"%s\"), MbrMaxX(\"%s\"), MbrMinY(\"%s\"), MbrMaxY(\"%s\") "
	 "FROM \"%s\".\"%s\"", idx_name, xgeometry, xgeometry, xgeometry,
	 xgeometry, xprefix, xtable);
    free (xtable);
    free (xgeometry);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "POPULATE SPATIAL INDEX", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }

    *spatial_index = idx_name;
    idx_prefix = malloc (5);
    strcpy (idx_prefix, "TEMP");
    *spatial_index_prefix = idx_prefix;
    *drop_spatial_index = 1;

  end:
    free (xprefix);
    return 1;

  error:
    free (xprefix);
    return 0;
}

static int
do_drop_blade_spatial_index (sqlite3 * handle, const char *spatial_index,
			     char **message)
{
/* dropping the transient Spatial Index supporting the Blade table */
    int ret;
    char *sql;
    char *xtable;
    char *errMsg = NULL;
    xtable = gaiaDoubleQuotedSql (spatial_index);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "DROP SPATIAL INDEX", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
do_drop_tmp_table (sqlite3 * handle, const char *tmp_table, char **message)
{
/* dropping the Temporary helper table */
    int ret;
    char *sql;
    char *xtable;
    char *errMsg = NULL;
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("DROP TABLE TEMP.\"%s\"", xtable);
    free (xtable);
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "DROP TEMPORAY TABLE", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }
    return 1;
}

static int
do_check_output (sqlite3 * handle, const char *db_prefix, const char *table,
		 const char *geometry, char **message)
{
/* checking the output table for validity */
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    int count = 0;
    int i;
    int ret;

    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* checking if the table already exists */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);
    if (count != 0)
      {
	  do_print_message2 (message, "ERROR: table %s.%s do already exists",
			     db_prefix, table);
	  goto error;
      }

/* checking geometry_columns */
    count = 0;
    sql = sqlite3_mprintf ("SELECT f_geometry_column, srid, geometry_type "
			   "FROM \"%s\".geometry_columns WHERE Lower(f_table_name) = Lower(%Q) "
			   "AND Lower(f_geometry_column) = Lower(%Q)",
			   xprefix, table, geometry);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT geometry_columns", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
	count++;
    sqlite3_free_table (results);
    if (count != 0)
      {
	  do_print_message3 (message,
			     "ERROR: table %s.%s Geometry %s is already registered",
			     db_prefix, table, geometry);
	  goto error;
      }

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    return 0;
}

static int
do_get_input_pk (struct output_table *tbl, sqlite3 * handle,
		 const char *db_prefix, const char *table, char **message)
{
/* retrieving the input table PK columns */
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;

    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* inspecting the table */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *type = results[(i * columns) + 2];
	  const char *notnull = results[(i * columns) + 3];
	  const char *value = results[(i * columns) + 5];
	  if (atoi (value) > 0)
	    {
		if (add_column_to_output_table
		    (tbl, name, type, atoi (notnull), GAIA_CUTTER_INPUT_PK,
		     atoi (value)) == NULL)
		  {
		      do_print_message2 (message,
					 "ERROR: insufficient memory (OutputTable wrapper/Input PK)",
					 db_prefix, table);
		      goto error;
		  }
	    }
      }
    sqlite3_free_table (results);

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    return 0;
}

static int
do_get_blade_pk (struct output_table *tbl, sqlite3 * handle,
		 const char *db_prefix, const char *table, char **message)
{
/* retrieving the blade table PK columns */
    int ret;
    char *sql;
    char *xprefix;
    char *xtable;
    char **results;
    int rows;
    int columns;
    int i;
    char *errMsg = NULL;

    xprefix = gaiaDoubleQuotedSql (db_prefix);

/* inspecting the table */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("PRAGMA \"%s\".table_info(\"%s\")", xprefix, xtable);
    free (xtable);
    ret = sqlite3_get_table (handle, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA table_info", errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }
    for (i = 1; i <= rows; i++)
      {
	  const char *name = results[(i * columns) + 1];
	  const char *type = results[(i * columns) + 2];
	  const char *notnull = results[(i * columns) + 3];
	  const char *value = results[(i * columns) + 5];
	  if (atoi (value) > 0)
	    {
		if (add_column_to_output_table
		    (tbl, name, type, atoi (notnull), GAIA_CUTTER_BLADE_PK,
		     atoi (value)) == NULL)
		  {
		      do_print_message2 (message,
					 "ERROR: insufficient memory (OutputTable wrapper/Blade PK)",
					 db_prefix, table);
		      goto error;
		  }
	    }
      }
    sqlite3_free_table (results);

    free (xprefix);
    return 1;

  error:
    free (xprefix);
    return 0;
}

static void
make_lowercase (char *name)
{
/* making a column name all lowercase */
    char *p = name;
    while (*p != '\0')
      {
	  if (*p >= 'A' && *p <= 'Z')
	      *p = *p - 'A' + 'a';
	  p++;
      }
}

static int
do_create_output_table (struct output_table *tbl, sqlite3 * handle,
			const char *table, const char *input_table,
			const char *blade_table, char **message)
{
/* attempting to create the Output Table */
    int ret;
    char *errMsg = NULL;
    char *sql;
    char *prev;
    char *xtable;
    char *xcolumn;
    char *zcolumn;
    char *zzcolumn;
    struct output_column *col;

/* composing the CREATE TABLE statement */
    xtable = gaiaDoubleQuotedSql (table);
    sql = sqlite3_mprintf ("CREATE TABLE \"%s\" (", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* adding a column to the table */
	  xcolumn = gaiaDoubleQuotedSql (col->base_name);
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		/* Input PK column */
		zcolumn =
		    sqlite3_mprintf ("input_%s_%s", input_table,
				     col->base_name);
		make_lowercase (zcolumn);
		zzcolumn = gaiaDoubleQuotedSql (zcolumn);
		col->real_name = zcolumn;
		sql =
		    sqlite3_mprintf ("%s,\n\t\"%s\" %s%s", prev, zzcolumn,
				     col->type,
				     (col->notnull) ? " NOT NULL" : "");
		free (zzcolumn);
		sqlite3_free (prev);
		prev = sql;
	    }
	  else if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		/* Blade PK column */
		zcolumn =
		    sqlite3_mprintf ("blade_%s_%s", blade_table,
				     col->base_name);
		make_lowercase (zcolumn);
		zzcolumn = gaiaDoubleQuotedSql (zcolumn);
		col->real_name = zcolumn;
		sql =
		    sqlite3_mprintf ("%s,\n\t\"%s\" %s", prev, zzcolumn,
				     col->type);
		free (zzcolumn);
		sqlite3_free (prev);
		prev = sql;
	    }
	  else if (col->role == GAIA_CUTTER_OUTPUT_PK)
	    {
		/* output table primary column */
		sql =
		    sqlite3_mprintf ("%s\n\t\"%s\" %s PRIMARY KEY", prev,
				     xcolumn, col->type);
		sqlite3_free (prev);
		prev = sql;
	    }
	  else
	    {
		/* ordinary column */
		sql =
		    sqlite3_mprintf ("%s,\n\t\"%s\" %s%s", prev, xcolumn,
				     col->type,
				     (col->notnull) ? " NOT NULL" : "");
		sqlite3_free (prev);
		prev = sql;
	    }
	  free (xcolumn);
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s)", prev);
    sqlite3_free (prev);

/* creating the Output Table */
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CREATE TABLE", errMsg);
	  sqlite3_free (errMsg);
	  return 0;
      }

    return 1;
}

static int
do_create_output_geometry (sqlite3 * handle, const char *table,
			   const char *geometry, int srid, int geom_type,
			   char **message)
{
/* attempting to create the Output Geometry */
    int ret;
    int retcode = 0;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    const char *type = "";
    const char *dims = "";

    switch (geom_type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  type = "POINT";
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  type = "LINESTRING";
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  type = "POLYGON";
	  break;
      };
    switch (geom_type)
      {
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
      case GAIA_MULTIPOINT:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTIPOLYGON:
      case GAIA_POINTM:
      case GAIA_LINESTRINGM:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTIPOLYGONM:
	  dims = "XY";
	  break;
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
	  dims = "XYZ";
	  break;
      };
    sql =
	sqlite3_mprintf
	("SELECT AddGeometryColumn(Lower(%Q), Lower(%Q), %d, %Q, %Q)", table,
	 geometry, srid, type, dims);
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "AddGeometryTable",
			       sqlite3_errmsg (handle));
	  goto end;
      }
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
		  {
		      /* testing for success */
		      if (sqlite3_column_int (stmt, 0) != 0)
			  retcode = 1;
		  }
	    }
      }

  end:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return retcode;
}

static gaiaGeomCollPtr
do_read_input_geometry (struct output_table *tbl, const void *cache,
			sqlite3_stmt * stmt_in, sqlite3 * handle,
			struct temporary_row *row, char **message,
			unsigned char **blob, int *blob_sz)
{
/* reading an Input Geometry */
    int ret;
    int icol2 = 0;
    int icol = 1;
    struct multivar *var;
    struct output_column *col;
    gaiaGeomCollPtr geom = NULL;
    const unsigned char *blob_value;
    int size;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  gpkg_mode = pcache->gpkg_mode;
      }
    *blob = NULL;
    *blob_sz = 0;

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		var = find_input_pk_value (row, icol2);
		if (var == NULL)
		    return NULL;
		icol2++;
		switch (var->type)
		  {
		      /* Input Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_in, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_in, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_in, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_in, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    while (1)
      {
	  /* scrolling the result set rows - Input */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_BLOB)
		  {
		      blob_value = sqlite3_column_blob (stmt_in, 0);
		      size = sqlite3_column_bytes (stmt_in, 0);
		      geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob_value, size,
						       gpkg_mode,
						       gpkg_amphibious);
		      *blob = (unsigned char *) blob_value;
		      *blob_sz = size;
		      return geom;
		  }
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT Geometry FROM INPUT",
				     sqlite3_errmsg (handle));
		if (geom != NULL)
		    gaiaFreeGeomColl (geom);
		return NULL;
	    }
      }

    if (geom == NULL)
	do_update_message (message, "found unexpected NULL Input Geometry");
    return geom;
}

static gaiaGeomCollPtr
do_read_blade_geometry (struct output_table *tbl, const void *cache,
			sqlite3_stmt * stmt_in, sqlite3 * handle,
			struct temporary_row *row, char **message,
			unsigned char **blob, int *blob_sz)
{
/* reading an Blase Geometry */
    int ret;
    int icol2 = 0;
    int icol = 1;
    struct multivar *var;
    struct output_column *col;
    gaiaGeomCollPtr geom = NULL;
    const unsigned char *blob_value;
    int size;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  gpkg_mode = pcache->gpkg_mode;
      }
    *blob = NULL;
    *blob_sz = 0;

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		var = find_blade_pk_value (row, icol2);
		if (var == NULL)
		    return NULL;
		icol2++;
		switch (var->type)
		  {
		      /* Blade Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_in, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_in, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_in, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_in, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    while (1)
      {
	  /* scrolling the result set rows - Input */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_BLOB)
		  {
		      blob_value = sqlite3_column_blob (stmt_in, 0);
		      size = sqlite3_column_bytes (stmt_in, 0);
		      geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob_value, size,
						       gpkg_mode,
						       gpkg_amphibious);
		      *blob = (unsigned char *) blob_value;
		      *blob_sz = size;
		      return geom;
		  }
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT Geometry FROM BLADE",
				     sqlite3_errmsg (handle));
		if (geom != NULL)
		    gaiaFreeGeomColl (geom);
		return NULL;
	    }
      }

    if (geom == NULL)
	do_update_message (message, "found unexpected NULL Blade Geometry");
    return geom;
}

static gaiaGeomCollPtr
do_prepare_point (gaiaPointPtr pt, int srid)
{
/* normalizing a Point Geometry */
    gaiaGeomCollPtr new_geom = NULL;

    if (pt->DimensionModel == GAIA_XY_Z || pt->DimensionModel == GAIA_XY_Z_M)
      {
	  new_geom = gaiaAllocGeomCollXYZ ();
	  gaiaAddPointToGeomCollXYZ (new_geom, pt->X, pt->Y, pt->Z);
      }
    else
      {
	  new_geom = gaiaAllocGeomColl ();
	  gaiaAddPointToGeomColl (new_geom, pt->X, pt->Y);
      }
    if (new_geom->MinX > pt->X)
	new_geom->MinX = pt->X;
    if (new_geom->MaxX < pt->X)
	new_geom->MaxX = pt->X;
    if (new_geom->MinY > pt->Y)
	new_geom->MinY = pt->Y;
    if (new_geom->MaxY < pt->Y)
	new_geom->MaxY = pt->Y;
    new_geom->Srid = srid;
    new_geom->DeclaredType = GAIA_POINT;
    return new_geom;
}

static gaiaGeomCollPtr
do_prepare_linestring (gaiaLinestringPtr ln, int srid)
{
/* normalizing a Linestring Geometry */
    gaiaGeomCollPtr new_geom = NULL;
    gaiaLinestringPtr new_ln;
    int iv;
    double x;
    double y;
    double z = 0.0;
    double m = 0.0;

    if (ln->DimensionModel == GAIA_XY_Z || ln->DimensionModel == GAIA_XY_Z_M)
	new_geom = gaiaAllocGeomCollXYZ ();
    else
	new_geom = gaiaAllocGeomColl ();
    new_geom->Srid = srid;
    new_geom->DeclaredType = GAIA_LINESTRING;

    new_ln = gaiaAddLinestringToGeomColl (new_geom, ln->Points);
    for (iv = 0; iv < ln->Points; iv++)
      {
	  if (ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (ln->Coords, iv, &x, &y, &z);
	    }
	  else if (ln->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (ln->Coords, iv, &x, &y, &m);
	    }
	  else if (ln->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (ln->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (ln->Coords, iv, &x, &y);
	    }
	  if (new_geom->MinX > x)
	      new_geom->MinX = x;
	  if (new_geom->MaxX < x)
	      new_geom->MaxX = x;
	  if (new_geom->MinY > y)
	      new_geom->MinY = y;
	  if (new_geom->MaxY < y)
	      new_geom->MaxY = y;
	  if (new_ln->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (new_ln->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (new_ln->Coords, iv, x, y);
	    }
      }
    return new_geom;
}

static gaiaGeomCollPtr
do_prepare_polygon (gaiaPolygonPtr pg, int srid)
{
/* normalizing a Polygon Geometry */
    gaiaGeomCollPtr new_geom = NULL;
    gaiaPolygonPtr new_pg;
    gaiaRingPtr rng;
    gaiaRingPtr new_rng;
    int iv;
    int ib;
    double x;
    double y;
    double z;
    double m;

    if (pg->DimensionModel == GAIA_XY_Z || pg->DimensionModel == GAIA_XY_Z_M)
	new_geom = gaiaAllocGeomCollXYZ ();
    else
	new_geom = gaiaAllocGeomColl ();
    new_geom->Srid = srid;
    new_geom->DeclaredType = GAIA_POLYGON;

/* Exterior Ring */
    rng = pg->Exterior;
    new_pg = gaiaAddPolygonToGeomColl (new_geom, rng->Points, pg->NumInteriors);
    new_rng = new_pg->Exterior;
    for (iv = 0; iv < rng->Points; iv++)
      {
	  m = 0.0;
	  z = 0.0;
	  if (rng->DimensionModel == GAIA_XY_Z)
	    {
		gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
	    }
	  else if (rng->DimensionModel == GAIA_XY_M)
	    {
		gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
	    }
	  else if (rng->DimensionModel == GAIA_XY_Z_M)
	    {
		gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
	    }
	  else
	    {
		gaiaGetPoint (rng->Coords, iv, &x, &y);
	    }
	  if (new_geom->MinX > x)
	      new_geom->MinX = x;
	  if (new_geom->MaxX < x)
	      new_geom->MaxX = x;
	  if (new_geom->MinY > y)
	      new_geom->MinY = y;
	  if (new_geom->MaxY < y)
	      new_geom->MaxY = y;
	  if (new_rng->DimensionModel == GAIA_XY_Z)
	    {
		gaiaSetPointXYZ (new_rng->Coords, iv, x, y, z);
	    }
	  else
	    {
		gaiaSetPoint (new_rng->Coords, iv, x, y);
	    }
      }
    for (ib = 0; ib < pg->NumInteriors; ib++)
      {
	  /* Interior Rings */
	  rng = pg->Interiors + ib;
	  new_rng = gaiaAddInteriorRing (new_pg, ib, rng->Points);
	  for (iv = 0; iv < rng->Points; iv++)
	    {
		m = 0.0;
		z = 0.0;
		if (rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaGetPointXYZ (rng->Coords, iv, &x, &y, &z);
		  }
		else if (rng->DimensionModel == GAIA_XY_M)
		  {
		      gaiaGetPointXYM (rng->Coords, iv, &x, &y, &m);
		  }
		else if (rng->DimensionModel == GAIA_XY_Z_M)
		  {
		      gaiaGetPointXYZM (rng->Coords, iv, &x, &y, &z, &m);
		  }
		else
		  {
		      gaiaGetPoint (rng->Coords, iv, &x, &y);
		  }
		if (new_rng->DimensionModel == GAIA_XY_Z)
		  {
		      gaiaSetPointXYZ (new_rng->Coords, iv, x, y, z);
		  }
		else
		  {
		      gaiaSetPoint (new_rng->Coords, iv, x, y);
		  }
	    }
      }
    return new_geom;
}

static int
do_insert_output_row (struct output_table *tbl, const void *cache,
		      sqlite3_stmt * stmt_out, sqlite3 * handle,
		      struct temporary_row *row, int n_geom, int res_prog,
		      int geom_type, void *item, int srid, char **message)
{
/* inserting into the Output table */
    int ret;
    int icol2 = 0;
    int icol = 1;
    struct multivar *var;
    struct output_column *col;
    gaiaGeomCollPtr geom = NULL;
    gaiaPointPtr pt;
    gaiaLinestringPtr ln;
    gaiaPolygonPtr pg;
    unsigned char *blob;
    int size;
    int gpkg_mode = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		/* binding Input PK values */
		var = find_input_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Input Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    icol2 = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		/* binding Blade PK values */
		var = find_blade_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Blade Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
/* binding "n_geom" and "res_prog" columns */
    sqlite3_bind_int (stmt_out, icol, n_geom);
    icol++;
    sqlite3_bind_int (stmt_out, icol, res_prog);
    icol++;
    switch (geom_type)
      {
	  /* preparing the Output Geometry */
      case GAIA_CUTTER_POINT:
	  pt = (gaiaPointPtr) item;
	  geom = do_prepare_point (pt, srid);
	  break;
      case GAIA_CUTTER_LINESTRING:
	  ln = (gaiaLinestringPtr) item;
	  geom = do_prepare_linestring (ln, srid);
	  break;
      case GAIA_CUTTER_POLYGON:
	  pg = (gaiaPolygonPtr) item;
	  geom = do_prepare_polygon (pg, srid);
	  break;
      default:
	  do_update_message (message, "ILLEGAL OUTPUT GEOMETRY");
	  return 0;
      };
    if (geom == NULL)
      {
	  do_update_message (message, "UNEXPECTED NULL OUTPUT GEOMETRY");
	  return 0;
      }
    gaiaToSpatiaLiteBlobWkbEx2 (geom, &blob, &size, gpkg_mode, tiny_point);
    if (blob == NULL)
      {
	  do_update_message (message, "UNEXPECTED NULL OUTPUT BLOB GEOMETRY");
	  gaiaFreeGeomColl (geom);
	  return 0;
      }
    sqlite3_bind_blob (stmt_out, icol, blob, size, free);
    gaiaFreeGeomColl (geom);

    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

/* some error occurred */
    do_update_sql_error (message, "INSERT INTO OUTPUT",
			 sqlite3_errmsg (handle));
    return 0;
}

static int
do_create_input_statement (struct output_table *tbl, sqlite3 * handle,
			   const char *input_db_prefix, const char *input_table,
			   const char *input_geom, sqlite3_stmt ** stmt_in,
			   char **message)
{
/* creating a Prepared Statemet - SELECT geometry FROM Input */
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;

/* composing the SQL statement */
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\" WHERE", xcolumn,
			 xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s AND \"%s\" = ?", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT GEOMETRY FROM INPUT",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    *stmt_in = stmt;
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_create_output_statement (struct output_table *tbl, sqlite3 * handle,
			    const char *out_table, sqlite3_stmt ** stmt_out,
			    char **message)
{
/* creating a Prepared Statemet - INSERT INTO Output */
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *xtable;
    char *sql;
    char *prev;
    struct output_column *col;

/* composing the SQL statement */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" VALUES (NULL", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s, ?, ?, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO OUTPUT",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    *stmt_out = stmt;
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
do_prepare_temp_points (struct output_table *tbl, sqlite3 * handle,
			const char *input_db_prefix, const char *input_table,
			const char *input_geom, const char *blade_db_prefix,
			const char *blade_table, const char *blade_geom,
			const char *spatial_index_prefix,
			const char *spatial_index, char **tmp_table,
			char **message)
{
/* creating and populating the temporary helper table - POINTs */
    int ret;
    char *errMsg = NULL;
    char *temporary_table;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    time_t now;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct output_column *col;
    int comma = 0;

    *tmp_table = NULL;
/* composing the SQL statement */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    time (&now);
    temporary_table = sqlite3_mprintf ("tmpcuttertbl_%u_%u", pid, now);
    xtable = gaiaDoubleQuotedSql (temporary_table);

    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE \"%s\" AS SELECT", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, i.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s i.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, b.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s b.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the "Touches" column */
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xcolumn2 = gaiaDoubleQuotedSql (blade_geom);
    sql =
	sqlite3_mprintf ("%s, ST_Touches(i.\"%s\", b.\"%s\") AS touches", prev,
			 xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql = sqlite3_mprintf ("%s FROM \"%s\".\"%s\" AS i", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s LEFT JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xcolumn2 = gaiaDoubleQuotedSql (blade_geom);
    sql =
	sqlite3_mprintf ("%sST_CoveredBy(i.\"%s\", b.\"%s\") = 1 ", prev,
			 xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    sql = sqlite3_mprintf ("%s AND b.ROWID IN (SELECT pkid FROM ", prev);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (spatial_index_prefix);
    xtable = gaiaDoubleQuotedSql (spatial_index);
    sql = sqlite3_mprintf ("%s \"%s\".\"%s\" WHERE", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    sql =
	sqlite3_mprintf
	("%s xmin <= MbrMaxX(i.\"%s\") AND xmax >= MbrMinX(i.\"%s\") ", prev,
	 xcolumn1, xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    sql =
	sqlite3_mprintf
	("%s AND ymin <= MbrMaxY(i.\"%s\") AND ymax >= MbrMinY(i.\"%s\")))",
	 prev, xcolumn1, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);

/* executing the SQL Query */
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CREATE TEMPORARY TABLE POINT #0",
			       errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }

    *tmp_table = temporary_table;
    return 1;

  error:
    if (temporary_table != NULL)
	sqlite3_free (temporary_table);
    return 0;
}

static int
do_create_temp_linestrings (struct output_table *tbl, sqlite3 * handle,
			    char **tmp_table, char **message)
{
/* creating and populating the temporary helper table - LINESTRINGs */
    int ret;
    char *errMsg = NULL;
    char *temporary_table;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    time_t now;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct output_column *col;
    int comma = 0;

    *tmp_table = NULL;
/* composing the SQL statement */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    time (&now);
    temporary_table = sqlite3_mprintf ("tmpcuttertbl_%u_%u", pid, now);
    xtable = gaiaDoubleQuotedSql (temporary_table);

    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE \"%s\" (", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, \"%s\" GENERIC", prev, xcolumn2);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" GENERIC", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */ ;
    xprefix = sqlite3_mprintf ("%s_n_geom", temporary_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s, \"%s\" INTEGER", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, \"%s\" GENERIC", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the nodes-geometry */
    xprefix = sqlite3_mprintf ("%s_nodes_geom", temporary_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s, \"%s\" BLOB", prev, xcolumn2);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
/* adding the cut-geometry */
    xprefix = sqlite3_mprintf ("%s_geom", temporary_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s, \"%s\" BLOB)", prev, xcolumn2);
    free (xcolumn2);
    sqlite3_free (prev);

/* executing the SQL Query */
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CREATE TEMPORARY TABLE LINESTRINGS",
			       errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }

    *tmp_table = temporary_table;
    return 1;

  error:
    if (temporary_table != NULL)
	sqlite3_free (temporary_table);
    return 0;
}

static int
do_create_temp_polygons (struct output_table *tbl, sqlite3 * handle,
			 char **tmp_table, char **message)
{
/* creating and populating the temporary helper table - POLYGONs */
    int ret;
    char *errMsg = NULL;
    char *temporary_table;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    time_t now;
#if defined(_WIN32) && !defined(__MINGW32__)
    int pid;
#else
    pid_t pid;
#endif
    struct output_column *col;
    int comma = 0;

    *tmp_table = NULL;
/* composing the SQL statement */
#if defined(_WIN32) && !defined(__MINGW32__)
    pid = _getpid ();
#else
    pid = getpid ();
#endif
    time (&now);
    temporary_table = sqlite3_mprintf ("tmpcuttertbl_%u_%u", pid, now);
    xtable = gaiaDoubleQuotedSql (temporary_table);

    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE \"%s\" (", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, \"%s\" GENERIC", prev, xcolumn2);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" GENERIC", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */ ;
    xprefix = sqlite3_mprintf ("%s_n_geom", temporary_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s, \"%s\" INTEGER", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, \"%s\" GENERIC", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the cut-geometry */
    xprefix = sqlite3_mprintf ("%s_geom", temporary_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s, \"%s\" BLOB)", prev, xcolumn2);
    free (xcolumn2);
    sqlite3_free (prev);

/* executing the SQL Query */
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "CREATE TEMPORARY TABLE POLYGONS",
			       errMsg);
	  sqlite3_free (errMsg);
	  goto error;
      }

    *tmp_table = temporary_table;
    return 1;

  error:
    if (temporary_table != NULL)
	sqlite3_free (temporary_table);
    return 0;
}

static int
is_null_blade (struct temporary_row *row)
{
/* testing for a NULL Blade reference */
    struct multivar *var = row->first_blade;
    while (var != NULL)
      {
	  if (var->type != SQLITE_NULL)
	      return 0;
	  var = var->next;
      }
    return 1;
}

static int
is_covered_by (const void *cache, gaiaGeomCollPtr input_g,
	       unsigned char *input_blob, int input_blob_sz,
	       gaiaGeomCollPtr blade_g, unsigned char *blade_blob,
	       int blade_blob_sz)
{
/* testing if the Input geometry is completely Covered By the Blade Geometry */
    return gaiaGeomCollPreparedCoveredBy (cache, input_g, input_blob,
					  input_blob_sz, blade_g, blade_blob,
					  blade_blob_sz);
}

static int
do_insert_temporary_linestrings (struct output_table *tbl, sqlite3 * handle,
				 const void *cache, sqlite3_stmt * stmt_out,
				 struct temporary_row *row,
				 gaiaGeomCollPtr geom, char **message,
				 int ngeom)
{
/* inserting a resolved Geometry in the Linestrings Helper Table */
    int ret;
    gaiaLinestringPtr ln;
    struct output_column *col;
    struct multivar *var;
    int icol2 = 0;
    int icol = 1;
    int n_geom = 0;
    unsigned char *blob;
    int size;
    int gpkg_mode = 0;
    int tiny_point = 0;
    gaiaGeomCollPtr g;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    if (ngeom < 0)
	n_geom = 0;
    else
	n_geom = ngeom;

    ln = geom->FirstLinestring;
    while (ln != NULL)
      {
	  /* separating elementary geometries */
	  icol2 = 0;
	  icol = 1;
	  if (ngeom < 0)
	      n_geom++;
	  g = do_prepare_linestring (ln, geom->Srid);
	  sqlite3_reset (stmt_out);
	  sqlite3_clear_bindings (stmt_out);
	  col = tbl->first;
	  while (col != NULL)
	    {
		if (col->role == GAIA_CUTTER_INPUT_PK)
		  {
		      /* binding Input PK values */
		      var = find_input_pk_value (row, icol2);
		      if (var == NULL)
			  return 0;
		      icol2++;
		      switch (var->type)
			{
			    /* Input Primary Key Column(s) */
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_out, icol,
						var->value.intValue);
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_out, icol,
						 var->value.doubleValue);
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_out, icol,
					       var->value.textValue,
					       strlen (var->value.textValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_out, icol);
			    break;
			};
		      icol++;
		  }
		col = col->next;
	    }
	  /* binding n_geom */
	  sqlite3_bind_int (stmt_out, icol, n_geom);
	  icol++;
	  icol2 = 0;
	  col = tbl->first;
	  while (col != NULL)
	    {
		if (col->role == GAIA_CUTTER_BLADE_PK)
		  {
		      /* binding Blade PK values */
		      var = find_blade_pk_value (row, icol2);
		      if (var == NULL)
			  return 0;
		      icol2++;
		      switch (var->type)
			{
			    /* Blade Primary Key Column(s) */
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_out, icol,
						var->value.intValue);
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_out, icol,
						 var->value.doubleValue);
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_out, icol,
					       var->value.textValue,
					       strlen (var->value.textValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_out, icol);
			    break;
			};
		      icol++;
		  }
		col = col->next;
	    }
	  /* binding Nodes */
	  sqlite3_bind_null (stmt_out, icol);
	  icol++;
	  /* binding Geometry */
	  gaiaToSpatiaLiteBlobWkbEx2 (g, &blob, &size, gpkg_mode, tiny_point);
	  if (blob == NULL)
	    {
		do_update_message (message,
				   "UNEXPECTED NULL TEMPORARY LINESTRING BLOB GEOMETRY");
		gaiaFreeGeomColl (geom);
		return 0;
	    }
	  sqlite3_bind_blob (stmt_out, icol, blob, size, free);
	  gaiaFreeGeomColl (g);
	  ret = sqlite3_step (stmt_out);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		/* some error occurred */
		do_update_sql_error (message,
				     "INSERT INTO TEMPORARY LINSTRINGS",
				     sqlite3_errmsg (handle));
		return 0;
	    }
	  ln = ln->Next;
      }
    return 1;
}

static int
do_insert_temporary_linestring_intersection (struct output_table *tbl,
					     sqlite3 * handle,
					     const void *cache,
					     sqlite3_stmt * stmt_out,
					     struct temporary_row *row,
					     int n_geom, gaiaGeomCollPtr nodes,
					     char **message)
{
/* inserting an Input/Blade intersection into the Linestrings Helper Table */
    int ret;
    struct output_column *col;
    struct multivar *var;
    int icol2 = 0;
    int icol = 1;
    unsigned char *blob;
    int size;
    int gpkg_mode = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		/* binding Input PK values */
		var = find_input_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Input Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    /* binding n_geom */
    sqlite3_bind_int (stmt_out, icol, n_geom);
    icol++;
    icol2 = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		/* binding Blade PK values */
		var = find_blade_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Blade Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    /* binding Nodes */
    gaiaToSpatiaLiteBlobWkbEx2 (nodes, &blob, &size, gpkg_mode, tiny_point);
    if (blob == NULL)
      {
	  do_update_message (message,
			     "UNEXPECTED NULL TEMPORARY LINESTRING NODES BLOB GEOMETRY");
	  return 0;
      }
    sqlite3_bind_blob (stmt_out, icol, blob, size, free);
    icol++;
    /* binding NULL geometry */
    sqlite3_bind_null (stmt_out, icol);
    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

    /* some error occurred */
    do_update_sql_error (message, "INSERT INTO TEMPORARY LINESTRINGS",
			 sqlite3_errmsg (handle));
    return 0;
}

static gaiaGeomCollPtr
do_extract_linestring_nodes (const void *cache, sqlite3_stmt * stmt_nodes,
			     gaiaLinestringPtr input_ln, int srid,
			     gaiaGeomCollPtr blade_g)
{
/* finding the points of intersection between tweo lines (nodes) */
    int ret;
    gaiaGeomCollPtr input_g;
    gaiaGeomCollPtr result = NULL;
    unsigned char *input_blob = NULL;
    int input_blob_sz;
    unsigned char *blade_blob = NULL;
    int blade_blob_sz;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    sqlite3_reset (stmt_nodes);
    sqlite3_clear_bindings (stmt_nodes);
    input_g = do_prepare_linestring (input_ln, srid);
    gaiaToSpatiaLiteBlobWkbEx2 (input_g, &input_blob, &input_blob_sz, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (input_g);
    gaiaToSpatiaLiteBlobWkbEx2 (blade_g, &blade_blob, &blade_blob_sz, gpkg_mode,
				tiny_point);
    sqlite3_bind_blob (stmt_nodes, 1, input_blob, input_blob_sz, free);
    sqlite3_bind_blob (stmt_nodes, 2, blade_blob, blade_blob_sz, free);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_nodes);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		if (sqlite3_column_type (stmt_nodes, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_nodes, 0);
		      int blob_sz = sqlite3_column_bytes (stmt_nodes, 0);
		      result =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
	    }
      }

    return result;
}

static int
do_populate_temp_linestrings (struct output_table *tbl, sqlite3 * handle,
			      const void *cache, const char *input_db_prefix,
			      const char *input_table, const char *input_geom,
			      const char *blade_db_prefix,
			      const char *blade_table, const char *blade_geom,
			      const char *spatial_index_prefix,
			      const char *spatial_index, const char *tmp_table,
			      int type, char **message)
{
/* populating the temporary helper table - LINESTRINGs */
    int ret;
    sqlite3_stmt *stmt_main = NULL;
    sqlite3_stmt *stmt_input = NULL;
    sqlite3_stmt *stmt_blade = NULL;
    sqlite3_stmt *stmt_tmp = NULL;
    sqlite3_stmt *stmt_nodes = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int cast2d = 0;
    int cast3d = 0;

    switch (type)
      {
      case GAIA_LINESTRINGM:
      case GAIA_MULTILINESTRINGM:
	  cast2d = 1;
	  break;
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRINGZM:
	  cast3d = 1;
	  break;
      };

/* composing the SQL statement - main SELECT query */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		sql = sqlite3_mprintf ("%s, b.\"%s\"", prev, xcolumn);
		free (xcolumn);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql = sqlite3_mprintf ("%s FROM \"%s\".\"%s\" AS i", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    sql = sqlite3_mprintf ("%sb.ROWID IN (SELECT pkid FROM ", prev);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (spatial_index_prefix);
    xtable = gaiaDoubleQuotedSql (spatial_index);
    sql = sqlite3_mprintf ("%s \"%s\".\"%s\" WHERE", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    sql =
	sqlite3_mprintf
	("%s xmin <= MbrMaxX(i.\"%s\") AND xmax >= MbrMinX(i.\"%s\") ", prev,
	 xcolumn, xcolumn);
    sqlite3_free (prev);
    prev = sql;
    sql =
	sqlite3_mprintf
	("%s AND ymin <= MbrMaxY(i.\"%s\") AND ymax >= MbrMinY(i.\"%s\")))",
	 prev, xcolumn, xcolumn);
    free (xcolumn);
    sqlite3_free (prev);

/* creating a Prepared Statement - main SELECT query */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_main, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "QUERYING LINESTRING INTERSECTIONS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT geometry FROM Input */
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\" WHERE", xcolumn,
			 xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s AND \"%s\" = ?", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT geometry FROM Input */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_input, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT GEOMETRY FROM INPUT",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT geometry FROM Blade */
    xcolumn = gaiaDoubleQuotedSql (blade_geom);
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\" WHERE", xcolumn,
			 xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s AND \"%s\" = ?", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT geometry FROM Blade */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_blade, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT GEOMETRY FROM BLADE",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - Inserting into TMP */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" VALUES (", xtable);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s, ?", prev);
		else
		    sql = sqlite3_mprintf ("%s?", prev);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */
    sql = sqlite3_mprintf ("%s, ?", prev);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the nodes columns */
    sql = sqlite3_mprintf ("%s, ?", prev);
    sqlite3_free (prev);
    prev = sql;
/* adding the geom columns */
    if (cast2d)
	sql = sqlite3_mprintf ("%s, CastToXY(?))", prev);
    else if (cast3d)
	sql = sqlite3_mprintf ("%s, CastToXYZ(?))", prev);
    else
	sql = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO TMP LINESTRINGS */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tmp, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO TMP LINESTRINGS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - finding NODES */
    sql =
	sqlite3_mprintf ("SELECT CollectionExtract(ST_Intersection(?, ?), 1)");

/* creating a Prepared Statement - finding NODES */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_nodes, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "FINDING LINESTRING NODES",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - checking matching Input/Blade pairs */
	  ret = sqlite3_step (stmt_main);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		gaiaGeomCollPtr input_g = NULL;
		gaiaGeomCollPtr blade_g = NULL;
		gaiaGeomCollPtr linear_blade_g = NULL;
		gaiaLinestringPtr ln;
		int n_geom = 0;
		unsigned char *input_blob;
		unsigned char *blade_blob;
		int input_blob_sz;
		int blade_blob_sz;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_main, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_main, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_main, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_main, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_main, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_main, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_main, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_main, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }

		/* reading the Input Geometry */
		input_g =
		    do_read_input_geometry (tbl, cache, stmt_input, handle,
					    &row, message, &input_blob,
					    &input_blob_sz);
		if (input_g == NULL)
		    goto error;

		if (is_null_blade (&row))
		  {
		      /* Input doesn't matches any Blade */
		      if (!do_insert_temporary_linestrings
			  (tbl, handle, cache, stmt_tmp, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    goto error;
			}
		      goto skip;
		  }

		/* reading the Blade Geometry */
		blade_g =
		    do_read_blade_geometry (tbl, cache, stmt_blade, handle,
					    &row, message, &blade_blob,
					    &blade_blob_sz);
		if (blade_g == NULL)
		    goto error;

		if (is_covered_by
		    (cache, input_g, input_blob, input_blob_sz, blade_g,
		     blade_blob, blade_blob_sz))
		  {
		      /* Input is completely Covered By Blade */
		      if (!do_insert_temporary_linestrings
			  (tbl, handle, cache, stmt_tmp, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    gaiaFreeGeomColl (blade_g);
			    goto error;
			}
		      goto skip;
		  }

		linear_blade_g = gaiaLinearize (blade_g, 1);
		ln = input_g->FirstLinestring;
		while (ln != NULL)
		  {
		      gaiaGeomCollPtr nodes;
		      n_geom++;
		      nodes =
			  do_extract_linestring_nodes (cache, stmt_nodes, ln,
						       input_g->Srid,
						       linear_blade_g);
		      if (nodes != NULL)
			{
			    /* saving an Input/Blade intersection */
			    if (!do_insert_temporary_linestring_intersection
				(tbl, handle, cache, stmt_tmp, &row, n_geom,
				 nodes, message))
			      {
				  reset_temporary_row (&row);
				  gaiaFreeGeomColl (input_g);
				  gaiaFreeGeomColl (blade_g);
				  gaiaFreeGeomColl (linear_blade_g);
				  gaiaFreeGeomColl (nodes);
				  goto error;
			      }
			    gaiaFreeGeomColl (nodes);
			}
		      ln = ln->Next;
		  }
		gaiaFreeGeomColl (linear_blade_g);

	      skip:
		reset_temporary_row (&row);
		gaiaFreeGeomColl (input_g);
		gaiaFreeGeomColl (blade_g);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: MAIN LINESTRINGS LOOP",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_main);
    sqlite3_finalize (stmt_input);
    sqlite3_finalize (stmt_blade);
    sqlite3_finalize (stmt_tmp);
    sqlite3_finalize (stmt_nodes);
    return 1;

  error:
    if (stmt_main == NULL)
	sqlite3_finalize (stmt_main);
    if (stmt_input == NULL)
	sqlite3_finalize (stmt_input);
    if (stmt_blade == NULL)
	sqlite3_finalize (stmt_blade);
    if (stmt_tmp == NULL)
	sqlite3_finalize (stmt_tmp);
    if (stmt_nodes == NULL)
	sqlite3_finalize (stmt_nodes);
    return 0;
}

static int
do_update_tmp_cut_linestring (sqlite3 * handle, sqlite3_stmt * stmt_upd,
			      sqlite3_int64 pk, const unsigned char *blob,
			      int blob_sz, char **message)
{
/* saving an Input Linestring cut against a renoded Blade */
    int ret;

    sqlite3_reset (stmt_upd);
    sqlite3_clear_bindings (stmt_upd);
/* binding the cut Input geometry */
    sqlite3_bind_blob (stmt_upd, 1, blob, blob_sz, free);
    sqlite3_bind_int64 (stmt_upd, 2, pk);
    /* updating the TMP table */
    ret = sqlite3_step (stmt_upd);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

    /* some error occurred */
    do_update_sql_error (message,
			 "step: UPDATE TMP SET cut-Polygon",
			 sqlite3_errmsg (handle));
    return 0;
}

static int
do_cut_tmp_linestrings (sqlite3 * handle, const void *cache,
			sqlite3_stmt * stmt_in, sqlite3_stmt * stmt_upd,
			struct temporary_row *row, char **message,
			const unsigned char *blade_blob, int blade_blob_sz)
{
/* cutting all Input Linestrings intersecting the renoded Blade */
    int ret;
    struct multivar *var;
    int icol = 1;
    gaiaGeomCollPtr blade_g;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    blade_g = gaiaFromSpatiaLiteBlobWkbEx (blade_blob, blade_blob_sz,
					   gpkg_mode, gpkg_amphibious);

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    var = row->first_blade;
    while (var != NULL)
      {
	  /* binding Primary Key values (from Blade) */
	  switch (var->type)
	    {
	    case SQLITE_INTEGER:
		sqlite3_bind_int64 (stmt_in, icol, var->value.intValue);
		break;
	    case SQLITE_FLOAT:
		sqlite3_bind_double (stmt_in, icol, var->value.doubleValue);
		break;
	    case SQLITE_TEXT:
		sqlite3_bind_text (stmt_in, icol,
				   var->value.textValue,
				   strlen (var->value.textValue),
				   SQLITE_STATIC);
		break;
	    default:
		sqlite3_bind_null (stmt_in, icol);
		break;
	    };
	  icol++;
	  var = var->next;
      }

    while (1)
      {
	  /* scrolling the result set rows - cut Linestrings */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		sqlite3_int64 pk = 0;
		unsigned char *blob = NULL;
		int blob_sz = 0;
		gaiaGeomCollPtr input_g;
		gaiaGeomCollPtr result;
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER
		    && sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      pk = sqlite3_column_int64 (stmt_in, 0);
		      blob = (unsigned char *) sqlite3_column_blob (stmt_in, 1);
		      blob_sz = sqlite3_column_bytes (stmt_in, 1);
		      input_g = gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
							     gpkg_mode,
							     gpkg_amphibious);
		      result =
			  gaiaGeometryIntersection_r (cache, input_g, blade_g);
		      if (result != NULL)
			{
			    gaiaToSpatiaLiteBlobWkbEx2 (result, &blob, &blob_sz,
							gpkg_mode, tiny_point);
			    gaiaFreeGeomColl (result);
			}
		      gaiaFreeGeomColl (input_g);
		  }
		if (blob != NULL)
		  {
		      if (!do_update_tmp_cut_linestring
			  (handle, stmt_upd, pk, blob, blob_sz, message))
			  goto error;
		  }
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: cut Linestrings",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }
    gaiaFreeGeomColl (blade_g);
    return 1;

  error:
    gaiaFreeGeomColl (blade_g);
    return 0;
}

static int
do_split_linestrings (struct output_table *tbl, sqlite3 * handle,
		      const void *cache, const char *input_db_prefix,
		      const char *input_table, const char *input_geom,
		      const char *blade_db_prefix, const char *blade_table,
		      const char *blade_geom, const char *tmp_table,
		      char **message)
{
/* cutting all Input Linestrings intersecting some Blade */
    int ret;
    sqlite3_stmt *stmt_blades = NULL;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_upd = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;

/* composing the SQL statement - SELECT FROM Blades */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xcolumn1 = gaiaDoubleQuotedSql (blade_geom);
    xprefix = sqlite3_mprintf ("%s_nodes_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf
	("%s, ST_Snap(b.\"%s\", ST_UnaryUnion(ST_Collect(t.\"%s\")), 0.000000001)",
	 prev, xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("%s FROM TEMP.\"%s\" AS t", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND b.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s b.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s) WHERE t.\"%s\" IS NULL GROUP BY", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT FROM Blades */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_blades, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM BLADES",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT Input */
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xtable = gaiaDoubleQuotedSql (tmp_table);
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT t.ROWID, ST_GeometryN(i.\"%s\", t.\"%s\") FROM TEMP.\"%s\" AS t",
	 xcolumn1, xcolumn2, xtable);
    free (xcolumn1);
    free (xcolumn2);
    free (xtable);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS i ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s) WHERE t.\"%s\" IS NULL", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s AND t.\"%s\" = ?", prev, xcolumn1);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT Input */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT INPUT FROM TMP",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - UPDATE tmp SET cut-Geom */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("UPDATE TEMP.\"%s\" SET \"%s\" = ? WHERE ROWID = ?",
			 xtable, xcolumn1);
    free (xcolumn1);
    free (xtable);

/* creating a Prepared Statement - UPDATE tmp SET cut-Geom */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "UPDATE TMP cut-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - renoded Blades */
	  ret = sqlite3_step (stmt_blades);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_blades, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_blades, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_blades, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_blades, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the Blade */
		if (sqlite3_column_type (stmt_blades, icol) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_blades, icol);
		      int blob_sz = sqlite3_column_bytes (stmt_blades, icol);
		      /* cutting all Input geoms intersecting the Blade */
		      if (!do_cut_tmp_linestrings
			  (handle, cache, stmt_in, stmt_upd, &row, message,
			   blob, blob_sz))
			{
			    reset_temporary_row (&row);
			    goto error;
			}
		  }
		else
		  {
		      do_update_message (message, "Unexpected NULL Blade\n");
		      reset_temporary_row (&row);
		      goto error;
		  }

		reset_temporary_row (&row);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: BLADES", sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_blades);
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_upd);
    return 1;

  error:
    if (stmt_blades != NULL)
	sqlite3_finalize (stmt_blades);
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
    return 0;
}

static int
do_insert_temporary_polygons (struct output_table *tbl, sqlite3 * handle,
			      const void *cache, sqlite3_stmt * stmt_out,
			      struct temporary_row *row, gaiaGeomCollPtr geom,
			      char **message, int ngeom)
{
/* inserting a resolved Geometry in the Polygons Helper Table */
    int ret;
    gaiaPolygonPtr pg;
    struct output_column *col;
    struct multivar *var;
    int icol2 = 0;
    int icol = 1;
    int n_geom = 0;
    unsigned char *blob;
    int size;
    int gpkg_mode = 0;
    int tiny_point = 0;
    gaiaGeomCollPtr g;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    if (ngeom < 0)
	n_geom = 0;
    else
	n_geom = ngeom;

    pg = geom->FirstPolygon;
    while (pg != NULL)
      {
	  /* separating elementary geometries */
	  icol2 = 0;
	  icol = 1;
	  if (ngeom < 0)
	      n_geom++;
	  g = do_prepare_polygon (pg, geom->Srid);
	  sqlite3_reset (stmt_out);
	  sqlite3_clear_bindings (stmt_out);
	  col = tbl->first;
	  while (col != NULL)
	    {
		if (col->role == GAIA_CUTTER_INPUT_PK)
		  {
		      /* binding Input PK values */
		      var = find_input_pk_value (row, icol2);
		      if (var == NULL)
			  return 0;
		      icol2++;
		      switch (var->type)
			{
			    /* Input Primary Key Column(s) */
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_out, icol,
						var->value.intValue);
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_out, icol,
						 var->value.doubleValue);
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_out, icol,
					       var->value.textValue,
					       strlen (var->value.textValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_out, icol);
			    break;
			};
		      icol++;
		  }
		col = col->next;
	    }
	  /* binding n_geom */
	  sqlite3_bind_int (stmt_out, icol, n_geom);
	  icol++;
	  icol2 = 0;
	  col = tbl->first;
	  while (col != NULL)
	    {
		if (col->role == GAIA_CUTTER_BLADE_PK)
		  {
		      /* binding Blade PK values */
		      var = find_blade_pk_value (row, icol2);
		      if (var == NULL)
			  return 0;
		      icol2++;
		      switch (var->type)
			{
			    /* Blade Primary Key Column(s) */
			case SQLITE_INTEGER:
			    sqlite3_bind_int64 (stmt_out, icol,
						var->value.intValue);
			    break;
			case SQLITE_FLOAT:
			    sqlite3_bind_double (stmt_out, icol,
						 var->value.doubleValue);
			    break;
			case SQLITE_TEXT:
			    sqlite3_bind_text (stmt_out, icol,
					       var->value.textValue,
					       strlen (var->value.textValue),
					       SQLITE_STATIC);
			    break;
			default:
			    sqlite3_bind_null (stmt_out, icol);
			    break;
			};
		      icol++;
		  }
		col = col->next;
	    }
	  /* binding Geometry */
	  gaiaToSpatiaLiteBlobWkbEx2 (g, &blob, &size, gpkg_mode, tiny_point);
	  if (blob == NULL)
	    {
		do_update_message (message,
				   "UNEXPECTED NULL TEMPORARY POLYGON BLOB GEOMETRY");
		gaiaFreeGeomColl (geom);
		return 0;
	    }
	  sqlite3_bind_blob (stmt_out, icol, blob, size, free);
	  gaiaFreeGeomColl (g);
	  ret = sqlite3_step (stmt_out);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      ;
	  else
	    {
		/* some error occurred */
		do_update_sql_error (message, "INSERT INTO TEMPORARY POLYGONS",
				     sqlite3_errmsg (handle));
		return 0;
	    }
	  pg = pg->Next;
      }
    return 1;
}

static int
do_insert_temporary_polygon_intersection (struct output_table *tbl,
					  sqlite3 * handle,
					  sqlite3_stmt * stmt_out,
					  struct temporary_row *row, int n_geom,
					  char **message)
{
/* inserting an Input/Blade intersection into the Polygons Helper Table */
    int ret;
    struct output_column *col;
    struct multivar *var;
    int icol2 = 0;
    int icol = 1;

    sqlite3_reset (stmt_out);
    sqlite3_clear_bindings (stmt_out);
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		/* binding Input PK values */
		var = find_input_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Input Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    /* binding n_geom */
    sqlite3_bind_int (stmt_out, icol, n_geom);
    icol++;
    icol2 = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		/* binding Blade PK values */
		var = find_blade_pk_value (row, icol2);
		if (var == NULL)
		    return 0;
		icol2++;
		switch (var->type)
		  {
		      /* Blade Primary Key Column(s) */
		  case SQLITE_INTEGER:
		      sqlite3_bind_int64 (stmt_out, icol, var->value.intValue);
		      break;
		  case SQLITE_FLOAT:
		      sqlite3_bind_double (stmt_out, icol,
					   var->value.doubleValue);
		      break;
		  case SQLITE_TEXT:
		      sqlite3_bind_text (stmt_out, icol,
					 var->value.textValue,
					 strlen (var->value.textValue),
					 SQLITE_STATIC);
		      break;
		  default:
		      sqlite3_bind_null (stmt_out, icol);
		      break;
		  };
		icol++;
	    }
	  col = col->next;
      }
    /* binding NULL geometry */
    sqlite3_bind_null (stmt_out, icol);
    ret = sqlite3_step (stmt_out);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

    /* some error occurred */
    do_update_sql_error (message, "INSERT INTO TEMPORARY POLYGONS",
			 sqlite3_errmsg (handle));
    return 0;
}

static int
do_populate_temp_polygons (struct output_table *tbl, sqlite3 * handle,
			   const void *cache, const char *input_db_prefix,
			   const char *input_table, const char *input_geom,
			   const char *blade_db_prefix, const char *blade_table,
			   const char *blade_geom,
			   const char *spatial_index_prefix,
			   const char *spatial_index, const char *tmp_table,
			   int type, char **message)
{
/* populating the temporary helper table - POLYGONs */
    int ret;
    sqlite3_stmt *stmt_main = NULL;
    sqlite3_stmt *stmt_input = NULL;
    sqlite3_stmt *stmt_blade = NULL;
    sqlite3_stmt *stmt_tmp = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int cast2d = 0;
    int cast3d = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
      }

    switch (type)
      {
      case GAIA_POLYGONM:
      case GAIA_MULTIPOLYGONM:
	  cast2d = 1;
	  break;
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGONZM:
	  cast3d = 1;
	  break;
      };

/* composing the SQL statement - main SELECT query */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		sql = sqlite3_mprintf ("%s, b.\"%s\"", prev, xcolumn);
		free (xcolumn);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql = sqlite3_mprintf ("%s FROM \"%s\".\"%s\" AS i", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    sql = sqlite3_mprintf ("%sb.ROWID IN (SELECT pkid FROM ", prev);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (spatial_index_prefix);
    xtable = gaiaDoubleQuotedSql (spatial_index);
    sql = sqlite3_mprintf ("%s \"%s\".\"%s\" WHERE", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    sql =
	sqlite3_mprintf
	("%s xmin <= MbrMaxX(i.\"%s\") AND xmax >= MbrMinX(i.\"%s\") ", prev,
	 xcolumn, xcolumn);
    sqlite3_free (prev);
    prev = sql;
    sql =
	sqlite3_mprintf
	("%s AND ymin <= MbrMaxY(i.\"%s\") AND ymax >= MbrMinY(i.\"%s\")))",
	 prev, xcolumn, xcolumn);
    free (xcolumn);
    sqlite3_free (prev);

/* creating a Prepared Statement - main SELECT query */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_main, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "QUERYING POLYGON INTERSECTIONS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT geometry FROM Input */
    xcolumn = gaiaDoubleQuotedSql (input_geom);
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\" WHERE", xcolumn,
			 xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s AND \"%s\" = ?", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT geometry FROM Input */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_input, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT GEOMETRY FROM INPUT",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT geometry FROM Blade */
    xcolumn = gaiaDoubleQuotedSql (blade_geom);
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("SELECT \"%s\" FROM \"%s\".\"%s\" WHERE", xcolumn,
			 xprefix, xtable);
    free (xcolumn);
    free (xprefix);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s AND \"%s\" = ?", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT geometry FROM Blade */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_blade, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT GEOMETRY FROM BLADE",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* creating a Prepared Statement - Inserting into TMP */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" VALUES (", xtable);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s, ?", prev);
		else
		    sql = sqlite3_mprintf ("%s?", prev);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */
    sql = sqlite3_mprintf ("%s, ?", prev);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the geom columns */
    if (cast2d)
	sql = sqlite3_mprintf ("%s, CastToXY(?))", prev);
    else if (cast3d)
	sql = sqlite3_mprintf ("%s, CastToXYZ(?))", prev);
    else
	sql = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO TMP POLYGONS */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tmp, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO TMP POLYGONS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - checking matching Input/Blade pairs */
	  ret = sqlite3_step (stmt_main);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		gaiaGeomCollPtr input_g = NULL;
		gaiaGeomCollPtr blade_g = NULL;
		gaiaPolygonPtr pg;
		int n_geom = 0;
		unsigned char *input_blob;
		unsigned char *blade_blob;
		int input_blob_sz;
		int blade_blob_sz;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_main, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_main, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_main, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_main, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_main, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_main, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_main, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_main, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }

		/* reading the Input Geometry */
		input_g =
		    do_read_input_geometry (tbl, cache, stmt_input, handle,
					    &row, message, &input_blob,
					    &input_blob_sz);
		if (input_g == NULL)
		    goto error;

		if (is_null_blade (&row))
		  {
		      /* Input doesn't matches any Blade */
		      if (!do_insert_temporary_polygons
			  (tbl, handle, cache, stmt_tmp, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    goto error;
			}
		      goto skip;
		  }

		/* reading the Blade Geometry */
		blade_g =
		    do_read_blade_geometry (tbl, cache, stmt_blade, handle,
					    &row, message, &blade_blob,
					    &blade_blob_sz);
		if (blade_g == NULL)
		    goto error;

		if (is_covered_by
		    (cache, input_g, input_blob, input_blob_sz, blade_g,
		     blade_blob, blade_blob_sz))
		  {
		      /* Input is completely Covered By Blade */
		      if (!do_insert_temporary_polygons
			  (tbl, handle, cache, stmt_tmp, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    gaiaFreeGeomColl (blade_g);
			    goto error;
			}
		      goto skip;
		  }

		if (is_covered_by
		    (cache, blade_g, blade_blob, blade_blob_sz, input_g,
		     input_blob, input_blob_sz))
		  {
		      /* Blade is completely Covered By Input */
		      gaiaGeomCollPtr g =
			  gaiaGeometryIntersection_r (cache, input_g, blade_g);
		      if (!do_insert_temporary_polygons
			  (tbl, handle, cache, stmt_tmp, &row, g, message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    gaiaFreeGeomColl (blade_g);
			    gaiaFreeGeomColl (g);
			    goto error;
			}
		      gaiaFreeGeomColl (g);
		      goto skip;
		  }

		pg = input_g->FirstPolygon;
		while (pg != NULL)
		  {
		      unsigned char *pg_blob;
		      int pg_blob_sz;
		      gaiaGeomCollPtr pg_geom =
			  do_prepare_polygon (pg, input_g->Srid);
		      gaiaToSpatiaLiteBlobWkbEx2 (pg_geom, &pg_blob,
						  &pg_blob_sz, gpkg_mode,
						  tiny_point);
		      n_geom++;
		      if (gaiaGeomCollPreparedIntersects
			  (cache, pg_geom, pg_blob, pg_blob_sz, blade_g,
			   blade_blob, blade_blob_sz))
			{
			    /* saving an Input/Blade intersection */
			    if (!do_insert_temporary_polygon_intersection
				(tbl, handle, stmt_tmp, &row, n_geom, message))
			      {
				  reset_temporary_row (&row);
				  gaiaFreeGeomColl (input_g);
				  gaiaFreeGeomColl (blade_g);
				  gaiaFreeGeomColl (pg_geom);
				  free (pg_blob);
				  goto error;
			      }
			}
		      free (pg_blob);
		      gaiaFreeGeomColl (pg_geom);
		      pg = pg->Next;
		  }

	      skip:
		reset_temporary_row (&row);
		gaiaFreeGeomColl (input_g);
		gaiaFreeGeomColl (blade_g);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: MAIN POLYGONS LOOP",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_main);
    sqlite3_finalize (stmt_input);
    sqlite3_finalize (stmt_blade);
    sqlite3_finalize (stmt_tmp);
    return 1;

  error:
    if (stmt_main == NULL)
	sqlite3_finalize (stmt_main);
    if (stmt_input == NULL)
	sqlite3_finalize (stmt_input);
    if (stmt_blade == NULL)
	sqlite3_finalize (stmt_blade);
    if (stmt_tmp == NULL)
	sqlite3_finalize (stmt_tmp);
    return 0;
}

static int
do_update_tmp_cut_polygon (sqlite3 * handle, sqlite3_stmt * stmt_upd,
			   sqlite3_int64 pk, const unsigned char *blob,
			   int blob_sz, char **message)
{
/* saving an Input Polygon cut against a renoded Blade */
    int ret;

    sqlite3_reset (stmt_upd);
    sqlite3_clear_bindings (stmt_upd);
/* binding the cut Input geometry */
    sqlite3_bind_blob (stmt_upd, 1, blob, blob_sz, free);
    sqlite3_bind_int64 (stmt_upd, 2, pk);
    /* updating the TMP table */
    ret = sqlite3_step (stmt_upd);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	return 1;

    /* some error occurred */
    do_update_sql_error (message,
			 "step: UPDATE TMP SET cut-Polygon",
			 sqlite3_errmsg (handle));
    return 0;
}

static int
do_cut_tmp_polygons (sqlite3 * handle, const void *cache,
		     sqlite3_stmt * stmt_in, sqlite3_stmt * stmt_upd,
		     struct temporary_row *row, char **message,
		     const unsigned char *blade_blob, int blade_blob_sz)
{
/* cutting all Input Polygons intersecting the renoded Blade */
    int ret;
    struct multivar *var;
    int icol = 1;
    gaiaGeomCollPtr blade_g;
    int gpkg_amphibious = 0;
    int gpkg_mode = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  gpkg_mode = pcache->gpkg_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    blade_g = gaiaFromSpatiaLiteBlobWkbEx (blade_blob, blade_blob_sz,
					   gpkg_mode, gpkg_amphibious);

    sqlite3_reset (stmt_in);
    sqlite3_clear_bindings (stmt_in);
    var = row->first_blade;
    while (var != NULL)
      {
	  /* binding Primary Key values (from Blade) */
	  switch (var->type)
	    {
	    case SQLITE_INTEGER:
		sqlite3_bind_int64 (stmt_in, icol, var->value.intValue);
		break;
	    case SQLITE_FLOAT:
		sqlite3_bind_double (stmt_in, icol, var->value.doubleValue);
		break;
	    case SQLITE_TEXT:
		sqlite3_bind_text (stmt_in, icol,
				   var->value.textValue,
				   strlen (var->value.textValue),
				   SQLITE_STATIC);
		break;
	    default:
		sqlite3_bind_null (stmt_in, icol);
		break;
	    };
	  icol++;
	  var = var->next;
      }

    while (1)
      {
	  /* scrolling the result set rows - cut Polygons */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		sqlite3_int64 pk = 0;
		unsigned char *blob = NULL;
		int blob_sz = 0;
		gaiaGeomCollPtr input_g;
		gaiaGeomCollPtr result;
		if (sqlite3_column_type (stmt_in, 0) == SQLITE_INTEGER
		    && sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      pk = sqlite3_column_int64 (stmt_in, 0);
		      blob = (unsigned char *) sqlite3_column_blob (stmt_in, 1);
		      blob_sz = sqlite3_column_bytes (stmt_in, 1);
		      input_g = gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
							     gpkg_mode,
							     gpkg_amphibious);
		      result =
			  gaiaGeometryIntersection_r (cache, input_g, blade_g);
		      if (result != NULL)
			{
			    gaiaToSpatiaLiteBlobWkbEx2 (result, &blob, &blob_sz,
							gpkg_mode, tiny_point);
			    gaiaFreeGeomColl (result);
			}
		      gaiaFreeGeomColl (input_g);
		  }
		if (blob != NULL)
		  {
		      if (!do_update_tmp_cut_polygon
			  (handle, stmt_upd, pk, blob, blob_sz, message))
			  goto error;
		  }
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: cut Polygons",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }
    gaiaFreeGeomColl (blade_g);
    return 1;

  error:
    gaiaFreeGeomColl (blade_g);
    return 0;
}

static int
do_split_polygons (struct output_table *tbl, sqlite3 * handle,
		   const void *cache, const char *input_db_prefix,
		   const char *input_table, const char *input_geom,
		   const char *blade_db_prefix, const char *blade_table,
		   const char *blade_geom, const char *tmp_table,
		   char **message)
{
/* cutting all Input Polygons intersecting some Blade */
    int ret;
    sqlite3_stmt *stmt_blades = NULL;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_upd = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;

/* composing the SQL statement - SELECT FROM Blades */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xcolumn1 = gaiaDoubleQuotedSql (blade_geom);
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql =
	sqlite3_mprintf
	("%s, b.\"%s\" FROM TEMP.\"%s\" AS t", prev, xcolumn1, xtable);
    free (xcolumn1);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND b.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s b.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s) WHERE t.\"%s\" IS NULL GROUP BY", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT FROM Blades */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_blades, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM BLADES",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - SELECT Input */
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xtable = gaiaDoubleQuotedSql (tmp_table);
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf
	("SELECT t.ROWID, ST_GeometryN(i.\"%s\", t.\"%s\") FROM TEMP.\"%s\" AS t",
	 xcolumn1, xcolumn2, xtable);
    free (xcolumn1);
    free (xcolumn2);
    free (xtable);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS i ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql = sqlite3_mprintf ("%s) WHERE t.\"%s\" IS NULL", prev, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s AND t.\"%s\" = ?", prev, xcolumn1);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT Input */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT INPUT FROM TMP",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - UPDATE tmp SET cut-Geom */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("UPDATE TEMP.\"%s\" SET \"%s\" = ? WHERE ROWID = ?",
			 xtable, xcolumn1);
    free (xcolumn1);
    free (xtable);

/* creating a Prepared Statement - UPDATE tmp SET cut-Geom */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "UPDATE TMP cut-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - renoded Blades */
	  ret = sqlite3_step (stmt_blades);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_blades, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_blades, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_blades, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_blades, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the Blade */
		if (sqlite3_column_type (stmt_blades, icol) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_blades, icol);
		      int blob_sz = sqlite3_column_bytes (stmt_blades, icol);
		      /* cutting all Input geoms intersecting the Blade */
		      if (!do_cut_tmp_polygons
			  (handle, cache, stmt_in, stmt_upd, &row, message,
			   blob, blob_sz))
			{
			    reset_temporary_row (&row);
			    goto error;
			}
		  }
		else
		  {
		      do_update_message (message, "Unexpected NULL Blade\n");
		      reset_temporary_row (&row);
		      goto error;
		  }

		reset_temporary_row (&row);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: BLADES", sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_blades);
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_upd);
    return 1;

  error:
    if (stmt_blades != NULL)
	sqlite3_finalize (stmt_blades);
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
    return 0;
}

static gaiaGeomCollPtr
do_compute_diff_polygs (const void *cache, sqlite3_stmt * stmt_diff,
			gaiaPolygonPtr input_pg, int srid,
			gaiaGeomCollPtr union_g)
{
/* computing the difference between two Polygons */
    int ret;
    gaiaGeomCollPtr input_g;
    gaiaGeomCollPtr result = NULL;
    unsigned char *input_blob = NULL;
    int input_blob_sz;
    unsigned char *union_blob = NULL;
    int union_blob_sz;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    sqlite3_reset (stmt_diff);
    sqlite3_clear_bindings (stmt_diff);
    input_g = do_prepare_polygon (input_pg, srid);
    gaiaToSpatiaLiteBlobWkbEx2 (input_g, &input_blob, &input_blob_sz, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (input_g);
    gaiaToSpatiaLiteBlobWkbEx2 (union_g, &union_blob, &union_blob_sz, gpkg_mode,
				tiny_point);
    sqlite3_bind_blob (stmt_diff, 1, input_blob, input_blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_diff, 2, union_blob, union_blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_diff, 3, union_blob, union_blob_sz, SQLITE_STATIC);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_diff);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		if (sqlite3_column_type (stmt_diff, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_diff, 0);
		      int blob_sz = sqlite3_column_bytes (stmt_diff, 0);
		      result =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
	    }
      }

    free (input_blob);
    free (union_blob);
    return result;
}

static int
do_get_uncovered_polygons (struct output_table *tbl, sqlite3 * handle,
			   const void *cache, const char *input_db_prefix,
			   const char *input_table, const char *input_geom,
			   const char *tmp_table, int type, char **message)
{
/* recovering all Input portions not covered by any Blade */
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    sqlite3_stmt *stmt_diff = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int cast2d = 0;
    int cast3d = 0;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
      }

    switch (type)
      {
      case GAIA_POLYGONM:
      case GAIA_MULTIPOLYGONM:
	  cast2d = 1;
	  break;
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGONZM:
	  cast3d = 1;
	  break;
      };

/* composing the SQL statement - union of all already assigned portions */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s, i.\"%s\", ST_UnaryUnion(ST_Collect(t.\"%s\")) ",
			 prev, xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    xcolumn1 = gaiaDoubleQuotedSql (tmp_table);
    sql =
	sqlite3_mprintf
	("%s FROM \"%s\".\"%s\" AS i LEFT JOIN TEMP.\"%s\" AS t ON (", prev,
	 xprefix, xtable, xcolumn1);
    free (xprefix);
    free (xtable);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s) GROUP BY", prev);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT FROM TMP */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM TMP Union-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the Prepared Statement - Inserting into TMP */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" VALUES (", xtable);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s, ?", prev);
		else
		    sql = sqlite3_mprintf ("%s?", prev);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */
    sql = sqlite3_mprintf ("%s, ?", prev);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the geom columns */
    if (cast2d)
	sql = sqlite3_mprintf ("%s, CastToXY(?))", prev);
    else if (cast3d)
	sql = sqlite3_mprintf ("%s, CastToXYZ(?))", prev);
    else
	sql = sqlite3_mprintf ("%s, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO TMP POLYGONS */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO TMP POLYGONS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the Prepared Statement - polygons difference */
    sql =
	sqlite3_mprintf ("SELECT ST_Difference(ST_Snap(?, ?, 0.000000001), ?)");

/* creating a Prepared Statement - polygons difference */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_diff, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "POLYGONS DIFFERENCE",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - from Temporary Helper */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		gaiaGeomCollPtr input_g = NULL;
		gaiaGeomCollPtr union_g = NULL;
		const unsigned char *blob;
		int blob_sz;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    add_null_pk_value (&row, 'B', icol2);
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the "geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, icol);
		      blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      input_g =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
		icol++;
		/* fetching the "union_geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, icol);
		      blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      union_g =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
		if (union_g == NULL)
		  {
		      /* fully uncovered Input Geometry */
		      if (!do_insert_temporary_polygons
			  (tbl, handle, cache, stmt_out, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    goto error;
			}
		  }
		else
		  {
		      /* partialy uncovered Input Geometry */
		      int n_geom = 0;
		      gaiaPolygonPtr pg = input_g->FirstPolygon;
		      while (pg != NULL)
			{
			    gaiaGeomCollPtr diff_g =
				do_compute_diff_polygs (cache, stmt_diff, pg,
							input_g->Srid,
							union_g);
			    n_geom++;
			    if (diff_g != NULL)
			      {
				  if (!do_insert_temporary_polygons
				      (tbl, handle, cache, stmt_out, &row,
				       diff_g, message, n_geom))
				    {
					reset_temporary_row (&row);
					gaiaFreeGeomColl (input_g);
					gaiaFreeGeomColl (union_g);
					gaiaFreeGeomColl (diff_g);
					goto error;
				    }
				  gaiaFreeGeomColl (diff_g);
			      }
			    pg = pg->Next;
			}
		  }

		reset_temporary_row (&row);
		gaiaFreeGeomColl (input_g);
		gaiaFreeGeomColl (union_g);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT FROM TEMPORARY POLIGONS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    sqlite3_finalize (stmt_diff);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    if (stmt_diff != NULL)
	sqlite3_finalize (stmt_diff);
    return 0;
}

static int
do_insert_output_polygons (struct output_table *tbl, sqlite3 * handle,
			   const void *cache, const char *out_table,
			   const char *tmp_table, char **message)
{
/* populating the output table */
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;
    struct temporary_row prev_row;
    int prev_ngeom = -1;
    int prog_res = -1;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
      }

    prev_row.first_input = NULL;
    prev_row.last_input = NULL;
    prev_row.first_blade = NULL;
    prev_row.last_blade = NULL;

/* composing the SQL statement - SELECT FROM TMP */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s \"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn1);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql =
	sqlite3_mprintf ("%s, \"%s\", \"%s\" FROM TEMP.\"%s\" ORDER BY", prev,
			 xcolumn1, xcolumn2, xtable);
    free (xtable);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s, \"%s\", MbrMinY(\"%s\") DESC, MbrMinX(\"%s\")",
			 prev, xcolumn1, xcolumn2, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;

/* creating a Prepared Statement - SELECT FROM TMP */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM TMP cut-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - INSERT INTO Output */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" VALUES(NULL", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, ?", prev);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s, ?, ?, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO OUTPUT */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO OUTPUT POLYGONS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - from Temporary Helper */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		int n_geom = 0;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the "n_geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_INTEGER)
		    n_geom = sqlite3_column_int (stmt_in, icol);
		icol++;
		if (check_same_input (&prev_row, &row) && n_geom == prev_ngeom)
		    ;
		else
		    prog_res = 1;
		prev_ngeom = n_geom;
		copy_input_values (&row, &prev_row);
		/* fetching the "geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      gaiaGeomCollPtr geom;
		      gaiaPolygonPtr pg;
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_in, icol);
		      int blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);

		      pg = geom->FirstPolygon;
		      while (pg)
			{
			    do_insert_output_row
				(tbl, cache, stmt_out, handle, &row, n_geom,
				 prog_res++, GAIA_CUTTER_POLYGON, pg,
				 geom->Srid, message);
			    pg = pg->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
		reset_temporary_row (&row);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT FROM TEMPORARY POLYGONS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    reset_temporary_row (&prev_row);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    reset_temporary_row (&prev_row);
    return 0;
}

static gaiaGeomCollPtr
do_compute_diff_lines (const void *cache, sqlite3_stmt * stmt_diff,
		       gaiaLinestringPtr input_ln, int srid,
		       gaiaGeomCollPtr union_g)
{
/* computing the difference between two Linestrings */
    int ret;
    gaiaGeomCollPtr input_g;
    gaiaGeomCollPtr result = NULL;
    unsigned char *input_blob = NULL;
    int input_blob_sz;
    unsigned char *union_blob = NULL;
    int union_blob_sz;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;
    int tiny_point = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
	  tiny_point = pcache->tinyPointEnabled;
      }

    sqlite3_reset (stmt_diff);
    sqlite3_clear_bindings (stmt_diff);
    input_g = do_prepare_linestring (input_ln, srid);
    gaiaToSpatiaLiteBlobWkbEx2 (input_g, &input_blob, &input_blob_sz, gpkg_mode,
				tiny_point);
    gaiaFreeGeomColl (input_g);
    gaiaToSpatiaLiteBlobWkbEx2 (union_g, &union_blob, &union_blob_sz, gpkg_mode,
				tiny_point);
    sqlite3_bind_blob (stmt_diff, 1, input_blob, input_blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_diff, 2, union_blob, union_blob_sz, SQLITE_STATIC);
    sqlite3_bind_blob (stmt_diff, 3, union_blob, union_blob_sz, SQLITE_STATIC);

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_diff);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		if (sqlite3_column_type (stmt_diff, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_diff, 0);
		      int blob_sz = sqlite3_column_bytes (stmt_diff, 0);
		      result =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
	    }
      }

    free (input_blob);
    free (union_blob);
    return result;
}

static int
do_get_uncovered_linestrings (struct output_table *tbl, sqlite3 * handle,
			      const void *cache, const char *input_db_prefix,
			      const char *input_table, const char *input_geom,
			      const char *tmp_table, int type, char **message)
{
/* recovering all Input portions not covered by any Blade */
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    sqlite3_stmt *stmt_diff = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int cast2d = 0;
    int cast3d = 0;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
      }

    switch (type)
      {
      case GAIA_LINESTRINGM:
      case GAIA_MULTILINESTRINGM:
	  cast2d = 1;
	  break;
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRINGZM:
	  cast3d = 1;
	  break;
      };

/* composing the SQL statement - union of all already assigned portions */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xcolumn1 = gaiaDoubleQuotedSql (input_geom);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    sql =
	sqlite3_mprintf ("%s, i.\"%s\", ST_UnaryUnion(ST_Collect(t.\"%s\")) ",
			 prev, xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    xcolumn1 = gaiaDoubleQuotedSql (tmp_table);
    sql =
	sqlite3_mprintf
	("%s FROM \"%s\".\"%s\" AS i LEFT JOIN TEMP.\"%s\" AS t ON (", prev,
	 xprefix, xtable, xcolumn1);
    free (xprefix);
    free (xtable);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s i.\"%s\" = t.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s) GROUP BY", prev);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, i.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }

/* creating a Prepared Statement - SELECT FROM TMP */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM TMP Union-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the Prepared Statement - Inserting into TMP */
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("INSERT INTO TEMP.\"%s\" VALUES (", xtable);
    free (xtable);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		if (comma)
		    sql = sqlite3_mprintf ("%s, ?", prev);
		else
		    sql = sqlite3_mprintf ("%s?", prev);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the n_geom column */
    sql = sqlite3_mprintf ("%s, ?", prev);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
/* adding the geom columns */
    if (cast2d)
	sql = sqlite3_mprintf ("%s, ?, CastToXY(?))", prev);
    else if (cast3d)
	sql = sqlite3_mprintf ("%s, ?, CastToXYZ(?))", prev);
    else
	sql = sqlite3_mprintf ("%s, ?, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO TMP LINESTRINGS */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO TMP LINESTRINGS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the Prepared Statement - linestrings difference */
    sql =
	sqlite3_mprintf ("SELECT ST_Difference(ST_Snap(?, ?, 0.000000001), ?)");

/* creating a Prepared Statement - linestrings difference */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_diff, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "LINESTRINGS DIFFERENCE",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - from Temporary Helper */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		gaiaGeomCollPtr input_g = NULL;
		gaiaGeomCollPtr union_g = NULL;
		const unsigned char *blob;
		int blob_sz;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    add_null_pk_value (&row, 'B', icol2);
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the "geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, icol);
		      blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      input_g =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
		icol++;
		/* fetching the "union_geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      blob = sqlite3_column_blob (stmt_in, icol);
		      blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      union_g =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);
		  }
		if (union_g == NULL)
		  {
		      /* fully uncovered Input Geometry */
		      if (!do_insert_temporary_linestrings
			  (tbl, handle, cache, stmt_out, &row, input_g,
			   message, -1))
			{
			    reset_temporary_row (&row);
			    gaiaFreeGeomColl (input_g);
			    goto error;
			}
		  }
		else
		  {
		      /* partialy uncovered Input Geometry */
		      int n_geom = 0;
		      gaiaLinestringPtr ln = input_g->FirstLinestring;
		      while (ln != NULL)
			{
			    gaiaGeomCollPtr diff_g =
				do_compute_diff_lines (cache, stmt_diff, ln,
						       input_g->Srid,
						       union_g);
			    n_geom++;
			    if (diff_g != NULL)
			      {
				  if (!do_insert_temporary_linestrings
				      (tbl, handle, cache, stmt_out, &row,
				       diff_g, message, n_geom))
				    {
					reset_temporary_row (&row);
					gaiaFreeGeomColl (input_g);
					gaiaFreeGeomColl (union_g);
					gaiaFreeGeomColl (diff_g);
					goto error;
				    }
				  gaiaFreeGeomColl (diff_g);
			      }
			    ln = ln->Next;
			}
		  }

		reset_temporary_row (&row);
		gaiaFreeGeomColl (input_g);
		gaiaFreeGeomColl (union_g);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT FROM TEMPORARY LINESTRINGS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    sqlite3_finalize (stmt_diff);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    if (stmt_diff != NULL)
	sqlite3_finalize (stmt_diff);
    return 0;
}

static int
do_insert_output_linestrings (struct output_table *tbl, sqlite3 * handle,
			      const void *cache, const char *input_db_prefix,
			      const char *input_table, const char *input_geom,
			      const char *out_table, const char *tmp_table,
			      char **message)
{
/* populating the output table */
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    int gpkg_mode = 0;
    int gpkg_amphibious = 0;
    struct temporary_row prev_row;
    int prev_ngeom = -1;
    int prog_res = -1;

    if (cache != NULL)
      {
	  struct splite_internal_cache *pcache =
	      (struct splite_internal_cache *) cache;
	  gpkg_mode = pcache->gpkg_mode;
	  gpkg_amphibious = pcache->gpkg_amphibious_mode;
      }

    prev_row.first_input = NULL;
    prev_row.last_input = NULL;
    prev_row.first_blade = NULL;
    prev_row.last_blade = NULL;

/* composing the SQL statement - SELECT FROM TMP */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, t.\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql =
	sqlite3_mprintf ("%s, t.\"%s\", t.\"%s\" FROM TEMP.\"%s\" AS t", prev,
			 xcolumn1, xcolumn2, xtable);
    free (xtable);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (input_db_prefix);
    xtable = gaiaDoubleQuotedSql (input_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS i ON(", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->base_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND t.\"%s\" = i.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s t.\"%s\" = i.\"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s) ORDER BY", prev);
    sqlite3_free (prev);
    prev = sql;
    comma = 0;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn1);
		else
		    sql = sqlite3_mprintf ("%s\"%s\"", prev, xcolumn1);
		free (xcolumn1);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xprefix = sqlite3_mprintf ("%s_n_geom", tmp_table);
    xcolumn1 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xprefix = sqlite3_mprintf ("%s_geom", tmp_table);
    xcolumn2 = gaiaDoubleQuotedSql (xprefix);
    sqlite3_free (xprefix);
    xprefix = gaiaDoubleQuotedSql (input_geom);
    sql =
	sqlite3_mprintf
	("%s, \"%s\", ST_Line_Locate_Point(i.\"%s\", ST_StartPoint(ST_GeometryN(t.\"%s\", t.\"%s\")))",
	 prev, xcolumn1, xprefix, xcolumn2, xcolumn1);
    free (xcolumn1);
    free (xcolumn2);
    free (xprefix);
    sqlite3_free (prev);
    prev = sql;

/* creating a Prepared Statement - SELECT FROM TMP */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM TMP cut-Geometries",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* composing the SQL statement - INSERT INTO Output */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("INSERT INTO MAIN.\"%s\" VALUES(NULL", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		sql = sqlite3_mprintf ("%s, ?", prev);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->real_name);
		sql = sqlite3_mprintf ("%s, ?", prev);
		free (xcolumn1);
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s, ?, ?, ?)", prev);
    sqlite3_free (prev);

/* creating a Prepared Statement - INSERT INTO OUTPUT */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "INSERT INTO OUTPUT LINESTRINGS",
			       sqlite3_errmsg (handle));
	  goto error;
      }

    while (1)
      {
	  /* scrolling the result set rows - from Temporary Helper */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		int n_geom = 0;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the "n_geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_INTEGER)
		    n_geom = sqlite3_column_int (stmt_in, icol);
		icol++;
		if (check_same_input (&prev_row, &row) && n_geom == prev_ngeom)
		    ;
		else
		    prog_res = 1;
		prev_ngeom = n_geom;
		copy_input_values (&row, &prev_row);
		/* fetching the "geom" column value */
		if (sqlite3_column_type (stmt_in, icol) == SQLITE_BLOB)
		  {
		      gaiaGeomCollPtr geom;
		      gaiaLinestringPtr ln;
		      const unsigned char *blob =
			  sqlite3_column_blob (stmt_in, icol);
		      int blob_sz = sqlite3_column_bytes (stmt_in, icol);
		      geom =
			  gaiaFromSpatiaLiteBlobWkbEx (blob, blob_sz,
						       gpkg_mode,
						       gpkg_amphibious);

		      ln = geom->FirstLinestring;
		      while (ln)
			{
			    do_insert_output_row
				(tbl, cache, stmt_out, handle, &row, n_geom,
				 prog_res++, GAIA_CUTTER_LINESTRING, ln,
				 geom->Srid, message);
			    ln = ln->Next;
			}
		      gaiaFreeGeomColl (geom);
		  }
		reset_temporary_row (&row);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT FROM TEMPORARY LINESTRINGS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    reset_temporary_row (&prev_row);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    reset_temporary_row (&prev_row);
    return 0;
}

static int
do_insert_output_points (struct output_table *tbl, sqlite3 * handle,
			 const void *cache, const char *input_db_prefix,
			 const char *input_table, const char *input_geom,
			 const char *out_table, const char *tmp_table,
			 char **message)
{
/* populating the Output table - POINTs */
    sqlite3_stmt *stmt_tmp = NULL;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    int ret;
    char *xtable;
    char *xcolumn;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;

/* composing the SQL statement - SELECT FROM Tempory Helper */
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Input Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_INPUT_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\"", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\"", prev, xcolumn);
		else
		    sql = sqlite3_mprintf ("%s \"%s\"", prev, xcolumn);
		free (xcolumn);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xtable = gaiaDoubleQuotedSql (tmp_table);
    sql = sqlite3_mprintf ("%s, touches FROM TEMP.\"%s\"", prev, xtable);
    free (xtable);
    sqlite3_free (prev);

/* creating a Prepared Statement - SELECT FROM Temporary Table */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_tmp, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "SELECT FROM TMP POINTs",
			       sqlite3_errmsg (handle));
	  goto error;
      }

/* creating a Prepared Statement - SELECT Geometry FROM Input */
    if (!do_create_input_statement
	(tbl, handle, input_db_prefix, input_table, input_geom, &stmt_in,
	 message))
	goto error;

/* creating a Prepared Statement - INSERT INTO Output */
    if (!do_create_output_statement
	(tbl, handle, out_table, &stmt_out, message))
	goto error;

    while (1)
      {
	  /* scrolling the result set rows - from Temporary Helper */
	  ret = sqlite3_step (stmt_tmp);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		/* fetched one row from the resultset */
		struct temporary_row row;
		int icol = 0;
		int icol2 = 0;
		int n_geom = 0;
		int ok_touches = -1;
		gaiaGeomCollPtr geom = NULL;
		gaiaPointPtr pt;
		unsigned char *input_blob;
		int input_blob_sz;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Input Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_INPUT_PK)
			{
			    switch (sqlite3_column_type (stmt_tmp, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'I', icol2,
						    sqlite3_column_int64
						    (stmt_tmp, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'I', icol2,
						       sqlite3_column_double
						       (stmt_tmp, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'I', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_tmp, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'I', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_tmp, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_tmp, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_tmp, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_tmp, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }
		/* fetching the "Touches" column value */
		if (sqlite3_column_type (stmt_tmp, icol) == SQLITE_INTEGER)
		    ok_touches = sqlite3_column_int (stmt_tmp, icol);
		if (ok_touches == 1)
		    do_set_null_blade_columns (&row);

		/* reading the Input Geometry */
		geom =
		    do_read_input_geometry (tbl, cache, stmt_in, handle, &row,
					    message, &input_blob,
					    &input_blob_sz);
		if (geom == NULL)
		    goto error;

		n_geom = 0;
		pt = geom->FirstPoint;
		while (pt != NULL)
		  {
		      /* inserting the Output row(s) */
		      n_geom++;
		      if (!do_insert_output_row
			  (tbl, cache, stmt_out, handle, &row, n_geom, 1,
			   GAIA_CUTTER_POINT, pt, geom->Srid, message))
			  goto error;
		      pt = pt->Next;
		  }
		gaiaFreeGeomColl (geom);
		reset_temporary_row (&row);
	    }
	  else
	    {
		do_update_sql_error (message,
				     "step: SELECT FROM TEMPORARY POINTS",
				     sqlite3_errmsg (handle));
		goto error;
	    }
      }

    sqlite3_finalize (stmt_tmp);
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);
    return 1;

  error:
    if (stmt_tmp != NULL)
	sqlite3_finalize (stmt_tmp);
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
    return 0;
}

static void
do_finish_output (struct output_table *tbl, sqlite3 * handle,
		  const char *out_table, const char *geometry,
		  const char *blade_db_prefix, const char *blade_table,
		  const char *blade_geom, const char *spatial_index_prefix,
		  const char *spatial_index)
{
/* assigning the BLADE to all uncut elements */
    int ret;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_out = NULL;
    char *xprefix;
    char *xtable;
    char *xcolumn1;
    char *xcolumn2;
    char *sql;
    char *prev;
    struct output_column *col;
    int comma = 0;
    char *errMsg = NULL;

/* creating the "tmpcutternull" Temporary Table */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("CREATE TEMPORARY TABLE TEMP.tmpcutternull AS "
			   "SELECT rowid AS in_rowid FROM MAIN.\"%s\" WHERE ",
			   xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s AND \"%s\" IS NULL", prev,
					 xcolumn2);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" IS NULL", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  sqlite3_free (errMsg);
	  goto error;
      }

/* preparing the INPUT statement */
    comma = 0;
    sql = sqlite3_mprintf ("SELECT");
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* adding the output PK column  */
	  if (col->role == GAIA_CUTTER_OUTPUT_PK)
	    {
		/* output table primary column */
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		sql = sqlite3_mprintf ("%s i.\"%s\"", prev, xcolumn1);
		sqlite3_free (prev);
		free (xcolumn1);
		prev = sql;
		comma = 1;
	    }
	  col = col->next;
      }
    col = tbl->first;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql =
			sqlite3_mprintf ("%s, b.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		else
		    sql =
			sqlite3_mprintf ("%s b.\"%s\" AS \"%s\"", prev,
					 xcolumn1, xcolumn2);
		free (xcolumn1);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("%s FROM MAIN.\"%s\" AS i", prev, xtable);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (blade_db_prefix);
    xtable = gaiaDoubleQuotedSql (blade_table);
    sql =
	sqlite3_mprintf ("%s JOIN \"%s\".\"%s\" AS b ON (", prev, xprefix,
			 xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn1 = gaiaDoubleQuotedSql (geometry);
    xcolumn2 = gaiaDoubleQuotedSql (blade_geom);
    sql =
	sqlite3_mprintf ("%sST_CoveredBy(i.\"%s\", b.\"%s\") = 1 ", prev,
			 xcolumn1, xcolumn2);
    free (xcolumn1);
    free (xcolumn2);
    sqlite3_free (prev);
    prev = sql;
    sql = sqlite3_mprintf ("%s AND b.ROWID IN (SELECT pkid FROM ", prev);
    sqlite3_free (prev);
    prev = sql;
    xprefix = gaiaDoubleQuotedSql (spatial_index_prefix);
    xtable = gaiaDoubleQuotedSql (spatial_index);
    sql = sqlite3_mprintf ("%s \"%s\".\"%s\" WHERE", prev, xprefix, xtable);
    free (xprefix);
    free (xtable);
    sqlite3_free (prev);
    prev = sql;
    xcolumn1 = gaiaDoubleQuotedSql (geometry);
    sql =
	sqlite3_mprintf
	("%s xmin <= MbrMaxX(i.\"%s\") AND xmax >= MbrMinX(i.\"%s\") ", prev,
	 xcolumn1, xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    sql =
	sqlite3_mprintf
	("%s AND ymin <= MbrMaxY(i.\"%s\") AND ymax >= MbrMinY(i.\"%s\")))",
	 prev, xcolumn1, xcolumn1);
    free (xcolumn1);
    sqlite3_free (prev);
    prev = sql;
    sql =
	sqlite3_mprintf
	("%s WHERE i.rowid IN (SELECT in_rowid FROM TEMP.tmpcutternull)", prev);
    sqlite3_free (prev);

/* creating the OUTPUT prepared statement */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the OUTPUT statement */
    xtable = gaiaDoubleQuotedSql (out_table);
    sql = sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET", xtable);
    free (xtable);
    prev = sql;
    col = tbl->first;
    comma = 0;
    while (col != NULL)
      {
	  /* Blade Primary Key Column(s) */
	  if (col->role == GAIA_CUTTER_BLADE_PK)
	    {
		xcolumn2 = gaiaDoubleQuotedSql (col->real_name);
		if (comma)
		    sql = sqlite3_mprintf ("%s, \"%s\" = ?", prev, xcolumn2);
		else
		    sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn2);
		free (xcolumn2);
		comma = 1;
		sqlite3_free (prev);
		prev = sql;
	    }
	  col = col->next;
      }
    sql = sqlite3_mprintf ("%s WHERE ", prev);
    sqlite3_free (prev);
    prev = sql;
    col = tbl->first;
    while (col != NULL)
      {
	  /* adding the output PK column  */
	  if (col->role == GAIA_CUTTER_OUTPUT_PK)
	    {
		/* output table primary column */
		xcolumn1 = gaiaDoubleQuotedSql (col->base_name);
		sql = sqlite3_mprintf ("%s \"%s\" = ?", prev, xcolumn1);
		sqlite3_free (prev);
		free (xcolumn1);
		prev = sql;
		comma = 1;
	    }
	  col = col->next;
      }

/* creating the OUTPUT prepared statement */
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt_out, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		struct temporary_row row;
		struct multivar *var;
		int icol = 1;
		int icol2 = 0;
		sqlite3_int64 pk;

		row.first_input = NULL;
		row.last_input = NULL;
		row.first_blade = NULL;
		row.last_blade = NULL;

		pk = sqlite3_column_int64 (stmt_in, 0);
		icol2 = 0;
		col = tbl->first;
		while (col != NULL)
		  {
		      /* Blade Primary Key Column(s) */
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    switch (sqlite3_column_type (stmt_in, icol))
			      {
			      case SQLITE_INTEGER:
				  add_int_pk_value (&row, 'B', icol2,
						    sqlite3_column_int64
						    (stmt_in, icol));
				  break;
			      case SQLITE_FLOAT:
				  add_double_pk_value (&row, 'B', icol2,
						       sqlite3_column_double
						       (stmt_in, icol));
				  break;
			      case SQLITE_TEXT:
				  add_text_pk_value (&row, 'B', icol2,
						     (const char *)
						     sqlite3_column_text
						     (stmt_in, icol));
				  break;
			      default:
				  add_null_pk_value (&row, 'B', icol2);
			      };
			    icol++;
			    icol2++;
			}
		      col = col->next;
		  }

		sqlite3_reset (stmt_out);
		sqlite3_clear_bindings (stmt_out);
		col = tbl->first;
		icol = 1;
		icol2 = 0;
		while (col != NULL)
		  {
		      if (col->role == GAIA_CUTTER_BLADE_PK)
			{
			    var = find_blade_pk_value (&row, icol2);
			    if (var == NULL)
				return;
			    icol2++;
			    switch (var->type)
			      {
				  /* Blade Primary Key Column(s) */
			      case SQLITE_INTEGER:
				  sqlite3_bind_int64 (stmt_out, icol,
						      var->value.intValue);
				  break;
			      case SQLITE_FLOAT:
				  sqlite3_bind_double (stmt_out, icol,
						       var->value.doubleValue);
				  break;
			      case SQLITE_TEXT:
				  sqlite3_bind_text (stmt_out, icol,
						     var->value.textValue,
						     strlen (var->
							     value.textValue),
						     SQLITE_STATIC);
				  break;
			      default:
				  sqlite3_bind_null (stmt_out, icol);
				  break;
			      };
			    icol++;
			}
		      col = col->next;
		  }
		sqlite3_bind_int64 (stmt_out, icol, pk);
		ret = sqlite3_step (stmt_out);
		reset_temporary_row (&row);
		if (ret == SQLITE_DONE || ret == SQLITE_ROW)
		    ;
		else
		    goto error;
	    }
	  else
	      goto error;
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_out);

/* dropping the "tmpcutternull" Temporary Table */
    sql = "DROP TABLE TEMP.tmpcutternull";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
	sqlite3_free (errMsg);
    return;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_out != NULL)
	sqlite3_finalize (stmt_out);
}

static int
do_cut_points (struct output_table *tbl, sqlite3 * handle, const void *cache,
	       const char *input_db_prefix, const char *input_table,
	       const char *input_geom, const char *blade_db_prefix,
	       const char *blade_table, const char *blade_geom,
	       const char *spatial_index_prefix, const char *spatial_index,
	       const char *out_table, char **tmp_table, int *drop_tmp_table,
	       char **message)
{
/* cutting Input POINTs */
    if (!do_prepare_temp_points
	(tbl, handle, input_db_prefix, input_table, input_geom, blade_db_prefix,
	 blade_table, blade_geom, spatial_index_prefix, spatial_index,
	 tmp_table, message))
	return 0;
    if (!do_insert_output_points
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 out_table, *tmp_table, message))
	return 0;
    do_finish_output (tbl, handle, out_table, input_geom, blade_db_prefix,
		      blade_table, blade_geom, spatial_index_prefix,
		      spatial_index);

    *drop_tmp_table = 1;
    return 1;
}

static int
do_cut_linestrings (struct output_table *tbl, sqlite3 * handle,
		    const void *cache, const char *input_db_prefix,
		    const char *input_table, const char *input_geom,
		    const char *blade_db_prefix, const char *blade_table,
		    const char *blade_geom, const char *spatial_index_prefix,
		    const char *spatial_index, const char *out_table,
		    char **tmp_table, int *drop_tmp_table, int type,
		    char **message)
{
/* cutting Input LINESTRINGs */
    if (!do_create_temp_linestrings (tbl, handle, tmp_table, message))
	return 0;
    if (!do_populate_temp_linestrings
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 blade_db_prefix, blade_table, blade_geom, spatial_index_prefix,
	 spatial_index, *tmp_table, type, message))
	return 0;
    if (!do_split_linestrings
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 blade_db_prefix, blade_table, blade_geom, *tmp_table, message))
	return 0;
    if (!do_get_uncovered_linestrings
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 *tmp_table, type, message))
	return 0;
    if (!do_insert_output_linestrings
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 out_table, *tmp_table, message))
	return 0;
    do_finish_output (tbl, handle, out_table, input_geom, blade_db_prefix,
		      blade_table, blade_geom, spatial_index_prefix,
		      spatial_index);

    *drop_tmp_table = 1;
    return 1;
}

static int
do_cut_polygons (struct output_table *tbl, sqlite3 * handle, const void *cache,
		 const char *input_db_prefix, const char *input_table,
		 const char *input_geom, const char *blade_db_prefix,
		 const char *blade_table, const char *blade_geom,
		 const char *spatial_index_prefix, const char *spatial_index,
		 const char *out_table, char **tmp_table, int *drop_tmp_table,
		 int type, char **message)
{
/* cutting Input POLYGONs */
    if (!do_create_temp_polygons (tbl, handle, tmp_table, message))
	return 0;
    if (!do_populate_temp_polygons
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 blade_db_prefix, blade_table, blade_geom, spatial_index_prefix,
	 spatial_index, *tmp_table, type, message))
	return 0;
    if (!do_split_polygons
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 blade_db_prefix, blade_table, blade_geom, *tmp_table, message))
	return 0;
    if (!do_get_uncovered_polygons
	(tbl, handle, cache, input_db_prefix, input_table, input_geom,
	 *tmp_table, type, message))
	return 0;
    if (!do_insert_output_polygons
	(tbl, handle, cache, out_table, *tmp_table, message))
	return 0;
    do_finish_output (tbl, handle, out_table, input_geom, blade_db_prefix,
		      blade_table, blade_geom, spatial_index_prefix,
		      spatial_index);

    *drop_tmp_table = 1;
    return 1;
}

SPATIALITE_DECLARE int
gaiaCutter (sqlite3 * handle, const void *cache, const char *xin_db_prefix,
	    const char *input_table, const char *xinput_geom,
	    const char *xblade_db_prefix, const char *blade_table,
	    const char *xblade_geom, const char *out_table, int transaction,
	    int ram_tmp_store, char **message)
{
/* main Cutter tool implementation */
    const char *in_db_prefix = "MAIN";
    const char *blade_db_prefix = "MAIN";
    char *input_geom = NULL;
    char *blade_geom = NULL;
    char *spatial_index_prefix = NULL;
    char *spatial_index = NULL;
    char *tmp_table = NULL;
    int input_type;
    int input_srid;
    int blade_srid;
    int retcode = 0;
    int ret;
    char *errMsg = NULL;
    int pending = 0;
    int drop_spatial_index = 0;
    int drop_tmp_table = 0;
    struct output_table *tbl = NULL;
    const char *sql;
    int pt_type = 0;
    int ln_type = 0;
    int pg_type = 0;

/* testing and validating the arguments */
    do_reset_message (message);
    if (xin_db_prefix != NULL)
	in_db_prefix = xin_db_prefix;
    if (xblade_db_prefix != NULL)
	blade_db_prefix = xblade_db_prefix;
    if (input_table == NULL)
      {
	  do_update_message (message, "ERROR: input table name can't be NULL");
	  goto end;
      }
    if (blade_table == NULL)
      {
	  do_update_message (message, "ERROR: blade table name can't be NULL");
	  goto end;
      }
    if (out_table == NULL)
      {
	  do_update_message (message, "ERROR: output table name can't be NULL");
	  goto end;
      }
    if (!do_check_input
	(handle, in_db_prefix, input_table, xinput_geom, &input_geom,
	 &input_srid, &input_type, message))
	goto end;
    if (!do_check_blade
	(handle, blade_db_prefix, blade_table, xblade_geom, &blade_geom,
	 &blade_srid, message))
	goto end;
    if (!do_check_output (handle, "MAIN", out_table, input_geom, message))
	goto end;
    if (input_srid != blade_srid)
      {
	  do_update_message (message,
			     "ERROR: both input and blade tables must share the same SRID");
	  goto end;
      }
    if (!do_check_nulls
	(handle, in_db_prefix, input_table, input_geom, "INPUT", message))
	goto end;
    if (!do_check_nulls
	(handle, blade_db_prefix, blade_table, blade_geom, "BLADE", message))
	goto end;

/* determining the Output Table layout */
    tbl = alloc_output_table ();
    if (tbl == NULL)
      {
	  do_update_message (message,
			     "ERROR: insufficient memory (OutputTable wrapper)");
	  goto end;
      }
    if (add_column_to_output_table
	(tbl, "PK_UID", "INTEGER", 0, GAIA_CUTTER_OUTPUT_PK, 0) == NULL)
      {
	  do_update_message (message,
			     "ERROR: insufficient memory (OutputTable wrapper)");
	  goto end;
      }
    if (!do_get_input_pk (tbl, handle, in_db_prefix, input_table, message))
	goto end;
    if (!do_get_blade_pk (tbl, handle, blade_db_prefix, blade_table, message))
	goto end;
    if (add_column_to_output_table
	(tbl, "n_geom", "INTEGER", 1, GAIA_CUTTER_NORMAL, 0) == NULL)
      {
	  do_update_message (message,
			     "ERROR: insufficient memory (OutputTable wrapper)");
	  goto end;
      }
    if (add_column_to_output_table
	(tbl, "res_prog", "INTEGER", 1, GAIA_CUTTER_NORMAL, 0) == NULL)
      {
	  do_update_message (message,
			     "ERROR: insufficient memory (OutputTable wrapper)");
	  goto end;
      }

/* setting the Temp Store */
    if (ram_tmp_store)
	sql = "PRAGMA temp_store=2";
    else
	sql = "PRAGMA temp_store=1";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA temp_store", errMsg);
	  sqlite3_free (errMsg);
	  goto end;
      }

    if (transaction)
      {
	  /* starting a Transaction */
	  ret = sqlite3_exec (handle, "BEGIN", NULL, NULL, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		do_update_sql_error (message, "BEGIN", errMsg);
		sqlite3_free (errMsg);
		goto end;
	    }
	  pending = 1;
      }

/* creating the Output Table */
    if (!do_create_output_table
	(tbl, handle, out_table, input_table, blade_table, message))
	goto end;
/* adding the Output Geometry */
    if (!do_create_output_geometry
	(handle, out_table, input_geom, input_srid, input_type, message))
	goto end;
/* verifying the Blade Spatial Index */
    if (!do_verify_blade_spatial_index
	(handle, blade_db_prefix, blade_table, blade_geom,
	 &spatial_index_prefix, &spatial_index, &drop_spatial_index, message))
	goto end;

    switch (input_type)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  pt_type = 1;
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  ln_type = 1;
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  pg_type = 1;
	  break;
      };

    if (pt_type)
      {
	  /* processing Input of (multi)POINT type */
	  if (!do_cut_points
	      (tbl, handle, cache, in_db_prefix, input_table, input_geom,
	       blade_db_prefix, blade_table, blade_geom, spatial_index_prefix,
	       spatial_index, out_table, &tmp_table, &drop_tmp_table, message))
	      goto end;
      }
    if (ln_type)
      {
	  /* processing Input of (multi)LINESTRING type */
	  if (!do_cut_linestrings
	      (tbl, handle, cache, in_db_prefix, input_table, input_geom,
	       blade_db_prefix, blade_table, blade_geom, spatial_index_prefix,
	       spatial_index, out_table, &tmp_table, &drop_tmp_table,
	       input_type, message))
	      goto end;
      }
    if (pg_type)
      {
	  /* processing Input of (multi)POLYGON type */
	  if (!do_cut_polygons
	      (tbl, handle, cache, in_db_prefix, input_table, input_geom,
	       blade_db_prefix, blade_table, blade_geom, spatial_index_prefix,
	       spatial_index, out_table, &tmp_table, &drop_tmp_table,
	       input_type, message))
	      goto end;
      }

    if (drop_tmp_table)
      {
	  /* dropping the Temporary Table */
	  drop_tmp_table = 0;
	  if (!do_drop_tmp_table (handle, tmp_table, message))
	      goto end;
      }

    if (transaction)
      {
	  /* committing the Transaction */
	  ret = sqlite3_exec (handle, "COMMIT", NULL, NULL, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		do_update_sql_error (message, "COMMIT", errMsg);
		sqlite3_free (errMsg);
		goto end;
	    }
	  pending = 0;
      }
    retcode = 1;

/* testing for invalid Output Geoms */
    if (!do_check_valid (handle, out_table, input_geom, message))
	retcode = 2;

  end:
    if (drop_spatial_index)
      {
	  /* dropping the transient Blade Spatial Index */
	  do_drop_blade_spatial_index (handle, spatial_index, message);
      }
    if (drop_tmp_table)
      {
	  /* dropping the Temporary Table */
	  do_drop_tmp_table (handle, tmp_table, message);
      }

/* resetting the Temp Store */
    sql = "PRAGMA temp_store=0";
    ret = sqlite3_exec (handle, sql, NULL, NULL, &errMsg);
    if (ret != SQLITE_OK)
      {
	  do_update_sql_error (message, "PRAGMA temp_store", errMsg);
	  sqlite3_free (errMsg);
      }

    if (input_geom != NULL)
	free (input_geom);
    if (blade_geom != NULL)
	free (blade_geom);
    if (spatial_index_prefix != NULL)
	free (spatial_index_prefix);
    if (spatial_index != NULL)
	sqlite3_free (spatial_index);
    if (tmp_table != NULL)
	sqlite3_free (tmp_table);
    if (tbl != NULL)
	destroy_output_table (tbl);
    if (transaction && pending)
      {
	  /* rolling back the Transaction */
	  ret = sqlite3_exec (handle, "ROLLBACK", NULL, NULL, &errMsg);
	  if (ret != SQLITE_OK)
	    {
		do_update_sql_error (message, "ROLLBACK", errMsg);
		sqlite3_free (errMsg);
	    }
      }
    return retcode;
}

#endif /* end GEOS conditionals */
