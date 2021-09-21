# Experiment on the data size impact

## 1. Info and setup

Date of experiment: 20 Sep 2021

Workload: AES128 encryption

Method: Short-circuit the supply at the high side of the schottky diode (so that no energy comes in during the operation), measure the voltage difference before and after the operation.

System capacitance: 33.6uF measured (charging with 100uA to 1V, measurements: 338.424ms to charge 1.006V)

## 2. Results

Voltage drop

| Data size   | V1    | V2    | V3    | V4    | V5    | Delta V avg. | Time    | Charge |
| ----------- | ----- | ----- | ----- | ----- | ----- | ------------ | ------- | ------ |
| 512B        | 36mV  | 36mV  | 36mV  | 36mV  | 36mV  | 36mV         | 0.876ms | 1.21uC |
| 1KB         | 56mV  | 56mV  | 56mV  | 56mV  | 56mV  | 56mV         | 1.680ms | 1.88uC |
| 2KB         | 97mV  | 97mV  | 97mV  | 97mV  | 97mV  | 97mV         | 3.288ms | 3.26uC |
| 3KB         | 133mV | 133mV | 133mV | 133mV | 133mV | 133mV        | 4.896ms | 4.47uC |
| 4KB (4080B) | 174mV | 174mV | 174mV | 174mV | 174mV | 174mV        | 6.479ms | 5.85uC |