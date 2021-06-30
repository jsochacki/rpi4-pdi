/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "mem.h"
#include "error.h"
#include "nvm.h"

#include <stdio.h>
#include <strings.h>


static memory_t memories[] = {
  {"flash",  NVM_FLASH,        FLASH_BASE_ADDR, "App & boot sections"},
  {"app",    NVM_APPLICATION,  FLASH_BASE_ADDR, "App section of FLASH"},
  {"boot",   NVM_BOOT,                       0, "Boot section of FLASH"},
  {"eeprom", NVM_EEPROM,      EEPROM_BASE_ADDR, "EEPROM base address"},
  {"prod",   NVM_NONE,      PROD_SIG_BASE_ADDR, "Production signature row"},
  {"user",   NVM_SIGNATURE, USER_SIG_BASE_ADDR, "User signature row"},
  {"fuse",   NVM_FUSE,          FUSE_BASE_ADDR, "Fuse base address"},
  {"lock",   NVM_FUSE,          LOCK_BASE_ADDR, "Lock bits base address"},
  {"io",     NVM_NONE,            IO_BASE_ADDR, "Mapped I/O base address"},
  {0}
};


void mem_print() {
  for (int i = 0; memories[i].name; i++)
    printf("  %-7s  %s\n", memories[i].name, memories[i].desc);
}


const memory_t *mem_get(const char *name) {
  for (int i = 0; memories[i].name; i++)
    if (!strcasecmp(memories[i].name, name))
      return &memories[i];

  fail("Unsupported memory name %s", name);

  return 0;
}


uint32_t mem_get_size(const memory_t *mem, const device_t *device) {
  if (!device) fail("Cannot determine memory size with out device");

  if (!strcasecmp(mem->name, "flash"))
    return device->app_size + device->boot_size;

  if (!strcasecmp(mem->name, "app"))    return device->app_size;
  if (!strcasecmp(mem->name, "boot"))   return device->boot_size;
  if (!strcasecmp(mem->name, "eeprom")) return device->eeprom_size;
  if (!strcasecmp(mem->name, "prod"))   return device->prod_size;
  if (!strcasecmp(mem->name, "user"))   return device->user_size;
  if (!strcasecmp(mem->name, "fuse"))   return device->fuse_size;
  if (!strcasecmp(mem->name, "lock"))   return device->lock_size;
  if (!strcasecmp(mem->name, "io"))     return IO_SIZE;

  return 0;
}


uint32_t mem_get_addr(const memory_t *mem, const device_t *device) {
  if (!strcasecmp(mem->name, "boot")) {
    if (!device) fail("Cannot determine base address with out device");
    return FLASH_BASE_ADDR + device->app_size;
  }

  return mem->address;
}


uint16_t mem_get_page_size(const memory_t *mem, const device_t *device) {
  if (!strcasecmp(mem->name, "eeprom"))
    return device ? device->eeprom_page : 32;

  if (!strcasecmp(mem->name, "flash") ||
      !strcasecmp(mem->name, "app")   ||
      !strcasecmp(mem->name, "boot")  ||
      !strcasecmp(mem->name, "user"))
    return device ? device->page_size : 512;

  return 0;
}
