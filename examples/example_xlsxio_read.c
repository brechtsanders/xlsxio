//#define PROCESS_FROM_MEMORY
#define PROCESS_FROM_FILEHANDLE
/*****************************************************************************
Copyright (C)  2016  Brecht Sanders  All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*****************************************************************************/

/**
 * @file example_xlsxio_read.c
 * @brief XLSX I/O example illustrating how to read from an .xlsx file.
 * @author Brecht Sanders
 *
 * This example reads data from an .xslx file using the simple method.
 */

//#define PROCESS_FROM_MEMORY

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if _WIN32
#include <windows.h>
#else
#define O_BINARY 0
#endif
#ifdef PROCESS_FROM_MEMORY
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#ifdef PROCESS_FROM_FILEHANDLE
//#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "xlsxio_read.h"

#if !defined(XML_UNICODE_WCHAR_T) && !defined(XML_UNICODE)
// UTF-8 version
#define X(s) s
#define XML_Char_printf printf
#else
// UTF-16 version
#define X(s) L##s
#define XML_Char_printf wprintf
#endif

const char* filename = "example.xlsx";

int main (int argc, char* argv[])
{
#ifdef _WIN32
  // switch Windows console to UTF-8
  SetConsoleOutputCP(CP_UTF8);
#endif

  if (argc > 1)
    filename = argv[1];

  xlsxioreader xlsxioread;

  XML_Char_printf(X("XLSX I/O library version %s\n"), xlsxioread_get_version_string());

#if defined(PROCESS_FROM_MEMORY)
  {
    // load file in memory
    int filehandle;
    char* buf = NULL;
    size_t buflen = 0;
    if ((filehandle = open(filename, O_RDONLY | O_BINARY)) != -1) {
      struct stat fileinfo;
      if (fstat(filehandle, &fileinfo) == 0) {
        if ((buf = malloc(fileinfo.st_size)) != NULL) {
          if (fileinfo.st_size > 0 && read(filehandle, buf, fileinfo.st_size) == fileinfo.st_size) {
            buflen = fileinfo.st_size;
          }
        }
      }
      close(filehandle);
    }
    if (!buf || buflen == 0) {
      fprintf(stderr, "Error loading .xlsx file\n");
      return 1;
    }
    if ((xlsxioread = xlsxioread_open_memory(buf, buflen, 1)) == NULL) {
      fprintf(stderr, "Error processing .xlsx data\n");
      return 1;
    }
  }
#elif defined(PROCESS_FROM_FILEHANDLE)
  // open .xlsx file for reading
  int filehandle;
  if ((filehandle = open(filename, O_RDONLY | O_BINARY, 0)) == -1) {
    fprintf(stderr, "Error opening .xlsx file\n");
    return 1;
  }
  if ((xlsxioread = xlsxioread_open_filehandle(filehandle)) == NULL) {
    fprintf(stderr, "Error reading .xlsx file\n");
    return 1;
  }
#else
  // open .xlsx file for reading
  if ((xlsxioread = xlsxioread_open(filename)) == NULL) {
    fprintf(stderr, "Error opening .xlsx file\n");
    return 1;
  }
#endif

  // list available sheets
  xlsxioreadersheetlist sheetlist;
  const XLSXIOCHAR* sheetname;
  printf("Available sheets:\n");
  if ((sheetlist = xlsxioread_sheetlist_open(xlsxioread)) != NULL) {
    while ((sheetname = xlsxioread_sheetlist_next(sheetlist)) != NULL) {
      XML_Char_printf(X(" - %s\n"), sheetname);
    }
    xlsxioread_sheetlist_close(sheetlist);
  }
  XML_Char_printf(X("\n"));

  // read values from first sheet
  xlsxioread_cell value;
  const char *typeNames[] = { "none", "value", "boolean", "string", "date" };

  printf("Contents of first sheet:\n");
  xlsxioreadersheet sheet = xlsxioread_sheet_open(xlsxioread, NULL, XLSXIOREAD_SKIP_EMPTY_ROWS);
  while (xlsxioread_sheet_next_row(sheet)) {
    while ((value = xlsxioread_sheet_next_cell_struct(sheet)) != NULL) {
      XML_Char_printf(X("%s\t"), value->data);
      // print with row num, cell num and type
      //XML_Char_printf("cell %zu %zu %s %s %s\n", value->row_num, value->col_num,
      //  typeNames[value->cell_type], value->number_fmt, value->data);
      free(value);
    }
    printf("\n");
  }
  xlsxioread_sheet_close(sheet);

  // clean up
  xlsxioread_close(xlsxioread);
  return 0;
}
