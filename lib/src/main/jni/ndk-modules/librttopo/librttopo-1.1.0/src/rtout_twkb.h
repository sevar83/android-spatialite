/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 *
 * rttopo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * rttopo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rttopo.  If not, see <http://www.gnu.org/licenses/>.
 *
 **********************************************************************
 *
 * Copyright 2013 Nicklas Avén
 * Copyright 2013 Nicklas Avén
 *
 **********************************************************************/



/**********************************************************************
 *
 * rttopo - topology library
 * http://git.osgeo.org/gitea/rttopo/librttopo
 * Copyright 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librttopo_geom_internal.h"
#include "rtgeom_log.h"
#include <limits.h>
#include "bytebuffer.h"

/* Maximum number of geometry dimmensions that internal arrays can hold */
#define MAX_N_DIMS 4

#define MAX_BBOX_SIZE 64
#define MAX_SIZE_SIZE 8


/**
* Header true/false flags
*/

#define FIRST_BYTE_SET_BBOXES(flag, bool)   ((flag) = ((bool) ? (flag) | 0x01 : (flag) & (~0x01)))
#define FIRST_BYTE_SET_SIZES(flag, bool)    ((flag) = ((bool) ? (flag) | 0x02 : (flag) & (~0x02)))
#define FIRST_BYTE_SET_IDLIST(flag, bool)   ((flag) = ((bool) ? (flag) | 0x04 : (flag) & (~0x04)))
#define FIRST_BYTE_SET_EXTENDED(flag, bool) ((flag) = ((bool) ? (flag) | 0x08 : (flag) & (~0x08)))
#define FIRST_BYTE_SET_EMPTY(flag, bool)    ((flag) = ((bool) ? (flag) | 0x10 : (flag) & (~0x10)))


/**
* Macros for manipulating the 'type_precision' int. An int8_t used as follows:
* Type 4 bits
* Precision 4 bits
*/

#define RTTYPE_PREC_SET_TYPE(flag, type) ((flag) = ((flag) & 0xF0) | (((type) & 0x0F)))
#define TYPE_PREC_SET_PREC(flag, prec) ((flag) = ((flag) & 0x0F) | (((prec) & 0x0F) << 4))

#define HIGHER_DIM_SET_HASZ(flag, bool) ((flag) = ((bool) ? (flag) | 0x01 : (flag) & (~0x01)))
#define HIGHER_DIM_SET_HASM(flag, bool) ((flag) = ((bool) ? (flag) | 0x02 : (flag) & (~0x02)))

#define HIGHER_DIM_SET_PRECZ(flag, prec) ((flag) = ((flag) & 0xE3) | (((prec) & 0x07) << 2))
#define HIGHER_DIM_SET_PRECM(flag, prec) ((flag) = ((flag) & 0x1F) | (((prec) & 0x07) << 5))

typedef struct
{
  /* Options defined at start */
  uint8_t variant;
  int8_t prec_xy;
  int8_t prec_z;
  int8_t prec_m;
  float factor[4]; /*What factor to multiply the coordiinates with to get the requested precision*/
} TWKB_GLOBALS;

typedef struct
{
  uint8_t variant;  /*options that change at runtime*/
  bytebuffer_t *header_buf;
  bytebuffer_t *geom_buf;
  int hasz;
  int hasm;
  const int64_t *idlist;
  int64_t bbox_min[MAX_N_DIMS];
  int64_t bbox_max[MAX_N_DIMS];
  int64_t accum_rels[MAX_N_DIMS]; /*Holds the acculmulated relative values*/
} TWKB_STATE;

static int rtgeom_to_twkb_buf(const RTCTX *ctx, const RTGEOM *geom, TWKB_GLOBALS *global_values, TWKB_STATE *ts);

static int rtpoint_to_twkb_buf(const RTCTX *ctx, const RTPOINT *line, TWKB_GLOBALS *global_values, TWKB_STATE *ts);
static int rtline_to_twkb_buf(const RTCTX *ctx, const RTLINE *line, TWKB_GLOBALS *global_values, TWKB_STATE *ts);
static int rtpoly_to_twkb_buf(const RTCTX *ctx, const RTPOLY *poly, TWKB_GLOBALS *global_values, TWKB_STATE *ts);
static int rtcollection_to_twkb_buf(const RTCTX *ctx, const RTCOLLECTION *col, TWKB_GLOBALS *global_values, TWKB_STATE *ts);
static int rtgeom_write_to_buffer(const RTCTX *ctx, const RTGEOM *geom, TWKB_GLOBALS *global_values, TWKB_STATE *parent_state);

