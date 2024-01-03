/*
  Copyright (c) 2020 Arduino.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**************************************************************************************
 * INCLUDE
 **************************************************************************************/

#include "Arduino_CRC32.h"

#include "crc.h"

/**************************************************************************************
 * PUBLIC MEMBER FUNCTIONS
 **************************************************************************************/

uint32_t Arduino_CRC32::calc(uint8_t const data[], uint32_t const len)
{
  crc_t crc32 = crc_init();
  crc32 = crc_update(crc32, data, len);
  crc32 = crc_finalize(crc32);
  return crc32;
}
