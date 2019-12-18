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

int BOSS_X = 30;
int BOSS_Y = 49; // 49

int BOSS_DISPLAY = 0;
int BOSS_SHOWN = 0;
int tail_swing = 0;

int EnemyBullet1_pos_y[16];
int EnemyBullet1_pos_x[16];

int EnemyBullet2_pos_y[3];
int EnemyBullet2_pos_x[3];

int ThreePhaseBullet = 0;
int Limited_MoveMent = 0;
int ThreePhaseCounter = 0;

int updatecount = 0;
int generate_new_bullet = 0;
int generate_new_space_ship = 0;
int StartGame = 0;

int generate_super_weapon = 0;
int SpaceShipBullet_pos_x[10];
int SpaceShipBullet_pos_y[10];
int SpaceShipBullet_remaining[10] = {0}; // masking purpose

int KillEnemyShip1 = 0;
int KillEnemyShip2 = 0;
int KillUFO = 0;
int TriggerAnimation[10] = {0}; // 11252019 Bullet Blinking Purpose
int KillAnimationPos[10][2];    // 11252019 Bullet Blinking Purpose
int TriggerAnimationIndex = -1;
int ufobullet_x_pos = 65;
int ufobullet_y_pos = -1;
int dividedBulletX[2] = {65, 65};
int dividedBulletMasking[2] = {-1};

int EnemyBullet1_masking[16] = {0}; // 11172019 collision detection purpose
int EnemyBullet2_masking[3] = {0};  // 12142019 collision detection purpose
int SpaceShipLifeCount = 6;
int ScoreCounter = 0;

int changethecolor = 0;
int bulletx_initial_point = 0;

int ufocounter = 0;
int UFO_Bullet_Masking = 0;

int BOSSBULLET1X[20] = {63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63};
int BOSSBULLET1Y[20] = {63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63};
int BOSSBULLETMASKING[20] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int BOSS_LIFE = 12;
TaskHandle_t moveSpaceShip_handle;
TaskHandle_t SpaceShipBullet_handle;
TaskHandle_t EnemyBullet_handle;
TaskHandle_t MovingEnemyShip1_handle;
TaskHandle_t Boss_handle;
TaskHandle_t Boss_bullet;
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
  if (((LPC_GPIOINT->IO0IntStatF >> 17) & (1 << 0)) == 1) {
    generate_new_bullet = 1;
    StartGame = 1;
    LPC_GPIOINT->IO0IntClr = (1 << 17);
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
      if (BOSS_DISPLAY == 0) {
        UFO_y = 63;
        do {
          UFO_x = 13 + (rand() % 43);
        } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
      }
    } else if (enemy_y >= 0 && UFO_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      if (BOSS_DISPLAY == 0) {
        enemy2_y = 63;
        do {
          enemy2_x = 9 + (rand() % 51);
        } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
      }
    } else if (enemy2_y >= 0 && UFO_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      if (BOSS_DISPLAY == 0) {
        enemy_y = 63;
        do {
          enemy_x = 11 + (rand() % 47);
        } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
      }
    } else if (enemy_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      delay__ms(70);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      if (BOSS_DISPLAY == 0) {
        enemy2_y = 63;
        do {
          enemy2_x = 9 + (rand() % 51);
        } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
        UFO_y = 63;
        do {
          UFO_x = 13 + (rand() % 43);
        } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
      }
    } else if (enemy2_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      delay__ms(70);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      if (BOSS_DISPLAY == 0) {
        enemy_y = 63;
        do {
          enemy_x = 11 + (rand() % 47);
        } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
        UFO_y = 63;
        do {
          UFO_x = 13 + (rand() % 43);
        } while (abs(UFO_x - enemy_x) < 8 || abs(UFO_x - enemy2_x) < 8);
      }
    } else if (UFO_y >= 0) {
      displayUFO(UFO_x, UFO_y, Purple, Green, Blue);
      delay__ms(70);
      displayUFO(UFO_x, UFO_y, Black, Black, Black);
      if (BOSS_DISPLAY == 0) {
        enemy_y = 63;
        do {
          enemy_x = 11 + (rand() % 47);
        } while (abs(enemy_x - enemy2_x) < 8 || abs(enemy_x - UFO_x) < 8);
        enemy2_y = 63;
        do {
          enemy2_x = 9 + (rand() % 51);
        } while (abs(enemy2_x - enemy_x) < 8 || abs(enemy2_x - UFO_x) < 8);
      }
    } else {
      if (BOSS_DISPLAY == 0) {
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
      } else {
        BOSS_SHOWN = 1;
      }
      delay__ms(70);
    }
    if (KillEnemyShip1 == 0) {
      if (BOSS_DISPLAY == 0) {
        --enemy_y;
      } else {
        if (enemy_y >= 0) {
          --enemy_y;
        }
      }
    } else {
      enemy_y = -1;
      KillEnemyShip1 = 0;
    }
    if (KillEnemyShip2 == 0) {
      if (BOSS_DISPLAY == 0) {
        enemy2_y--;
      } else {
        if (enemy2_y >= 0) {
          enemy2_y--;
        }
      }
    } else {
      enemy2_y = -1;
      KillEnemyShip2 = 0;
    }
    if (KillUFO == 0) {
      if (BOSS_DISPLAY == 0) {
        UFO_y--;
      } else {
        if (UFO_y >= 0) {
          UFO_y--;
        }
      }
    } else {
      UFO_y = -1;
      KillUFO = 0;
    }
  }
}
void BOSS(void *p) {
  while (1) {
    if (BOSS_SHOWN == 0) {
      BOSS_Y = -15;
      BOSS_X = 30;
    } else {
      BOSS_Y = 49;
    }
    BossDisplay(BOSS_X, BOSS_Y, tail_swing);
    if (tail_swing < 3) {
      tail_swing++;
    } else {
      tail_swing = 0;
      int random = rand() % 8;
      if (BOSS_LIFE > 0) {
        if (BOSS_X - 12 <= 12) {
          if (random == 0 || random == 4) {
            BOSS_X++;
          } else if (random == 1 || random == 5) {
            BOSS_X = BOSS_X + 2;
          } else if (random == 2 || random == 6) {
            BOSS_X = BOSS_X + 3;
          } else {
            BOSS_X = BOSS_X + 4;
          }
        } else if (BOSS_X + 12 >= 55) {
          if (random == 0 || random == 4) {
            BOSS_X--;
          } else if (random == 1 || random == 5) {
            BOSS_X = BOSS_X - 2;
          } else if (random == 2 || random == 6) {
            BOSS_X = BOSS_X - 3;
          } else {
            BOSS_X = BOSS_X - 4;
          }
        } else {
          if (random == 0) {
            BOSS_X++;
          } else if (random == 1) {
            BOSS_X = BOSS_X + 2;
          } else if (random == 2) {
            BOSS_X = BOSS_X + 3;
          } else if (random == 3) {
            BOSS_X = BOSS_X + 4;
          } else if (random == 4) {
            BOSS_X--;
          } else if (random == 5) {
            BOSS_X = BOSS_X - 2;
          } else if (random == 6) {
            BOSS_X = BOSS_X - 3;
          } else if (random == 7) {
            BOSS_X = BOSS_X - 4;
          }
        }
      }
      for (int i = 0; i <= 19; i++) {
        if (BOSSBULLETMASKING[i] == 0) {
          if (BOSS_LIFE > 0) {
            BOSSBULLET1X[i] = BOSS_X;
            BOSSBULLET1Y[i] = BOSS_Y - 1;
            BOSSBULLETMASKING[i] = 1;
          }
          break;
        } else {
          if (BOSSBULLET1Y[i] <= 0) {
            if (BOSS_LIFE > 0) {
              BOSSBULLET1X[i] = BOSS_X;
              BOSSBULLET1Y[i] = BOSS_Y - 1;
              BOSSBULLETMASKING[i] = 1;
            } else {
              BOSSBULLETMASKING[i] = 0;
            }
            break;
          }
        }
      }
    }
  }
}
int BCD = 30;

void BOSS_COUNT_DOWN(void *p) {
  while (1) {
    if (BCD > 0) {
      BCD--;
    } else {
      if (BOSS_DISPLAY == 0) {
        BOSS_DISPLAY = 1;
      }
    }
    delay__ms(500);
  }
}
void BOSS_BATTLE(void *p) {
  while (1) {
    if (BOSS_LIFE == 0) {
      KillAnimation(BOSS_X, BOSS_Y + 1);
      delay__ms(100);
      KillAnimation(BOSS_X - 2, BOSS_Y + 4);
      delay__ms(100);
      KillAnimation(BOSS_X + 2, BOSS_Y + 4);
      delay__ms(100);
      BOSS_DISPLAY = 0;
      BOSS_SHOWN = 0;
      BCD = 30;
      BOSS_LIFE = 12;
    }
    delay__ms(30);
  }
}
void BOSS_BULLET(void *p) {
  while (1) {
    for (int i = 0; i <= 19; i++) {
      if (BOSSBULLETMASKING[i] == 1) {
        BOSSWEAPON(BOSSBULLET1X[i], BOSSBULLET1Y[i]);
      }
    }
    delay__ms(30);
    for (int i = 0; i <= 19; i++) {
      UFODiagonalOff(BOSSBULLET1X[i], BOSSBULLET1Y[i]);
    }
    for (int i = 0; i <= 19; i++) {
      BOSSBULLET1Y[i]--;
    }
  }
}
void UpdateEnemyBullet() {
  for (int i = 0; i <= 15; i++) {
    if (EnemyBullet1_masking[i] == 0) {
      int bullet_y_pos = enemy_y - 9;
      EnemyBullet1_pos_y[i] = bullet_y_pos;
      EnemyBullet1_pos_x[i] = enemy_x;
      EnemyBullet1_masking[i] = 1;
      break;
    } else {
      if (EnemyBullet1_pos_y[i] <= 0) {
        int bullet_y_pos = enemy_y - 9;
        EnemyBullet1_pos_y[i] = bullet_y_pos;
        EnemyBullet1_pos_x[i] = enemy_x;
        break;
      }
    }
  }
}

int getSpaceshipPos() {
  acc = acceleration__get_data();
  if (abs(acc.x - prev_acc) > 30) {
    prev_acc = acc.x;
    if ((acc.x < 3700) && (acc.x > 2048)) {
      sp = 10;
    } else if ((acc.x >= 3700) && (acc.x <= 4096)) {
      sp = 10 + (acc.x - 3700) / 17;
    }
    if ((acc.x >= 0) && (acc.x <= 400)) {
      sp = 35 + (acc.x / 17);
    } else if ((acc.x > 400) && (acc.x < 2048)) {
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
  for (int i = 0; i <= 3; i++) {
    if (SpaceShipBullet_remaining[i] == 1) {
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
        SpaceShipBullet_remaining[i] = 0;
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
        SpaceShipBullet_remaining[i] = 0;
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
        SpaceShipBullet_remaining[i] = 0;
        sendCommand(0x03, 0x0001);
        displayScore(ScoreCounter, 0, 1, Black);
        ScoreCounter += 20;
        displayScore(ScoreCounter, 0, 1, White);
      } else if ((SpaceShipBullet_pos_x[i] == BOSS_X &&
                  (SpaceShipBullet_pos_y[i] <= BOSS_Y + 1 && SpaceShipBullet_pos_y[i] >= BOSS_Y)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 1 || SpaceShipBullet_pos_x[i] == BOSS_X - 1) &&
                  (SpaceShipBullet_pos_y[i] <= BOSS_Y + 3 && SpaceShipBullet_pos_y[i] >= BOSS_Y + 1)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 2 || SpaceShipBullet_pos_x[i] == BOSS_X - 2) &&
                  (SpaceShipBullet_pos_y[i] <= BOSS_Y + 3 && SpaceShipBullet_pos_y[i] >= BOSS_Y + 2)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 3 || SpaceShipBullet_pos_x[i] == BOSS_X - 3) &&
                  (SpaceShipBullet_pos_y[i] <= BOSS_Y + 4 && SpaceShipBullet_pos_y[i] >= BOSS_Y + 3)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 4 || SpaceShipBullet_pos_x[i] == BOSS_X - 4) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 4)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 5 || SpaceShipBullet_pos_x[i] == BOSS_X - 5) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 5)) ||
                 ((SpaceShipBullet_pos_x[i] >= BOSS_X - 12 && SpaceShipBullet_pos_x[i] <= BOSS_X - 6) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 5)) ||
                 ((SpaceShipBullet_pos_x[i] <= BOSS_X + 12 && SpaceShipBullet_pos_x[i] >= BOSS_X + 6) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 5)) ||
                 ((SpaceShipBullet_pos_x[i] >= BOSS_X - 12 && SpaceShipBullet_pos_x[i] <= BOSS_X - 6) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 4)) ||
                 ((SpaceShipBullet_pos_x[i] <= BOSS_X + 12 && SpaceShipBullet_pos_x[i] >= BOSS_X + 6) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 4)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X - 7 || SpaceShipBullet_pos_x[i] == BOSS_X - 8) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 3 || SpaceShipBullet_pos_y[i] == BOSS_Y + 2 ||
                   SpaceShipBullet_pos_y[i] == BOSS_Y + 1)) ||
                 ((SpaceShipBullet_pos_x[i] == BOSS_X + 7 || SpaceShipBullet_pos_x[i] == BOSS_X + 8) &&
                  (SpaceShipBullet_pos_y[i] == BOSS_Y + 3 || SpaceShipBullet_pos_y[i] == BOSS_Y + 2 ||
                   SpaceShipBullet_pos_y[i] == BOSS_Y + 1))) {
        SpaceShipBullet_remaining[i] = 0;
        BOSS_LIFE--;
        sendCommand(0x03, 0x0001);
        displayScore(ScoreCounter, 0, 1, Black);
        ScoreCounter += 40;
        displayScore(ScoreCounter, 0, 1, White);
        if (TriggerAnimationIndex < 9) {
          TriggerAnimationIndex++;
          TriggerAnimation[TriggerAnimationIndex] = 1;
          KillAnimationPos[TriggerAnimationIndex][0] = SpaceShipBullet_pos_x[i];
          KillAnimationPos[TriggerAnimationIndex][1] = SpaceShipBullet_pos_y[i] + 2;
        }
      }
    }
  }
}
void collisionDetectionBulletToBullet() {
  bool isRemove = false;
  for (int i = 0; i <= 3; i++) {
    for (int j = 0; j <= 15; j++) {
      if (EnemyBullet1_masking[j] == 1 && SpaceShipBullet_remaining[i] == 1) {
        if ((SpaceShipBullet_pos_x[i] == EnemyBullet1_pos_x[j]) &&
            (SpaceShipBullet_pos_y[i] == EnemyBullet1_pos_y[j])) {
          isRemove = true;
          EnemyBullet1_masking[j] = 0;
          SpaceShipBullet_remaining[i] = 0;
          break;
        }
      }
    }

    if (isRemove == false) {
      if (EnemyBullet2_masking[0] == 0 && SpaceShipBullet_remaining[i] == 1) {
        if ((SpaceShipBullet_pos_x[i] == EnemyBullet2_pos_x[0] && SpaceShipBullet_pos_y[i] == EnemyBullet2_pos_y[0]) ||
            (SpaceShipBullet_pos_x[i] == EnemyBullet2_pos_x[0] &&
             SpaceShipBullet_pos_y[i] == EnemyBullet2_pos_y[0] - 1)) {
          isRemove = true;
          EnemyBullet2_masking[0] = 1;
          SpaceShipBullet_remaining[i] = 0;
        }
      }
    }
    if (isRemove == false) {
      if (EnemyBullet2_masking[1] == 0 && SpaceShipBullet_remaining[i] == 1) {
        if ((SpaceShipBullet_pos_x[i] == EnemyBullet2_pos_x[1] && SpaceShipBullet_pos_y[i] == EnemyBullet2_pos_y[1])) {
          isRemove = true;
          EnemyBullet2_masking[1] = 1;
          SpaceShipBullet_remaining[i] = 0;
        }
      }
    }
    if (isRemove == false) {
      if (EnemyBullet2_masking[2] == 0 && SpaceShipBullet_remaining[i] == 1) {
        if ((SpaceShipBullet_pos_x[i] == EnemyBullet2_pos_x[2] && SpaceShipBullet_pos_y[i] == EnemyBullet2_pos_y[2]) ||
            (SpaceShipBullet_pos_x[i] == EnemyBullet2_pos_x[2] &&
             SpaceShipBullet_pos_y[i] == EnemyBullet2_pos_y[2] - 1)) {
          isRemove = true;
          EnemyBullet2_masking[2] = 1;
          SpaceShipBullet_remaining[i] = 0;
        }
      }
    }
    if (isRemove == false) {
      if (UFO_Bullet_Masking == 0 && SpaceShipBullet_remaining[i] == 1) {
        if (((SpaceShipBullet_pos_x[i] == ufobullet_x_pos) && (SpaceShipBullet_pos_y[i] == ufobullet_y_pos)) ||
            ((SpaceShipBullet_pos_x[i] == ufobullet_x_pos - 1) && (SpaceShipBullet_pos_y[i] == ufobullet_y_pos)) ||
            ((SpaceShipBullet_pos_x[i] == ufobullet_x_pos + 1) && (SpaceShipBullet_pos_y[i] == ufobullet_y_pos)) ||
            ((SpaceShipBullet_pos_x[i] == ufobullet_x_pos) && (SpaceShipBullet_pos_y[i] == ufobullet_y_pos - 1)) ||
            ((SpaceShipBullet_pos_x[i] == ufobullet_x_pos) && (SpaceShipBullet_pos_y[i] == ufobullet_y_pos + 1))) {
          isRemove = true;
          UFO_Bullet_Masking = 1;
          SpaceShipBullet_remaining[i] = 0;
          dividedBulletMasking[0] = 1;
          dividedBulletMasking[1] = 1;
          dividedBulletX[0] = ufobullet_x_pos + 2;
          dividedBulletX[1] = ufobullet_x_pos - 2;
        }
      }
    }
    if (isRemove == false) {
      if (dividedBulletMasking[0] == 1 && SpaceShipBullet_remaining[i] == 1) {
        if (SpaceShipBullet_pos_x[i] == dividedBulletX[0] &&
            (SpaceShipBullet_pos_y[i] == ufobullet_y_pos || SpaceShipBullet_pos_y[i] == ufobullet_y_pos - 1)) {
          isRemove = true;
          dividedBulletMasking[0] = 0;
          SpaceShipBullet_remaining[i] = 0;
        }
      }
    }
    if (isRemove == false) {
      if (dividedBulletMasking[1] == 1 && SpaceShipBullet_remaining[i] == 1) {
        if (SpaceShipBullet_pos_x[i] == dividedBulletX[1] &&
            (SpaceShipBullet_pos_y[i] == ufobullet_y_pos || SpaceShipBullet_pos_y[i] == ufobullet_y_pos - 1)) {
          isRemove = true;
          dividedBulletMasking[1] = 0;
          SpaceShipBullet_remaining[i] = 0;
        }
      }
    }
    if (isRemove == false) {
      for (int k = 0; k <= 19; k++) {
        if (BOSSBULLETMASKING[k] == 1 && SpaceShipBullet_remaining[i] == 1) {
          if (((SpaceShipBullet_pos_x[i] == BOSSBULLET1X[k]) && (SpaceShipBullet_pos_y[i] == BOSSBULLET1Y[k])) ||
              ((SpaceShipBullet_pos_x[i] == BOSSBULLET1X[k] - 1) && (SpaceShipBullet_pos_y[i] == BOSSBULLET1Y[k])) ||
              ((SpaceShipBullet_pos_x[i] == BOSSBULLET1X[k] + 1) && (SpaceShipBullet_pos_y[i] == BOSSBULLET1Y[k])) ||
              ((SpaceShipBullet_pos_x[i] == BOSSBULLET1X[k]) && (SpaceShipBullet_pos_y[i] == BOSSBULLET1Y[k] - 1)) ||
              ((SpaceShipBullet_pos_x[i] == BOSSBULLET1X[k]) && (SpaceShipBullet_pos_y[i] == BOSSBULLET1Y[k] + 1))) {
            isRemove = true;
            BOSSBULLETMASKING[k] = 2;
            SpaceShipBullet_remaining[i] = 0;
          }
        }
      }
    }
    isRemove = false;
  }
}

void collisionDetectionBulletToSpaceShip() {
  if (BOSS_DISPLAY == 0) {
    for (int i = 0; i <= 15; i++) {
      if ((EnemyBullet1_pos_x[i] == pos && (EnemyBullet1_pos_y[i] >= 0 && EnemyBullet1_pos_y[i] <= 9)) ||
          ((EnemyBullet1_pos_x[i] == pos + 1 || EnemyBullet1_pos_x[i] == pos - 1) &&
           (EnemyBullet1_pos_y[i] >= 2 && EnemyBullet1_pos_y[i] <= 7)) ||
          ((EnemyBullet1_pos_x[i] == pos + 2 || EnemyBullet1_pos_x[i] == pos - 2) &&
           (EnemyBullet1_pos_y[i] >= 1 && EnemyBullet1_pos_y[i] <= 6)) ||
          ((EnemyBullet1_pos_x[i] == pos + 3 || EnemyBullet1_pos_x[i] == pos - 3) &&
           (EnemyBullet1_pos_y[i] >= 0 && EnemyBullet1_pos_y[i] <= 1))) {
        if (EnemyBullet1_masking[i] == 1) {
          EnemyBullet1_masking[i] = 0;
          SpaceShipLifeCount--;
          sendCommand(0x03, 0x0004);
          changethecolor = 1;
        }
      }
    }
    for (int i = 0; i <= 2; i++) {
      if (EnemyBullet2_masking[i] == 0) {
        if ((EnemyBullet2_pos_x[i] == pos && (EnemyBullet2_pos_y[i] >= 0 && EnemyBullet2_pos_y[i] <= 9)) ||
            ((EnemyBullet2_pos_x[i] == pos + 1 || EnemyBullet2_pos_x[i] == pos - 1) &&
             (EnemyBullet2_pos_y[i] >= 2 && EnemyBullet2_pos_y[i] <= 7)) ||
            ((EnemyBullet2_pos_x[i] == pos + 2 || EnemyBullet2_pos_x[i] == pos - 2) &&
             (EnemyBullet2_pos_y[i] >= 1 && EnemyBullet2_pos_y[i] <= 6)) ||
            ((EnemyBullet2_pos_x[i] == pos + 3 || EnemyBullet2_pos_x[i] == pos - 3) &&
             (EnemyBullet2_pos_y[i] >= 0 && EnemyBullet2_pos_y[i] <= 1))) {
          EnemyBullet2_masking[i] = 1;
          SpaceShipLifeCount--;
          sendCommand(0x03, 0x0004);
          changethecolor = 1;
        }
      }
    }
    if (UFO_Bullet_Masking == 0) {
      if ((ufobullet_x_pos == pos && (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 9)) ||
          ((ufobullet_x_pos == pos + 1 || ufobullet_x_pos == pos - 1) &&
           (ufobullet_y_pos >= 2 && ufobullet_y_pos <= 7)) ||
          ((ufobullet_x_pos == pos + 2 || ufobullet_x_pos == pos - 2) &&
           (ufobullet_y_pos >= 1 && ufobullet_y_pos <= 6)) ||
          ((ufobullet_x_pos == pos + 3 || ufobullet_x_pos == pos - 3) &&
           (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 1))) {
        UFO_Bullet_Masking = 1;
        SpaceShipLifeCount--;
        sendCommand(0x03, 0x0004);
        changethecolor = 1;
      }
    }
    if (dividedBulletMasking[0] == 1) {
      if ((dividedBulletX[0] == pos && (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 9)) ||
          ((dividedBulletX[0] == pos + 1 || dividedBulletX[0] == pos - 1) &&
           (ufobullet_y_pos >= 2 && ufobullet_y_pos <= 7)) ||
          ((dividedBulletX[0] == pos + 2 || dividedBulletX[0] == pos - 2) &&
           (ufobullet_y_pos >= 1 && ufobullet_y_pos <= 6)) ||
          ((dividedBulletX[0] == pos + 3 || dividedBulletX[0] == pos - 3) &&
           (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 1))) {
        dividedBulletMasking[0] = 0;
        SpaceShipLifeCount--;
        sendCommand(0x03, 0x0004);
        changethecolor = 1;
      }
    }
    if (dividedBulletMasking[1] == 1) {
      if ((dividedBulletX[1] == pos && (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 9)) ||
          ((dividedBulletX[1] == pos + 1 || dividedBulletX[1] == pos - 1) &&
           (ufobullet_y_pos >= 2 && ufobullet_y_pos <= 7)) ||
          ((dividedBulletX[1] == pos + 2 || dividedBulletX[1] == pos - 2) &&
           (ufobullet_y_pos >= 1 && ufobullet_y_pos <= 6)) ||
          ((dividedBulletX[1] == pos + 3 || dividedBulletX[1] == pos - 3) &&
           (ufobullet_y_pos >= 0 && ufobullet_y_pos <= 1))) {
        dividedBulletMasking[1] = 0;
        SpaceShipLifeCount--;
        sendCommand(0x03, 0x0004);
        changethecolor = 1;
      }
    }
  }
  for (int i = 0; i <= 19; i++) {
    if (BOSSBULLETMASKING[i] == 1) {
      if ((BOSSBULLET1X[i] == pos && (BOSSBULLET1Y[i] >= 0 && BOSSBULLET1Y[i] <= 9)) ||
          ((BOSSBULLET1X[i] == pos + 1 || BOSSBULLET1X[i] == pos - 1) &&
           (BOSSBULLET1Y[i] >= 2 && BOSSBULLET1Y[i] <= 7)) ||
          ((BOSSBULLET1X[i] == pos + 2 || BOSSBULLET1X[i] == pos - 2) &&
           (BOSSBULLET1Y[i] >= 1 && BOSSBULLET1Y[i] <= 6)) ||
          ((BOSSBULLET1X[i] == pos + 3 || BOSSBULLET1X[i] == pos - 3) &&
           (BOSSBULLET1Y[i] >= 0 && BOSSBULLET1Y[i] <= 1))) {
        BOSSBULLETMASKING[i] = 2;
        SpaceShipLifeCount--;
        sendCommand(0x03, 0x0004);
        changethecolor = 1;
      }
    }
  }
}
void EnemyBullet(void *p) {
  while (1) {
    if (ThreePhaseBullet == 0) {
      if (ThreePhaseCounter < 3) {
        if (ThreePhaseCounter == 0) {
          Limited_MoveMent = 1;
          bulletx_initial_point = enemy2_x;
          EnemyBullet2_pos_x[0] = bulletx_initial_point;
          EnemyBullet2_pos_y[0] = enemy2_y - 7;
        }
        if (EnemyBullet2_masking[0] == 0) {
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0] - 1, Blue);
        }
      } else if (ThreePhaseCounter < 5) {
        if (ThreePhaseCounter == 3) {
          EnemyBullet2_pos_x[1] = bulletx_initial_point + 3;
          EnemyBullet2_pos_y[1] = enemy2_y - 7;
        }
        if (EnemyBullet2_masking[0] == 0) {
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0] - 1, Blue);
        }
        if (EnemyBullet2_masking[1] == 0) {
          displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Blue);
          displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Blue);
        }
      } else {
        if (ThreePhaseCounter == 5) {
          EnemyBullet2_pos_x[2] = bulletx_initial_point - 3;
          EnemyBullet2_pos_y[2] = enemy2_y - 7;
        }
        if (ThreePhaseCounter == 6) {
          Limited_MoveMent = 0;
        }
        if (EnemyBullet2_masking[0] == 0) {
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Blue);
          displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0] - 1, Blue);
        }
        if (EnemyBullet2_masking[1] == 0) {
          displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Blue);
          displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Blue);
        }
        if (EnemyBullet2_masking[2] == 0) {
          displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2], Blue);
          displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2] - 1, Blue);
        }
      }
      ThreePhaseCounter++;
    }
    if (ufocounter == 0) {
      ufobullet_x_pos = UFO_x;
      ufobullet_y_pos = UFO_y - 9;
    }
    if (UFO_Bullet_Masking == 0) {
      UFODiagonalOn(ufobullet_x_pos, ufobullet_y_pos);
    }
    if (dividedBulletMasking[0] == 1) {
      displayPixel(dividedBulletX[0], ufobullet_y_pos, Yellow);
      displayPixel(dividedBulletX[0], ufobullet_y_pos - 1, Yellow);
    }
    if (dividedBulletMasking[1] == 1) {
      displayPixel(dividedBulletX[1], ufobullet_y_pos, Yellow);
      displayPixel(dividedBulletX[1], ufobullet_y_pos - 1, Yellow);
    }
    ufocounter++;
    for (int i = 0; i <= 15; i++) {
      if (EnemyBullet1_masking[i] == 1) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Red);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Red);
      }
    }
    delay__ms(30);
    for (int i = 0; i <= 15; i++) {
      if (EnemyBullet1_masking[i] == 1) {
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Black);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Black);
      }
    }
    UFODiagonalOff(ufobullet_x_pos, ufobullet_y_pos);
    displayPixel(dividedBulletX[0], ufobullet_y_pos, Black);
    displayPixel(dividedBulletX[0], ufobullet_y_pos - 1, Black);
    displayPixel(dividedBulletX[1], ufobullet_y_pos, Black);
    displayPixel(dividedBulletX[1], ufobullet_y_pos - 1, Black);
    displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0], Black);
    displayPixel(EnemyBullet2_pos_x[0], EnemyBullet2_pos_y[0] - 1, Black);
    displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1], Black);
    displayPixel(EnemyBullet2_pos_x[1], EnemyBullet2_pos_y[1] - 1, Black);
    displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2], Black);
    displayPixel(EnemyBullet2_pos_x[2], EnemyBullet2_pos_y[2] - 1, Black);
    for (int i = 0; i <= 15; i++) {
      EnemyBullet1_pos_y[i] = EnemyBullet1_pos_y[i] - 1;
    }
    EnemyBullet2_pos_y[0] = EnemyBullet2_pos_y[0] - 1;
    EnemyBullet2_pos_y[1] = EnemyBullet2_pos_y[1] - 1;
    EnemyBullet2_pos_y[2] = EnemyBullet2_pos_y[2] - 1;
    ufobullet_y_pos--;
    if (((EnemyBullet2_pos_y[0] <= 0) && (EnemyBullet2_pos_y[1] <= 0) && (EnemyBullet2_pos_y[2] <= 0)) ||
        ((EnemyBullet2_masking[0] == 1) && (EnemyBullet2_masking[1] == 1) && (EnemyBullet2_masking[2] == 1))) {
      ThreePhaseCounter = 0;
      EnemyBullet2_masking[0] = 0;
      EnemyBullet2_masking[1] = 0;
      EnemyBullet2_masking[2] = 0;
      EnemyBullet2_pos_y[0] = 0;
      EnemyBullet2_pos_y[1] = 0;
      EnemyBullet2_pos_y[2] = 0;
    }
    if (ufobullet_y_pos <= 0 || (dividedBulletMasking[0] == 0 && dividedBulletMasking[1] == 0)) {
      ufocounter = 0;
      UFO_Bullet_Masking = 0;
      ufobullet_x_pos = 65;
      ufobullet_y_pos = -1;
      dividedBulletMasking[0] = -1;
      dividedBulletMasking[1] = -1;
    }
    collisionDetectionBulletToBullet();
    collisionDetectionBulletToSpaceShip();
    updatecount++;
    if (updatecount == 25) {
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
      vTaskSuspend(Boss_handle);
      vTaskSuspend(Boss_bullet);
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
      for (int i = 0; i <= 3; i++) {
        if (SpaceShipBullet_remaining[i] == 0) {
          SpaceShipBullet_pos_x[i] = pos;
          SpaceShipBullet_pos_y[i] = 10;
          SpaceShipBullet_remaining[i] = 1;
          sendCommand(0x03, 0x0002);
          break;
        } else {
          if (SpaceShipBullet_pos_y[i] >= 63) {
            SpaceShipBullet_pos_x[i] = pos;
            SpaceShipBullet_pos_y[i] = 10;
            sendCommand(0x03, 0x0002);
            break;
          }
        }
      }
      generate_new_bullet = 0;
    }
    for (int i = 0; i <= 3; i++) {
      if (SpaceShipBullet_remaining[i] == 1) {
        displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], White);
        displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, White);
      }
    }
    delay__ms(30);
    for (int i = 0; i <= 3; i++) {
      if (SpaceShipBullet_remaining[i] == 1) {
        displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], Black);
        displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, Black);
      }
    }
    for (int i = 0; i <= 3; i++) {
      SpaceShipBullet_pos_y[i]++;
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

  sw1 = gpio__construct_as_input(0, 17); // change from 29 to 17
  LPC_GPIOINT->IO0IntEnF = (1 << 17);

  displayInit();
  acceleration__init();
  EnemyBullet1_pos_y[0] = 1;

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, button_interrupt);
  NVIC_EnableIRQ(GPIO_IRQn);

  xTaskCreate(TitleScreen, "TS", 2048U, 0, 3, &TitleScreen_handle);
  xTaskCreate(LifeDisplay, "LD", 1024U, 0, 1, &LifeDisplay_handle);
  xTaskCreate(moveSpaceShip, "MB", 1024U, 0, 1, &moveSpaceShip_handle);
  xTaskCreate(SpaceShipBullet, "SB", 1024U, 0, 1, &SpaceShipBullet_handle);
  xTaskCreate(EnemyBullet, "EB", 1024U, 0, 1, &EnemyBullet_handle);
  xTaskCreate(KillAnimationTask, "KAT", 1024U, 0, 1, 0);
  xTaskCreate(MovingEnemyShip1, "Enemyship1", 1024U, 0, 2, &MovingEnemyShip1_handle);
  xTaskCreate(BOSS, "BS", 1024U, 0, 2, &Boss_handle);
  xTaskCreate(BOSS_BULLET, "BSB", 1024U, 0, 1, &Boss_bullet);
  xTaskCreate(refreshdisplay, "refresh display", 2048U, 0, 4, 0);
  xTaskCreate(BOSS_COUNT_DOWN, "BOSS_count_down", 512U, 0, 1, 0);
  xTaskCreate(BOSS_BATTLE, "BBA", 512U, 0, 1, 0);
  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}