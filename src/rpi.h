/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#pragma once

#include <stdint.h>
#include <stdbool.h>

void rpi_gpio_dir(uint8_t pin, bool in);
void rpi_gpio_set(uint8_t pin);
void rpi_gpio_clr(uint8_t pin);
bool rpi_gpio_get(uint8_t pin);
void rpi_delay(uint64_t us);
bool rpi_init();
