#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "lcd1602.h"
#include "tm1637.h"
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Keypad configuration
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3
const uint keypadRows[] = {16, 17, 18, 19};
const uint keypadCols[] = {20, 21, 22};
char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

// Configuração do display LCD
#define I2C_PORT i2c0
#define LCD_SDA 6
#define LCD_SCL 7
#define LCD_ADDR 0x27
#define LCD_ROWS 2
#define LCD_COLS 16

// Button configuration
#define BUTTON_PIN 15

// Application states
typedef enum {
  STATE_IDLE,
  STATE_ASK_RESET,
  STATE_INPUT_MIN,
  STATE_INPUT_MAX,
  STATE_SHOW_MIN,
  STATE_SHOW_MAX
} AppState;

// Persistent storage variables
int temperaturaMinima = 0;
int temperaturaMaxima = 0;
char inputBuffer[17] = {0};

// Global state variables
AppState currentState = STATE_IDLE;
bool lcdOn = false;
bool lastButtonState = true;
absolute_time_t lastDebounceTime = 0;

void update_display() {
  lcd1602Clear();
  switch(currentState) {
    case STATE_ASK_RESET:
      lcd1602SetCursor(0, 0);
      lcd1602WriteString("Deseja redefinir");
      lcd1602SetCursor(0, 1);
      lcd1602WriteString("as temperaturas?");
      break;

    case STATE_INPUT_MIN:
      lcd1602SetCursor(0, 0);
      lcd1602WriteString("Temp min (C):");
      lcd1602SetCursor(0, 1);
      lcd1602WriteString(inputBuffer);
      break;

    case STATE_INPUT_MAX:
      lcd1602SetCursor(0, 0);
      lcd1602WriteString("Temp max (C):");
      lcd1602SetCursor(0, 1);
      lcd1602WriteString(inputBuffer);
      break;

    case STATE_SHOW_MIN: {
      char temp[16];
      snprintf(temp, sizeof(temp), "%d C", temperaturaMinima);
      lcd1602SetCursor(0, 0);
      lcd1602WriteString("Temp Minima:");
      lcd1602SetCursor(0, 1);
      lcd1602WriteString(temp);
      break;
    }

    case STATE_SHOW_MAX: {
      char temp[16];
      snprintf(temp, sizeof(temp), "%d C", temperaturaMaxima);
      lcd1602SetCursor(0, 0);
      lcd1602WriteString("Temp Maxima:");
      lcd1602SetCursor(0, 1);
      lcd1602WriteString(temp);
      break;
    }

    default:
      break;
  }
}

void handle_button_press() {
  bool buttonState = gpio_get(BUTTON_PIN);
  if (buttonState != lastButtonState) {
    lastDebounceTime = get_absolute_time();
  }

  if (absolute_time_diff_us(lastDebounceTime, get_absolute_time()) > 10000) {
    if (!buttonState) {
      switch(currentState) {
        case STATE_ASK_RESET:
          currentState = STATE_INPUT_MIN;
          memset(inputBuffer, 0, sizeof(inputBuffer));
          break;

        case STATE_INPUT_MIN:
          temperaturaMinima = atoi(inputBuffer);
          currentState = STATE_INPUT_MAX;
          memset(inputBuffer, 0, sizeof(inputBuffer));
          break;

        case STATE_INPUT_MAX:
          temperaturaMaxima = atoi(inputBuffer);
          currentState = STATE_IDLE;
          lcdOn = false;
          lcd1602Clear();
          lcd1602Control(0, 0, 0);
          break;

        case STATE_SHOW_MIN:
          currentState = STATE_SHOW_MAX;
          break;

        case STATE_SHOW_MAX:
          currentState = STATE_SHOW_MIN;
          break;

        default:
          break;
      }
      update_display();
    }
  }
  lastButtonState = buttonState;
}

void setupKeypad() {
  for (int i = 0; i < 4; i++) {
    gpio_init(keypadRows[i]);
    gpio_set_dir(keypadRows[i], GPIO_OUT);
    gpio_put(keypadRows[i], 0);
  }
  for (int i = 0; i < 3; i++) {
    gpio_init(keypadCols[i]);
    gpio_set_dir(keypadCols[i], GPIO_IN);
    gpio_pull_down(keypadCols[i]);
  }
}

char readKeypad() {
  for (uint8_t row = 0; row < 4; row++) {
    gpio_put(keypadRows[row], 1);
    sleep_us(10);
    for (uint8_t col = 0; col < 3; col++) {
      if (gpio_get(keypadCols[col])) {
        gpio_put(keypadRows[row], 0);
        return keymap[row][col];
      }
    }
    gpio_put(keypadRows[row], 0);
    sleep_us(10);
  }
  return '\0';
}

int main() {
  stdio_init_all();
  setupKeypad();

  // Initialize LCD
  if (lcd1602Init(0, 0x27) != 0) {
    printf("LCD initialization failed!\n");
    return 1;
  }
  lcd1602Control(0, 0, 0); // Start with backlight off
  lcdOn = false;

  // Initialize button
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  while(1) {
    char key = readKeypad();
    handle_button_press();

    if (key != '\0') {
      switch(currentState) {
        case STATE_IDLE:
          if (key == '*') {
            if (!lcdOn) {
              lcd1602Control(1, 0, 0);
              lcdOn = true;
            }
            currentState = STATE_ASK_RESET;
            update_display();
          } else if (key == '#') {
            if (!lcdOn) {
              lcd1602Control(1, 0, 0);
              lcdOn = true;
            }
            currentState = STATE_SHOW_MIN;
            update_display();
          }
          break;

        case STATE_ASK_RESET:
          if (key == '#') {
            currentState = STATE_IDLE;
            lcdOn = false;
            lcd1602Clear();
            lcd1602Control(0, 0, 0);
          }
          break;

        case STATE_INPUT_MIN:
        case STATE_INPUT_MAX:
          if (isdigit(key)) {
            if (strlen(inputBuffer) < LCD_COLS) {
              strncat(inputBuffer, &key, 1);
              update_display();
            }
          }
          break;

        case STATE_SHOW_MIN:
        case STATE_SHOW_MAX:
          if (key == '#') {
            currentState = STATE_IDLE;
            lcdOn = false;
            lcd1602Clear();
            lcd1602Control(0, 0, 0);
          }
          break;
      }
      sleep_ms(250);
    }
    sleep_ms(10);
  }

  lcd1602Shutdown();
  return 0;
}
