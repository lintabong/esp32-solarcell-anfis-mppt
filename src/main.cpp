#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <SPI.h>
#include <DallasTemperature.h>

// Pin konfigurasi
#define LDR_PIN 4          // LDR di GPIO4 (ADC2)
#define DS18B20_PIN 34      // DS18B20 di GPIO34 (ADC1)

// Inisialisasi LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Alamat I2C LCD biasanya 0x27 atau 0x3F

// Inisialisasi untuk DS18B20
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  // Inisialisasi serial untuk debugging
  Serial.begin(115200);

  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();
  lcd.print("Initializing...");
  
  // Inisialisasi DS18B20
  sensors.begin();
  delay(2000);
}

void loop() {
  // Membaca nilai LDR
  int ldrValue = analogRead(LDR_PIN);
  
  // Membaca suhu dari DS18B20
  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0); // Membaca suhu dari sensor pertama

  // Debug output di Serial Monitor
  Serial.print("LDR Value: ");
  Serial.print(ldrValue);
  Serial.print(" | Temperature: ");
  Serial.println(temperature);

  // Menampilkan ke LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("LDR: ");
  lcd.print(ldrValue);

  lcd.setCursor(0, 1);
  lcd.print("Temp: ");
  if (temperature == DEVICE_DISCONNECTED_C) {
    lcd.print("Error");
  } else {
    lcd.print(temperature);
    lcd.print(" C");
  }

  // Delay untuk pembacaan berikutnya
  delay(1000);
}
