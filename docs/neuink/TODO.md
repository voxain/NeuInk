# Open items

Things deliberately left for later. Each one says when to pick it up.

## Battery statistics

Battery health and usage. The page exists (X4 Pro, 0.3.0); what is left is health and cycles.

- X3: the BQ27220 fuel gauge keeps its own counters. Read CycleCount (0x2A), StateOfHealth (0x2E, percent) and
  FullChargeCapacity (0x12, against DesignCapacity 0x3C) over I2C in `BatteryMonitor`, next to the voltage,
  current and charge reads it already does. Check the register addresses against the datasheet and a real X3
  before relying on them.
- X4 Pro: done in 0.3.0 (graph, runtimes, per-app charge; see Settings > Battery). Charge sessions and estimated full
  cycles from the voltage-based percentage are not counted yet.
- Shared page: show real gauge values where there are any, estimates elsewhere, and say which is which.
