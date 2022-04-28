# parameters:
#   PREFIX=<path>       Path were install places files (defaults to /usr/local)
#   WITH_LIBZIP=1       Use libzip instead of minizip
#   WIDE=1              Also build UTF-16 library (xlsxio_readw) - experimental, only tested on Windows
#   STATICDLL=1         Build a static DLL (= doesn't depend on any other DLLs) - only supported on Windows

ifeq ($(OS),)
OS = $(shell uname -s)
endif
PREFIX = /usr/local
CC   = gcc
CPP  = g++
AR   = ar
LIBPREFIX = lib
LIBEXT = .a
ifeq ($(OS),Windows_NT)
BINEXT = .exe
SOEXT = .dll
else ifeq ($(OS),Darwin)
BINEXT =
SOEXT = .dylib
else
BINEXT =
SOEXT = .so
endif
INCS = -Iinclude -Ilib
CFLAGS = -O3
override CFLAGS += $(INCS)
STATIC_CFLAGS = -DBUILD_XLSXIO_STATIC
SHARED_CFLAGS = -DBUILD_XLSXIO_DLL
LIBS =
LDFLAGS =
ifeq ($(OS),Darwin)
override CFLAGS += -I/opt/local/include -I/opt/local/lib/libzip/include
override LDFLAGS += -L/opt/local/lib
#override CFLAGS += -arch i386 -arch x86_64
#override LDFLAGS += -arch i386 -arch x86_64
STRIPFLAG =
else
STRIPFLAG = -s
endif
MKDIR = mkdir -p
RM = rm -f
RMDIR = rm -rf
CP = cp -f
CPDIR = cp -rf
DOXYGEN := $(shell which doxygen)

OSALIAS := $(OS)
ifeq ($(OS),Windows_NT)
ifneq (,$(findstring x86_64,$(shell gcc --version)))
OSALIAS := win64
else
OSALIAS := win32
endif
endif

ifdef WITH_LIBZIP
ZIPLIB_LDFLAGS = -lzip
ZIPLIB_DEPS_LDFLAGS = -Wl,--as-needed -lz -lbz2 -lcrypto -lgdi32
override CFLAGS += -DUSE_LIBZIP
else
ZIPLIB_LDFLAGS = -lminizip
ZIPLIB_DEPS_LDFLAGS = -Wl,--as-needed -lz
override CFLAGS += -DUSE_MINIZIP
endif

XLSXIOREAD_OBJ = lib/xlsxio_read.o lib/xlsxio_read_sharedstrings.o
XLSXIOREAD_LDFLAGS = $(ZIPLIB_LDFLAGS) -lexpat
XLSXIOREADW_LDFLAGS = $(ZIPLIB_LDFLAGS) -lexpatw
XLSXIOREAD_SHARED_LDFLAGS =
XLSXIOWRITE_OBJ = lib/xlsxio_write.o
XLSXIOWRITE_LDFLAGS = $(ZIPLIB_LDFLAGS)
XLSXIOWRITE_SHARED_LDFLAGS =
ifneq ($(OS),Windows_NT)
override SHARED_CFLAGS += -fPIC
endif
ifeq ($(OS),Windows_NT)
XLSXIOREAD_SHARED_LDFLAGS += -Wl,--out-implib,$@$(LIBEXT) -Wl,--compat-implib -Wl,--output-def,$(@:%.dll=%.def)
XLSXIOWRITE_SHARED_LDFLAGS += -Wl,--out-implib,$@$(LIBEXT) -Wl,--compat-implib -Wl,--output-def,$(@:%.dll=%.def)
else ifeq ($(OS),Darwin)
else
XLSXIOWRITE_LDFLAGS += -pthread
endif
ifeq ($(OS),Darwin)
OS_LINK_FLAGS = -dynamiclib -o $@
else
OS_LINK_FLAGS = -shared -Wl,-soname,$@ $(STRIPFLAG)
endif

ifdef STATICDLL
ifeq ($(OS),Windows_NT)
# lines below to compile Windows DLLs with no dependancies
ifdef WITH_LIBZIP
override CFLAGS += -DZIP_STATIC
endif
XLSXIOREAD_LDFLAGS += -static $(ZIPLIB_DEPS_LDFLAGS)
XLSXIOREADW_LDFLAGS += -static $(ZIPLIB_DEPS_LDFLAGS)
XLSXIOWRITE_LDFLAGS += -static $(ZIPLIB_DEPS_LDFLAGS)
endif
endif

LIBLIST = xlsxio_read xlsxio_write
TOOLS_BIN = xlsxio_xlsx2csv$(BINEXT) xlsxio_csv2xlsx$(BINEXT)
EXAMPLES_BIN = example_xlsxio_write_getversion$(BINEXT) example_xlsxio_write$(BINEXT) example_xlsxio_read$(BINEXT) example_xlsxio_read_advanced$(BINEXT)

ifdef WIDE
CFLAGS_W = $(CFLAGS) -DXML_UNICODE
LIBLIST += xlsxio_readw
EXAMPLES_BIN += example_xlsxio_readw$(BINEXT)
endif

COMMON_PACKAGE_FILES = README.md LICENSE.txt Changelog.txt
SOURCE_PACKAGE_FILES = $(COMMON_PACKAGE_FILES) Makefile CMakeLists.txt CMake/ *.in doc/Doxyfile include/*.h lib/*.c lib/*.h src/*.c examples/*.c build/*.cbp

default: all

all: static-lib shared-lib tools

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.static.o: %.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS)

%.shared.o: %.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS)

static-lib: $(LIBLIST:%=$(LIBPREFIX)%$(LIBEXT))

shared-lib: $(LIBLIST:%=$(LIBPREFIX)%$(SOEXT))

$(LIBPREFIX)xlsxio_read$(LIBEXT): $(XLSXIOREAD_OBJ:%.o=%.static.o)
	$(AR) cr $@ $^

$(LIBPREFIX)xlsxio_read$(SOEXT): $(XLSXIOREAD_OBJ:%.o=%.shared.o)
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(XLSXIOREAD_SHARED_LDFLAGS) $(XLSXIOREAD_LDFLAGS) $(LDFLAGS) $(LIBS)

$(LIBPREFIX)xlsxio_write$(LIBEXT): $(XLSXIOWRITE_OBJ:%.o=%.static.o)
	$(AR) cr $@ $^

$(LIBPREFIX)xlsxio_write$(SOEXT): $(XLSXIOWRITE_OBJ:%.o=%.shared.o)
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(XLSXIOWRITE_SHARED_LDFLAGS) $(XLSXIOWRITE_LDFLAGS) $(LDFLAGS) $(LIBS)

ifdef WIDE
%.wstatic.o: %.c
	$(CC) -c -o $@ $< $(STATIC_CFLAGS) $(CFLAGS_W) 

%.wshared.o: %.c
	$(CC) -c -o $@ $< $(SHARED_CFLAGS) $(CFLAGS_W)

$(LIBPREFIX)xlsxio_readw$(LIBEXT): $(XLSXIOREAD_OBJ:%.o=%.wstatic.o)
	$(AR) cr $@ $^

$(LIBPREFIX)xlsxio_readw$(SOEXT): $(XLSXIOREAD_OBJ:%.o=%.wshared.o)
	$(CC) -o $@ $(OS_LINK_FLAGS) $^ $(XLSXIOREAD_SHARED_LDFLAGS) $(XLSXIOREADW_LDFLAGS) $(LIBS)
endif

examples: $(EXAMPLES_BIN)

example_xlsxio_write_getversion$(BINEXT): examples/example_xlsxio_write_getversion.static.o $(LIBPREFIX)xlsxio_write$(LIBEXT)
	$(CC) -o $@ examples/$(@:%$(BINEXT)=%.static.o) $(LIBPREFIX)xlsxio_write$(LIBEXT) $(XLSXIOWRITE_LDFLAGS) $(LDFLAGS)

example_xlsxio_write$(BINEXT): examples/example_xlsxio_write.static.o $(LIBPREFIX)xlsxio_write$(LIBEXT)
	$(CC) -o $@ examples/$(@:%$(BINEXT)=%.static.o) $(LIBPREFIX)xlsxio_write$(LIBEXT) $(XLSXIOWRITE_LDFLAGS) $(LDFLAGS)

example_xlsxio_read$(BINEXT): examples/example_xlsxio_read.static.o $(LIBPREFIX)xlsxio_read$(LIBEXT)
	$(CC) -o $@ examples/$(@:%$(BINEXT)=%.static.o) $(LIBPREFIX)xlsxio_read$(LIBEXT) $(XLSXIOREAD_LDFLAGS) $(LDFLAGS)

example_xlsxio_read_advanced$(BINEXT): examples/example_xlsxio_read_advanced.static.o $(LIBPREFIX)xlsxio_read$(LIBEXT)
	$(CC) -o $@ examples/$(@:%$(BINEXT)=%.static.o) $(LIBPREFIX)xlsxio_read$(LIBEXT) $(XLSXIOREAD_LDFLAGS) $(LDFLAGS)

ifdef WIDE
example_xlsxio_readw$(BINEXT): examples/example_xlsxio_read.wstatic.o $(LIBPREFIX)xlsxio_readw$(LIBEXT)
	$(CC) -o $@ examples/$(@:%w$(BINEXT)=%.wstatic.o) $(LIBPREFIX)xlsxio_readw$(LIBEXT) $(XLSXIOREADW_LDFLAGS) $(LDFLAGS)
endif

tools: $(TOOLS_BIN)

xlsxio_xlsx2csv$(BINEXT): src/xlsxio_xlsx2csv.static.o $(LIBPREFIX)xlsxio_read$(LIBEXT)
	$(CC) -o $@ $< $(LIBPREFIX)xlsxio_read$(LIBEXT) $(XLSXIOREAD_LDFLAGS) $(LDFLAGS)

xlsxio_csv2xlsx$(BINEXT): src/xlsxio_csv2xlsx.static.o $(LIBPREFIX)xlsxio_write$(LIBEXT)
	$(CC) -o $@ $< $(LIBPREFIX)xlsxio_write$(LIBEXT) $(XLSXIOWRITE_LDFLAGS) $(LDFLAGS)

.PHONY: doc
doc:
ifdef DOXYGEN
	$(DOXYGEN) doc/Doxyfile
endif

install: all doc
	$(MKDIR) $(PREFIX)/include $(PREFIX)/lib $(PREFIX)/bin
	$(CP) include/*.h $(PREFIX)/include/
	$(CP) *$(LIBEXT) $(PREFIX)/lib/
ifeq ($(OS),Windows_NT)
	$(CP) *.def $(PREFIX)/lib/
	$(CP) *$(SOEXT) $(PREFIX)/bin/
else
	$(CP) *$(SOEXT) $(PREFIX)/lib/
endif
	$(CP) $(TOOLS_BIN) $(PREFIX)/bin/
ifdef DOXYGEN
	$(CPDIR) doc/man $(PREFIX)/
	$(MKDIR) $(PREFIX)/share/xlsxio
	$(CPDIR) doc/html $(PREFIX)/share/xlsxio/
endif

.PHONY: version
version:
	sed -ne "s/^#define\s*XLSXIO_VERSION_[A-Z]*\s*\([0-9]*\)\s*$$/\1./p" include/xlsxio_version.h | tr -d "\n" | sed -e "s/\.$$//" > version

.PHONY: package
package: version
	tar cfJ xlsxio-$(shell cat version).tar.xz --transform="s?^?xlsxio-$(shell cat version)/?" $(SOURCE_PACKAGE_FILES)

.PHONY: package
binarypackage: version
	$(MAKE) PREFIX=binpkg_$(OSALIAS)_temp install STATICDLL=1 WIDE=1
ifneq ($(OS),Windows_NT)
	tar cfJ "xlsxio-$(shell cat version)-$(OSALIAS).tar.xz" --transform="s?^binpkg_$(OSALIAS)_temp/??" $(COMMON_PACKAGE_FILES) binpkg_$(OSALIAS)_temp/*
else
	rm -f xlsxio-$(shell cat version)-$(OSALIAS).zip
	cp -f $(COMMON_PACKAGE_FILES) binpkg_$(OSALIAS)_temp/
	cd binpkg_$(OSALIAS)_temp && zip -r -9 "../xlsxio-$(shell cat version)-binary-$(OSALIAS).zip" $(COMMON_PACKAGE_FILES) * && cd ..
endif
	rm -rf binpkg_$(OSALIAS)_temp

.PHONY: clean
clean:
	$(RM) lib/*.o examples/*.o src/*.o *$(LIBEXT) *$(SOEXT) $(TOOLS_BIN) $(EXAMPLES_BIN) version xlsxio-*.tar.xz doc/doxygen_sqlite3.db
ifeq ($(OS),Windows_NT)
	$(RM) *.def
endif
	$(RMDIR) doc/html doc/man

