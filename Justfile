#set working-directory:='./modules/qmk_firmware'
#set positional-arguments
set positional-arguments
default:
    @just --list

list:
    qmk userspace-list


clean:
    qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 -iname "*.uf2f" -iname "*.bin" -iname "*.hex"
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

annepro2:
    @qmk compile -kb annepro2/c18  -km macvim -j20

lily58:
    qmk compile -kb bhekb/lily58_2040/rp  -km macvim -j20

#[positional-arguments]
gen-compile-db *args='':
    @qmk generate-compilation-database $@

qmk *args='':
    @qmk $@
flash-lily58:
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim -bl uf2-split-left
