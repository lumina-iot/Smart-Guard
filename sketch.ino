#define BLYNK_TEMPLATE_ID "TMPL3wVJu4PUY"
#define BLYNK_TEMPLATE_NAME "Smart Guard"
#define BLYNK_AUTH_TOKEN "eY_EY9E6IDSRnR01DMOS2J1J8aR5O2o1"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// ---------- Wi-Fi ----------
char WIFI_SSID[] = "Wokwi-GUEST";
char WIFI_PASSWORD[] = "";

// ---------- Pin configuration ----------
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define MOTION_PIN 27
#define LED_PIN 18
#define BUZZER_PIN 19

// ---------- Objects ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;

// ---------- Alert settings ----------
const float HIGH_TEMP = 35.0;

// ---------- LCD helper ----------
void lcdLine(int row, String message) {
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
  lcd.print(message.substring(0, 16));
}

// ---------- Sensor reading ----------
void readSensors() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Button connected between GPIO 27 and GND
  bool motion = (digitalRead(MOTION_PIN) == LOW);

  bool sensorError = isnan(temperature) || isnan(humidity);
  bool highTemperature =
      !sensorError && temperature >= HIGH_TEMP;

  bool alert = motion || highTemperature;

  // Local LED and buzzer
  digitalWrite(LED_PIN, alert ? HIGH : LOW);

  if (alert) {
    tone(BUZZER_PIN, 2000);
  } else {
    noTone(BUZZER_PIN);
  }

  // LCD display
  if (sensorError) {
    lcdLine(0, "DHT22 ERROR");
    lcdLine(1, "Check sensor");
  } else {
    char line[17];
    snprintf(line, sizeof(line), "T:%.1fC H:%.0f%%",
             temperature, humidity);
    lcdLine(0, String(line));

    if (motion && highTemperature) {
      lcdLine(1, "MOTION+HIGH TEMP");
    } else if (motion) {
      lcdLine(1, "Motion: DETECTED");
    } else if (highTemperature) {
      lcdLine(1, "ALERT: HIGH TEMP");
    } else {
      lcdLine(1, "Status: NORMAL");
    }

    // Send readings to Blynk only when connected
    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, temperature);
      Blynk.virtualWrite(V1, humidity);
    }
  }

  // Send alert status to Blynk
  if (Blynk.connected()) {
    Blynk.virtualWrite(V2, motion ? 1 : 0);
    Blynk.virtualWrite(V3, highTemperature ? 1 : 0);
    Blynk.virtualWrite(V4, alert ? 1 : 0);
  }

  // Serial Monitor
  Serial.println("--- SMART GUARD ---");

  if (!sensorError) {
    Serial.printf("Temperature: %.1f C\n", temperature);
    Serial.printf("Humidity: %.1f %%\n", humidity);
  } else {
    Serial.println("DHT22 ERROR");
  }

  Serial.println(motion ? "Motion: DETECTED"
                        : "Motion: NOT DETECTED");
  Serial.println(alert ? "Status: ALERT" : "Status: NORMAL");
  Serial.println(Blynk.connected()
                     ? "Blynk: CONNECTED"
                     : "Blynk: DISCONNECTED");
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(MOTION_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  noTone(BUZZER_PIN);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  dht.begin();

  lcdLine(0, "SMARTGUARD IoT");
  lcdLine(1, "Connecting...");

  Serial.println("Starting Smart Guard...");

  // Connect to Blynk and Wokwi Wi-Fi
  Blynk.begin(BLYNK_AUTH_TOKEN,
              WIFI_SSID, WIFI_PASSWORD);

  lcdLine(0, "SMARTGUARD IoT");
  lcdLine(1, "Blynk Connected");

  // Read sensors every 2 seconds
  timer.setInterval(2000L, readSensors);
}

// ---------- Main loop ----------
void loop() {
  Blynk.run();
  timer.run();
}