#include <stdlib.h>
#include <stdio.h>
#include "xlsxio_write.h"

int main (int argc, char* argv[])
{
  xlsxiowriter handle;
  if (argc <= 1)
    return 0;
  unlink(argv[1]);
  if ((handle = xlsxiowrite_open(argv[1], NULL)) == NULL) {
    fprintf(stderr, "Error creating zip file\n");
    return 1;
  }
  //write data
  int i;
  for (i = 0; i < 1000; i++) {
    xlsxiowrite_add_cell_string(handle, "Test");
    xlsxiowrite_add_cell_string(handle, NULL);
    xlsxiowrite_add_cell_string(handle, "1");
    xlsxiowrite_add_cell_string(handle, "2");
    xlsxiowrite_add_cell_string(handle, "3");
    xlsxiowrite_next_row(handle);
  }
  //close data
  xlsxiowrite_close(handle);
  return 0;
}

/*
  TO DO:
  - htmlencode cell values
  - value types: integer, floating point, date/time
  - alignment
  - layout
  - header functions
*/
