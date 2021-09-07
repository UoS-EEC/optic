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


# Settings
adc_step = 32                   # Set the step of ADC readings
target_end_voltage = 1.8        # Target end voltage



print("Settings:")
print("Step of ADC reading: %d" % adc_step, "(%.3fmV per step)" % (adc_step / 4095 * 3.6 * 1000))
print("Target end voltage: %.2f V" % target_end_voltage)
print("")

v_step = adc_step / 4095 * 3.6
n_step = 0
thresholds = []
v_th = n_step * v_step + target_end_voltage
i = 128
# The goal is to generate a look-up table: threshold[profiling_result / adc_step]
while v_th < 3.6:
    while i >= 0 and v_arr[i] < v_th:
        i -= 1
    thresholds.append(i)
    n_step += 1
    v_th = n_step * v_step + target_end_voltage

print("index  profiling_result  v_droop  n_pot   v_th")
i = 0
for i in range(len(thresholds)):
    print("%5d  %16d  %7.3f  %5d  %.3f" % (i, i * adc_step,  i * v_step, thresholds[i], v_arr[thresholds[i]]))
    i += 1

print("\nTarget end threshold: %d" % thresholds[0])
print("Lookup table:")
print("adc_to_threshold[%d] = {" % (len(thresholds) - 1))

for i in range(1, len(thresholds)):
    print("    %3d,  // %3d, %.3fV" % (thresholds[i], i - 1, v_arr[thresholds[i]]))
    i += 1
print("};")
# %%