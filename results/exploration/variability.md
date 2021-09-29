# Experiment on the impact of variable peripheral configurations and devices

## 1. Info and setup

Workload: AES encryption of 4KB (actually 4080B) data, configured as 128-bit, 192-bit, and 256-bit respectively.

Method: Short-circuit the supply at the high side of the schottky diode (so that no energy comes in during the operation), measure the voltage difference before and after the operation.

Capacitance becore the cold start switch: 33.6uF measured (charging with 100uA to 1V, measurements: 338.424ms to charge 1.006V)

So the system capacitance should be 35uF with additional 1.5uF on board (and it it so as measured).

Error bars (variations) are omitted as they are too small to observe (<5mV, precision of the scope).

## 2. Results

### Data size

Date of experiment: 21 Sep 2021

Workload: AES128 encryption

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
| 3.5KB       | 169mV   | 5.700ms | 5.68uC |
| 4KB (4080B) | 194mV   | 6.479ms | 6.52uC |


### Peripheral configurations

Date of experiment: 21 Sep 2021

Working on Device No. 1, 4KB encryption

| Configuration | Delta V | Time    |
| ------------- | ------- | ------- |
| 128-bit       | 194mV   | 6.479ms |
| 192-bit       | 230mV   | 7.638ms |
| 256-bit       | 245mV   | 8.606ms |


### Devices

Date of experiment: 23 Sep 2021

AES 128-bit encryption 4KB

| Device No. | Delta V | Time    | Capacitance |
| ---------- | ------- | ------- | ----------- |
| 1          | 194mV   | 6.479ms | 35uF        |
| 2          | 185mV   | 6.444ms | 29uF        |
| 3          | 179mV   | 6.462ms | 29uF        |


### Voltage

Date of experiment: 23 Sep 2021

| Start voltage | End Voltage | Delta V |
| ------------- | ----------- | ------- |
| 2.020V        | 1.836V      | 184mV   |
| 2.102V        | 1.917V      | 185mV   |
| 2.214V        | 2.030V      | 184mV   |
| 2.301V        | 2.117V      | 184mV   |
| 2.424V        | 2.240V      | 184mV   |
| 2.542V        | 2.352V      | 190mV   |
| 2.634V        | 2.444V      | 190mV   |
| 2.710V        | 2.521V      | 189mV   |
| 3.360V        | 3.166V      | 194mV   |
