# MCU

# Bootloader selection
BOOTLOADER = custom
PROGRAM_CMD = annepro2_tools --boot $(BUILD_DIR)/$(TARGET).bin

# Build Options
#   change yes to no to disable
#
BOOTMAGIC_ENABLE = no # Enable Bootmagic Lite
MOUSEKEY_ENABLE = yes # Mouse keys
EXTRAKEY_ENABLE = no # Audio control and System control
CONSOLE_ENABLE = no # Console for debug
COMMAND_ENABLE = no # Commands for debug and configuration
NKRO_ENABLE = no            # Enable N-Key Rollover

OS_DETECTION_ENABLE = yes

# Custom RGB matrix handling
RGB_MATRIX_ENABLE = yes

QMK_SETTINGS = yes
TAP_DANCE_ENABLE = no
COMBO_ENABLE = no
KEY_OVERRIDE_ENABLE = no
