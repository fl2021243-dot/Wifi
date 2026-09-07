# ESP32 Wi-Fi & BLE Security Lab

> **Educational and authorized security testing only.**
>
> This project is intended for cybersecurity education, research, and testing in controlled environments. Use it only with hardware, wireless networks, and devices that you own or have explicit authorization to test.
>
> Do not use this project against unauthorized networks or third-party devices. You are responsible for complying with applicable laws and regulations.

## Overview

This project provides an ESP32-based wireless security lab for experimenting with Wi-Fi and Bluetooth Low Energy (BLE) concepts in a controlled environment.

The ESP32 hosts a local web interface that can be accessed from a browser after connecting to the ESP32 access point.

## Features

* Wi-Fi network scanning

  * SSID
  * BSSID
  * Channel
  * RSSI

* BLE device scanning

  * Device name
  * MAC address
  * RSSI

* Wi-Fi deauthentication demonstration

  * Selected test network
  * Lab networks

* Connected-client discovery using promiscuous-mode frame capture

* Fake Access Point / beacon demonstration

  * Supports multiple test SSIDs

* BLE advertisement testing

  * Apple
  * Samsung
  * Google Fast Pair
  * Tile

* Local Web UI

  * `http://192.168.4.1`

* Serial command interface

* Safety controls

  * Configurable automatic timeout
  * Default timeout: 30 seconds
  * Arm/disarm control

## Repository Structure

```text
.
├── esp32_wifi_ble_lab/
│   └── esp32_wifi_ble_lab.ino


The `.ino` file is intended for the **Arduino IDE**, while `src/main.cpp` can be used with **PlatformIO**.

Both implement the same main ESP32 firmware. Choose the development environment you prefer.

## Requirements

### Hardware

* ESP32 Dev Module
* USB data cable
* Optional second ESP32 for controlled BLE testing

### Software

Use either:

* Arduino IDE with the ESP32 board package

or

* PlatformIO with Visual Studio Code / CLI

## Build and Upload Using Arduino IDE

### 1. Install ESP32 Board Support

Open:

**File → Preferences → Additional Boards Manager URLs**

Add the official Espressif ESP32 Arduino board package URL.

Then open:

**Tools → Board → Boards Manager**

Search for **ESP32** and install the ESP32 package by Espressif Systems.

### 2. Select the Board

Navigate to:

**Tools → Board → ESP32 Arduino → ESP32 Dev Module**

### 3. Open the Firmware

Open:

```text
esp32_wifi_ble_lab/esp32_wifi_ble_lab.ino
```

### 4. Connect the ESP32

Connect the ESP32 development board to your computer using a USB data cable.

Select the correct serial port under:

**Tools → Port**

### 5. Upload the Firmware

Click **Upload** in the Arduino IDE.

Wait until the firmware has been successfully compiled and uploaded to the ESP32.

## Accessing the Web Interface

After the firmware starts, the ESP32 creates a local Wi-Fi Access Point.

Default lab configuration:

```text
SSID: pavan
Web UI: http://192.168.4.1
```

Connect your test device to the ESP32 Wi-Fi network.

Then open a web browser and enter:

```text
http://192.168.4.1
```

The ESP32-hosted local web interface should now appear.

## Serial Interface

The firmware also provides a serial command interface.

Open the Serial Monitor using:

```text
115200 baud
```

Available commands include:

```text
scan
blescan
clients
ssid <name>
beacon [seconds]
deauth <index|all> [seconds]
deauthc <index> [seconds]
ble [seconds]
stop
arm
disarm
help
```

### Command Overview

`scan` — Scan nearby Wi-Fi networks.

`blescan` — Scan nearby BLE devices.

`clients` — Discover connected clients during the configured capture period.

`ssid <name>` — Configure the test SSID used by the beacon demonstration.

`beacon [seconds]` — Start the beacon demonstration for the specified duration.

`deauth <index|all> [seconds]` — Run the deauthentication demonstration against an authorized lab network.

`deauthc <index> [seconds]` — Run the demonstration against a selected authorized test client.

`ble [seconds]` — Start the BLE advertisement demonstration.

`stop` — Stop active demonstrations.

`arm` — Enable demonstration controls.

`disarm` — Disable demonstration controls.

`help` — Display available commands.

## BLE Test Target

The repository also contains:

```text
ble_test_target/ble_test_target.ino
```

This sketch can be uploaded to a **second ESP32**.

It creates a small BLE GATT test server that can be used to verify BLE communication in a controlled lab without involving third-party devices.

## Safety

Always perform wireless-security testing in an isolated and authorized environment.

Recommended setup:

```text
ESP32 Lab Board
      │
      ├── Your Test Wi-Fi Network
      │
      ├── Your Test Device
      │
      └── Optional Second ESP32
```

Do not perform deauthentication, beacon, BLE advertisement, or other wireless testing against networks or devices without explicit authorization.

## Disclaimer

This project is provided for **educational, cybersecurity research, and authorized penetration-testing purposes only**.

The author is not responsible for unauthorized or illegal use of this project.