/*
  Copyright (C) 2021 Buildbotics LLC.
  Copyright (C) 2015 DiUS Computing Pty. Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdbool.h>
#include <stdint.h>


#define WAIT_ATTEMPTS 20000
#define MAX_RETRY 10


typedef enum {
  NVM_NONE,
  NVM_FLASH,
  NVM_APPLICATION,
  NVM_BOOT,
  NVM_SIGNATURE,
  NVM_FUSE,
  NVM_EEPROM,
} nvm_t;


enum {
  NVM_NOP                           = 0x00,
  NVM_CHIP_ERASE                    = 0x40, // cmdex
  NVM_READ                          = 0x43, // pdi read

  NVM_LOAD_PAGE_BUF                 = 0x23, // pdi write
  NVM_ERASE_PAGE_BUF                = 0x26, // cmdex

  NVM_ERASE_FLASH_PAGE              = 0x2b, // pdi write
  NVM_WRITE_FLASH_PAGE              = 0x2e, // pdi write
  NVM_ERASE_WRITE_FLASH_PAGE        = 0x2f, // pdi write
  NVM_FLASH_CRC                     = 0x78, // cmdex
  NVM_FLASH_RANGE_CRC               = 0x3a, // cmdex

  NVM_ERASE_APP_SECTION             = 0x20, // pdi write
  NVM_ERASE_APP_SECTION_PAGE        = 0x22, // pdi write
  NVM_WRITE_APP_SECTION_PAGE        = 0x24, // pdi write
  NVM_ERASE_WRITE_APP_SECTION_PAGE  = 0x25, // pdi write
  NVM_APP_SECTION_CRC               = 0x38, // cmdex

  NVM_ERASE_BOOT_SECTION            = 0x68, // pdi write
  NVM_ERASE_BOOT_SECTION_PAGE       = 0x2a, // pdi write
  NVM_WRITE_BOOT_SECTION_PAGE       = 0x2c, // pdi write
  NVM_ERASE_WRITE_BOOT_SECTION_PAGE = 0x2d, // pdi write
  NVM_BOOT_SECTION_CRC              = 0x39, // cmdex

  NVM_READ_USERSIG_ROW              = 0x03, // pdi read
  NVM_ERASE_USERSIG_ROW             = 0x18, // pdi write
  NVM_WRITE_USERSIG_ROW             = 0x1a, // pdi write
  NVM_READ_CALIBRATION_ROW          = 0x02, // pdi read

  NVM_READ_FUSE                     = 0x07, // pdi read
  NVM_WRITE_FUSE                    = 0x4c, // pdi write
  NVM_WRITE_LOCK_BITS               = 0x08, // cmdex

  NVM_LOAD_EEPROM_PAGE_BUF          = 0x33, // pdi write
  NVM_ERASE_EEPROM_PAGE_BUF         = 0x36, // cmdex

  NVM_ERASE_EEPROM                  = 0x30, // cmdex
  NVM_ERASE_EEPROM_PAGE             = 0x32, // pdi write
  NVM_WRITE_EEPROM_PAGE             = 0x34, // pdi write
  NVM_ERASE_WRITE_EEPROM_PAGE       = 0x35, // pdi write
  NVM_READ_EEPROM                   = 0x06  // pdi read
};

#define NVM_REG_BASE          0x010001c0
#define NVM_REG_ADDR_OFFS     0x00
#define NVM_REG_DATA_OFFS     0x04
#define NVM_REG_CMD_OFFS      0x0a
#define NVM_REG_CTRLA_OFFS    0x0b
#define NVM_REG_STATUS_OFFS   0x0f
#define NVM_REG_LOCKBITS_OFFS 0x10

#define NVM_CTRLA_CMDEX_bm (1 << 0)
#define NVM_STATUS_BUSY_bm (1 << 7)

#define PDI_NVMEN_bm          0x02


bool nvm_read(uint32_t addr, uint8_t *buf, uint32_t len);
int32_t nvm_read_device_id();
bool nvm_write_page(nvm_t type, uint32_t addr, const uint8_t *buf,
                    uint16_t len);
bool nvm_erase_page(nvm_t type, uint32_t addr);
bool nvm_chip_erase();
bool nvm_write_fuse(uint8_t num, uint8_t value);
int32_t nvm_flash_crc();
