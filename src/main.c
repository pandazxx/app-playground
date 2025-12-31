#include "zephyr/logging/log.h"
#include "zephyr/logging/log_core.h"
#include "zephyr/net/mqtt.h"
#include "zephyr/sys/printk.h"
#include <app_dns_lib/app_dns_lib.h>
#include <app_mqtt_lib/app_mqtt_lib.h>
#include <app_poll_lib/app_poll_lib.h>
#include <app_wifi_lib/app_wifi_lib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(app_playground, LOG_LEVEL_DBG);
static app_dns_record test_dns_record = {.resolved = false,
                                         .host_name = "hcs01.local"};

static struct mqtt_topic ina3221_topic = {
    .topic = MQTT_UTF8_LITERAL("v1/sensors/ina3221/state"),
    .qos = MQTT_QOS_1_AT_LEAST_ONCE,
};

#define SENSOR_ATTR_INA3221_SELECTED_CHANNEL (SENSOR_ATTR_PRIV_START + 1)

struct ina3221_channel_meta {
  const struct device *dev;
  uint8_t channel;
  const char *tag;
};
// static const struct device *const ina1 = DEVICE_DT_GET(DT_NODELABEL(ina1));

#define INA3221_CHANNEL_TAG(node_id)                                           \
  DT_PROP_OR(node_id, app_tag,                                                 \
             DT_PROP_OR(node_id, app_description,                              \
                        DT_PROP_OR(node_id, label, "channel")))

#define INA3221_CHANNEL_ENTRY(node_id)                                         \
  IF_ENABLED(DT_NODE_HAS_STATUS(DT_PARENT(node_id), okay),                     \
             ({                                                                \
                  .dev = DEVICE_DT_GET(DT_PARENT(node_id)),                    \
                  .channel = DT_REG_ADDR(node_id),                             \
                  .tag = INA3221_CHANNEL_TAG(node_id),                         \
              }, ))

static const struct ina3221_channel_meta ina3221_channels[] = {
#if DT_HAS_COMPAT_STATUS_OKAY(app_ina3221channel)
    DT_FOREACH_STATUS_OKAY(app_ina3221channel, INA3221_CHANNEL_ENTRY)
#endif
};

// static int32_t sensor_value_to_milli(const struct sensor_value *val) {
//   /* Convert sensor_value (val1 + val2/1e6) to milli-units without floats. */
//   return (int32_t)(val->val1 * 1000) + (int32_t)(val->val2 / 1000);
// }

static int ina3221_read_channel(const struct device *dev, uint8_t channel,

                                struct sensor_value *vbus,
                                struct sensor_value *icur) {
  struct sensor_value idx = {.val1 = channel + 1, .val2 = 0};

  int ret = sensor_attr_set(
      dev, SENSOR_CHAN_ALL,
      (enum sensor_attribute)SENSOR_ATTR_INA3221_SELECTED_CHANNEL, &idx);
  if (ret != 0) {
    return ret;
  }

  ret = sensor_sample_fetch(dev);
  if (ret != 0) {
    return ret;
  }

  ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, vbus);
  if (ret != 0) {
    return ret;
  }

  return sensor_channel_get(dev, SENSOR_CHAN_CURRENT, icur);
}

int main(void) {
  app_wifi_lib_init();
  app_dns_lib_init();
  app_poll_lib_init();

  if (ARRAY_SIZE(ina3221_channels) == 0) {
    LOG_WRN("No INA3221 channels defined in devicetree");
  }
  LOG_ERR("found INA3221 modules: count=%d", ARRAY_SIZE(ina3221_channels));
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

  while (1) {
    app_poll_loop();

    for (size_t idx = 0; idx < ARRAY_SIZE(ina3221_channels); idx++) {
      const struct ina3221_channel_meta *meta = &ina3221_channels[idx];
      // if (!device_is_ready(ina1)) {
      if (!device_is_ready(meta->dev)) {
        LOG_ERR("INA3221 device not ready: %s", meta->dev->name);
        continue;
      }

      struct sensor_value vbus;
      struct sensor_value icur;
      // int rc = ina3221_read_channel(ina1, meta->channel, &vbus, &icur);
      int rc = ina3221_read_channel(meta->dev, meta->channel, &vbus, &icur);
      if (rc != 0) {
        LOG_WRN("INA3221 read failed: dev=%s ch=%u rc=%d", meta->dev->name,
                meta->channel, rc);
        continue;
      }

      int32_t v_bus_mv = sensor_value_to_milli(&vbus);
      int32_t i_bus_ma = sensor_value_to_milli(&icur);
      char mqtt_buf[256];
      int len =
          snprintk(mqtt_buf, sizeof(mqtt_buf),
                   "{\"tag\":\"%s\",\"channel\":%u,\"v_bus_mv\":%d,"
                   "\"i_bus_ma\":%d, \"temp_c\":%d}",
                   meta->tag, meta->channel + 1, v_bus_mv, i_bus_ma, i_bus_ma);
      if (len < 0) {
        continue;
      }
      if (len >= sizeof(mqtt_buf)) {
        len = sizeof(mqtt_buf) - 1;
      }
      LOG_DBG("sensor: %s", mqtt_buf);

      app_mqtt_publish(ina3221_topic, (struct mqtt_binstr){
                                          .data = mqtt_buf,
                                          .len = len,
                                      });
    }

    k_sleep(K_SECONDS(1));
  }
}
