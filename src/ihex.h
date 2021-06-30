/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>


#define IHEX_LINE_LENGTH 16
#define IHEX_MIN_STRING  11

#define IHEX_OFFS_LEN     1
#define IHEX_OFFS_ADDR    3
#define IHEX_OFFS_TYPE    7
#define IHEX_OFFS_DATA    9

enum {
  IHEX_DATA_RECORD,
  IHEX_END_OF_FILE_RECORD,
  IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD,
  IHEX_START_SEGMENT_ADDRESS_RECORD,
  IHEX_EXTENDED_LINEAR_ADDRESS_RECORD,
  IHEX_START_LINEAR_ADDRESS_RECORD
};

enum {
  IHEX_ERROR_NONE,
  IHEX_ERROR_FILE,
  IHEX_ERROR_SIZE,
  IHEX_ERROR_FMT,
  IHEX_ERROR_CRC
};


void ihex_write(FILE *f, uint8_t *data, uint32_t len);
uint8_t ihex_read(FILE *f, uint8_t *data, uint32_t maxlen, uint32_t *max_addr);
const char *ihex_error_str(uint8_t err);
