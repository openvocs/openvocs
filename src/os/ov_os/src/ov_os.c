#include "../include/ov_os.h"

#include <ov_arch/ov_arch.h>
#if OV_ARCH == OV_LINUX

#include <ov_os_linux/ov_os_linux.h>

int ov_os_spawn(char const *workdir, char const *binary,
                char const *const *arguments) {
    return ov_os_linux_spawn(workdir, binary, arguments);
}

#else

int ov_os_spawn(char const *workdir, char const *binary,
                char const *const *arguments) {
    return -1;
}

#endif
