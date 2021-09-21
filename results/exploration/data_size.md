# Experiment on the data size impact

## 1. Info and setup

Date of experiment: 20 Sep 2021

Workload: AES128 encryption

Method: Short-circuit the supply at the high side of the schottky diode (so that no energy comes in during the operation), measure the voltage difference before and after the operation.

System capacitance: 33.6uF measured (charging with 100uA to 1V, measurements: 338.424ms to charge 1.006V)

## 2. Results

Voltage drop. Errors are omitted as they are too small to measure (<5mV).

| Data size   | Delta V | Time    | Charge |
| ----------- | ------- | ------- | ------ |
| 256B        | 26mV    | 0.475ms | 0.87uC |
| 512B        | 41mV    | 0.876ms | 1.38uC |
| 1KB         | 61mV    | 1.680ms | 2.05uC |
| 1.5KB       | 82mV    | 2.484ms | 2.76uC |
| 2KB         | 107mV   | 3.288ms | 3.60uC |
| 2.5KB       | 128mV   | 4.092ms | 4.30uC |
| 3KB         | 153mV   | 4.896ms | 5.14uC |
| 3.5B        | 169mV   | 5.700ms | 5.68uC |
| 4KB (4080B) | 194mV   | 6.479ms | 6.52uC |