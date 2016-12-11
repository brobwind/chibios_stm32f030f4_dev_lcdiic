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
#include "pcf8574.h"


/**
 * Reference manual:
 * 1. NXP: http://www.nxp.com/documents/data_sheet/PCF8574_PCF8574A.pdf
 * 2. TI: http://www.ti.com/lit/ds/symlink/pcf8574.pdf
 */

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

static msg_t pcf8574ReadRegister(I2CDriver *i2cp, pcf8574_sad_t sad, uint8_t *rxbuf, size_t n) {
    return i2cMasterReceiveTimeout(i2cp, sad, rxbuf, n, TIME_INFINITE);
}

static msg_t pcf8574WriteRegister(I2CDriver *i2cp, pcf8574_sad_t sad, uint8_t *txbuf, uint8_t n) {
    return i2cMasterTransmitTimeout(i2cp, sad, txbuf, n, NULL, 0, TIME_INFINITE);
}

static msg_t set_port_multi(void *ip, uint8_t modify, uint8_t *val, uint8_t len) {
    uint8_t ret;
    const PCF8574Driver *drv = (PCF8574Driver *)ip;

    if (modify) {
        uint8_t idx;
        for (idx = 0; idx < len; idx++)
            val[idx] = drv->config->mask | val[idx];
    }

    i2cAcquireBus(drv->config->i2cp);
    i2cStart(drv->config->i2cp, drv->config->i2ccfg);

    ret = pcf8574WriteRegister(drv->config->i2cp, drv->config->sad, val, len);

    i2cReleaseBus(drv->config->i2cp);

    return ret;
}

static msg_t get_port_multi(void *ip, uint8_t *val, uint8_t len) {
    msg_t ret;
    const PCF8574Driver *drv = (PCF8574Driver *)ip;

    i2cAcquireBus(drv->config->i2cp);
    i2cStart(drv->config->i2cp, drv->config->i2ccfg);

    ret = pcf8574ReadRegister(drv->config->i2cp, drv->config->sad, val, len);

    i2cReleaseBus(drv->config->i2cp);

    return ret;
}

static msg_t set_port_once(void *ip, uint8_t modify, uint8_t val) {
    return set_port_multi(ip, modify, &val, 1);
}

static msg_t get_port_once(void *ip, uint8_t *val) {
    return get_port_multi(ip, val, 1);
}

static const struct PCF8574VMT vmt_pcf8574 = {
    set_port_multi, get_port_multi,
    set_port_once, get_port_once
};

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

void pcf8574ObjectInit(PCF8574Driver *devp) {
    devp->vmt = &vmt_pcf8574;
    devp->config = NULL;

    devp->state = PCF8574_STOP;
}

void pcf8574Start(PCF8574Driver *devp, const PCF8574Config *config) {
    osalDbgCheck((devp != NULL) && (config != NULL));

    osalDbgAssert((devp->state == PCF8574_STOP) || (devp->state == PCF8574_READY),
            "pcf8574Start(), invalid state");

    devp->config = config;

    /* Init port */
    set_port_once(devp, 0, devp->config->mask | devp->config->value);

    devp->state = PCF8574_READY;
}

void pcf8574Stop(PCF8574Driver *devp) {
    osalDbgAssert((devp->state == PCF8574_STOP) || (devp->state == PCF8574_READY),
            "pcf8574Stop(), invalid state");

    if (devp->state == PCF8574_READY) {
        /* Set all I/O pin to input */
        set_port_once(devp, 0, 0xff);
    }

    devp->state = PCF8574_STOP;
}
