/*
  Copyright (C) 2021 Buildbotics LLC.

  Based on code from http://www.airspayce.com/mikem/bcm2835/

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "rpi.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>


#define BCM2835_ST_BASE    0x3000
#define BCM2835_GPIO_BASE  0x200000

#define BCM2835_GPFSEL0    0x00
#define BCM2835_GPSET0     0x1c
#define BCM2835_GPCLR0     0x28
#define BCM2835_GPLEV0     0x34

#define BCM2835_GPIO_FSEL_INPT 0
#define BCM2835_GPIO_FSEL_OUTP 1
#define BCM2835_GPIO_FSEL_MASK 7

#define BCM2835_ST_CLO 4
#define BCM2835_ST_CHI 8


volatile uint32_t *_gpio = 0;
volatile uint32_t *_st   = 0;


static void _gpio_fsel(uint8_t pin, uint8_t mode) {
  // Function selects are 10 pins per 32 bit word, 3 bits per pin
  volatile uint32_t *paddr = _gpio + BCM2835_GPFSEL0 / 4 + (pin / 10);
  uint8_t  shift = (pin % 10) * 3;
  uint32_t mask  = BCM2835_GPIO_FSEL_MASK << shift;
  uint32_t value = mode << shift;

  *paddr = (*paddr & ~mask) | (value & mask);
}


void rpi_gpio_dir(uint8_t pin, bool in) {
  _gpio_fsel(pin, in ? BCM2835_GPIO_FSEL_INPT : BCM2835_GPIO_FSEL_OUTP);
}


void rpi_gpio_set(uint8_t pin) {
  volatile uint32_t *paddr = _gpio + BCM2835_GPSET0 / 4 + pin / 32;
  *paddr = 1 << (pin % 32);
}


void rpi_gpio_clr(uint8_t pin) {
  volatile uint32_t *paddr = _gpio + BCM2835_GPCLR0 / 4 + pin / 32;
  *paddr = 1 << (pin % 32);
}


bool rpi_gpio_get(uint8_t pin) {
  volatile uint32_t *paddr = _gpio + BCM2835_GPLEV0 / 4 + pin / 32;
  return *paddr & (1 << (pin % 32));
}


// Read the System Timer Counter (64-bits)
static uint64_t _st_read() {
  uint32_t hi = *(volatile uint32_t *)(_st + BCM2835_ST_CHI / 4);
  uint32_t lo = *(volatile uint32_t *)(_st + BCM2835_ST_CLO / 4);
  uint64_t st = *(volatile uint32_t *)(_st + BCM2835_ST_CHI / 4);

  // Test for overflow
  if (st == hi) {
    st <<= 32;
    st += lo;

  } else {
    st <<= 32;
    st += *(volatile uint32_t *)(_st + BCM2835_ST_CLO / 4);
  }

  return st;
}


void rpi_delay(uint64_t us) {
  uint64_t start = _st_read();
  while (_st_read() < start + us) continue;
}


static bool error(const char *msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  return false;
}


bool rpi_init() {
  if (_gpio) return true;

  // Try to read device tree
  FILE *fp = fopen("/proc/device-tree/soc/ranges", "rb");
  if (!fp) return error("Unable to open device tree");

  unsigned char buf[16];
  int ret = fread(buf, 1, sizeof(buf), fp);
  fclose(fp);

  off_t base;
  size_t size;
  if (8 <= ret) {
    base = buf[4] << 24 | buf[5] << 16 | buf[6]  << 8 | buf[7];
    size = buf[8] << 24 | buf[9] << 16 | buf[10] << 8 | buf[11];

    if (!base) { // RPI 4
      base = buf[8]  << 24 | buf[9]  << 16 | buf[10] << 8 | buf[11];
      size = buf[12] << 24 | buf[13] << 16 | buf[14] << 8 | buf[15];
    }

  } else return error("Unable to read device tree");

  // Open /dev/mem
  int fd = -1;
  if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    return error("Unable to open /dev/mem");

  // Map memory
  uint32_t *mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
  if (mem == MAP_FAILED) return error("Failed to map memory");

  // Save base addresses.  Divided by 4 for (uint32_t *) access.
  _gpio = mem + BCM2835_GPIO_BASE / 4;
  _st   = mem + BCM2835_ST_BASE   / 4;

  return true;
}
