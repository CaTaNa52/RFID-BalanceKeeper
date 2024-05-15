# RFID-BalanceKeeper Documentation

Welcome to the official documentation for the RFID-BalanceKeeper project!

## Contents

- [Description](#description)
- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Troubleshooting](#troubleshooting)
- [Resources](#resources)
- [Contact](#contact)

## Description

RFID-BalanceKeeper is an hobby project for managing balances on RFID cards. This project provides a simple and effective way to use RFID cards to load funds with money and pay at designated locations.
   
The purpose of the project is to create a cost-effective solution for loading and using credit using rfid cards. 

Possible applications include festivals, fairs and similar events. Most festivals use tokens. The problem with these is that they can usually no longer be used after they have been used once. They have to be purchased again for each new festival. It is also time-consuming for the stand employee to accept tokens and give change. 
So there is room for improvement, especially as the queues at the stands are getting longer and longer. With our project, these processes can be improved and optimized.  

**Hardware components:**
1. RFID Card
2. Terminal (Cashier)
3. Terminal (Stand staff)

**Top up credit:**
1. visitor enters the festival area and goes to the cash desk as usual
2. visitor hands the cashier the money, e.g. EUR 20.00
3. cashier types EUR 20.00 into the terminal device, places the RFID card on it and confirms.
4. RFID card with EUR 20.00 credit is handed over to the visitor.

**Use credit:**
1. visitor goes to the cake stand and wants to buy a piece of cake.
2. stand employee types the price of the piece of cake into his terminal and confirms
3. visitor places his card on the terminal.
4. an acoustic signal indicates whether the credit has been deducted or has failed.
5. the terminal then shows how much credit is left on the card

## Requirements
List of hardware components -> [Click here](https://github.com/CaTaNa52/RFID-BalanceKeeper/tree/main/Hardware) 

**Functional requirements:** 
- **_Identification_**: The RFID card MUST be able to transmit a unique identifier that allows the reader to distinguish it from other cards.
- **_Data_ _Storage_**: The card SHOULD have the ability to store certain information associated with the user or their profile, such as access permissions and balance.
- **_Security_**: The card SHOULD integrate mechanisms to secure the transmitted data to prevent unauthorized access or manipulation.
- **_Read_ _Range_**: The RFID card SHOULD have a sufficient read range to ensure reliable communication with the reader without the need for physical contact.
- **_Lifespan_**: The card SHOULD have a sufficiently long lifespan to enable long-term use without needing regular replacement.
- **_Compatibility_**: The RFID card SHOULD be compatible with common RFID readers and systems to ensure smooth integration and interoperability.
- **_Responsiveness_**: The card SHOULD respond quickly to reader requests and perform data transfers efficiently to minimize delays in access.

**Nonfunctional requirements:**
- All components except for the RFID card MUST be created using a 3D printer.
- All components MUST be disinfectable.
- All components MUST be robust against 'light' impacts.
- All components MUST be securely fastened.
- All components must be securely enclosed.
- The terminals MUST be able to lay flat on a table.
- The user interface of the terminals MUST be easy to operate.
- Adequate space for expansion and cooling of a battery MUST be provided in the simulation dummy.
- An appropriate acoustic signal MUST be emitted upon successful loading or unloading of credit.
- The terminals MUST be implemented with an Arduino.


## Installation
Step-by-step guide for installing the project and all necessary components. \
[Building instructions](./building%20instructions)

## Configuration
Instructions for configuring the project and its components. \
// TODO

## Usage
A brief guide on how to use the project and its features. \
// TODO

## Troubleshooting
Common issues and their solutions. \
// TODO

## Resources
Links to additional resources such as manuals, tutorials, or forums. \
// TODO

## Contact
@Habib Kilic: <<habibkilic@hotmail.de>> \
@Yusuf Türkyilmaz <<yusuf_tuerkyilmaz@hotmail.de>> \
@Bilal Aydemir <<bilalaydemir@outlook.de>>


---

Please note that this documentation may not cover all details. For further information, refer to the respective sections or other documents in the repository.
