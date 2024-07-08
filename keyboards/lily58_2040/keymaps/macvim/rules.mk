KEYCOMBOE_ENABLE = no
AUTO_SHIFT_ENABLE = no

MOUSEKEY_ENABLE = yes
OLED_ENABLE = yes

DEFERRED_EXEC_ENABLE = yes


PROGRAM_CMD =  pico-dfu -y $(BUILD_DIR)/$(TARGET).uf2


# If you want to change the display of OLED, you need to change here
SRC +=  ./lib/rgb_state_reader.c \
        ./lib/layer_state_reader.c \
        ./lib/logo_reader.c \
        keyloggermod.c

OPT_DEFS += -DMK_KINETIC_SPEED -DMOUSEKEY_INITIAL_SPEED=420 -DOLED_FONT_H="\"lib/glcdfont_lily.c\""

