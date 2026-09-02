## What this is

Firmware for a physical "Air Purifier Button": an ESP32-2432S028R ("Cheap
Yellow Display" / CYD) board shows an on/off graphic and toggles a Home
Assistant switch entity when touched. That virtual switch is bridged to the
real Tuya smart plug that powers the air purifier via
`homeassistant/link_air_purifier_button.yaml` (a bidirectional HA
automation) — neither firmware controls the purifier by itself without that
automation installed in Home Assistant (Settings → Automations & Scenes →
Edit in YAML).

There are **two independent, parallel firmware implementations of the same
device**, kept in sync by hand for comparison:

- **PlatformIO/Arduino** (repo root: `src/`, `include/`, `tools/`) — the
  original, hand-rolled implementation.
- **ESPHome** (`esphome/`) — a declarative reimplementation of the same
  behavior, for comparison against the Arduino version.

A change to the button's behavior (new image, different toggle logic, etc.)
generally needs to be made in both places.

## Hardware facts that aren't obvious from either config alone

- Board has **no PSRAM** and only 4MB flash. This constrains both builds:
  the Arduino build needs `board_build.partitions = huge_app.csv` (a single
  3MB app partition, no OTA) once the embedded image arrays are added, and
  the ESPHome build needs `color_palette: 8BIT` on the display — a full
  16-bit 320×240 framebuffer (150KB) doesn't fit as one contiguous
  allocation without PSRAM and setup() fails with `display is marked
  FAILED: unspecified`.
- The **touchscreen (XPT2046) is wired on a completely separate SPI bus**
  from the display (display: CLK14/MOSI13/MISO12/CS15/DC2; touch:
  CLK25/MOSI32/MISO39/CS33/IRQ36). Anything that assumes touch shares the
  display's SPI bus (e.g. TFT_eSPI's built-in touch support) will show a
  calibration screen but never register touches on this board.
- Backlight is GPIO21, active high.
- Only "was the screen touched" matters, not touch coordinates — neither
  firmware calibrates touch coordinates.

## PlatformIO / Arduino build (repo root)

```
pio run                    # build
pio run -t upload          # build + flash over USB
pio device monitor         # serial monitor (115200 baud)
pio run -t clean
```

- `TFT_eSPI` is configured entirely via `build_flags` in `platformio.ini`
  (the `USER_SETUP_LOADED` pattern) rather than editing the library's
  `User_Setup.h` — keep new display config there, not in a vendored header.
- `tft.setSwapBytes(true)` is required before `pushImage()` calls — without
  it, colors come out corrupted (classic TFT_eSPI RGB565 byte-order issue).
- On/off images are baked into flash as RGB565 C arrays
  (`include/off_image.h`, `include/on_image.h`), generated from
  `assets/off.png` / `assets/on.png` via:
  ```
  python tools/convert_image.py <src.png> <dst.h> <array_name> <width> <height>
  ```
  Regenerate these whenever the source PNGs change; the header isn't
  derived automatically at build time.
- MQTT/Home Assistant integration: publishes MQTT discovery config,
  state/command/availability topics with LWT, under `air_purifier_button/*`
  topics and `homeassistant/switch/air_purifier_button/config`. `MQTT_HOST`
  may be a plain IP or an `.local` mDNS hostname — `.local` resolution goes
  through `ESPmDNS`/`MDNS.queryHost()` explicitly (the default `WiFiClient`
  DNS path does not resolve `.local` names), re-resolved on every
  connection attempt.
- Secrets: `include/secrets.h` (gitignored; real WiFi/MQTT credentials) vs.
  `include/secrets.h.example` (committed template). Copy the example and
  fill in real values before building.

## ESPHome build (`esphome/` directory)

**Must be run from PowerShell, not Git Bash** — ESPHome's ESP-IDF installer
explicitly refuses to run under an MSYS/MinGW shell on Windows.

```powershell
cd esphome
python -m esphome config air-purifier-button.yaml              # validate only
python -m esphome compile air-purifier-button.yaml              # build only
python -m esphome run air-purifier-button.yaml --device COM3    # build + flash + attach logs
python -m esphome logs air-purifier-button.yaml                 # attach to logs only
```

- No image-conversion step needed — the `image:` component reads
  `assets/*.png` directly and converts to RGB565 at compile time.
- No manual MQTT discovery — the `switch.air_purifier_button` entity is
  auto-exposed to Home Assistant via the native `api:` component; add the
  device in HA under Settings → Devices & Services (it's discovered via
  mDNS).
- `is_svg_file()` in ESPHome's image component does a naive `<svg`
  substring search on raw file bytes to guess file type. If a PNG happens
  to contain that byte sequence anywhere (e.g. in embedded metadata from an
  export tool), it gets misdetected as SVG and crashes ESPHome's SVG
  renderer on binary data. If a new PNG asset fails to compile with a Rust
  panic (`can't convert bytes to utf-8`), re-save it through Pillow to
  strip whatever non-standard chunk is triggering the false match.
- The display's `resize` is contain-fit (preserves aspect ratio, does not
  crop or stretch), so images generally don't fill the full 320×240 frame.
  The display `lambda` clears the screen and centers whichever image is
  active — see the comments there before changing it.
- Secrets: `esphome/secrets.yaml` (gitignored) vs.
  `esphome/secrets.yaml.example` (committed template) — same pattern as
  the Arduino build's `secrets.h`.

## No automated tests

`test/` is PlatformIO's unused default scaffold — there is no test suite
for either firmware.
