#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {

    if (argc != 2) {
        return 1;
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
