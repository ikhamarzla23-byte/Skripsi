#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

const byte MLX90640_ADDRESS = 0x33; 
uint16_t frame[834];       
float mlx90640To[768];     
paramsMLX90640 mlx90640;           

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Sistem Inisialisasi MLX90640 Berhasil!");
  Serial.println("Format: Raw IR | Suhu Objek (°C)");
  Serial.println("----------------------------------------");

  Wire.begin(21, 22); // SDA, SCL ESP32
  MLX90640_I2CInit();
  MLX90640_I2CFreqSet(400000); 

  MLX90640_SetRefreshRate(MLX90640_ADDRESS, 0x02); 

  uint16_t eeMLX90640[832];
  MLX90640_DumpEE(MLX90640_ADDRESS, eeMLX90640);
  MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
}

void loop() {
  int status = MLX90640_GetFrameData(MLX90640_ADDRESS, frame);
  if (status < 0) {
    delay(100);
    return; 
  }

  float emissivity = 0.95; 
  float tr = 23.15;        
  MLX90640_CalculateTo(frame, &mlx90640, emissivity, tr, mlx90640To);

  // Mengambil data mentah Raw IR dan Suhu Objek dari piksel indeks 0
  uint16_t raw_ir = frame[0];       
  float suhu_objek = mlx90640To[0];   

  // Tampilkan ke Serial Monitor sesuai format yang diminta
  Serial.print("Raw IR: ");
  Serial.print(raw_ir);
  Serial.print("  |  Suhu Objek: ");
  Serial.print(suhu_objek);            
  Serial.println(" °C");

  delay(1000);
}