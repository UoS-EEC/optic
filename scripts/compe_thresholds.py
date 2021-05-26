# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

v_max = 1.2 * 3

print("CEREF_n : v threshold")
for n in range(32):
    v_th = v_max * (n + 1) / 32
    print(n, ":", "%.4f" % v_th)
