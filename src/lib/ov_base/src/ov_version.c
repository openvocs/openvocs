#include "ov_version.h"
#include <string.h>
/*----------------------------------------------------------------------------*/
static char g_additional_info[1000] = {0};
/*----------------------------------------------------------------------------*/
char const *ov_version_additional_info() { return g_additional_info; }
/*----------------------------------------------------------------------------*/
bool ov_version_set_additional_info(char const *additional_info) {

  if (0 == additional_info) {
    return false;
  } else {
    size_t str_length = strnlen(additional_info, sizeof(g_additional_info));
    if (str_length > sizeof(g_additional_info) - 1) {
      return false;
    } else {
      strcpy(g_additional_info, additional_info);
      return true;
    }
  }
}
/*----------------------------------------------------------------------------*/
