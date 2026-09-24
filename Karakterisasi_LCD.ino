#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set alamat I2C (umumnya 0x27 atau 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // Inisialisasi bus I2C khusus ESP32 (SDA = GPIO 21, SCL = GPIO 22)
  Wire.begin(21, 22);

  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Baris pertama
  lcd.setCursor(0, 0);
  lcd.print("Welcome");

  // Baris kedua
  lcd.setCursor(0, 1);
  lcd.print("Endministrator");
}

void loop() {
  // Statis
}