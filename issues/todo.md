# Lily58 2040 Keymap Refactoring Todo List

## Phase 1: File Structure
- [x] Create new files: `custom_keycodes.h/c`, `layers.h`, `layouts.h`, `oled_display.h/c`, `tap_hold.h/c`
- [x] Move code from `lily58_2040.h/c` to appropriate new files
- [x] Update `rules.mk` and includes
- [x] Test build

## Phase 2: Naming Standards
- [x] Rename custom keycodes: `UK_*` → `CUSTOM_*`
- [x] Rename layers: `L*` → `LAYER_*`
- [x] Rename layout: `LAYOUT_HRM` → `LAYOUT_HOMEROW_MOD`
- [x] Fix typos: `is_oppsite_hand` → `is_opposite_hand`
- [x] Remove duplicate layer aliases

## Phase 3: Code Cleanup
- [x] Remove all commented-out code
- [x] Define constants for magic numbers
- [x] Fix indentation and formatting
- [x] Clean up duplicate includes

## Phase 4: Documentation
- [ ] Add header comments to all files
- [ ] Document public functions and macros
- [ ] Create README.md
- [ ] Add inline comments for complex logic

## Phase 5: Testing
- [ ] Test all functionality works
- [ ] Verify no performance regression
- [ ] Check for compilation warnings 
