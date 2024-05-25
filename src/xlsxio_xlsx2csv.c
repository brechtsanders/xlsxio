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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xlsxio_read.h"
#include "xlsxio_version.h"

struct xlsx_data {
  xlsxioreader xlsxioread;
  FILE* dst;
  int nobom;
  int sheetnumbers;
  const char* newline;
  char separator;
  char quote;
  const char* filename;
  unsigned int sheetcount;
};

int sheet_row_callback (size_t row, size_t maxcol, void* callbackdata)
{
  struct xlsx_data* data = (struct xlsx_data*)callbackdata;
  fprintf(data->dst, "%s", data->newline);
  return 0;
}

int sheet_cell_callback (size_t row, size_t col, const XLSXIOCHAR* value, void* callbackdata)
{
  struct xlsx_data* data = (struct xlsx_data*)callbackdata;
  if (col > 1)
    fprintf(data->dst, "%c", data->separator);
  if (value) {
    //print quoted value?
    if (data->quote && (strchr(value, data->quote) || strchr(value, data->separator) || strchr(value, '\n') || strchr(value, '\r'))) {
      const char* p = value;
      fprintf(data->dst, "%c", data->quote);
      while (*p) {
        //duplicate quote character
        if (*p == data->quote)
          fprintf(data->dst, "%c%c", *p, *p);
        //otherwise just print character
        else
          fprintf(data->dst, "%c", *p);
        p++;
      }
      fprintf(data->dst, "%c", data->quote);
    } else {
      fprintf(data->dst, "%s", value);
    }
  }
  return 0;
}

int xlsx_list_sheets_callback (const char* name, void* callbackdata)
{
  char* filename;
  struct xlsx_data* data = (struct xlsx_data*)callbackdata;
  size_t filenamelen = strlen(data->filename) + (data->sheetnumbers ? 16 : strlen(name)) + 6;
  data->sheetcount++;
  //determine output file
  if ((filename = (char*)malloc(filenamelen)) == NULL) {
    fprintf(stderr, "Memory allocation error\n");
  } else {
    //determine export filename
    if (data->sheetnumbers)
      snprintf(filename, filenamelen, "%s.%u.csv", data->filename, data->sheetcount);
    else
      snprintf(filename, filenamelen, "%s.%s.csv", data->filename, name);
    //display status
    printf("Sheet found: %s, exporting to: %s\n", name, filename);
    //open output file
    if ((data->dst = fopen(filename, "wb")) == NULL) {
      fprintf(stderr, "Error creating output file: %s\n", filename);
    } else {
      //write UTF-8 BOM header
      if (!data->nobom)
        fprintf(data->dst, "\xEF\xBB\xBF");
      //process data
      xlsxioread_process(data->xlsxioread, name, XLSXIOREAD_SKIP_EMPTY_ROWS, sheet_cell_callback, sheet_row_callback, data);
      //close output file
      fclose(data->dst);
    }
    //clean up
    free(filename);
  }
  return 0;
}

void show_help ()
{
  printf(
    "Usage:  xlsxio_xlsx2csv [-h] [-s separator] [-n] xlsxfile ...\n"
    "Parameters:\n"
    "  -h          \tdisplay command line help\n"
    "  -s separator\tspecify separator to use (default is comma)\n"
    "  -b          \tdon't write UTF-8 BOM signature\n"
    "  -n          \tuse UNIX style line breaks\n"
    "  -u          \tuse sheet number in output file name (instead of sheet name)\n"
    "  xlsxfile    \tpath to .xlsx file (multiple may be specified)\n"
    "Description:\n"
    "Converts all sheets in all specified .xlsx files to individual CSV (Comma Separated Values) files.\n"
    "Version: " XLSXIO_VERSION_STRING "\n"
    "\n"
  );
}

int main (int argc, char* argv[])
{
  int i;
  char* param;
  xlsxioreader xlsxioread;
  struct xlsx_data sheetdata = {
    .nobom = 0,
    .sheetnumbers = 0,
    .newline = "\r\n",
    .separator = ',',
    .quote = '"',
  };
  //process command line parameters
  if (argc == 1) {
    show_help();
    return 0;
  }
  for (i = 1; i < argc; i++) {
    //check for command line parameters
    if (argv[i][0] && (argv[i][0] == '/' || argv[i][0] == '-')) {
      switch (tolower(argv[i][1])) {
        case 'h' :
        case '?' :
          show_help();
          return 0;
        case 's' :
          if (argv[i][2])
            param = argv[i] + 2;
          else if (i + 1 < argc && argv[i + 1])
            param = argv[++i];
          else
            param = NULL;
          if (param)
            sheetdata.separator = param[0];
          continue;
        case 'b' :
          sheetdata.nobom = 1;
          continue;
        case 'u' :
          sheetdata.sheetnumbers = 1;
          continue;
        case 'n' :
          sheetdata.newline = "\n";
          continue;
      }
    }
    //open .xlsx file
    if ((xlsxioread = xlsxioread_open(argv[i])) != NULL) {
      sheetdata.xlsxioread = xlsxioread;
      sheetdata.filename = argv[i];
      sheetdata.sheetcount = 0;
      //iterate through available sheets
      printf("Processing file: %s\n", argv[i]);
      xlsxioread_list_sheets(xlsxioread, xlsx_list_sheets_callback, &sheetdata);
      //close .xlsx file
      xlsxioread_close(xlsxioread);
    }
  }
  return 0;
}
