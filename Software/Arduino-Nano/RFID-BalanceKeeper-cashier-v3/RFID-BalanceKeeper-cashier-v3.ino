#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AESLib.h> 

/**
 * @file KermesSystem2026.ino
 * @brief RFID-basiertes Bezahlsystem mit AES-Verschlüsselung für die IGH Kermes 2026.
 */

// --- Hardware-Konfiguration ---
#define RST_PIN         A0
#define SS_PIN          10
#define BUZZER_PIN      A3

// --- System-Instanzen ---
LiquidCrystal_I2C lcd(0x27, 20, 4);
MFRC522 mfrc522(SS_PIN, RST_PIN);
AESLib aes;

/** @brief 128-Bit AES Schlüssel (16 Zeichen). Muss auf allen Terminals identisch sein. */
byte aes_key[] = {"IGMGHERNE22026ke"}; 

// --- Keypad-Konfiguration ---
const byte rows[4] = { 2, 3, 4, 5 };
const byte cols[4] = { 6, 7, 8, 9 };
char keys[4][4] = {
  { '1', '2', '3', 'A' }, { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' }, { '*', '0', '#', 'D' },
};
Keypad myKeypad = Keypad(makeKeymap(keys), rows, cols, 4, 4);

String inputString = "";
bool isSessionActive = false; // Tracker, um den Indikator-Status global zu halten

/**
 * @brief Initialisierung der Hardware-Komponenten und Start der seriellen Kommunikation.
 */
void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  pinMode(BUZZER_PIN, OUTPUT);
  clearDisplay();
  
  Serial.println(F("========================================"));
  Serial.println(F("   KERMES 2026 - SYSTEM START"));
  Serial.println(F("========================================"));
}

/**
 * @brief Hauptschleife: Überwacht Tastatureingaben und RFID-Kartenpräsenz.
 */
void loop() {
  char mainKey = myKeypad.getKey();
  
  // Überprüfung der Aktionstasten A, B und D
  if (mainKey && (mainKey == 'A' || mainKey == 'B' || mainKey == 'D')) {
    // Sicherstellen, dass eine Karte zum Bearbeiten auf dem Leser liegt
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      showNoCardError();
    } else {
      startSession(mainKey);
    }
  }

  // Automatischer Start der Guthabenanzeige beim Auflegen einer Karte
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    startSession('X'); 
  }
}

/**
 * @brief Verwaltet eine aktive Sitzung, um mehrfache Operationen ohne Neuauflegen zu ermöglichen.
 * @param initialAction Der Tastaturbefehl, der die Sitzung initiiert hat.
 */
void startSession(char initialAction) {
  isSessionActive = true; 
  Serial.println(F("\n[SESSION] Karte erkannt."));
  
  // --- SESSION ICON INITIAL ANZEIGEN ---
  lcd.setCursor(15, 1); 
  lcd.write(0xFF); 

  // Führt die erste gewählte Aktion sofort aus
  if (initialAction == 'A' || initialAction == 'X') readCreditFromCard_Simple();
  else if (initialAction == 'B') handleCharging();
  else if (initialAction == 'D') handleSetCredit();

  unsigned long lastActivity = millis();
  
  // Hält den Benutzer im Modus, bis der Timeout greift oder '*' gedrückt wird
  while (isSessionActive) {
    char sessionKey = myKeypad.getKey();
    if (sessionKey) {
      lastActivity = millis(); // Timer bei jeder Interaktion zurücksetzen
      if (sessionKey == 'A') readCreditFromCard_Simple();
      else if (sessionKey == 'B') handleCharging();
      else if (sessionKey == 'D') handleSetCredit();
      else if (sessionKey == '*') isSessionActive = false;
    }
    // Automatischer Abbruch nach 30 Sekunden Inaktivität (Schutz gegen Vergessen der Karte)
    if (millis() - lastActivity > 30000) isSessionActive = false;
    delay(50);
  }

  // Karte in den Schlafmodus versetzen und Krypto-Einheit abschalten
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
  isSessionActive = false; // Status zurücksetzen
  clearDisplay(); // Entfernt Icon und zeigt Standby-Text
  Serial.println(F("[SESSION] Beendet.\n"));
  delay(1000); 
}

/**
 * @brief Liest verschlüsselte Daten von Sektor 0, Block 1 und entschlüsselt diese.
 * @return Das Guthaben als Integer oder -1 bei Kommunikationsfehlern.
 */
int getCreditFromChip() {
  MFRC522::MIFARE_Key rfidKey;
  for (byte i = 0; i < 6; i++) rfidKey.keyByte[i] = 0xFF; // Werkseitiger Standard-Key
  
  // Authentifizierung am Datenblock 1 erforderlich für MIFARE Classic
  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &rfidKey, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
    return -1;
  }

  byte buffer[18];
  byte len = 18;
  // Auslesen des 16-Byte-Blocks vom Chip
  if (mfrc522.MIFARE_Read(1, buffer, &len) != MFRC522::STATUS_OK) return -1;

  Serial.println(F("------------------------------------------------------------"));
  Serial.print(F("[LOG] Rohdaten (AES-Hex): "));
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] < 0x10) Serial.print("0");
    Serial.print(buffer[i], HEX); Serial.print(" ");
  }
  Serial.println();

  // AES Entschlüsselung: IV muss vor jeder Operation genullt werden
  byte temp_iv[16]; memset(temp_iv, 0, 16); 
  byte decryptedData[16];
  aes.decrypt(buffer, 16, decryptedData, aes_key, 128, temp_iv);

  // Rekonstruktion des Integers: High-Byte (Position 0) und Low-Byte (Position 1)
  int credit = (decryptedData[0] << 8) + decryptedData[1];
  Serial.print(F("[LOG] Entschlüsseltes Guthaben: ")); Serial.println(credit);
  
  // Bereichsprüfung zur Vermeidung von fehlerhaften Werten
  return (credit < 0 || credit > 20000) ? 0 : credit;
}

/**
 * @brief Verschlüsselt einen Integer-Wert und schreibt ihn auf Sektor 0, Block 1.
 * @param finalValue Der neue Gesamtwert, der auf die Karte geschrieben werden soll.
 */
void writeNewValueToCard(int finalValue) {
  if (finalValue < 0) finalValue = 0;
  
  // Vorbereitung des 16-Byte-Datenblocks
  byte dataToEncrypt[16] = {0}; 
  // Zerlegung des Integers in zwei Bytes für den Block-Speicher
  dataToEncrypt[0] = (finalValue >> 8) & 0xFF; // Oberes Byte extrahieren
  dataToEncrypt[1] = finalValue & 0xFF;        // Unteres Byte extrahieren

  byte temp_iv[16]; memset(temp_iv, 0, 16); 
  byte encryptedData[16];
  // Verschlüsselung der Daten mit 128-Bit AES
  aes.encrypt(dataToEncrypt, 16, encryptedData, aes_key, 128, temp_iv);

  Serial.println(F("------------------------------------------------------------"));
  Serial.print(F("[LOG] Zielwert: ")); Serial.println(finalValue);
  Serial.print(F("[LOG] AES-Block: "));
  for (byte i = 0; i < 16; i++) {
    if (encryptedData[i] < 0x10) Serial.print("0");
    Serial.print(encryptedData[i], HEX); Serial.print(" ");
  }
  Serial.println();

  // Physisches Schreiben des verschlüsselten Buffers auf die Karte
  if (mfrc522.MIFARE_Write(1, encryptedData, 16) == MFRC522::STATUS_OK) {
    successFeedback(finalValue);
  } else {
    printError();
  }
}

/**
 * @brief Admin-Menü zum direkten Überschreiben des Kartenguthabens.
 */
void handleSetCredit() {
  lcd.clear(); lcd.print("Setze Wert: ");
  if (isSessionActive) { lcd.setCursor(15, 1); lcd.write(0xFF); }
  
  inputString = "";
  while (1) {
    char key = myKeypad.getKey();
    if (key) {
      if (key == '#') { 
        writeNewValueToCard(inputString.toInt()); 
        break; 
      }
      else if (key == '*') { clearDisplay(); break; }
      else { 
        inputString += key; 
        lcd.setCursor(12, 0); lcd.print(inputString); 
      }
    }
  }
}

/**
 * @brief Auflade-Menü: Liest den aktuellen Stand und addiert die Benutzereingabe.
 */
void handleCharging() {
  lcd.clear(); lcd.print("Aufladen: ");
  if (isSessionActive) { lcd.setCursor(15, 1); lcd.write(0xFF); }
  
  inputString = "";
  while (1) {
    char key = myKeypad.getKey();
    if (key) {
      if (key == '#') { 
        int current = getCreditFromChip(); 
        if (current != -1) {
          writeNewValueToCard(current + inputString.toInt()); 
        }
        break; 
      }
      else if (key == '*') { clearDisplay(); break; }
      else { 
        inputString += key; 
        lcd.setCursor(10, 0); lcd.print(inputString); 
      }
    }
  }
}

/**
 * @brief Liest den Kontostand aus und zeigt ihn für 2 Sekunden auf dem Display an.
 */
void readCreditFromCard_Simple() {
  int credit = getCreditFromChip();
  lcd.clear();
  if (isSessionActive) { lcd.setCursor(15, 1); lcd.write(0xFF); }
  
  lcd.setCursor(0, 0);
  if (credit == -1) lcd.print("Chip-Fehler!");
  else { 
    lcd.print("Guthaben: "); 
    lcd.print(String(credit) + ",-"); 
  }
  delay(2000); clearDisplay();
}

/**
 * @brief Bestätigung eines erfolgreichen Schreibvorgangs durch Ton und LCD-Update.
 * @param amount Der neue Guthabenstand zur Anzeige.
 */
void successFeedback(int amount) {
  tone(BUZZER_PIN, 2500, 200); 
  lcd.clear(); 
  if (isSessionActive) { lcd.setCursor(15, 1); lcd.write(0xFF); }
  
  lcd.setCursor(0, 0);
  lcd.print("Neu: " + String(amount) + ",-");
  delay(3000); clearDisplay();
}

/**
 * @brief Setzt das LCD auf die Standard-Willkommensnachricht zurück.
 * Berücksichtigt den Session-Status für den Indikator.
 */
void clearDisplay() { 
  lcd.clear(); 
  lcd.print("IGH Kermes 2026"); 
  if (isSessionActive) {
    lcd.setCursor(15, 1);
    lcd.write(0xFF);
  }
}

/**
 * @brief Fehler-Feedback falls keine Karte erkannt wurde.
 */
void showNoCardError() {
  lcd.clear(); lcd.print("  KARTE FEHLT!");
  tone(BUZZER_PIN, 1000, 300); 
  delay(2000); clearDisplay();
}

/**
 * @brief Allgemeines Fehler-Feedback bei gescheiterten Chip-Operationen.
 */
void printError() {
  lcd.clear(); lcd.print("Fehler!");
  tone(BUZZER_PIN, 500, 100); delay(150); tone(BUZZER_PIN, 500, 100);
  delay(2000); clearDisplay();
}