#include "led_matrix.h"
#include <string.h>
void displayInit() {
  R1 = gpio__construct_as_output(0, 6);
  G1 = gpio__construct_as_output(0, 7);
  B1 = gpio__construct_as_output(0, 8);

  R2 = gpio__construct_as_output(0, 26);
  G2 = gpio__construct_as_output(0, 25);
  B2 = gpio__construct_as_output(1, 31);

  A = gpio__construct_as_output(1, 20);
  B = gpio__construct_as_output(1, 23);
  C = gpio__construct_as_output(1, 28);
  D = gpio__construct_as_output(1, 29);
  E = gpio__construct_as_output(1, 30);

  CLK = gpio__construct_as_output(2, 0);
  LAT = gpio__construct_as_output(2, 1);
  OE = gpio__construct_as_output(2, 2);

  gpio__reset(R1);
  gpio__reset(G1);
  gpio__reset(B1);

  gpio__reset(R2);
  gpio__reset(G2);
  gpio__reset(B2);

  gpio__reset(A);
  gpio__reset(B);
  gpio__reset(C);
  gpio__reset(D);
  gpio__reset(E);

  gpio__reset(CLK);
  gpio__reset(LAT);
  gpio__reset(OE);

  memset(matrixbuff, 0, MATRIX_HEIGHT * MATRIX_NROWS);
}

void displayPixel(int x, int y, int color) {
  if ((x < 0) || (x > 63))
    return;
  if ((y < 0) || (y > 63))
    return;

  if (x > 31) {
    x = x - 32;
    color = color << 3;
  }

  // matrixbuff[x][y] |= color;
  matrixbuff[x][y] = color;
}

void displaySpaceShip(int pos, int color) {
  displayPixel(pos, 0, color);
  displayPixel(pos, 1, color);
  displayPixel(pos + 1, 1, color);
  displayPixel(pos + 1, 2, color);
  displayPixel(pos + 2, 2, color);
  displayPixel(pos + 3, 0, color);
  displayPixel(pos + 3, 1, color);
  displayPixel(pos + 3, 2, color);
  displayPixel(pos + 3, 3, color);
  displayPixel(pos + 4, 2, color);
  displayPixel(pos + 5, 2, color);
  displayPixel(pos + 5, 1, color);
  displayPixel(pos + 6, 1, color);
  displayPixel(pos + 6, 0, color);
  displayPixel(pos + 1, 4, color);
  displayPixel(pos + 2, 4, color);
  displayPixel(pos + 3, 4, color);
  displayPixel(pos + 4, 4, color);
  displayPixel(pos + 5, 4, color);
  displayPixel(pos + 1, 5, color);
  displayPixel(pos + 2, 5, color);
  displayPixel(pos + 3, 5, color);
  displayPixel(pos + 4, 5, color);
  displayPixel(pos + 5, 5, color);
  displayPixel(pos + 1, 6, color);
  displayPixel(pos + 3, 6, color);
  displayPixel(pos + 5, 6, color);
  displayPixel(pos + 2, 7, color);
  displayPixel(pos + 4, 7, color);
  displayPixel(pos + 3, 8, color);
  displayPixel(pos + 3, 9, color);
}
void displayEnemyShip1(int pos, int initialize_y, int color) {
  displayPixel(pos, initialize_y, color);
  displayPixel(pos - 1, initialize_y - 1, color);
  displayPixel(pos + 1, initialize_y - 1, color);
  displayPixel(pos - 1, initialize_y - 2, color);
  displayPixel(pos + 1, initialize_y - 2, color);
  displayPixel(pos - 2, initialize_y - 2, color);
  displayPixel(pos + 2, initialize_y - 2, color);
  displayPixel(pos - 1, initialize_y - 3, color);
  displayPixel(pos + 1, initialize_y - 3, color);
  displayPixel(pos - 2, initialize_y - 3, color);
  displayPixel(pos + 2, initialize_y - 3, color);
  displayPixel(pos - 1, initialize_y - 4, color);
  displayPixel(pos + 1, initialize_y - 4, color);
  displayPixel(pos - 2, initialize_y - 4, color);
  displayPixel(pos + 2, initialize_y - 4, color);
  displayPixel(pos - 1, initialize_y - 5, color);
  displayPixel(pos + 1, initialize_y - 5, color);
  displayPixel(pos, initialize_y - 6, color);
  displayPixel(pos - 1, initialize_y - 6, color);
  displayPixel(pos + 1, initialize_y - 6, color);
  displayPixel(pos, initialize_y - 7, color);
  displayPixel(pos - 1, initialize_y - 7, color);
  displayPixel(pos + 1, initialize_y - 7, color);
  displayPixel(pos, initialize_y - 8, color);
}
void displayEnemyShip2(int pos, int initialize_y, int color1, int color2) {
  displayPixel(pos, initialize_y, color1);
  displayPixel(pos - 1, initialize_y, color2);
  displayPixel(pos + 1, initialize_y, color2);
  displayPixel(pos - 2, initialize_y, color1);
  displayPixel(pos + 2, initialize_y, color1);
  displayPixel(pos - 3, initialize_y, color2);
  displayPixel(pos + 3, initialize_y, color2);

  displayPixel(pos, initialize_y - 1, color2);
  displayPixel(pos - 1, initialize_y - 1, color1);
  displayPixel(pos + 1, initialize_y - 1, color1);
  displayPixel(pos - 2, initialize_y - 1, color2);
  displayPixel(pos + 2, initialize_y - 1, color2);
  displayPixel(pos - 3, initialize_y - 1, color1);
  displayPixel(pos + 3, initialize_y - 1, color1);
  displayPixel(pos - 4, initialize_y - 1, color2);
  displayPixel(pos + 4, initialize_y - 1, color2);

  displayPixel(pos, initialize_y - 2, color1);
  displayPixel(pos - 1, initialize_y - 2, color2);
  displayPixel(pos + 1, initialize_y - 2, color2);
  displayPixel(pos - 2, initialize_y - 2, color1);
  displayPixel(pos + 2, initialize_y - 2, color1);
  displayPixel(pos - 3, initialize_y - 2, color2);
  displayPixel(pos + 3, initialize_y - 2, color2);

  displayPixel(pos, initialize_y - 3, color2);
  displayPixel(pos - 1, initialize_y - 3, color1);
  displayPixel(pos + 1, initialize_y - 3, color1);
  displayPixel(pos - 2, initialize_y - 3, color2);
  displayPixel(pos + 2, initialize_y - 3, color2);

  displayPixel(pos, initialize_y - 4, color2);
  displayPixel(pos - 1, initialize_y - 4, color2);
  displayPixel(pos + 1, initialize_y - 4, color2);

  displayPixel(pos, initialize_y - 5, color1);
  displayPixel(pos - 1, initialize_y - 5, color2);
  displayPixel(pos + 1, initialize_y - 5, color2);

  displayPixel(pos, initialize_y - 6, color2);
}
void display() {
  for (uint8_t row = 0; row < 32; row++) {
    for (uint8_t col = 0; col < 64; col++) {
      if (matrixbuff[row][col] & 0x1) {
        gpio__set(B1);
      } else {
        gpio__reset(B1);
      }
      if (matrixbuff[row][col] & 0x2) {
        gpio__set(G1);
      } else {
        gpio__reset(G1);
      }
      if (matrixbuff[row][col] & 0x4) {
        gpio__set(R1);
      } else {
        gpio__reset(R1);
      }
      if (matrixbuff[row][col] & 0x8) {
        gpio__set(B2);
      } else {
        gpio__reset(B2);
      }
      if (matrixbuff[row][col] & 0x10) {
        gpio__set(G2);
      } else {
        gpio__reset(G2);
      }
      if (matrixbuff[row][col] & 0x20) {
        gpio__set(R2);
      } else {
        gpio__reset(R2);
      }

      gpio__set(CLK);
      gpio__reset(CLK);
    }

    gpio__set(OE);
    gpio__set(LAT);

    gpio__reset(A);
    gpio__reset(B);
    gpio__reset(C);
    gpio__reset(D);
    gpio__reset(E);

    if (row & 0x1) {
      gpio__set(A);
    }

    if (row & 0x2) {
      gpio__set(B);
    }

    if (row & 0x4) {
      gpio__set(C);
    }

    if (row & 0x8) {
      gpio__set(D);
    }

    if (row & 0x10) {
      gpio__set(E);
    }

    gpio__reset(LAT);
    gpio__reset(OE);
  }
}