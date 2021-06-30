/*
  Copyright (C) 2021 Buildbotics LLC.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

#include "error.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


void fail(const char *msg, ...) {
  printf("ERROR: ");

  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);

  printf("\n");

  exit(1);
}
