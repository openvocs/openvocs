#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ov_os.c"
#include <ov_base/ov_error_codes.h>

static int process_exists(pid_t pid) { return kill(pid, 0) == 0; }

void test_spawn_creates_process(void) {

  char const *outfile = "/tmp/spawn_test.pid";

  unlink(outfile);

  char const *args[] = {outfile, NULL};

  int rc = ov_os_spawn(
    "/tmp", OPENVOCS_ROOT "/build/test/ov_os/ov_os_spawn_client_test.run",
    args, true);

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
                   args, true);

  fprintf(stderr, "Spawn: %i\n", rc);
  assert(rc == -OV_ERROR_CODE_NOT_FOUND_ERROR);
}


 int spawn_interactively(bool detach_child) {

  char const *args[] = {"--keep-running", NULL};

  return ov_os_spawn("/tmp", OPENVOCS_ROOT "/build/test/ov_os/ov_os_spawn_client_test.run",
                     args, detach_child);

}

int main(int argc, char **argv) { 

  if(argc == 1) {
    test_spawn_creates_process();
    _exit(0);
  }


  spawn_interactively((strlen(argv[1]) == strlen("--detach")) &&
                      (0 == strcmp(argv[1], "--detach")));

  while(true) {sleep(1);}

}

