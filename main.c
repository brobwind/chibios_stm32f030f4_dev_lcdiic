/*
 * Copyright (C) 2016 https://www.brobwind.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "pcf8574.h"
#include "lcdiic.h"


/*===========================================================================*/
/* LED blinker                                                               */
/*===========================================================================*/

static THD_WORKING_AREA(waLedBlinker, 32);
static THD_FUNCTION(LedBlinker, arg) {

  (void)arg;
  chRegSetThreadName("LedBlinker");
  while (true) {
    palClearPad(GPIOA, GPIOA_LED_GREEN);
    chThdSleepMilliseconds(500);
    palSetPad(GPIOA, GPIOA_LED_GREEN);
    chThdSleepMilliseconds(500);
  }
}

/*===========================================================================*/
/* LCD display                                                               */
/*===========================================================================*/

static const I2CConfig i2ccfg = { // I2CCLK=48MHz, SCL=~100kHz
  STM32_TIMINGR_PRESC(0x0B)  |
  STM32_TIMINGR_SCLDEL(0x04) | STM32_TIMINGR_SDADEL(0x02) |
  STM32_TIMINGR_SCLH(0x0F)   | STM32_TIMINGR_SCLL(0x13),
  0,
  0
};

/* Primary LCD display configuration */
static const PCF8574Config pcf8574cfg = {
    &I2CD1,
    &i2ccfg,
    PCF8574A_SAD_0X3E,
    0x00,
    0x00,
};

static PCF8574Driver PCF8574D1;

static const LCDIICConfig lcdiiccfg = {
    &PCF8574D1,
    &pcf8574cfg,
};

static LCDIICDriver LCDIICD1;

/* Secondary LCD display configuration */
static const PCF8574Config pcf8574cfgadv = {
    &I2CD1,
    &i2ccfg,
    PCF8574A_SAD_0X3F,
    0x00,
    0x00,
};

static PCF8574Driver PCF8574D2;

static const LCDIICConfig lcdiiccfgadv = {
    &PCF8574D2,
    &pcf8574cfgadv,
};

static LCDIICDriver LCDIICD2;

/* Delay function, used in control LCD display */
static void delayUs(uint32_t val) {
    (void)val;
}

static void delayMs(uint32_t val) {
    chThdSleepMilliseconds(val);
}

/* Primary LCD display thread */
static THD_WORKING_AREA(waLcdDisplay, 256);
static __attribute__((noreturn)) THD_FUNCTION(LcdDisplay, arg) {
  (void)arg;
  chRegSetThreadName("LcdDisplay");

  pcf8574ObjectInit(&PCF8574D1);
  pcf8574Start(&PCF8574D1, &pcf8574cfg);

  lcdiicObjectInit(&LCDIICD1, &delayUs, &delayMs);
  lcdiicStart(&LCDIICD1, &lcdiiccfg);

  {
    const uint8_t SYMBOL[][8] = {
        { 0x04, 0x0e, 0x0e, 0x0e, 0x1f, 0x00, 0x04, 0x00 }, // bell
        { 0x02, 0x03, 0x02, 0x0e, 0x1e, 0x0c, 0x00, 0x00 }, // note
        { 0x00, 0x0e, 0x15, 0x17, 0x11, 0x0e, 0x00, 0x00 }, // clock
        { 0x00, 0x0a, 0x1f, 0x1f, 0x0e, 0x04, 0x00, 0x00 }, // heart
        { 0x00, 0x0c, 0x1d, 0x0f, 0x0f, 0x06, 0x00, 0x00 }, // duck
        { 0x00, 0x01, 0x03, 0x16, 0x1c, 0x08, 0x00, 0x00 }, // check
        { 0x00, 0x1b, 0x0e, 0x04, 0x0e, 0x1b, 0x00, 0x00 }, // cross
        { 0x01, 0x01, 0x05, 0x09, 0x1f, 0x08, 0x04, 0x00 }, // retarrow
    };
    uint8_t idx;

    for (idx = 0; idx < sizeof(SYMBOL) / sizeof(SYMBOL[0]); idx++) {
        lcdiicUpdatePattern(&LCDIICD1, idx, SYMBOL[idx]);
    }

    for (idx = 0; idx < sizeof(SYMBOL) / sizeof(SYMBOL[0][0]); idx++) {
        uint8_t tmp;
        lcdiicReadData(&LCDIICD1, 0, idx, &tmp);

		if (idx % 8 == 0) {
            chprintf((BaseSequentialStream *)&SD1, "---- dump pattern: %d ----\r\n", idx / 8);
		}

        chprintf((BaseSequentialStream*)&SD1, "%02x", tmp);
        if (idx % 8 == 7) {
            chprintf((BaseSequentialStream *)&SD1, "\r\n");
        } else {
            chprintf((BaseSequentialStream *)&SD1, " ");
        }
    }
  }

  while (true) {
    char buf[32];
    systime_t millisec;

    chsnprintf(buf, sizeof(buf), "ChibiOS/RT %s", CH_KERNEL_VERSION);
    lcdiicDrawText(&LCDIICD1, 0, 0, buf, strlen(buf));

    millisec = ST2MS(chVTGetSystemTime());
    chsnprintf(buf, sizeof(buf), "%6d:%02d:%02d.%03d", millisec / 3600 / 1000,
            (millisec / 60 / 1000) % 60, (millisec / 1000) % 60, millisec % 1000);
    lcdiicDrawText(&LCDIICD1, 1, 0, buf, strlen(buf));

    chThdSleepMilliseconds(200);
  }

  lcdiicStop(&LCDIICD1);
  pcf8574Stop(&PCF8574D1);
}

/* Secondary LCD display thread */
static THD_WORKING_AREA(waLcdDisplayAdv, 256);
static __attribute__((noreturn)) THD_FUNCTION(LcdDisplayAdv, arg) {
  (void)arg;
  uint8_t next = 0, ch, idx = 0;
  chRegSetThreadName("LcdDisplayAdv");

  pcf8574ObjectInit(&PCF8574D2);
  pcf8574Start(&PCF8574D2, &pcf8574cfgadv);

  lcdiicObjectInit(&LCDIICD2, &delayUs, &delayMs);
  lcdiicStart(&LCDIICD2, &lcdiiccfgadv);

  while (true) {
    char buf[32];
    volatile uint32_t *uuid = (volatile uint32_t *)0x1FFFF7AC;

    switch (next++) {
    case 0: ch = '+'; break;
    case 1: ch = '*'; next = 0; break;
    }

    chsnprintf(buf, sizeof(buf), "%c%s", ch, BOARD_NAME);
    lcdiicDrawText(&LCDIICD2, 0, 0, buf, strlen(buf));

    chsnprintf(buf, sizeof(buf), "UID: %08x[%d]", uuid[idx], idx);
    lcdiicDrawText(&LCDIICD2, 1, 0, buf, strlen(buf));

    if (idx++ == 2) idx = 0;

    chThdSleepMilliseconds(1000);
  }

  lcdiicStop(&LCDIICD1);
  pcf8574Stop(&PCF8574D1);
}

/*
 * Application entry point.
 */
int __attribute__((noreturn)) main(void)
{
  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * I2CD1 I/O pins setup.(It bypasses board.h configurations)
   */
  palSetPadMode(GPIOA, GPIOA_PIN9, PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST);   /* SCL */
  palSetPadMode(GPIOA, GPIOA_PIN10, PAL_MODE_ALTERNATE(4) | PAL_STM32_OSPEED_HIGHEST);  /* SDA */

  /*
   * Activates the serial driver 1 using the driver default configuration.
   */
  sdStart(&SD1, NULL);

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waLedBlinker, sizeof(waLedBlinker), NORMALPRIO, LedBlinker, NULL);

  /*
   * Creates the LCD display thread.
   */
  chThdCreateStatic(waLcdDisplay, sizeof(waLcdDisplay), NORMALPRIO + 1, LcdDisplay, NULL);
  chThdCreateStatic(waLcdDisplayAdv, sizeof(waLcdDisplayAdv), NORMALPRIO + 1, LcdDisplayAdv, NULL);

  while(TRUE){
    chThdSleepMilliseconds(500);
  }
}
