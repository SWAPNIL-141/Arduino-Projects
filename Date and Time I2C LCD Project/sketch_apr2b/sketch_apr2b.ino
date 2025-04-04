#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();  // Arka ışığı aç
  lcd.setCursor(0, 0);
  lcd.print("Saat Bekleniyor...");
  lcd.setCursor(0, 1);
  lcd.print("Tarih Bekleniyor...");
}

void loop() {
  if (Serial.available()) {
    String veri = Serial.readStringUntil('\n');  // PC'den veri al
    int ayirici = veri.indexOf('|');  // Saat ve tarihi ayır

    if (ayirici != -1) {
      String saat = veri.substring(0, ayirici);
      String tarih = veri.substring(ayirici + 1);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Saat: " + saat);
      lcd.setCursor(0, 1);
      lcd.print("Tarih: " + tarih);
    }
  }
}
