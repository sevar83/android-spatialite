/* 
/ walk_sst_oocalc97.c
/
/ Test of SST operations - LibreOffice (BIFF8)
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
*/#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "freexl.h"

const char * expected_string[] = { 
    "Col 1",
    "Col 2",
    "time",
    "Datetime",
    "date",
    "A",
    "B"
};

int main (int argc, char *argv[])
{
    const void *handle;
    int ret;
    unsigned int sst_count;
    int i;
    const char *sst_entry;

    ret = freexl_open ("testdata/oocalc_simple97.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN ERROR: %d\n", ret);
	return -1;
    }
    
    ret = freexl_get_info(handle, FREEXL_BIFF_STRING_COUNT, &sst_count);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for SST string count: %d\n", ret);
	return -2;
    }
    if (sst_count != 7) {
	fprintf(stderr, "Unexpected SST string count: %d\n", sst_count);
	return -3;
    }

    for (i = 0; i < sst_count; ++i) {
	ret = freexl_get_SST_string (handle, i, &sst_entry);
	if (ret != FREEXL_OK) {
	    fprintf(stderr, "get_SST_string error for %i: %d\n", i, ret);
	    return -4;
	}
	if (strcmp(sst_entry, expected_string[i]) != 0) {
	    fprintf(stderr, "string mismatch at %i: got %s expected %s\n",
		    i, sst_entry, expected_string[i]);
	    return -5;
	}
    }
    
    /* error case checks */
    ret = freexl_get_SST_string (handle, sst_count, &sst_entry);
    if (ret != FREEXL_BIFF_ILLEGAL_SST_INDEX) {
	fprintf(stderr, "get_SST_string error for +1: %d\n", ret);
	return -6;
    }

    ret = freexl_get_SST_string (NULL, 0, &sst_entry);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "get_SST_string error for null handle: %d\n", ret);
	return -7;
    }

    ret = freexl_get_SST_string (handle, 0, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "get_SST_string error for null arg: %d\n", ret);
	return -8;
    }

    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -6;
    }

    return 0;
}
