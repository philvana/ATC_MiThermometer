# MJWSD05MMC room setpoint (fork feature)

Firmware branch: `setpoint-temperature`  
Devices: **MJWSD05MMC** and **MJWSD05MMC_EN** (V2.3, with top + bottom buttons).

This fork adds a **room heating setpoint** (consigne pièce) for home automation. It is **not** the thermostat relay output: the value is shown on the LCD and sent over BLE for your installation (MQTT / Home Assistant via OpenMQTTGateway).

---

## Summary

| Topic | Behaviour |
|--------|-----------|
| LCD clock | Hidden when TelinkMiFlasher **12-hour clock** is **off** |
| LCD line s2 | Shows setpoint (10 → 18 → 18.5 → … → 23 °C) instead of date |
| Bottom button (PC4) | Short press cycles setpoint (when clock is hidden) |
| Top button (PB6) | Unchanged: BLE connect / screen mode / factory reset (with bottom held) |
| BLE format | **PVVX** (30 hex `servicedata`) — required for OMG / Theengs |
| Setpoint in MQTT | Encoded in **`batt`** field (`setpoint °C = batt / 2`) |
| Real battery | Use **`volt`** only (do not use `batt` as % when clock is hidden) |

Build example (EN variant):

```text
make PROJECT_NAME=BTE_v58 POJECT_DEF="-DDEVICE_TYPE=DEVICE_MJWSD05MMC_EN"
```

Flash `BTE_v58.bin` via [TelinkMiFlasher](https://pvvx.github.io/ATC_MiThermometer/TelinkMiFlasher.html).

---

## TelinkMiFlasher settings

| Setting | Recommended |
|---------|-------------|
| Device | MJWSD05MMC or MJWSD05MMC_EN |
| **Advertising type** | **PVVX** (not BTHome default) |
| **12-hour clock** | **Off** (hides clock, enables setpoint + `batt` hack) |
| Encryption / bindkey | **Off** for passive OMG scan |
| Long Range | **Off** (Web Bluetooth / OMG use 1M PHY) |

Click **Send config** after changes.

---

## Buttons (MJWSD05MMC)

| Button | GPIO | Role |
|--------|------|------|
| **Top** | PB6 (`GPIO_KEY2`) | Short press: connectable BLE ~80 s. Long ~1.75 s: LCD screen type. Hold ~5 s + **bottom pressed**: factory reset |
| **Bottom** | PC4 (`GPIO_KEY1` / `GPIO_RDS1`) | Short press: **cycle setpoint** (only if clock hidden) |

Setpoint sequence (°C): **10 → 18 → 18.5 → 19 → 19.5 → 20 → 20.5 → 21 → 21.5 → 22 → 22.5 → 23 → 10**.

---

## OpenMQTTGateway / Home Assistant

Decoder profile: `LYWSD03MMC/MJWSD05MMC_PVVX` (Theengs, built into OMG).

When clock is hidden, firmware sends **setpoint × 2** in the PVVX `battery_level` byte (OMG field `batt`):

| Room setpoint | `batt` in MQTT | `volt` |
|---------------|----------------|--------|
| 10.0 °C | 20 | Battery voltage (V) |
| 18.0 °C | 36 | Battery voltage (V) |
| 18.5 °C | 37 | Battery voltage (V) |
| 23.0 °C | 46 | Battery voltage (V) |

**Legacy sensors** (stock firmware or clock shown) still send **0–100 %** in `batt`. In a mixed fleet, use `volt` for battery and only treat `batt` as setpoint when value is in **20–46** and the device is known to run this fork with clock hidden.

### Home Assistant template example

```yaml
template:
  - sensor:
      - name: "Living room target temperature"
        state: "{{ (states('sensor.living_room_batt') | int(0)) / 2 }}"
        unit_of_measurement: "°C"
      - name: "Living room battery voltage"
        state: "{{ states('sensor.living_room_volt') }}"
        unit_of_measurement: "V"
```

MQTT topic (OMG): `home/<gateway>/BTtoMQTT/<MAC_NO_COLONS>`  
Example MAC `A4:C1:38:42:FE:ED` → `A4C13842FEED`.

### OMG gateway tips (Ethernet / fixed power)

Default BLE scan interval (~100 s) is conservative. For faster updates:

```json
{"interval":30000,"scanduration":12000,"save":true}
```

Publish to: `home/<OMG_gateway>/commands/MQTTtoBT/config`.

Thermometers advertise about every **5 s**; a **12 s** scan can miss a device occasionally — shorter `interval` helps.

---

## Implementation notes (developers)

| File | Purpose |
|------|---------|
| `src/room_setpoint.c` | Setpoint table, cycle, `room_sp_key_poll()` |
| `src/app.h` | `cfg_hide_clock()`, `room_sp_idx` in `cfg_t` |
| `src/app.c` | Defaults, OTA migration, KEY1 poll in `main_loop()`, PC4 wake |
| `src/custom_beacon.c` | `room_sp_pvvx_batt_byte()` in PVVX advert |
| `src/lcd_mjwsd05mmc*.c` | `show_s2_setpoint()` when clock hidden |
| `src/trigger.c` | `rs1_invert = 1` for PC4 active-low |

`cfg_hide_clock()` is `!cfg.flg.time_am_pm` (TelinkMiFlasher **12-hour clock** bit).

OTA from firmware ≤ 0x58: forces `time_am_pm = 0` and clears legacy `flg3` hide-clock bit.

---

## Troubleshooting

| Symptom | Check |
|---------|--------|
| Date still on LCD | **12-hour clock** must be **off** in flasher |
| Bottom button does nothing | Flash latest `setpoint-temperature` build; short press only |
| OMG sees device but `batt` = 93–100 | Stock / clock visible — not setpoint mode; use `volt` for battery |
| OMG never sees MAC | PVVX, no encryption, no Long Range; press **top** button; move closer; lower OMG `interval` |
| `batt` should be 36 for 18 °C but is 100 | Wrong firmware or clock not hidden on **that** MAC |

---

## Relation to upstream (pvvx)

This feature is specific to this fork (`philvana/ATC_MiThermometer`). Upstream may not accept the `batt` semantic change without a dedicated Theengs field. For upstream PR, a separate design (extra adv byte or BTHome object) would be needed.
