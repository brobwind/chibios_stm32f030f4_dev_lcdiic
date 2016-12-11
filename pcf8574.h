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
#ifndef __PCF8574_H__
#define __PCF8574_H__

#include "hal.h"

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

#if !HAL_USE_I2C
#error "PCF8574 requires HAL_USE_I2C"
#endif

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

typedef enum {
    /* PCF8574 slave address range: A2 | A1 | A0 */
    PCF8574_SAD_0X20 = 0x20,     //  0    0    0
    PCF8574_SAD_0X21 = 0x21,     //  0    0    1
    PCF8574_SAD_0X22 = 0x22,     //  0    1    0
    PCF8574_SAD_0X23 = 0x23,     //  0    1    1
    PCF8574_SAD_0X24 = 0x24,     //  1    0    0
    PCF8574_SAD_0X25 = 0x25,     //  1    0    1
    PCF8574_SAD_0X26 = 0x26,     //  1    1    0
    PCF8574_SAD_0X27 = 0x27,     //  1    1    1

    /* PCF8574A slave address range: A2 | A1 | A0 */
    PCF8574A_SAD_0X38 = 0x38,     //  0    0    0
    PCF8574A_SAD_0X39 = 0x39,     //  0    0    1
    PCF8574A_SAD_0X3A = 0x3A,     //  0    1    0
    PCF8574A_SAD_0X3B = 0x3B,     //  0    1    1
    PCF8574A_SAD_0X3C = 0x3C,     //  1    0    0
    PCF8574A_SAD_0X3D = 0x3D,     //  1    0    1
    PCF8574A_SAD_0X3E = 0x3E,     //  1    1    0
    PCF8574A_SAD_0X3F = 0x3F,     //  1    1    1
} pcf8574_sad_t;

typedef enum {
    PCF8574_UNINIT = 0,
    PCF8574_STOP = 1,
    PCF8574_READY = 2,
} pcf8574_state_t;

typedef struct {
    I2CDriver *i2cp;
    const I2CConfig *i2ccfg;

    pcf8574_sad_t sad;
    uint8_t mask;
    uint8_t value;
} PCF8574Config;

#define _pcf8574_methods \
    msg_t (*set_port_multi)(void *instance, uint8_t modify, uint8_t *val, uint8_t len); \
    msg_t (*get_port_multi)(void *instance, uint8_t *val, uint8_t len); \
    msg_t (*set_port_once)(void *instance, uint8_t modify, uint8_t val); \
    msg_t (*get_port_once)(void *instance, uint8_t *val); \

struct PCF8574VMT {
    _pcf8574_methods
};

#define _pcf8574_data \
    pcf8574_state_t state; \
    const PCF8574Config *config;

typedef struct PCF8574Driver {
    const struct PCF8574VMT *vmt;
    _pcf8574_data;
} PCF8574Driver;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

#define pcf8574SetPortMulti(ip, modify, val, len) \
    (ip)->vmt->set_port_multi(ip, modify, val, len)

#define pcf8574GetPortMulti(ip, val) \
    (ip)->vmt->get_port_multi(ip, val, len)

#define pcf8574SetPortOnce(ip, modify, val) \
    (ip)->vmt->set_port_once(ip, modify, val)

#define pcf8574GetPortOnce(ip, val) \
    (ip)->vmt->get_port_once(ip, val)

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

void pcf8574ObjectInit(PCF8574Driver *devp);
void pcf8574Start(PCF8574Driver *devp, const PCF8574Config *config);
void pcf8574Stop(PCF8574Driver *devp);

#ifdef __cplusplus
}
#endif

#endif /* __PCF8574_H__ */
