#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9
#define RELAY_PIN A0       // PC0
#define BUTTON_MODE_PIN 2  // PD2

#define MAX_UIDS 10
#define UID_LENGTH 4

MFRC522 rfid(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Vector pentru stocarea UID-urilor
byte storedUIDs[MAX_UIDS][UID_LENGTH];
int savedUIDCount = 0;

byte mode = 0; // 0 = citire, 1 = scriere, 2 = stergere
byte lastMode = 0;

// Pentru a preveni update-uri inutile ale LCD-ului
void afiseazaModPeLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Secure Box");
  lcd.setCursor(0, 1);
  if (mode == 0) lcd.print("Mod: CITIRE");
  else if (mode == 1) lcd.print("Mod: SCRIERE");
  else lcd.print("Mod: STERGERE");
}

// Inițializează vectorul cu 0xFF (memorie goală)
void initUIDs() {
  for (int i = 0; i < MAX_UIDS; i++) {
    for (int j = 0; j < UID_LENGTH; j++) {
      storedUIDs[i][j] = 0xFF;
    }
  }
  savedUIDCount = 0;
}

int findEmptySlot() {
  for (int i = 0; i < MAX_UIDS; i++) {
    if (storedUIDs[i][0] == 0xFF) return i;
  }
  return MAX_UIDS;
}

bool compareUIDs(byte *uid1, byte *uid2) {
  for (byte i = 0; i < UID_LENGTH; i++) {
    if (uid1[i] != uid2[i]) return false;
  }
  return true;
}

bool isUIDAuthorized(byte *uid) {
  for (int i = 0; i < MAX_UIDS; i++) {
    if (storedUIDs[i][0] != 0xFF && compareUIDs(uid, storedUIDs[i])) return true;
  }
  return false;
}

void saveUID(byte *uid) {
  int index = findEmptySlot();
  if (index < MAX_UIDS) {
    for (byte i = 0; i < UID_LENGTH; i++) {
      storedUIDs[index][i] = uid[i];
    }
    savedUIDCount++;
  }
}

void deleteUID(byte *uid) {
  for (int i = 0; i < MAX_UIDS; i++) {
    if (storedUIDs[i][0] != 0xFF && compareUIDs(uid, storedUIDs[i])) {
      for (int j = 0; j < UID_LENGTH; j++) {
        storedUIDs[i][j] = 0xFF;
      }
      savedUIDCount--;
      break;
    }
  }
}

void setup() {
  delay(100);
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  DDRC |= (1 << DDC0);         // A0 OUTPUT
  PORTC |= (1 << PORTC0);      // HIGH - yala dezactivata

  DDRD &= ~(1 << DDD2);        // PD2 INPUT
  PORTD |= (1 << PORTD2);      // Pull-up ON

  lcd.init();
  lcd.backlight();
  afiseazaModPeLCD();
  delay(2000);

  initUIDs();
}

void loop() {
  // Schimbare mod doar la apasare si eliberare buton
  if (!(PIND & (1 << PIND2))) {
    mode = (mode + 1) % 3;
    afiseazaModPeLCD();
  }

  // Afiseaza mod daca s-a schimbat (prevenim update-uri dese pe LCD)
  if (mode != lastMode) {
    afiseazaModPeLCD();
    lastMode = mode;
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  // Verifica dimensiunea UID
  if (rfid.uid.size != UID_LENGTH) {
    lcd.clear();
    lcd.print("UID invalid!");
    delay(1000);
    afiseazaModPeLCD();
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }

  byte *uid = rfid.uid.uidByte;
  char uidStr[2 * UID_LENGTH + 1];
  for (byte i = 0; i < UID_LENGTH; i++) {
    sprintf(&uidStr[i * 2], "%02X", uid[i]);
  }
  uidStr[2 * UID_LENGTH] = '\0';

  // Afisare UID
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UID:");
  lcd.setCursor(0, 1);
  lcd.print(uidStr);
  delay(700);

  if (mode == 1) { // SCRIERE
    if (!isUIDAuthorized(uid)) {
      if (savedUIDCount < MAX_UIDS) {
        saveUID(uid);
        lcd.clear();
        lcd.print("UID salvat!");
      } else {
        lcd.clear();
        lcd.print("Memorie plina!");
      }
    } else {
      lcd.clear();
      lcd.print("UID deja salvat!");
    }
    delay(900);
  } else if (mode == 2) { // STERGERE
    if (isUIDAuthorized(uid)) {
      deleteUID(uid);
      lcd.clear();
      lcd.print("UID sters!");
    } else {
      lcd.clear();
      lcd.print("UID inexistent!");
    }
    delay(900);
  } else { // CITIRE
    if (isUIDAuthorized(uid)) {
      lcd.clear();
      lcd.print("Acces permis!");
      PORTC &= ~(1 << PORTC0);;  // deschide yala
      delay(3000);
      PORTC |= (1 << PORTC0);;
      delay(1000);
    } else {
      lcd.clear();
      lcd.print("Acces respins!");
      delay(900);
    }
  }
  afiseazaModPeLCD();
  delay(2000);
  
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}