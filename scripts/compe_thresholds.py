# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3
# %%

# print("CEREF_n : v threshold")
# for n in range(32):
#     v_th = 3.6 * (n + 1) / 32
#     print("%2d" % n, ":", "%.4f" % v_th)

print("adc_th[32] = {")
for n in range(32):
    adc = 4095 * (n + 1) / 32 - 64
    v_th = 3.6 * (n + 1) / 32
    # print("%2d" % n, ":", "%4d" % adc)
    print("%4d" % adc, ",")
    # print(adc, ":", v_th)
print("};")

# %%
t = 3000
x = (t - 1.7e3) / 1.2e3
y = -2.8e-5 * pow(x, 3) - 2.1e-8 * pow(x, 2) - 1.2e-5 * x + 0.002
print(y)
percent = (y / 0.0020965666504629632 - 1) * 100
print("%f" % percent, "%")


# %%
