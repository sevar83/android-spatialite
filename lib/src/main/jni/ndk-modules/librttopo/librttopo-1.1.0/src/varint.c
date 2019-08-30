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
 * Copyright (C) 2014 Sandro Santilli <strk@kbt.io>
 * Copyright (C) 2013 Nicklas Avén
 *
 **********************************************************************/



#include "rttopo_config.h"
#include "varint.h"
#include "rtgeom_log.h"
#include "librttopo_geom.h"

/* -------------------------------------------------------------------------------- */

static size_t
_varint_u64_encode_buf(const RTCTX *ctx, uint64_t val, uint8_t *buf)
{
  uint8_t grp;
  uint64_t q = val;
  uint8_t *ptr = buf;
  while (1)
  {
    /* We put the 7 least significant bits in grp */
    grp = 0x7f & q;
    /* We rightshift our input value 7 bits */
    /* which means that the 7 next least significant bits */
    /* becomes the 7 least significant */
    q = q >> 7;
    /* Check if, after our rightshifting, we still have */
    /* anything to read in our input value. */
    if ( q > 0 )
    {
      /* In the next line quite a lot is happening. */
      /* Since there is more to read in our input value */
      /* we signal that by setting the most siginicant bit */
      /* in our byte to 1. */
      /* Then we put that byte in our buffer and move the pointer */
      /* forward one step */
      *ptr = 0x80 | grp;
      ptr++;
    }
    else
    {
      /* The same as above, but since there is nothing more */
      /* to read in our input value we leave the most significant bit unset */
      *ptr = grp;
      ptr++;
      return ptr - buf;
    }
  }
  /* This cannot happen */
  rterror(ctx, "%s: Got out of infinite loop. Consciousness achieved.", __func__);
  return (size_t)0;
}


size_t
varint_u64_encode_buf(const RTCTX *ctx, uint64_t val, uint8_t *buf)
{
  return _varint_u64_encode_buf(ctx, val, buf);
}


size_t
varint_u32_encode_buf(const RTCTX *ctx, uint32_t val, uint8_t *buf)
{
  return _varint_u64_encode_buf(ctx, (uint64_t)val, buf);
}

size_t
varint_s64_encode_buf(const RTCTX *ctx, int64_t val, uint8_t *buf)
{
  return _varint_u64_encode_buf(ctx, zigzag64(ctx, val), buf);
}

size_t
varint_s32_encode_buf(const RTCTX *ctx, int32_t val, uint8_t *buf)
{
  return _varint_u64_encode_buf(ctx, (uint64_t)zigzag32(ctx, val), buf);
}

/* Read from signed 64bit varint */
int64_t
varint_s64_decode(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end, size_t *size)
{
  return unzigzag64(ctx, varint_u64_decode(ctx, the_start, the_end, size));
}

/* Read from unsigned 64bit varint */
uint64_t
varint_u64_decode(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end, size_t *size)
{
  uint64_t nVal = 0;
  int nShift = 0;
  uint8_t nByte;
  const uint8_t *ptr = the_start;

  /* Check so we don't read beyond the twkb */
  while( ptr < the_end )
  {
    nByte = *ptr;
    /* Hibit is set, so this isn't the last byte */
    if (nByte & 0x80)
    {
      /* We get here when there is more to read in the input varInt */
      /* Here we take the least significant 7 bits of the read */
      /* byte and put it in the most significant place in the result variable. */
      nVal |= ((uint64_t)(nByte & 0x7f)) << nShift;
      /* move the "cursor" of the input buffer step (8 bits) */
      ptr++;
      /* move the cursor in the resulting variable (7 bits) */
      nShift += 7;
    }
    else
    {
      /* move the "cursor" one step */
      ptr++;
      /* Move the last read byte to the most significant */
      /* place in the result and return the whole result */
      *size = ptr - the_start;
      return nVal | ((uint64_t)nByte << nShift);
    }
  }
  rterror(ctx, "%s: varint extends past end of buffer", __func__);
  return 0;
}

size_t
varint_size(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end)
{
  const uint8_t *ptr = the_start;

  /* Check so we don't read beyond the twkb */
  while( ptr < the_end )
  {
    /* Hibit is set, this isn't the last byte */
    if (*ptr & 0x80)
    {
      ptr++;
    }
    else
    {
      ptr++;
      return ptr - the_start;
    }
  }
  return 0;
}

uint64_t zigzag64(const RTCTX *ctx, int64_t val)
{
  return (val << 1) ^ (val >> 63);
}

uint32_t zigzag32(const RTCTX *ctx, int32_t val)
{
  return (val << 1) ^ (val >> 31);
}

uint8_t zigzag8(const RTCTX *ctx, int8_t val)
{
  return (val << 1) ^ (val >> 7);
}

int64_t unzigzag64(const RTCTX *ctx, uint64_t val)
{
        if ( val & 0x01 )
            return -1 * (int64_t)((val+1) >> 1);
        else
            return (int64_t)(val >> 1);
}

int32_t unzigzag32(const RTCTX *ctx, uint32_t val)
{
        if ( val & 0x01 )
            return -1 * (int32_t)((val+1) >> 1);
        else
            return (int32_t)(val >> 1);
}

int8_t unzigzag8(const RTCTX *ctx, uint8_t val)
{
        if ( val & 0x01 )
            return -1 * (int8_t)((val+1) >> 1);
        else
            return (int8_t)(val >> 1);
}


