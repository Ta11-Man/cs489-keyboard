/*
 * ========================================================================
 * PICO KEYBOARD FIRMWARE (ARDUINO IDE)
 * ========================================================================
 * Board: Raspberry Pi Pico
 * Library: Keyboard.h
 * Logic: Matrix Scanning (Active High / Pulldown)
 * High on columns, pull from rows
 * Left side
 */

#include <Keyboard.h>

// ========================================================================
//   PIN CONFIGURATION
// ========================================================================

// COLUMNS (Outputs/Strobes) - Driven HIGH
const int COL_PINS[] = { 21, 20, 19, 18, 17, 16, 1 };

// ROWS (Inputs/Reads) - Set to PULLDOWN
const int ROW_PINS[] = { 28, 27, 26, 22 };

const int COL_COUNT = sizeof(COL_PINS) / sizeof(COL_PINS[0]);
const int ROW_COUNT = sizeof(ROW_PINS) / sizeof(ROW_PINS[0]);

// ========================================================================
//   KEYMAP CONFIGURATION
// ========================================================================

// Custom Keycodes
#define ______  0x00
#define KC_L1   0xF1 // Internal code for Layer 1

// Define Right-side modifiers if not defined by library
#ifndef KEY_LEFT_CTRL
#define KEY_LEFT_CTRL  0x84
#endif
#ifndef KEY_LEFT_SHIFT
#define KEY_LEFT_SHIFT 0x85
#endif
#ifndef KEY_LEFT_ALT
#define KEY_LEFT_ALT   0x86
#endif

// The Keymap (2 Layers, 4 Rows, 7 Cols)
const uint8_t keymaps[2][ROW_COUNT][COL_COUNT] = {
  // LAYER 0 (BASE)
  {
    { 'q',          'w',    'e',    'r',            't',          KEY_BACKSPACE, ______ }, // Row 28 (Index 0)
    { 'a',          's',    'd',    'f',            'g',          '?',           '!'    }, // Row 27 (Index 1)
    { KEY_LEFT_ALT, 'z',    'x',    'c',            'v',          KC_L1,         ______ }, // Row 26 (Index 2)
    { ______,       ______, ______, KEY_LEFT_SHIFT, KEY_LEFT_CTRL,               ______ }  // Row 22 (Index 3)
  },

  // LAYER 1 (SYM)
  {
    { '@',    '\'',   '$',    '/',            '-',          KEY_BACKSPACE, ______   },
    { '~',    '<',    '%',    '*',            '+',          '\'',          KEY_FUNC },
    { ';',    '>',    '&',    '^',            '=',          'KC_L1',       ______   },
    { ______, ______, ______, KEY_LEFT_SHIFT, KEY_LEFT_ALT, ' ',           ______   }
  }
};

// ========================================================================
//   STATE MANAGEMENT
// ========================================================================

bool keyState[ROW_COUNT][COL_COUNT];      // Current state
bool lastKeyState[ROW_COUNT][COL_COUNT];  // Previous state
unsigned long lastDebounceTime[ROW_COUNT][COL_COUNT];
const unsigned long DEBOUNCE_DELAY = 10;

int currentLayer = 0;

void setup() {
  // 1. Initialize Column Pins (Outputs)
  for (int i = 0; i < COL_COUNT; i++) {
    pinMode(COL_PINS[i], OUTPUT);
    digitalWrite(COL_PINS[i], LOW); // Start Low
  }

  // 2. Initialize Row Pins (Inputs)
  for (int i = 0; i < ROW_COUNT; i++) {
    pinMode(ROW_PINS[i], INPUT_PULLDOWN); // Active High Logic
  }

  // Initialize State Arrays
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c < COL_COUNT; c++) {
      keyState[r][c] = false;
      lastKeyState[r][c] = false;
      lastDebounceTime[r][c] = 0;
    }
  }

  // Start USB
  Keyboard.begin();
  delay(1000); 
}

void scanMatrix() {
  // First Pass: Determine Active Layer
  // We check the physical location of KC_L1 (Row 2, Col 0 in 0-indexed array)
  // Note: We need to know the state *before* sending keys, so we use the state from the previous scan
  // or logic inside the loop. For simplicity, we check keyState directly.
  if (keyState[2][0] == true) { 
    currentLayer = 1;
  } else {
    currentLayer = 0;
  }

  // Iterate Columns (Outputs)
  for (int col = 0; col < COL_COUNT; col++) {
    
    // Drive Column HIGH
    digitalWrite(COL_PINS[col], HIGH);
    delayMicroseconds(10); // Stability delay

    // Read Rows (Inputs)
    for (int row = 0; row < ROW_COUNT; row++) {
      int reading = digitalRead(ROW_PINS[row]);
      unsigned long now = millis();

      // Debounce Check
      if (reading != lastKeyState[row][col]) {
        lastDebounceTime[row][col] = now;
      }

      if ((now - lastDebounceTime[row][col]) > DEBOUNCE_DELAY) {
        // If state has stabilized and is different from stored state
        if (reading != keyState[row][col]) {
          keyState[row][col] = reading;

          // --- KEY EVENT HANDLER ---
          uint8_t keycode = keymaps[currentLayer][row][col];

          // Handle Layer Key specifically (don't send to PC)
          if (keycode == KC_L1) {
            // Layer logic is handled at start of scan, 
            // but we consume the event here so it doesn't print.
          } 
          else if (keycode != ______) {
            if (keyState[row][col] == HIGH) {
              // Key Pressed
              Keyboard.press(keycode);
            } else {
              // Key Released
              Keyboard.release(keycode);
            }
          }
        }
      }
      // Update last reading for debounce tracking
      lastKeyState[row][col] = reading;
    }

    // Drive Column LOW
    digitalWrite(COL_PINS[col], LOW);
  }
}

void loop() {
  scanMatrix();
  delay(1); // Short delay to yield
}