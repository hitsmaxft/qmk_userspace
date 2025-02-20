LEADER_ENABLE = yes

# Build Options
#   change yes to no to disable
#

# Custom RGB matrix handling
RGB_MATRIX_ENABLE = yes

QMK_SETTINGS = yes
TAP_DANCE_ENABLE = no
COMBO_ENABLE = no
KEY_OVERRIDE_ENABLE = no

OS_DETECTION_ENABLE = no

# home mod tapping
CONSOLE_ENABLE = no # Console for debug
COMMAND_ENABLE = yes # Commands for debug and configuration

SRC += os_autoconfig.c
