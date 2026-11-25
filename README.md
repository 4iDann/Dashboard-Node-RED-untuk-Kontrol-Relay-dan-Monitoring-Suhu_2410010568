# Dashboard-Node-RED-untuk-Kontrol-Relay-dan-Monitoring-Suhu_2410010568

Mini proyek IoT: ESP32 + DHT22 + Node-RED Dashboard via MQTT (HiveMQ public broker).

## Isi repo
- `flow_nodered_relay_suhu.json` : ekspor flow Node-RED
- `esp32_relay_suhu.ino` : kode ESP32
- `screenshot_dashboard.png` : screenshot dashboard
- `video_demo.mp4` : video demo (maks. 60 detik)

## Topik MQTT
| Fungsi | Topik | Arah |
|---|---|---|
| Suhu | `iot/<NIM>/sensor/suhu` | ESP32 -> Node-RED |
| Relay (ON/OFF) | `iot/<NIM>/aktuator/relay` | Node-RED -> ESP32 |
| PWM (0-255) | `iot/<NIM>/aktuator/pwm` | Node-RED -> ESP32 |

## Pin
DHT22 = GPIO 4, Relay = GPIO 26, LED PWM = GPIO 2
