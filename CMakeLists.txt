CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(xlsxio DESCRIPTION "C libraries for reading and writing .xlsx files")

# get version number
FILE(READ "include/xlsxio_version.h" VERFILE)
STRING(REGEX MATCH "#define XLSXIO_VERSION_MAJOR ([0-9]*)" DUMMYVAR ${VERFILE})
SET(XLSXIO_VER_MAJOR ${CMAKE_MATCH_1})
STRING(REGEX MATCH "#define XLSXIO_VERSION_MINOR ([0-9]*)" DUMMYVAR ${VERFILE})
SET(XLSXIO_VER_MINOR ${CMAKE_MATCH_1})
STRING(REGEX MATCH "#define XLSXIO_VERSION_MICRO ([0-9]*)" DUMMYVAR ${VERFILE})
SET(XLSXIO_VER_MICRO ${CMAKE_MATCH_1})
SET(XLSXIO_VER "${XLSXIO_VER_MAJOR}.${XLSXIO_VER_MINOR}.${XLSXIO_VER_MICRO}")
UNSET(DUMMYVAR)
UNSET(VERFILE)
MESSAGE(STATUS "XLSX I/O library version: ${XLSXIO_VER}")

# parameters
OPTION(BUILD_STATIC "Build static libraries" ON)
OPTION(BUILD_SHARED "Build shared libraries" ON)
OPTION(BUILD_PC_FILES "Build pkg-config .pc" ON)
OPTION(BUILD_TOOLS "Build tools" ON)
OPTION(BUILD_EXAMPLES "Build examples" ON)
OPTION(WITH_LIBZIP "Use libzip instead of Minizip" OFF)
OPTION(WITH_MINIZIP_NG "Use Minizip NG" OFF)
OPTION(MINIZIP_NG_SUFFIX "Minizip NG suffix (default is empty)" "")
OPTION(WITH_WIDE "Also build UTF-16 library (libxlsxio_readw)" OFF)
SET(ZLIB_DIR "" CACHE PATH "Path to the zlib library")
IF(WITH_LIBZIP)
  SET(LIBZIP_DIR "" CACHE PATH "Path to the libzip library")
ELSE()
  SET(MINIZIP_DIR "" CACHE PATH "Path to the Minizip library")
ENDIF()
SET(EXPAT_DIR "" CACHE PATH "Path to the Expat library")

# check conditions
IF(NOT BUILD_STATIC AND NOT BUILD_SHARED)
  MESSAGE(FATAL_ERROR "Cannot build with both BUILD_STATIC and BUILD_SHARED disabled")
ENDIF()

# dependancies
SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
#   dependancy: zlib (dependancy of libzip/Minizip, should be detected there)
IF(ZLIB_DIR)
  SET(ZLIB_ROOT ${ZLIB_DIR})
ENDIF()
#FIND_PACKAGE(ZLIB REQUIRED)
#   dependancy: libzip/minizip/minizip-ng
IF(WITH_LIBZIP)
  FIND_PACKAGE(LibZip REQUIRED)
  SET(ANYZIP_INCLUDE_DIRS ${LIBZIP_INCLUDE_DIRS})
  SET(ANYZIP_LIBRARIES ${LIBZIP_LIBRARIES})
  SET(ANYZIP_DEF USE_LIBZIP)
ELSEIF(WITH_MINIZIP_NG)
  FIND_PACKAGE(minizip${MINIZIP_NG_SUFFIX} REQUIRED)
  SET(ANYZIP_INCLUDE_DIRS "${minizip-ng_DIR}/../../../include/minizip${MINIZIP_NG_SUFFIX}")
  SET(ANYZIP_LIBRARIES minizip${MINIZIP_NG_SUFFIX})
  SET(ANYZIP_DEF USE_MINIZIP;USE_MINIZIP_NG)
ELSE()
  FIND_PACKAGE(Minizip REQUIRED)
  SET(ANYZIP_INCLUDE_DIRS ${MINIZIP_INCLUDE_DIRS})
  SET(ANYZIP_LIBRARIES ${MINIZIP_LIBRARIES})
  SET(ANYZIP_DEF USE_MINIZIP)
ENDIF()
#   dependancy: expat
IF(EXPAT_DIR)
  #FIND_PACKAGE(EXPAT MODULE REQUIRED NO_DEFAULT_PATH PATHS ${EXPAT_DIR})
  FIND_PATH(EXPAT_INCLUDE_DIR NAMES expat.h NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/include ${EXPAT_DIR})
  FIND_LIBRARY(EXPAT_LIBRARIES NAMES expat libexpat NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/lib ${EXPAT_DIR})
ELSE()
  FIND_PACKAGE(EXPAT REQUIRED)
ENDIF()
#   dependancy: expatw (if wide library was requested)
IF(WITH_WIDE)
  IF(EXPAT_DIR)
    FIND_LIBRARY(EXPATW_LIBRARIES NAMES expatw libexpatw NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/lib ${EXPAT_DIR})
  ELSE()
    FIND_LIBRARY(EXPATW_LIBRARIES NAMES expatw)
  ENDIF()
ENDIF()

# Doxygen
FIND_PACKAGE(Doxygen)
OPTION(BUILD_DOCUMENTATION "Create and install API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

# build parameters
#SET(CMAKE_CXX_FLAGS "-g -Wall")
#ADD_SUBDIRECTORY(src)

IF(NOT WIN32 AND NOT APPLE)
  SET(THREADLIB "-pthread")
ENDIF()

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(${ANYZIP_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${EXPAT_INCLUDE_DIRS})

# build definitions
SET(ALLTARGETS_LIB)
SET(ALLTARGETS_EXE)
SET(LINKTYPES)
IF(BUILD_STATIC)
  LIST(APPEND LINKTYPES "STATIC")
ENDIF()
IF(BUILD_SHARED)
  LIST(APPEND LINKTYPES "SHARED")
ENDIF()

FOREACH(LINKTYPE ${LINKTYPES})
  ADD_LIBRARY(xlsxio_read_${LINKTYPE} ${LINKTYPE} lib/xlsxio_read.c lib/xlsxio_read_sharedstrings.c)
  SET_TARGET_PROPERTIES(xlsxio_read_${LINKTYPE} PROPERTIES COMPILE_DEFINITIONS "BUILD_XLSXIO;${ANYZIP_DEF};BUILD_XLSXIO_${LINKTYPE}")
  SET_TARGET_PROPERTIES(xlsxio_read_${LINKTYPE} PROPERTIES OUTPUT_NAME xlsxio_read)
  IF(MINGW AND LINKTYPE STREQUAL "SHARED")
    SET_TARGET_PROPERTIES(xlsxio_read_${LINKTYPE} PROPERTIES LINK_FLAGS "-Wl,--output-def=libxlsxio_read.def")
    SET_SOURCE_FILES_PROPERTIES(xlsxio_read.def PROPERTIES HEADER_FILE_ONLY TRUE)
  ENDIF()
  TARGET_INCLUDE_DIRECTORIES(xlsxio_read_${LINKTYPE} PRIVATE lib)
  TARGET_LINK_LIBRARIES(xlsxio_read_${LINKTYPE} ${ANYZIP_LIBRARIES} ${EXPAT_LIBRARIES})
  SET(ALLTARGETS_LIB ${ALLTARGETS_LIB} xlsxio_read_${LINKTYPE})

  ADD_LIBRARY(xlsxio_write_${LINKTYPE} ${LINKTYPE} lib/xlsxio_write.c)
  SET_TARGET_PROPERTIES(xlsxio_write_${LINKTYPE} PROPERTIES COMPILE_DEFINITIONS "BUILD_XLSXIO;${ANYZIP_DEF};BUILD_XLSXIO_${LINKTYPE}")
  SET_TARGET_PROPERTIES(xlsxio_write_${LINKTYPE} PROPERTIES OUTPUT_NAME xlsxio_write)
  IF(MINGW AND LINKTYPE STREQUAL "SHARED")
    SET_TARGET_PROPERTIES(xlsxio_write_${LINKTYPE} PROPERTIES LINK_FLAGS "-Wl,--output-def=libxlsxio_write.def")
  ENDIF()
  TARGET_LINK_LIBRARIES(xlsxio_write_${LINKTYPE} ${ANYZIP_LIBRARIES} ${THREADLIB})
  SET(ALLTARGETS_LIB ${ALLTARGETS_LIB} xlsxio_write_${LINKTYPE})

  IF(WITH_WIDE)
    ADD_LIBRARY(xlsxio_readw_${LINKTYPE} ${LINKTYPE} lib/xlsxio_read.c lib/xlsxio_read_sharedstrings.c)
    SET_TARGET_PROPERTIES(xlsxio_readw_${LINKTYPE} PROPERTIES DEFINE_SYMBOL "BUILD_XLSXIO_DLL")
    SET_TARGET_PROPERTIES(xlsxio_readw_${LINKTYPE} PROPERTIES COMPILE_DEFINITIONS "XML_UNICODE;BUILD_XLSXIO;${ANYZIP_DEF}")
    SET_TARGET_PROPERTIES(xlsxio_readw_${LINKTYPE} PROPERTIES OUTPUT_NAME xlsxio_readw)
    IF(MINGW AND LINKTYPE STREQUAL "SHARED")
      SET_TARGET_PROPERTIES(xlsxio_readw_${LINKTYPE} PROPERTIES LINK_FLAGS "-Wl,--output-def=libxlsxio_readw.def")
    ENDIF()
    TARGET_INCLUDE_DIRECTORIES(xlsxio_readw_${LINKTYPE} PRIVATE lib)
    TARGET_LINK_LIBRARIES(xlsxio_readw_${LINKTYPE} ${ANYZIP_LIBRARIES} ${EXPATW_LIBRARIES})
    SET(ALLTARGETS_LIB ${ALLTARGETS_LIB} xlsxio_readw_${LINKTYPE})

    #ADD_LIBRARY(xlsxio_writew_${LINKTYPE} ${LINKTYPE} lib/xlsxio_write.c)
    #SET_TARGET_PROPERTIES(xlsxio_writew_${LINKTYPE} PROPERTIES DEFINE_SYMBOL "BUILD_XLSXIO_DLL")
    #SET_TARGET_PROPERTIES(xlsxio_writew_${LINKTYPE} PROPERTIES COMPILE_DEFINITIONS "XML_UNICODE;BUILD_XLSXIO;${ANYZIP_DEF}")
    #SET_TARGET_PROPERTIES(xlsxio_writew_${LINKTYPE} PROPERTIES OUTPUT_NAME xlsxio_write_w)
    #IF(MINGW AND LINKTYPE STREQUAL "SHARED")
    #  SET_TARGET_PROPERTIES(xlsxio_writew_${LINKTYPE} PROPERTIES LINK_FLAGS "-Wl,--output-def=libxlsxio_writew.def")
    #ENDIF()
    #TARGET_LINK_LIBRARIES(xlsxio_writew_${LINKTYPE} ${ANYZIP_LIBRARIES} ${THREADLIB})
    #SET(ALLTARGETS_LIB ${ALLTARGETS_LIB} xlsxio_writew_${LINKTYPE})
  ENDIF()

  SET(EXELINKTYPE ${LINKTYPE})
ENDFOREACH()
#SET_TARGET_PROPERTIES(xlsxio_read_SHARED PROPERTIES DEFINE_SYMBOL "BUILD_XLSXIO_DLL")
#SET_TARGET_PROPERTIES(xlsxio_write_SHARED PROPERTIES DEFINE_SYMBOL "BUILD_XLSXIO_DLL")

IF(BUILD_TOOLS)
  ADD_EXECUTABLE(xlsxio_xlsx2csv src/xlsxio_xlsx2csv.c)
  TARGET_LINK_LIBRARIES(xlsxio_xlsx2csv xlsxio_read_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} xlsxio_xlsx2csv)

  ADD_EXECUTABLE(xlsxio_csv2xlsx src/xlsxio_csv2xlsx.c)
  TARGET_LINK_LIBRARIES(xlsxio_csv2xlsx xlsxio_write_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} xlsxio_csv2xlsx)
ENDIF()

IF(BUILD_EXAMPLES)
  ADD_EXECUTABLE(example_xlsxio_write_getversion examples/example_xlsxio_write_getversion.c)
  TARGET_LINK_LIBRARIES(example_xlsxio_write_getversion xlsxio_write_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} example_xlsxio_write_getversion)

  ADD_EXECUTABLE(example_xlsxio_write examples/example_xlsxio_write.c)
  TARGET_LINK_LIBRARIES(example_xlsxio_write xlsxio_write_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} example_xlsxio_write)

  ADD_EXECUTABLE(example_xlsxio_read examples/example_xlsxio_read.c)
  TARGET_LINK_LIBRARIES(example_xlsxio_read xlsxio_read_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} example_xlsxio_read)

  ADD_EXECUTABLE(example_xlsxio_read_advanced examples/example_xlsxio_read_advanced.c)
  TARGET_LINK_LIBRARIES(example_xlsxio_read_advanced xlsxio_read_${EXELINKTYPE})
  SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} example_xlsxio_read_advanced)

  IF(WITH_WIDE)
    ADD_EXECUTABLE(example_xlsxio_readw examples/example_xlsxio_read.c)
    SET_TARGET_PROPERTIES(example_xlsxio_readw PROPERTIES COMPILE_DEFINITIONS "XML_UNICODE")
    TARGET_LINK_LIBRARIES(example_xlsxio_readw xlsxio_readw_${EXELINKTYPE})
    SET(ALLTARGETS_EXE ${ALLTARGETS_EXE} example_xlsxio_readw)
  ENDIF()
ENDIF()

IF(BUILD_DOCUMENTATION)
  IF(NOT DOXYGEN_FOUND)
    MESSAGE(FATAL_ERROR "Doxygen is needed to build the documentation.")
  ENDIF()
  ADD_CUSTOM_TARGET(doc ALL
    COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile
    #WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
  INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/man
    DESTINATION .
  )
  INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/html
    DESTINATION share/xlsxio
  )
ENDIF()

# installation specifications
INSTALL(TARGETS ${ALLTARGETS_LIB}
  EXPORT  xlsxioTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
INSTALL(EXPORT xlsxioTargets
  NAMESPACE xlsxio::
  DESTINATION cmake
)
FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/xlsxio-config.cmake.in"
[[INCLUDE(CMakeFindDependencyMacro)
IF (@WITH_LIBZIP@)
  FIND_DEPENDENCY(LibZip)
ELSE()
  FIND_DEPENDENCY(minizip CONFIG)
ENDIF()
IF (@EXPAT_DIR@)
  FIND_PATH(EXPAT_INCLUDE_DIR NAMES expat.h NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/include ${EXPAT_DIR})
  FIND_LIBRARY(EXPAT_LIBRARIES NAMES expat libexpat NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/lib ${EXPAT_DIR})
ELSE()
  FIND_DEPENDENCY(EXPAT)
ENDIF()

IF(@WITH_WIDE@)
  IF(@EXPAT_DIR@)
    FIND_LIBRARY(EXPATW_LIBRARIES NAMES expatw libexpatw NO_DEFAULT_PATH PATHS ${EXPAT_DIR}/lib ${EXPAT_DIR})
  ELSE()
    FIND_LIBRARY(EXPATW_LIBRARIES NAMES expatw)
  ENDIF()
ENDIF()

INCLUDE("${CMAKE_CURRENT_LIST_DIR}/xlsxioTargets.cmake")
]])
CONFIGURE_FILE("${CMAKE_CURRENT_BINARY_DIR}/xlsxio-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/xlsxio-config.cmake" @ONLY)
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/xlsxio-config.cmake" DESTINATION cmake)

INSTALL(TARGETS ${ALLTARGETS_EXE}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
INSTALL(DIRECTORY include/
  DESTINATION include 
  FILES_MATCHING PATTERN "xlsxio*.h"
)
IF(MINGW AND BUILD_SHARED)
  INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/libxlsxio_read.def" DESTINATION lib)
  INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/libxlsxio_write.def" DESTINATION lib)
  IF(WITH_WIDE)
    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/libxlsxio_readw.def" DESTINATION lib)
    #INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/libxlsxio_writew.def" DESTINATION lib)
  ENDIF()
ENDIF()

# generate .cmake and .pc files
MACRO(generate_and_install_config_files _NAME _DESC _LIBS _HEADER)
  SET(XLSXIO_NAME "${_NAME}")
  SET(XLSXIO_DESC "${_DESC}")
  SET(XLSXIO_HEADER "${_HEADER}")
  STRING(REGEX REPLACE "^lib\([^ ]*\)" "\\1" XLSXIO_LIB "${XLSXIO_NAME}")
  STRING(REPLACE ";" " " XLSXIO_LDFLAGS "-l${XLSXIO_LIB} ${_LIBS}")
  STRING(REGEX REPLACE "\([^ ]*\)[/\\]lib([^ /\\.]*).[^ ]*" "-L\\1 -l\\2" XLSXIO_LDFLAGS "${XLSXIO_LDFLAGS}")
  CONFIGURE_FILE(templateConfig.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}Config.cmake" @ONLY)
  CONFIGURE_FILE(templateConfigVersion.cmake.in "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}ConfigVersion.cmake" @ONLY)
  INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}Config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}ConfigVersion.cmake" DESTINATION cmake)
  IF (BUILD_PC_FILES)
    CONFIGURE_FILE(template.pc.in "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}.pc" @ONLY)
    INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/${XLSXIO_NAME}.pc" DESTINATION lib/pkgconfig)
  ENDIF()
ENDMACRO()
generate_and_install_config_files("libxlsxio_write" "C library for writing .xlsx files" "${ANYZIP_LIBRARIES} ${THREADLIB}" "xlsxio_write.h")
generate_and_install_config_files("libxlsxio_read" "C library for writing .xlsx files" "${ANYZIP_LIBRARIES} ${EXPAT_LIBRARIES}" "xlsxio_read.h")
IF(WITH_WIDE)
  generate_and_install_config_files("libxlsxio_readw" "C library for reading .xlsx files using wide characters" "${ANYZIP_LIBRARIES} ${EXPAT_LIBRARIES}" "xlsxio_read.h")
ENDIF()
