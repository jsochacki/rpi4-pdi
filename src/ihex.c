/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "ihex.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>


void ihex_write(FILE *f, uint8_t *data, uint32_t len) {
  bool write_addr = false;

  for (uint32_t i = 0; i < len; i += IHEX_LINE_LENGTH) {
    uint8_t bytes = IHEX_LINE_LENGTH < (len - i) ? IHEX_LINE_LENGTH : (len - i);

    if (i && !(i & 0xffff)) write_addr = true;

    // Detect empty
    bool empty = true;
    for (unsigned j = 0; empty && j < bytes; j++)
      empty = data[j] == 0xff;

    if (empty) {
      write_addr = true;
      data += bytes;
      continue;
    }

    // Write extended segment address
    if (write_addr) {
      uint16_t addr = i >> 4;
      uint8_t crc = 0x100U - (4 + (addr >> 8) + (addr & 0xff));
      fprintf(f, ":02000002%04x%02x\n", addr, crc);

      write_addr = false;
    }

    // Start
    uint8_t crc = bytes + ((i >> 8) & 0xff) + (i & 0xff);
    fprintf(f, ":%02x%04x00", bytes, (uint16_t)i);

    // Data
    for (unsigned j = 0; j < bytes; j++) {
      crc += *data;
      fprintf(f, "%02x", *data++);
    }

    // End
    crc = 0x100U - crc;
    fprintf(f, "%02x\n", crc);
  }

  // Write end
  fprintf(f, ":00000001FF\n");
}


static uint8_t _get_nibble(char c) {
  if ('0' <= c && c <= '9') return (uint8_t)(c - '0');
  if ('A' <= c && c <= 'F') return (uint8_t)(c - 'A' + 10);
  if ('a' <= c && c <= 'f') return (uint8_t)(c - 'a' + 10);
  return 0;
}


static uint8_t _get_byte(char *data) {
  return (_get_nibble(data[0]) << 4) + _get_nibble(data[1]);
}


static uint16_t _get_word(char *data) {
  return ((uint16_t)_get_byte(data) << 8) + _get_byte(data + 2);
}


uint8_t ihex_read(FILE *f, uint8_t *data, uint32_t maxlen, uint32_t *max_addr) {
  uint32_t segment = 0;

  while (!feof(f)) {
    char str[128];
    if (!fgets(str, sizeof(str), f)) return IHEX_ERROR_FILE;

    // Trim whitespace
    char *end = str + strlen(str) - 1;
    while (str < end && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    if (strlen(str) < IHEX_MIN_STRING) return IHEX_ERROR_FMT;

    uint8_t   len = _get_byte(&str[IHEX_OFFS_LEN]);
    uint32_t addr = _get_word(&str[IHEX_OFFS_ADDR]);
    if (maxlen <= addr + segment) return IHEX_ERROR_SIZE;

    uint8_t type = _get_byte(&str[IHEX_OFFS_TYPE]);
    if (len * 2 + IHEX_MIN_STRING != strlen(str)) return IHEX_ERROR_FMT;

    switch (type) {
      case IHEX_DATA_RECORD:
        for (uint8_t i = 0; i < len; i++) {
          uint8_t byte = _get_byte(&str[IHEX_OFFS_DATA + i * 2]);
          if (byte != 0xff) *max_addr = addr + segment + i + 1;
          data[addr + segment + i] = byte;
        }
        break;

      case IHEX_END_OF_FILE_RECORD: return IHEX_ERROR_NONE;

      case IHEX_EXTENDED_SEGMENT_ADDRESS_RECORD:
        segment = (uint32_t)_get_word(&str[IHEX_OFFS_DATA]) << 4;
        break;

      case IHEX_START_SEGMENT_ADDRESS_RECORD:   break;
      case IHEX_START_LINEAR_ADDRESS_RECORD:    break;
      default: return IHEX_ERROR_FMT;
    }
  }

  return IHEX_ERROR_NONE;
}


const char *ihex_error_str(uint8_t err) {
  switch (err) {
  case IHEX_ERROR_NONE: return "None";
  case IHEX_ERROR_FILE: return "I/O error";
  case IHEX_ERROR_SIZE: return "Size too large";
  case IHEX_ERROR_FMT:  return "Invalid format";
  case IHEX_ERROR_CRC:  return "CRC check failed";
  }

  return "Unknown";
}
