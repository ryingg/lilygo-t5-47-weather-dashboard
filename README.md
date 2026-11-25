# Lilygo T5-4.7" ESPHome Weather Dashboard

ESPHome configuration for the original (non-S3/Plus) Lilygo T5-4.7" e-paper
board, driving a Home Assistant hourly weather forecast display with
battery-efficient deep sleep and physical button controls.

## Hardware

- Lilygo T5-4.7" e-paper, original revision (ESP32-D0WD-V3, **not** S3/Plus),
  8MB onboard PSRAM.
- 3.7V LiPo battery (tested with a 2000mAh cell) via the onboard ADC on
  GPIO36 (11db attenuation, 2:1-ish divider — see `batt_volt` filter for
  calibration notes).

### Physical buttons (left to right, facing the screen)

| Label | GPIO    | Behavior                                                  |
|-------|---------|------------------------------------------------------------|
| S5    | —       | Hardware RESET, not a GPIO, unused in this config           |
| S6    | GPIO0   | BOOT strap pin, unused in this config                       |
| S3    | GPIO35  | Enter deep sleep immediately                                 |
| S2    | GPIO34  | Toggle sleep-on-wake; if now enabled, also enters sleep now  |
| S4    | GPIO39  | Refresh display / wake from deep sleep (shared wakeup pin)   |

## Display component

`t547_custom/t547/` is a locally vendored, patched copy of
[cjb0001/esphome-components](https://github.com/cjb0001/esphome-components)'
`t547` component (`idf5-arduino3` branch), which itself vendors LilyGo's
official Arduino e-paper driver with ESP-IDF 5 compatibility fixes. It's
fully self-contained — no external git library fetch at build time.

Patches on top of the upstream `t547` component, tuned to approximate the
original `ashald/esphome@lilygo-t5-47` driver's single-flash, low-ghosting
behavior on ESPHome 2026.11.0 + Arduino framework:

- Monochrome mode (`greyscale: false`) uses a reduced clear/contrast pass
  (`epd_clear_area_cycles(..., 1, 50)` + `epd_draw_image_frames(..., 6)`,
  added as a new generalized function in `epd_driver.c/.h`) instead of the
  stock 15-frame full grayscale redraw, cutting a ~12-flash update down to a
  single flash while keeping acceptable contrast.
- `clean()` and `setup()` no longer force an extra heavy clear/redraw on
  every boot/wake — the display only draws when explicitly updated via
  `component.update`, once real Home Assistant data has arrived, avoiding a
  double-flash and a blank/`nan` first frame on every wake.

## Efficiency notes

- `wifi.manual_ip` skips the DHCP handshake on every wake (fill in your own
  `static_ip`/`gateway` via `secrets.yaml`).
- `wifi.power_save_mode: NONE` + `fast_connect: true` reduce association
  time per wake.
- `logger.level: WARN` cuts serial/flash overhead versus default `DEBUG`.
- `display.update_interval: never` — the display never redraws on its own
  polling schedule, only on explicit `component.update` calls (buttons, or
  once Home Assistant data is fully received).

## Secrets

Copy the keys referenced via `!secret` in `lilygo-t5-47.yaml` into your own
`secrets.yaml` (not tracked in this repo):

```yaml
wifi_ssid: "..."
wifi_password: "..."
ota_password: "..."
ap_password: "..."
static_ip: "..."
gateway: "..."
```
