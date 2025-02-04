#include "pico/stdlib.h"
#include "tm1637.h"
#include <hardware/gpio.h>
#include <pico/stdio.h>
#include <pico/time.h>
#include <stdio.h>

#define DISPLAY_CLK 2
#define DISPLAY_DIO 3
// #define KEY_R1 16
// #define KEY_R2 17
// #define KEY_R3 18
// #define KEY_R4 19
// #define KEY_C1 20
// #define KEY_C2 21
// #define KEY_C3 22
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 3

const uint keypadRows[] = {16, 17, 18, 19};  // R1 até R4
const uint keypadCols[] = {20, 21, 22};          // C1 até C3

char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
  { '1', '2', '3' },
  { '4', '5', '6' },
  { '7', '8', '9' },
  { '*', '0', '#' }
};

void setupKeypad () {
  // Inicialize os pinos de linha, como output e com estado baixo
  for (int i = 0; i < 4; i++) {
    gpio_init(keypadRows[i]);
    gpio_set_dir(keypadRows[i], GPIO_OUT);
    gpio_put(keypadRows[i], 0);
  }

  // Inicialize os pinos de coluna, como input e em pulldown
  for (int i = 0; i < 3; i++) {
    gpio_init(keypadCols[i]);
    gpio_set_dir(keypadCols[i], GPIO_IN);
    gpio_pull_down(keypadCols[i]);
  }
}

char readKeypad() {
  for (uint8_t row = 0; row < 4; row++) {
    // Ativa a linha atual
    gpio_put(keypadRows[row], 1);
    sleep_us(10);  // Allow signals to settle

    // Verifica cada coluna
    for (uint8_t col = 0; col < 3; col++) {
      if (gpio_get(keypadCols[col])) {
        // Desativa alinha antes do retorno para evitar ghosting
        gpio_put(keypadRows[row], 0);
        return keymap[row][col];
      }
    }

    // Desativa a linha depois de verificar as colunas
    gpio_put(keypadRows[row], 0);
    sleep_us(10);
  }
  return '\0';
}

int main(int argc, char **argv) {
  stdio_init_all();
  // int i;
  // char szTemp[8];

  // Inicializa o display de 7 segmentos
  tm1637Init(DISPLAY_CLK, DISPLAY_DIO);
  tm1637SetBrightness(5);

  // Inicializa o teclado matricial
  setupKeypad();


  while (1) {
    // for (i = 0; i < 9999; i++) {
    //   if (i & 1)
    //     sprintf(szTemp, "%02d:%02d", i / 100, i % 100);
    //   else
    //     sprintf(szTemp, "%02d %02d", i / 100, i % 100);
    //   tm1637ShowDigits(szTemp);
    //   sleep_ms(1000);
    // }
    char key = readKeypad();
    if (key != '\0') {
      printf("Pressed: %c\n", key);
      sleep_ms(250);  // Simple debounce delay
    }
    sleep_ms(10);  // Reduce CPU usage

  }
  return 0;
}
