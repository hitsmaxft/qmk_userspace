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

annepro2-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c18 -km macvim -j20

annepro2-ble213:
    ANNEPRO2_BLE_PROFILE=ap2d213 bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c18 -km macvim -j20

annepro2-ble213-log:
    ANNEPRO2_BLE_PROFILE=ap2d213 ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c18 -km macvim -j20

annepro2-c15:
    bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c15 -km default -j20

annepro2-profile-test:
    cc -std=c11 -Wall -Wextra -Werror \
        -I modules/qmk_firmware/keyboards/annepro2 \
        modules/qmk_firmware/keyboards/annepro2/annepro2_ble_profile.c \
        modules/qmk_firmware/keyboards/annepro2/tests/ble_profile_test.c \
        -o /tmp/annepro2_ble_profile_test
    /tmp/annepro2_ble_profile_test

annepro2-state-test:
    cc -std=c11 -Wall -Wextra -Werror \
        -I modules/qmk_firmware/keyboards/annepro2 \
        modules/qmk_firmware/keyboards/annepro2/annepro2_ble_state.c \
        modules/qmk_firmware/keyboards/annepro2/tests/ble_state_test.c \
        -o /tmp/annepro2_ble_state_test
    /tmp/annepro2_ble_state_test

annepro2-parser-test:
    cc -std=c11 -Wall -Wextra -Werror \
        -I modules/qmk_firmware/keyboards/annepro2 \
        modules/qmk_firmware/keyboards/annepro2/annepro2_ble_parser.c \
        modules/qmk_firmware/keyboards/annepro2/tests/ble_parser_test.c \
        -o /tmp/annepro2_ble_parser_test
    /tmp/annepro2_ble_parser_test

annepro2-led-regression:
    # AP2D removed C18's external LED MCU. Keep the C18 LED protocol and
    # driver files byte-identical to the branch base.
    base="$(git -C modules/qmk_firmware merge-base origin/master HEAD)"; \
      git -C modules/qmk_firmware diff --exit-code "$base" -- \
        keyboards/annepro2/ap2_led.c \
        keyboards/annepro2/ap2_led.h \
        keyboards/annepro2/protocol.c \
        keyboards/annepro2/protocol.h \
        keyboards/annepro2/rgb_driver.c \
        keyboards/annepro2/rgb_driver.h

annepro2-test: annepro2-profile-test annepro2-state-test annepro2-parser-test annepro2-led-regression

annepro2-validate: annepro2-test annepro2 annepro2-ble213 annepro2-c15

annepro2-recover-ap2d-data output='/tmp/ap2d-key-3.08.data.bin':
    python3 tools/reverse/annepro2/recover_ap2d_data.py \
        assets/ap2_fw/annepro2d/firmware/3.08/annepro2_discovery_KEY_APP.bin \
        --output {{ output }}

annepro2-ble-crossflash-check:
    PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
        -s tools/reverse/annepro2/tests -p 'test_*.py' -v
    PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py

annepro2-ble213-name-image output='c18-ble-2.13-annepro2c.bin':
    PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/patch_ble213_name.py \
        --output "{{ output }}"

annepro2-ble-crossflash-plan output='/tmp/annepro2-ble213-iap-plan.json':
    PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/plan_ble_iap.py \
        --output {{ output }}

annepro2-ble-crossflash-backup-check full_flash information_page:
    PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py \
        --full-flash-backup {{ full_flash }} \
        --information-page-backup {{ information_page }} \
        --require-hardware-backups

flash-annepro2-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk flash -kb annepro2/c18 -km macvim

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
