#!/bin/sh
CC=gcc
CXX=g++

# determine target platform
TARGET=
if ( uname -s | grep -qi "MINGW\|MSYS" ); then
  if ( gcc --version | grep -q x86_64 ); then
    TARGET=win64
  else
    TARGET=win32
  fi
fi
if [ "$TARGET" = "" ]; then
  echo "Unsupported platform, currently only MinGW on Windows is supported"
  exit 1;
fi

VERSION=$(sed -ne "s/^#define\s*XLSXIO_VERSION_[A-Z]*\s*\([0-9]*\)\s*$/\1./p" include/xlsxio_version.h | tr -d "\n" | sed -e "s/\.$//")
BUILDDST=build_$TARGET
ZIPFILE=xlsxio-$VERSION-binary-$TARGET.zip
#CFLAGS="-DUSE_LIBZIP"
#LDFLAGS="-lzip -lz"
CFLAGS="-DUSE_MINIZIP"
LDFLAGS="-lminizip -lz"

rm -f $ZIPFILE &> /dev/null
mkdir -p $BUILDDST &&
$CC -mdll -static -o$BUILDDST/xlsxio_read.dll -Wl,--out-implib,$BUILDDST/libxlsxio_read.a -DBUILD_XLSXIO_DLL -DBUILD_XLSXIO_STATIC_DLL -Iinclude lib/xlsxio_read.c lib/xlsxio_read_sharedstrings.c $CFLAGS $LDFLAGS -lexpat &&
$CC -mdll -static -o$BUILDDST/xlsxio_write.dll -Wl,--out-implib,$BUILDDST/libxlsxio_write.a -DBUILD_XLSXIO_DLL -DBUILD_XLSXIO_STATIC_DLL -Iinclude lib/xlsxio_write.c $CFLAGS $LDFLAGS &&
$CC -mconsole -s -o$BUILDDST/xlsxio_xlsx2csv.exe src/xlsxio_xlsx2csv.c -Iinclude $BUILDDST/libxlsxio_read.a &&
$CC -mconsole -s -o$BUILDDST/xlsxio_csv2xlsx.exe src/xlsxio_csv2xlsx.c -Iinclude $BUILDDST/libxlsxio_write.a &&
zip -qj $ZIPFILE LICENSE.txt Changelog.txt README.md include/* $BUILDDST/* &&
echo Created $ZIPFILE &&
rm -rf $BUILDDST
