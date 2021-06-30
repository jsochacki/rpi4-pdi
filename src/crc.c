/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "crc.h"


uint32_t crc24(uint16_t word, uint32_t crc) {
  return (((crc << 1) ^ word) ^ ((0x800000 & crc) ? 0x80001b : 0)) & 0xffffff;
}


uint32_t crc24_block(const uint8_t *data, unsigned len, uint32_t crc) {
  for (unsigned i = 0; i < len; i += 2)
    crc = crc24(data[i + 1] << 8 | data[i], crc);

  return crc;
}
