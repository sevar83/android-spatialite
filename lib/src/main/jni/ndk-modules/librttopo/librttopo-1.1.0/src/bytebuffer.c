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




#include "rttopo_config.h"
#include "librttopo_geom_internal.h"
#include "bytebuffer.h"

/**
* Allocate a new bytebuffer_t. Use bytebuffer_destroy to free.
*/
bytebuffer_t*
bytebuffer_create(const RTCTX *ctx)
{
  RTDEBUG(ctx, 2,"Entered bytebuffer_create");
  return bytebuffer_create_with_size(ctx, BYTEBUFFER_STARTSIZE);
}

/**
* Allocate a new bytebuffer_t. Use bytebuffer_destroy to free.
*/
bytebuffer_t*
bytebuffer_create_with_size(const RTCTX *ctx, size_t size)
{
  RTDEBUGF(ctx, 2,"Entered bytebuffer_create_with_size %d", size);
  bytebuffer_t *s;

  s = rtalloc(ctx, sizeof(bytebuffer_t));
  s->buf_start = rtalloc(ctx, size);
  s->readcursor = s->writecursor = s->buf_start;
  s->capacity = size;
  memset(s->buf_start,0,size);
  RTDEBUGF(ctx, 4,"We create a buffer on %p of %d bytes", s->buf_start, size);
  return s;
}

/**
* Allocate just the internal buffer of an existing bytebuffer_t
* struct. Useful for allocating short-lived bytebuffers off the stack.
*/
void
bytebuffer_init_with_size(const RTCTX *ctx, bytebuffer_t *b, size_t size)
{
  b->buf_start = rtalloc(ctx, size);
  b->readcursor = b->writecursor = b->buf_start;
  b->capacity = size;
  memset(b->buf_start, 0, size);
}

/**
* Free the bytebuffer_t and all memory managed within it.
*/
void
bytebuffer_destroy(const RTCTX *ctx, bytebuffer_t *s)
{
  RTDEBUG(ctx, 2,"Entered bytebuffer_destroy");
  RTDEBUGF(ctx, 4,"The buffer has used %d bytes",bytebuffer_getlength(ctx, s));

  if ( s->buf_start )
  {
    RTDEBUGF(ctx, 4,"let's free buf_start %p",s->buf_start);
    rtfree(ctx, s->buf_start);
    RTDEBUG(ctx, 4,"buf_start is freed");
  }
  if ( s )
  {
    rtfree(ctx, s);
    RTDEBUG(ctx, 4,"bytebuffer_t is freed");
  }
  return;
}

/**
* Set the read cursor to the beginning
*/
void
bytebuffer_reset_reading(const RTCTX *ctx, bytebuffer_t *s)
{
  s->readcursor = s->buf_start;
}

/**
* Reset the bytebuffer_t. Useful for starting a fresh string
* without the expense of freeing and re-allocating a new
* bytebuffer_t.
*/
void
bytebuffer_clear(const RTCTX *ctx, bytebuffer_t *s)
{
  s->readcursor = s->writecursor = s->buf_start;
}

/**
* If necessary, expand the bytebuffer_t internal buffer to accomodate the
* specified additional size.
*/
static inline void
bytebuffer_makeroom(const RTCTX *ctx, bytebuffer_t *s, size_t size_to_add)
{
  RTDEBUGF(ctx, 2,"Entered bytebuffer_makeroom with space need of %d", size_to_add);
  size_t current_write_size = (s->writecursor - s->buf_start);
  size_t capacity = s->capacity;
  size_t required_size = current_write_size + size_to_add;

  RTDEBUGF(ctx, 2,"capacity = %d and required size = %d",capacity ,required_size);
  while (capacity < required_size)
    capacity *= 2;

  if ( capacity > s->capacity )
  {
    RTDEBUGF(ctx, 4,"We need to realloc more memory. New capacity is %d", capacity);
    s->buf_start = rtrealloc(ctx, s->buf_start, capacity);
    s->capacity = capacity;
    s->writecursor = s->buf_start + current_write_size;
    s->readcursor = s->buf_start + (s->readcursor - s->buf_start);
  }
  return;
}

/**
* Writes a uint8_t value to the buffer
*/
void
bytebuffer_append_byte(const RTCTX *ctx, bytebuffer_t *s, const uint8_t val)
{
  RTDEBUGF(ctx, 2,"Entered bytebuffer_append_byte with value %d", val);
  bytebuffer_makeroom(ctx, s, 1);
  *(s->writecursor)=val;
  s->writecursor += 1;
  return;
}


/**
* Writes a uint8_t value to the buffer
*/
void
bytebuffer_append_bulk(const RTCTX *ctx, bytebuffer_t *s, void * start, size_t size)
{
  RTDEBUGF(ctx, 2,"bytebuffer_append_bulk with size %d",size);
  bytebuffer_makeroom(ctx, s, size);
  memcpy(s->writecursor, start, size);
  s->writecursor += size;
  return;
}

/**
* Writes a uint8_t value to the buffer
*/
void
bytebuffer_append_bytebuffer(const RTCTX *ctx, bytebuffer_t *write_to,bytebuffer_t *write_from )
{
  RTDEBUG(ctx, 2,"bytebuffer_append_bytebuffer");
  size_t size = bytebuffer_getlength(ctx, write_from);
  bytebuffer_makeroom(ctx, write_to, size);
  memcpy(write_to->writecursor, write_from->buf_start, size);
  write_to->writecursor += size;
  return;
}


/**
* Writes a signed varInt to the buffer
*/
void
bytebuffer_append_varint(const RTCTX *ctx, bytebuffer_t *b, const int64_t val)
{
  size_t size;
  bytebuffer_makeroom(ctx, b, 8);
  size = varint_s64_encode_buf(ctx, val, b->writecursor);
  b->writecursor += size;
  return;
}

/**
* Writes a unsigned varInt to the buffer
*/
void
bytebuffer_append_uvarint(const RTCTX *ctx, bytebuffer_t *b, const uint64_t val)
{
  size_t size;
  bytebuffer_makeroom(ctx, b, 8);
  size = varint_u64_encode_buf(ctx, val, b->writecursor);
  b->writecursor += size;
  return;
}


/*
* Writes Integer to the buffer
*/
void
bytebuffer_append_int(const RTCTX *ctx, bytebuffer_t *buf, const int val, int swap)
{
  RTDEBUGF(ctx, 2,"Entered bytebuffer_append_int with value %d, swap = %d", val, swap);

  RTDEBUGF(ctx, 4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
  char *iptr = (char*)(&val);
  int i = 0;

  if ( sizeof(int) != RTWKB_INT_SIZE )
  {
    rterror(ctx, "Machine int size is not %d bytes!", RTWKB_INT_SIZE);
  }

  bytebuffer_makeroom(ctx, buf, RTWKB_INT_SIZE);
  /* Machine/request arch mismatch, so flip byte order */
  if ( swap)
  {
    RTDEBUG(ctx, 4,"Ok, let's do the swaping thing");
    for ( i = 0; i < RTWKB_INT_SIZE; i++ )
    {
      *(buf->writecursor) = iptr[RTWKB_INT_SIZE - 1 - i];
      buf->writecursor += 1;
    }
  }
  /* If machine arch and requested arch match, don't flip byte order */
  else
  {
    RTDEBUG(ctx, 4,"Ok, let's do the memcopying thing");
    memcpy(buf->writecursor, iptr, RTWKB_INT_SIZE);
    buf->writecursor += RTWKB_INT_SIZE;
  }

  RTDEBUGF(ctx, 4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
  return;

}





/**
* Writes a float64 to the buffer
*/
void
bytebuffer_append_double(const RTCTX *ctx, bytebuffer_t *buf, const double val, int swap)
{
  RTDEBUGF(ctx, 2,"Entered bytebuffer_append_double with value %lf swap = %d", val, swap);

  RTDEBUGF(ctx, 4,"buf_start = %p and write_cursor=%p", buf->buf_start,buf->writecursor);
  char *dptr = (char*)(&val);
  int i = 0;

  if ( sizeof(double) != RTWKB_DOUBLE_SIZE )
  {
    rterror(ctx, "Machine double size is not %d bytes!", RTWKB_DOUBLE_SIZE);
  }

  bytebuffer_makeroom(ctx, buf, RTWKB_DOUBLE_SIZE);

  /* Machine/request arch mismatch, so flip byte order */
  if ( swap )
  {
    RTDEBUG(ctx, 4,"Ok, let's do the swapping thing");
    for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
    {
      *(buf->writecursor) = dptr[RTWKB_DOUBLE_SIZE - 1 - i];
      buf->writecursor += 1;
    }
  }
  /* If machine arch and requested arch match, don't flip byte order */
  else
  {
    RTDEBUG(ctx, 4,"Ok, let's do the memcopying thing");
    memcpy(buf->writecursor, dptr, RTWKB_DOUBLE_SIZE);
    buf->writecursor += RTWKB_DOUBLE_SIZE;
  }

  RTDEBUG(ctx, 4,"Return from bytebuffer_append_double");
  return;

}

/**
* Reads a signed varInt from the buffer
*/
int64_t
bytebuffer_read_varint(const RTCTX *ctx, bytebuffer_t *b)
{
  size_t size;
  int64_t val = varint_s64_decode(ctx, b->readcursor, b->buf_start + b->capacity, &size);
  b->readcursor += size;
  return val;
}

/**
* Reads a unsigned varInt from the buffer
*/
uint64_t
bytebuffer_read_uvarint(const RTCTX *ctx, bytebuffer_t *b)
{
  size_t size;
  uint64_t val = varint_u64_decode(ctx, b->readcursor, b->buf_start + b->capacity, &size);
  b->readcursor += size;
  return val;
}

/**
* Returns the length of the current buffer
*/
size_t
bytebuffer_getlength(const RTCTX *ctx, bytebuffer_t *s)
{
  return (size_t) (s->writecursor - s->buf_start);
}


/**
* Returns a new bytebuffer were both ingoing bytebuffers is merged.
* Caller is responsible for freeing both incoming bytefyffers and resulting bytebuffer
*/
bytebuffer_t*
bytebuffer_merge(const RTCTX *ctx, bytebuffer_t **buff_array, int nbuffers)
{
  size_t total_size = 0, current_size, acc_size = 0;
  int i;
  for ( i = 0; i < nbuffers; i++ )
  {
    total_size += bytebuffer_getlength(ctx, buff_array[i]);
  }

  bytebuffer_t *res = bytebuffer_create_with_size(ctx, total_size);
  for ( i = 0; i < nbuffers; i++)
  {
    current_size = bytebuffer_getlength(ctx, buff_array[i]);
    memcpy(res->buf_start+acc_size, buff_array[i]->buf_start, current_size);
    acc_size += current_size;
  }
  res->writecursor = res->buf_start + total_size;
  res->readcursor = res->buf_start;
  return res;
}


