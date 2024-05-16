#include <Key.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN A0
#define SS_PIN  10

LiquidCrystal_I2C lcd(0x27, 20,4);
MFRC522 mfrc522(SS_PIN, RST_PIN);

int i=2;

const byte rows[4] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
const byte cols[4] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad 
 
char keys[4][4] = { //create 2D arry for keys
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'},
};
Keypad mykeypad = Keypad(makeKeymap(keys), rows, cols, 4, 4);

bool isFirstInput = true; // Variable to track first input
String inputString = ""; // Variable to store the input string

/*********************************************************************************************************************************/
void setup() {
  Serial.begin(9600); //enable serial monitor

  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();

  lcd.init();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("IGH Kermes 2024");
  lcd.setCursor(2,1);
}

/*********************************************************************************************************************************/
void loop() {
  char myKey = mykeypad.getKey(); //get key and put in to the variable

  if (myKey) {
    if (isFirstInput) { // Check if it's the first input
      clearSecondRow(); // Clear the second row for the first input
      isFirstInput = false; // Set isFirstInput to false after the first input
    }
    if (myKey == '#') {
      writeCredit(inputString); // Pass the input string to writeCredit() function
      inputString = ""; // Clear the input string
      isFirstInput = true; // Reset isFirstInput for the next input cycle
      return; // <== Semikolon am Ende des return-Statements
    }
    lcd.setCursor(i, 1);
    lcd.print(String(myKey) + ",-");
    inputString += myKey; // Add the pressed key to the input string
    i = i + 1;
  }

  /* 
  RFID
  When the RFID card has been inserted, the screen is reset and displays the new UID.  
  */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  // Clear the second row before displaying the RFID data
  clearSecondRow();
  
  // Print the UID of RFID card
  lcd.setCursor(0,0);
  lcd.print("ID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    lcd.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    lcd.print(mfrc522.uid.uidByte[i], HEX);
  }
  lcd.setCursor(0,0);
  i = 2;

  readCredit();
  
  mfrc522.PICC_HaltA(); // Halt PICC
}

/*********************************************************************************************************************************/
void clearSecondRow() {
  lcd.setCursor(0, 1); // Set cursor to the beginning of the second row
  lcd.print("                    "); // Print 20 spaces to clear the row (20x4 display)
  lcd.setCursor(0, 1); // Set cursor back to the beginning of the second row
  i = 2;
  inputString = "";
}

/*********************************************************************************************************************************/
void writeCredit(String input) { // Receive the input string as argument
  clearSecondRow();
  Serial.print("Guthaben wird aufgeladen für: ");
  Serial.println(input);
  readCredit();
}

/*********************************************************************************************************************************/
void readCredit(){
  clearSecondRow();
  lcd.setCursor(2,1);
  lcd.print("Guthaben: xx");
  Serial.print("Guthaben: xx");
  delay(2000);
  clearSecondRow();
  finishConversation();
}

void finishConversation() {
  lcd.setCursor(0,0);
  lcd.print("IGH Kermes 2024");
}
