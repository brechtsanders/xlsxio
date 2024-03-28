#/bin sh
if [ -z "$WINDIR" ]; then
  OSARCH=$(uname -s)_$(uname -m)
  EXEEXT=
  CMAKEMAKEFILETYPE=Unix
else
  OSARCH=win$( if [ "$(uname -m)" == "x86_64" ]; then echo 64; else echo 32; fi)
  EXEEXT=.exe
  CMAKEMAKEFILETYPE=MSYS
fi
BUILDDIR=$(realpath $(dirname $0)/_build_$OSARCH)
BASEDIR=$(realpath $(dirname $0)/..)
VERSION=$(sed -ne "s/^#define\s*XLSXIO_VERSION_[A-Z]*\s*\([0-9]*\)\s*$/\1./p" $(dirname $0)/../include/xlsxio_version.h | tr -d "\n" | sed -e "s/\.$//")
ZIPFILE=$(realpath $(dirname $0)/xlsxio-$VERSION-$OSARCH.zip)

mkdir -p "$BUILDDIR"
mkdir -p "$BUILDDIR"/CMakeFiles/CMakeTmp/CMakeFiles
cd "$BUILDDIR"
#mkdir -p CMakeFiles/CMakeTmp/CMakeFiles
#touch CMakeFiles/CMakeTmp/CMakeFiles/cmake.check_cache
echo Configuring... &&
 cmake$EXEEXT -G"$CMAKEMAKEFILETYPE Makefiles" -DCMAKE_INSTALL_PREFIX:PATH="_package_" -DCMAKE_BUILD_TYPE:STRING=Release -DBUILD_STATIC:BOOL=ON -DBUILD_SHARED:BOOL=ON -DWITH_LIBZIP:BOOL=OFF -DWITH_WIDE:BOOL=ON -DBUILD_DOCUMENTATION:BOOL=OFF ../.. &&
 if [ "$CMAKEMAKEFILETYPE" = "MSYS" ]; then
  # build statically linked DLL on Windows
  sed -i -e "s/\.dll\.a/.a -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++/" CMakeFiles/xlsxio_write_SHARED.dir/linklibs.rsp &&
  sed -i -e "s/^C_DEFINES\s*=.*-DBUILD_XLSXIO_DLL/& -DSTATIC/" CMakeFiles/xlsxio_write_SHARED.dir/flags.make
  sed -i -e "s/\.dll\.a/.a -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++/" CMakeFiles/xlsxio_read_SHARED.dir/linklibs.rsp &&
  sed -i -e "s/^C_DEFINES\s*=.*-DBUILD_XLSXIO_DLL/& -DSTATIC/" CMakeFiles/xlsxio_read_SHARED.dir/flags.make
  sed -i -e "s/\.dll\.a/.a -static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++/" CMakeFiles/xlsxio_readw_SHARED.dir/linklibs.rsp &&
  sed -i -e "s/^C_DEFINES\s*=.*-DBUILD_XLSXIO_DLL/& -DSTATIC/" CMakeFiles/xlsxio_readw_SHARED.dir/flags.make
 fi &&
 echo Building... &&
 make install/strip VERBOSE=1 &&
 echo Making binary package: $ZIPFILE... &&
 #pushd $BUILDDIR/_package_ &> /dev/null &&
 cd _package_ &&
 cp -f $BASEDIR/LICENSE.txt $BASEDIR/Changelog.txt $BASEDIR/*.md . &&
 rm -f $ZIPFILE &&
 zip -r -9 $ZIPFILE * &&
 #popd &> /dev/null &&
 cd .. &&
 echo OK

