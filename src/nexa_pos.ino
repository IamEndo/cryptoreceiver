#include <Arduino.h>
#include "config.h"
#include <lvgl.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <qrcodegen.h>

// ... LVGL display+touch init glue goes here (reuse board example)

void setup() {
  Serial.begin(115200);
  LittleFS.begin(true);
  lv_init();

  // init display + touch drivers (from board example)

  // build UI (numeric keypad + QR canvas)
  // see nexa_pos example sketch in docs
}

void loop() {
  lv_timer_handler();
  delay(5);
}
