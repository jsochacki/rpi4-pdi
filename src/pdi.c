/*
  Copyright (C) 2021 Buildbotics LLC.
  Copyright (C) 2015 DiUS Computing Pty. Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#define _GNU_SOURCE

#include "pdi.h"
#include "rpi.h"

#include <sched.h>
#include <sys/mman.h>
#include <string.h>


static struct {
  uint8_t clk;
  uint8_t data;

  volatile bool stop;
  bool failed;
  bool done;
  uint32_t length;
  uint8_t *buf;
  uint32_t offs;
  uint8_t byte;
  pdi_pos_t pos;
  uint64_t ticks;
  pdi_dir_t dir;
} pdi;



static void _next_byte() {
  pdi.ticks = 0; // reset timeout

  // If in input mode, store last received byte
  if (pdi.dir == PDI_IN) pdi.buf[pdi.offs] = pdi.byte;

  if (pdi.length <= ++pdi.offs) {
    pdi.done = true;
    pdi.offs = 0;
  }

  // reinit (also used if pdi.dir == PDI_IN)
  pdi.pos = XF_ST;

  if (!pdi.done && pdi.dir == PDI_OUT) pdi.byte = pdi.buf[pdi.offs];
  else pdi.byte = 0;
}


// https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
static bool parity(uint8_t v) {
  v ^= v >> 4;
  v &= 0xf;
  return (0x6996 >> v) & 1;
}


static void clock_falling_edge() {rpi_gpio_clr(pdi.clk);}
static void clock_rising_edge()  {rpi_gpio_set(pdi.clk);}


static void blind_clock(unsigned n) {
  while (n--) {
    clock_falling_edge();
    clock_rising_edge();
  }
}


static void clock_out() {
  clock_falling_edge();

  if (pdi.done) rpi_gpio_set(pdi.data); // IDLE
  else {
    bool bit = 0;

    switch (pdi.pos++) {
    case XF_ST: bit = 0; break;

    case XF_0: case XF_1: case XF_2: case XF_3:
    case XF_4: case XF_5: case XF_6: case XF_7:
      bit = (pdi.byte >> (pdi.pos - 1)) & 1;
      break;

    case XF_PAR: bit = parity(pdi.byte); break;
    case XF_SP0: bit = 1;                break;
    case XF_SP1: bit = 1; _next_byte();  break;
    }

    if (bit) rpi_gpio_set(pdi.data);
    else rpi_gpio_clr(pdi.data);
  }

  clock_rising_edge();
}


static void clock_in() {
  clock_falling_edge();
  clock_rising_edge();

  if (!pdi.done) {
    bool bit = rpi_gpio_get(pdi.data);

    switch (pdi.pos) {
    case XF_ST:
      pdi.pos   += !bit; // expect data next if low bit
      pdi.ticks +=  bit; // if idle count timeout
      break;

    case XF_0: case XF_1: case XF_2: case XF_3:
    case XF_4: case XF_5: case XF_6: case XF_7:
      pdi.byte |= bit << pdi.pos;
      pdi.pos++;
      break;

    case XF_PAR:
      if (bit != parity(pdi.byte)) pdi.failed = true;
      pdi.pos++;
      break;

    case XF_SP0: if (!bit) pdi.failed = true; pdi.pos++;    break;
    case XF_SP1: if (!bit) pdi.failed = true; _next_byte(); break;
    }
  }
}


static bool pdi_run(uint32_t length, uint8_t *buf, pdi_dir_t dir) {
  pdi.length = length;
  pdi.buf    = buf;
  pdi.done   = false;
  pdi.failed = false;
  pdi.offs   = 0;
  pdi.pos    = XF_ST;
  pdi.ticks  = 0;
  pdi.byte   = dir == PDI_IN ? 0 : buf[0];

  // Handle direction change
  if (dir != pdi.dir) {
    if (dir == PDI_OUT) {
      rpi_gpio_set(pdi.data);
      rpi_gpio_dir(pdi.data, false);
      blind_clock(2); // minimum 1 clock in this transition direction

    } else
      // a variable number of idle clocks required before start bit received,
      // this will happen automatically by clock_in()
      rpi_gpio_dir(pdi.data, true);

    pdi.dir = dir;
  }

  while (!pdi.done && !pdi.failed) {
    if (pdi.stop || PDI_TIMEOUT <= pdi.ticks) return false;
    if (dir == PDI_OUT) clock_out();
    else clock_in();
  }

  return !pdi.failed;
}


void pdi_break() {
  rpi_gpio_dir(pdi.data, false);
  blind_clock(12);
  blind_clock(12);
}


void pdi_stop() {pdi.stop = true;}


bool pdi_send(const uint8_t *buf, uint32_t len) {
  return pdi_run(len, (uint8_t *)buf, PDI_OUT);
}


bool pdi_recv(uint8_t *buf, uint32_t len) {return pdi_run(len, buf, PDI_IN);}


bool pdi_init(uint8_t clk_pin, uint8_t data_pin) {
  if (!rpi_init()) return false;

  // Set PDI vars
  pdi.stop = false;
  pdi.clk  = clk_pin;
  pdi.data = data_pin;
  pdi.dir  = PDI_IN;

  // Request high priority
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &sp);

  // Set CPU afinity
  cpu_set_t cs;
  CPU_ZERO(&cs);
  CPU_SET(0, &cs);
  sched_setaffinity(1, sizeof(cpu_set_t), &cs);

  // Lock memory
  mlockall(MCL_CURRENT | MCL_FUTURE);

  // Init I/O
  rpi_gpio_clr(pdi.data);
  rpi_gpio_clr(pdi.clk);
  rpi_gpio_dir(pdi.clk, false);
  rpi_gpio_dir(pdi.data, false);

  return true;
}


bool pdi_open() {
  pdi_break();

  // Enter PDI mode
  rpi_gpio_set(pdi.data);
  rpi_delay(1);    // xmega256a3 says 90-1000ns reset pulse width
  blind_clock(16); // next 16 pdi_clk cycles within 100us

  const uint8_t buf[] = {
    STCS | PDI_REG_CONTROL, 0x07, // 2 idle bits
    STCS | PDI_REG_RESET,   0x59, // hold device in reset
    KEY, 0xff, 0x88, 0xd8, 0xcd, 0x45, 0xab, 0x89, 0x12, // enable NVM
  };

  return pdi_send(buf, sizeof(buf));
}


static bool _clear_reset() {
  const uint8_t buf[] = {STCS | PDI_REG_RESET, 0, LDCS | PDI_REG_RESET};
  uint8_t status = 0;

  do {
    if (!pdi_send(buf, sizeof(buf)) || !pdi_recv(&status, 1)) return false;
  } while (status);

  return true;
}


void pdi_close() {
  pdi_open();
  _clear_reset();
  pdi_break();

  // Release gpio pins
  rpi_gpio_dir(pdi.clk, true);
  rpi_gpio_dir(pdi.data, true);

  // Normal priority
  struct sched_param sp;
  memset(&sp, 0, sizeof(sp));
  sp.sched_priority = 0;
  sched_setscheduler(0, SCHED_OTHER, &sp);

  // Unlock memory
  munlockall();
}
