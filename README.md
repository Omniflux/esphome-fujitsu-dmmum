# Fujitsu AirStage-H Central Controller component for ESPHome

An ESPHome component to replace Fujitsu AirStage-H (product line previously known as Halcyon) central remote controllers.

Should be compatible with remote control models UTY-DMMUM, UTY-DMMYM, UTY-DMMGM, and UTY-DMMXM.

```yaml
substitutions:
  device_name: halcyon-ou
  friendly_name: Fujitsu Halcyon
  device_description: Atom Lite + FOSV
  esp_board: m5stack-atom

external_components:
  - source: github://Omniflux/esphome-tzsp
  - source: github://Omniflux/esphome-fujitsu-dmmum

packages:
  wifi: !include common/wifi.yaml

  central_controller:
    url: https://github.com/Omniflux/esphome-fujitsu-dmmum
    files:
      - path: central_controller.yaml
      - path: indoor_unit.yaml
        vars:
          unit_number: 1
          id: indoor_unit_1
          name: Indoor Unit 1
      - path: indoor_unit.yaml
        vars:
          unit_number: 2
          id: indoor_unit_2
          name: Indoor Unit 2

uart:
  tx_pin: GPIO22  # Device dependent
  rx_pin: GPIO19  # Device dependent
  baud_rate: 500
  parity: EVEN
  rx_full_threshold: 120

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}
  comment: ${device_description}

esp32:
  board: ${esp_board}
  framework:
    type: esp-idf

api:

ota:
  - platform: esphome
    password: !secret ota_password

#logger:
#  level: DEBUG

button:
  - platform: restart
    name: Restart
  - platform: safe_mode
    name: Restart (Safe Mode)

sensor:
  - platform: uptime
    name: Uptime
```

You can use esphome (or Home Assistant) sensors to report the current temperature and humidity to the Home Assistant climate component

```yaml
sensor:
  - platform: homeassistant # https://esphome.io/components/sensor/homeassistant.html
    id: indoor_unit_1_sensor_temperature # ESPHome sensor name used below
    entity_id: sensor.room_1_sensor_temperature # Home Assistant entity_id
    filters: # Home Assistant sensor data is °C, optionally convert to °F 
      - lambda: return x * (9.0/5.0) + 32.0;
    unit_of_measurement: "°F"

  - platform: homeassistant
    id: indoor_unit_1_sensor_humidity
    entity_id: sensor.room_1_sensor_humidity

climate:
  - id: !extend indoor_unit_1
    sensor: indoor_unit_1_sensor_temperature
    humidity_sensor: indoor_unit_1_sensor_humidity
```

Configure TZSP and use Wireshark with [fujitsu-airstage-h-dissector](https://github.com/Omniflux/fujitsu-airstage-h-dissector) to debug / decode the Fujitsu serial protocol.

```yaml
fujitsu_general_airstage_h_central_controller:
  tzsp:
    ip: 192.168.12.23
    protocol: 254
```

## Related projects
- FOSV's [Fuji-Atom-Interface](https://github.com/FOSV/Fuji-Atom-Interface) - Open hardware interface compatible with this component
<!-- -->
- My [esphome-fujitsu-halcyon](https://github.com/Omniflux/esphome-fujitsu-halcyon) - Fujitsu AirStage-H 3-wire Individual Controller component for ESPHome
<!-- -->
- Aaron Zhang's [esphome-fujitsu](https://github.com/FujiHeatPump/esphome-fujitsu)
- Jaroslaw Przybylowicz's [fuji-iot](https://github.com/jaroslawprzybylowicz/fuji-iot)
- Raal Goff's [FujiHeatPump](https://github.com/unreality/FujiHeatPump)
- Raal Goff's [FujiHK](https://github.com/unreality/FujiHK)
<!-- -->
- Myles Eftos's [Reverse engineering](https://hackaday.io/project/19473-reverse-engineering-a-fujitsu-air-conditioner-unit)
