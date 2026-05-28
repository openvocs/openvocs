#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "ov_os.c"
#include <ov_base/ov_error_codes.h>

static int process_exists(pid_t pid) { return kill(pid, 0) == 0; }

void test_spawn_creates_process(void) {

    char const *outfile = "/tmp/spawn_test.pid";

    unlink(outfile);

    char const *args[] = {outfile, NULL};

    int rc = ov_os_spawn(
        "/tmp", OPENVOCS_ROOT "/build/test/ov_os/ov_os_spawn_client_test.run",
        args);

    fprintf(stderr, "Spawn: %i\n", rc);
    assert(rc > 0);

    /*
     * Warten bis Child Datei erzeugt
     */
    for (int i = 0; i < 50; ++i) {
        if (access(outfile, F_OK) == 0) {
            break;
        }

        usleep(100000);
    }

    assert(access(outfile, F_OK) == 0);

    FILE *f = fopen(outfile, "r");
    assert(f != NULL);

    pid_t pid = 0;
    fscanf(f, "%d", &pid);

    fclose(f);

    unlink(outfile);

    assert(pid > 0);

    assert(process_exists(pid));

    kill(pid, SIGTERM);

    // try to execute non-existing binary
    rc = ov_os_spawn("/tmp",
                     OPENVOCS_ROOT "ov_os/ov_os_spawn_client_test.run_run_run_",
                     args);

    fprintf(stderr, "Spawn: %i\n", rc);
    assert(rc == -OV_ERROR_CODE_NOT_FOUND_ERROR);
}

int main() { test_spawn_creates_process(); }
