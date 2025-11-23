# 🌐 Modular IoT Framework (Zephyr / ESP32 / STM32)

A modular, self-discovering IoT framework for **power, environmental, and fan-control modules**, each running independently yet automatically integrating into a **local MQTT + Prometheus + Grafana ecosystem**.

Supports:
- Wi-Fi + BLE provisioning  
- Auto-discovery (mDNS / DNS-SD)  
- MQTT telemetry and control  
- Prometheus metrics export  
- Hardware unique ID generation  
- Optional cloud bridging (AWS IoT / Azure / EMQX / HiveMQ Cloud)

---

## 🧱 1. Architecture Overview

Each module (Power Supply, Env Monitor, Fan Controller) is an SoC-based micro-node:
- **BLE provisioning** for Wi-Fi and broker setup
- **MQTT communication** for telemetry and commands
- **mDNS / DNS-SD** for self-discovery in LAN
- **Unique ID-based device identification**
- **Optional HTTP `/metrics`** endpoint for Prometheus

### Data Flow
```
[BLE phone app]
      ↓ (Wi-Fi SSID / MQTT host)
 [Module joins Wi-Fi]
      ↓
 [Publishes "announce" via MQTT]
      ↓
 [Site controller registers device]
      ↓
 [Prometheus & Grafana display metrics]
```

---

## ⚙️ 2. Core Components

| Component | Function |
|------------|-----------|
| **BLE Provisioning** | User sends Wi-Fi SSID, password, and broker info via BLE GATT |
| **Wi-Fi Interface** | Primary network transport |
| **mDNS / DNS-SD** | Advertise service: `_zephyrmod._tcp.local` |
| **MQTT Client** | Pub/sub for telemetry and control |
| **Unique ID** | Derived from SoC hardware ID → short 8-char code |
| **NVS/Settings** | Persistent storage for credentials/config |
| **Prometheus Exporter** | Provides `/metrics` endpoint or MQTT-to-Prometheus bridge |
| **Grafana Dashboard** | Displays system state and triggers alerts |

---

## 🔑 3. Unique Device Identification

Every SoC has a hardware unique identifier (UID) burned in at factory.

### Typical Sources
| SoC | UID Source | Example |
|-----|-------------|----------|
| **ESP32 / ESP32-C3** | eFuse MAC address | `A4:CF:12:34:56:78` |
| **STM32** | 96-bit Unique ID register | `0x33 00 12 45 AA BB ...` |
| **RP2040** | Flash serial | `0x10000000D6CFE3AB` |

### UID → 8-Character Short Code

To keep things user-friendly, each module converts its UID into a short, readable code.

```python
import hashlib
CROCK = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"

def short8(uid_bytes: bytes) -> str:
    h = hashlib.blake2s(uid_bytes, digest_size=5).digest()
    v = int.from_bytes(h, 'big')
    out = []
    for _ in range(8):
        out.append(CROCK[v & 0x1F])
        v >>= 5
    return ''.join(reversed(out))
```

Example:
```
UID: A4:CF:12:34:56:78 → short ID: 7X9G2C4M
```

### Usage in Framework
| Purpose | Example |
|----------|----------|
| **Device ID** | `psu-7X9G2C4M` |
| **mDNS Hostname** | `psu-7X9G2C4M.local` |
| **BLE Name** | `ZEPHYR-PSU-7X9G2C4M` |
| **MQTT Topic** | `site/lab1/power/psu-7X9G2C4M/state` |

All identities are derived from the same UID hash, guaranteeing consistency.

---

## 📡 4. MQTT Structure

### Topic Hierarchy
```
site/<site_id>/<module_kind>/<device_id>/
  ├── state   → telemetry JSON
  ├── status  → online/offline retained
  ├── cfg     → config (retained)
  └── cmd     → control commands
```

### Example Payloads

**Telemetry**
```json
{
  "ts": 1739798123,
  "v_bus": 12.47,
  "i_bus": 1.83,
  "t_board": 41.2,
  "fan_rpm": 1720,
  "fw": "psu/1.3.2"
}
```

**Command**
```json
{
  "output_enable": true,
  "v_set": 12.0,
  "i_limit": 3.0
}
```

---

## 🌍 5. mDNS / DNS-SD Discovery

Each device advertises itself as a local network service:

```
Service: _zephyrmod._tcp.local
Instance: PSU-7X9G2C4M._zephyrmod._tcp.local
TXT:
  kind=power
  fw=1.3.2
  metrics=/metrics
  device_id=psu-7X9G2C4M
```

- Enables **site controller** to auto-register devices.
- Works with `avahi-browse` or `dns-sd` tools.

---

## 📲 6. BLE Provisioning

| Characteristic | UUID | Access | Purpose |
|----------------|------|--------|----------|
| SSID | 0x2A00 | Write | Wi-Fi SSID |
| Password | 0x2A01 | Write | Wi-Fi password |
| Broker | 0x2A02 | Write | MQTT host/port |
| Site ID | 0x2A03 | Write | String |
| Apply | 0x2A04 | Write/Notify | Trigger config save |
| Status | 0x2A05 | Notify | Return OK/FAIL |

**Provisioning flow:**
1. Device boots unprovisioned → BLE advertises `ZEPHYR-PSU-7X9G2C4M`
2. User connects via **nRF Connect** / mobile app
3. Writes Wi-Fi + MQTT settings
4. Device stores to NVS → connects → disables BLE

---

## 🧮 7. Prometheus + Grafana Integration

| Component | Role |
|------------|------|
| **Prometheus** | Scrapes exporter or `/metrics` endpoints |
| **MQTT-Exporter** | Converts telemetry JSON to metrics |
| **Grafana** | Dashboard + alerting |
| **Alert Bridge (optional)** | Webhook → MQTT → LED/beeper trigger |

---

## 🔐 8. Security Practices

| Layer | Protection |
|--------|-------------|
| **BLE** | Encrypted pairing (SMP) |
| **Storage** | Encrypted NVS (Zephyr `CONFIG_NVS_ENCRYPTION=y`) |
| **MQTT** | TLS or PSK-based |
| **Identity** | Device certificate or UID-based keypair |
| **Access control** | Per-topic ACLs on broker |
| **Firmware integrity** | Signed images with MCUboot |

---

## 💡 9. Example Zephyr Configuration (`prj.conf`)

```ini
CONFIG_BT=y
CONFIG_BT_PERIPHERAL=y
CONFIG_BT_SMP=y
CONFIG_BT_SECURITY=y
CONFIG_WIFI=y
CONFIG_NET_IPV4=y
CONFIG_NET_CONFIG_NEED_IPV4=y
CONFIG_SETTINGS=y
CONFIG_SETTINGS_NVS=y
CONFIG_MQTT_LIB=y
CONFIG_NEWLIB_LIBC=y
CONFIG_LOG=y
CONFIG_UNIQUE_ID=y
```

---

## 🚀 10. Future Extensions

1. OTA Firmware Updates (MCUboot / HTTP)
2. Edge Policy Engine for local decision logic
3. Device Twin / Shadow for cloud sync
4. Secure QR-code BLE provisioning
5. Power budgeting across modules
6. CAN / RS-485 inter-module bus
7. Machine-learning edge analytics
8. Fleet management dashboard

---

## 🧩 11. Inter-Module Communication

| Bus | Best For | Distance | Features |
|------|-----------|-----------|-----------|
| **I²C / SPI** | On same PCB | <1 m | Simple sensors |
| **CAN Bus** | Real-time sync | up to 1 km | Multi-master, robust |
| **RS-485** | Long range | up to 1.2 km | Master-slave, low cost |

---

## 🧰 12. Development Environment

### Build Firmware
```bash
west build -b esp32c3_devkitm samples/modular_iot
west flash
```

### Run Prometheus & Grafana locally
```bash
docker-compose up -d
```
Prometheus → http://localhost:9090  
Grafana → http://localhost:3000

---

## 🧠 13. References
- [Zephyr Project Documentation](https://docs.zephyrproject.org/latest/)
- [RFC 6762 / RFC 6763 – mDNS & DNS-SD](https://datatracker.ietf.org/doc/html/rfc6762)
- [MQTT v3.1.1 / v5 Spec](https://mqtt.org)
- [Prometheus Documentation](https://prometheus.io/docs/)
- [Grafana Documentation](https://grafana.com/docs/)
- [Mosquitto Dynamic Security Plugin](https://mosquitto.org/)

---

## 🧾 License
MIT License © 2025  
Developed for modular embedded systems research and educational use.
