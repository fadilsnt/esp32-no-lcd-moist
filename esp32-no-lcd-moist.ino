#include <WiFi.h>
#include <HTTPClient.h>

#include <max6675.h>
#include <PZEM004Tv30.h>

// ================= WIFI =================
const char* ssid     = "7k produksi";
const char* password = "tujukuda777";

// const char* ssid     = "LANTAI BAWAH_";
// const char* password = "tujukuda12345";

// const char* ssid     = "kudatuju";
// const char* password = "abcd12345";

const char* endpoint = "https://mgt.iot7k.com/api/machine-readings";

// ================= MAX6675 =================
#define THERMO_SCK 18
#define THERMO_CS  5
#define THERMO_SO  19
MAX6675 thermocouple(THERMO_SCK, THERMO_CS, THERMO_SO);

// =====HIGROMETER =================
// #define HIGRO 34

// ================= PZEM =================
#define PZEM_RX 16
#define PZEM_TX 17
HardwareSerial pzemSerial(2);
PZEM004Tv30 pzem(pzemSerial, PZEM_RX, PZEM_TX);

// ================= TIMING =================
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 10000; // jeda 1 menit

// ================= HELPER FUNCTION =================
// Fungsi untuk mengecek dan mengganti NaN dengan 0
float safeValue(float value) {
  return isnan(value) ? 0.0 : value;
}

void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("WiFi connecting");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  delay(1000);
}

void setup() {
  Serial.begin(115200);

  // PZEM
  pzemSerial.begin(9600, SERIAL_8N1, PZEM_RX, PZEM_TX);

  connectWiFi();
}

void loop() {
  // ===== GEt data sensor  =====
  float temp = thermocouple.readCelsius();
  // int soil = analogRead(HIGRO);
  int soil = 0;

  float voltage = pzem.voltage();
  float current = pzem.current();
  float power   = pzem.power();
  float energy  = pzem.energy();

  const char* machine_id_val = "MESIN-20";
  // ===== SEND HTTP POST=====
  if (millis() - lastSend > SEND_INTERVAL) {
    lastSend = millis();

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      
      // Gunakan endpoint yang sama
      http.begin(endpoint);
      
      // Tambahkan ini untuk mengatasi error 302 dan masalah SSL
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
      // http.setInsecure(); // Aktifkan jika masih ada kendala koneksi HTTPS

      http.addHeader("Content-Type", "application/json");

      // 2. Susun Payload sesuai variabel API Server dengan pengecekan NaN
      float safeVoltage = safeValue(voltage);
      float safeCurrent = safeValue(current);
      float safePower = safeValue(power);
      float safeTemp = safeValue(temp);
      
      String payload = "{";
      payload += "\"machine_id\":\"" + String(machine_id_val) + "\",";
      payload += "\"amp\":"        + String(safeCurrent, 3) + ","; // PZEM current
      payload += "\"hm\":"         + String(safePower, 2)   + ","; // PZEM power -> hm
      payload += "\"temp\":"       + String(safeTemp, 2)    + ","; // MAX6675 temp
      payload += "\"moist\":"      + String(soil);                 // Higrometer -> moist
      payload += "}";

      int code = http.POST(payload);
      
      // Serial monitor untuk debugging
      Serial.print("Payload: ");
      Serial.println(payload);
      Serial.print("HTTP Code: ");
      Serial.println(code);

      if (code > 0) {
        String response = http.getString();
        Serial.println("Response: " + response);
      } else {
        Serial.print("Error: ");
        Serial.println(http.errorToString(code).c_str());
      }

      http.end();
    }
  }

  delay(500);
}
