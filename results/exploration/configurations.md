# Experiment on the impact of variable peripheral configurations

## 1. Info and setup

Date of experiment: 21 Sep 2021

Workload: AES encryption of 4KB (actually 4080B) data, configured as 128-bit, 192-bit, and 256-bit respectively.

Method: Short-circuit the supply at the high side of the schottky diode (so that no energy comes in during the operation), measure the voltage difference before and after the operation.

System capacitance: 33.6uF measured (charging with 100uA to 1V, measurements: 338.424ms to charge 1.006V)

## 2. Results

Voltage drop. Errors are omitted as they are too small to measure (<5mV).

| Configuration | Delta V | Time    |
| ------------- | ------- | ------- |
| 128-bit       | 194mV   | 6.479ms |
| 192-bit       | 230mV   | 7.638ms |
| 256-bit       | 245mV   | 8.606ms |
