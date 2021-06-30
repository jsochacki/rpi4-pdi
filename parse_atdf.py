#!/usr/bin/env python3

import sys
from xml.etree import ElementTree as ET


for filename in sys.argv[1:]:
  r = ET.parse(filename).getroot()

  for i in r.findall('.//interface'):
    if i.attrib['type'] == 'pdi':
      device = r.find('./devices/device').attrib['name'].lower()[2:]
      sig = 0

      for seg in r.findall('.//memory-segment'):
        name = seg.attrib['name']

        if name == 'FUSES': fuse_size = int(seg.attrib['size'], 0)

        if name == 'APP_SECTION':
          app_size = int(seg.attrib['size'], 0)
          page_size = int(seg.attrib['pagesize'], 0)

        if name == 'EEPROM':
          eeprom_size = int(seg.attrib['size'], 0)
          eeprom_page = int(seg.attrib['pagesize'], 0)

        if name == 'BOOT_SECTION':    boot_size    = int(seg.attrib['size'], 0)
        if name == 'INTERNAL_SRAM':   sram_size    = int(seg.attrib['size'], 0)
        if name == 'LOCKBITS':        lock_size    = int(seg.attrib['size'], 0)
        if name == 'PROD_SIGNATURES': prodsig_size = int(seg.attrib['size'], 0)
        if name == 'USER_SIGNATURES': usersig_size = int(seg.attrib['size'], 0)

      for prop in r.findall('.//property'):
        name = prop.attrib['name']
        value = prop.attrib['value']

        if name[0:9] == 'SIGNATURE':
          i = int(name[9:])
          sig += int(value, 0) << (8 * (2 - i))

      print('  {')
      print('    "%s",'   % device)
      print('    0x%06x,' % sig)
      print('    %d,'     % page_size)
      print('    %d,'     % sram_size)
      print('    %d,'     % eeprom_size)
      print('    %d,'     % eeprom_page)
      print('    %d,'     % app_size)
      print('    %d,'     % boot_size)
      print('    %d,'     % fuse_size)
      print('    %d,'     % lock_size)
      print('    %d,'     % usersig_size)
      print('    %d,'     % prodsig_size)
      print('  },')
