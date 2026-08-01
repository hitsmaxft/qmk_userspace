#set working-directory:='./modules/qmk_firmware'
#set positional-arguments
set positional-arguments

default_keymap := 'macvim'

kb_ap2 := 'annepro2/c18'
kb_ap2_c18d := 'annepro2/c18d'
kb_ap2_c2d := 'annepro2/c2d'
kb_lily58 := 'lily58_2040/rp'
kb_gh60 := 'gh60/gh60'


default:
    just --list

list:
    bash scripts/qmk-worktree.sh qmk userspace-list


clean:
    bash scripts/qmk-worktree.sh qmk clean
    @echo "delete all uf2/hex/bin files"
    @find . -maxdepth 1 \( -iname "*.uf2" -o -iname "*.bin" -o -iname "*.hex" \) -print
    @rm -f *.uf2
    @rm -f *.bin
    @rm -f *.hex

_compile_kb kb km=default_keymap *args='':
    bash scripts/qmk-worktree.sh qmk compile -kb {{kb}} -km {{km}} -j20 {{args}}


annepro2: ( _compile_kb 'annepro2/c18')
    echo "compile annepro2"

annepro2-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c18 -km macvim -j20

annepro2-c18d: ( _compile_kb kb_ap2_c18d)

annepro2-c18d-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c18d -km macvim -j20

annepro2-c15:
    bash scripts/qmk-worktree.sh qmk compile -kb annepro2/c15 -km default -j20

annepro2-c2d: ( _compile_kb kb_ap2_c2d 'default')

# Reuse one disposable QMK checkout for the full matrix. Running the four
# individual recipes would expand the same 20,000+ QMK files four times.
annepro2-build-matrix:
    bash scripts/qmk-worktree.sh bash "$(pwd)/scripts/annepro2-build-matrix.sh"

annepro2-protocol-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        "$qmk_source/keyboards/annepro2/c18/annepro2_ble_protocol.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_protocol_test.c" \
        -o /tmp/annepro2_ble_protocol_c18_test
    /tmp/annepro2_ble_protocol_c18_test
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -DEXPECTED_BLE_213=1 \
        -I "$qmk_source/keyboards/annepro2" \
        -I "$qmk_source/keyboards/annepro2/c18d" \
        "$qmk_source/keyboards/annepro2/c18d/annepro2_ble_213_slot.c" \
        "$qmk_source/keyboards/annepro2/c18d/annepro2_ble_protocol.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_protocol_test.c" \
        -o /tmp/annepro2_ble_protocol_c18d_test
    /tmp/annepro2_ble_protocol_c18d_test

annepro2-slot-config-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        "$qmk_source/keyboards/annepro2/annepro2_ble_slot_config.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_slot_config_test.c" \
        -o /tmp/annepro2_ble_slot_config_test
    /tmp/annepro2_ble_slot_config_test

annepro2-state-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        "$qmk_source/keyboards/annepro2/annepro2_ble_state.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_state_test.c" \
        -o /tmp/annepro2_ble_state_test
    /tmp/annepro2_ble_state_test

annepro2-ble213-slot-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        -I "$qmk_source/keyboards/annepro2/c18d" \
        "$qmk_source/keyboards/annepro2/c18d/annepro2_ble_213_slot.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_213_slot_test.c" \
        -o /tmp/annepro2_ble_213_slot_test
    /tmp/annepro2_ble_213_slot_test

annepro2-parser-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        "$qmk_source/keyboards/annepro2/annepro2_ble_parser.c" \
        "$qmk_source/keyboards/annepro2/tests/ble_parser_test.c" \
        -o /tmp/annepro2_ble_parser_test
    /tmp/annepro2_ble_parser_test

annepro2-vendor-hid-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      cc -std=c11 -Wall -Wextra -Werror \
        -I "$qmk_source/keyboards/annepro2" \
        "$qmk_source/keyboards/annepro2/annepro2_vendor_hid.c" \
        "$qmk_source/keyboards/annepro2/tests/vendor_hid_test.c" \
        -o /tmp/annepro2_vendor_hid_test
    /tmp/annepro2_vendor_hid_test

annepro2-led-regression:
    # AP2D removed C18's external LED MCU. Keep the C18 LED protocol and
    # driver implementation byte-identical to the branch base. ap2_led.h only
    # differs by the leading whitespace removed to satisfy QMK's license lint.
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      base="$(git -C "$qmk_source" merge-base origin/master HEAD)"; \
      git -C "$qmk_source" diff --exit-code "$base" -- \
        keyboards/annepro2/ap2_led.c \
        keyboards/annepro2/protocol.c \
        keyboards/annepro2/protocol.h \
        keyboards/annepro2/rgb_driver.c \
        keyboards/annepro2/rgb_driver.h; \
      git -C "$qmk_source" diff --ignore-all-space --exit-code "$base" -- \
        keyboards/annepro2/ap2_led.h

annepro2-isolation-test:
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      if rg -n 'ANNEPRO2_BLE_(DEFAULT|FIXED)_PROFILE|ANNEPRO2_BLE_PROFILE|KC_AP2_BLE(205|213)|annepro2_ble_(get|set)_profile' \
        "$qmk_source/keyboards/annepro2"; then \
        echo "stale runtime BLE profile switching remains" >&2; \
        exit 1; \
      fi
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      if test -e "$qmk_source/keyboards/annepro2/annepro2_ble_profile.c" || \
         test -e "$qmk_source/keyboards/annepro2/annepro2_ble_profile.h"; then \
        echo "stale runtime BLE profile module remains" >&2; \
        exit 1; \
      fi
    qmk_source="$(bash scripts/qmk-source-path.sh)"; \
      if rg -n 'annepro2_ble_213|c18d/' "$qmk_source/keyboards/annepro2/c18/rules.mk"; then \
        echo "C18 still references BLE 2.13 implementation" >&2; \
        exit 1; \
      fi

annepro2-test: annepro2-protocol-test annepro2-slot-config-test annepro2-state-test annepro2-ble213-slot-test annepro2-parser-test annepro2-vendor-hid-test annepro2-led-regression annepro2-isolation-test

annepro2-validate: annepro2-test annepro2-build-matrix

annepro2-recover-ap2d-data output='/tmp/ap2d-key-3.08.data.bin':
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      python3 tools/reverse/annepro2/recover_ap2d_data.py \
        "$ap2_fw/annepro2d/firmware/3.08/annepro2_discovery_KEY_APP.bin" \
        --output {{ output }}

annepro2-ble-crossflash-check:
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
        -s tools/reverse/annepro2/tests -p 'test_*.py' -v
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py

annepro2-console-audit log *args='':
    PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/audit_console_log.py "{{ log }}" {{ args }}

annepro2-ble213-name-image output='c18-ble-2.13-annepro2c.bin':
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/patch_ble213_name.py \
        --output "{{ output }}"

annepro2-ble-crossflash-plan output='/tmp/annepro2-ble213-iap-plan.json':
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/plan_ble_iap.py \
        --output {{ output }}

annepro2-ble-crossflash-backup-check full_flash information_page:
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py \
        --full-flash-backup {{ full_flash }} \
        --information-page-backup {{ information_page }} \
        --require-hardware-backups

annepro2-iap-probe:
    annepro2_tools --probe

# Flash the exact official AP2D BLE 2.13 image and deliberately leave the
# keyboard in IAP so the matching KEY image can be written in the same session.
annepro2-flash-ble213-official:
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py; \
      annepro2_tools --target ble \
        "$ap2_fw/annepro2d/firmware/3.08/annepro2_discovery_ble.bin"

# Restore the exact official C18 BLE 2.05 image and deliberately leave the
# keyboard in IAP so the matching C18 KEY can be written next.
annepro2-flash-ble205-official:
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/validate_ble_crossflash.py; \
      annepro2_tools --target ble \
        "$ap2_fw/annepro2/c18/firmware/2.36.3/ap2_c18_0205_ble.bin"

# Regenerate the exact 2C compatibility-name image from the official input
# before every write. The generator enforces size, input/output hashes, fixed
# offsets, and the four-byte diff. Leave IAP active for the KEY write.
annepro2-flash-ble213-2c:
    ap2_fw="$(bash scripts/ap2-fw-source-path.sh)"; \
      AP2_FW_SOURCE="$ap2_fw" PYTHONDONTWRITEBYTECODE=1 python3 \
        tools/reverse/annepro2/patch_ble213_name.py \
        --output /tmp/c18-ble-2.13-annepro2c.verified.bin
    annepro2_tools --target ble \
        /tmp/c18-ble-2.13-annepro2c.verified.bin

# Flash an already-built model-specific KEY artifact, then restart the keyboard.
annepro2-flash-key image='annepro2_c18_macvim.bin':
    annepro2_tools --target main --boot "{{ image }}"

flash-annepro2-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk flash -kb annepro2/c18 -km macvim

flash-annepro2-c18d-log:
    ANNEPRO2_BLE_DEBUG=yes bash scripts/qmk-worktree.sh qmk flash -kb annepro2/c18d -km macvim

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
