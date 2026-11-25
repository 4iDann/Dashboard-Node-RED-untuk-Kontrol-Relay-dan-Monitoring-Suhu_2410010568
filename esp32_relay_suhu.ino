/*
 * Mini Proyek Materi 10 - Kontrol Relay & Monitoring Suhu (ESP32 + DHT22 + MQTT)
 *
 * Fitur:
 *  - Kirim suhu DHT22 tiap 3 detik  -> iot/2410010568/sensor/suhu
 *  - Terima "ON"/"OFF" untuk relay  <- iot/2410010568/aktuator/relay (GPIO 26)
 *  - Terima PWM 0-255 untuk LED     <- iot/2410010568/aktuator/pwm   (GPIO 2)
 *
 * Library (Library Manager): "PubSubClient" (Nick O'Leary),
 *                            "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"
 * Kompatibel dengan ESP32 Arduino core 2.x dan 3.x.
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ====== KONFIGURASI (ganti sesuai kebutuhan) ======
const char* WIFI_SSID = "Wokwi-GUEST";       // Wokwi: "Wokwi-GUEST"
const char* WIFI_PASS = "";   // Wokwi: ""
const char* MQTT_SERVER = "broker.hivemq.com";
const int   MQTT_PORT   = 1883;

const char* NIM = "2410010568";

// ====== PIN ======
#define DHT_PIN   4        // pin data DHT22
#define DHT_TYPE  DHT22
#define RELAY_PIN 26
#define LED_PIN   2

// ====== MODE SIMULASI SUHU ======
// 1 = suhu dibuat naik-turun acak (25-40 C) tiap 3 detik, untuk demo video
// 0 = baca suhu asli dari sensor DHT22
#define SIMULASI_SUHU 1

// ====== PWM ======
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;    // 8 bit -> 0..255
#if ESP_ARDUINO_VERSION_MAJOR < 3
const int PWM_CH = 0;
#endif

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient mqtt(espClient);

char topicSuhu[64], topicRelay[64], topicPwm[64];
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 3000;  // 3 detik

void pwmWrite(int duty) {
  duty = constrain(duty, 0, 255);
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(LED_PIN, duty);
#else
  ledcWrite(PWM_CH, duty);
#endif
}

void setupWiFi() {
  Serial.print("Menghubungkan WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi terhubung, IP: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  Serial.printf("Pesan [%s]: %s\n", topic, msg.c_str());

  if (strcmp(topic, topicRelay) == 0) {
    msg.toUpperCase();
    if (msg == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Relay -> ON");
    } else if (msg == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Relay -> OFF");
    }
  } else if (strcmp(topic, topicPwm) == 0) {
    int val = constrain(msg.toInt(), 0, 255);
    pwmWrite(val);
    Serial.printf("PWM LED -> %d\n", val);
  }
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Menghubungkan MQTT...");
    String clientId = String("esp32-") + NIM + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    if (mqtt.connect(clientId.c_str())) {
      Serial.println("terhubung");
      mqtt.subscribe(topicRelay);
      mqtt.subscribe(topicPwm);
    } else {
      Serial.printf("gagal (rc=%d), coba lagi 3 detik\n", mqtt.state());
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  snprintf(topicSuhu,  sizeof(topicSuhu),  "iot/%s/sensor/suhu", NIM);
  snprintf(topicRelay, sizeof(topicRelay), "iot/%s/aktuator/relay", NIM);
  snprintf(topicPwm,   sizeof(topicPwm),   "iot/%s/aktuator/pwm", NIM);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RES);
#else
  ledcSetup(PWM_CH, PWM_FREQ, PWM_RES);
  ledcAttachPin(LED_PIN, PWM_CH);
#endif
  pwmWrite(0);

  dht.begin();
  setupWiFi();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(callback);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) setupWiFi();
  if (!mqtt.connected()) reconnect();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastSend >= SEND_INTERVAL) {
    lastSend = now;
#if SIMULASI_SUHU
    static float simSuhu = 27.0;
    simSuhu += random(-20, 21) / 10.0;   // berubah -2.0 s/d +2.0 tiap kirim
    simSuhu = constrain(simSuhu, 25.0, 40.0);
    float t = simSuhu;
#else
    float t = dht.readTemperature();
#endif
    if (isnan(t)) {
      Serial.println("Gagal membaca DHT22");
      return;
    }
    char buf[10];
    dtostrf(t, 4, 1, buf);
    mqtt.publish(topicSuhu, buf);
    Serial.printf("Kirim suhu: %s C\n", buf);
  }
}