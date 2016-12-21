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
#ifndef __LCDIIC_H__
#define __LCDIIC_H__

#include "hal.h"
#include "pcf8574.h"


/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

#define LCD_LINE_MAX_LEN            0x40

/**
 * =========================================================
 * DISPLAY CONTROL INSTRUCTION
 * =========================================================
 */

/**
 * 1. Clear display
 * - Execution time: 1.52ms
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   0   0   0   0  0   1
 */
#define LCD_CMD_CLEAR_DISPLAY        0x01

/**
 * 2. Return home
 * - Execution time: 1.52ms
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   0   0   0   0   1   -
 */
#define LCD_CMD_RETURN_HOME          0x02


/**
 * 3. Entry mode set
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   0   0   0   1 I/D  SH
 *                                    |    `- Shift of entire display: 1 - shift
 *                                    `- Increment/decrement of DDRAM address: 0 - dec, 1 - inc
 */
#define LCD_CMD_ENTRY_MODE_SET       0x04

#define LCD_ENTRY_MODE_SHIFT         0x01 /* Shift of entire display */
#define LCD_ENTRY_MODE_INC           0x02 /* Increment of DDRAM address */

/**
 * 4. Display on/off control:
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   0   0   1   D   C   B
 *                                |   |   `- Cursor Blink ON/OFF: 0 - off, 1 - on
 *                                |   `- Cursor ON/OFF:  0 - off, 1 - on
 *                                `- Display ON/OFF: 0 - off, 1 - on
 */
#define LCD_CMD_DISPLAY_CONTROL      0x08

#define LCD_CURSOR_BLINK_ON          0x01 /* Cursor blink on */
#define LCD_CURSOR_ON                0x02 /* Cursor on */
#define LCD_DISPLAY_ON               0x04 /* Display on */

/**
 * 5. Cursor or display shift
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   0   1 S/C R/L   -   -
 *                            0   0 - Shift cursor to the left, AC is decreased by 1
 *                            0   1 - Shift cursor to the right, AC is increased by 1
 *                            1   0 - Shift all the display to the left, cursor moves according to the display
 *                            1   1 - Shift all the display to the right, cursor moves according to the display
 */
#define LCD_CMD_CONTENT_SHIFT        0x10

#define LCD_SHIFT_TO_RIGHT           0x04 /* Shift to the right */
#define LCD_SHIFT_DISPLAY            0x08 /* Shift display */

/**
 * 6. Function set:
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   0   1  DL   N   F   -   -
 *                        |   |   `- Display font type: 0 - 5x8 dots, 1 - 5x11 dots
 *                        |   `- Display line number: 0 - 1-line, 1 - 2-line
 *                        `- Interface data length: 0 - 4-bit, 1 - 8 bit
 */
#define LCD_CMD_FUNCTION_SET         0x20

#define LCD_DISPLAY_FONT_TYPE        0x04 /* 5x11 dots format display mode */
#define LCD_DISPLAY_MODE             0x08 /* 2-line display mode */
#define LCD_BUS_MODE                 0x10 /* 8-bit bus mode */

/**
 * 7. Set CGRAM address
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     0   1 AC5 AC4 AC3 AC2 AC1 AC0
 */
#define LCD_CMD_SET_CGRAM_ADDR       0x40

#define LCD_CGRAM_ADDR_MASK          0x3f /* CGRAM address mask */

/**
 * 8. Set DDRAM address
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   0     1 AC6 AC5 AC4 AC3 AC2 AC1 AC0
 */
#define LCD_CMD_SET_DDRAM_ADDR       0x80

#define LCD_DDRAM_ADDR_MASK          0x7f /* DDRAM address mask */

/**
 * 9. Read busy flag & address
 * - Execution time: 0us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  0   1    BF AC6 AC5 AC4 AC3 AC2 AC1 AC0
 *           `- busy flag: 0 - idle, 1 - busy
 */
#define LCD_ADDRESS_COUNTER          0x7f /* Address counter mask */
#define LCD_BUSY_FLAG                0x80 /* Read busy flag */

/**
 * 10. Write data from RAM
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  1   0    D7  D6  D5  D4  D3  D2  D1  D0
 */

/**
 * 11. Read data from RAM
 * - Execution time: 38us
 * RS|R/W | DB7|DB6|DB5|DB4|DB3|DB2|DB1|DB0
 *  1   1    D7  D6  D5  D4  D3  D2  D1  D0
 */

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef enum {
    LCDIIC_UNINIT = 0,
    LCDIIC_STOP = 1,
    LCDIIC_READY = 2,
} lcdiic_state_t;

typedef enum {
    LCDIIC_BUS_MODE_4BIT = 0,
    LCDIIC_BUS_MODE_8BIT = 1,
} lcdiic_bus_mode_t;

typedef union {
    struct {
        uint8_t rs: 1;
        uint8_t rw: 1;
        uint8_t en: 1;
        uint8_t bl: 1;
        uint8_t dt: 4;
    } u;
    uint8_t v;
} lcdiic_port_cfg;

typedef struct {
    PCF8574Driver *drvp;
    const PCF8574Config *drvcfg;
} LCDIICConfig;

#define _lcdiic_methods \
    uint8_t (*isBusy)(void *instance); \
    void (*setBacklight)(void *instance, uint8_t on); \
    void (*toggleBacklight)(void *instance); \
    void (*setDisplay)(void *instance, uint8_t display, uint8_t cursor, uint8_t blink); \
    void (*clearScreen)(void *instance); \
    void (*shiftContent)(void *instance, uint8_t display, uint8_t right); \
    void (*returnHome)(void *instance); \
    void (*updatePattern)(void *instance, uint8_t pos, const uint8_t *pat); \
    void (*moveTo)(void *instance, uint8_t row, uint8_t col); \
    msg_t (*readData)(void *instance, uint8_t ddram, uint8_t offset, uint8_t *val); \
    void (*addChar)(void *instance, uint8_t ch); \
    uint8_t (*drawText)(void *instance, uint8_t row, uint8_t col, const char *text, uint8_t len);

struct LCDIICVMT {
    _lcdiic_methods
};

#define _lcdiic_data \
    lcdiic_port_cfg port; \
    lcdiic_state_t state; \
    const LCDIICConfig *config; \
    mutex_t mutex;

typedef struct LCDIICDriver {
    const struct LCDIICVMT *vmt;
    void (*delayUs)(uint32_t val);
    void (*delayMs)(uint32_t val);
    _lcdiic_data;
} LCDIICDriver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define lcdiicIsBusy(ip) \
    (ip)->vmt->isBusy(ip)

#define lcdiicSetBacklight(ip, on) \
    (ip)->vmt->setBacklight(ip, on)

#define lcdiicToggleBacklight(ip) \
    (ip)->vmt->toggleBacklight(ip)

#define lcdiicSetDisplay(ip, display, cursor, blink) \
    (ip)->vmt->setDisplay(ip, display, cursor, blink)

#define lcdiicShiftContent(ip, display, right) \
    (ip)->vmt->shiftContent(ip, display, right)

#define lcdiicClearScreen(ip) \
    (ip)->vmt->clearScreen(ip)

#define lcdiicReturnHome(ip) \
    (ip)->vmt->returnHome(ip)

#define lcdiicUpdatePattern(ip, pos, pat) \
    (ip)->vmt->updatePattern(ip, pos, pat)

#define lcdiicMoveTo(ip, row, col) \
    (ip)->vmt->moveTo(ip, row, col)

#define lcdiicReadData(ip, ddram, offset, val) \
    (ip)->vmt->readData(ip, ddram, offset, val)

#define lcdiicAddChar(ip, ch) \
    (ip)->vmt->addChar(ip, ch)

#define lcdiicDrawText(ip, row, col, text, len) \
    (ip)->vmt->drawText(ip, row, col, text, len)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void lcdiicObjectInit(LCDIICDriver *devp, void (*delayUs)(uint32_t), void (*delayMs)(uint32_t));
void lcdiicStart(LCDIICDriver *devp, const LCDIICConfig *config);
void lcdiicStop(LCDIICDriver *devp);

#ifdef __cplusplus
}
#endif

#endif /* __LCDIIC_H__ */
