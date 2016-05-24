/* 
/ check_boolean_biff8.c
/
/ Test cases for Excel built-in BOOLERR values
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
   
    ret = freexl_open ("testdata/testbool.xls", &handle);
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

    ret = freexl_select_active_worksheet (handle, 0);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error setting active worksheet: %d\n", ret);
	return -4;
    }
    
    ret = freexl_worksheet_dimensions (handle, &num_rows, &num_columns);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting worksheet dimensions: %d\n", ret);
	return -5;
    }
    if ((num_rows != 3) || (num_columns != 4)) {
	fprintf(stderr, "Unexpected active sheet dimensions: %u x %u\n",
	    num_rows, num_columns);
	return -5;
    }

    ret = freexl_get_cell_value (handle, 1, 1, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (1,1): %d\n", ret);
	return -6;
    }
    if (cell_value.type != FREEXL_CELL_INT) {
	fprintf(stderr, "Unexpected cell (1,1) type: %u\n", cell_value.type);
	return -7;
    }
    if (cell_value.value.int_value != 0) {
	fprintf(stderr, "Unexpected cell (1,1) value: %d\n", cell_value.value.int_value);
	return -8;
    }

    ret = freexl_get_cell_value (handle, 2, 1, &cell_value);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "Error getting cell value (2,1): %d\n", ret);
	return -9;
    }
    if (cell_value.type != FREEXL_CELL_INT) {
	fprintf(stderr, "Unexpected cell (2,1) type: %u\n", cell_value.type);
	return -10;
    }
    if (cell_value.value.int_value != 1) {
	fprintf(stderr, "Unexpected cell (2,1) value: %d\n", cell_value.value.int_value);
	return -11;
    }
    
    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -12;
    }

    return 0;
}
