#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         A0           // Configurable, see typical pin layout above
#define SS_PIN          10           // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);    // Create MFRC522 instance

void setup() {
  Serial.begin(9600);                // Initialize serial communications with the PC
  SPI.begin();                       // Init SPI bus
  mfrc522.PCD_Init();                 // Init MFRC522 card
  Serial.println(F("Resetting RFID card to default..."));
}

void loop() {
  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Authenticate with the card
  MFRC522::StatusCode status;
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Clear block 1
  byte block = 1;
  byte buffer[18];
  memset(buffer, 0, 18);
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Writing to block 1 failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println(F("Block 1 cleared successfully."));
  }

  // Authenticate with the card for block 4
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 4, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Authentication for block 4 failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Clear block 4
  block = 4;
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Writing to block 4 failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  } else {
    Serial.println(F("Block 4 cleared successfully."));
  }

  Serial.println(F("RFID card reset to default successfully."));

  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  // Wait a bit before continuing to avoid reading the card multiple times
  delay(2000);
}
