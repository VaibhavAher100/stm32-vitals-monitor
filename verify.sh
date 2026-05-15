#!/bin/sh
# Verify project source compiles clean under -Wall -Wextra -Werror.
# Run from repo root. Requires arm-none-eabi-gcc in PATH or CROSS_GCC set.
#
# Usage:
#   ./verify.sh
#   CROSS_GCC=/path/to/arm-none-eabi-gcc ./verify.sh

GCC="${CROSS_GCC:-arm-none-eabi-gcc}"

INCLUDES="\
  -Ifirmware/Core/Inc \
  -Ifirmware/Drivers/CMSIS/Device/ST/STM32L4xx/Include \
  -Ifirmware/Drivers/CMSIS/Include \
  -Ifirmware/Middlewares/FreeRTOS/Source/include \
  -Ifirmware/Middlewares/FreeRTOS/Source/portable/GCC/ARM_CM4F"

FLAGS="\
  -mcpu=cortex-m4 \
  -std=gnu11 \
  -DSTM32L476xx \
  -Wall -Wextra -Werror \
  -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb \
  -fsyntax-only"

SOURCES="\
  firmware/Core/Src/uart.c \
  firmware/Core/Src/i2c.c \
  firmware/Core/Src/tmp117.c \
  firmware/Core/Src/max30102.c \
  firmware/Core/Src/filter.c \
  firmware/Core/Src/bpm.c \
  firmware/Core/Src/iwdg.c \
  firmware/Core/Src/tasks_vitals.c \
  firmware/Core/Src/main.c"

FAIL=0
for f in $SOURCES; do
    if "$GCC" $FLAGS $INCLUDES "$f" 2>&1; then
        echo "PASS  $f"
    else
        echo "FAIL  $f"
        FAIL=1
    fi
done

echo ""
if [ "$FAIL" -eq 0 ]; then
    echo "All files: 0 errors, 0 warnings under -Wall -Wextra -Werror"
    exit 0
else
    echo "FAIL: warnings or errors found"
    exit 1
fi
