// Filename: Lab3_Part1
// Author: Raymond Chinn, Xander Geurkink
// Date: 11/5/2025
// Description: Reads serial input and displays it on an I2C LCD using low-level
//              I2C communication (Wire library).
//
// Ensure that the LiquidCrystal I2C library by Frank de Brabander is installed in your Arduino IDE Library Manager
// 
// Version: 1.0
// ============================================================================
// INCLUDES
// ============================================================================
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// LCD CONFIGURATION
// ============================================================================
#define LCD_ADDR 0x27                    // I2C address of LCD (typically 0x27 or 0x3F)
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);  // LCD object: 16 columns x 2 rows

// ============================================================================
// LCD COMMAND CODES 
// ============================================================================
#define LCD_CLEAR_DISPLAY 0x01    // Clear entire display
#define LCD_CURSOR_HOME 0x80      // Set cursor to first line, first position

// Name: setup
// Description: Initializes serial communication, I2C bus, and LCD display.
void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  delay(2);
}

// Name: loop
// Description: Continuously monitors serial input. When data is available,
//              clears the LCD and displays received text using low-level I2C
//              commands.
void loop() {
  // Write code that takes Serial input and displays it to the LCD.
  if (Serial.available() > 0) {
    sendCommand(LCD_CLEAR_DISPLAY);  // Clear LCD screen
    delay(2);
    sendCommand(LCD_CURSOR_HOME);  // Set cursor to first line

    // Read and display characters from Serial Monitor
    while (Serial.available() > 0) {
      char c = Serial.read();

      // Only display printable characters
      if (c >= 32 && c <= 126) {
        sendData(c);
      }

      delay(10);
    }
  }
}

// ============================================================================
// LCD COMMUNICATION FUNCTIONS
// ============================================================================

// Name: sendCommand
// Description: Takes a command byte and sends to the LCD (RS=0). Used for instructions
//              like clear display, cursor positioning, display control, etc.
void sendCommand(uint8_t cmd) {
  sendByte(cmd, 0x00); // send mode = 00000000 for to be OR'ed and set RS bit to 0
  delay(50);
}

// Name: sendData
// Description: Sends a data byte to the LCD (RS=1). Used for displaying
//              characters on the screen.
void sendData(uint8_t data) {
  sendByte(data, 0x01); // send mode = 00001111 for to be OR'ed and set RS bit to 1
  delay(50);
}

// Name: sendByte
// Description: Takes an unsigned 8-bit character data and RS bit value (0x00 command, 0x01 data)
//              Sends a full 8-bit byte to the LCD by splitting it into two 4-bit nibbles 
//              (upper and lower). LCD operates in 4-bit mode, so each byte requires two 
//              transmission cycles.
void sendByte(uint8_t data, uint8_t mode) {
  uint8_t upper_4 = data & 0xF0; // Extract upper four bits of data to LCD
  uint8_t lower_4 = (data << 4) & 0xF0; // Extract lower four bits of data to LCD
  
  // Send upper and lower nibbles of data byte seperately to LCD
  sendNibble(upper_4, mode);
  sendNibble(lower_4, mode);
}

// Name: sendNibble
// Description: Takes an unsigned 8-bit character data storing a nibble and RS bit value.
//              Sends a 4-bit nibble to the LCD via I2C. Uses enable pulse
//              (high-to-low transition) to latch data into LCD. 
void sendNibble(uint8_t nibble, uint8_t mode) {
  // Create byte for LCD by OR-ing together the uint8's for nibble, backlight, and mode
  uint8_t data = nibble | mode | 0x08;

  // Write and toggle enable bit tp HIGH 
  Wire.beginTransmission(0x27);
  Wire.write(data | 0x04);
  Wire.endTransmission();
  delay(5);

  // Write again and toggle enable bit to LOW to latch data
  Wire.beginTransmission(0x27);
  Wire.write(data & ~0x04);
  Wire.endTransmission();
  delay(50);
}