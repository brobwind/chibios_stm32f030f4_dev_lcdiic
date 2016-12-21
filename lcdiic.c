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
#include "hal.h"
#include "lcdiic.h"


/**
 * Reference manual:
 * 1. Datasheet: https://www.sparkfun.com/datasheets/LCD/HD44780.pdf
 * 2. Wiki: https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller
 */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static void lcdiicWriteLocked(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t val) {
    PCF8574Driver *portdrvp = drvp->config->drvp;
    uint8_t buf[6], cnt = 0;

    drvp->port.u.dt = (val >> 4) & 0x0F;
    buf[cnt++] = drvp->port.v;

    drvp->port.u.en = 0x01;
    buf[cnt++] = drvp->port.v;

    drvp->port.u.en = 0x00;
    buf[cnt++] = drvp->port.v;

    if (mode == LCDIIC_BUS_MODE_8BIT) {
        goto done;
    }

    drvp->port.u.dt = (val >> 0) & 0x0F;
    buf[cnt++] = drvp->port.v;

    drvp->port.u.en = 0x01;
    buf[cnt++] = drvp->port.v;

    drvp->port.u.en = 0x00;
    buf[cnt++] = drvp->port.v;

done:
    pcf8574SetPort(portdrvp, 1, buf, cnt);

    // Max execution time(!Clear display & !Return home) is 37us when f(OSC) is 270kHz
    drvp->delayUs(37);
}

static msg_t lcdiicReadLocked(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t *val) {
    PCF8574Driver *portdrvp = drvp->config->drvp;
    lcdiic_port_cfg portval;
    msg_t ret;
    uint8_t buf[2];

    drvp->port.u.dt = 0x0f;
    buf[0] = drvp->port.v;
    drvp->port.u.en = 0x01;
    buf[1] = drvp->port.v;
    pcf8574SetPort(portdrvp, 1, buf, 2);

    ret = pcf8574GetPortOb(portdrvp, &portval.v);
    if (ret != MSG_OK) goto out;

    *val = portval.u.dt << 4;

    if (mode == LCDIIC_BUS_MODE_8BIT) goto done;

    drvp->port.u.en = 0x00;
    buf[0] = drvp->port.v;
    drvp->port.u.en = 0x01;
    buf[1] = drvp->port.v;
    pcf8574SetPort(portdrvp, 1, buf, 2);

    ret = pcf8574GetPortOb(portdrvp, &portval.v);
    if (ret != MSG_OK) goto out;

    *val |= portval.u.dt;

done:
    drvp->port.u.en = 0x00;
    pcf8574SetPortOb(portdrvp, 1, drvp->port.v);

    if (drvp->port.u.rs != 0x00 && drvp->port.u.rw != 0x01) {
        drvp->delayUs(37);
    }

out:
    return ret;
}

/* Write to instruction register */
static void lcdiicIrWrite(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t val) {
    chMtxLock(&drvp->mutex);
    drvp->port.u.rs = 0x00;
    drvp->port.u.rw = 0x00;
    lcdiicWriteLocked(drvp, mode, val);
    chMtxUnlock(&drvp->mutex);
}

/* Check if busy: 0 - idle, 1 - busy  */
static int8_t lcdiicCheckBusy(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t *addr) {
    uint8_t val;
    msg_t ret;

    chMtxLock(&drvp->mutex);
    drvp->port.u.rs = 0x00;
    drvp->port.u.rw = 0x01;
    ret = lcdiicReadLocked(drvp, mode, &val);
    chMtxUnlock(&drvp->mutex);

    if (addr != NULL) {
        *addr = (ret == MSG_OK) ? (val & LCD_ADDRESS_COUNTER) : 0xff;
    }

    return ret == MSG_OK ? (val & LCD_BUSY_FLAG) != 0 : 1;
}

/* Write to data register */
static void lcdiicDrWrite(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t val) {
    chMtxLock(&drvp->mutex);
    drvp->port.u.rs = 0x01;
    drvp->port.u.rw = 0x00;
    lcdiicWriteLocked(drvp, mode, val);
    chMtxUnlock(&drvp->mutex);
}

/* Read from data register */
static msg_t lcdiicDrRead(LCDIICDriver *drvp, lcdiic_bus_mode_t mode, uint8_t *val) {
    msg_t ret;

    chMtxLock(&drvp->mutex);
    drvp->port.u.rs = 0x01;
    drvp->port.u.rw = 0x01;
    ret = lcdiicReadLocked(drvp, mode, val);
    chMtxUnlock(&drvp->mutex);

    return ret;
}

static uint8_t isBusy(void *ip) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    return lcdiicCheckBusy(drvp, LCDIIC_BUS_MODE_4BIT, NULL);
}

static void setBacklight(void *ip, uint8_t on) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    PCF8574Driver *portdrvp = drvp->config->drvp;

    drvp->port.u.bl = !!on;
    pcf8574SetPortOb(portdrvp, 1, drvp->port.v);
}

static void toggleBacklight(void *ip) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;

    setBacklight(drvp, !drvp->port.u.bl);
}

static void setDisplay(void *ip, uint8_t display, uint8_t cursor, uint8_t blink) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t ctrl = 0;

    if (display)    ctrl |= LCD_DISPLAY_ON;
    if (cursor)     ctrl |= LCD_CURSOR_ON;
    if (blink)      ctrl |= LCD_CURSOR_BLINK_ON;

    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_DISPLAY_CONTROL | ctrl);
}

static void clearScreen(void *ip) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;

    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_CLEAR_DISPLAY);
    drvp->delayMs(2);
}

static void shiftContent(void *ip, uint8_t display, uint8_t right) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t ctrl = 0;
    if (display)    ctrl |= LCD_SHIFT_DISPLAY;
    if (right)      ctrl |= LCD_SHIFT_TO_RIGHT;
    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_CONTENT_SHIFT | ctrl);
}

static void returnHome(void *ip) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;

    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_RETURN_HOME);
    drvp->delayMs(2);
}

static void updatePattern(void *ip, uint8_t pos, const uint8_t *pat) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t idx;

    pos = (pos & 0x07) << 3;
    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_SET_CGRAM_ADDR | pos);

    for (idx = 0; idx < 8; idx++) {
        lcdiicDrWrite(drvp, LCDIIC_BUS_MODE_4BIT, pat[idx]);
    }
}

static void moveTo(void *ip, uint8_t row, uint8_t col) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t pos = (row * LCD_LINE_MAX_LEN + col) & LCD_DDRAM_ADDR_MASK;

    lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_SET_DDRAM_ADDR | pos);
}

static msg_t readData(void *ip, uint8_t ddram, uint8_t offset, uint8_t *val) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t pos;

    if (ddram) {
        pos =  offset & LCD_DDRAM_ADDR_MASK;
        lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_SET_DDRAM_ADDR | pos);
    } else {
        pos = offset & LCD_CGRAM_ADDR_MASK;
        lcdiicIrWrite(drvp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_SET_CGRAM_ADDR | pos);
    }

    return lcdiicDrRead(drvp, LCDIIC_BUS_MODE_4BIT, val);
}

static void addChar(void *ip, uint8_t ch) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    lcdiicDrWrite(drvp, LCDIIC_BUS_MODE_4BIT, ch);
}

static uint8_t drawText(void *ip, uint8_t row, uint8_t col, const char *text, uint8_t len) {
    LCDIICDriver *drvp = (LCDIICDriver *)ip;
    uint8_t idx;

    moveTo(drvp, row, col);

    for (idx = 0; idx < LCD_LINE_MAX_LEN && idx < len; idx++) {
        lcdiicDrWrite(drvp, LCDIIC_BUS_MODE_4BIT, text[idx]);
    }
    return idx;
}

static const struct LCDIICVMT vmt_lcdiic = {
    isBusy, setBacklight, toggleBacklight, setDisplay, clearScreen,
    shiftContent, returnHome, updatePattern, moveTo, readData,
    addChar, drawText,
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void lcdiicObjectInit(LCDIICDriver *devp, void (*delayUs)(uint32_t), void (*delayMs)(uint32_t)) {
    devp->vmt = &vmt_lcdiic;
    devp->delayUs = delayUs;
    devp->delayMs = delayMs;

    devp->port.v = 0x00;
    devp->port.u.bl = 0x01;

    devp->config = NULL;

    chMtxObjectInit(&devp->mutex);

    devp->state = LCDIIC_STOP;
}

void lcdiicStart(LCDIICDriver *devp, const LCDIICConfig *config) {
    chDbgCheck((devp != NULL) && (config != NULL));

    chDbgAssert((devp->state == LCDIIC_STOP) || (devp->state == LCDIIC_READY),
            "lcdiicStart(), invalid state");

    devp->config = config;

    /* LCD Initialize - 4-Bit Interface */

    /* 1. Wait time > 40ms */
    devp->delayMs(40);

    /* 2. Mode selection: from unknown mode to 4-bit mode */
    /* https://en.wikipedia.org/wiki/Hitachi_HD44780_LCD_controller */
    /* - State1: 8-bit mode */
    /* - State2: 4-bit mode, waiting for the first set of 4 bits */
    /* - State3: 4-bit mode, waiting for the second set of 4 bits */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_8BIT, LCD_CMD_FUNCTION_SET | LCD_BUS_MODE);
    /* Wait time > 4.1ms */
    devp->delayMs(5);

    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_8BIT, LCD_CMD_FUNCTION_SET | LCD_BUS_MODE);
    /* Wait time > 100us */
    devp->delayMs(1);

    /* The LCD is now in either state1 or state3 */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_8BIT, LCD_CMD_FUNCTION_SET | LCD_BUS_MODE);

    /* Now that the LCD is definitely in 8-bit mode, switch to 4-bit mode */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_8BIT, LCD_CMD_FUNCTION_SET);

    /* 3. Function set: number of display lines and character font */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_FUNCTION_SET |
            LCD_DISPLAY_MODE); // Set to 2-line and font size to 5x8

    /* 4. Display off */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_DISPLAY_CONTROL);

    /* 5. Display clear */
    clearScreen(devp);

    /* 6. Entry mode set */
    lcdiicIrWrite(devp, LCDIIC_BUS_MODE_4BIT, LCD_CMD_ENTRY_MODE_SET | LCD_ENTRY_MODE_INC);

    /* 7. Return home */
    returnHome(devp);

    lcdiicCheckBusy(devp, LCDIIC_BUS_MODE_4BIT, NULL);

    /* 8. Display on, cursor off, blink off */
    setDisplay(devp, 1, 0, 0);

    devp->state = LCDIIC_READY;
}

void lcdiicStop(LCDIICDriver *devp) {
    chDbgAssert((devp->state == LCDIIC_STOP) || (devp->state == LCDIIC_READY),
            "lcdiicStop(), invalid state");

    /* 1. Display off */
    setDisplay(devp, 0, 0, 0);

    /* 2. Backlight off */
    setBacklight(devp, 0);

    devp->state = LCDIIC_STOP;
}
