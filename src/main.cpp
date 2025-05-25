#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

// PINURI
#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN A0
#define BUTTON_MODE_PIN 2
#define BUTTON_UNLOCK_PIN 3

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

bool isWriteMode = false;
String acceptedUID = "A1B2C3D4";  // UID-ul autorizat – poți înlocui cu al tău

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // yala inițial dezactivată

  pinMode(BUTTON_MODE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_UNLOCK_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Smart Seif");
  lcd.setCursor(0, 1);
  lcd.print("Pornit...");
  delay(2000);
  lcd.clear();
}

void loop() {
  // Buton pentru dezactivare yală
  if (digitalRead(BUTTON_UNLOCK_PIN) == LOW) {
    digitalWrite(RELAY_PIN, HIGH);
    lcd.clear();
    lcd.print("Yala dezactivata");
    delay(1000);
  }

  // Buton pentru schimbare mod citire/scriere
  if (digitalRead(BUTTON_MODE_PIN) == LOW) {
    isWriteMode = !isWriteMode;
    lcd.clear();
    lcd.print(isWriteMode ? "Mod: SCRIERE" : "Mod: CITIRE");
    delay(1000);
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UID:");
  lcd.setCursor(0, 1);
  lcd.print(uid);

  if (isWriteMode) {
    acceptedUID = uid;
    lcd.clear();
    lcd.print("UID salvat!");
    delay(1500);
  } else {
    if (uid == acceptedUID) {
      lcd.clear();
      lcd.print("Acces permis!");
      digitalWrite(RELAY_PIN, LOW);  // deschide yala
      delay(3000);
      digitalWrite(RELAY_PIN, LOW);   // închide yala

    } else {
      lcd.clear();
      lcd.print("Acces respins!");
      delay(2000);
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
