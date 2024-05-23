#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN A0
#define SS_PIN 10

LiquidCrystal_I2C lcd(0x27, 20, 4);  // Create LiquidCrystal_I2C instance
MFRC522 mfrc522(SS_PIN, RST_PIN);    // Create MFRC522 instance

int i = 2;  // for iteration over second row of the lcd display starts from 2

const byte rows[4] = { 2, 3, 4, 5 };  //connect to the row pinouts of the keypad
const byte cols[4] = { 6, 7, 8, 9 };  //connect to the column pinouts of the keypad

char keys[4][4] = {
  //create 2D array for keys
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' },
};

Keypad myKeypad = Keypad(makeKeymap(keys), rows, cols, 4, 4);

String inputString = "";   // String to hold input
bool cardPresent = false;  // Flag to indicate if a card is present

const String correctPassword = "1234"; // Set your password here

void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card
  
  lcd.init();
  lcd.backlight();
  clearDisplay();
  delay(3000);

  lcd.clear();

  // Prompt user to enter password
  if (checkPassword()) {
    lcd.clear();
    // Prompt user to enter a number (price)
    lcd.print("Preis eingeben:");
    inputString = getUserInput();
    lcd.clear();
    lcd.print("Preis:");
    lcd.setCursor(0, 1);
    lcd.print(inputString);
    delay(2000);
    clearDisplay();
  } else {
    lcd.clear();
    lcd.print("Falsches Passwort!");
    delay(2000);
    setup();
  }
}

void loop() {
  // Prompt cashier to enter amount
  lcd.print("Betrag eingeben:");
  String amountString = getUserInput();
  int amount = amountString.toInt();
  
  // Prompt user to scan card
  lcd.clear();
  lcd.print("Karte scannen...");
  
  // Wait for card
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Read and update credit on card
    updateCreditOnCard(-amount);
    
    // Display updated credit
    readCreditFromCard();
    
    mfrc522.PICC_HaltA(); // Halt PICC
    mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
    
    delay(5000); // Delay to allow user to see the result
    clearDisplay();
  }
}

bool checkPassword() {
  lcd.clear();
  lcd.print("Passwort eingeben:");
  String password = getUserInput();
  return password == correctPassword;
}

String getUserInput() {
  String input = "";
  char key;
  while (true) {
    key = myKeypad.getKey();
    if (key) {
      if (key == '#') {  // Assuming '#' is used to confirm input
        if (input.length() > 0) {
          break;
        }
      } else if (key == '*') {  // Assuming '*' is used to clear input
        input = "";
        lcd.setCursor(0, 1);
        lcd.print("                ");        // Clear the second row
      } else if (key >= '0' && key <= '9') {  // Only allow numeric input
        input += key;
        lcd.setCursor(0, 1);
        lcd.print(input);
      }  // Ignore 'A', 'B', 'C', and 'D'
    }
  }
  return input;
}

void updateCreditOnCard(int credit) {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    printError();
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    printError();
    return;
  }

  // Authenticate using key A
  byte block = 1;  // Block to write the credit value
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    printError();
    return;
  }

  // Read current credit
  byte buffer[18];
  byte len = 18;
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    printError();
    return;
  }

  // Convert buffer to current credit
  int currentCredit = (buffer[0] << 8) + buffer[1];

  // Add new credit to current credit
  int newCredit = currentCredit + credit;

  // Prepare buffer with new credit value
  buffer[0] = newCredit >> 8;                   // High byte of credit
  buffer[1] = newCredit & 0xFF;                 // Low byte of credit
  for (byte i = 2; i < 16; i++) buffer[i] = 0;  // Pad the rest with zeros

  // Write block
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    printError();
    return;
  } else {
    lcd.clear();
    lcd.print("Afiyet olsun :D");
    lcd.setCursor(0, 1);
    lcd.print("Guthaben: " + String(newCredit) + ",-");
    delay(4000);
    clearDisplay();
  }

  mfrc522.PICC_HaltA();       // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
}

void readCreditFromCard() {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    printError();
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    printError();
    return;
  }

  byte buffer[18];
  byte block = 1;  // Block to read the credit value
  MFRC522::StatusCode status;
  byte len = 18;

  // Authenticate using key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    printError();
    return;
  }

  // Read current credit
  status = mfrc522.MIFARE_Read(block, buffer, &len);
  if (status != MFRC522::STATUS_OK) {
    printError();
    return;
  }

  // Convert buffer to current credit
  int currentCredit = (buffer[0] << 8) + buffer[1];
  lcd.clear();  // Clear LCD display
  lcd.print("Guthaben: ");
  lcd.print(String(currentCredit) + ",-");  // Display entered credit on LCD
  delay(3000);
  clearDisplay();

  mfrc522.PICC_HaltA();       // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
}

void clearDisplay() {
  lcd.clear();
  lcd.print("IGH Kermes 2024");
}

void printError() {
  lcd.clear();
  lcd.print("Fehler.. Karte??");
  lcd.setCursor(0, 1);
  lcd.print("Ya Sabr..");
  delay(3000);
  clearDisplay();
}
