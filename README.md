
Using I2C Serial Interface Module For 1602 LCD Display

Hardware requirements:
1. STM32F030F4-DEV V1.0
- MCU: STM32F030F4P6
- HSE: 8MHz
- USART1: TX - PA2, RX - PA3
- I2C1: SCL - PA9, SDA - PA10
- LED: PA4

2. Serial LCD I2C Module - PCF8574T
- Pin assignment
	PCF8574T | LCD1602
	-------------------------
	P0         RS
	P1         RW
    P2         E
	P3         BL
	P4         D4
    P5         D5
	P6         D6
	P7         D7

3. Two 1602 LCD displays:
- Primary display(I2C address: 0x3e) will show:
	1. OS name and version: ChibiOS/RT 4.0.0
	2. System ticks

- Secondary display(I2C address: 0x3f) will show:
	1. Board name: STM32F030F4-DEV
	2. Unique Device ID (12 bytes)

Software requirements:
1. ChibiOS/RT: Commit ID: af64942

For detail info, please refer:
	https://www.brobwind.com/archives/1324
