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
 * Copyright 2015 Nicklas Avén <nicklas.aven@jordogskog.no>
 *
 **********************************************************************/



#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "varint.h"

#include "rtgeom_log.h"
#define BYTEBUFFER_STARTSIZE 128

typedef struct
{
  size_t capacity;
  uint8_t *buf_start;
  uint8_t *writecursor;
  uint8_t *readcursor;
}
bytebuffer_t;

void bytebuffer_init_with_size(const RTCTX *ctx, bytebuffer_t *b, size_t size);
bytebuffer_t *bytebuffer_create_with_size(const RTCTX *ctx, size_t size);
bytebuffer_t *bytebuffer_create(const RTCTX *ctx);
void bytebuffer_destroy(const RTCTX *ctx, bytebuffer_t *s);
void bytebuffer_clear(const RTCTX *ctx, bytebuffer_t *s);
void bytebuffer_append_byte(const RTCTX *ctx, bytebuffer_t *s, const uint8_t val);
void bytebuffer_append_varint(const RTCTX *ctx, bytebuffer_t *s, const int64_t val);
void bytebuffer_append_uvarint(const RTCTX *ctx, bytebuffer_t *s, const uint64_t val);
uint64_t bytebuffer_read_uvarint(const RTCTX *ctx, bytebuffer_t *s);
int64_t bytebuffer_read_varint(const RTCTX *ctx, bytebuffer_t *s);
size_t bytebuffer_getlength(const RTCTX *ctx, bytebuffer_t *s);
bytebuffer_t* bytebuffer_merge(const RTCTX *ctx, bytebuffer_t **buff_array, int nbuffers);
void bytebuffer_reset_reading(const RTCTX *ctx, bytebuffer_t *s);

void bytebuffer_append_bytebuffer(const RTCTX *ctx, bytebuffer_t *write_to,bytebuffer_t *write_from);
void bytebuffer_append_bulk(const RTCTX *ctx, bytebuffer_t *s, void * start, size_t size);
void bytebuffer_append_int(const RTCTX *ctx, bytebuffer_t *buf, const int val, int swap);
void bytebuffer_append_double(const RTCTX *ctx, bytebuffer_t *buf, const double val, int swap);
#endif /* _BYTEBUFFER_H */
