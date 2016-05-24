/* 
/ xl2sql.c
/
/ FreeXL Sample code
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ ------------------------------------------------------------------------------
/ 
/ Version: MPL 1.1/GPL 2.0/LGPL 2.1
/ 
/ The contents of this file are subject to the Mozilla Public License Version
/ 1.1 (the "License"); you may not use this file except in compliance with
/ the License. You may obtain a copy of the License at
/ http://www.mozilla.org/MPL/
/ 
/ Software distributed under the License is distributed on an "AS IS" basis,
/ WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/ for the specific language governing rights and limitations under the
/ License.
/
/ The Original Code is the FreeXL library
/
/ The Initial Developer of the Original Code is Alessandro Furieri
/ 
/ Portions created by the Initial Developer are Copyright (C) 2011
/ the Initial Developer. All Rights Reserved.
/ 
/ Contributor(s):
/ Brad Hards
/ 
/ Alternatively, the contents of this file may be used under the terms of
/ either the GNU General Public License Version 2 or later (the "GPL"), or
/ the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
/ in which case the provisions of the GPL or the LGPL are applicable instead
/ of those above. If you wish to allow use of your version of this file only
/ under the terms of either the GPL or the LGPL, and not to allow others to
/ use your version of this file under the terms of the MPL, indicate your
/ decision by deleting the provisions above and replace them with the notice
/ and other provisions required by the GPL or the LGPL. If you do not delete
/ the provisions above, a recipient may use your version of this file under
/ the terms of any one of the MPL, the GPL or the LGPL.
/ 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freexl.h"

static void
make_table_name (const char *prefix, unsigned short index, char *table_name)
{
/* generating an SQL clean table name */
    char buf[2048];
    char *in = buf;
    char *out = table_name;
    sprintf (buf, "%s_%02u", prefix, index);

/* masking for SQL */
    *out++ = '"';
    while (*in != '\0')
      {
	  if (*in == '"')
	      *out++ = '"';
	  *out++ = *in++;
      }
    *out++ = '"';
    *out = '\0';
}

static void
print_sql_string (const char *string)
{
/* printing a well formatted SQL string */
    const char *p = string;
    putchar (',');
    putchar (' ');
    putchar ('\'');
    while (*p != '\0')
      {
	  if (*p == '\'')
	    {
		/* masking any ' as '' */
		putchar ('\'');
	    }
	  putchar (*p);
	  p++;
      }
    putchar ('\'');
}

int
main (int argc, char *argv[])
{
    unsigned int worksheet_index;
    const char *table_prefix = "xl_table";
    char table_name[2048];
    const void *handle;
    int ret;
    unsigned int info;
    unsigned int max_worksheet;
    unsigned int rows;
    unsigned short columns;
    unsigned int row;
    unsigned short col;

    if (argc == 2 || argc == 3)
      {
	  if (argc == 3)
	      table_prefix = argv[2];
      }
    else
      {
	  fprintf (stderr, "usage: xl2sql path.xls [table_prefix]\n");
	  return -1;
      }

/* opening the .XLS file [Workbook] */
    ret = freexl_open (argv[1], &handle);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "OPEN ERROR: %d\n", ret);
	  return -1;
      }

/* checking for Password (obfuscated/encrypted) */
    ret = freexl_get_info (handle, FREEXL_BIFF_PASSWORD, &info);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_PASSWORD] Error: %d\n", ret);
	  goto stop;
      }
    switch (info)
      {
      case FREEXL_BIFF_PLAIN:
	  break;
      case FREEXL_BIFF_OBFUSCATED:
      default:
	  fprintf (stderr, "Password protected: (not accessible)\n");
	  goto stop;
      };

/* querying BIFF Worksheet entries */
    ret = freexl_get_info (handle, FREEXL_BIFF_SHEET_COUNT, &max_worksheet);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "GET-INFO [FREEXL_BIFF_SHEET_COUNT] Error: %d\n",
		   ret);
	  goto stop;
      }

/* SQL output */
    printf ("--\n-- this SQL script was automatically created by xl2sql\n");
    printf ("--\n-- input .xls document was: %s\n--\n", argv[1]);
    printf ("\nBEGIN;\n\n");

    for (worksheet_index = 0; worksheet_index < max_worksheet;
	 worksheet_index++)
      {
	  const char *utf8_worsheet_name;
	  make_table_name (table_prefix, worksheet_index, table_name);
	  ret =
	      freexl_get_worksheet_name (handle, worksheet_index,
					 &utf8_worsheet_name);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "GET-WORKSHEET-NAME Error: %d\n", ret);
		goto stop;
	    }
	  /* selecting the active Worksheet */
	  ret = freexl_select_active_worksheet (handle, worksheet_index);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "SELECT-ACTIVE_WORKSHEET Error: %d\n", ret);
		goto stop;
	    }
	  /* dimensions */
	  ret = freexl_worksheet_dimensions (handle, &rows, &columns);
	  if (ret != FREEXL_OK)
	    {
		fprintf (stderr, "WORKSHEET-DIMENSIONS Error: %d\n", ret);
		goto stop;
	    }

	  printf ("--\n-- creating a DB table\n");
	  printf ("-- extracting data from Worksheet #%u: %s\n--\n",
		  worksheet_index, utf8_worsheet_name);
	  printf ("CREATE TABLE %s (\n", table_name);
	  printf ("\trow_no INTEGER NOT NULL PRIMARY KEY");
	  for (col = 0; col < columns; col++)
	      printf (",\n\tcol_%03u MULTITYPE", col);
	  printf (");\n");

	  printf ("--\n-- populating the same table\n--\n");
	  for (row = 0; row < rows; row++)
	    {
		/* INSERT INTO statements */
		FreeXL_CellValue cell;

		printf ("INSERT INTO %s (row_no", table_name);
		for (col = 0; col < columns; col++)
		    printf (", col_%03u", col);
		printf (") VALUES (%u", row);
		for (col = 0; col < columns; col++)
		  {
		      ret = freexl_get_cell_value (handle, row, col, &cell);
		      if (ret != FREEXL_OK)
			{
			    fprintf (stderr,
				     "CELL-VALUE-ERROR (r=%u c=%u): %d\n", row,
				     col, ret);
			    goto stop;
			}
		      switch (cell.type)
			{
			case FREEXL_CELL_INT:
			    printf (", %d", cell.value.int_value);
			    break;
			case FREEXL_CELL_DOUBLE:
			    printf (", %1.12f", cell.value.double_value);
			    break;
			case FREEXL_CELL_TEXT:
			case FREEXL_CELL_SST_TEXT:
			    print_sql_string (cell.value.text_value);
			    break;
			case FREEXL_CELL_DATE:
			case FREEXL_CELL_DATETIME:
			case FREEXL_CELL_TIME:
			    printf (", '%s'", cell.value.text_value);
			    break;
			case FREEXL_CELL_NULL:
			default:
			    printf (", NULL");
			    break;
			};
		  }
		printf (");\n");
	    }
	  printf ("\n-- done: table end\n\n\n\n");
      }
    printf ("COMMIT;\n");

  stop:
/* closing the .XLS file [Workbook] */
    ret = freexl_close (handle);
    if (ret != FREEXL_OK)
      {
	  fprintf (stderr, "CLOSE ERROR: %d\n", ret);
	  return -1;
      }

    return 0;
}
