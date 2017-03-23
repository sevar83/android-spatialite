/* 
/ check_datetime_biff8.c
/
/ Test cases for Excel built-in date and time formats
/
/ Author: Brad Hards bradh@frogmouth.net
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

int main (int argc, char *argv[])
{
    const void *handle;
    int ret;
    unsigned int info;
    const char *worksheet_name;
    unsigned short active_idx;
    unsigned int num_rows;
    unsigned short num_columns;
    FreeXL_CellValue cell_value;
   
    ret = freexl_open ("testdata/datetime2003.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN ERROR: %d\n", ret);
	return -1;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_VERSION, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF version: %d\n", ret);
	return -2;
    }
    if (info != FREEXL_BIFF_VER_8)
    {
	fprintf(stderr, "Unexpected BIFF version: %d\n", info);
	return -3;
    }

    ret = freexl_get_info(handle, FREEXL_BIFF_DATEMODE, &info);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for BIFF date mode: %d\n", ret);
	return -4;
    }
    if (info != FREEXL_BIFF_DATEMODE_1900)
    {
	fprintf(stderr, "Unexpected BIFF date mode: %d\n", info);
	return -5;
    }

    ret = freexl_select_active_worksheet (handle, 0);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error setting active worksheet: %d\n", ret);
	return -6;
    }
    
    ret = freexl_worksheet_dimensions (handle, &num_rows, &num_columns);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet dimensions: %d\n", ret);
	return -7;
    }
    if ((num_rows != 15) || (num_columns != 2)) {
	fprintf(stderr, "Unexpected active sheet dimensions: %u x %u\n",
	    num_rows, num_columns);
	return -8;
    }

    ret = freexl_get_cell_value (handle, 1, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,0): %d\n", ret);
	return -9;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (1,0) type: %u\n", cell_value.type);
	return -10;
    }
    if (strcmp(cell_value.value.text_value, "09:58:45") != 0) {
	fprintf(stderr, "Unexpected cell (1,0) value: %s\n", cell_value.value.text_value);
	return -11;
    }

    ret = freexl_get_cell_value (handle, 2, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (2,0): %d\n", ret);
	return -12;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (2,0) type: %u\n", cell_value.type);
	return -13;
    }
    if (strcmp(cell_value.value.text_value, "22:45:00") != 0) {
	fprintf(stderr, "Unexpected cell (2,0) value: %s\n", cell_value.value.text_value);
	return -14;
    }
    
    ret = freexl_get_cell_value (handle, 3, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (3,0): %d\n", ret);
	return -15;
    }
    if (cell_value.type != FREEXL_CELL_DATETIME) {
	fprintf(stderr, "Unexpected cell (3,0) type: %u\n", cell_value.type);
	return -16;
    }
    if (strcmp(cell_value.value.text_value, "2007-11-06 16:28:00") != 0) {
	fprintf(stderr, "Unexpected cell (3,0) value: %s\n", cell_value.value.text_value);
	return -17;
    }

    ret = freexl_get_cell_value (handle, 4, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (4,0): %d\n", ret);
	return -18;
    }
    if (cell_value.type != FREEXL_CELL_DATE) {
	fprintf(stderr, "Unexpected cell (4,0) type: %u\n", cell_value.type);
	return -19;
    }
    if (strcmp(cell_value.value.text_value, "2007-03-23") != 0) {
	fprintf(stderr, "Unexpected cell (4,0) value: %s\n", cell_value.value.text_value);
	return -20;
    }

    ret = freexl_get_cell_value (handle, 5, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (5,0): %d\n", ret);
	return -21;
    }
    if (cell_value.type != FREEXL_CELL_DATE) {
	fprintf(stderr, "Unexpected cell (5,0) type: %u\n", cell_value.type);
	return -22;
    }
    if (strcmp(cell_value.value.text_value, "2005-06-04") != 0) {
	fprintf(stderr, "Unexpected cell (5,0) value: %s\n", cell_value.value.text_value);
	return -23;
    }

    ret = freexl_get_cell_value (handle, 6, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (6,0): %d\n", ret);
	return -24;
    }
    if (cell_value.type != FREEXL_CELL_DATE) {
	fprintf(stderr, "Unexpected cell (6,0) type: %u\n", cell_value.type);
	return -25;
    }
    if (strcmp(cell_value.value.text_value, "2011-07-05") != 0) {
	fprintf(stderr, "Unexpected cell (6,0) value: %s\n", cell_value.value.text_value);
	return -26;
    }

    ret = freexl_get_cell_value (handle, 7, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (7,0): %d\n", ret);
	return -27;
    }
    if (cell_value.type != FREEXL_CELL_DATE) {
	fprintf(stderr, "Unexpected cell (7,0) type: %u\n", cell_value.type);
	return -28;
    }
    if (strcmp(cell_value.value.text_value, "2012-10-01") != 0) {
	fprintf(stderr, "Unexpected cell (7,0) value: %s\n", cell_value.value.text_value);
	return -29;
    }
    
    ret = freexl_get_cell_value (handle, 8, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (8,0): %d\n", ret);
	return -30;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (8,0) type: %u\n", cell_value.type);
	return -31;
    }
    if (strcmp(cell_value.value.text_value, "12:26:00") != 0) {
	fprintf(stderr, "Unexpected cell (8,0) value: %s\n", cell_value.value.text_value);
	return -32;
    }

    ret = freexl_get_cell_value (handle, 9, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (9,0): %d\n", ret);
	return -33;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (9,0) type: %u\n", cell_value.type);
	return -34;
    }
    if (strcmp(cell_value.value.text_value, "00:25:46") != 0) {
	fprintf(stderr, "Unexpected cell (9,0) value: %s\n", cell_value.value.text_value);
	return -35;
    }

    ret = freexl_get_cell_value (handle, 10, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (9,0): %d\n", ret);
	return -36;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (10,0) type: %u\n", cell_value.type);
	return -37;
    }
    if (strcmp(cell_value.value.text_value, "13:12:00") != 0) {
	fprintf(stderr, "Unexpected cell (10,0) value: %s\n", cell_value.value.text_value);
	return -38;
    }
 
    ret = freexl_get_cell_value (handle, 11, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (11,0): %d\n", ret);
	return -39;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (11,0) type: %u\n", cell_value.type);
	return -40;
    }
    if (strcmp(cell_value.value.text_value, "14:56:30") != 0) {
	fprintf(stderr, "Unexpected cell (11,0) value: %s\n", cell_value.value.text_value);
	return -41;
    }
    

    ret = freexl_get_cell_value (handle, 12, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (12,0): %d\n", ret);
	return -42;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (12,0) type: %u\n", cell_value.type);
	return -43;
    }
    if (strcmp(cell_value.value.text_value, "00:45:04") != 0) {
	fprintf(stderr, "Unexpected cell (12,0) value: %s\n", cell_value.value.text_value);
	return -44;
    }


    ret = freexl_get_cell_value (handle, 13, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (13,0): %d\n", ret);
	return -45;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (13,0) type: %u\n", cell_value.type);
	return -46;
    }
    if (strcmp(cell_value.value.text_value, "04:45:02") != 0) {
	fprintf(stderr, "Unexpected cell (13,0) value: %s\n", cell_value.value.text_value);
	return -47;
    }

    ret = freexl_get_cell_value (handle, 14, 0, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (14,0): %d\n", ret);
	return -48;
    }
    if (cell_value.type != FREEXL_CELL_TIME) {
	fprintf(stderr, "Unexpected cell (14,0) type: %u\n", cell_value.type);
	return -49;
    }
    if (strcmp(cell_value.value.text_value, "04:45:30") != 0) {
	fprintf(stderr, "Unexpected cell (14,0) value: %s\n", cell_value.value.text_value);
	return -50;
    }

    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -51;
    }

    return 0;
}
