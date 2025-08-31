// config.h – NexaPOS device configuration
#pragma once

// Display type: CYD ESP32-2432S028R (ILI9341 + XPT2046 touch)
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Pin definitions (adjust per board)
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_SCLK 18
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   32
#define TOUCH_CS 33
#define TOUCH_IRQ 36

// Wi-Fi defaults (optional; can be set in UI)
#define WIFI_SSID     ""
#define WIFI_PASSWORD ""

// Shop info
#define DEFAULT_SHOP_NAME "My Shop"
#define DEFAULT_ADDR      "nexa:qxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" // Replace with real address for testing

// Fiat defaults
#define DEFAULT_CURRENCY "USD"

// HD wallet (v4+)
#define COIN_TYPE 29223   // Nexa SLIP-0044 coin type (confirmed)

// Storage
#define LOG_FILE "/sales.csv"

// Feature flags
#define USE_SD false       // Set true if board has SD card
