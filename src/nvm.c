/*
  Copyright (C) 2021 Buildbotics LLC.
  Copyright (C) 2015 DiUS Computing Pty. Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "nvm.h"
#include "pdi.h"
#include "devices.h"


#define _RETRY_LOOP(OP) do {                    \
    for (int i = 0; i < MAX_RETRY; i++) {       \
      if (OP) return true;                      \
      pdi_open();                               \
    }                                           \
    return false;                               \
  } while (0)


static bool _store_byte(uint32_t addr, uint8_t value) {
  uint8_t cmds[] =
    {STS | SZ_4 << 2 | SZ_1, addr, addr >> 8, addr >> 16, addr >> 24, value};
  return pdi_send(cmds, sizeof(cmds));
}


static bool _load_u24(uint32_t addr, uint8_t *value) {
  uint8_t cmds[] =
    {LDS | SZ_4 << 2 | SZ_3, addr, addr >> 8, addr >> 16, addr >> 24};

  return pdi_send(cmds, sizeof(cmds)) && pdi_recv(value, 3);
}


static bool _ldcs(uint8_t reg, uint8_t *value) {
  uint8_t cmd = LDCS | reg;
  return pdi_send(&cmd, 1) && pdi_recv(value, 1);
}


static bool _store_address(uint32_t addr) {
  uint8_t cmds[] = {ST | PTR | SZ_4, addr, addr >> 8, addr >> 16, addr >> 24};
  return pdi_send(cmds, sizeof(cmds));
}


static bool _store_repeat(uint32_t count) {
  uint8_t cmds[] = {REPEAT | SZ_4, count, count >> 8, count >> 16, count >> 24};
  return pdi_send(cmds, sizeof(cmds));
}


static bool nvm_execute() {
  return _store_byte(NVM_REG_BASE + NVM_REG_CTRLA_OFFS, NVM_CTRLA_CMDEX_bm);
}


static bool nvm_command(uint8_t cmd) {
  return _store_byte(NVM_REG_BASE + NVM_REG_CMD_OFFS, cmd);
}


static bool _wait_busy() {
  if (!_store_address(NVM_REG_BASE + NVM_REG_STATUS_OFFS)) return false;

  uint8_t cmd    = LD | xPTR | SZ_1;
  uint8_t status = 0;

  for (int i = 0; i < WAIT_ATTEMPTS; i++) {
    if (!pdi_send(&cmd, 1) || !pdi_recv(&status, 1)) break;
    if (!(status & NVM_STATUS_BUSY_bm)) return true;
  }

  return false;
}


static bool _is_enabled() {
  uint8_t status = 0;
  if (!_ldcs(PDI_REG_STATUS, &status)) return false;
  return status & PDI_NVMEN_bm;
}


static bool _wait_enabled() {
  for (int i = 0; i < WAIT_ATTEMPTS; i++)
    if (_is_enabled()) return true;

  return false;
}


static bool _exec(uint8_t cmd) {
  return
    _wait_enabled()  &&
    _wait_busy()     &&
    nvm_command(cmd) &&
    nvm_execute()    &&
    _wait_enabled()  &&
    _wait_busy();
}


static bool _read(uint32_t addr, uint8_t *buf, uint32_t len) {
  uint8_t cmd = LD | xPTRpp | SZ_1;

  return
    _wait_enabled()        &&
    _wait_busy()           &&
    nvm_command(NVM_READ)  &&
    _store_address(addr)   &&
    _store_repeat(len - 1) &&
    pdi_send(&cmd, 1)      &&
    pdi_recv(buf, len);
}


bool nvm_read(uint32_t addr, uint8_t *buf, uint32_t len) {
  _RETRY_LOOP(_read(addr, buf, len));
}


int32_t nvm_read_device_id() {
  pdi_open();

  uint8_t buf[3] = {0};

  if (nvm_read(DEVICE_ID_ADDR, buf, sizeof(buf)))
    return buf[0] << 16 | buf[1] << 8 | buf[2];

  return -1;
}


static bool _write_page(uint8_t erase_page_buf_cmd, uint8_t load_page_buf_cmd,
                        uint8_t write_erase_cmd, uint32_t addr,
                        const uint8_t *buf, uint16_t len) {
  uint8_t write[] = {ST | xPTRpp | SZ_1};
  uint8_t dummy[] = {ST | xPTRpp | SZ_1, 0}; // trigger erase+program

  return
    _exec(erase_page_buf_cmd)      &&
    nvm_command(load_page_buf_cmd) &&
    _store_address(addr)           &&
    _store_repeat(len - 1)         &&
    pdi_send(write, sizeof(write)) &&
    pdi_send(buf, len)             &&
    nvm_command(write_erase_cmd)   &&
    _store_address(addr)           &&
    pdi_send(dummy, sizeof(dummy)) &&
    _wait_busy();
}


static bool _write_eeprom_page(uint32_t addr, const uint8_t *buf,
                               uint16_t len) {
  return _write_page(NVM_ERASE_EEPROM_PAGE_BUF, NVM_LOAD_EEPROM_PAGE_BUF,
                     NVM_ERASE_WRITE_EEPROM_PAGE, addr, buf, len);
}

static bool _write_flash_page(uint8_t write_erase_cmd, uint32_t addr,
                              const uint8_t *buf, uint16_t len) {
  return _write_page(NVM_ERASE_PAGE_BUF, NVM_LOAD_PAGE_BUF, write_erase_cmd,
                     addr, buf, len);
}


bool nvm_write_page(nvm_t type, uint32_t addr, const uint8_t *buf,
                    uint16_t len) {
  uint8_t cmd = 0;

  switch (type) {
  case NVM_FLASH:       cmd = NVM_ERASE_WRITE_FLASH_PAGE;        break;
  case NVM_APPLICATION: cmd = NVM_ERASE_WRITE_APP_SECTION_PAGE;  break;
  case NVM_BOOT:        cmd = NVM_ERASE_WRITE_BOOT_SECTION_PAGE; break;
  case NVM_EEPROM:      _RETRY_LOOP(_write_eeprom_page(addr, buf, len));
  case NVM_SIGNATURE:
    if (!nvm_erase_page(type, addr)) return false;
    cmd = NVM_WRITE_USERSIG_ROW;
    break;
  case NVM_FUSE:
  case NVM_NONE: break;
  }

  if (!cmd) return false;

  _RETRY_LOOP(_write_flash_page(cmd, addr, buf, len));
}


static bool _erase_page(uint8_t cmd, uint32_t addr) {
  uint8_t dummy[] = {ST | xPTRpp | SZ_1, 0}; // trigger erase+program

  return
    _wait_enabled()                &&
    _wait_busy()                   &&
    nvm_command(cmd)               &&
    _store_address(addr)           &&
    pdi_send(dummy, sizeof(dummy)) &&
    _wait_busy();
}


bool nvm_erase_page(nvm_t type, uint32_t addr) {
  uint8_t cmd = 0;

  switch (type) {
  case NVM_FLASH:       cmd = NVM_ERASE_FLASH_PAGE;        break;
  case NVM_APPLICATION: cmd = NVM_ERASE_APP_SECTION_PAGE;  break;
  case NVM_BOOT:        cmd = NVM_ERASE_BOOT_SECTION_PAGE; break;
  case NVM_EEPROM:      cmd = NVM_ERASE_EEPROM_PAGE;       break;
  case NVM_SIGNATURE:   cmd = NVM_ERASE_USERSIG_ROW;       break;
  case NVM_FUSE:
  case NVM_NONE: break;
  }

  if (!cmd) return false;

  _RETRY_LOOP(_erase_page(cmd, addr));
}


bool nvm_chip_erase() {_RETRY_LOOP(_exec(NVM_CHIP_ERASE));}


static bool _write_fuse(uint8_t num, uint8_t value) {
  return
    _wait_enabled()                          &&
    _wait_busy()                             &&
    nvm_command(NVM_WRITE_FUSE)              &&
    _store_byte(FUSE_BASE_ADDR + num, value) &&
    _wait_busy();
}


bool nvm_write_fuse(uint8_t num, uint8_t value) {
  _RETRY_LOOP(_write_fuse(num, value));
}


static bool _crc(uint8_t *crc) {
  uint8_t cmd = NVM_FLASH_CRC;
  uint32_t addr = NVM_REG_BASE + NVM_REG_DATA_OFFS;

  return
    _wait_enabled()      &&
    _wait_busy()         &&
    nvm_command(cmd)     &&
    nvm_execute()        &&
    _wait_enabled()      &&
    _wait_busy()         &&
    _load_u24(addr, crc);
}


static bool _crc_loop(uint8_t *crc) {_RETRY_LOOP(_crc(crc));}


int32_t nvm_flash_crc() {
  // Note, only NVM_FLASH_CRC seems to work. NVM_FLASH_RANGE_CRC,
  // NVM_APP_SECTION_CRC and NVM_BOOT_SECTION_CRC return inconsistent values
  // at least on the xmega192a3u.

  uint8_t crc[3] = {0};
  if (!_crc_loop(crc)) return -1;
  return crc[2] << 16 | crc[1] << 8 | crc[0];
}
