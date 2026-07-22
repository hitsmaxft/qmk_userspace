# MCU name
MCU = STM32F103
BOARD = BHEZNZ_STM32_F103

# Bootloader selection
BOOTLOADER = stm32duino

# Build Options
#   change yes to no to disable
#
#
#
# BOOTMAGIC_ENABLE = no       # Enable Bootmagic Lite
# MOUSEKEY_ENABLE = yes       # Mouse keys
# EXTRAKEY_ENABLE = yes       # Audio control and System control
CONSOLE_ENABLE = yes        # Console for debug
# COMMAND_ENABLE = yes        # Commands for debug and configuration
# NKRO_ENABLE = yes           # Enable N-Key Rollover
# BACKLIGHT_ENABLE = no       # Enable keyboard backlight functionality
# RGBLIGHT_ENABLE = no        # Enable keyboard RGB underglow
#
RGB_MATRIX = no

AUDIO_ENABLE = no           # Audio output
CUSTOM_MATRIX = lite

# project specific files

UART_DRIVER_REQUIRED = yes

##fro avr only QUANTUM_LIB_SRC += uart.c

OPT_DEFS += -DNOZNZ=1

CUSTOM_MATRIX = lite

SRC += matrix.c
