# 📡 LoRa to MQTT Gateway

> A lightweight LoRa-to-MQTT bridge for Raspberry Pi that receives environmental sensor data over-the-air and publishes it to an MQTT broker.

[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi-brightgreen?style=flat-square&logo=raspberrypi)](https://www.raspberrypi.com/)
[![Language](https://img.shields.io/badge/Language-C%2B%2B-%2300599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-MIT-blue?style=flat-square)](LICENSE)
[![LoRa](https://img.shields.io/badge/Radio-RFM95W%20LoRa-orange?style=flat-square)](https://www.semiconductor.samsung.com/products/semiconductor-for-iot/rf-m95w/)

---

## 📑 Table of Contents

- [Overview](#-overview)
- [Architecture](#-architecture)
- [Prerequisites](#-prerequisites)
- [Installation](#-installation)
- [Configuration](#-configuration)
- [Building](#-building)
- [Running](#-running)
- [Hardware Wiring](#-hardware-wiring)
- [Message Format](#-message-format)
- [Radio Configuration](#-radio-configuration)
- [Logging](#-logging)
- [Troubleshooting](#-troubleshooting)
- [Related Projects](#-related-projects)

---

## 🎯 Overview

This gateway bridges **LoRa radio traffic** to an **MQTT message broker**. It continuously listens for incoming LoRa packets from environmental sensor nodes, decodes the payload (temperature, humidity, pressure, battery voltage), and forwards the data as structured XML messages to a configurable MQTT topic.

**Typical use case:** Deploy multiple battery-powered sensor nodes in the field → this gateway collects all readings → MQTT broker distributes data to dashboards, databases, or alerting systems.

---

## 🏗️ Architecture

```
┌──────────────┐      LoRa (868 MHz)      ┌──────────────────┐      MQTT      ┌─────────────┐
│  Sensor Node │ ────────────────────────► │  Raspberry Pi    │ ──────────────► │  MQTT       │
│  (RFM95W)    │   temperature, humidity, │  Gateway         │   XML payload   │  Broker     │
│              │   pressure, voltage      │  (this project)  │                 │             │
└──────────────┘                          └──────────────────┘                 └─────────────┘
```

| Component | Technology |
|-----------|-----------|
| **Language** | C++ |
| **Radio Driver** | [RadioHead](https://github.com/hallard/RadioHead) (RH_RF95) |
| **GPIO/SPI** | [bcm2835](http://www.airspayce.com/mikem/bcm2835/) library |
| **MQTT Client** | [libmosquittopp](https://github.com/eclipse/mosquitto) (C++) |
| **Config Parser** | [libconfig++](https://www.hyperrealm.com/libconfig/) |
| **Radio Module** | RFM95W (Semtech SX1276-based) |

---

## 📋 Prerequisites

### Hardware

- Raspberry Pi (tested on **Model 1 B**, works on any BCM2835-based Pi)
- **RFM95W** LoRa transceiver module (868 MHz or 915 MHz variant)
- Jumper wires for SPI connection

### Software Dependencies

```bash
sudo apt update
sudo apt install -y g++ make libbcm2835-dev libmosquittopp-dev libconfig++-dev
```

> **Note:** The `bcm2835` library provides low-level GPIO and SPI access. The Pi must run a Raspberry Pi-compatible OS.

---

## 🚀 Installation

```bash
# Clone the repository
git clone https://github.com/denysobukh/RFM95-MQTT-Gateway.git
cd RFM95-MQTT-Gateway

# Initialize the RadioHead submodule
git submodule update --init --recursive
```

---

## ⚙️ Configuration

Edit [`gateway.cfg`](gateway.cfg) to match your environment:

| Parameter | Type | Description | Example |
|-----------|------|-------------|---------|
| `radio_freq` | float | LoRa carrier frequency in MHz | `868.45` |
| `client_id` | string | Unique MQTT client identifier | `"rpi"` |
| `host` | string | MQTT broker hostname or IP | `"nas.loc"` |
| `port` | int | MQTT broker port | `1883` |
| `topic` | string | MQTT topic for publishing sensor data | `"sensor/weather"` |

> ⚠️ Ensure `radio_freq` matches the frequency used by your sensor nodes and is legal in your region (868 MHz in EU, 915 MHz in US).

---

## 🔨 Building

```bash
make          # Compile the gateway binary
make clean    # Remove build artifacts (*.o, gateway)
```

The compiled binary `gateway` will appear in the project root.

---

## ▶️ Running

The gateway requires **root privileges** to access GPIO/SPI peripherals:

```bash
sudo ./gateway
```

### Auto-start on Boot

Add to your crontab (`crontab -e`):

```cron
@reboot /usr/bin/screen -dm bash -c 'sleep 1; cd /home/pi/RFM95-MQTT-Gateway; sudo ./gateway; exec sh'
```

This launches the gateway in a detached `screen` session after a 1-second delay (allowing WiFi/network to initialize).

### Graceful Shutdown

Press **Ctrl+C** to stop the gateway cleanly. MQTT connections and syslog will be closed properly.

---

## 🔌 Hardware Wiring

Connect the RFM95W module to the Raspberry Pi via SPI:

| RFM95W Pin | Raspberry Pi Pin | Function |
|:----------:|:----------------:|----------|
| **3.3V** | 1 | 3.3V Power |
| **GND** | 6 | Ground |
| **DIO0** | 7 (GPIO 4) | IRQ / Ready |
| **RESET** | 11 (GPIO 17) | Reset |
| **NSS** | 22 (GPIO 25) | SPI Chip Select |
| **MOSI** | 19 | SPI Master Out Slave In |
| **MISO** | 21 | SPI Master In Slave Out |
| **SCK** | 23 | SPI Clock |

> 💡 **Tip:** Always use **3.3V logic**. The RFM95W is **not** 5V tolerant — connecting to 5V pins will destroy the module.

---

## 📨 Message Format

Decoded sensor data is published as XML to the configured MQTT topic:

```xml
<message time="2025-01-15 14:32:07 +0100" from="1" to="255" rssi="-85" id="1">
  <temperature>22.5</temperature>
  <humidity>45.0</humidity>
  <pressure>101325</pressure>
  <voltage>3.30</voltage>
</message>
```

| Field | Unit | Description |
|-------|------|-------------|
| `temperature` | °C | Air temperature (one decimal) |
| `humidity` | %RH | Relative humidity (one decimal) |
| `pressure` | Pa | Atmospheric pressure |
| `voltage` | V | Sensor node battery voltage (two decimals) |

---

## 📻 Radio Configuration

The following radio parameters are compiled into `gateway.cpp`:

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Tx Power** | 14 dBm | Via PA_BOOST (EU legal limit) |
| **Modem Config** | `Bw500Cr45Sf128` | 500 kHz BW, CR 4/5, SF 128 — fast, short-range |
| **Preamble** | 8 bytes | |
| **Mode** | Promiscuous | Accepts packets from any node address |
| **CRC** | Enabled | Packet integrity verification |

> 📖 Alternative modem configs in RadioHead: `Bw125Cr45Sf128` (medium range), `Bw31_25Cr48Sf512` (long range), `Bw125Cr48Sf4096` (maximum range). Adjust in `gateway.cpp` if your deployment requires different range/speed trade-offs.

---

## 📝 Logging

The gateway writes to **syslog** (`LOG_LOCAL1` facility):

```bash
# View gateway logs
sudo journalctl -u RFM95-MQTT-Gateway   # if using systemd
grep "RFM95 to MQTT Gateway" /var/log/syslog
```

Console output includes packet details (RSSI, source/destination, decoded values) in real time.

---

## 🐛 Troubleshooting

| Symptom | Possible Cause | Fix |
|---------|---------------|-----|
| `bcm2835_init() Failed` | Not running as root | Use `sudo ./gateway` |
| `RF95 module init failed` | SPI wiring issue | Verify all 8 connections, check for loose wires |
| No packets received | Frequency mismatch | Ensure `radio_freq` matches your sensor nodes |
| No packets received | Modem config mismatch | Ensure nodes use the same `Bw/Cr/Sf` settings |
| MQTT connection fails | Broker unreachable | Verify `host`/`port` in `gateway.cfg`, check network |
| Very low RSSI values | Antenna not connected | Attach a proper ~17 cm (868 MHz) or ~16 cm (915 MHz) antenna |

---

## 🔗 Related Projects

- **[environment-sensor-node](https://github.com/denysobukh/environment-sensor-node)** — The transmitting sensor node that pairs with this gateway



