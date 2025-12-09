/*
 * ========================================================================
 * PICO KEYBOARD FIRMWARE (ARDUINO IDE)
 * ========================================================================
 * File: left.ino
 * Board: Raspberry Pi Pico
 * Library: Keyboard.h (Built-in to Arduino Mbed OS RP2040 Board package)
 * Features: 4 Layers, Sticky Keys, Pinout for Left Hand
 *
 */

#include <Keyboard.h>

// ========================================================================
//   PIN CONFIGURATION
// ========================================================================

// COLUMNS (Outputs/Strobes) - Driven HIGH during scan
const int COL_PINS[] = { 21, 20, 19, 18, 17, 16, 1 }; 

// ROWS (Inputs/Reads) - Set to PULLDOWN
const int ROW_PINS[] = { 28, 27, 26, 22 };

const int COL_COUNT = sizeof(COL_PINS) / sizeof(COL_PINS[0]);
const int ROW_COUNT = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);

// Onboard LED Pin
#define LED_PIN 25

// ========================================================================
//   KEY DEFINITIONS & MACROS
// ========================================================================

#define ______  0x00

// Internal Layer/Function Codes
#define KC_L1     0xF1 
#define KEY_FUNC  0xF2
#define STICKY    0xF3

// Standard Key Mapping
#define DEL       KEY_DELETE
#define SUPER     KEY_LEFT_GUI
#define UP        KEY_UP_ARROW
#define DOWN      KEY_DOWN_ARROW
#define LEFT      KEY_LEFT_ARROW
#define RIGHT     KEY_RIGHT_ARROW

// F-Keys
#define F1  KEY_F1
#define F2  KEY_F2
#define F3  KEY_F3
#define F4  KEY_F4
#define F5  KEY_F5
#define F6  KEY_F6
#define F7  KEY_F7
#define F8  KEY_F8
#define F9  KEY_F9
#define F10 KEY_F10
#define F11 KEY_F11
#define F12 KEY_F12

// Media Keys (Placeholders)
#define VOL_UP    0x00 
#define VOL_DOWN  0x00

// Modifiers
#ifndef KEY_LEFT_CTRL
#define KEY_LEFT_CTRL   0x80
#endif
#ifndef KEY_LEFT_SHIFT
#define KEY_LEFT_SHIFT  0x81
#endif
#ifndef KEY_LEFT_ALT
#define KEY_LEFT_ALT    0x82
#endif

// ========================================================================
//   KEYMAP CONFIGURATION (4 LAYERS)
// ========================================================================

const uint8_t keymaps[4][ROW_COUNT][COL_COUNT] = {
  // LAYER 0 (BASE)
  {
    { 'q',          'w',    'e',    'r',            't',          KEY_BACKSPACE, ______ },
    { 'a',          's',    'd',    'f',            'g',          '?',           '!'    },
    { KEY_LEFT_ALT, 'z',    'x',    'c',            'v',          KC_L1,         ______ },
    { ______,       ______, ______, KEY_LEFT_SHIFT, KEY_LEFT_CTRL, ' ',          ______ }
  },

  // LAYER 1 (SYM) - Triggered by KC_L1
  {
    { '@',    ':',   '$',    '/',            '-',          KEY_BACKSPACE, ______   },
    { '~',    '<',    '%',    '*',            '+',          '\'',          KEY_FUNC },
    { ';',    '>',    '&',    '^',            '=',          KC_L1,         ______   },
    { ______, ______, ______, KEY_LEFT_SHIFT, KEY_LEFT_CTRL, ' ',           ______   }
  },
  // LAYER 2 (FN) - Triggered by KEY_FUNC (on Layer 1)
  {
    { F1,     F2,     F3,     F4, F5, DEL, ______ },
    { F6,     F7,     F8,     F9, F10, VOL_UP, KEY_FUNC },
    { STICKY, SUPER, ______,  F11, F12,  KC_L1,  ______ },
    { ______, ______, ______, KEY_LEFT_SHIFT, KEY_LEFT_CTRL, VOL_DOWN, ______ }
  },

  // LAYER 3 (SHIFTED LAYER 1) - Triggered by Layer 1 + Shift
  {
    { ______, ______, ______, '\\', '_', ______, ______ },
    { ______, ______, '|', UP, '`', '\"', ______ },
    { ______, ______, LEFT, DOWN, RIGHT, ______,  ______ },
    { ______, ______, ______, ______, ______, ______, ' ' }
  }
};

// ========================================================================
//   STATE MANAGEMENT
// ========================================================================

bool keyState[ROW_COUNT][COL_COUNT];       // Current stable state
bool prevInputState[ROW_COUNT][COL_COUNT]; // State from previous loop (for edge detection)
bool lastRawState[ROW_COUNT][COL_COUNT];   // Raw state for debouncing
unsigned long lastDebounceTime[ROW_COUNT][COL_COUNT];
const unsigned long DEBOUNCE_DELAY = 5;    

// Sticky Logic State
bool stickyModeEnabled = false; 
bool stickyL1 = false;          
bool stickyFunc = false;        
bool stickyShift = false;       
bool stickyCtrl = false;        
bool stickyAlt = false;         

void setup() {
  // Initialize Column Pins (Outputs)
  for (int i = 0; i < COL_COUNT; i++) {
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(COL_PINS[i], LOW);
  }
  // Initialize Row Pins (Inputs)
  for (int i = 0; i < ROW_COUNT; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLDOWN);
  }

  // Initialize LED Pin (Output)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start LED off

  // Initialize Arrays
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COL_COUNT; c++) {
      keyState[r][c] = false;
      prevInputState[r][c] = false;
      lastRawState[r][c] = false;
      lastDebounceTime[r][c] = 0;
    }
  }

  Keyboard.begin();
  delay(500); 

  // Send a report to release all keys on connection/reboot.
  Keyboard.releaseAll(); 
  delay(500);
}

void scanMatrix() {
  // 1. SCAN PHYSICAL MATRIX & UPDATE STATES
  for (int col = 0; col < COL_COUNT; col++) {
    digitalWrite(COL_PINS[col], HIGH);
    delayMicroseconds(10);

    for (int row = 0; row < ROW_COUNT; row++) {
      int reading = digitalRead(ROW_PINS[row]);
      unsigned long now = millis();

      // Debounce Logic
      if (reading != lastRawState[row][col]) {
        lastDebounceTime[row][col] = now;
      }

      if ((now - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
        if (reading != keyState[row][col]) {
          keyState[row][col] = reading;
        }
      }
      lastRawState[row][col] = reading;
    }
    digitalWrite(COL_PINS[col], LOW);
  }

  // --- Check if any key is held to control the LED ---
  bool isAnyKeyPressed = false;
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COL_COUNT; c++) {
      if (keyState[r][c]) {
        isAnyKeyPressed = true;
        break; 
      }
    }
    if (isAnyKeyPressed) break; 
  }
  digitalWrite(LED_PIN, isAnyKeyPressed ? HIGH : LOW);
  // --------------------------------------------------------

  // 2. LOGIC PHASE: CALCULATE LAYERS
  
  bool activeL1   = stickyL1;
  bool activeFunc = stickyFunc;
  bool activeShift = stickyShift;
  
  if (keyState[2][5]) activeL1 = true;
  if (keyState[1][6]) activeFunc = true;
  if (keyState[3][2]) activeShift = true;
  if (keyState[3][3] && stickyModeEnabled) stickyCtrl = true; 
  
  int currentLayer = 0; 
  if (activeL1) {
    currentLayer = 1; 
    if (activeShift) currentLayer = 3; 
    if (activeFunc) currentLayer = 2; 
  }

  // 3. ACTION PHASE: SEND KEYS (Edge Triggered)
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COL_COUNT; c++) {
      
      bool pressed = keyState[r][c];
      bool prevPressed = prevInputState[r][c];
      
      // CRITICAL OPTIMIZATION: Only process if state CHANGED
      if (pressed == prevPressed) {
        continue; 
      }
      
      // Update history for next loop
      prevInputState[r][c] = pressed;

      uint8_t code = keymaps[currentLayer][r][c];
      
      // -- HANDLE SPECIAL KEYS --
      if (code == STICKY) {
        if (pressed) { // On Press Only
           
           if (stickyModeEnabled) {
              Keyboard.releaseAll();
           }

           stickyModeEnabled = !stickyModeEnabled; // Toggle Mode
           
           if (!stickyModeEnabled) {
             // Reset all sticky flags after sending release signal
             stickyL1 = stickyFunc = stickyShift = stickyCtrl = stickyAlt = false;
           }
        }
        continue;
      }

      if (code == KC_L1) {
        if (stickyModeEnabled && pressed) stickyL1 = !stickyL1;
        continue; 
      }

      if (code == KEY_FUNC) {
        if (stickyModeEnabled && pressed) stickyFunc = !stickyFunc;
        continue;
      }

      // -- HANDLE MODIFIERS --
      if (code == KEY_LEFT_SHIFT || code == KEY_LEFT_CTRL || code == KEY_LEFT_ALT) {
         if (stickyModeEnabled) {
           if (pressed) { // Toggle on press in sticky mode
             if (code == KEY_LEFT_SHIFT) stickyShift = !stickyShift;
             if (code == KEY_LEFT_CTRL)  stickyCtrl  = !stickyCtrl;
             if (code == KEY_LEFT_ALT)   stickyAlt   = !stickyAlt;
           }
         } else {
           // Normal Mode: Pass through directly
           if (pressed) Keyboard.press(code);
           else Keyboard.release(code);
         }
         continue;
      }

      // -- HANDLE STANDARD KEYS --
      if (code != ______) {
        if (pressed) {
          // Apply sticky modifiers just before key press
          if (stickyShift) Keyboard.press(KEY_LEFT_SHIFT);
          if (stickyCtrl)  Keyboard.press(KEY_LEFT_CTRL);
          if (stickyAlt)   Keyboard.press(KEY_LEFT_ALT);
          
          Keyboard.press(code);
        } else {
          Keyboard.release(code);
        }
      }
    }
  }
}

void loop() {
  scanMatrix();
}
