/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdint.h>


#define FLASH_BASE_ADDR    0x0800000
#define EEPROM_BASE_ADDR   0x08c0000
#define PROD_SIG_BASE_ADDR 0x08e0200
#define USER_SIG_BASE_ADDR 0x08e0400
#define FUSE_BASE_ADDR     0x08f0020
#define LOCK_BASE_ADDR     0x08f0027
#define IO_BASE_ADDR       0x1000000
#define SRAM_BASE_ADDR     0x1002000
#define DEVICE_ID_ADDR     0x1000090

#define IO_SIZE            0x1000


typedef struct {
  const char *name;
  uint32_t sig;
  uint16_t page_size;
  uint32_t sram_size;
  uint32_t eeprom_size;
  uint32_t eeprom_page;
  uint32_t app_size;
  uint32_t boot_size;
  uint32_t fuse_size;
  uint32_t lock_size;
  uint32_t user_size;
  uint32_t prod_size;
} device_t;


extern device_t devices[];

const device_t *devices_find(char *name);
const device_t *devices_find_by_sig(uint32_t sig);
void devices_print(const device_t *device);
