rpipdi
=============

Program Atmel XMEGA chips from a Raspberry Pi 2 or newer via the PDI protocol.
This code uses GPIO pins and realtime priority to enable PDI programming on the
Pi.  See the original software for more information:
https://github.com/DiUS/xmega-pdi-pi2

# Features
  - Chip erase
  - Program application & boot areas
  - Read & write fuses
  - Read & write EEPROM
  - Read & write user row
  - Dump memory to screen
  - Read and write intel HEX files
  - Configurable GPIO selection
  - Configurable memory address and size
  - Device auto detection
  - CRC checking
  - Make no changes on CRC match

# Usage
```
Usage: ./rpipdi [OPTIONS]

OPTIONS:
  -a [ADDRESS]     Manually set base address (default=0x00800000)
  -s [SIZE]        Manually set memory size
  -m [MEMORY]      Set memory type, base address and size by name
  -i [DEVICE]      Manually select device
  -c [PIN]         Set GPIO pin to use as PDI_CLK
  -d [PIN]         Set GPIO pin to use as PDI_DATA
  -D               Dump memory
  -e               Erase the selected memory one page at a time.
  -E               Erase entire chip, except for the user signature row
  -w [FILE.HEX]    Write Intel HEX file to FLASH
  -r [FILE.HEX]    Read Intel HEX file from FLASH
  -f [FUSE=VALUE]  Write a fuse or lock bit
  -x               Make no changes if chip and HEX file CRCs match
  -q               Print less information
  -h               Show this help and exit

MEMORY:
  flash    App & boot sections
  app      App section of FLASH
  boot     Boot section of FLASH
  eeprom   EEPROM base address
  prod     Production signature row
  user     User signature row
  fuse     Fuse base address
  lock     Lock bits base address
  io       Mapped I/O base address
```

All addresses are in the PDI address space.  See the xmega datasheets.

# Suported devices
```
  xmega128a1   xmega128a1u  xmega128a3   xmega128a3u  xmega128a4u
  xmega128b1   xmega128b3   xmega128c3   xmega128d3   xmega128d4
  xmega16a4    xmega16a4u   xmega16c4    xmega16d4    xmega16e5
  xmega192a3   xmega192a3   xmega192a3u  xmega192a3u  xmega192c3
  xmega192d3   xmega256a3   xmega256a3b  xmega256a3bu xmega256a3u
  xmega256c3   xmega256d3   xmega32a4    xmega32a4u   xmega32c3
  xmega32c4    xmega32d3    xmega32d4    xmega32e5    xmega384c3
  xmega384d3   xmega64a1    xmega64a1u   xmega64a3    xmega64a3u
  xmega64a4u   xmega64b1    xmega64b3    xmega64c3    xmega64d3
  xmega64d4    xmega8e5
```

# Examples
Note, ``rpidpi`` currently must be run as root as it uses ``/dev/mem`` to
access GPIO.

## Erase chip and write FLASH
    sudo ./rpipdi -c 27 -d 23 -E -w firmware.hex

## Dump fuse bytes to screen
    sudo ./rpipdi -c 24 -d 21 -D -m fuse

## Erase chip, write FLASH and fuse byte if CRC does not match
    sudo ./rpipdi -c 27 -d 23 -E -f 2=0xbe -w firmware.hex -x

## Erase user row
    sudo ./rpipdi -c 27 -d 23 -m user -e

## Print the boot section CRC
    sudo ./rpipdi -c 27 -d 23 -m boot -x

# Building
    sudo apt-get update
    sudo apt-get install -y build-essential git
    git clone git@github.com:buildbotics/rpipdi.git
    cd rpipdi
    make
