#include <Wire.h>
#include <Adafruit_MLX90640.h>

Adafruit_MLX90640 mlx;

// Array tempat menyimpan 768 titik piksel suhu (32x24)
float mlx90640Frame[768];

unsigned long startTime = 0;
int currentIntervalIndex = 0;

// Daftar interval waktu uji: 0, 5, 10, 15, ..., 60 menit (dalam milidetik)
const unsigned long intervals[] = {
  0,        // 0 menit
  300000,   // 5 menit  (5 * 60 * 1000 ms)
  600000,   // 10 menit
  900000,   // 15 menit
  1200000,  // 20 menit
  1500000,  // 25 menit
  1800000,  // 30 menit
  2100000,  // 35 menit
  2400000,  // 40 menit
  2700000,  // 45 menit
  3000000,  // 50 menit
  3300000,  // 55 menit
  3600000   // 60 menit
};
const int totalIntervals = sizeof(intervals) / sizeof(intervals[0]);

void setup() {
  // Inisialisasi Serial Monitor
  Serial.begin(115200);
  while (!Serial) delay(10); // Menunggu Serial aktif

  // Inisialisasi I2C (SDA = GPIO 21, SCL = GPIO 22)
  Wire.begin(21, 22);

  // Inisialisasi Sensor MLX90640
  if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("[ERROR] MLX90640 tidak terdeteksi! Periksa pengkabelan VCC, GND, SDA, dan SCL.");
    while (1) delay(100);
  }

  // Pengaturan frekuensi dan resolusi pembacaan
  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_2_HZ);

  // Header Tampilan Serial Monitor
  Serial.println("\n==================================================");
  Serial.println("   KARAKTERISASI SUHU SENSOR MLX90640 (ESP32)");
  Serial.println("==================================================");
  Serial.println("Menit\tSuhu Tubuh (°C)");
  Serial.println("--------------------------------------------------");

  startTime = millis();
}

void loop() {
  unsigned long elapsedTime = millis() - startTime;

  // Memeriksa apakah sudah mencapai interval pengujian berikutnya
  if (currentIntervalIndex < totalIntervals) {
    if (elapsedTime >= intervals[currentIntervalIndex]) {
      
      // Ambil 1 frame data dari MLX90640
      if (mlx.getFrame(mlx90640Frame) == 0) {
        float maxTemp = -999.0;

        // Cari suhu tertinggi (objek terpanas/suhu tubuh) dari 768 piksel
        for (int i = 0; i < 768; i++) {
          if (mlx90640Frame[i] > maxTemp) {
            maxTemp = mlx90640Frame[i];
          }
        }

        int minuteLabel = currentIntervalIndex * 5;

        // Tampilkan hasil pengujian ke Serial Monitor (Menit murni tanpa "M-")
        Serial.print(minuteLabel);
        Serial.print("\t");
        Serial.println(maxTemp, 2); // Tampilkan 2 angka di belakang koma

        currentIntervalIndex++;
      }
    }
  }

  delay(500); // Polling ringan
}