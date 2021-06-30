/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdint.h>


uint32_t crc24(uint16_t word, uint32_t crc);
uint32_t crc24_block(const uint8_t *data, unsigned len, uint32_t crc);
