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
 * @file example_xlsxio_read_advanced.c
 * @brief XLSX I/O example illustrating how to read from an .xlsx file.
 * @author Brecht Sanders
 *
 * This example reads data from an .xslx file using the callback method.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "xlsxio_read.h"

const char* filename = "example.xlsx";

//callback data structure for listing sheets
struct xlsx_list_sheets_data {
  char* firstsheet;
};

//callback function for listing sheets
int xlsx_list_sheets_callback (const char* name, void* callbackdata)
{
  struct xlsx_list_sheets_data* data = (struct xlsx_list_sheets_data*)callbackdata;
  if (!data->firstsheet)
    data->firstsheet = strdup(name);
  printf(" - %s\n", name);
  return 0;
}

//callback function for end of row
int sheet_row_callback (size_t row, size_t maxcol, void* callbackdata)
{
  printf("\n");
  return 0;
}

//callback function for cell data
int sheet_cell_callback (size_t row, size_t col, const XLSXIOCHAR* value, void* callbackdata)
{
  if (col > 1)
    printf("\t");
  if (value)
    printf("%s", value);
  return 0;
}

int main (int argc, char* argv[])
{
  xlsxioreader xlsxioread;
  //open .xlsx file for reading
  if ((xlsxioread = xlsxioread_open(filename)) == NULL) {
    fprintf(stderr, "Error opening .xlsx file\n");
    return 1;
  }
  //list available sheets
  struct xlsx_list_sheets_data sheetdata;
  sheetdata.firstsheet = NULL;
  printf("Available sheets:\n");
  xlsxioread_list_sheets(xlsxioread, xlsx_list_sheets_callback, &sheetdata);

  //read data
  xlsxioread_process(xlsxioread, sheetdata.firstsheet, XLSXIOREAD_SKIP_EMPTY_ROWS, sheet_cell_callback, sheet_row_callback, NULL);

  //clean up
  free(sheetdata.firstsheet);
  xlsxioread_close(xlsxioread);
  return 0;
}
