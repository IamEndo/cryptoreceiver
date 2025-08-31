# NexaPOS – ESP32 Crypto Payment Receiver (QR)
A tiny, open-source point-of-sale receiver for Nexa on an ESP32 with a 2.8" 240×320 touch display (LVGL).

Type an amount → device shows a Nexa payment URI QR → customer scans with Wally/Otoplo → pay.
Offline-first, privacy-first, and beginner-friendly.

⚠️ This project is receive-only. Private keys are shown once at setup to derive an xpub and then wiped. The device cannot spend.

# Table of contents

Project vision
Hardware
Software stack
Feature set (v1 → v4)
Architecture & security model
Screens & UX
Quick start (dev)
Build & flash
Configuration
How it works (under the hood)
Data & exports
Roadmap & milestones
Contributing & workflow
Repo layout
FAQ
License

# Project vision

Goal: Make accepting Nexa in shops as simple as tapping numbers and showing a QR—no phones required.
Constraints: Must work offline (no price fetch), privacy-preserving (rotate address every sale), and be useable by non-technical staff.
End state: A countertop device with touch keypad, large QR, audible/visual “Payment accepted” when the first sub-block confirms (targeting four seconds).

# Hardware

Target board class:
ESP32 + 2.4/2.8/3.5" TFT with touch (e.g., ESP32-2432S028R / CYD family)
Display: ILI9341 (240×320) or similar
Touch: XPT2046 (resistive) or equivalent
Wi-Fi & BLE built-in
microSD slot on many variants (optional)
5V / 2A USB-C (or solid 5V USB) power
Any ESP32 with a ≥240×320 screen will work; UI is built with LVGL, rendering a large, high-contrast QR that scans instantly.

# Software stack

Language: C++ (Arduino Core for ESP32)
UI: [LVGL] for widgets/keypad, LovyanGFX for fast display drivers
QR: [Project Nayuki qrcodegen] (small & robust)
Storage: Preferences (NVS) for settings + LittleFS (or SD) for logs/exports
Networking: WiFi + minimal WebSocket client (Rostrum) + tiny HTTP server for file export
Crypto (Phase 4): BIP39 seed generation + BIP32 derivation → store xpub only (receive-only)
Optional: NimBLE-Arduino for BLE onboarding
Exact library versions are pinned in platformio.ini/library.properties or the Arduino library.json (to be added in first commits).

# Feature set (v1 → v4)

v1 – MVP (offline QR)
Touch keypad (LVGL): enter amount (2 decimals)
Build Nexa URI: nexa:<address>?amount=<decimal>&label=<shop>
Render large QR on screen
Append sale to local log (time, amount, address)

v2 – Fiat mode (still offline-friendly)
Toggle [NEXA / Fiat] on keypad screen
Cached FX rate (persisted with timestamp); if offline/stale, show ~ indicator
Convert using minor units internally (integer = NEXA × 100)

v3 – Payment status
Seen (mempool) → spinner
Accepted (sub-block / Tailstorm target 1–4s) → green circle + chime
Confirmed (1+ block) → checkmark
Source: Rostrum (Electrum-like) WS subscribe initially; wire Tailstorm sub-blocks once node support is available

v4 – HD wallet & rotation

Onboarding shows 12-word BIP39, then derives xpub (receive-only)
Seed/xprv wiped after setup
Per-sale address rotation; configurable gap
Optionally pre-derive N addresses; never show seed again in UI

# Architecture & security model

Receive-only device
Setup: generate BIP39 12 words → derive xpub (HD path set in config.h once confirmed for Nexa) → forget seed/xprv
Operate: derive fresh address per sale from xpub
Offline-first
Device shows QR without network
Fiat uses last cached FX or falls back to NEXA-only
No custody / no spending
The device never signs transactions
Theft scenario: attacker gets only public data (xpub, logs)
Privacy
Address rotates every sale by default
Logs contain amounts, derived address index, and optional txid/status—no PII

# Screens & UX

Home: amount field, big keypad (1 2 3 / 4 5 6 / 7 8 9 / . 0 00 / C ⌫ QR), unit label (NEXA or fiat)
Show QR: full-size QR with URI; “Show this QR in Wally/Otoplo”
Status: spinner → green “Payment accepted” → checkmark (with times)
History: paginated list; “Export CSV” button
Settings (PIN-gated):
Shop name/label
Merchant address (until v4), or xpub (v4+)
Currency & fiat toggle default
Wi-Fi setup (or BLE onboarding)
Address rotation on/off, gap size
“Show seed” does not exist (seed is only at first-run)

# Quick start (dev)

Arduino IDE
Boards Manager → install esp32 by Espressif Systems
Libraries: lvgl, LovyanGFX, qrcodegen, ArduinoJson, Preferences, LittleFS (and WebSocketsClient later)
Clone & open

git clone https://github.com/<you>/NexaPOS-ESP32.git
cd NexaPOS-ESP32

Select board & COM port (ESP32 Dev Module or your specific CYD board)
First flash: run the provided display+touch demo for your board (examples folder) to verify panel & touch
Build the app and flash src/nexa_pos.ino

# Build & flash

Board:    ESP32 Dev Module (or your CYD variant)
Upload:   921600 baud (or 460800 if flaky)
Partition: Default (set "Minimal SPIFFS" if you need larger LittleFS)
FS:       LittleFS (run "ESP32 LittleFS Data Upload" if using the plugin)

LittleFS is used for logs; it auto-formats on first boot.
If your board has SD, set USE_SD=1 in config.h to mirror logs to /sd/sales.csv.

# Configuration

All runtime settings live in Preferences (NVS); first boot asks for:
Shop name (label for URIs)
Merchant address (v1–v3) or xpub (v4+)
Currency (for fiat display), default NEXA
PIN for Settings
Edit defaults in src/config.h.
In v4, the setup wizard also creates 12 BIP39 words and immediately discards secrets after deriving xpub.

# How it works (under the hood)

1) Building the payment QR
URI: nexa:<address>?amount=<decimal>&label=<shop>
Amount is two decimals in UI; internally we store minor units (integer = NEXA × 100)
QR created with qrcodegen and rendered to an LVGL canvas at large size for reliable scans

2) Fiat mode
When toggled, keypad edits fiat; device converts to NEXA using cached FX
If FX is stale or missing, we show ~ and allow NEXA-only entry

3) Payment status (“bingo”)
Rostrum WS: subscribe to each displayed address; mempool hit triggers “Seen”
Tailstorm sub-blocks: once available via node/WS, we treat the first sub-block inclusion as “Payment accepted” (green circle)
A subsequent full block confirmation shows the checkmark

4) HD wallet & rotation

First-run: show 12-word seed → derive xpub (HD path constant set after confirming Nexa SLIP coin type); wipe seed/xprv
Operation: derive address[index] from xpub per sale; bump index (gap tracking optional)
Security: settings store only xpub and counters; no secret at rest

# Data & exports

Log files (LittleFS or SD):
/sales.ndjson – one JSON object per line
/sales.csv – spreadsheet-friendly
Record fields: timestamp (UTC), amount_minor_units, display_amount, currency, address, index, uri, status (requested|seen|accepted|confirmed), optional txid
Export: device spins up a tiny HTTP server (Wi-Fi) and exposes /sales.csv and /sales.ndjson for download from your laptop/phone on the same LAN
Example CSV

ts,amount_minor,display_amount,currency,address,index,uri,status,txid
2025-08-31T10:15:12Z,1250,12.50,NEXA,nexa:q...,42,"nexa:q...?amount=12.50&label=My%20Shop",accepted,

# Roadmap & milestones

M0 – Repo bootstrap ✅
Pin library versions, add board bring-up examples
CI build (compile-only) for common ESP32 boards

M1 – MVP UI & QR (v1)
Touch keypad → Nexa URI → QR
Basic settings in NVS; sales logging

M2 – Fiat mode (v2)
Currency toggle, cached FX with timestamp & stale indicator
Safe rounding; store minor units only

M3 – Payment status (v3)
Rostrum WS: seen/confirmed
“Payment accepted” UI; green circle + beep
Hook Tailstorm sub-blocks when public endpoints are stable

M4 – HD wallet & rotation (v4)

BIP39 setup → xpub → wipe seed/xprv
Per-sale address rotation; gap management
Optional xpub QR export for accounting

M5 – Polish & field tests

Enclosure tweaks, brightness/timeout, watchdog
Export over SD/HTTP, CSV schema lock
Translations & accessibility

# Contributing & workflow

Issues: use labels good-first-issue, help-wanted, bug, enhancement, hardware, ui, wallet, network
Branches: main (stable), dev (next), feature branches feat/<area>-<short>
PRs: small, focused; include:
What/why
Screenshots or a short screen recording (if UI)
Tested board(s) & steps
Coding style: Arduino/C++17, clang-format (config provided)
Commits: Conventional Commits (feat:, fix:, chore:, docs:)
New to hardware/C++? Start with display bring-up and QR rendering—great first PRs are adding a new board config or improving the keypad UX.

# Repo layout

NexaPOS-ESP32/
├─ src/
│  ├─ nexa_pos.ino          # main app
│  ├─ ui/                   # LVGL screens & widgets
│  ├─ qr/                   # QR builder & canvas renderer
│  ├─ net/                  # WiFi/WS client (Rostrum), tiny HTTP server
│  ├─ wallet/               # BIP39/BIP32 (xpub-only), address derivation
│  ├─ store/                # Preferences (NVS) + LittleFS I/O
│  └─ config.h              # pins, display, currency defaults, HD path constant (to confirm)
├─ examples/
│  ├─ bringup_cyd_28/       # verify TFT + touch for ESP32-2432S028R
│  └─ minimal_qr/           # QR-only sketch
├─ data/                    # LittleFS data (if used)
├─ docs/
│  ├─ wiring.md             # pinouts per board
│  ├─ protocol.md           # URIs, status states, logs schema
│  └─ roadmap.md
├─ .github/
│  ├─ ISSUE_TEMPLATE/...
│  └─ workflows/ci.yml      # compile check
└─ LICENSE

# FAQ 

Why two decimals?
To match Nexa’s common UX; internally we store minor units (integer = NEXA × 100) to avoid rounding drift.

Can I use my own address instead of generating a seed?
Yes. In v1–v3 you can paste a merchant address in Settings. In v4+ we recommend the xpub model for rotation without secrets.

Does it need internet?
No for basic QR display. Yes for fiat price refresh and live payment status.

How do I get “Payment accepted” in ~1–4 seconds?
That’s the Tailstorm sub-block target. We’ll expose it as soon as public node endpoints make sub-block notifications available. Until then, we show Seen (mempool) and Confirmed (1+ block).

What if the device is stolen?
It stores no seed and cannot spend. Regenerate on a new device with your seed (kept offline during setup).

# License

N/A

# Legal & safety

This is not a payment processor or custody service; it only displays payment requests and monitors the network.
Always verify regulatory requirements for your jurisdiction.
Contributions that add private key storage or spending will be reviewed critically; receive-only is a core design goal.
