/*
 * 1394-Based Digital Camera Control Library
 * Checksum algorithms
 * Copyright (C) 2006 Mikael Olenfalk, Tobii Technology AB, Stockholm Sweden
 *
 * Written by Mikael Olenfalk <mikael _DOT_ olenfalk _AT_ tobii _DOT_ com>
 * Version : 16/02/2005 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "checksum.h"

uint16_t dc1394_checksum_crc16 (const uint8_t* buffer, uint32_t buffer_size)
{
  uint32_t i, j, c, bit;
  uint32_t crc = 0;
  for (i = 0; i < buffer_size; i++) {
    c = (uint32_t)*buffer++;
    for (j = 0x80; j; j >>= 1) {
      bit = crc & 0x8000;
      crc <<= 1;
      if (c & j) bit ^= 0x8000;
      if (bit) crc ^= 0x1021;
    }
  }
  return (uint16_t)(crc & 0xffff);
}
