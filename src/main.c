#include "zephyr/logging/log_core.h"
#include "zephyr/sys/printk.h"
#include <app_nvs_lib/app_nvs_lib.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_playground, LOG_LEVEL_DBG);

#define test_int_key1 1
#define test_utf8_key2 2
#define key_not_exist 3

int main(void) {

  app_nvs_lib_init();
  int32_t result1 = app_nvs_read_int32(test_int_key1);

  LOG_DBG("Result int 1: result=%d", result1);
  result1++;

  app_nvs_write_int32(test_int_key1, result1);

  char buf[64];
  char readbuf[128];

  snprintk(buf, sizeof(buf), "NO: %d", result1);

  int rs = app_nvs_read_utf8(test_utf8_key2, readbuf, sizeof(readbuf));
  readbuf[rs] = 0;
  LOG_DBG("Read UTF8: %s", readbuf);

  app_nvs_write_utf8(test_utf8_key2, buf, strlen(buf));

  rs = app_nvs_read_utf8(key_not_exist, readbuf, sizeof(readbuf));

  LOG_DBG("Read result for key_not_exist: rs=%d, errmsg=%s", rs,
          strerror(abs(rs)));

  while (1) {
    k_msleep(1000);
  }
  return 0;
}
