

# Build

```bash
west build -b esp32s3_devkitc/esp32s3/procpu  . -DZEPHYR_EXTRA_MODULES="$(pwd)/../app-nvs-lib;$(pwd)/../app-bt-lib;$(pwd)/../app-wifi-lib;$(pwd)/../app-mqtt-lib;$(pwd)/../app-dns-lib;$(pwd)/../app-poll-lib"

west build -b esp32c3_supermini  . -DZEPHYR_EXTRA_MODULES="$(pwd)/../app-nvs-lib;$(pwd)/../app-bt-lib;$(pwd)/../app-wifi-lib;$(pwd)/../app-mqtt-lib;$(pwd)/../app-dns-lib;$(pwd)/../app-poll-lib"


```
esp32c3_supermini
