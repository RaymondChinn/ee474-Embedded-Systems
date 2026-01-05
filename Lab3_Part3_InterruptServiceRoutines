// Filename: Lab3_Part3
// Author: Raymond Chinn, Xander Geurkink
// Date: 11/5/2025
// Description: This file demonstrates the concept and effects of interrupts and ISRs  
// through a button, timer, BLE signal, and LCD screen.
//
// ============================================================================
// INCLUDES
// ============================================================================
// BLE (Bluetooth Low Energy) Libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// I2C LCD Display Libraries
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// BLE CONFIGURATION
// ============================================================================
#define SERVICE_UUID "8edc2edf-93df-4fee-8e82-d90264dcc220"
#define CHARACTERISTIC_UUID "4de7c084-cfc4-4045-add6-ae43826b55e4"

// ============================================================================
// HARDWARE PIN DEFINITIONS
// ============================================================================
#define BUTTON_PIN 7

// ============================================================================
// HARDWARE OBJECTS
// ============================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // LCD at I2C address 0x27, 16 cols x 2 rows
hw_timer_t *timer = NULL;             // Hardware timer for counter

// ============================================================================
// GLOBAL STATE VARIABLES
// ============================================================================
// Interrupt flags (volatile because modified in ISR)
volatile bool buttonPressed = false;      // Set by button interrupt
volatile bool bleSignalReceived = false;  // Set by BLE callback
volatile int counter = 0;  
volatile int currTime = 0;               // Incremented every second by timer

// Display management variables
volatile bool displayingMessage = false;  // True when showing temporary message
unsigned long messageStartTime = 0;       // Timestamp when message started

// ============================================================================
// BLE CALLBACK CLASS
// ============================================================================
// Name: MyCallbacks
// Description: Handles BLE characteristic write events. When a BLE client
//              writes to the characteristic, sets the bleSignalReceived flag.
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    bleSignalReceived = true;  // Flag that BLE signal was received
  }
};


// ============================================================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================

// Name: onTimer
// Description: Timer ISR that increments the counter every second.
//              IRAM_ATTR ensures this function is stored in RAM for fast access.
void IRAM_ATTR onTimer() {
  counter++;  // Increment counter (called every 1 second)
}

// Name: handleButtonInterrupt
// Description: External interrupt handler triggered when button is pressed.
//              Sets buttonPressed flag for main loop to handle.
void IRAM_ATTR handleButtonInterrupt() {
  buttonPressed = true;  // Flag that button was pressed
}

// Name: setupBLE
// Description: Initializes BLE with device name, creates service and 
//              characteristic, attaches callbacks, and starts advertising
void setupBLE() {
  BLEDevice::init("Raymond'sESP32");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

// Name: setup
// Description: Main initialization function. Sets up serial communication,
//              LCD display, hardware timer, button interrupt, and BLE.
void setup() {
  Serial.begin(115200);
  
 // --- LCD Initialization ---
  Wire.begin();        // Initialize I2C communication
  lcd.init();          // Initialize LCD
  lcd.backlight();     // Turn on LCD backlight
  delay(2);            // Small delay for LCD stability

  // Creates a timer, attaches an interrupt, and sets alarm for counter incrementing
  timer = timerBegin(1000000); // 1MHz frequency 
  timerAttachInterrupt(timer, &onTimer); // Attach timer ISR to timer
  timerAlarm(timer, 1000000, true, 0); // 1000000 us = 1 sec, unlimited auto-reload enabled

  // Configure button pin as input with internal pull-up resistor
  // This means the pin reads HIGH normally and LOW when button is pressed
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Attach an external interrupt to the button pin
  // Triggers on FALLING edge (i.e., when button is pressed)
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonInterrupt, FALLING);

  // BLE Setup 
  setupBLE();
}

// Name: loop
// Description: Main program loop. Handles button presses, BLE signals, 
//              message display timing, and counter updates on LCD.
void loop() {
  // If the button has been pressed, print out "Button Pressed" on the LCD.
  
  if (buttonPressed) {
    currTime = counter;
    buttonPressed = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Button Pressed");

    displayingMessage = true;
    messageStartTime = millis();
  }

  // If a signal has been received over BLE, print out “New Message!” on the LCD.
  if (bleSignalReceived) {
    currTime = counter;
    bleSignalReceived = false;
    displayingMessage = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New Message!");

    messageStartTime = millis();
  }

  // Check if 2 seconds have passed to resume counter
  if (displayingMessage && (millis() - messageStartTime >= 2000)) {
    displayingMessage = false;
    lcd.clear();
    counter = currTime;
  }

  // Update counter display when not showing a message
  if (!displayingMessage) {
    lcd.setCursor(0, 0);
    lcd.print("Counter:        ");  
    lcd.setCursor(0, 0);
    lcd.print("Counter: ");
    lcd.print(counter);
  }

  delay(100);  
}
