/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "devices.h"

#include <strings.h>
#include <stdio.h>


device_t devices[] = {
#include "devices.dat"
  {0} // Sentinel
};


const device_t *devices_find(char *name) {
  for (int i = 0; devices[i].name; i++)
    if (strcasecmp(name, devices[i].name) == 0)
      return &devices[i];

  return 0;
}


const device_t *devices_find_by_sig(uint32_t sig) {
  for (int i = 0; devices[i].name; i++)
    if (devices[i].sig == sig)
      return &devices[i];

  return 0;
}


void devices_print(const device_t *device) {
  printf(
    "Device:      %12s\n"
    "Chip ID:         0x%06x\n"
    "Page size:      %9u\n"
    "Application:    %8uK\n"
    "Boot:           %8uK\n"
    "SRAM:           %8uK\n"
    "EEPROM:         %8uK\n"
    "EEPROM page:    %9u\n"
    "Fuses:          %9u\n"
    "Lock bytes:     %9u\n"
    "User row:       %9u\n"
    "Production row: %9u\n",
    device->name, device->sig, device->page_size, device->app_size >> 10,
    device->boot_size >> 10, device->sram_size >> 10, device->eeprom_size >> 10,
    device->eeprom_page, device->fuse_size, device->lock_size,
    device->user_size, device->prod_size);
}
