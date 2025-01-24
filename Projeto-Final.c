#include "pico/stdlib.h"
#include "tm1637.h"
#include <pico/time.h>
#include <stdio.h>

#define DISPLAY_CLK 2
#define DISPLAY_DIO 3

int main(int argc, char **argv) {
  int i;
  char szTemp[8];

  tm1637Init(CLK, DIO);
  tm1637SetBrightness(5);

  for (i = 0; i < 9999; i++) {
    if (i & 1)
      sprintf(szTemp, "%02d:%02d", i / 100, i % 100);
    else
      sprintf(szTemp, "%02d %02d", i / 100, i % 100);
    tm1637ShowDigits(szTemp);
    sleep_ms(1000);
  }
  return 0;
}
