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
#ifdef _WIN32
#include <fcntl.h>      //O_BINARY
#endif
#include "xlsxio_write.h"
#include "xlsxio_version.h"

#define BUFFER_SIZE 256
#define DEFAULT_DETECTION_ROWS 20

int append_buffer_data (char** pdata, size_t* pdatalen, const char* bufferdata, size_t bufferdatalen)
{
  //allocate larger data buffer, abort in case of memory allocation error
  char *temp = (char*)realloc(*pdata, *pdatalen + bufferdatalen + 1);
  if (temp == NULL)
    return 1;
  *pdata = temp;
  //append new data and adjust length
  memcpy(*pdata + *pdatalen, bufferdata, bufferdatalen);
  *pdatalen += bufferdatalen;
  return 0;
}

void show_help ()
{
  printf(
    "Usage:  xlsxio_csv2xlsx [-h] [-s separator] [-d rows] [-n] csvfile ...\n"
    "Parameters:\n"
    "  -h          \tdisplay command line help\n"
    "  -s separator\tspecify separator to use (default is comma)\n"
    "  -d rows     \trows used for column width detection (default is %i)\n"
    "  -t          \ttread top row as header row\n"
    "  csvfile     \tpath to CSV file (multiple may be specified)\n"
    "Description:\n"
    "Converts all specified CSV (Comma Separated Values) files to .xlsx files.\n"
    "Version: " XLSXIO_VERSION_STRING "\n"
    "\n", DEFAULT_DETECTION_ROWS
  );
}

int main (int argc, char* argv[])
{
  int i;
  char* param;
  FILE* src;
  char separator = ',';
  char quote = '"';
  int toprowheaders = 0;
  xlsxiowriter xlsxiowrite;
  char* sheetname;
  char* filename;
  size_t xlsxdetectionrows = DEFAULT_DETECTION_ROWS;
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
            separator = param[0];
          continue;
        case 'd' :
          if (argv[i][2])
            param = argv[i] + 2;
          else if (i + 1 < argc && argv[i + 1])
            param = argv[++i];
          else
            param = NULL;
          if (param)
            xlsxdetectionrows = strtol(param, (char**)NULL, 10);
          continue;
        case 't' :
          toprowheaders = 1;
          continue;
      }
    }
    //open CSV file
    sheetname = NULL;
    filename = NULL;
    src = NULL;
    if (strcmp(argv[i], "-") == 0) {
      src = stdin;
#ifdef _WIN32
      setmode(fileno(stdin), O_BINARY);
#endif
      sheetname = strdup("Sheet1");
      filename = strdup("data.xlsx");
    } else if ((src = fopen(argv[i], "rb")) == NULL) {
      fprintf(stderr, "Error opening file: %s\n", argv[i]);
    } else {
      //determine sheetname
      if ((sheetname = strdup(argv[i])) != NULL) {
        char* p;
        int i;
        //strip extension
        if ((p = strrchr(sheetname, '.')) != NULL)
          *p = 0;
        //strip path
        i = strlen(sheetname);
        while (i-- > 0) {
          if (sheetname[i] == '/'
#ifdef _WIN32
            || sheetname[i] == '\\'
#endif
          ) {
            memmove(sheetname, sheetname + i + 1, strlen(sheetname) - i);
            break;
          }
        }
      }
      //determine output filename
      if ((filename = (char*)malloc(strlen(argv[i]) + 6)) == NULL ){
        fprintf(stderr, "Memory allocation error\n");
      } else {
        strcpy(filename, argv[i]);
        strcat(filename, ".xlsx");
      }
    }
    if (src && filename && sheetname) {
      //create .xslx file
      if ((xlsxiowrite = xlsxiowrite_open(filename, sheetname)) != NULL) {
        xlsxiowrite_set_detection_rows(xlsxiowrite, xlsxdetectionrows);
        xlsxiowrite_set_row_height(xlsxiowrite, 1);
        //skip UTF-8 BOM header if present
        unsigned char bom[3];
        if (!(fread(bom, 1, 3, src) == 3 && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)) {
          fseek(src, 0, SEEK_SET);
        }
        //process CSV file
        char buf[BUFFER_SIZE];
        size_t buflen = 0;
        size_t bufstart = 0;
        size_t bufpos = 0;
        char* value = NULL;
        size_t valuelen = 0;
        size_t unquoted = 1;
        char lastchar = 0;
        int toprow = 1;
        //read data one buffer at a tome
        while ((buflen = fread(buf, 1, BUFFER_SIZE, src)) > 0) {
          //process each character in buffer
          while (bufpos < buflen) {
            if (unquoted) {
              if (buf[bufpos] == separator || buf[bufpos] == '\n') {
                //process end of cell
                append_buffer_data(&value, &valuelen, buf + bufstart, bufpos - bufstart);
                bufstart = bufpos + 1;
                //detect CRLF line breaks and skip CR
                if (buf[bufpos] == '\n' && valuelen > 0 && value[valuelen - 1] == '\r')
                  valuelen--;
                //write cell value
                if (value)
                  value[valuelen] = 0;
                if (toprow && toprowheaders)
                  xlsxiowrite_add_column(xlsxiowrite, value, 0);
                else
                  xlsxiowrite_add_cell_string(xlsxiowrite, value);
                //clean up cell value
                free(value);
                value = NULL;
                valuelen = 0;
                //process end of line
                if (buf[bufpos] == '\n') {
                  xlsxiowrite_next_row(xlsxiowrite);
                  toprow = 0;
                }
              } else if (buf[bufpos] == quote) {
                //process upen quote
                append_buffer_data(&value, &valuelen, buf + bufstart, bufpos - bufstart + (lastchar == quote ? 1 : 0));
                bufstart = bufpos + 1;
                unquoted = 0;
              }
            } else if (buf[bufpos] == quote) {
              //process close quote
              append_buffer_data(&value, &valuelen, buf + bufstart, bufpos - bufstart);
              bufstart = bufpos + 1;
              unquoted = 1;
            }
            //prepare for processing next character
            lastchar = buf[bufpos];
            bufpos++;
          }
          //dump data at end of buffer and reset counters
          append_buffer_data(&value, &valuelen, buf + bufstart, bufpos - bufstart);
          bufstart = 0;
          bufpos = 0;
        }
        //close .xlsx file
        xlsxiowrite_close(xlsxiowrite);
        //clean up
        free(value);
      }
      //close CSV file
      fclose(src);
    }
    //clean up
    free(filename);
    free(sheetname);
  }
  return 0;
}
