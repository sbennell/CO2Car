# Changelog

All notable changes to this project will be documented in this file.

---

## [v0.2.0] - 2025-04-04

### Added
- ✅ Optional debug logging: added `#define DEBUG` flag to toggle sensor distance output.
- ✅ About section at the top of the code with project description and instructions.

### Changed
- 🔁 Refactored `startRace()` to trigger relay and start timer simultaneously for improved accuracy.

---

## [v0.1.0] - 2025-04-03

### Added
- 🎯 Basic CO₂ car race timer functionality using two VL53L0X distance sensors.
- 🔌 Relay control to simulate CO₂ canister firing.
- 🖥 Serial interface for race start (`'S'` command) and result output.
- ⏱ Millisecond timing for both lanes with automatic winner declaration.
- 🔄 Automatic system reset after each race for quick repeat runs.
