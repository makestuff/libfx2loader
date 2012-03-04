#
# Copyright (C) 2009-2012 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Helper script for building the binary distribution. It's unlikely you'll need
# this unless you're forking the project.
#
#!/bin/bash
export LIB=libfx2loader
export DATE=$(date +%Y%m%d)
rm -rf ${LIB}-${DATE}
mkdir ${LIB}-${DATE}

# Linux x86_64 binaries
mkdir -p ${LIB}-${DATE}/linux.x86_64/rel
cp -rp linux.x86_64/rel/*.so ${LIB}-${DATE}/linux.x86_64/rel/
mkdir -p ${LIB}-${DATE}/linux.x86_64/dbg
cp -rp linux.x86_64/dbg/*.so ${LIB}-${DATE}/linux.x86_64/dbg/

# Linux i686 binaries
mkdir -p ${LIB}-${DATE}/linux.i686/rel
cp -rp linux.i686/rel/*.so ${LIB}-${DATE}/linux.i686/rel/
mkdir -p ${LIB}-${DATE}/linux.i686/dbg
cp -rp linux.i686/dbg/*.so ${LIB}-${DATE}/linux.i686/dbg/

# Linux armel binaries
mkdir -p ${LIB}-${DATE}/linux.armel/rel
cp -rp linux.armel/rel/*.so ${LIB}-${DATE}/linux.armel/rel/
mkdir -p ${LIB}-${DATE}/linux.armel/dbg
cp -rp linux.armel/dbg/*.so ${LIB}-${DATE}/linux.armel/dbg/

# Linux ppc binaries
mkdir -p ${LIB}-${DATE}/linux.ppc/rel
cp -rp linux.ppc/rel/*.so ${LIB}-${DATE}/linux.ppc/rel/
mkdir -p ${LIB}-${DATE}/linux.ppc/dbg
cp -rp linux.ppc/dbg/*.so ${LIB}-${DATE}/linux.ppc/dbg/

# MacOS binaries
mkdir -p ${LIB}-${DATE}/darwin/rel
cp -rp darwin/rel/*.dylib ${LIB}-${DATE}/darwin/rel/
mkdir -p ${LIB}-${DATE}/darwin/dbg
cp -rp darwin/dbg/*.dylib ${LIB}-${DATE}/darwin/dbg/

# Windows binaries
mkdir -p ${LIB}-${DATE}/win32/rel
cp -rp win32/rel/*.dll ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.lib ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.pdb ${LIB}-${DATE}/win32/rel/
mkdir -p ${LIB}-${DATE}/win32/dbg
cp -rp win32/dbg/*.dll ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.lib ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.pdb ${LIB}-${DATE}/win32/dbg/

# FX2Loader binaries
export FX2CLI=../../apps/fx2loader
cp -rp ${FX2CLI}/linux.x86_64/rel/libargtable2.so ${FX2CLI}/linux.x86_64/rel/fx2loader ${LIB}-${DATE}/linux.x86_64/rel/
cp -rp ${FX2CLI}/linux.x86_64/dbg/libargtable2.so ${FX2CLI}/linux.x86_64/dbg/fx2loader ${LIB}-${DATE}/linux.x86_64/dbg/
cp -rp ${FX2CLI}/linux.i686/rel/libargtable2.so ${FX2CLI}/linux.i686/rel/fx2loader ${LIB}-${DATE}/linux.i686/rel/
cp -rp ${FX2CLI}/linux.i686/dbg/libargtable2.so ${FX2CLI}/linux.i686/dbg/fx2loader ${LIB}-${DATE}/linux.i686/dbg/
cp -rp ${FX2CLI}/linux.armel/rel/libargtable2.so ${FX2CLI}/linux.armel/rel/fx2loader ${LIB}-${DATE}/linux.armel/rel/
cp -rp ${FX2CLI}/linux.armel/dbg/libargtable2.so ${FX2CLI}/linux.armel/dbg/fx2loader ${LIB}-${DATE}/linux.armel/dbg/
cp -rp ${FX2CLI}/linux.ppc/rel/libargtable2.so ${FX2CLI}/linux.ppc/rel/fx2loader ${LIB}-${DATE}/linux.ppc/rel/
cp -rp ${FX2CLI}/linux.ppc/dbg/libargtable2.so ${FX2CLI}/linux.ppc/dbg/fx2loader ${LIB}-${DATE}/linux.ppc/dbg/
cp -rp ${FX2CLI}/darwin/rel/libargtable2.dylib ${FX2CLI}/darwin/rel/fx2loader ${LIB}-${DATE}/darwin/rel/
cp -rp ${FX2CLI}/darwin/dbg/libargtable2.dylib ${FX2CLI}/darwin/dbg/fx2loader ${LIB}-${DATE}/darwin/dbg/
cp -rp ${FX2CLI}/win32/rel/argtable2.dll ${FX2CLI}/win32/rel/fx2loader.exe ${LIB}-${DATE}/win32/rel/
cp -rp ${FX2CLI}/win32/dbg/argtable2.dll ${FX2CLI}/win32/dbg/fx2loader.exe ${LIB}-${DATE}/win32/dbg/

# Firmware
mkdir -p ${LIB}-${DATE}/firmware
cp -rp firmware/firmware.hex ${LIB}-${DATE}/firmware/

# Headers
cp -rp ../../common/makestuff.h ${LIB}-${DATE}/
cp -rp ../libbuffer/libbuffer.h ${LIB}-${DATE}/
cp -rp ${LIB}.h ${LIB}-${DATE}/

cp -p COPYING COPYING.LESSER ${LIB}-${DATE}/
cat > ${LIB}-${DATE}/README <<EOF
FX2Loader Binary Distribution

FX2Loader is a library for loading and saving Cypress FX2LP firmware, and converting between the
various file formats it uses (.bix, .hex, .iic).

Overview here: http://www.makestuff.eu/wordpress/?page_id=343
Source code here: https://github.com/makestuff/libfx2loader
API docs here: http://bit.ly/fx2loader-api
Example code here: http://bit.ly/fx2loader-ex

The libfx2loader library is licensed under the LGPLv3; the fx2loader executable is licensed under
the GPLv3.

The default FX2LP firmware (i.e with EEPROM isolated) enumerates as VID=0x04B4 (Cypress) and
PID=0x8613 (FX2LP), and provides support for loading firmware into RAM. In order to write the EEPROM
you need to load a firmware which supports EEPROM writes (e.g the provided firmware) into RAM first.

You can use any file type (.hex/.ihx, .bix or .iic) as a source or destination, and it will be
converted if necessary. The EEPROM native format is .iic and the RAM native format is .bix. The
conversion between different representations is potentially lossy (e.g there may be some
configuration data beyond the end of the I2C records in an EEPROM or .iic file, which will be lost
if you convert it to a .hex file).

If you're unsure about the suitability of a new firmware (wherever you got it from), it's a good
idea to load it into RAM first to make sure it's not totally broken. Writing bad firmware to EEPROM
will brick your device.

If you manage to brick your device, you will need to isolate the EEPROM. Your board may provide a
jumper for this purpose. Failing that you will need to cut the SDA track between the FX2LP and the
EEPROM, load a good firmware into RAM, reconnect the EEPROM (e.g replace the jumper) and load the
good firmware into EEPROM.

Uploading your own SDCC-compiled firmware:
    Write SDCC-generated I8HEX file to FX2LP's RAM:
        fx2loader -v 0x04B4 -p 0x8613 firmware.hex
    Write SDCC-generated I8HEX file to FX2LP's EEPROM (assuming current firmware supports EEPROM writes):
        fx2loader -v 0x04B4 -p 0x8613 firmware.hex eeprom

Backup and restore the existing firmware:
    Backup FX2LP's existing 128kbit (16kbyte) EEPROM data:
        fx2loader -v 0x04B4 -p 0x8613 eeprom:128 backup.iic
    Restore FX2LP's 128kbit (16kbyte) EEPROM data from backup I2C file (assuming current firmware supports EEPROM writes):
        fx2loader -v 0x04B4 -p 0x8613 backup.iic eeprom

Convert between .hex files, .bix files and .iic files (file extensions are considered):
    fx2loader -v 0x04B4 -p 0x8613 myfile.iic myfile.bix
EOF

# Package it up
tar zcf ${LIB}-${DATE}.tar.gz ${LIB}-${DATE}
rm -rf ${LIB}-${DATE}
cp -p ${LIB}-${DATE}.tar.gz /mnt/ukfsn/bin/
