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
#include "uart.h"
#include <math.h>

////////////////////////////////////////////////////////////////////////////////////
// all the commands needed in the datasheet(http://geekmatic.in.ua/pdf/Catalex_MP3_board.pdf)
static int8_t Send_buf[8] = {0}; // The MP3 player undestands orders in a 8 int string
                                 // 0X7E FF 06 command 00 00 00 EF;(if command =01 next song order)
#define NEXT_SONG 0X01
#define PREV_SONG 0X02
#define CMD_PLAY_W_INDEX 0X03 // DATA IS REQUIRED (number of song)
#define VOLUME_UP_ONE 0X04
#define VOLUME_DOWN_ONE 0X05
#define CMD_SET_VOLUME 0X06 // DATA IS REQUIRED (number of volume from 0 up to 30(0x1E))
#define SET_DAC 0X17
#define CMD_PLAY_WITHVOLUME 0X22 // data is needed  0x7E 06 22 00 xx yy EF;(xx volume)(yy number of song)
#define CMD_SEL_DEV 0X09         // SELECT STORAGE DEVICE, DATA IS REQUIRED
#define DEV_TF 0X02              // HELLO,IM THE DATA REQUIRED
#define SLEEP_MODE_START 0X0A
#define SLEEP_MODE_WAKEUP 0X0B
#define CMD_RESET 0X0C // CHIP RESET
#define CMD_PLAY 0X0D  // RESUME PLAYBACK
#define CMD_PAUSE 0X0E // PLAYBACK IS PAUSED
#define CMD_PLAY_WITHFOLDER                                                                                            \
  0X0F // DATA IS NEEDED, 0x7E 06 0F 00 01 02 EF;(play the song with the directory \01\002xxxxxx.mp3
#define STOP_PLAY 0X16
#define PLAY_FOLDER 0X17   // data is needed 0x7E 06 17 00 01 XX EF;(play the 01 folder)(value xx we dont care)
#define SET_CYCLEPLAY 0X19 // data is needed 00 start; 01 close
#define SET_DAC 0X17       // data is needed 00 start DAC OUTPUT;01 DAC no output
////////////////////////////////////////////////////////////////////////////////////

acceleration__axis_data_s acc;
int prev_acc = 0;
int sp = 0;
gpio_s sw1;
int pos = 40;
int enemy_y = 63;
int enemy_x = 20;

int enemy2_y = 63;
int enemy2_x = 50;

int UFO_y = 63;
int UFO_x = 35;

int EnemyBullet1_pos_y[16];
int EnemyBullet1_pos_x[16];

int EnemyBullet2_pos_y[3];
int EnemyBullet2_pos_x[3];

int ThreePhaseBullet = 0;
int Limited_MoveMent = 0;
int ThreePhaseCounter = 0;

int NumberOfBullet = -1; // index of
int updatecount = 0;
int generate_new_bullet = 0;
int generate_new_space_ship = 0;
int StartGame = 0;

int generate_super_weapon = 0;
int SpaceShipBullet_pos_x[10];
int SpaceShipBullet_pos_y[10];
int NumberOfBulletS = -1;
int KillEnemyShip1 = 0;
int KillEnemyShip2 = 0;
int KillUFO = 0;
int TriggerAnimation[10] = {0}; // 11252019 Bullet Blinking Purpose
int KillAnimationPos[10][2];    // 11252019 Bullet Blinking Purpose
int TriggerAnimationIndex = -1;

int supertrigger = 0;
int superKillAnimationPos[2] = {0};

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
TaskHandle_t TitleScreen_handle;
TaskHandle_t LifeDisplay_handle;

void sendCommand(int8_t command, int16_t dat) {
  Send_buf[0] = 0x7e;               // starting byte
  Send_buf[1] = 0xff;               // version
  Send_buf[2] = 0x06;               // the number of bytes of the command without starting byte and ending byte
  Send_buf[3] = command;            //
  Send_buf[4] = 0x00;               // 0x00 = no feedback, 0x01 = feedback
  Send_buf[5] = (int8_t)(dat >> 8); // datah
  Send_buf[6] = (int8_t)(dat);      // datal
  Send_buf[7] = 0xef;               // ending byte
  for (uint8_t i = 0; i < 8; i++)
    uart__polled_put(UART__3, Send_buf[i]);
}

void button_interrupt(void) {
  if (((LPC_GPIOINT->IO0IntStatF >> 29) & (1 << 0)) == 1) {
    generate_new_bullet = 1;
    StartGame = 1;
    LPC_GPIOINT->IO0IntClr = (1 << 29);
  } else if (((LPC_GPIOINT->IO0IntStatR >> 30) & (1 << 0)) == 1) {
    if (NumberOfBulletS == -1) {
      generate_super_weapon = 1;
    }
    LPC_GPIOINT->IO0IntClr = (1 << 30);
  }
}

void MovingEnemyShip1(void *p) {
  while (1) {
    if (enemy_y >= 0 && enemy2_y >= 0 && UFO_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);

    } else if (enemy_y >= 0 && enemy2_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      UFO_y = 63;
      do {
        UFO_x = 13 + (rand() % 43);
      } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
    } else if (enemy_y >= 0 && UFO_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      enemy2_y = 63;
      do {
        enemy2_x = 9 + (rand() % 51);
      } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
    } else if (enemy2_y >= 0 && UFO_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      enemy_y = 63;
      do {
        enemy_x = 11 + (rand() % 47);
      } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
    } else if (enemy_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      enemy2_y = 63;
      do {
        enemy2_x = 9 + (rand() % 51);
      } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
      UFO_y = 63;
      do {
        UFO_x = 13 + (rand() % 43);
      } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
    } else if (enemy2_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      delay__ms(70);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      enemy_y = 63;
      do {
        enemy_x = 11 + (rand() % 47);
      } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
      UFO_y = 63;
      do {
        UFO_x = 13 + (rand() % 43);
      } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
    } else if (UFO_y >= 0) {
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      enemy_y = 63;
      do {
        enemy_x = 11 + (rand() % 47);
      } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
      enemy2_y = 63;
      do {
        enemy2_x = 9 + (rand() % 51);
      } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
    } else {
      UFO_y = 63;
      do {
        UFO_x = 13 + (rand() % 43);
      } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
      enemy_y = 63;
      do {
        enemy_x = 11 + (rand() % 47);
      } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
      enemy2_y = 63;
      do {
        enemy2_x = 9 + (rand() % 51);
      } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
    }
    if (KillEnemyShip1 == 0) {
      --enemy_y;
    } else {
      enemy_y = -1;
      KillEnemyShip1 = 0;
    }
    if (KillEnemyShip2 == 0) {
      // if (Limited_MoveMent == 0) {
      enemy2_y--;
      //}
    } else {
      enemy2_y = -1;
      KillEnemyShip2 = 0;
    }
    if (KillUFO == 0) {
      UFO_y--;
    } else {
      UFO_y = -1;
      KillUFO = 0;
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
  acc = acceleration__get_data();
  if (abs(acc.x - prev_acc) > 50) {
    prev_acc = acc.x;
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
    sendCommand(0x03, 0x0004);
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
    sendCommand(0x03, 0x0004);
  } else if (PixelCheckEnemy(UFO_x, UFO_y) || PixelCheckEnemy(UFO_x, UFO_y - 1) || PixelCheckEnemy(UFO_x, UFO_y - 2) ||
             PixelCheckEnemy(UFO_x, UFO_y - 3) || PixelCheckEnemy(UFO_x, UFO_y - 4) ||
             PixelCheckEnemy(UFO_x, UFO_y - 5) || PixelCheckEnemy(UFO_x, UFO_y - 6) ||
             PixelCheckEnemy(UFO_x, UFO_y - 7) || PixelCheckEnemy(UFO_x, UFO_y - 8) ||
             PixelCheckEnemy(UFO_x - 1, UFO_y - 1) || PixelCheckEnemy(UFO_x - 1, UFO_y - 2) ||
             PixelCheckEnemy(UFO_x - 1, UFO_y - 3) || PixelCheckEnemy(UFO_x - 1, UFO_y - 4) ||
             PixelCheckEnemy(UFO_x - 1, UFO_y - 5) || PixelCheckEnemy(UFO_x - 1, UFO_y - 6) ||
             PixelCheckEnemy(UFO_x - 1, UFO_y - 7) || PixelCheckEnemy(UFO_x + 1, UFO_y - 1) ||
             PixelCheckEnemy(UFO_x + 1, UFO_y - 2) || PixelCheckEnemy(UFO_x + 1, UFO_y - 3) ||
             PixelCheckEnemy(UFO_x + 1, UFO_y - 4) || PixelCheckEnemy(UFO_x + 1, UFO_y - 5) ||
             PixelCheckEnemy(UFO_x + 1, UFO_y - 6) || PixelCheckEnemy(UFO_x + 1, UFO_y - 7) ||
             PixelCheckEnemy(UFO_x + 2, UFO_y - 2) || PixelCheckEnemy(UFO_x + 2, UFO_y - 3) ||
             PixelCheckEnemy(UFO_x + 2, UFO_y - 4) || PixelCheckEnemy(UFO_x + 2, UFO_y - 5) ||
             PixelCheckEnemy(UFO_x + 2, UFO_y - 6) || PixelCheckEnemy(UFO_x - 2, UFO_y - 2) ||
             PixelCheckEnemy(UFO_x - 2, UFO_y - 3) || PixelCheckEnemy(UFO_x - 2, UFO_y - 4) ||
             PixelCheckEnemy(UFO_x - 2, UFO_y - 5) || PixelCheckEnemy(UFO_x - 2, UFO_y - 6) ||
             PixelCheckEnemy(UFO_x - 3, UFO_y - 3) || PixelCheckEnemy(UFO_x - 3, UFO_y - 4) ||
             PixelCheckEnemy(UFO_x - 3, UFO_y - 5)) {
    SpaceShipLifeCount--;
    changethecolor = 1;
    sendCommand(0x03, 0x0004);
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
      sendCommand(0x03, 0x0001);
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
      sendCommand(0x03, 0x0001);
      displayScore(ScoreCounter, 0, 1, Black);
      ScoreCounter += 10;
      displayScore(ScoreCounter, 0, 1, White);
    } else if ((SpaceShipBullet_pos_x[i] == UFO_x &&
                (SpaceShipBullet_pos_y[i] <= UFO_y && SpaceShipBullet_pos_y[i] >= UFO_y - 8)) ||
               ((SpaceShipBullet_pos_x[i] == UFO_x - 1 || SpaceShipBullet_pos_x[i] == UFO_x + 1) &&
                (SpaceShipBullet_pos_y[i] <= UFO_y - 1 && SpaceShipBullet_pos_y[i] >= UFO_y - 7)) ||
               ((SpaceShipBullet_pos_x[i] == UFO_x - 2 || SpaceShipBullet_pos_x[i] == UFO_x + 2) &&
                (SpaceShipBullet_pos_y[i] <= UFO_y - 2 && SpaceShipBullet_pos_y[i] >= UFO_y - 6)) ||
               (SpaceShipBullet_pos_x[i] == UFO_x - 3 &&
                (SpaceShipBullet_pos_y[i] <= UFO_y - 3 && SpaceShipBullet_pos_y[i] >= UFO_y - 5))) {
      if (TriggerAnimationIndex < 9) {
        TriggerAnimationIndex++;
        TriggerAnimation[TriggerAnimationIndex] = 1;
        KillAnimationPos[TriggerAnimationIndex][0] = SpaceShipBullet_pos_x[i];
        KillAnimationPos[TriggerAnimationIndex][1] = SpaceShipBullet_pos_y[i] + 2;
      }
      KillUFO = 1;
      SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
      SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
      NumberOfBulletS--;
      sendCommand(0x03, 0x0001);
      displayScore(ScoreCounter, 0, 1, Black);
      ScoreCounter += 20;
      displayScore(ScoreCounter, 0, 1, White);
    }
  }
}
void collisionDetectionSuperCanon(int centerpos, int y) {
  NumberOfBullet_Remaining = -1;
  for (int i = 0; i <= NumberOfBullet; i++) {
    if (EnemyBullet1_masking[i] == 0) {
      if ((centerpos == EnemyBullet1_pos_x[i] && y + 2 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 1 == EnemyBullet1_pos_x[i] && y + 2 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 1 == EnemyBullet1_pos_x[i] && y + 2 == EnemyBullet1_pos_y[i]) ||
          (centerpos == EnemyBullet1_pos_x[i] && y + 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 1 == EnemyBullet1_pos_x[i] && y + 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 1 == EnemyBullet1_pos_x[i] && y + 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 2 == EnemyBullet1_pos_x[i] && y + 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 2 == EnemyBullet1_pos_x[i] && y + 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos == EnemyBullet1_pos_x[i] && y == EnemyBullet1_pos_y[i]) ||
          (centerpos + 1 == EnemyBullet1_pos_x[i] && y == EnemyBullet1_pos_y[i]) ||
          (centerpos - 1 == EnemyBullet1_pos_x[i] && y == EnemyBullet1_pos_y[i]) ||
          (centerpos + 2 == EnemyBullet1_pos_x[i] && y == EnemyBullet1_pos_y[i]) ||
          (centerpos - 2 == EnemyBullet1_pos_x[i] && y == EnemyBullet1_pos_y[i]) ||
          (centerpos == EnemyBullet1_pos_x[i] && y - 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 1 == EnemyBullet1_pos_x[i] && y - 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 1 == EnemyBullet1_pos_x[i] && y - 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 2 == EnemyBullet1_pos_x[i] && y - 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 2 == EnemyBullet1_pos_x[i] && y - 1 == EnemyBullet1_pos_y[i]) ||
          (centerpos == EnemyBullet1_pos_x[i] && y - 2 == EnemyBullet1_pos_y[i]) ||
          (centerpos + 1 == EnemyBullet1_pos_x[i] && y - 2 == EnemyBullet1_pos_y[i]) ||
          (centerpos - 1 == EnemyBullet1_pos_x[i] && y - 2 == EnemyBullet1_pos_y[i])) {
        EnemyBullet1_masking[i] = 1;
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
  if (centerpos >= enemy_x - 3 && centerpos <= enemy_x + 3 && y >= enemy_y - 10 && y <= enemy_y + 2) {
    supertrigger = 1;
    superKillAnimationPos[0] = enemy_x;
    superKillAnimationPos[1] = enemy_y;
    KillEnemyShip1 = 1;
    displayScore(ScoreCounter, 0, 1, Black);
    ScoreCounter += 15;
    displayScore(ScoreCounter, 0, 1, White);
  }
  if (centerpos >= enemy2_x - 6 && centerpos <= enemy2_x + 6 && y >= enemy2_y - 8 && y <= enemy2_y + 1) {
    supertrigger = 1;
    superKillAnimationPos[0] = enemy2_x;
    superKillAnimationPos[1] = enemy2_y;
    KillEnemyShip2 = 1;
    displayScore(ScoreCounter, 0, 1, Black);
    ScoreCounter += 10;
    displayScore(ScoreCounter, 0, 1, White);
  }
  if (centerpos >= UFO_x - 6 && centerpos <= UFO_x + 5 && y >= UFO_y - 10 && y <= UFO_y + 2) {
    supertrigger = 1;
    superKillAnimationPos[0] = UFO_x;
    superKillAnimationPos[1] = UFO_y;
    KillUFO = 1;
    displayScore(ScoreCounter, 0, 1, Black);
    ScoreCounter += 25;
    displayScore(ScoreCounter, 0, 1, White);
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
        sendCommand(0x03, 0x0004);
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
    if (ThreePhaseBullet == 0) {
      if (ThreePhaseCounter < 3) {
        if (ThreePhaseCounter == 0) {
          // ThreePhaseBullet = 1;
          Limited_MoveMent = 1;
          EnemyBullet2_pos_x[0] = enemy2_x;
          EnemyBullet2_pos_y[0] = enemy2_y - 7;
        }
        displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
        displayPixel(EnemyBullet2_pos_x[0] - 1, EnemyBullet2_pos_y[0] - 1, Blue);
      } else if (ThreePhaseCounter < 5) {
        if (ThreePhaseCounter == 3) {
          EnemyBullet2_pos_x[1] = enemy2_x;
          EnemyBullet2_pos_y[1] = enemy2_y - 7;
        }
        displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
        displayPixel(EnemyBullet2_pos_x[0] - 1, EnemyBullet2_pos_y[0] - 1, Blue);
        displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Blue);
        displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Blue);
      } else {
        if (ThreePhaseCounter == 5) {
          EnemyBullet2_pos_x[2] = enemy2_x;
          EnemyBullet2_pos_y[2] = enemy2_y - 7;
        }
        if (ThreePhaseCounter == 6) {
          Limited_MoveMent = 0;
        }
        displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
        displayPixel(EnemyBullet2_pos_x[0] - 1, EnemyBullet2_pos_y[0] - 1, Blue);
        displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Blue);
        displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Blue);
        displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2], Blue);
        displayPixel(EnemyBullet2_pos_x[2] + 1, EnemyBullet2_pos_y[2] - 1, Blue);
      }
      ThreePhaseCounter++;
    }
    for (int i = 0; i <= NumberOfBullet; i++) {
      displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Red);
      displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Red);
    }
    delay__ms(30);
    for (int i = 0; i <= 15; i++) {
      displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Black);
      displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Black);
    }
    displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Black);
    displayPixel(EnemyBullet2_pos_x[0] - 1, EnemyBullet2_pos_y[0] - 1, Black);
    displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Black);
    displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Black);
    displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2], Black);
    displayPixel(EnemyBullet2_pos_x[2] + 1, EnemyBullet2_pos_y[2] - 1, Black);
    for (int i = 0; i <= NumberOfBullet; i++) {
      EnemyBullet1_pos_y[i] = EnemyBullet1_pos_y[i] - 1;
    }
    EnemyBullet2_pos_x[0] = EnemyBullet2_pos_x[0] - 1;
    EnemyBullet2_pos_y[0] = EnemyBullet2_pos_y[0] - 1;
    EnemyBullet2_pos_y[1] = EnemyBullet2_pos_y[1] - 1;
    EnemyBullet2_pos_x[2] = EnemyBullet2_pos_x[2] + 1;
    EnemyBullet2_pos_y[2] = EnemyBullet2_pos_y[2] - 1;
    if ((EnemyBullet2_pos_x[0] <= 0 || EnemyBullet2_pos_y[0] <= 0) && (EnemyBullet2_pos_y[1] <= 0) &&
        (EnemyBullet2_pos_x[2] >= 63 || EnemyBullet2_pos_y[2] <= 0)) {
      // ThreePhaseBullet = 0;
      ThreePhaseCounter = 0;
    }
    collisionDetectionBulletToBullet();
    collisionDetectionBulletToSpaceShip();
    updatecount++;
    if (NumberOfBullet == -1) {
      UpdateEnemyBullet();
    }
    if (updatecount == 40) {
      UpdateEnemyBullet();
      updatecount = 0;
    }
  }
}
void moveSpaceShip(void *p) {
  int counter = 0;
  while (1) {
    pos = (generate_super_weapon == 0) ? getSpaceshipPos() : pos;
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
      // vTaskSuspend(MovingEnemyShip2_handle);
      displayEndScreen(ScoreCounter);
      delay__ms(100);
      sendCommand(0x03, 0x0005);
      delay__ms(3000);
      sendCommand(0x08, 0x0003);
      vTaskSuspend(LifeDisplay_handle);

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
        sendCommand(0x03, 0x0002);
      }
      generate_new_bullet = 0;
    }
    if (generate_super_weapon == 1) {
      int centerpos = pos;
      SuperWeaPonPhaseOne(centerpos);
      generate_super_weapon = 0;
      for (int i = 13; i <= 66; i++) {
        SuperWeaPonPhaseTwo(centerpos, i);
        collisionDetectionSuperCanon(centerpos, i);
      }
      // generate_super_weapon = 0;
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
      sendCommand(0x16, 0x0000);
      vTaskSuspend(TitleScreen_handle);
    }
  }
}
void KillAnimationTask(void *p) {
  while (1) {
    if (supertrigger == 1) {
      KillAnimation(superKillAnimationPos[0], superKillAnimationPos[1]);
      KillAnimation(superKillAnimationPos[0] + 4, superKillAnimationPos[1] - 6);
      KillAnimation(superKillAnimationPos[0] - 2, superKillAnimationPos[1] - 3);
      supertrigger = 0;
    }
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

  uart__init(UART__3, 96 * 1000 * 1000, 9600);                       // Initialize UART3
  gpio__construct_with_function(GPIO__PORT_4, 28, GPIO__FUNCTION_2); // Enable TX3
  gpio__construct_with_function(GPIO__PORT_4, 29, GPIO__FUNCTION_2); // Enable RX3
  sendCommand(0x06, 0x0016);
  sendCommand(0x08, 0x0003);

  sw1 = gpio__construct_as_input(0, 29);
  (void)gpio__construct_as_input(0, 30);
  LPC_GPIOINT->IO0IntEnF = (1 << 29);
  LPC_GPIOINT->IO0IntEnR = (1 << 30);

  displayInit();
  acceleration__init();
  EnemyBullet1_pos_y[0] = 1;

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, button_interrupt);
  NVIC_EnableIRQ(GPIO_IRQn);

  xTaskCreate(TitleScreen, "TS", 4096U, 0, 3, &TitleScreen_handle);
  xTaskCreate(LifeDisplay, "LD", 2048U, 0, 1, &LifeDisplay_handle);
  xTaskCreate(moveSpaceShip, "MB", 2048U, 0, 1, &moveSpaceShip_handle);
  xTaskCreate(SpaceShipBullet, "SB", 2048U, 0, 1, &SpaceShipBullet_handle);
  xTaskCreate(EnemyBullet, "EB", 2048U, 0, 1, &EnemyBullet_handle);
  xTaskCreate(KillAnimationTask, "KAT", 1024U, 0, 1, 0);
  xTaskCreate(MovingEnemyShip1, "Enemyship1", 2048U, 0, 2, &MovingEnemyShip1_handle);

  xTaskCreate(refreshdisplay, "refresh display", 2048U, 0, 4, 0);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}