default:
    @just -l

clean:
    cd $QMK_HOME
    qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 -iname "*.uf2f" -iname "*.bin" -iname "*.hex"
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

annepro2:
    cd $QMK_HOME
    @qmk compile -kb annepro2/c18  -km macvim -j20

lily58:
    cd $QMK_HOME
    @qmk compile -kb bhekb/lily58_2040/rp  -km macvim -j20


flash-lily58:
    cd $QMK_HOME
    @qmk flash -kb bhekb/lily58_2040/rp  -km macvim -bl uf2-split-left
