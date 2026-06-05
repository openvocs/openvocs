#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

int main(int argc, char **argv) {

  if (argc != 2) {
    return 1;
  }

  if((strlen(argv[1]) == strlen("--keep-running")) &&
    (0 == strcmp(argv[1], "--keep-running"))) {

    while(true) {sleep(1);};
    _exit(1);

  }

  FILE *f = fopen(argv[1], "w");
  if (!f) {
    return 2;
  }

  fprintf(f, "%d\n%s\n", getpid(), argv[0]);
  fclose(f);

  sleep(5);

  return 0;
}
