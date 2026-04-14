#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AESLib.h> 

/**
 * @file KermesVerkauf2026.ino
 * @brief Terminal für Verkaufsstände mit AES-Verschlüsselung, Guthabenprüfung und Abbuchungslogik.
 */

// --- Hardware-Konfiguration ---
#define RST_PIN A0      // Reset-Pin für den RFID-Reader
#define SS_PIN 10       // Slave-Select-Pin für den RFID-Reader
#define BUZZER_PIN A3   // Pin für den Signalgeber (Piepser)

// --- System-Instanzen ---
LiquidCrystal_I2C lcd(0x27, 20, 4); // I2C LCD Instanz (Adresse 0x27, 20 Spalten, 4 Zeilen)
MFRC522 mfrc522(SS_PIN, RST_PIN);   // MFRC522 RFID Instanz
AESLib aes;                         // AES Verschlüsselungs-Bibliothek

/** @brief 128-Bit AES Schlüssel. MUSS exakt gleich wie bei der Aufladestation sein! */
byte aes_key[] = {"IGMGHERNE22026ke"}; 

// --- Keypad-Konfiguration (4x4 Matrix) ---
const byte rows[4] = { 2, 3, 4, 5 };
const byte cols[4] = { 6, 7, 8, 9 };
char keys[4][4] = {
  { '1', '2', '3', 'A' }, { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' }, { '*', '0', '#', 'D' },
};
Keypad myKeypad = Keypad(makeKeymap(keys), rows, cols, 4, 4);

// --- Globale Variablen ---
String inputString = "";      // Speichert den festgelegten Verkaufspreis (z.B. für Döner)
bool isSessionActive = false; // Flag für den Aktivitäts-Indikator im Display (Ecke rechts)
const String correctPassword = "1234"; // Geräte-PIN beim Start

/**
 * @brief Initialisierung der Hardware und Abfrage der Grundeinstellungen (PIN & Preis).
 */
void setup() {
  Serial.begin(9600);
  SPI.begin();             // Startet den SPI-Bus für RFID
  mfrc522.PCD_Init();      // Initialisiert den RFID-Reader
  lcd.init();              // Initialisiert das Display
  lcd.backlight();         // Schaltet Display-Beleuchtung ein
  pinMode(BUZZER_PIN, OUTPUT);
  
  lcd.clear();
  lcd.print("System Start...");
  
  Serial.println(F("========================================"));
  Serial.println(F("   KERMES 2026 - VERKAUFSSTAND START"));
  Serial.println(F("========================================"));
  
  delay(2000);
  lcd.clear();

  // 1. Sicherheit: PIN-Eingabe beim Starten des Geräts
  if (checkPassword()) {
    Serial.println(F("[AUTH] Passwort korrekt."));
    lcd.clear();
    
    // 2. Preis-Konfiguration: Welcher Betrag soll bei jedem Scan abgebucht werden?
    lcd.print("Preis eingeben:");
    inputString = getUserInput();
    
    Serial.print(F("[SETUP] Fixer Preis gesetzt auf: ")); 
    Serial.print(inputString); Serial.println(F(" Euro"));
    
    lcd.clear();
    lcd.print("Preis bereit.");
    delay(1500);
    clearDisplay(); // Bereitet den Verkaufs-Bildschirm vor
  } else {
    Serial.println(F("[AUTH] Falsches Passwort!"));
    lcd.clear();
    lcd.print("Falsches Passwort!");
    delay(2000);
    setup(); // Startet Setup neu bei falscher PIN
  }
}

/**
 * @brief Hauptschleife: Zeigt den Preis an und wartet auf eine RFID-Karte zum Abbuchen.
 */
void loop() {
  int amount = inputString.toInt(); // Konvertiert den eingegebenen Preis in eine Zahl
  
  // --- DAUERHAFTE STANDBY-ANZEIGE ---
  lcd.setCursor(0, 0);
  lcd.print("Preis: " + String(amount) + " Euro     "); // Leerzeichen am Ende überschreiben Reste
  lcd.setCursor(0, 1);
  lcd.print("Karte scannen...   ");

  // --- RFID SCAN LOGIK ---
  // Prüft, ob eine neue Karte an den Sensor gehalten wird
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    isSessionActive = true;
    lcd.setCursor(15, 1); lcd.write(0xFF); // Zeigt "Aktivität" im Display an

    Serial.println(F("------------------------------------------------------------"));
    Serial.println(F("[ACTION] Verkauf gestartet."));

    // Betrag abziehen (negativer Wert wird übergeben)
    updateCreditOnCard(-amount); 
    
    // Kommunikation mit Karte sauber beenden
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    isSessionActive = false;
    delay(4000); // 4 Sekunden Pause, damit man das Restguthaben lesen kann
    clearDisplay();
  }
}

/**
 * @brief Kernlogik: Liest Guthaben, entschlüsselt AES, prüft Deckung und schreibt neuen Wert.
 * @param creditChange Betrag der addiert (Aufladung) oder subtrahiert (Abbuchung) wird.
 */
void updateCreditOnCard(int creditChange) {
  MFRC522::MIFARE_Key rfidKey;
  for (byte i = 0; i < 6; i++) rfidKey.keyByte[i] = 0xFF; // Standard-Werksschlüssel

  // 1. Authentifizierung an Block 1
  if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &rfidKey, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
    printError("Auth Fehler");
    return;
  }

  // 2. Verschlüsselte Daten vom Chip lesen
  byte buffer[18];
  byte len = 18;
  if (mfrc522.MIFARE_Read(1, buffer, &len) != MFRC522::STATUS_OK) {
    printError("Lese-Fehler");
    return;
  }

  // 3. AES ENTSCHLÜSSELUNG
  byte temp_iv[16]; memset(temp_iv, 0, 16); // IV muss immer auf 0 gesetzt werden
  byte decryptedData[16];
  aes.decrypt(buffer, 16, decryptedData, aes_key, 128, temp_iv);

  // Guthaben aus dem Speicher rekonstruieren (High-Byte/Low-Byte Verfahren)
  int currentCredit = (decryptedData[0] << 8) + decryptedData[1];
  
  // 4. SICHERHEITS-CHECK: Hat der Kunde genug Geld?
  if (creditChange < 0 && currentCredit < abs(creditChange)) {
    Serial.print(F("[DENIED] Zu wenig Guthaben: ")); Serial.println(currentCredit);
    lcd.clear();
    lcd.print("ZU WENIG GELD!");
    lcd.setCursor(0, 1);
    lcd.print("Nur: " + String(currentCredit) + ",- Euro");
    tone(BUZZER_PIN, 200, 1000); // Tiefer Fehlerton
    return; // Abbruch der Funktion: Nichts wird auf die Karte geschrieben!
  }

  // 5. NEUEN WERT BERECHNEN
  int newCredit = currentCredit + creditChange; 

  // 6. AES VERSCHLÜSSELUNG DES NEUEN WERTES
  byte dataToEncrypt[16] = {0};
  dataToEncrypt[0] = (newCredit >> 8) & 0xFF; // Höherwertiges Byte speichern
  dataToEncrypt[1] = newCredit & 0xFF;        // Niederwertiges Byte speichern

  memset(temp_iv, 0, 16); // IV für Verschlüsselung vorbereiten
  byte encryptedData[16];
  aes.encrypt(dataToEncrypt, 16, encryptedData, aes_key, 128, temp_iv);

  // 7. NEUEN STAND AUF CHIP SPEICHERN
  if (mfrc522.MIFARE_Write(1, encryptedData, 16) != MFRC522::STATUS_OK) {
    printError("Schreib-Fehler");
    return;
  } else {
    // 8. ERFOLGSMELDUNG
    lcd.clear();
    lcd.print("Afiyet olsun :D"); // Guten Appetit!
    
    lcd.setCursor(0, 1);
    // Anzeige des Restguthabens mit Leerzeichen zum Überschreiben alter "2026"-Reste
    lcd.print("Rest: " + String(newCredit) + ",-      "); 
    
    if (isSessionActive) { lcd.setCursor(15, 1); lcd.write(0xFF); }
    Serial.print(F("[SUCCESS] Zahlung erfolgreich. Neuer Stand: ")); Serial.println(newCredit);
  }
}

// --- Hilfsfunktionen ---

/**
 * @brief PIN-Abfrage Logik.
 */
bool checkPassword() {
  lcd.clear();
  lcd.print("PIN eingeben:");
  String password = getUserInput();
  return password == correctPassword;
}

/**
 * @brief Verarbeitet Tastatureingaben vom Keypad bis zur Bestätigung mit '#'.
 */
String getUserInput() {
  String input = "";
  char key;
  while (true) {
    key = myKeypad.getKey();
    if (key) {
      if (key == '#') { // Bestätigungstaste
        if (input.length() > 0) break;
      } else if (key == '*') { // Korrekturtaste (löschen)
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");
      } else if (key >= '0' && key <= '9') { // Nur Zahlen erlauben
        input += key;
        lcd.setCursor(0, 1);
        lcd.print(input);
      }
    }
  }
  return input;
}

/**
 * @brief Säubert das Display und stellt den Session-Indikator wieder her.
 */
void clearDisplay() {
  lcd.clear();
  if (isSessionActive) {
    lcd.setCursor(15, 1);
    lcd.write(0xFF);
  }
}

/**
 * @brief Zeigt eine Fehlermeldung auf dem Display und im Seriellen Monitor an.
 */
void printError(String logMsg) {
  Serial.print(F("[ERROR] ")); Serial.println(logMsg);
  lcd.clear();
  lcd.print("Fehler.. Karte??");
  lcd.setCursor(0, 1);
  lcd.print("Ya Sabr.."); // O Geduld...
  delay(3000);
  clearDisplay();
}