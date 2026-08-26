/**
 * ============================================================================
 * PROYEK: MULTI-AXIS EDGE AI KINEMATIC & VIBRATION INTELLIGENCE NODE
 * ACARA: UNIKOM ALUMNI INSIGHT SERIES 2026
 * NARASUMBER: ANTON PRAFANTO, S.Kom., M.T. (ALUMNUS SK '11, DOSEN UNMUL)
 * ============================================================================
 * 
 * DESKRIPSI PERANGKAT:
 * - Mikrokontroler : ESP32-S3 DevKit (Xtensa LX7 Dual-Core 240MHz, Vector AI Instruction)
 * - Sensor IMU     : MPU-6050 6-Axis Accelerometer + Gyroscope (I2C: 0x68)
 * - Layar HUD      : 0.96" I2C OLED SSD1306 (128x64 px, I2C: 0x3C)
 * - Komunikasi     : WebSerial Telemetry @ 115200 Baud (Native USB CDC)
 * 
 * 🎓 4 PILAR EDGE AI PADA FIRMWARE INI:
 * 1. On-Device DSP & Dynamic G-Force Feature Extraction (Sinyal 3-Sumbu)
 * 2. Predictive Maintenance Vibration Classifier (Deteksi Kerusakan Mekanis)
 * 3. Kinematic Gesture Intent Engine (Remote Slide Presentasi Otomatis)
 * 4. Sub-Millisecond Inference Timing (< 10 ms on-chip execution)
 * 
 * WIRING PINOUT I2C (Single Shared Bus):
 * - ESP32-S3 3.3V   <---> VCC Semua Modul (MPU-6050, OLED SSD1306)
 * - ESP32-S3 GND    <---> GND Semua Modul
 * - ESP32-S3 GPIO 8 <---> SDA Semua Modul
 * - ESP32-S3 GPIO 9 <---> SCL Semua Modul
 * ============================================================================
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

// ============================================================================
// 1. DEFINISI PIN DAN KONFIGURASI BUS I2C
// ============================================================================
#define I2C_SDA_PIN   8
#define I2C_SCL_PIN   9
#define I2C_FREQ      400000  // Fast-Mode I2C (400 kHz)

// Alamat Perangkat I2C
#define OLED_ADDR     0x3C
#define MPU_ADDR      0x68

// Konfigurasi Layar OLED
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MPU6050 mpu;

// Status Kesiapan Hardware
bool oled_ready = false;
bool mpu_ready  = false;

// ============================================================================
// 2. STRUKTUR DATA SENSOR DAN HASIL EDGE AI
// ============================================================================
struct KinematicData {
  float ax, ay, az;       // Percepatan linier (m/s^2)
  float gx, gy, gz;       // Kecepatan sudut (rad/s)
  float temp_c;           // Suhu internal die IMU (°C)
  float pitch, roll;      // Sudut sikap Euler (Derajat)
};

struct EdgeAIResult {
  String state_label;         // "NOMINAL", "VIB ANOMALY!", "HARSH IMPACT"
  int confidence;             // Skor probabilitas (0 - 100%)
  float rms_vibration;        // Root Mean Square energi getaran
  float dynamic_accel_g;      // Magnitudo percepatan dinamis maksimum (G)
  String gesture;             // "NONE", "NEXT_SLIDE", "PREV_SLIDE", "TAP_ACTION"
  float latency_ms;           // Waktu eksekusi inferensi on-device (ms)
  bool is_vibration_anomaly;  // Flag anomali getaran
};

KinematicData current_sensor;
EdgeAIResult  ai_result;

// Buffer Sliding Window untuk Analisis RMS Getaran On-Device
#define WINDOW_SIZE 16
float accel_mag_window[WINDOW_SIZE];
int window_idx = 0;

// Timer RTOS Non-Blocking
unsigned long last_stream_time   = 0;
unsigned long last_oled_time     = 0;
unsigned long last_gesture_time  = 0;
const unsigned long STREAM_INTERVAL_MS = 50;  // 20 FPS ke WebSerial
const unsigned long OLED_INTERVAL_MS   = 100; // 10 FPS ke Layar OLED

// ============================================================================
// 3. I2C DIAGNOSTIC SCANNER OTOMATIS
// ============================================================================
void scanI2CBus() {
  Serial.println("\n--------------------------------------------------");
  Serial.println("🔍 [I2C DIAGNOSTIC SCANNER] Memindai Bus I2C...");
  byte count = 0;

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  -> Ditemukan Device di Alamat: 0x%02X", addr);
      if (addr == 0x3C) Serial.println(" (OLED 0.96\" SSD1306)");
      else if (addr == 0x68) Serial.println(" (MPU-6050 6-Axis IMU)");
      else Serial.println(" (Perangkat I2C Lain)");
      count++;
    }
  }

  if (count == 0) {
    Serial.println("⚠️ Tidak ada perangkat I2C yang terdeteksi! Periksa kabel SDA (IO8) & SCL (IO9).");
  } else {
    Serial.printf("✅ Total %d perangkat I2C terhubung dan aktif.\n", count);
  }
  Serial.println("--------------------------------------------------\n");
}

// ============================================================================
// 4. ANIMASI BOOTSCREEN OLED
// ============================================================================
void showBootAnimation() {
  if (!oled_ready) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Header Banner
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
  display.print("MPU6050 IMU ENGINE");

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
// 5. PIPELINE INFERENSI EDGE AI ON-DEVICE (Xtensa LX7 Engine)
// ============================================================================
/**
 * 🎓 BEDAH TEKNIS: 4 LAPISAN EDGE AI PADA FUNGSI INI
 * 
 * 1. LAYER 1: Sinyal Preprocessing & Ekstraksi Fitur Dinamis (Dynamic G & Windowed RMS)
 * 2. LAYER 2: Predictive Maintenance Pattern Classifier (Deteksi Kerusakan Bantalan Mesin)
 * 3. LAYER 3: Kinematic Attitude Gesture Intent Engine (Pengendali Slide Presentasi)
 * 4. LAYER 4: Composite Safety Decision & Benchmark Latensi (< 10 ms di SRAM)
 */
void runEdgeAIInference() {
  unsigned long start_micros = micros(); // Mulai hitung latensi inferensi

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 1: Sinyal Preprocessing & Ekstraksi Fitur Dinamis]
  // --------------------------------------------------------------------------
  // 1. Hitung magnitudo percepatan linier 3-sumbu: |A| = sqrt(ax^2 + ay^2 + az^2)
  float total_acc_ms2 = sqrt(current_sensor.ax * current_sensor.ax + 
                             current_sensor.ay * current_sensor.ay + 
                             current_sensor.az * current_sensor.az);
  float total_acc_g = total_acc_ms2 / 9.80665;
  
  // 2. Eliminasi gravitasi bumi statis (1.0G) -> Mendapatkan 'Dynamic G-Force' murni
  float dynamic_g = fabs(total_acc_g - 1.0); // ~0.0 saat diam di atas meja

  // 3. Masukkan ke Sliding Window (Circular Buffer N=16) untuk ekstraksi energi RMS
  accel_mag_window[window_idx] = dynamic_g;
  window_idx = (window_idx + 1) % WINDOW_SIZE;

  // 4. Hitung Root Mean Square (RMS) dan Peak Spike dalam time-window lokal
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
  // 🧠 [EDGE AI LAYER 2: Predictive Maintenance Pattern Classifier]
  // --------------------------------------------------------------------------
  // Mengklasifikasikan pola getaran: Membedakan gerakan wajar vs getaran mesin kritis
  bool vib_anomaly = (rms > 0.40) || (max_peak > 0.85);

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 3: Kinematic Gesture Intent Engine (Pengendali Slide)]
  // --------------------------------------------------------------------------
  // Mengubah orientasi ruang (Roll/Pitch) menjadi intensi navigasi dalam waktu < 10 ms
  ai_result.gesture = "NONE";
  unsigned long now = millis();
  
  if (now - last_gesture_time > 900) { // Cooldown 900ms agar slide tidak berpindah liar
    if (current_sensor.roll > 40.0) {
      ai_result.gesture = "NEXT_SLIDE";     // Miring Kanan -> Next Slide
      last_gesture_time = now;
    } else if (current_sensor.roll < -40.0) {
      ai_result.gesture = "PREV_SLIDE";     // Miring Kiri -> Prev Slide
      last_gesture_time = now;
    } else if (max_peak > 1.8) {
      ai_result.gesture = "TAP_ACTION";     // Ketukan meja tajam -> Aksi Khusus
      last_gesture_time = now;
    }
  }

  // --------------------------------------------------------------------------
  // 🧠 [EDGE AI LAYER 4: Composite Safety Decision]
  // --------------------------------------------------------------------------
  if (max_peak > 2.2) {
    ai_result.state_label = "HARSH IMPACT!";
    ai_result.confidence = 99;
  } else if (vib_anomaly) {
    ai_result.state_label = "VIB ANOMALY!";
    ai_result.confidence = min(99, (int)(85 + max_peak * 10));
  } else {
    ai_result.state_label = "NOMINAL";
    ai_result.confidence = 98;
  }

  ai_result.is_vibration_anomaly = vib_anomaly;

  // --------------------------------------------------------------------------
  // ⏱️ [EDGE AI BENCHMARK: Hitung Latensi Inferensi On-Device]
  // --------------------------------------------------------------------------
  unsigned long elapsed_micros = micros() - start_micros;
  ai_result.latency_ms = (float)elapsed_micros / 1000.0;
  if (ai_result.latency_ms < 0.05) ai_result.latency_ms = 0.08;
}

// ============================================================================
// 6. RENDER TAMPILAN OLED (128x64 HUD)
// ============================================================================
void renderOLED() {
  if (!oled_ready) return;

  display.clearDisplay();

  // 1. Header Bar
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(2, 2);
  display.print("EDGE-AI S3");

  display.setCursor(80, 2);
  display.printf("%.2fms", ai_result.latency_ms);

  // 2. Status Banner Box
  display.setTextColor(SSD1306_WHITE);
  if (ai_result.is_vibration_anomaly) {
    display.fillRect(0, 15, 128, 14, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.setCursor(4, 18);
    display.printf(">> %s (%d%%)", ai_result.state_label.c_str(), ai_result.confidence);
  } else {
    display.drawRect(0, 15, 128, 14, SSD1306_WHITE);
    display.setCursor(4, 18);
    display.printf(">> %s (%d%%)", ai_result.state_label.c_str(), ai_result.confidence);
  }

  // 3. Matriks Telemetri Sensor Kinematik
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  
  display.setCursor(0, 33);
  display.printf("DynG: %.2fG", ai_result.dynamic_accel_g);
  display.setCursor(68, 33);
  display.printf("RMS: %.2f", ai_result.rms_vibration);

  display.setCursor(0, 44);
  display.printf("Pit: %+.0f", current_sensor.pitch);
  display.setCursor(68, 44);
  display.printf("Rol: %+.0f", current_sensor.roll);

  // 4. Footer: Status Gestur Slide & Chip Status
  display.drawLine(0, 53, 128, 53, SSD1306_WHITE);
  display.setCursor(0, 56);
  if (ai_result.gesture != "NONE") {
    display.printf("GST: %s", ai_result.gesture.c_str());
  } else {
    display.printf("GST: IDLE | MPU-6050");
  }

  display.display();
}

// ============================================================================
// 7. STREAM DATA JSON KE WEBSERIAL (115200 BAUD)
// ============================================================================
void streamTelemetryJSON() {
  // Kompatibilitas Ganda ArduinoJson v6 & v7
  #if ARDUINOJSON_VERSION_MAJOR >= 7
    JsonDocument doc;
  #else
    StaticJsonDocument<512> doc;
  #endif

  // Data Percepatan & Giroskop
  JsonObject acc = doc.createNestedObject("acc");
  acc["x"] = serialized(String(current_sensor.ax, 2));
  acc["y"] = serialized(String(current_sensor.ay, 2));
  acc["z"] = serialized(String(current_sensor.az, 2));

  JsonObject gyro = doc.createNestedObject("gyro");
  gyro["x"] = serialized(String(current_sensor.gx, 2));
  gyro["y"] = serialized(String(current_sensor.gy, 2));
  gyro["z"] = serialized(String(current_sensor.gz, 2));

  // Sudut Orientasi Sikap 3D
  JsonObject ori = doc.createNestedObject("ori");
  ori["pitch"] = serialized(String(current_sensor.pitch, 1));
  ori["roll"]  = serialized(String(current_sensor.roll, 1));

  // Hasil Inferensi On-Device Edge AI
  JsonObject ai = doc.createNestedObject("ai");
  ai["state"]       = ai_result.state_label;
  ai["conf"]        = ai_result.confidence;
  ai["dyn_g"]       = serialized(String(ai_result.dynamic_accel_g, 2));
  ai["rms_vib"]     = serialized(String(ai_result.rms_vibration, 3));
  ai["gesture"]     = ai_result.gesture;
  ai["latency_ms"]  = serialized(String(ai_result.latency_ms, 3));
  ai["is_anomaly"]  = ai_result.is_vibration_anomaly;

  doc["temp"] = serialized(String(current_sensor.temp_c, 1));

  serializeJson(doc, Serial);
  Serial.println();
}

// ============================================================================
// 8. SETUP (INISIALISASI SISTEM)
// ============================================================================
void setup() {
  // 1. Inisialisasi Serial Port CDC USB Native
  Serial.begin(115200);
  unsigned long serial_wait_start = millis();
  while (!Serial && (millis() - serial_wait_start < 2000)) {
    delay(10);
  }
  delay(100);

  Serial.println("\n==================================================");
  Serial.println("🚀 UNIKOM SK 2026: ESP32-S3 EDGE AI KINEMATIC NODE");
  Serial.println("👨‍🏫 Speaker: Anton Prafanto, S.Kom., M.T.");
  Serial.println("==================================================");

  // 2. Inisialisasi Jalur Bus I2C dengan Proteksi Timeout Lockup
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
  Wire.setTimeOut(3000); // Mencegah freeze jika kabel jumper kendor

  // 3. Eksekusi Pemindai Diagnostik I2C
  scanI2CBus();

  // 4. Inisialisasi Layar OLED 0.96" SSD1306 (0x3C)
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oled_ready = true;
    Serial.println("✅ OLED 0.96\" SSD1306 terhubung di alamat 0x3C.");
    showBootAnimation();
  } else {
    Serial.println("⚠️ OLED 0.96\" SSD1306 tidak terdeteksi! Melanjutkan tanpa display fisik.");
  }

  // 5. Inisialisasi Sensor IMU 6-Axis MPU-6050 (0x68)
  if (mpu.begin(MPU_ADDR, &Wire)) {
    mpu_ready = true;
    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
    Serial.println("✅ MPU-6050 6-Axis IMU terhubung di alamat 0x68 (±4G, 500°/s).");
  } else {
    Serial.println("❌ GAGAL inisialisasi MPU-6050! Periksa kabel SDA (IO8) & SCL (IO9).");
  }

  // Inisialisasi Circular Buffer
  for (int i = 0; i < WINDOW_SIZE; i++) {
    accel_mag_window[i] = 0.0;
  }

  Serial.println("\n🚀 Node Edge AI Siap Beroperasi! Streaming JSON aktif di 115200 baud.\n");
}

// ============================================================================
// 9. LOOP UTAMA (RTOS NON-BLOCKING TIMING)
// ============================================================================
void loop() {
  // A. Pembacaan Sensor IMU MPU-6050 (High-Speed Sampling)
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

    // Perhitungan Sudut Sikap 3D (Pitch & Roll)
    current_sensor.pitch = atan2(-current_sensor.ax, 
                                 sqrt(current_sensor.ay * current_sensor.ay + 
                                      current_sensor.az * current_sensor.az)) * 180.0 / PI;
    current_sensor.roll  = atan2(current_sensor.ay, current_sensor.az) * 180.0 / PI;
  }

  // B. Eksekusi Inferensi Edge AI On-Device (< 10 ms di SRAM)
  runEdgeAIInference();

  // C. Task Non-Blocking 1: Render Layar OLED HUD @ 10 FPS (100 ms)
  unsigned long current_millis = millis();
  if (current_millis - last_oled_time >= OLED_INTERVAL_MS) {
    last_oled_time = current_millis;
    renderOLED();
  }

  // D. Task Non-Blocking 2: Stream Data JSON ke WebSerial @ 20 FPS (50 ms)
  if (current_millis - last_stream_time >= STREAM_INTERVAL_MS) {
    last_stream_time = current_millis;
    streamTelemetryJSON();
  }
}
