# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3
# %%

# import matplotlib.pyplot as plt

n_arr = []
v_arr = []
# print("Threshold : V_th")
for n in range(129):
    v = (56 + 6.8 + 10) / (n / 128 * 10 + 6.8) * 0.4
    # print("%4d" % n, " : ", "%6f" % v)
    n_arr.append(n)
    v_arr.append(v)

# plt.plot(n_arr, v_arr)
# plt.ylabel('V_th (V)')
# plt.xlabel('POT N')
# plt.show()

# adc_arr = []
# for n in range(4096):
#     adc_arr.append(n / 4095 * 3.6)
#     print("%4d" % n, " : ", "%6f" % adc_arr[n])


# step = 0.025    # 25mV step
# n_step = (3.6 - 1.8) / step

# for i in range(n_step + 1):


# Settings
adc_step = 32                   # Set the step of ADC readings
target_end_voltage = 1.8        # Target end voltage

print("Step of ADC reading: %d" % adc_step)
print("Target end voltage: %.2f V" % target_end_voltage)
print("")
v_step = adc_step / 4095 * 3.6
n_step = 0

thresholds = []
v_th = n_step * v_step + target_end_voltage
i = 128
# Our goal is to generate a look-up table: threshold[profiling_result / 20]
while v_th < 3.6:
    while i >= 0 and v_arr[i] < v_th:
        i -= 1
    thresholds.append(i)
    n_step += 1
    v_th = n_step * v_step + target_end_voltage

print("index    adc_profiling_result    v_droop    n_pot    v_th")
i = 0
for i in range(len(thresholds)):
    print(i, i * adc_step, i * v_step, thresholds[i], v_arr[thresholds[i]])
    i += 1

print("\nCopy the array below")
print("adc_to_threshold[%d] = {" % len(thresholds))
i = 0
for i in range(len(thresholds)):
    print("%d," % thresholds[i])
    i += 1
print("};")
# %%
