# Lily58 2040 Keymap Code Style and Structure Improvement Plan

## Executive Summary

This plan outlines a comprehensive refactoring strategy to improve the code style, structure, maintainability, and readability of the Lily58 2040 keymap source code. The current codebase has several areas that can be enhanced for better organization, consistency, and maintainability.

## Current State Analysis

### Strengths
- Functional keymap with multiple layers and custom keycodes
- Custom layout support (HRM layout)
- OLED display functionality
- Flow tap and chordal hold features
- Proper QMK integration

### Issues Identified

#### 1. Code Organization and Structure
- **Mixed concerns**: Keymap definitions, custom keycodes, and implementation logic are scattered across files
- **Inconsistent file organization**: Some functionality is in header files that should be in source files
- **Missing separation of concerns**: UI logic, keymap definitions, and custom keycode handling are mixed

#### 2. Naming Conventions
- **Inconsistent naming**: Mix of different naming styles (e.g., `UK_VGCP`, `LN_TAB`, `HRM_A`)
- **Unclear abbreviations**: Many abbreviations lack clear meaning (e.g., `UK_`, `LN_`, `HRM_`)
- **Inconsistent layer naming**: Multiple aliases for the same layer (e.g., `LLOWER`, `LLW`)

#### 3. Code Style Issues
- **Inconsistent formatting**: Mixed indentation and spacing
- **Commented out code**: Large blocks of commented code that should be removed
- **Magic numbers**: Hard-coded values without clear documentation
- **Inconsistent comment style**: Mix of comment formats

#### 4. Maintainability Issues
- **Tight coupling**: Custom keycodes and their implementations are tightly coupled
- **Hard to extend**: Adding new features requires modifying multiple files
- **Limited documentation**: Insufficient inline documentation for complex logic

## Improvement Plan

### Phase 1: Code Organization and File Structure

#### 1.1 Reorganize File Structure
```
keyboards/lily58_2040/keymaps/macvim/
├── keymap.c              # Main keymap definitions only
├── config.h              # Configuration constants
├── custom_keycodes.h     # Custom keycode definitions
├── custom_keycodes.c     # Custom keycode implementations
├── layers.h              # Layer definitions and constants
├── layouts.h             # Layout definitions (HRM, etc.)
├── oled_display.h        # OLED display interface
├── oled_display.c        # OLED display implementation
├── tap_hold.h           # Tap/hold functionality interface (flow tap + chordal hold)
├── tap_hold.c           # Tap/hold functionality implementation
└── rules.mk             # Build configuration
```

#### 1.2 Separate Concerns
- **keymap.c**: Only keymap definitions and layer arrays
- **custom_keycodes.c**: All custom keycode processing logic
- **oled_display.c**: All OLED-related functionality
- **tap_hold.c**: All tap/hold related functionality (flow tap, chordal hold, shift tab logic)

### Phase 2: Naming Convention Standardization

#### 2.1 Custom Keycode Naming
**Current**: `UK_VGCP`, `UK_CAPR`, `UK_STAB`
**Proposed**: `CUSTOM_VIM_YANK`, `CUSTOM_CAPTURE_REGION`, `CUSTOM_SHIFT_TAB`

#### 2.2 Layer Naming
**Current**: `BASE_MAC`, `BASE_WIN`, `LLOWER`, `LRAISE`, `LFUNC`, `LNAVI`, `LNUM`, `LDEBUG`
**Proposed**: `LAYER_BASE_MAC`, `LAYER_BASE_WIN`, `LAYER_LOWER`, `LAYER_RAISE`, `LAYER_FUNCTION`, `LAYER_NAVIGATION`, `LAYER_NUMBER`, `LAYER_DEBUG`

#### 2.3 Layout Naming
**Current**: `LAYOUT_HRM`
**Proposed**: `LAYOUT_HOMEROW_MOD`

#### 2.4 Function Naming
**Current**: `is_oppsite_hand_tap_flow`, `flow_tap_chordal_reset`
**Proposed**: `is_opposite_hand_tap_flow`, `reset_chordal_flow_tap`

### Phase 3: Code Style Improvements

#### 3.1 Consistent Formatting
- Apply consistent indentation (4 spaces)
- Standardize spacing around operators and keywords
- Use consistent brace placement
- Apply consistent comment formatting

#### 3.2 Remove Dead Code
- Remove all commented-out code blocks
- Remove unused variables and functions
- Clean up unused includes

#### 3.3 Magic Number Elimination
- Define constants for all magic numbers
- Group related constants together
- Add clear documentation for each constant

#### 3.4 Documentation Standards
- Add comprehensive header comments for each file
- Document all public functions and macros
- Add inline comments for complex logic
- Create a README for the keymap

### Phase 4: Architecture Improvements

#### 4.1 Modular Design
- Create clear interfaces between modules
- Reduce coupling between components
- Make the codebase more testable

#### 4.2 Configuration Management
- Centralize all configuration in `config.h`
- Use feature flags for optional functionality
- Make configuration more user-friendly

#### 4.3 Error Handling
- Add proper error checking for critical operations
- Implement graceful degradation for optional features
- Add debug output for troubleshooting

### Phase 5: Performance and Reliability

#### 5.1 Memory Optimization
- Optimize OLED display buffer usage
- Reduce stack usage in keycode processing
- Optimize timer usage

#### 5.2 Reliability Improvements
- Add bounds checking for array access
- Implement proper state management
- Add watchdog functionality where appropriate

## Implementation Timeline

### Week 1: Foundation
- [ ] Create new file structure
- [ ] Move existing code to new files
- [ ] Establish basic naming conventions

### Week 2: Core Refactoring
- [ ] Refactor custom keycodes
- [ ] Improve layer definitions
- [ ] Standardize layout definitions

### Week 3: Feature Modules
- [ ] Refactor OLED display code
- [ ] Refactor tap/hold functionality (flow tap + chordal hold)

### Week 4: Polish and Documentation
- [ ] Add comprehensive documentation
- [ ] Remove dead code
- [ ] Final testing and validation

## Success Criteria

### Code Quality Metrics
- **Cyclomatic complexity**: Reduce complexity of key functions
- **Code duplication**: Eliminate duplicate code patterns
- **Documentation coverage**: Achieve 100% documentation for public APIs
- **Test coverage**: Add unit tests for critical functionality

### Maintainability Goals
- **Time to add new layer**: < 30 minutes
- **Time to add new custom keycode**: < 15 minutes
- **Time to modify layout**: < 20 minutes
- **Code review time**: Reduce by 50%

### Performance Goals
- **Compile time**: No significant increase
- **Runtime performance**: Maintain or improve current performance
- **Memory usage**: Reduce by 10-20%

## Risk Mitigation

### Technical Risks
- **Breaking changes**: Maintain backward compatibility during transition
- **Build failures**: Implement incremental changes with testing
- **Performance regression**: Benchmark before and after changes

### Process Risks
- **Scope creep**: Stick to defined phases and timeline
- **Quality issues**: Implement code review process
- **Documentation gaps**: Make documentation part of acceptance criteria

## Conclusion

This refactoring plan will transform the Lily58 2040 keymap codebase into a well-structured, maintainable, and extensible codebase. The improvements will make it easier for developers to understand, modify, and extend the functionality while maintaining the current feature set and performance characteristics.

The modular approach will also make it easier to port features to other keyboards and contribute back to the QMK community. 
