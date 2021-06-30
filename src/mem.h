/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include "devices.h"
#include "nvm.h"

#include <stdint.h>


typedef struct {
  const char *name;
  nvm_t type;
  uint32_t address;
  const char *desc;
} memory_t;


void mem_print();
const memory_t *mem_get(const char *name);
uint32_t mem_get_size(const memory_t *mem, const device_t *device);
uint32_t mem_get_addr(const memory_t *mem, const device_t *device);
uint16_t mem_get_page_size(const memory_t *mem, const device_t *device);
