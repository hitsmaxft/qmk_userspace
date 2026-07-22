#set working-directory:='./modules/qmk_firmware'
#set positional-arguments
set positional-arguments

default_keymap := 'macvim'

kb_ap2 := 'annepro2/c18'
kb_lily58 := 'lily58_2040/rp'
kb_gh60 := 'gh60/gh60'


default:
    just --list

list:
    bash scripts/qmk-worktree.sh qmk userspace-list


clean:
    bash scripts/qmk-worktree.sh qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 -iname "*.uf2f" -iname "*.bin" -iname "*.hex"
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

_compile_kb kb km=default_keymap *args='':
    bash scripts/qmk-worktree.sh qmk compile -kb {{kb}} -km {{km}} -j20 {{args}}


annepro2: ( _compile_kb 'annepro2/c18')
    echo "compile annepro2"

lily58: ( _compile_kb kb_lily58 'macvim')

# will clean build at beginning
gen-compile-db kb km=default_keymap:
    bash scripts/qmk-worktree.sh qmk generate-compilation-database -kb {{ kb }} -km {{ km }}

#[positional-arguments]
qmk *args='':
    @bash scripts/qmk-worktree.sh qmk $@

flash-lily58:
    @bash scripts/qmk-worktree.sh qmk flash -kb lily58_2040/rp -km macvim

flash-lily58-left:
    @bash scripts/qmk-worktree.sh qmk flash -kb lily58_2040/rp -km macvim -bl uf2-split-left

submodule-reset:
    git submodule foreach git reset --hard HEAD

submodule-update *args='':
    git submodule update --init ${@}
