#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

#include "acceleration.h"
#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "i2c.h"
#include "led_matrix.h"
#include "lpc_peripherals.h"
#include "semphr.h"
#include "sj2_cli.h"
#include <math.h>

acceleration__axis_data_s acc;
gpio_s sw1;
int pos = 40;
int enemy_y = 63;
int enemy_x = 20;

int enemy2_y = 63;
int enemy2_x = 50;

int EnemyBullet1_pos_y[32];
int EnemyBullet1_pos_x[32];
int NumberOfBullet = -1; // index of
int updatecount = 0;
int generate_new_bullet = 0;
int generate_new_space_ship = 0;
int SpaceShipBullet_pos_x[50];
int SpaceShipBullet_pos_y[50];
int NumberOfBulletS = -1;
int KillEnemyShip1 = 0;
int KillEnemyShip2 = 0;

void button_interrupt(void) {
  generate_new_bullet = 1;
  LPC_GPIOINT->IO0IntClr = (1 << 29);
}

void MovingEnemyShip1(void *p) {
  while (1) {
    if (enemy_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      if (KillEnemyShip1 == 0) {
        --enemy_y;
      } else {
        enemy_y = -1;
        KillEnemyShip1 = 0;
      }
      delay__ms(10);
    } else {
      enemy_y = 63;
      enemy_x = rand();
      enemy_x = enemy_x % 65;

      if (enemy_x < 15) {
        enemy_x = 15;
      }
      if (enemy_x > 55) {
        enemy_x = 55;
      }
      delay__ms(10);
    }
  }
}

void MovingEnemyShip2(void *p) {
  while (1) {
    if (enemy2_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      delay__ms(70);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      if (KillEnemyShip2 == 0) {
        enemy2_y--;
      } else {
        enemy2_y = -1;
        KillEnemyShip2 = 0;
      }
      delay__ms(10);
    } else {
      enemy2_y = 63;
      enemy2_x = rand();
      enemy2_x = enemy2_x % 65;

      if (enemy_y >= 56) {
        if (enemy_x >= 32) {
          enemy2_x = enemy_x - 12;
        } else {
          enemy2_x = enemy_x + 12;
        }
      } else {
        if (enemy2_x < 15) {
          enemy2_x = 15;
        }
        if (enemy2_x > 55) {
          enemy2_x = 55;
        }
        delay__ms(10);
      }
    }
  }
}

void UpdateEnemyBullet() {
  if (NumberOfBullet == -1 || EnemyBullet1_pos_y[0] >= 0) {
    NumberOfBullet++; // in this
    int bullet_y_pos = enemy_y - 9;
    EnemyBullet1_pos_y[NumberOfBullet] = bullet_y_pos;
    EnemyBullet1_pos_x[NumberOfBullet] = enemy_x;
  } else {
    int bullet_y_pos = enemy_y - 9;
    for (int i = 0; i < NumberOfBullet; i++) {
      EnemyBullet1_pos_y[i] = EnemyBullet1_pos_y[i + 1];
      EnemyBullet1_pos_x[i] = EnemyBullet1_pos_x[i + 1];
    }
    EnemyBullet1_pos_y[NumberOfBullet] = bullet_y_pos;
    EnemyBullet1_pos_x[NumberOfBullet] = enemy_x;
  }
}
void EnemyBullet(void *p) {
  while (1) {
    if (NumberOfBullet == -1) {
      UpdateEnemyBullet();
    } else {
      for (int i = 0; i <= NumberOfBullet; i++) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], White);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, White);
      }
      delay__ms(35);
      for (int i = 0; i <= NumberOfBullet; i++) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Black);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Black);
      }
      for (int i = 0; i <= NumberOfBullet; i++) {
        EnemyBullet1_pos_y[i] = EnemyBullet1_pos_y[i] - 1;
      }
      updatecount++;
      if (updatecount == 15) {
        UpdateEnemyBullet();
        updatecount = 0;
      }
    }
    delay__ms(20);
  }
}

int getSpaceshipPos() {
  int sp = 0;
  acc = acceleration__get_data();
  if ((acc.x < 3400) && (acc.x > 2048)) {
    sp = 3;
  } else if ((acc.x >= 3400) && (acc.x <= 4096)) {
    sp = 3 + (acc.x - 3400) / 25;
  }
  if ((acc.x >= 0) && (acc.x <= 700)) {
    sp = 32 + (acc.x / 25);
  } else if ((acc.x > 700) && (acc.x < 2048)) {
    sp = 60;
  }
  return sp;
}

void moveSpaceShip(void *p) {
  while (1) {
    pos = getSpaceshipPos();
    displaySpaceShip(pos, White);
    delay__ms(30);
    displaySpaceShip(pos, Black);
  }
}

void SpaceShipBullet(void *p) {
  while (1) {
    if (generate_new_bullet == 1) {
      NumberOfBulletS++;
      SpaceShipBullet_pos_x[NumberOfBulletS] = pos;
      SpaceShipBullet_pos_y[NumberOfBulletS] = 10;
      generate_new_bullet = 0;
    }
    for (int i = 0; i <= NumberOfBulletS; i++) {
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], White);
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, White);
    }
    delay__ms(25);
    for (int i = 0; i <= NumberOfBulletS; i++) {
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], Black);
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, Black);
    }
    for (int i = 0; i <= NumberOfBulletS; i++) {
      SpaceShipBullet_pos_y[i]++;
    }
    if (SpaceShipBullet_pos_y[0] >= 63) {
      for (int i = 0; i < NumberOfBulletS; i++) {
        SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
        SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
      }
      if (NumberOfBulletS >= 0) {
        NumberOfBulletS--;
      }
    }
    for (int i = 0; i <= NumberOfBulletS; i++) {
      if (matrixbuff[SpaceShipBullet_pos_x[i]][SpaceShipBullet_pos_y[i] + 2] != 0) {
        if ((SpaceShipBullet_pos_x[i] <= enemy_x + 2) && (SpaceShipBullet_pos_x[i] >= enemy_x - 2) &&
            (((SpaceShipBullet_pos_y[i] + 1 <= enemy_y) && (SpaceShipBullet_pos_y[i] + 1 >= enemy_y - 8)) ||
             ((SpaceShipBullet_pos_y[i] + 2 <= enemy_y) && (SpaceShipBullet_pos_y[i] + 2 >= enemy_y - 8)))) {
          KillAnimation(SpaceShipBullet_pos_y[i] + 2, SpaceShipBullet_pos_x[i]);
          KillEnemyShip1 = 1;
          SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
          SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
          NumberOfBulletS--;
        } else if ((SpaceShipBullet_pos_x[i] <= enemy2_x + 4) && (SpaceShipBullet_pos_x[i] >= enemy2_x - 4) &&
                   (((SpaceShipBullet_pos_y[i] + 1 <= enemy2_y) && (SpaceShipBullet_pos_y[i] + 1 >= enemy2_y - 6)) ||
                    ((SpaceShipBullet_pos_y[i] + 2 <= enemy2_y) && (SpaceShipBullet_pos_y[i] + 2 >= enemy2_y - 6)))) {
          KillAnimation(SpaceShipBullet_pos_y[i] + 2, SpaceShipBullet_pos_x[i]);
          KillEnemyShip2 = 1;
          SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
          SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
          NumberOfBulletS--;
        }
      }
    }
    delay__ms(25);
  }
}

void refreshdisplay(void *p) {
  while (1) {
    display();
    vTaskDelay(2);
  }
}

int main(void) {
  sw1 = gpio__construct_as_input(0, 29);
  LPC_GPIOINT->IO0IntEnF = (1 << 29);
  displayInit();
  acceleration__init();
  EnemyBullet1_pos_y[0] = 1;

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, button_interrupt);
  NVIC_EnableIRQ(GPIO_IRQn);

  xTaskCreate(moveSpaceShip, "MB", 1024U, 0, 2, 0);
  xTaskCreate(SpaceShipBullet, "SB", 1024U, 0, 2, 0);
  xTaskCreate(EnemyBullet, "EB", 1024U, 0, 2, 0);
  xTaskCreate(MovingEnemyShip1, "Enemy ship", 1024U, 0, 3, 0);
  xTaskCreate(MovingEnemyShip2, "Enemy ship", 1024U, 0, 3, 0);
  xTaskCreate(refreshdisplay, "refresh display", 1024U, 0, 4, 0);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}