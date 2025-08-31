// --- core includes
#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <WiFi.h>          // used later for export / Rostrum
#include <lvgl.h>
#include <qrcodegen.h>     // Project Nayuki - install "qrcodegen" lib

// --- display driver glue (from your board example)
// #include <LovyanGFX.hpp>
// static LGFX tft; // <-- if your example defines this, use it

// ---------- SETTINGS ----------
static String SHOP_NAME   = "My Shop";
static String MERCHANT_ADDR = "nexa:q........................."; // put any valid Nexa address here
static bool   SHOW_FIAT   = false;   // sprint 2 toggles this
static float  FX_USD_PER_NEX = 0.000001f; // placeholder, set in sprint 2
// ------------------------------

static lv_obj_t *scrMain, *taAmount, *btnShowQR, *lblUnit, *canvasQR, *lblStatus;
static char amountStr[16] = "0.00";   // 2 dp (NEXA format)
static const int QR_PIXELS = 220;     // QR canvas size
static const int QR_BORDER = 4;       // quiet zone modules

// Minimal URL-encode for spaces (enough for label)
static String urlEncode(String s) { s.replace(" ", "%20"); return s; }

static String buildNexaURI() {
  // ensure 2dp
  String amt(amountStr);
  if (!amt.length()) amt = "0.00";
  return "nexa:" + MERCHANT_ADDR + "?amount=" + amt + "&label=" + urlEncode(SHOP_NAME);
}

static void drawQRToCanvas(lv_obj_t* canvas, const String& text) {
  using namespace qrcodegen;

  uint8_t tmp[qrcodegen_BUFFER_LEN_MAX];
  uint8_t qrc[qrcodegen_BUFFER_LEN_MAX];

  bool ok = qrcodegen_encodeText(text.c_str(), tmp, qrc,
                                 qrcodegen_Ecc_MEDIUM,
                                 qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                                 qrcodegen_Mask_AUTO, true);
  if (!ok) return;

  int qrSize = qrcodegen_getSize(qrc);     // modules per side
  int modPixels = QR_PIXELS / (qrSize + QR_BORDER*2);
  int imgSize = (qrSize + QR_BORDER*2) * modPixels;

  // Create/resize canvas buffer
  static lv_color_t *buf = nullptr;
  static int bufW = 0, bufH = 0;
  if (bufW != imgSize || bufH != imgSize) {
    if (buf) free(buf);
    bufW = bufH = imgSize;
    buf = (lv_color_t*)malloc(bufW * bufH * sizeof(lv_color_t));
    lv_canvas_set_buffer(canvas, buf, bufW, bufH, LV_IMG_CF_TRUE_COLOR);
  }

  // Paint white background
  lv_color_t white = lv_color_white();
  for (int i=0;i<bufW*bufH;i++) buf[i] = white;

  // Draw modules
  lv_color_t black = lv_color_black();
  for (int y = -QR_BORDER; y < qrSize + QR_BORDER; y++) {
    for (int x = -QR_BORDER; x < qrSize + QR_BORDER; x++) {
      bool dark = (x >=0 && x < qrSize && y >=0 && y < qrSize) ? qrcodegen_getModule(qrc, x, y) : false;
      if (!dark) continue;
      int px = (x + QR_BORDER) * modPixels;
      int py = (y + QR_BORDER) * modPixels;
      for (int yy=0; yy<modPixels; yy++) {
        for (int xx=0; xx<modPixels; xx++) {
          buf[(py+yy)*bufW + (px+xx)] = black;
        }
      }
    }
  }
  lv_obj_center(canvas);
  lv_obj_invalidate(canvas);
}

static void showQR_cb(lv_event_t * e) {
  String uri = buildNexaURI();
  drawQRToCanvas(canvasQR, uri);
  lv_label_set_text(lblStatus, "Show this QR in Wally / Otoplo");
}

static void keypad_cb(lv_event_t * e) {
  const char *txt = lv_btnmatrix_get_btn_text((lv_obj_t*)e->current_target, lv_btnmatrix_get_selected_btn((lv_obj_t*)e->current_target));
  String s(amountStr);

  if (strcmp(txt, "C") == 0) { s = "0.00"; }
  else if (strcmp(txt, "⌫") == 0) {
    if (s.length() > 0) s.remove(s.length()-1);
    if (s.length()==0) s="0";
  } else if (strcmp(txt, ".") == 0) {
    if (s.indexOf('.') < 0) s += ".";
  } else if (strcmp(txt, "00") == 0) {
    s += "00";
  } else {
    s += txt; // digit
  }
  // normalize to max 2dp
  int dot = s.indexOf('.');
  if (dot >= 0 && s.length() > dot + 3) s = s.substring(0, dot + 3);
  // ensure format
  if (s[0] == '.') s = "0" + s;
  if (s.indexOf('.') < 0) s += ".00";
  else {
    while (s.substring(s.indexOf('.')+1).length() < 2) s += "0";
  }
  s.toCharArray(amountStr, sizeof(amountStr));
  lv_textarea_set_text(taAmount, amountStr);
}

static void build_ui() {
  scrMain = lv_scr_act();

  // Amount display
  taAmount = lv_textarea_create(scrMain);
  lv_obj_set_width(taAmount, 150);
  lv_textarea_set_one_line(taAmount, true);
  lv_textarea_set_text(taAmount, amountStr);
  lv_obj_align(taAmount, LV_ALIGN_TOP_LEFT, 8, 8);

  lblUnit = lv_label_create(scrMain);
  lv_label_set_text(lblUnit, "NEXA"); // sprint 2 will toggle
  lv_obj_align_to(lblUnit, taAmount, LV_ALIGN_OUT_RIGHT_MID, 8, 0);

  // Keypad
  static const char * btnm_map[] = {
    "1","2","3","\n",
    "4","5","6","\n",
    "7","8","9","\n",
    ".","0","00","\n",
    "C","⌫","QR",""
  };
  lv_obj_t *keypad = lv_btnmatrix_create(scrMain);
  lv_btnmatrix_set_map(keypad, btnm_map);
  lv_obj_set_size(keypad, 140, 180);
  lv_obj_align(keypad, LV_ALIGN_LEFT_MID, 8, 30);
  lv_obj_add_event_cb(keypad, keypad_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // QR Canvas
  canvasQR = lv_canvas_create(scrMain);
  lv_obj_set_size(canvasQR, QR_PIXELS, QR_PIXELS);
  lv_obj_align(canvasQR, LV_ALIGN_RIGHT_MID, -8, -10);

  // Show QR button (alternate to keypad "QR")
  btnShowQR = lv_btn_create(scrMain);
  lv_obj_align(btnShowQR, LV_ALIGN_BOTTOM_MID, 0, -6);
  lv_obj_add_event_cb(btnShowQR, showQR_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *lbl = lv_label_create(btnShowQR);
  lv_label_set_text(lbl, "Show QR");

  // Status label
  lblStatus = lv_label_create(scrMain);
  lv_obj_align(lblStatus, LV_ALIGN_BOTTOM_LEFT, 8, -6);
  lv_label_set_text(lblStatus, "");
}

void setup() {
  // if using LovyanGFX directly, init it here:
  // tft.init(); tft.setRotation(1);

  // LVGL init (use your board’s display + tick glue; most CYD examples have this set up)
  lv_init();

  // FS + prefs
  LittleFS.begin(true);
  Preferences prefs;
  prefs.begin("nexa-ps", false);
  SHOP_NAME = prefs.getString("shop", SHOP_NAME);
  MERCHANT_ADDR = prefs.getString("addr", MERCHANT_ADDR);
  prefs.end();

  build_ui();
}

void loop() {
  lv_timer_handler(); // LVGL heartbeat
  delay(5);
}
