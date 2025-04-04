[v0.1.0] - 2025-04-03
Added

    🎯 Basic CO₂ car race timer functionality using two VL53L0X distance sensors.

    🔌 Relay control to simulate CO₂ canister firing.

    🖥 Serial interface for race start ('S' command) and result output.

    ⏱ Millisecond timing for both lanes with automatic winner declaration.

    🔄 Automatic system reset after each race for quick repeat runs.

[v0.2.0] - 2025-04-04
Added

    ✅ Optional debug logging: added #define DEBUG flag to toggle sensor distance output.

    ✅ About section at top of code (project description placeholder).

Changed

    🔁 Refactored startRace() to initiate relay trigger and timer simultaneously for improved timing accuracy.