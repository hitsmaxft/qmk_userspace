#set working-directory:='./modules/qmk_firmware'
#set positional-arguments
set positional-arguments

default_keymap := 'macvim'

kb_ap2 := 'annepro2/c18'
kb_lily58 := 'bhekb/lily58_2040/rp'
kb_gh60 := 'gh60/gh60'
c_pwd := `pwd`


default:
    just --list

list:
    qmk userspace-list


clean:
    qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 -iname "*.uf2f" -iname "*.bin" -iname "*.hex"
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

_compile_kb kb km=default_keymap *args='':
    qmk compile -kb {{kb}}  -km {{km}} -j20

_compile_kb_extra path kb km=default_keymap *args='':
    qmk compile --compiledb --env EXTRA_KEYBOARD_FOLDER_PATH="{{c_pwd}}/{{path}}" -kb {{kb}}  -km {{km}} -j20

annepro2: ( _compile_kb 'annepro2/c18')
    echo "compile annepro2"

lily58: ( _compile_kb_extra 'modules/qmk-keyboard-lily58-2040' kb_lily58 'macvim')

# will clean build at beginning
gen-compile-db kb km=default_keymap:
    qmk generate-compilation-database -kb {{ kb }} -km {{ km }}
    cp ./modules/qmk_firmware/compile_commands.json ./

#[positional-arguments]
qmk *args='':
    @qmk $@

flash-lily58:
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim

flash-lily58-left:
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim -bl uf2-split-left

submodule-reset:
    git submodule foreach git reset --hard HEAD

submodule-update *args='':
    git submodule update --init ${@}

