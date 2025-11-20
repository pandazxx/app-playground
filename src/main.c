#include "zephyr/logging/log_core.h"
#include <app_nvs_lib/app_nvs_lib.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_playground, LOG_LEVEL_DBG);

#define test_int_key1 1
#define test_utf8_key2 2

int main(void) {

  app_nvs_lib_init();
  int32_t result1 = app_nvs_read_int32(test_int_key1);

  LOG_DBG("Result int 1: result=%d", result1);
  result1++;

  app_nvs_write_int32(test_int_key1, result1);

  while (1) {
    k_msleep(1000);
  }
  return 0;
}
