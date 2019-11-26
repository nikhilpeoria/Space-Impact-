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

int EnemyBullet1_pos_y[16];
int EnemyBullet1_pos_x[16];
int NumberOfBullet = -1; // index of
int updatecount = 0;
int generate_new_bullet = 0;
int generate_new_space_ship = 0;
int StartGame = 0;

int SpaceShipBullet_pos_x[10];
int SpaceShipBullet_pos_y[10];
int NumberOfBulletS = -1;
int KillEnemyShip1 = 0;
int KillEnemyShip2 = 0;

int TriggerAnimation[10] = {0}; // 11252019 Bullet Blinking Purpose
int KillAnimationPos[10][2];    // 11252019 Bullet Blinking Purpose
int TriggerAnimationIndex = -1;

int NumberOfBulletS_Remaining = -1;  // 11172019 collision detection purpose
int SpaceShipBullet_remaining_x[10]; // 11172019 collision detection purpose
int SpaceShipBullet_remaining_y[10]; // 11172019 collision detection purpose

int NumberOfBullet_Remaining = -1;
int EnemyBullet1_masking[16] = {0}; // 11172019 collision detection purpose

int SpaceShipLifeCount = 6;
int ScoreCounter = 0;

int changethecolor = 0;
TaskHandle_t moveSpaceShip_handle;
TaskHandle_t SpaceShipBullet_handle;
TaskHandle_t EnemyBullet_handle;
TaskHandle_t MovingEnemyShip1_handle;
TaskHandle_t MovingEnemyShip2_handle;
TaskHandle_t TitleScreen_handle;

void button_interrupt(void) {
  generate_new_bullet = 1;
  StartGame = 1;
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
    } else {
      enemy_y = 63;
      do {
        enemy_x = 9 + (rand() % 51);
      } while (abs(enemy2_x - enemy_x) < 8);
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
    } else {
      enemy2_y = 63;
      do {
        enemy2_x = 11 + (rand() % 47);
      } while (abs(enemy2_x - enemy_x) < 8);
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

int getSpaceshipPos() {
  int sp = 0;
  acc = acceleration__get_data();
  if ((acc.x < 3400) && (acc.x > 2048)) {
    sp = 10;
  } else if ((acc.x >= 3400) && (acc.x <= 4096)) {
    sp = 10 + (acc.x - 3400) / 29;
  }
  if ((acc.x >= 0) && (acc.x <= 700)) {
    sp = 35 + (acc.x / 29);
  } else if ((acc.x > 700) && (acc.x < 2048)) {
    sp = 59;
  }
  return sp;
}

bool PixelCheckEnemy(int Enemyx, int Enemyy) {
  if (Enemyx == pos && (Enemyy >= 0 && Enemyy <= 9)) {
    return true;
  }
  if ((Enemyx == pos + 1 || Enemyx == pos - 1) && (Enemyy >= 2 && Enemyy <= 7)) {
    return true;
  }
  if ((Enemyx == pos + 2 || Enemyx == pos - 2) && (Enemyy >= 1 && Enemyy <= 6)) {
    return true;
  }
  if ((Enemyx == pos + 3 || Enemyx == pos - 3) && (Enemyy >= 0 && Enemyy <= 1)) {
    return true;
  }
  return false;
}

void collisionDetectionEnemyShipToSpaceShip() {
  if (PixelCheckEnemy(enemy_x, enemy_y) || PixelCheckEnemy(enemy_x, enemy_y - 1) ||
      PixelCheckEnemy(enemy_x, enemy_y - 2) || PixelCheckEnemy(enemy_x, enemy_y - 3) ||
      PixelCheckEnemy(enemy_x, enemy_y - 4) || PixelCheckEnemy(enemy_x, enemy_y - 5) ||
      PixelCheckEnemy(enemy_x, enemy_y - 6) || PixelCheckEnemy(enemy_x, enemy_y - 7) ||
      PixelCheckEnemy(enemy_x, enemy_y - 8) || PixelCheckEnemy(enemy_x + 1, enemy_y - 1) ||
      PixelCheckEnemy(enemy_x + 1, enemy_y - 2) || PixelCheckEnemy(enemy_x + 1, enemy_y - 3) ||
      PixelCheckEnemy(enemy_x + 1, enemy_y - 4) || PixelCheckEnemy(enemy_x + 1, enemy_y - 5) ||
      PixelCheckEnemy(enemy_x + 1, enemy_y - 6) || PixelCheckEnemy(enemy_x + 1, enemy_y - 7) ||
      PixelCheckEnemy(enemy_x - 1, enemy_y - 1) || PixelCheckEnemy(enemy_x - 1, enemy_y - 2) ||
      PixelCheckEnemy(enemy_x - 1, enemy_y - 3) || PixelCheckEnemy(enemy_x - 1, enemy_y - 4) ||
      PixelCheckEnemy(enemy_x - 1, enemy_y - 5) || PixelCheckEnemy(enemy_x - 1, enemy_y - 6) ||
      PixelCheckEnemy(enemy_x - 1, enemy_y - 7) || PixelCheckEnemy(enemy_x + 2, enemy_y - 2) ||
      PixelCheckEnemy(enemy_x + 2, enemy_y - 3) || PixelCheckEnemy(enemy_x + 2, enemy_y - 4) ||
      PixelCheckEnemy(enemy_x - 2, enemy_y - 2) || PixelCheckEnemy(enemy_x - 2, enemy_y - 3) ||
      PixelCheckEnemy(enemy_x - 2, enemy_y - 4)) {
    SpaceShipLifeCount--;
    changethecolor = 1;
  } else if (PixelCheckEnemy(enemy2_x, enemy2_y) || PixelCheckEnemy(enemy2_x, enemy2_y - 1) ||
             PixelCheckEnemy(enemy2_x, enemy2_y - 2) || PixelCheckEnemy(enemy2_x, enemy2_y - 3) ||
             PixelCheckEnemy(enemy2_x, enemy2_y - 4) || PixelCheckEnemy(enemy2_x, enemy2_y - 5) ||
             PixelCheckEnemy(enemy2_x, enemy2_y - 6) || PixelCheckEnemy(enemy2_x + 1, enemy2_y) ||
             PixelCheckEnemy(enemy2_x + 1, enemy2_y - 1) || PixelCheckEnemy(enemy2_x + 1, enemy2_y - 2) ||
             PixelCheckEnemy(enemy2_x + 1, enemy2_y - 3) || PixelCheckEnemy(enemy2_x + 1, enemy2_y - 4) ||
             PixelCheckEnemy(enemy2_x + 1, enemy2_y - 5) || PixelCheckEnemy(enemy2_x - 1, enemy2_y) ||
             PixelCheckEnemy(enemy2_x - 1, enemy2_y - 1) || PixelCheckEnemy(enemy2_x - 1, enemy2_y - 2) ||
             PixelCheckEnemy(enemy2_x - 1, enemy2_y - 3) || PixelCheckEnemy(enemy2_x - 1, enemy2_y - 4) ||
             PixelCheckEnemy(enemy2_x - 1, enemy2_y - 5) || PixelCheckEnemy(enemy2_x + 2, enemy2_y) ||
             PixelCheckEnemy(enemy2_x + 2, enemy2_y - 1) || PixelCheckEnemy(enemy2_x + 2, enemy2_y - 2) ||
             PixelCheckEnemy(enemy2_x + 2, enemy2_y - 3) || PixelCheckEnemy(enemy2_x - 2, enemy2_y) ||
             PixelCheckEnemy(enemy2_x - 2, enemy2_y - 1) || PixelCheckEnemy(enemy2_x - 2, enemy2_y - 2) ||
             PixelCheckEnemy(enemy2_x - 2, enemy2_y - 3) || PixelCheckEnemy(enemy2_x + 3, enemy2_y) ||
             PixelCheckEnemy(enemy2_x + 3, enemy2_y - 1) || PixelCheckEnemy(enemy2_x + 3, enemy2_y - 2) ||
             PixelCheckEnemy(enemy2_x - 3, enemy2_y) || PixelCheckEnemy(enemy2_x - 3, enemy2_y - 1) ||
             PixelCheckEnemy(enemy2_x - 3, enemy2_y - 2) || PixelCheckEnemy(enemy2_x + 4, enemy2_y - 1) ||
             PixelCheckEnemy(enemy2_x - 4, enemy2_y - 1)) {
    SpaceShipLifeCount--;
    changethecolor = 1;
  }
}

void collisionDetectionBulletToEnemy() {
  for (int i = 0; i <= NumberOfBulletS; i++) {
    if ((SpaceShipBullet_pos_x[i] == enemy_x && SpaceShipBullet_pos_y[i] <= enemy_y &&
         SpaceShipBullet_pos_y[i] >= enemy_y - 8) ||
        ((SpaceShipBullet_pos_x[i] == enemy_x + 1 || SpaceShipBullet_pos_x[i] == enemy_x - 1) &&
         (SpaceShipBullet_pos_y[i] <= enemy_y - 1 && SpaceShipBullet_pos_y[i] >= enemy_y - 7)) ||
        ((SpaceShipBullet_pos_x[i] == enemy_x + 2 || SpaceShipBullet_pos_x[i] == enemy_x - 2) &&
         (SpaceShipBullet_pos_y[i] <= enemy_y - 2 && SpaceShipBullet_pos_y[i] >= enemy_y - 4))) {
      if (TriggerAnimationIndex < 9) {
        TriggerAnimationIndex++;
        TriggerAnimation[TriggerAnimationIndex] = 1;
        KillAnimationPos[TriggerAnimationIndex][0] = SpaceShipBullet_pos_x[i];
        KillAnimationPos[TriggerAnimationIndex][1] = SpaceShipBullet_pos_y[i] + 2;
      }
      KillEnemyShip1 = 1;
      SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
      SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
      NumberOfBulletS--;
      displayScore(ScoreCounter, 0, 1, Black);
      ScoreCounter += 15;
      displayScore(ScoreCounter, 0, 1, White);
    } else if ((SpaceShipBullet_pos_x[i] == enemy2_x && SpaceShipBullet_pos_y[i] <= enemy2_y &&
                SpaceShipBullet_pos_y[i] >= enemy2_y - 6) ||
               ((SpaceShipBullet_pos_x[i] == enemy2_x + 1 || SpaceShipBullet_pos_x[i] == enemy2_x - 1) &&
                (SpaceShipBullet_pos_y[i] <= enemy2_y && SpaceShipBullet_pos_y[i] >= enemy2_y - 5)) ||
               ((SpaceShipBullet_pos_x[i] == enemy2_x + 2 || SpaceShipBullet_pos_x[i] == enemy2_x - 2) &&
                (SpaceShipBullet_pos_y[i] <= enemy2_y && SpaceShipBullet_pos_y[i] >= enemy2_y - 3)) ||
               ((SpaceShipBullet_pos_x[i] == enemy2_x + 3 || SpaceShipBullet_pos_x[i] == enemy2_x - 3) &&
                (SpaceShipBullet_pos_y[i] <= enemy2_y && SpaceShipBullet_pos_y[i] >= enemy2_y - 2)) ||
               ((SpaceShipBullet_pos_x[i] == enemy2_x + 4 || SpaceShipBullet_pos_x[i] == enemy2_x - 4) &&
                (SpaceShipBullet_pos_y[i] == enemy2_y - 1))) {
      if (TriggerAnimationIndex < 9) {
        TriggerAnimationIndex++;
        TriggerAnimation[TriggerAnimationIndex] = 1;
        KillAnimationPos[TriggerAnimationIndex][0] = SpaceShipBullet_pos_x[i];
        KillAnimationPos[TriggerAnimationIndex][1] = SpaceShipBullet_pos_y[i] + 2;
      }
      KillEnemyShip2 = 1;
      SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
      SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
      NumberOfBulletS--;
      displayScore(ScoreCounter, 0, 1, Black);
      ScoreCounter += 10;
      displayScore(ScoreCounter, 0, 1, White);
    }
  }
}
void collisionDetectionBulletToBullet() {
  NumberOfBulletS_Remaining = -1;
  NumberOfBullet_Remaining = -1;
  bool isRemove = false;
  for (int i = 0; i <= NumberOfBulletS; i++) {
    for (int j = 0; j <= NumberOfBullet; j++) { // Binary Search maybe ??
      if (EnemyBullet1_masking[j] == 0) {
        if ((SpaceShipBullet_pos_x[i] == EnemyBullet1_pos_x[j]) &&
            (SpaceShipBullet_pos_y[i] == EnemyBullet1_pos_y[j])) {
          isRemove = true;
          EnemyBullet1_masking[j] = 1;
          break;
        }
      }
    }
    if (isRemove == false) {
      NumberOfBulletS_Remaining++;
      SpaceShipBullet_remaining_x[NumberOfBulletS_Remaining] = SpaceShipBullet_pos_x[i];
      SpaceShipBullet_remaining_y[NumberOfBulletS_Remaining] = SpaceShipBullet_pos_y[i];
    }
    isRemove = false;
  }
  for (int i = 0; i <= NumberOfBulletS_Remaining; i++) {
    SpaceShipBullet_pos_x[i] = SpaceShipBullet_remaining_x[i];
    SpaceShipBullet_pos_y[i] = SpaceShipBullet_remaining_y[i];
    SpaceShipBullet_remaining_x[i] = 0;
    SpaceShipBullet_remaining_y[i] = 0;
  }
  NumberOfBulletS = NumberOfBulletS_Remaining;
  for (int i = 0; i <= NumberOfBullet; i++) {
    if (EnemyBullet1_masking[i] == 0) {
      NumberOfBullet_Remaining++;
      EnemyBullet1_pos_x[NumberOfBullet_Remaining] = EnemyBullet1_pos_x[i];
      EnemyBullet1_pos_y[NumberOfBullet_Remaining] = EnemyBullet1_pos_y[i];
    } else {
      EnemyBullet1_masking[i] = 0;
    }
  }
  NumberOfBullet = NumberOfBullet_Remaining;
}

void collisionDetectionBulletToSpaceShip() {
  NumberOfBullet_Remaining = -1;
  for (int i = 0; i <= NumberOfBullet; i++) {
    if ((EnemyBullet1_pos_x[i] == pos && (EnemyBullet1_pos_y[i] >= 0 && EnemyBullet1_pos_y[i] <= 9)) ||
        ((EnemyBullet1_pos_x[i] == pos + 1 || EnemyBullet1_pos_x[i] == pos - 1) &&
         (EnemyBullet1_pos_y[i] >= 2 && EnemyBullet1_pos_y[i] <= 7)) ||
        ((EnemyBullet1_pos_x[i] == pos + 2 || EnemyBullet1_pos_x[i] == pos - 2) &&
         (EnemyBullet1_pos_y[i] >= 1 && EnemyBullet1_pos_y[i] <= 6)) ||
        ((EnemyBullet1_pos_x[i] == pos + 3 || EnemyBullet1_pos_x[i] == pos - 3) &&
         (EnemyBullet1_pos_y[i] >= 0 && EnemyBullet1_pos_y[i] <= 1))) {
      if (EnemyBullet1_masking[i] == 0) {
        EnemyBullet1_masking[i] = 1;
        SpaceShipLifeCount--;
        changethecolor = 1;
      }
    }
  }
  for (int i = 0; i <= NumberOfBullet; i++) {
    if (EnemyBullet1_masking[i] == 0) {
      NumberOfBullet_Remaining++;
      EnemyBullet1_pos_x[NumberOfBullet_Remaining] = EnemyBullet1_pos_x[i];
      EnemyBullet1_pos_y[NumberOfBullet_Remaining] = EnemyBullet1_pos_y[i];
    } else {
      EnemyBullet1_masking[i] = 0;
    }
  }
  NumberOfBullet = NumberOfBullet_Remaining;
}
void EnemyBullet(void *p) {
  while (1) {
    if (NumberOfBullet == -1) {
      UpdateEnemyBullet();
    } else {
      for (int i = 0; i <= NumberOfBullet; i++) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Red);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Red);
      }
      delay__ms(30);
      for (int i = 0; i <= NumberOfBullet; i++) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Black);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Black);
      }
      for (int i = 0; i <= NumberOfBullet; i++) {
        EnemyBullet1_pos_y[i] = EnemyBullet1_pos_y[i] - 1;
      }
      collisionDetectionBulletToBullet();
      collisionDetectionBulletToSpaceShip();
      updatecount++;
      if (updatecount == 40) {
        UpdateEnemyBullet();
        updatecount = 0;
      }
    }
  }
}
void moveSpaceShip(void *p) {
  int counter = 0;
  while (1) {
    pos = getSpaceshipPos();
    if (changethecolor == 0) {
      displaySpaceShip(pos, SkyBlue);
      collisionDetectionEnemyShipToSpaceShip();
    } else {
      if (counter <= 5) {
        displaySpaceShip(pos, Red);
        counter++;
      } else {
        changethecolor = 0;
        counter = 0;
      }
    }
    delay__ms(30);
    displaySpaceShip(pos, Black);
  }
}

void LifeDisplay(void *p) {
  while (1) {
    if (SpaceShipLifeCount == 0) {
      vTaskSuspend(moveSpaceShip_handle);
      vTaskSuspend(SpaceShipBullet_handle);
      vTaskSuspend(EnemyBullet_handle);
      vTaskSuspend(MovingEnemyShip1_handle);
      vTaskSuspend(MovingEnemyShip2_handle);
      displayEndScreen(ScoreCounter);
    } else
      HealthMeter(SpaceShipLifeCount);
    delay__ms(30);
  }
}

void SpaceShipBullet(void *p) {
  while (1) {
    if (generate_new_bullet == 1) {
      if (NumberOfBulletS < 3) {
        NumberOfBulletS++;
        SpaceShipBullet_pos_x[NumberOfBulletS] = pos;
        SpaceShipBullet_pos_y[NumberOfBulletS] = 10;
      }
      generate_new_bullet = 0;
    }
    for (int i = 0; i <= NumberOfBulletS; i++) {
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], White);
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, White);
    }
    delay__ms(30);
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
    collisionDetectionBulletToBullet();
    collisionDetectionBulletToEnemy();
  }
}

void refreshdisplay(void *p) {
  while (1) {
    display();
    vTaskDelay(3);
  }
}

void TitleScreen(void *p) {
  while (1) {
    StartScreen();
    if (StartGame == 1) {
      ClearDisplay();
      displayInitial();
      vTaskSuspend(TitleScreen_handle);
    }
  }
}
void KillAnimationTask(void *p) {
  while (1) {
    for (int i = 9; i >= 0; i--) {
      if (TriggerAnimation[i] == 1) {
        TriggerAnimation[i] = 0;
        KillAnimation(KillAnimationPos[i][0], KillAnimationPos[i][1]);
        TriggerAnimationIndex--;
      }
    }
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

  xTaskCreate(TitleScreen, "TS", 4096U, 0, 3, &TitleScreen_handle);
  xTaskCreate(LifeDisplay, "LD", 2048U, 0, 1, 0);
  xTaskCreate(moveSpaceShip, "MB", 2048U, 0, 1, &moveSpaceShip_handle);
  xTaskCreate(SpaceShipBullet, "SB", 2048U, 0, 1, &SpaceShipBullet_handle);
  xTaskCreate(EnemyBullet, "EB", 2048U, 0, 1, &EnemyBullet_handle);
  xTaskCreate(KillAnimationTask, "KAT", 1024U, 0, 1, 0);
  xTaskCreate(MovingEnemyShip1, "Enemyship1", 2048U, 0, 2, &MovingEnemyShip1_handle);
  xTaskCreate(MovingEnemyShip2, "Enemyship2", 2048U, 0, 2, &MovingEnemyShip2_handle);

  xTaskCreate(refreshdisplay, "refresh display", 2048U, 0, 4, 0);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}