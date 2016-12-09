#! /bin/bash
set -e -x

FW=build/ch.bin
ADDRESS=0x08000000

OCD=/usr/local/bin/openocd
#${OCD} -f interface/stlink-v2-1.cfg -f target/stm32f0x.cfg -c init -c "reset halt" -c "stm32f1x unlock 0" -c "reset halt" -c "exit"
${OCD} -f interface/stlink-v2-1.cfg -f target/stm32f0x.cfg -c init -c "reset halt" \
		-c "flash write_image erase ${FW} ${ADDRESS} bin" \
		-c "verify_image ${FW} ${ADDRESS} bin" -c "reset run" -c shutdown
