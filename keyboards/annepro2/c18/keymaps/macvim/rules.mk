# MCU

# Bootloader selection
BOOTLOADER = custom
PROGRAM_CMD = annepro2_tools --boot $(BUILD_DIR)/$(TARGET).bin

LEADER_ENABLE = yes

# Build Options
#   change yes to no to disable
#
BOOTMAGIC_ENABLE = no # Enable Bootmagic Lite
MOUSEKEY_ENABLE = yes # Mouse keys
EXTRAKEY_ENABLE = yes # Audio control and System control
NKRO_ENABLE = no            # Enable N-Key Rollover

OS_DETECTION_ENABLE = yes

# Custom RGB matrix handling
RGB_MATRIX_ENABLE = yes
OPT_DEFS += -DANNEPRO2_BLE_STATUS_INDICATOR_ENABLE

QMK_SETTINGS = yes
TAP_DANCE_ENABLE = no
COMBO_ENABLE = no
KEY_OVERRIDE_ENABLE = no

TRI_LAYER_ENABLE = yes

# custom DEBOUNCE
DEBOUNCE_TYPE = asym_eager_defer_pk



# home mod tapping
CONSOLE_ENABLE = no

ifeq ($(ANNEPRO2_BLE_DEBUG),yes)
    CONSOLE_ENABLE = yes
    OPT_DEFS += -DANNEPRO2_BLE_DEBUG
endif

ifeq ($(ANNEPRO2_BLE_PROFILE),ap2d213)
    OPT_DEFS += -DANNEPRO2_BLE_PROFILE=ANNEPRO2_BLE_PROFILE_AP2D_213
endif
COMMAND_ENABLE = no # Commands for debug and configuration

SRC += custom.c os_autoconfig.c
