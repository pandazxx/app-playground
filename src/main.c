#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_playground, LOG_LEVEL_INF);
int main(void) {
  while (1) {
    k_msleep(1000);
  }
  return 0;
}
