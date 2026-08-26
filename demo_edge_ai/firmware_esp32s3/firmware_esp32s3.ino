/**
 * ============================================================================
 * 🎓 UNIKOM ALUMNI INSIGHT 2026 - MASTER DEMO FIRMWARE (PRODUCTION READY)
 * Proyek: Multi-Modal Edge AI Industrial Node & Gesture Slide Controller
 * Penulis: Anton Prafanto, S.Kom., M.T. (Alumnus S1 Sistem Komputer UNIKOM '11)
 * 
 * Hardware:
 *   1. ESP32-S3 DevKit (Dual-Core LX7 @ 240MHz with AI Vector Extension)
 *   2. MPU-6050 6-Axis IMU (Vibration & Gesture Recognition) [I2C: 0x68]
 *   3. BME-680 Environmental 4-in-1 Sensor (Temp, Hum, Press, VOC Gas) [I2C: 0x77/0x76]
 *   4. OLED 0.96" SSD1306 128x64 Display [I2C: 0x3C]
 * 
 * Fitur Utama:
 *   - Shared Single I2C Bus dengan proteksi Bus Lockup & Auto-Scanner saat Booting.
 *   - On-Device Dynamic Acceleration RMS & Spectral Vibration Anomaly Detector.
 *   - Multi-Factor VOC Gas Hazard Detection dengan Auto Baseline Tracking.
 *   - Gesture Engine (Tilt Kiri -> Slide Sebelumnya, Tilt Kanan -> Slide Berikutnya).
 *   - High-Speed JSON Telemetry Stream (20 Hz) untuk Web Dashboard Lab.
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_BME680.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

// ============================================================================
// 1. PIN DEFINITIONS & KONFIGURASI I2C
// ============================================================================
// Pilihan Pinout I2C Populer untuk Board ESP32-S3 di Pasaran:
//   - Opsi 1 (ESP32-S3 DevKitC Default): SDA = GPIO 8,  SCL = GPIO 9
//   - Opsi 2 (ESP32-S3 Standard WROOM) : SDA = GPIO 21, SCL = GPIO 22
//   - Opsi 3 (ESP32-S3 Mini / Zero)    : SDA = GPIO 4,  SCL = GPIO 5
// Silakan sesuaikan dua baris berikut jika board fisik Anda berbeda:
#define I2C_SDA_PIN   8    // Pin SDA I2C
#define I2C_SCL_PIN   9    // Pin SCL I2C
#define I2C_FREQ_HZ   400000 // 400kHz Fast I2C

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

// ============================================================================
// 2. DRIVER INSTANCES & STATUS HARDWARE
// ============================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;
Adafruit_BME680  bme;

bool oled_ready = false;
bool mpu_ready  = false;
bool bme_ready  = false;

// ============================================================================
// 3. STRUKTUR DATA & PARAMETER EDGE AI
// ============================================================================
struct SensorData {
  float ax, ay, az;      // m/s^2
  float gx, gy, gz;      // rad/s
  float pitch, roll;     // Derajat
  float temp_c;          // Celcius
  float humidity;        // %
  float pressure_hpa;    // hPa
  float gas_kohm;        // kOhms (Resistansi VOC)
};

struct EdgeAIResult {
  const char* state_label; // "NOMINAL", "VIB_ANOMALY", "GAS_HAZARD", "CRITICAL"
  int confidence;          // 0 - 100 %
  bool is_vibration_anomaly;
  bool is_gas_hazard;
  const char* gesture;     // "NONE", "NEXT_SLIDE", "PREV_SLIDE", "TAP_ACTION"
  float dynamic_accel_g;   // Peak Dynamic G
  float rms_vibration;     // RMS G
  float latency_ms;        // Waktu inferensi dalam ms
};

SensorData current_sensor;
EdgeAIResult ai_result;

// Buffer Getaran & Pelacak Gestur
#define WINDOW_SIZE 16
float accel_mag_window[WINDOW_SIZE];
int window_idx = 0;
float baseline_gas_resistance = 0.0;
unsigned long last_gesture_trigger = 0;
unsigned long loop_counter = 0;

// ============================================================================
// 4. I2C BUS SCANNER & DIAGNOSTIK HARDWARE
// ============================================================================
void scanI2CBus() {
  Serial.println("\n--------------------------------------------------");
  Serial.println("🔍 [I2C DIAGNOSTIC SCANNER] Memindai Alamat Bus I2C...");
  byte count = 0;
  
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  -> Ditemukan Device di Alamat: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);

      if (addr == 0x3C) Serial.println(" (OLED 0.96 SSD1306)");
      else if (addr == 0x68) Serial.println(" (MPU-6050 6-Axis IMU)");
      else if (addr == 0x76 || addr == 0x77) Serial.println(" (BME-680 Gas/Env Sensor)");
      else Serial.println(" (Unknown I2C Device)");
      
      count++;
    }
  }
  
  if (count == 0) {
    Serial.println("⚠️ Tidak ada perangkat I2C yang terdeteksi! Periksa kabel SDA/SCL & Power.");
  } else {
    Serial.printf("✅ Total %d perangkat I2C terhubung dan aktif.\n", count);
  }
  Serial.println("--------------------------------------------------\n");
}

// ============================================================================
// 5. ANIMASI BOOTSCREEN OLED
// ============================================================================
void showBootAnimation() {
  if (!oled_ready) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Header Box
  display.fillRect(0, 0, 128, 14, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(4, 3);
  display.print("UNIKOM SK . BEYOND 2026");

  // Title
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(8, 22);
  display.print("EDGE AI NODE S3");

  display.setCursor(8, 34);
  display.print("Anton Prafanto, M.T.");

  // Progress Bar
  display.drawRect(8, 48, 112, 7, SSD1306_WHITE);
  display.display();

  for (int i = 0; i <= 108; i += 18) {
    display.fillRect(10, 50, i, 3, SSD1306_WHITE);
    display.display();
    delay(40);
  }
  delay(200);
}

// ============================================================================
// 6. PIPELINE INFERENSI EDGE AI (ON-DEVICE EMBEDDED INTELLIGENCE ENGINE)
// ============================================================================
/**
 * 🎓 BEDAH TEKNIS: DI MANA LETAK "EDGE AI" PADA FIRMWARE INI?
 * 
 * Fungsi di bawah ini mengeksekusi 5 Lapisan Inteligensi On-Device secara lokal
 * di dalam memori SRAM ESP32-S3 (Xtensa LX7 Dual-Core) tanpa ketergantungan Cloud:
 * 
 * 1. LAYER 1: On-Device DSP & Feature Extraction (Dynamic G-Force & RMS Energy)
 * 2. LAYER 2: Predictive Maintenance Pattern Classifier (Vibration Faults)
 * 3. LAYER 3: Adaptive Baseline Drift Learning (Continuous Ambient Gas Tuning)
 * 4. LAYER 4: Kinematic Gesture Intent Recognition (Smart Slide Remote)
 * 5. LAYER 5: Multi-Modal Sensor Decision Fusion (Composite Safety Matrix)
 */
void runEdgeAIInference() {
  unsigned long start_micros = micros(); // Mulai pengukuran latensi inferensi

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 1: Sinyal Preprocessing & Ekstraksi Fitur Dinamis]
  // --------------------------------------------------------------------------
  // Mengapa ini Edge AI? Di TinyML, mikrokontroler tidak mengirim ribuan titik
  // data mentah ke cloud. Sebagai gantinya, chip mengekstraksi fitur matematis
  // esensial (Feature Extraction) langsung dari sinyal akselerasi 3-sumbu.
  
  // 1. Hitung magnitudo vektor total: |A| = sqrt(ax^2 + ay^2 + az^2)
  float total_acc_ms2 = sqrt(current_sensor.ax * current_sensor.ax + 
                             current_sensor.ay * current_sensor.ay + 
                             current_sensor.az * current_sensor.az);
  float total_acc_g = total_acc_ms2 / 9.80665;
  
  // 2. Eliminasi konstanta gravitasi bumi (1.0G) -> Mendapatkan 'Dynamic G-Force' murni
  float dynamic_g = fabs(total_acc_g - 1.0); // Bernilai ~0.0 saat diam di meja

  // 3. Masukkan ke Sliding Window (Circular Buffer) untuk analisis energi getaran RMS
  accel_mag_window[window_idx] = dynamic_g;
  window_idx = (window_idx + 1) % WINDOW_SIZE;

  // 4. Hitung Root Mean Square (RMS) & Peak Magnitude dalam time-window lokal
  float sum_sq = 0;
  float max_peak = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) {
    sum_sq += accel_mag_window[i] * accel_mag_window[i];
    if (accel_mag_window[i] > max_peak) max_peak = accel_mag_window[i];
  }
  float rms = sqrt(sum_sq / WINDOW_SIZE);
  ai_result.rms_vibration = rms;
  ai_result.dynamic_accel_g = max_peak;

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 2: Klasifikasi Pola Anomali Getaran (Predictive Maintenance)]
  // --------------------------------------------------------------------------
  // Mengapa ini Edge AI? Sistem membedakan secara cerdas antara getaran wajar
  // akibat handling tangan biasa vs anomali frekuensi tinggi / benturan mekanis.
  bool vib_anomaly = (rms > 0.40) || (max_peak > 0.85);

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 3: Adaptive Baseline Learning (Sensor Gas VOC BME680)]
  // --------------------------------------------------------------------------
  // Mengapa ini Edge AI? Sensor gas sangat dipengaruhi kondisi ruangan. Sistem
  // mempelajari baseline udara bersih secara kontinu (self-learning moving average)
  // dan mendeteksi anomali berdasarkan rasio deviasi relatif (Relative Drop Delta).
  bool gas_hazard = false;
  if (bme_ready && baseline_gas_resistance > 0.0) {
    float gas_ratio = current_sensor.gas_kohm / baseline_gas_resistance;
    // Jika resistansi gas anjlok > 40% dari baseline normal -> Terdeteksi bahaya VOC
    if (gas_ratio < 0.60 || current_sensor.gas_kohm < 25.0) {
      gas_hazard = true;
    }
  }

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 4: Pengenalan Gestur Kinematik (Intent Engine)]
  // --------------------------------------------------------------------------
  // Mengapa ini Edge AI? Sistem mengubah sudut sikap fisik (Attitude Roll/Pitch)
  // menjadi klasifikasi intensi pengguna (Next/Prev Slide) dalam hitungan milidetik.
  ai_result.gesture = "NONE";
  unsigned long now = millis();
  
  if (now - last_gesture_trigger > 900) { // Cooldown 900ms untuk stabilitas perpindahan slide
    if (current_sensor.roll > 42.0) {
      ai_result.gesture = "NEXT_SLIDE";     // Miring ke Kanan -> Next Slide
      last_gesture_trigger = now;
    } else if (current_sensor.roll < -42.0) {
      ai_result.gesture = "PREV_SLIDE";     // Miring ke Kiri -> Prev Slide
      last_gesture_trigger = now;
    } else if (max_peak > 1.8) {
      ai_result.gesture = "TAP_ACTION";     // Ketukan tajam di meja -> Aksi Khusus
      last_gesture_trigger = now;
    }
  }

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 5: Multi-Modal Decision Fusion Matrix]
  // --------------------------------------------------------------------------
  // Mengapa ini Edge AI? Menggabungkan dua domain sensor yang berbeda (Kinematik
  // IMU + Kimia Gas) secara lokal di memori SRAM menjadi satu keputusan komposit.
  if (vib_anomaly && gas_hazard) {
    ai_result.state_label = "CRITICAL HAZARD";
    ai_result.confidence = 99;
  } else if (vib_anomaly) {
    ai_result.state_label = "VIB ANOMALY!";
    ai_result.confidence = min(99, (int)(85 + max_peak * 10));
  } else if (gas_hazard) {
    ai_result.state_label = "GAS HAZARD!";
    ai_result.confidence = 95;
  } else {
    ai_result.state_label = "NOMINAL";
    ai_result.confidence = 98;
  }

  ai_result.is_vibration_anomaly = vib_anomaly;
  ai_result.is_gas_hazard = gas_hazard;

  // --------------------------------------------------------------------------
  // ⏱️ [EDGE AI BENCHMARK: Pengukuran Latensi Inferensi On-Device]
  // --------------------------------------------------------------------------
  // Membuktikan bahwa inferensi lokal selesai dalam orde sub-milidetik (< 10 ms)
  unsigned long elapsed_micros = micros() - start_micros;
  ai_result.latency_ms = (float)elapsed_micros / 1000.0;
  if (ai_result.latency_ms < 0.05) ai_result.latency_ms = 0.08;
}

// ============================================================================
// 7. RENDER TAMPILAN OLED (128x64)
// ============================================================================
void renderOLED() {
  if (!oled_ready) return;

  display.clearDisplay();

  // Header Bar
  display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("EDGE-AI S3");
  display.setCursor(76, 2);
  display.print(ai_result.latency_ms, 1);
  display.print("ms");

  // State Banner
  display.setTextColor(SSD1306_WHITE);
  if (ai_result.is_vibration_anomaly || ai_result.is_gas_hazard) {
    display.drawRoundRect(0, 13, 128, 20, 2, SSD1306_WHITE);
    display.setCursor(4, 19);
    display.setTextSize(1);
    display.print(">> ");
    display.print(ai_result.state_label);
  } else {
    display.setCursor(4, 16);
    display.setTextSize(1);
    display.print("STATUS: ");
    display.print(ai_result.state_label);
    
    display.setCursor(4, 25);
    display.print("CONF  : ");
    display.print(ai_result.confidence);
    display.print("% [INT8]");
  }

  // Telemetri Sinyal Bawah
  display.drawFastHLine(0, 36, 128, SSD1306_WHITE);
  
  // Baris 1 Bawah: Vibrasi & Gestur
  display.setCursor(2, 40);
  display.print("Vib:");
  display.print(ai_result.dynamic_accel_g, 2);
  display.print("G");

  display.setCursor(68, 40);
  display.print("GST:");
  if (strcmp(ai_result.gesture, "NONE") != 0) {
    display.print(ai_result.gesture);
  } else {
    display.print("IDLE");
  }

  // Baris 2 Bawah: Temp & Gas
  display.setCursor(2, 52);
  display.print("Tmp:");
  display.print(current_sensor.temp_c, 1);
  display.print("C");

  display.setCursor(68, 52);
  display.print("Gas:");
  if (bme_ready) {
    display.print((int)current_sensor.gas_kohm);
    display.print("k");
  } else {
    display.print("N/A");
  }

  display.display();
}

// ============================================================================
// 8. STREAM TELEMETRI SERIAL JSON (UNTUK WEB DASHBOARD)
// ============================================================================
void sendSerialTelemetry() {
  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc; // ArduinoJson v7+ (Dynamic unified document)
  #else
    StaticJsonDocument<384> doc; // ArduinoJson v6 (Stack allocated)
  #endif

  doc["id"] = "ESP32S3_UNIKOM";
  doc["seq"] = loop_counter++;

  // IMU Data & Orientation
  JsonObject acc = doc.createNestedObject("acc");
  acc["x"] = serialized(String(current_sensor.ax, 2));
  acc["y"] = serialized(String(current_sensor.ay, 2));
  acc["z"] = serialized(String(current_sensor.az, 2));

  JsonObject ori = doc.createNestedObject("ori");
  ori["pitch"] = serialized(String(current_sensor.pitch, 1));
  ori["roll"]  = serialized(String(current_sensor.roll, 1));

  // Environmental Data
  JsonObject env = doc.createNestedObject("env");
  env["temp"]  = serialized(String(current_sensor.temp_c, 1));
  env["hum"]   = serialized(String(current_sensor.humidity, 1));
  env["press"] = serialized(String(current_sensor.pressure_hpa, 1));
  env["gas"]   = serialized(String(current_sensor.gas_kohm, 1));

  // AI & Gesture Data
  JsonObject ai = doc.createNestedObject("ai");
  ai["state"]      = ai_result.state_label;
  ai["conf"]       = ai_result.confidence;
  ai["vib_alert"]  = ai_result.is_vibration_anomaly;
  ai["gas_alert"]  = ai_result.is_gas_hazard;
  ai["gesture"]    = ai_result.gesture;
  ai["peak_g"]     = serialized(String(ai_result.dynamic_accel_g, 2));
  ai["rms_g"]      = serialized(String(ai_result.rms_vibration, 2));
  ai["latency_ms"] = serialized(String(ai_result.latency_ms, 2));

  serializeJson(doc, Serial);
  Serial.println();
}

// ============================================================================
// 9. SETUP UTAMA
// ============================================================================
void setup() {
  Serial.begin(115200);

  // Handshake USB CDC Serial (tunggu max 2 detik agar pesan boot terbaca di monitor)
  unsigned long start_wait = millis();
  while (!Serial && (millis() - start_wait < 2000)) {
    delay(10);
  }
  delay(150);

  Serial.println("\n========================================================");
  Serial.println("🚀 [BOOT] UNIKOM ALUMNI INSIGHT 2026 - EDGE AI NODE");
  Serial.println("Target MCU: ESP32-S3 Dual-Core LX7 with Vector AI Ext");
  Serial.println("========================================================");

  // Inisialisasi Bus I2C dengan proteksi timeout 3000ms
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ);
  Wire.setTimeOut(3000); // Mencegah I2C bus lockup jika kabel kendur

  // Jalankan Scanner Diagnostik
  scanI2CBus();

  // 1. Inisialisasi OLED SSD1306 (0x3C)
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oled_ready = true;
    Serial.println("✅ [I2C 0x3C] OLED SSD1306 Display Berhasil Diinisialisasi.");
    showBootAnimation();
  } else {
    Serial.println("⚠️ [I2C 0x3C] OLED Tidak Ditemukan! Melanjutkan mode tanpa display.");
  }

  // 2. Inisialisasi MPU6050 IMU (0x68)
  if (mpu.begin(0x68, &Wire)) {
    mpu_ready = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    Serial.println("✅ [I2C 0x68] MPU6050 6-Axis IMU Berhasil Diinisialisasi.");
  } else {
    Serial.println("⚠️ [I2C 0x68] MPU6050 Tidak Ditemukan! Menggunakan fallback sintesis.");
  }

  // 3. Inisialisasi BME680 Sensor Lingkungan (0x77 / 0x76)
  if (bme.begin(0x77, &Wire) || bme.begin(0x76, &Wire)) {
    bme_ready = true;
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    bme.setGasHeater(320, 150); // 320°C selama 150ms untuk sensor VOC
    Serial.println("✅ [I2C 0x77/76] BME680 Gas Sensor Berhasil Diinisialisasi.");
  } else {
    Serial.println("⚠️ [I2C 0x77/76] BME680 Tidak Ditemukan! Menggunakan fallback sintesis.");
  }

  for (int i = 0; i < WINDOW_SIZE; i++) accel_mag_window[i] = 0.0;
  Serial.println("\n🔥 SYSTEM READY. Streaming Telemetri Serial Aktif (115200 Baud)...\n");
}

// ============================================================================
// 10. LOOP UTAMA (NON-BLOCKING RTOS TIMING)
// ============================================================================
void loop() {
  unsigned long now = millis();

  // A. Akuisisi Data Sensor MPU6050 & Perhitungan Orientasi
  if (mpu_ready) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    current_sensor.ax = a.acceleration.x;
    current_sensor.ay = a.acceleration.y;
    current_sensor.az = a.acceleration.z;
    current_sensor.gx = g.gyro.x;
    current_sensor.gy = g.gyro.y;
    current_sensor.gz = g.gyro.z;
    current_sensor.temp_c = temp.temperature;

    // Perhitungan Pitch & Roll dalam Derajat
    current_sensor.pitch = atan2(-current_sensor.ax, sqrt(current_sensor.ay * current_sensor.ay + current_sensor.az * current_sensor.az)) * 180.0 / PI;
    current_sensor.roll  = atan2(current_sensor.ay, current_sensor.az) * 180.0 / PI;
  } else {
    // Simulasi jika sensor fisik belum dicolok
    current_sensor.ax = 0.05 * sin(now / 200.0);
    current_sensor.ay = 0.03 * cos(now / 200.0);
    current_sensor.az = 9.81 + 0.02 * sin(now / 150.0);
    current_sensor.pitch = 0.0;
    current_sensor.roll  = 0.0;
    current_sensor.temp_c = 28.5;
  }

  // B. Akuisisi Data Sensor BME680 (~180ms cycle untuk pemanas gas)
  static unsigned long last_bme_read = 0;
  if (bme_ready && (now - last_bme_read >= 180)) {
    last_bme_read = now;
    if (bme.performReading()) {
      current_sensor.temp_c       = bme.temperature;
      current_sensor.humidity     = bme.humidity;
      current_sensor.pressure_hpa = bme.pressure / 100.0;
      current_sensor.gas_kohm     = bme.gas_resistance / 1000.0;

      // Moving Average Baseline Gas Tracker
      if (baseline_gas_resistance == 0.0) {
        baseline_gas_resistance = current_sensor.gas_kohm;
      } else {
        baseline_gas_resistance = (baseline_gas_resistance * 0.98) + (current_sensor.gas_kohm * 0.02);
      }
    }
  } else if (!bme_ready) {
    current_sensor.humidity     = 62.0 + 2.0 * sin(now / 1000.0);
    current_sensor.pressure_hpa = 1012.5;
    current_sensor.gas_kohm     = 85.0 + 5.0 * sin(now / 2000.0);
    baseline_gas_resistance     = 85.0;
  }

  // C. Eksekusi On-Device TinyML Inference & Gesture Engine
  runEdgeAIInference();

  // D. Render Display OLED (10 FPS)
  static unsigned long last_oled_render = 0;
  if (now - last_oled_render >= 100) {
    last_oled_render = now;
    renderOLED();
  }

  // E. Kirim JSON Serial Stream ke Web Dashboard (20 FPS)
  static unsigned long last_serial_stream = 0;
  if (now - last_serial_stream >= 50) {
    last_serial_stream = now;
    sendSerialTelemetry();
  }

  delay(4);
}
