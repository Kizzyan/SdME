#include "tm1637.h"
#include "pico/stdlib.h"
#include <hardware/gpio.h>
#include <pico/time.h>
#include <stdio.h>
#include <unistd.h>
//
// Larry's TM1637 library
// Copyright (c) 2018 BitBank Software, Inc.
// Project started 3/17/2018
// written by Larry Bank (bitbank@pobox.com)
//
// The TM1637 from Titan Micro Electronics is a 7-segment LED controller
// It features a 2-wire interface (Clock + Data), but doesn't conform to I2C nor
// SPI standards It can control up to 6 7-segment digits along with decimal
// points or colons as well as has 16 button/keyboard inputs The datasheet says
// that it runs on 5V with 5V logic, but I've run it on 3.3V (both Vcc and
// signals) and it seems to behave correctly. Since the communication protocol
// is custom, it requires "bit-banging" of 2 IO pins to control it.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//
// table which translates a digit into the segments
//
const unsigned char cDigit2Seg[] = {0x3f, 0x6,  0x5b, 0x4f, 0x66,
                                    0x6d, 0x7d, 0x07, 0x7f, 0x6f};
static unsigned char bClockPin, bDataPin;
#define CLOCK_DELAY 5
#define LOW 0
#define HIGH 1
//
// Initialize the tm1637 by specifying the digital pins used to communicate
// returns 0 for success, -1 for failure
//
int tm1637Init(unsigned char bClock, unsigned char bData) {
  bClockPin = bClock;
  bDataPin = bData;
  gpio_init(bClockPin);
  gpio_set_dir(bClockPin, GPIO_OUT);
  gpio_init(bDataPin);
  gpio_set_dir(bDataPin, GPIO_OUT);
  gpio_put(bClockPin, LOW);
  gpio_put(bDataPin, LOW);
  return 0;
} /* tm1637Init() */
//
// Start wire transaction
//
static void tm1637Start(void) {
  gpio_put(bDataPin, HIGH);
  gpio_put(bClockPin, HIGH);
  sleep_us(CLOCK_DELAY);
  gpio_put(bDataPin, LOW);
} /* tm1637Start() */
//
// Stop wire transaction
//
static void tm1637Stop(void) {
  gpio_put(bClockPin, LOW); // clock low
  sleep_us(CLOCK_DELAY);
  gpio_put(bDataPin, LOW); // data low
  sleep_us(CLOCK_DELAY);
  gpio_put(bClockPin, HIGH); // clock high
  sleep_us(CLOCK_DELAY);
  gpio_put(bDataPin, HIGH); // data high
} /* tm1637Stop() */
//
// Get data ack
//
static unsigned char tm1637GetAck(void) {
  unsigned char bAck = 1;

  //
  // With the sysfs GPIO driver, ArmbianIO is not prepared to reverse
  // the direction of an existing GPIO pin
  // We can safely disable this code on Linux
  //
  // read ack
  gpio_put(bClockPin, LOW); // clock to low
  //   AIOAddGPIO(bDataPin, GPIO_IN); // data as input
  sleep_us(CLOCK_DELAY);

  //   bAck = AIOReadGPIO(bDataPin);
  gpio_put(bClockPin, HIGH); // clock high
  sleep_us(CLOCK_DELAY);
  gpio_put(bClockPin, LOW); // clock to low

  //   AIOAddGPIO(bDataPin, GPIO_OUT);

  return bAck;
} /* tm1637GetAck() */
//
// Write a unsigned char to the controller
//
static void tm1637WriteByte(unsigned char b) {
  unsigned char i;

  for (i = 0; i < 8; i++) {
    gpio_put(bClockPin, LOW); // clock low
    if (b & 1)                // LSB to MSB
      gpio_put(bDataPin, HIGH);
    else
      gpio_put(bDataPin, LOW);
    sleep_us(CLOCK_DELAY);
    gpio_put(bClockPin, HIGH); // clock high
    sleep_us(CLOCK_DELAY);
    b >>= 1;
  }
} /* tm1637Writeunsigned char() */
//
// Write a sequence of unsigned chars to the controller
//
static void tm1637Write(unsigned char *pData, unsigned char bLen) {
  unsigned char b, bAck;
  bAck = 1;
  tm1637Start();
  for (b = 0; b < bLen; b++) {
    tm1637WriteByte(pData[b]);
    bAck &= tm1637GetAck();
  }
  tm1637Stop();
} /* tm1637Write() */
//
// Set brightness (0-8)
//
void tm1637SetBrightness(unsigned char b) {
  unsigned char bControl;
  if (b == 0)        // display off
    bControl = 0x80; // display off
  else {
    if (b > 8)
      b = 8;
    bControl = 0x88 | (b - 1);
  }
  tm1637Write(&bControl, 1);
} /* tm1637SetBrightness() */
//
// Display a string of 4 digits and optional colon
// by passing a string such as "12:34" or "45 67"
//
void tm1637ShowDigits(char *pString) {
  unsigned char b, bTemp[16]; // commands and data to transmit
  unsigned char i, j;

  j = 0;
  bTemp[0] = 0x40; // memory write command (auto increment mode)
  tm1637Write(bTemp, 1);

  bTemp[j++] = 0xc0; // set display address to first digit command
  for (i = 0; i < 5; i++) {
    if (i == 2) // position of the colon
    {
      if (pString[i] == ':') // turn on correct bit
        bTemp[2] |= 0x80;    // second digit high bit controls colon LEDs
    } else {
      b = 0;
      if (pString[i] >= '0' && pString[i] <= '9') {
        b = cDigit2Seg[pString[i] & 0xf]; // segment data
      }
      bTemp[j++] = b;
    }
  }
  tm1637Write(bTemp, j); // send to the display
}
