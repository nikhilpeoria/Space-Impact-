#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "i2c.h"
#include "led_matrix.h"
#include "lpc_peripherals.h"
#include "semphr.h"
#include "sj2_cli.h"
#include <math.h>
typedef void (*function_pointer_t)(void);
void gpio0__attach_interrupt(uint32_t pin, function_pointer_t callback);
void gpio0__interrupt_dispatcher(void);
uint32_t find_pin_that_generated_interrupt();
void clear_pin_interrupt(uint32_t);
uint32_t getpos(uint32_t n);

gpio_s sw1, sw2;
int pos = 48;
int enemy_y = 63;
int enemy_x = 20;

int enemy2_y = 63;
int enemy2_x = 50;

int EnemyBullet1_pos_y[32];
int EnemyBullet1_pos_x[32];
int NumberOfBullet = -1; // index of
int updatecount = 0;
// SemaphoreHandle_t switch_pressed_signal;
int generate_new_bullet = 0;
int generate_new_space_ship = 0;
int SpaceShipBullet_pos_x[50];
int SpaceShipBullet_pos_y[50];
int NumberOfBulletS = -1;
static function_pointer_t gpio0_falling_callbacks[32];
int KillEnemyShip1 = 0;
int KillEnemyShip2 = 0;
// static volatile uint8_t slave_memory[256];

/*bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory_value) {
  *memory_value = slave_memory[memory_index];
  return true;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  slave_memory[memory_index] = memory_value;
  return true;
}*/

void gpio0__attach_interrupt(uint32_t pin, function_pointer_t callback) {
  gpio0_falling_callbacks[pin] = callback;
  LPC_GPIOINT->IO0IntEnF |= (1 << pin);
}

void gpio0__interrupt_dispatcher(void) {

  function_pointer_t attached_user_handler_falling;
  const uint32_t pin = find_pin_that_generated_interrupt();
  if (LPC_GPIOINT->IO0IntStatF)
    attached_user_handler_falling = gpio0_falling_callbacks[pin];

  attached_user_handler_falling();
  clear_pin_interrupt(pin);
}

uint32_t getpos(uint32_t n) {
  uint32_t i;
  uint32_t temp = 1;
  for (i = 0; i <= 31; i++) {
    temp = (1 << i);
    if (n & temp)
      break;
  }
  return i;
}

uint32_t find_pin_that_generated_interrupt() {
  uint32_t pin = 0;
  pin = getpos(LPC_GPIOINT->IO0IntStatF);
  return pin;
}

void clear_pin_interrupt(uint32_t pin) { LPC_GPIOINT->IO0IntClr |= (1 << pin); }

void pin30_isr(void) {
  /*displaySpaceShip(pos, Black);
  displaySpaceShip(++pos, Red);*/
  generate_new_bullet = 1;
}
void pin29_isr(void) {
  /*displaySpaceShip(pos, Black);
  displaySpaceShip(--pos, Red);*/
  generate_new_space_ship = 1;
}
void Boom(int center_y, int space_ship_x) {
  int start_y = center_y;
  int start_x = space_ship_x;
  displayPixel(start_x, start_y, Yellow);
  delay__ms(14);
  displayPixel(start_x - 1, start_y, Yellow);
  displayPixel(start_x + 1, start_y, Yellow);
  displayPixel(start_x, start_y - 1, Yellow);
  displayPixel(start_x, start_y + 1, Yellow);
  displayPixel(start_x - 1, start_y - 1, Yellow);
  displayPixel(start_x - 1, start_y + 1, Yellow);
  displayPixel(start_x + 1, start_y - 1, Yellow);
  displayPixel(start_x + 1, start_y + 1, Yellow);
  delay__ms(14);
  displayPixel(start_x - 2, start_y, Yellow);
  displayPixel(start_x + 2, start_y, Yellow);
  displayPixel(start_x, start_y - 2, Yellow);
  displayPixel(start_x, start_y + 2, Yellow);
  displayPixel(start_x - 1, start_y - 2, Yellow);
  displayPixel(start_x + 1, start_y - 2, Yellow);
  displayPixel(start_x - 1, start_y + 2, Yellow);
  displayPixel(start_x + 1, start_y + 2, Yellow);
  displayPixel(start_x - 2, start_y - 1, Yellow);
  displayPixel(start_x - 2, start_y + 1, Yellow);
  displayPixel(start_x + 2, start_y - 1, Yellow);
  displayPixel(start_x + 2, start_y + 1, Yellow);
  displayPixel(start_x - 2, start_y - 2, Yellow);
  displayPixel(start_x - 2, start_y + 2, Yellow);
  displayPixel(start_x + 2, start_y - 2, Yellow);
  displayPixel(start_x + 2, start_y + 2, Yellow);
  delay__ms(14);
  displayPixel(start_x - 3, start_y, Yellow);
  displayPixel(start_x + 3, start_y, Yellow);
  displayPixel(start_x, start_y - 3, Yellow);
  displayPixel(start_x, start_y + 3, Yellow);
  displayPixel(start_x - 3, start_y - 1, Yellow);
  displayPixel(start_x - 3, start_y - 2, Yellow);
  displayPixel(start_x - 3, start_y + 1, Yellow);
  displayPixel(start_x - 3, start_y + 2, Yellow);
  displayPixel(start_x + 3, start_y - 1, Yellow);
  displayPixel(start_x + 3, start_y - 2, Yellow);
  displayPixel(start_x + 3, start_y + 1, Yellow);
  displayPixel(start_x + 3, start_y + 2, Yellow);
  displayPixel(start_x - 1, start_y - 3, Yellow);
  displayPixel(start_x - 2, start_y - 3, Yellow);
  displayPixel(start_x + 1, start_y - 3, Yellow);
  displayPixel(start_x + 2, start_y - 3, Yellow);
  displayPixel(start_x - 1, start_y + 3, Yellow);
  displayPixel(start_x - 2, start_y + 3, Yellow);
  displayPixel(start_x + 1, start_y + 3, Yellow);
  displayPixel(start_x + 2, start_y + 3, Yellow);
  delay__ms(14);
  displayPixel(start_x, start_y, Black);
  delay__ms(14);
  displayPixel(start_x - 1, start_y, Black);
  displayPixel(start_x + 1, start_y, Black);
  displayPixel(start_x, start_y - 1, Black);
  displayPixel(start_x, start_y + 1, Black);
  displayPixel(start_x - 1, start_y - 1, Black);
  displayPixel(start_x - 1, start_y + 1, Black);
  displayPixel(start_x + 1, start_y - 1, Black);
  displayPixel(start_x + 1, start_y + 1, Black);
  delay__ms(14);
  displayPixel(start_x - 2, start_y, Black);
  displayPixel(start_x + 2, start_y, Black);
  displayPixel(start_x, start_y - 2, Black);
  displayPixel(start_x, start_y + 2, Black);
  displayPixel(start_x - 1, start_y - 2, Black);
  displayPixel(start_x + 1, start_y - 2, Black);
  displayPixel(start_x - 1, start_y + 2, Black);
  displayPixel(start_x + 1, start_y + 2, Black);
  displayPixel(start_x - 2, start_y - 1, Black);
  displayPixel(start_x - 2, start_y + 1, Black);
  displayPixel(start_x + 2, start_y - 1, Black);
  displayPixel(start_x + 2, start_y + 1, Black);
  displayPixel(start_x - 2, start_y - 2, Black);
  displayPixel(start_x - 2, start_y + 2, Black);
  displayPixel(start_x + 2, start_y - 2, Black);
  displayPixel(start_x + 2, start_y + 2, Black);
  delay__ms(14);
  displayPixel(start_x - 3, start_y, Black);
  displayPixel(start_x + 3, start_y, Black);
  displayPixel(start_x, start_y - 3, Black);
  displayPixel(start_x, start_y + 3, Black);
  displayPixel(start_x - 3, start_y - 1, Black);
  displayPixel(start_x - 3, start_y - 2, Black);
  displayPixel(start_x - 3, start_y + 1, Black);
  displayPixel(start_x - 3, start_y + 2, Black);
  displayPixel(start_x + 3, start_y - 1, Black);
  displayPixel(start_x + 3, start_y - 2, Black);
  displayPixel(start_x + 3, start_y + 1, Black);
  displayPixel(start_x + 3, start_y + 2, Black);
  displayPixel(start_x - 1, start_y - 3, Black);
  displayPixel(start_x - 2, start_y - 3, Black);
  displayPixel(start_x + 1, start_y - 3, Black);
  displayPixel(start_x + 2, start_y - 3, Black);
  displayPixel(start_x - 1, start_y + 3, Black);
  displayPixel(start_x - 2, start_y + 3, Black);
  displayPixel(start_x + 1, start_y + 3, Black);
  displayPixel(start_x + 2, start_y + 3, Black);
  delay__ms(14);
}
void MovingEnemyShip(void *p) {
  while (1) {
    if (enemy_y >= 0) {
      displayEnemyShip1(enemy_x, enemy_y, Purple);
      delay__ms(95);
      displayEnemyShip1(enemy_x, enemy_y, Black);
      if (KillEnemyShip1 == 0) {
        enemy_y--;
      } else {
        enemy_y = -2;
      }
      delay__ms(95);
    } else {
      KillEnemyShip1 = 0;
      enemy_y = 63;
      enemy_x = rand();
      enemy_x = enemy_x % 65;
      if (enemy_x < 15) {
        enemy_x = 15;
      }
      if (enemy_x > 55) {
        enemy_x = 55;
      }
      delay__ms(95);
    }
  }
}

void MovingEnemyShip2(void *p) {
  while (1) {
    if (enemy2_y >= 0) {
      displayEnemyShip2(enemy2_x, enemy2_y, SkyBlue, Red);
      delay__ms(75);
      displayEnemyShip2(enemy2_x, enemy2_y, Black, Black);
      if (KillEnemyShip2 == 0) {
        enemy2_y--;
      } else {
        enemy2_y = -2;
      }
      delay__ms(75);
    } else {
      KillEnemyShip2 = 0;
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
      }
      delay__ms(75);
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
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i], Blue);
        displayPixel(EnemyBullet1_pos_x[i], EnemyBullet1_pos_y[i] - 1, Blue);
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
    delay__ms(35);
  }
}
void moveSpaceShip(void *p) {
  while (1) {
    if (generate_new_space_ship == 1) {
      displaySpaceShip(pos, Black);
      displaySpaceShip(--pos, Red);
      generate_new_space_ship = 0;
    } else {
      displaySpaceShip(pos, Red);
    }
    delay__ms(55);
  }
}
void SpaceShipBullet(void *p) {
  while (1) {
    if (generate_new_bullet == 1) {
      if (NumberOfBulletS == -1) {
        NumberOfBulletS++;
        SpaceShipBullet_pos_x[NumberOfBulletS] = pos + 3;
        SpaceShipBullet_pos_y[NumberOfBulletS] = 10;
      } else {
        if (SpaceShipBullet_pos_y[0] >= 63) {
          for (int i = 0; i < NumberOfBulletS; i++) {
            SpaceShipBullet_pos_x[i] = SpaceShipBullet_pos_x[i + 1];
            SpaceShipBullet_pos_y[i] = SpaceShipBullet_pos_y[i + 1];
          }
          SpaceShipBullet_pos_x[NumberOfBulletS] = pos + 3;
          SpaceShipBullet_pos_y[NumberOfBulletS] = 10;
          // x = pos+3;, y = 10
        } else {
          NumberOfBulletS++;
          SpaceShipBullet_pos_x[NumberOfBulletS] = pos + 3;
          SpaceShipBullet_pos_y[NumberOfBulletS] = 10;
        }
      }
      generate_new_bullet = 0;
    }
    if (NumberOfBulletS >= 0 && (matrixbuff[SpaceShipBullet_pos_x[0]][SpaceShipBullet_pos_y[0] + 1] != 0 ||
                                 matrixbuff[SpaceShipBullet_pos_x[0]][SpaceShipBullet_pos_y[0] + 2] != 0)) {
      if ((SpaceShipBullet_pos_x[0] <= enemy_x + 2) && (SpaceShipBullet_pos_x[0] >= enemy_x - 2) &&
          (((SpaceShipBullet_pos_y[0] + 1 <= enemy_y) && (SpaceShipBullet_pos_y[0] + 1 >= enemy_y - 8)) ||
           ((SpaceShipBullet_pos_y[0] + 2 <= enemy_y) && (SpaceShipBullet_pos_y[0] + 2 >= enemy_y - 8)))) {
        Boom(SpaceShipBullet_pos_y[0] + 2, SpaceShipBullet_pos_x[0]);
        KillEnemyShip1 = 1;
      } else if ((SpaceShipBullet_pos_x[0] <= enemy2_x + 4) && (SpaceShipBullet_pos_x[0] >= enemy2_x - 4) &&
                 (((SpaceShipBullet_pos_y[0] + 1 <= enemy2_y) && (SpaceShipBullet_pos_y[0] + 1 >= enemy2_y - 6)) ||
                  ((SpaceShipBullet_pos_y[0] + 2 <= enemy2_y) && (SpaceShipBullet_pos_y[0] + 2 >= enemy2_y - 6)))) {
        Boom(SpaceShipBullet_pos_y[0] + 2, SpaceShipBullet_pos_x[0]);
        KillEnemyShip2 = 1;
      }
    }
    for (int i = 0; i <= NumberOfBulletS; i++) {
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i], Blue);
      displayPixel(SpaceShipBullet_pos_x[i], SpaceShipBullet_pos_y[i] + 1, Blue);
    }
    delay__ms(35);
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
    delay__ms(35);
  }
}
// void spaceship(void *p) {
//   while (1) {
//     if (xSemaphoreTake(switch_pressed_signal, portMAX_DELAY)) {
//       displaySpaceShip(++pos, 4);
//     }
//   }
// }

void refreshdisplay(void *p) {
  while (1) {
    display();
    delay__ms(1);
  }
}

int main(void) {
  // i2c2__slave_init(0x86);
  // switch_pressed_signal = xSemaphoreCreateBinary();
  sw1 = gpio__construct_as_input(0, 29);
  sw2 = gpio__construct_as_input(0, 30);
  displayInit();
  displaySpaceShip(pos, Red);
  EnemyBullet1_pos_y[0] = 1;
  gpio0__attach_interrupt(30, pin30_isr);
  gpio0__attach_interrupt(29, pin29_isr);

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher);
  NVIC_EnableIRQ(GPIO_IRQn);
  xTaskCreate(moveSpaceShip, "MB", 1024U, 0, 2, 0);
  xTaskCreate(SpaceShipBullet, "SB", 1024U, 0, 2, 0);
  xTaskCreate(EnemyBullet, "EB", 1024U, 0, 2, 0);
  xTaskCreate(MovingEnemyShip, "Enemy ship", 1024U, 0, 3, 0);
  xTaskCreate(MovingEnemyShip2, "Enemy ship", 1024U, 0, 3, 0);
  xTaskCreate(refreshdisplay, "refresh display", 1024U, 0, 4, 0);

  puts("Starting RTOS");
  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}