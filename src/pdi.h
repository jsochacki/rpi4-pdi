/*
  Copyright (C) 2021 Buildbotics LLC.
  Copyright (C) 2015 DiUS Computing Pty. Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define PDI_TIMEOUT     200000 // In PDI clocks
#define PDI_REG_STATUS  0
#define PDI_REG_RESET   1
#define PDI_REG_CONTROL 2


// PDI commands
enum {
  LDS    = 0x00, // lower 4 bits: address-size; data-size(byte, word, 3, long)
  STS    = 0x40, // "-"
  LD     = 0x20, // lower 4 bits: ptr mode; size(byte, word, 3, long)
  ST     = 0x60, // -"-
  LDCS   = 0x80, // lower 2 bits: register no
  STCS   = 0xc0, // -"-
  KEY    = 0xe0,
  REPEAT = 0xa0  // lower 2 bits indicate length field following
};

// pointer modes for LD/ST commands: *ptr, *ptr++, ptr, ptr++
enum {
  xPTR   = 0 << 2,
  xPTRpp = 1 << 2,
  PTR    = 2 << 2,
  PTRpp  = 3 << 2,
};

typedef enum {SZ_1, SZ_2, SZ_3, SZ_4} pdi_size_t;
typedef enum {PDI_OUT, PDI_IN} pdi_dir_t;

typedef enum {
  XF_ST = -1,
  XF_0, XF_1, XF_2, XF_3, XF_4, XF_5, XF_6, XF_7,
  XF_PAR, XF_SP0, XF_SP1
} pdi_pos_t;


void pdi_break(); ///< Send double-break
void pdi_stop();

// Be mindful of clock gaps - no printfs
bool pdi_send(const uint8_t *buf, uint32_t len);
bool pdi_recv(uint8_t *buf, uint32_t len);

bool pdi_init(uint8_t clk_pin, uint8_t data_pin);
bool pdi_open();
void pdi_close();
