#!/bin/bash
export LIB=libfx2loader
export DATE=$(date +%Y%m%d)
rm -rf ${LIB}-${DATE}
mkdir ${LIB}-${DATE}

# Linux binaries
mkdir -p ${LIB}-${DATE}/linux/rel
cp -rp linux/rel/*.so ${LIB}-${DATE}/linux/rel/
cp -rp fx2loader/linux/rel/fx2loader ${LIB}-${DATE}/linux/rel/
mkdir -p ${LIB}-${DATE}/linux/dbg
cp -rp linux/dbg/*.so ${LIB}-${DATE}/linux/dbg/
cp -rp fx2loader/linux/dbg/fx2loader ${LIB}-${DATE}/linux/dbg/

# MacOS binaries
mkdir -p ${LIB}-${DATE}/darwin/rel
cp -rp darwin/rel/*.dylib ${LIB}-${DATE}/darwin/rel/
cp -rp fx2loader/darwin/rel/fx2loader ${LIB}-${DATE}/darwin/rel/
mkdir -p ${LIB}-${DATE}/darwin/dbg
cp -rp darwin/dbg/*.dylib ${LIB}-${DATE}/darwin/dbg/
cp -rp fx2loader/darwin/dbg/fx2loader ${LIB}-${DATE}/darwin/dbg/

# Windows binaries
mkdir -p ${LIB}-${DATE}/win32/rel
cp -rp win32/rel/*.dll ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.lib ${LIB}-${DATE}/win32/rel/
cp -rp win32/rel/*.pdb ${LIB}-${DATE}/win32/rel/
cp -rp fx2loader/win32/rel/fx2loader.exe ${LIB}-${DATE}/win32/rel/
mkdir -p ${LIB}-${DATE}/win32/dbg
cp -rp win32/dbg/*.dll ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.lib ${LIB}-${DATE}/win32/dbg/
cp -rp win32/dbg/*.pdb ${LIB}-${DATE}/win32/dbg/
cp -rp fx2loader/win32/dbg/fx2loader.exe ${LIB}-${DATE}/win32/dbg/

# Headers
cp -rp ../../common/makestuff.h ${LIB}-${DATE}/
cp -rp ../libbuffer/libbuffer.h ${LIB}-${DATE}/
cp -rp ${LIB}.h ${LIB}-${DATE}/

cp -p LICENSE.txt ${LIB}-${DATE}/
cat > ${LIB}-${DATE}/README <<EOF
FX2Loader Binary Distribution

FX2Loader is a library for loading and saving Cypress FX2LP firmware, and converting between the
various file formats it uses (.bix, .hex, .iic).

Overview here: http://www.makestuff.eu/wordpress/?page_id=343
Source code here: https://github.com/makestuff/libfx2loader
API docs here: http://bit.ly/fx2loader-api
Example code here: http://bit.ly/fx2loader-ex
EOF

# Package it up
tar zcf ${LIB}-${DATE}.tar.gz ${LIB}-${DATE}
rm -rf ${LIB}-${DATE}
cp -p ${LIB}-${DATE}.tar.gz /mnt/ukfsn/bin/
