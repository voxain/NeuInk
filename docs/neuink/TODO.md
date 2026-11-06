# Open items

Things deliberately left for later. Each one says when to pick it up.

## Battery statistics

A Settings page (or part of About) with battery health and usage.

- X3: the BQ27220 fuel gauge keeps its own counters. Read CycleCount (0x2A), StateOfHealth (0x2E, percent) and
  FullChargeCapacity (0x12, against DesignCapacity 0x3C) over I2C in `BatteryMonitor`, next to the voltage,
  current and charge reads it already does. Check the register addresses against the datasheet and a real X3
  before relying on them.
- X4 Pro: no gauge (voltage on GPIO0 through the ADC only), so count it ourselves: charge sessions, estimated
  full cycles from the voltage-based percentage, screen-on against sleep time and drain per hour of reading,
  stored in the state file. These are estimates, not measured capacity.
- Shared page: show real gauge values where there are any, estimates elsewhere, and say which is which.
