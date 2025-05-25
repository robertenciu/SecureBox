#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
MFRC522 rfid(10, 9);

MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();

  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  lcd.setCursor(0, 0);
  lcd.print("Apropie cardul");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  byte block = 1;
  byte buffer[18];
  byte size = sizeof(buffer);

  if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
    Serial.println("Autentificare esuata");
    return;
  }

  if (rfid.MIFARE_Read(block, buffer, &size) != MFRC522::STATUS_OK) {
    Serial.println("Citire esuata");
    return;
  }

  char text[17];
  for (int i = 0; i < 16; i++) {
    text[i] = (char)buffer[i];
  }
  text[16] = '\0';

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Card citit:");
  lcd.setCursor(0, 1);
  lcd.print(text);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();

  delay(2000); 
}
