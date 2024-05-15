#include <Key.h>
#include <Keypad.h>

const byte rows[4] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
const byte cols[4] = {6, 7, 8, 9}; //connect to the column pinouts of the keypad 
 
char keys[4][4] = { //create 2D arry for keys
  {'D', 'C', 'B', 'A'},
  {'#', '9', '6', '3'},
  {'0', '8', '5', '2'},
  {'*', '7', '4', '1'},
};
Keypad mykeypad = Keypad(makeKeymap(keys), rows, cols, 4, 4);

void setup() {
  Serial.begin(9600); //enable serial monitor
}
 
void loop() {
  char myKey = mykeypad.getKey(); //get key and put in to the variable
  if (myKey) { //check condition
    Serial.println(myKey); //print key   
  }
}