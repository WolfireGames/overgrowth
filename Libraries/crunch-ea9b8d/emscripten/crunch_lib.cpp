/* Copyright (c) 2013, Evan Parker, Brandon Jones. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#define PLATFORM_NACL // This disables use of 64 bit integers, among other things.

#include <stddef.h> // For NULL, size_t
#include <cstring> // for malloc etc

#include "crn_decomp.h"

extern "C" {
  unsigned int crn_get_width(void *src, unsigned int src_size);
  unsigned int crn_get_height(void *src, unsigned int src_size);
  unsigned int crn_get_levels(void *src, unsigned int src_size);
  unsigned int crn_get_dxt_format(void *src, unsigned int src_size);
  unsigned int crn_get_bytes_per_block(void *src, unsigned int src_size);
  unsigned int crn_get_uncompressed_size(void *p, unsigned int size, unsigned int level);
  void crn_decompress(void *src, unsigned int src_size, void *dst, unsigned int dst_size, unsigned int firstLevel, unsigned int levelCount);
}

unsigned int crn_get_width(void *src, unsigned int src_size) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  return tex_info.m_width;
}

unsigned int crn_get_height(void *src, unsigned int src_size) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  return tex_info.m_height;
}

unsigned int crn_get_levels(void *src, unsigned int src_size) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  return tex_info.m_levels;
}

unsigned int crn_get_dxt_format(void *src, unsigned int src_size) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  return tex_info.m_format;
}

unsigned int crn_get_bytes_per_block(void *src, unsigned int src_size) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  return crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);
}

unsigned int crn_get_uncompressed_size(void *src, unsigned int src_size, unsigned int level) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);
  const crn_uint32 width = tex_info.m_width >> level;
  const crn_uint32 height = tex_info.m_height >> level;
  const crn_uint32 blocks_x = (width + 3) >> 2;
  const crn_uint32 blocks_y = (height + 3) >> 2;
  const crn_uint32 row_pitch = blocks_x * crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);
  const crn_uint32 total_face_size = row_pitch * blocks_y;
  return total_face_size;
}

void crn_decompress(void *src, unsigned int src_size, void *dst, unsigned int dst_size, unsigned int firstLevel, unsigned int levelCount) {
  crnd::crn_texture_info tex_info;
  crnd::crnd_get_texture_info(static_cast<crn_uint8*>(src), src_size, &tex_info);

  crn_uint32 width = tex_info.m_width >> firstLevel;
  crn_uint32 height = tex_info.m_height >> firstLevel;
  crn_uint32 bytes_per_block = crnd::crnd_get_bytes_per_dxt_block(tex_info.m_format);

  void *pDecomp_images[1];
  pDecomp_images[0] = dst;

  crnd::crnd_unpack_context pContext =
      crnd::crnd_unpack_begin(static_cast<crn_uint8*>(src), src_size);

  for (int i = firstLevel; i < firstLevel + levelCount; ++i) {
    crn_uint32 blocks_x = (width + 3) >> 2;
    crn_uint32 blocks_y = (height + 3) >> 2;
    crn_uint32 row_pitch = blocks_x * bytes_per_block;
    crn_uint32 total_level_size = row_pitch * blocks_y;

    crnd::crnd_unpack_level(pContext, pDecomp_images, total_level_size, row_pitch, i);
    pDecomp_images[0] = (char*)pDecomp_images[0] + total_level_size;

    width = width >> 1;
    height = height >> 1;
  }

  crnd::crnd_unpack_end(pContext);
}