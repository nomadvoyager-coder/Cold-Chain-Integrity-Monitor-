#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWireNg_CurrentPlatform.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ── WIFI ──
const char* WIFI_SSID = "ask Shagill 4 password";
const char* WIFI_PASS = "testing1234";

// ── SUPABASE ──
const char* SB_URL = "https://tijcmtfbrmvzoyoqwqry.supabase.co/rest/v1/sensor_data";
const char* SB_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InRpamNtdGZicm12em95b3F3cXJ5Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzY4MjQyNzUsImV4cCI6MjA5MjQwMDI3NX0.eCvstSWuENCdhNm6ScfPAoOckvpynfYuSs7Dwsq0ksA";

#define ONE_WIRE_BUS    4
#define BUZZER_PIN     20
#define FAN_PIN        21
#define SDA_PIN         6
#define SCL_PIN         7
#define TEMP_THRESHOLD  8.0
#define RATE_THRESHOLD  0.5
#define UPLOAD_INTERVAL 30000UL  // push to Supabase every 30s

LiquidCrystal_I2C lcd(0x27, 16, 2);
OneWireNg_CurrentPlatform ow(ONE_WIRE_BUS, false);

float prevTemp = 0.0, rateOfRise = 0.0;
unsigned long prevRateTime = 0, lastUpload = 0;

float readTempC() {
  if (ow.reset() != OneWireNg::EC_SUCCESS) return -999.0;
  ow.writeByte(0xCC); ow.writeByte(0x44); delay(750);
  if (ow.reset() != OneWireNg::EC_SUCCESS) return -999.0;
  ow.writeByte(0xCC); ow.writeByte(0xBE);
  uint8_t lsb = ow.readByte(), msb = ow.readByte();
  ow.reset();
  return ((int16_t)((msb << 8) | lsb)) * 0.0625f;
}

void uploadToSupabase(float temp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, skipping upload.");
    return;
  }
  WiFiClientSecure client;
  client.setInsecure(); // skip SSL cert verification (fine for prototype)
  HTTPClient http;
  http.begin(client, SB_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("apikey", SB_KEY);
  http.addHeader("Authorization", String("Bearer ") + SB_KEY);
  http.addHeader("Prefer", "return=minimal");
  String body = "{\"temperature\":" + String(temp, 2) + "}";
  int code = http.POST(body);
  Serial.print("Supabase POST: "); Serial.println(code); // 201 = success
  http.end();
}

void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500); Serial.print("."); tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    lcd.setCursor(0,1); lcd.print("WiFi OK         ");
    delay(1000);
  } else {
    Serial.println("\nWiFi failed — running offline.");
    lcd.setCursor(0,1); lcd.print("No WiFi-Offline ");
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT); pinMode(FAN_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); digitalWrite(FAN_PIN, LOW);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(50000);
  lcd.init(); lcd.backlight();
  lcd.setCursor(0,0); lcd.print("ColdChain IoT");
  lcd.setCursor(0,1); lcd.print("Starting...");

  connectWiFi();

  float t = readTempC();
  if (t == -999.0) {
    lcd.clear(); lcd.setCursor(0,0); lcd.print("SENSOR ERROR!");
    Serial.println("ERROR: DS18B20 not found.");
    while (true) { digitalWrite(BUZZER_PIN,HIGH); delay(200); digitalWrite(BUZZER_PIN,LOW); delay(800); }
  }

  prevTemp = t; prevRateTime = millis(); lastUpload = millis();
  delay(500); lcd.clear();
  Serial.println("ColdChain Monitor Started.");
  Serial.println("Temp(C) | Rate(C/min) | Fan | Status");
}

void loop() {
  float temp = readTempC();

  if (temp == -999.0) {
    lcd.clear(); lcd.setCursor(0,0); lcd.print("SENSOR LOST!");
    Serial.println("ERROR: Sensor disconnected!");
    digitalWrite(BUZZER_PIN,HIGH); delay(200); digitalWrite(BUZZER_PIN,LOW);
    delay(2000); return;
  }

  unsigned long now = millis();

  // rate of rise
  if (now - prevRateTime >= 30000UL) {
    rateOfRise = (temp - prevTemp) / ((now - prevRateTime) / 60000.0);
    prevTemp = temp; prevRateTime = now;
  }
  if (temp < TEMP_THRESHOLD - 1.0) rateOfRise = 0.0;

  bool breach   = (temp > TEMP_THRESHOLD);
  bool preAlert = (!breach && rateOfRise > RATE_THRESHOLD);

  digitalWrite(FAN_PIN, breach ? HIGH : LOW);

  if (breach) {
    for (int i = 0; i < 2; i++) { digitalWrite(BUZZER_PIN,HIGH); delay(150); digitalWrite(BUZZER_PIN,LOW); delay(100); }
    delay(600);
  } else if (preAlert) {
    digitalWrite(BUZZER_PIN,HIGH); delay(60); digitalWrite(BUZZER_PIN,LOW); delay(1500);
  } else {
    digitalWrite(BUZZER_PIN,LOW); delay(250);
  }

  // LCD
  lcd.setCursor(0,0);
  lcd.print("Temp:"); lcd.print(temp,1); lcd.print((char)223); lcd.print("C   ");
  lcd.setCursor(0,1);
  if (breach)        lcd.print("!! BREACH !!    ");
  else if (preAlert) lcd.print("WARN:Rising Fast");
  else               lcd.print("Status: OK      ");

  // Serial
  Serial.print(temp,2); Serial.print(" C | ");
  Serial.print(rateOfRise,3); Serial.print(" C/min | Fan:");
  Serial.print(breach?"ON":"OFF"); Serial.print(" | ");
  Serial.println(breach?"BREACH":preAlert?"PRE-ALERT":"OK");

  // Supabase upload every 30s
  if (now - lastUpload >= UPLOAD_INTERVAL) {
    uploadToSupabase(temp);
    lastUpload = now;
  }
}