
# 将过去定制的分裂 led 逻辑放到这里

ifeq ($(SPLIT_LCD_FEATURE), yes)
# lcd feature for split lily58
	OPT_DEFS += -DOLED_FONT_H="\"$(USER_PATH)/glcdfont_lily.c\""
	SRC +=  ./lib/rgb_state_reader.c \
		./lib/layer_state_reader.c \
		./lib/logo_reader.c \
		$(USER_PATH)/lcd_apm_counter.c \
		$(USER_PATH)/keyloggermod.c
endif
