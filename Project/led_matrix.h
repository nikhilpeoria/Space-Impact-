#pragma once

#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "delay.h"
#include "gpio.h"

#define MATRIX_HEIGHT 64
#define MATRIX_WIDTH 64
#define MATRIX_NROWS 32

typedef enum {

  Black = 0,
  Blue = 1,
  Green = 2,
  SkyBlue = 3,
  Red = 4,
  Purple = 5,
  Yellow = 6,
  White = 7

} color_code;

gpio_s R1;
gpio_s G1;
gpio_s B1;

gpio_s R2;
gpio_s G2;
gpio_s B2;

gpio_s A;
gpio_s B;
gpio_s C;
gpio_s D;
gpio_s E;

gpio_s CLK;
gpio_s LAT;
gpio_s OE;

uint8_t matrixbuff[MATRIX_NROWS][MATRIX_HEIGHT];

void displayInit();
void display();
void displaySpaceShip(int pos, int color);
void displayEnemyShip1(int pos, int initialize_y, int color);
void displayEnemyShip2(int pos, int initialize_y, int color1, int color2);
void displayPixel(int x, int y, int color);
void KillAnimation(int start_y, int start_x);