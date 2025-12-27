#include "zephyr/logging/log.h"
#include "zephyr/logging/log_core.h"
#include "zephyr/net/mqtt.h"
#include "zephyr/sys/printk.h"
#include <app_dns_lib/app_dns_lib.h>
#include <app_mqtt_lib/app_mqtt_lib.h>
#include <app_poll_lib/app_poll_lib.h>
#include <app_wifi_lib/app_wifi_lib.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(app_playground, LOG_LEVEL_DBG);
static app_dns_record test_dns_record = {.resolved = false,
                                         .host_name = "hcs01.local"};

static struct mqtt_topic test_topic1 = {
    .topic = MQTT_UTF8_LITERAL("sensor/test"),
    .qos = MQTT_QOS_1_AT_LEAST_ONCE,
};

int main(void) {
  app_wifi_lib_init();
  app_dns_lib_init();
  app_poll_lib_init();

  int result = app_wifi_lib_do_something(21);
  printk("Result = %d\n", result);
  wait_for_network();
  printk("done waiting network\n");
  // k_sleep(K_SECONDS(10));
  struct sockaddr_in mqtt_addr;
  memset(&mqtt_addr, 0, sizeof(mqtt_addr));

  mqtt_addr.sin_family = AF_INET;
  mqtt_addr.sin_port = htons(1883); // port
  inet_pton(AF_INET, "10.10.10.123", &mqtt_addr.sin_addr);

  app_mqtt_lib_init("just for test", &mqtt_addr);
  LOG_DBG("waiting for mqtt");

  // wait_for_mqtt();
  // LOG_DBG("queryinng dns");
  // app_dns_query(&test_dns_record);
  //
  // k_sleep(K_SECONDS(10));
  //
  // LOG_DBG("queryinng dns");
  // app_dns_query(&test_dns_record);
  //
  // app_poll_start();
  int test_topic_value = 0;
  while (1) {
    app_poll_loop();
    uint8_t mqtt_buf[256];

    snprintk(mqtt_buf, sizeof(mqtt_buf), "test_topic1:  %d",
             test_topic_value++);
    app_mqtt_publish(test_topic1, (struct mqtt_binstr){
                                      .data = mqtt_buf,
                                      .len = strlen(mqtt_buf),
                                  });
    k_sleep(K_SECONDS(1));
  }
}
