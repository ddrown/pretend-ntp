#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

void perror_exit(const char *msg) {
  perror(msg);
  exit(1);
}


