/*
  Copyright (C) 2021 Buildbotics LLC.
  Copyright (C) 2015 DiUS Computing Pty. Ltd.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "pdi.h"
#include "nvm.h"
#include "ihex.h"
#include "devices.h"
#include "mem.h"
#include "crc.h"
#include "error.h"

#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>


#define MAX_FUSES 32
#define BUF_SIZE  (512 * 1024)


typedef struct {
  uint8_t num;
  uint8_t value;
} fuse_t;


static void _sig(int sig) {
  signal(sig, SIG_DFL);
  pdi_stop();
}


static void dump_skipped(uint32_t skipped) {
  if (skipped) printf("* skipped %08x bytes of 'ff'\n", skipped);
}


static void dump_data(uint32_t address, uint8_t *data, unsigned size) {
  uint32_t skipped = 0;

  for (unsigned i = 0; i < size; i += 16) {
    bool empty = true;
    for (unsigned j = 0; empty && j < 16 && i + j < size; j++)
      if (data[i + j] != 0xff) empty = false;

    if (empty) {
      skipped += 16;
      continue;
    }

    dump_skipped(skipped);
    skipped = 0;

    printf("%08x  ", address + i);

    for (unsigned j = 0; j < 16 && i + j < size; j++) {
      if (j == 8) putchar(' ');
      printf(" %02x", data[i + j]);
    }

    printf("  |");

    for (unsigned j = 0; j < 16 && i + j < size; j++) {
      if (isprint(data[i + j])) putchar(data[i + j]);
      else putchar('.');
    }

    printf("|\n");
  }

  dump_skipped(skipped);
  putchar('\n');
}


static fuse_t parse_fuse(const char *s) {
  char *equal = strchr(s, '=');
  if (!equal) fail("Invalid fuse format: %s", s);

  *equal = 0;
  int num   = strtol(s, 0, 0);
  int value = strtol(equal + 1, 0, 0);

  if (num < 0 || 255 < num || value < 0 || 255 < value)
    fail("Invalid fuse setting: %s", s);

  fuse_t fuse;
  fuse.num   = num;
  fuse.value = value;

  return fuse;
}


void help(const char *name) {
  printf(
    "Usage: %s [OPTIONS]\n"
    "\n"
    "OPTIONS:\n"
    "  -a [ADDRESS]     Manually set base address (default=0x%08x)\n"
    "  -s [SIZE]        Manually set memory size\n"
    "  -m [MEMORY]      Set memory type, base address and size by name\n"
    "  -i [DEVICE]      Manually select device\n"
    "  -c [PIN]         Set GPIO pin to use as PDI_CLK\n"
    "  -d [PIN]         Set GPIO pin to use as PDI_DATA\n"
    "  -D               Dump memory\n"
    "  -e               Erase the selected memory one page at a time.\n"
    "  -E               Erase entire chip, except for the user signature row\n"
    "  -w [FILE.HEX]    Write Intel HEX file to memory\n"
    "  -r [FILE.HEX]    Read Intel HEX file from memory\n"
    "  -f [FUSE=VALUE]  Write a fuse or lock bit\n"
    "  -x               Make no changes if chip and HEX file CRCs match\n"
    "  -q               Print less information\n"
    "  -h               Show this help and exit\n"
    "\n"
    "MEMORY:\n",
    name, FLASH_BASE_ADDR);

  mem_print();

  printf("\nDEVICE:\n  ");

  for (int i = 0; devices[i].name; i++) {
    printf("%-13s", devices[i].name);
    if ((i + 1) % 5 == 0) printf("\n  ");
  }

  printf("\n");
}


int main(int argc, char *argv[]) {
  signal(SIGINT,  _sig);
  signal(SIGTERM, _sig);
  signal(SIGQUIT, _sig);

  const device_t *device       = 0;
  const memory_t *mem          = mem_get("flash");
  uint32_t        address      = 0;
  uint32_t        size         = 0;
  bool            dump         = false;
  uint8_t         clk_pin      = 0;
  uint8_t         data_pin     = 0;
  const char     *read_file    = 0;
  const char     *write_file   = 0;
  bool            chip_erase   = false;
  bool            erase        = false;
  bool            crc_check    = false;
  bool            verbose      = true;
  uint8_t         num_fuses    = 0;
  fuse_t          fuses[MAX_FUSES];
  uint8_t         buf[BUF_SIZE];
  int             opt;

  while ((opt = getopt(argc, argv, "a:s:m:c:d:r:w:DEexqi:f:h")) != -1) {
    switch (opt) {
    case 'a': address    = strtoul(optarg, 0, 0); break;
    case 's': size       = strtoul(optarg, 0, 0); break;
    case 'm': mem        = mem_get(optarg);       break;
    case 'c': clk_pin    = atoi(optarg);          break;
    case 'd': data_pin   = atoi(optarg);          break;
    case 'r': read_file  = optarg;                break;
    case 'w': write_file = optarg;                break;
    case 'D': dump       = true;                  break;
    case 'E': chip_erase = true;                  break;
    case 'e': erase      = true;                  break;
    case 'x': crc_check  = true;                  break;
    case 'q': verbose    = false;                 break;

    case 'i':
      device = devices_find(optarg);
      if (!device) fail("Unrecognized device %s", device);
      break;

    case 'f':
      if (num_fuses == MAX_FUSES) fail("Too many fuses");
      fuses[num_fuses++] = parse_fuse(optarg);
      break;

    case 'h': help(argv[0]); return 0;
    default:  help(argv[0]); return 1; break;
    }
  }

  if (clk_pin == data_pin)
    fail("Set clock and data pins to the correct GPIO lines using the "
         "'-c PIN' and '-d PIN' options");

  if (BUF_SIZE < size) fail("Size too large");

  if (!pdi_init(clk_pin, data_pin)) fail("Failed to init PDI");

  // Get and check device by ID
  uint32_t dev_id = nvm_read_device_id();
  if (!device) device = devices_find_by_sig(dev_id);

  if (device) {if (verbose) {devices_print(device); printf("\n");}}
  else if (dev_id != (uint32_t)-1) fail("Unsupported device ID 0x%06x", dev_id);
  else fail("Device not detected, please specify a device with '-i'");

  if (device->sig != dev_id)
    printf("WARNING detected device ID 0x%06x does not match specified "
           "device %s with ID 0x%06x\n", dev_id, device->name, device->sig);

  // Resolve mem address and size
  if (mem) {
    if (!address) address = mem_get_addr(mem, device);
    if (!size)       size = mem_get_size(mem, device);
  }

  // Read memory
  if ((dump || read_file) && !nvm_read(address, buf, size))
    fail("Failed to read %u bytes from address 0x%08x", size, address);

  // Dump memory
  if (dump) dump_data(address, buf, size);

  // Check CRC
  uint32_t chip_crc = -1;
  if (crc_check) {
    if (mem->type == NVM_FLASH) chip_crc = nvm_flash_crc();

    if (mem->type != NVM_FLASH || chip_crc == (uint32_t)-1) {
      // Read memory if we haven't already
      if ((!dump && !read_file) && !nvm_read(address, buf, size))
        fail("Failed to read %u bytes from address 0x%08x", size, address);

      chip_crc = crc24_block(buf, size, 0);
    }

    if (verbose) printf("CRC 0x%06x for %s\n", chip_crc, mem->name);
  }

  // Save HEX file
  if (read_file) {
    FILE *f = fopen(read_file, "wt");
    if (!f) fail("Failed to open file %s", read_file);

    ihex_write(f, buf, size);
    fclose(f);

    if (verbose)
      printf("Wrote %u bytes to %s from %s\n", size, read_file, mem->name);
  }

  // Compute pages
  uint16_t page_size = mem_get_page_size(mem, device);
  uint32_t pages = 0;

  if (page_size) {
    pages = size / page_size;
    if (size % page_size) pages++;

  } else {
    if (write_file) fail("Cannot write to %s", mem->name);
    if (erase)      fail("Cannot erase %s",    mem->name);
  }

  // Load HEX file
  uint32_t computed_crc = 0;
  uint16_t page_fill[BUF_SIZE / 512];
  if (write_file) {
    // Clear memory
    memset(buf, 0xff, BUF_SIZE);

    // Read HEX file
    FILE *f = fopen(write_file, "rt");
    if (!f) fail("Failed to open file %s", write_file);

    uint32_t bytes = 0;
    if (ihex_read(f, buf, BUF_SIZE, &bytes) || !bytes)
      fail("Failed to read HEX file %s", write_file);

    fclose(f);

    // Check each page
    for (unsigned i = 0; i < pages; i++) {
      uint32_t offset = i * page_size;

      // Compute page fill
      page_fill[i] = page_size;
      uint8_t *ptr = buf + offset + page_size;
      for (unsigned j = 0; j < page_size; j++)
        if (*--ptr == 0xff) page_fill[i]--;
        else break;

      // Limit range of last page
      if (size < offset + page_fill[i])
        page_fill[i] = size % page_size;
    }

    // Compute CRC
    computed_crc = crc24_block(buf, size, 0);

    if (crc_check) {
      if (computed_crc == chip_crc) {
        if (verbose) printf("CRCs match, nothing to do\n");
        // TODO Fuse and locks bits may not match the requested programming
        return 0;
      }

      if (verbose) printf("CRCs do not match, proceeding\n");
    }
  }

  // Erase chip
  if (chip_erase) {
    if (!nvm_chip_erase()) fail("Failed to perform chip erase");
    if (verbose) printf("Chip erased\n");
  }

  // Erase memory
  if (erase) {
    for (unsigned i = 0; i < pages; i++) {
      uint32_t offset = i * page_size;
      uint32_t addr = address + offset;

      if (!nvm_erase_page(mem->type, addr))
        fail("Failed to erase page at address 0x%08x", addr);
    }

    if (verbose) printf("Erased %u %s pages\n", pages, mem->name);
  }

  // Write fuses
  for (unsigned i = 0; i < num_fuses; i++) {
    if (device->fuse_size + device->lock_size <= fuses[i].num)
      fail("Invalid fuse %d for device %s", fuses[i].num, device->name);

    if (!nvm_write_fuse(fuses[i].num, fuses[i].value))
      fail("Failed to write fuse %d", fuses[i].num);

    if (verbose)
      printf("Wrote 0x%02x to fuse %d\n", fuses[i].value, fuses[i].num);
  }

  // Write IHEX to memory
  if (write_file) {
    // Erase and write pages
    uint32_t empty = 0;

    for (unsigned i = 0; i < pages; i++) {
      uint32_t offset = i * page_size;
      uint32_t addr = address + offset;

      if (!page_fill[i]) {
        if (!nvm_erase_page(mem->type, addr))
          fail("Failed to erase page at address 0x%08x", addr);

        empty++;

      } else if (!nvm_write_page(mem->type, addr, &buf[offset], page_fill[i]))
        fail("Failed to write page at address 0x%08x", addr);
    }

    if (verbose) printf("Wrote %u pages to %s\n", pages - empty, mem->name);

    // Check CRC
    if (crc_check) {
      if (mem->type == NVM_FLASH) chip_crc = nvm_flash_crc();

      if (mem->type != NVM_FLASH || chip_crc == (uint32_t)-1) {
        if (!nvm_read(address, buf, size))
          fail("Failed to read %u bytes from address 0x%08x", size, address);

        chip_crc = crc24_block(buf, size, 0);
      }

      if (computed_crc != chip_crc)
        fail("Computed CRC 0x%06x does not match chip CRC 0x%06x for %s",
             computed_crc, chip_crc, mem->name);
      else if (verbose) printf("CRC correct\n");
    }
  }

  pdi_close();

  return 0;
}
