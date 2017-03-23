/* 
/ walk_fat_oocal97.c
/
/ Test of FAT operations - LibreOffice (BIFF8)
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

const unsigned int expected_fat_flag[] = { 
        0xfffffffd,
        0xffffffff,
        0xfffffffe,
        0x00000004,
        0x00000005,
        0x00000006,
        0x00000007,
        0x00000008,
        0x00000009,
        0xfffffffe,
        0x0000000b,
        0xfffffffe,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff,
        0xffffffff
 };
 
int main (int argc, char *argv[])
{
    const void *handle;
    int ret;
    unsigned int fat_count = 0;
    unsigned int fat_flag = 0;
    int i;

    ret = freexl_open ("testdata/oocalc_simple97.xls", &handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "OPEN ERROR: %d\n", ret);
	return -1;
    }
    
    ret = freexl_get_info(handle, FREEXL_CFBF_FAT_COUNT, &fat_count);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "GET_INFO ERROR for FAT entries count: %d\n", ret);
	return -2;
    }
    if (fat_count != 128) {
	fprintf(stderr, "Unexpected FAT entries count: %d\n", fat_count);
	return -3;
    }

    for (i = 0; i < fat_count; ++i) {
	ret = freexl_get_FAT_entry (handle, i, &fat_flag);
	if (ret != FREEXL_OK) {
	    fprintf(stderr, "get_FAT_entry error for %i: %d\n", i, ret);
	    return -4;
	}
	if (fat_flag != expected_fat_flag[i]) {
	    fprintf(stderr, "unexpected flag at %i: 0x%08x (expected 0x%08x)\n",
		    i, fat_flag, expected_fat_flag[i]);
	}
    }
    
    /* error case checks */
    ret = freexl_get_FAT_entry (handle, fat_count, &fat_flag);
    if (ret != FREEXL_CFBF_ILLEGAL_FAT_ENTRY) {
	fprintf(stderr, "get_FAT_entry error for +1: %d\n", ret);
	return -6;
    }

    ret = freexl_get_FAT_entry (NULL, 0, &fat_flag);
    if (ret != FREEXL_NULL_HANDLE) {
	fprintf(stderr, "get_FAT_entry error for null handle: %d\n", ret);
	return -7;
    }

    ret = freexl_get_FAT_entry (handle, 0, NULL);
    if (ret != FREEXL_NULL_ARGUMENT) {
	fprintf(stderr, "get_FAT_entry error for null arg: %d\n", ret);
	return -8;
    }

    ret = freexl_close (handle);
    if (ret != FREEXL_OK) {
	fprintf(stderr, "CLOSE ERROR: %d\n", ret);
	return -6;
    }

    return 0;
}
