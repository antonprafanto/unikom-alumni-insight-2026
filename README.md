# 🎓 UNIKOM Alumni Insight Series 2026
### *Program Studi S1 Sistem Komputer – Universitas Komputer Indonesia (UNIKOM)*

> **Topik:** *Computer Engineering Beyond 2026: IoT, Edge AI, and The Future of Connected Systems*  
> **Narasumber:** **Anton Prafanto, S.Kom., M.T.**  
> *(Alumnus S1 Sistem Komputer UNIKOM '11 · S2 Teknik Elektro ITB · Dosen & Peneliti S1 Informatika Universitas Mulawarman)*

---

## 🌐 Akses Langsung (Live GitHub Pages)

* 🏠 **Portal Hub & Landing Page Utama:**  
  👉 [https://antonprafanto.github.io/unikom-alumni-insight-2026/](https://antonprafanto.github.io/unikom-alumni-insight-2026/)

* 📽️ **Slide Presentasi Interaktif (Keynote Deck):**  
  👉 [https://antonprafanto.github.io/unikom-alumni-insight-2026/event_alumni_insight/slides_alumni_insight_2026.html](https://antonprafanto.github.io/unikom-alumni-insight-2026/event_alumni_insight/slides_alumni_insight_2026.html)

* 📘 **Master Handout & Starter Kit Mahasiswa (Siap Cetak PDF):**  
  👉 [https://antonprafanto.github.io/unikom-alumni-insight-2026/event_alumni_insight/handout_mahasiswa_edge_ai.html](https://antonprafanto.github.io/unikom-alumni-insight-2026/event_alumni_insight/handout_mahasiswa_edge_ai.html)

* ⚡ **Live Edge AI Web Telemetry Workstation (ESP32-S3):**  
  👉 [https://antonprafanto.github.io/unikom-alumni-insight-2026/demo_edge_ai/dashboard_web/index.html](https://antonprafanto.github.io/unikom-alumni-insight-2026/demo_edge_ai/dashboard_web/index.html)

---

## 📌 Ringkasan Acara & Deskripsi Materi

Materi ini dirancang khusus untuk membedah arah transformasi keilmuan **Teknik/Sistem Komputer** di era pasca-2026, di mana komputasi cerdas (*Machine Learning / Deep Learning*) bergeser dari ketergantungan penuh pada server *cloud* menuju eksekusi langsung di perangkat fisik (*Edge AI & TinyML*).

### 🎯 5 Babak Perjalanan Materi:
1. **01 Identitas & Akar Lab UNIKOM:** Menghubungkan penguasaan dasar perangkat keras (*register, timer, interrupt, low-level C/C++*) dengan keunggulan langka anak Sistem Komputer di industri AI modern.
2. **02 Problem Lapangan (Cloud AI Bottleneck):** Membedah kelemahan latensi, bandwidth, privasi, dan keandalan offline pada arsitektur terpusat lewat analogi *Refleks Tubuh vs Telepon Dokter*.
3. **03 Dapur Pacu Edge AI & TinyML:** Menjelaskan kuantisasi model INT8 (*analogi membulatkan uang belanjaan*) dan ekstraksi frekuensi sinyal (*analogi montir/stetoskop dokter mesin*).
4. **04 Bukti Terapan Lapangan:** Studi kasus nyata sistem kendali mandiri *Smart Poultry Farming* dan pemantauan kualitas udara bergerak *POPULING (Mobile IoT)*.
5. **05 Blueprint Mahasiswa & Transformasi Skripsi:** 4 level peta jalan belajar mahasiswa serta klinik judul tugas akhir standar usang vs standar *Beyond 2026* berbobot publikasi ilmiah.

---

## 📁 Berkas Repositori Event

```text
unikom-alumni-insight-2026/
├── index.html                                        # Portal Hub & Landing Page utama
├── qr_github.svg                                     # Aset QR code portofolio GitHub
├── event_alumni_insight/
│   ├── slides_alumni_insight_2026.html               # Slide presentasi interaktif (13 slide)
│   ├── handout_mahasiswa_edge_ai.html                # Modul handout mahasiswa (Fitur Cetak PDF)
│   └── qr_github.svg                                 # Aset QR code lokal event
├── demo_edge_ai/                                     # 🚀 Paket Live Demo Mahasiswa
│   ├── firmware_esp32s3/                             # Source code Arduino C++ ESP32-S3
│   ├── dashboard_web/                                # WebSerial Telemetry Workstation (HTML/JS)
│   ├── wokwi/                                        # Diagram simulasi browser gratis Wokwi
│   └── README.md                                     # Panduan perakitan & troubleshooting
├── .nojekyll                                         # Bypass Jekyll build untuk GitHub Pages
└── README.md                                         # Dokumentasi repositori
```

---

## 🚀 Panduan Praktikum & Live Demo Mahasiswa (ESP32-S3)

Bagi adik-adik mahasiswa Sistem Komputer UNIKOM yang ingin **mempraktikkan langsung proyek Edge AI & TinyML**:

👉 **Silakan buka modul panduan lengkap di: [`demo_edge_ai/README.md`](demo_edge_ai/README.md)**

### 🧭 Langkah Cepat yang Dapat Anda Lakukan:
1. **Jika Memiliki Alat Fisik (ESP32-S3 + MPU-6050 + OLED 0.96"):**
   * Ikuti skema kabel I2C (hanya 4 kabel) pada [`demo_edge_ai/README.md`](demo_edge_ai/README.md).
   * Buka dan upload firmware di [`demo_edge_ai/firmware_esp32s3/firmware_esp32s3.ino`](demo_edge_ai/firmware_esp32s3/firmware_esp32s3.ino) via **Arduino IDE**.
   * Buka [`demo_edge_ai/dashboard_web/index.html`](demo_edge_ai/dashboard_web/index.html) di Chrome/Edge, lalu klik **"Connect USB"** untuk melihat grafik getaran & orientasi 3D real-time.
2. **Jika Belum Memiliki Alat Fisik (100% Gratis di Browser):**
   * Gunakan simulator online Wokwi dengan mengimpor konfigurasi [`demo_edge_ai/wokwi/diagram.json`](demo_edge_ai/wokwi/diagram.json) dan [`demo_edge_ai/wokwi/libraries.txt`](demo_edge_ai/wokwi/libraries.txt).
3. **Mengembangkan Ide Tugas Akhir / Skripsi:**
   * Pelajari bab arsitektur TinyML, kuantisasi INT8, dan 5 bank ide judul skripsi terapan pada dokumentasi demo.

---

## 🛠️ Pilihan Hardware Starter Kit Mahasiswa (< Rp 200.000)

| Komponen | Spesifikasi Inti | Estimasi Harga |
| :--- | :--- | :--- |
| **ESP32-S3 DevKit** | Dual-core 240 MHz, 512 KB SRAM, Wi-Fi/BLE, Vector AI Instruction (PIE) | ± Rp 80.000–120.000 |
| **Raspberry Pi Pico 2** | Dual Cortex-M33 / Hazard3 RISC-V @ 150 MHz, 520 KB SRAM | ± Rp 80.000–95.000 |
| **STM32F401/F411 (BlackPill)**| ARM Cortex-M4 dengan FPU, ST Edge AI / X-CUBE-AI toolchain | ± Rp 55.000–75.000 |
| **Sensor IMU (MPU-6050)** | Accelerometer + Gyroscope 6-Axis (Deteksi Getaran Mesin) | ± Rp 25.000 |
| **Mikrofon I2S (INMP441)** | Omnidirectional digital mic (Keyword Spotting & Audio Anomali) | ± Rp 18.000 |

---

## 👨‍🏫 Kontak Narasumber

* **Nama:** Anton Prafanto, S.Kom., M.T.
* **Institusi:** Program Studi S1 Informatika, Fakultas Teknik, Universitas Mulawarman (UNMUL), Samarinda
* **Email Resmi:** [antonprafanto@unmul.ac.id](mailto:antonprafanto@unmul.ac.id)
* **GitHub:** [github.com/antonprafanto](https://github.com/antonprafanto)
* **SINTA ID:** 6659198 | **Scopus Author ID:** 57195931078

---
*Didedikasikan untuk Almamater tercinta: Program Studi Sistem Komputer, Fakultas Teknik dan Ilmu Komputer, Universitas Komputer Indonesia (UNIKOM).*
