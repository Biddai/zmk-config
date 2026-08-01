# Nicepad Wireless ZMK Config

This repository contains a ZMK firmware config for a wireless Nicepad-style
10-key macropad using a nice!nano v2-compatible nRF52840 controller.

## Build Target

The GitHub Actions build matrix currently builds one firmware:

```yaml
board: nice_nano_v2
shield: nicepad
snippet: studio-rpc-usb-uart
```

ZMK Studio is enabled and unlocked by default:

```text
CONFIG_ZMK_STUDIO=y
CONFIG_ZMK_STUDIO_LOCKING=n
```

## Hardware Layout

The keypad is configured as 10 direct GPIO keys in a 4-row by 3-column physical
layout, with only one key in the first row.

```text
[0,0]
[1,0] [1,1] [1,2]
[2,0] [2,1] [2,2]
[3,0] [3,1] [3,2]
```

Direct pin mapping:

| Position | Pin |
| --- | --- |
| `[0,0]` | `D4` |
| `[1,0]` | `D5` |
| `[1,1]` | `D6` |
| `[1,2]` | `D15` |
| `[2,0]` | `D7` |
| `[2,1]` | `D14` |
| `[2,2]` | `D16` |
| `[3,0]` | `D8` |
| `[3,1]` | `D9` |
| `[3,2]` | `D10` |

All keys use `GPIO_ACTIVE_LOW | GPIO_PULL_UP`.

## Layers

There are five layers:

| Layer | Display Name | Top Key | Other Keys |
| --- | --- | --- | --- |
| 0 | `Base` | `&to 1` | `1` through `9` |
| 1 | `Layer 2` | `&to 2` | `1` through `9` |
| 2 | `Layer 3` | `&to 3` | `1` through `9` |
| 3 | `Layer 4` | `&to 4` | `1` through `9` |
| 4 | `BT` | `&to 0` | Bluetooth profile controls |

The `BT` layer contains:

```text
BT_SEL 0  BT_SEL 1  BT_SEL 2
BT_SEL 3  BT_SEL 4  BT_CLR
BT_PRV    BT_NXT    BT_CLR_ALL
```

## OLED

The display is a `128x64` SSD1306 OLED at I2C address `0x3c`.

I2C pins:

| Signal | nice!nano Pin | nRF Pin |
| --- | --- | --- |
| SDA | `D2` | `P0.17` |
| SCL | `D3` | `P0.20` |

The OLED is configured with:

```dts
segment-remap;
com-invdir;
inversion-on;
```

This flips the screen orientation and keeps the black/white polarity used by
the custom status screen.

## Custom Status Screen

The status screen is implemented in
`boards/shields/nicepad/custom_status_screen.c`.

It shows:

- A custom top banner.
- USB/BLE output status.
- BLE profile number when using BLE.
- Battery icon.
- Current layer name.

The layer name uses a custom bitmap font stored in
`boards/shields/nicepad/nicepad_layer_font.h`.

Layer-name rendering behavior:

- Uses the live ZMK layer display name.
- Studio-renamed layer names are shown on the OLED.
- Polls for Studio name changes only while USB is connected.
- Uses fast direct canvas-buffer rendering.
- Tries single-line `20px` text first.
- Falls back to two-line `20px`, then two-line `15px` if needed.

## Power Management

Battery-saving options are enabled:

```text
CONFIG_ZMK_DISPLAY_BLANK_ON_IDLE=y
CONFIG_ZMK_IDLE_TIMEOUT=30000
CONFIG_ZMK_SLEEP=y
CONFIG_ZMK_IDLE_SLEEP_TIMEOUT=900000
CONFIG_ZMK_EXT_POWER=y
```

In practice:

- Display blanks after 30 seconds of inactivity.
- Firmware enters deep sleep after 15 minutes of inactivity.
- External power support is enabled.

## Notes

- The encoder experiment was removed; this config is the 10-key version.
- ZMK Studio can edit key bindings and layer names, but encoder bindings are
  not currently part of this firmware.
- The build is intended for GitHub Actions rather than a local Zephyr SDK setup.
