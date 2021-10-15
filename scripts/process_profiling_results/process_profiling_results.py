# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3
# %%
# Find mean, max, and min
fin = open('input.txt', 'r')
fout = open('output.txt', 'w')

# Strips the newline character
for i in range(10):
    min = 4096
    max = 0
    for ii in range(10):
        num = int(fin.readline())
        if num < min:
            min = num
        if num > max:
            max = num
    line = fin.readline()
    temp = line.split(" ")
    mean = int(temp[1])
    err_plus = max - mean
    err_minus = mean - min
    convert = 3.6 / 4095 * 1000     # Factor that converts digital values to mV voltage
    fout.write(str(i))
    fout.write(",")
    temp = str(mean * convert).split(".")
    fout.write(temp[0])
    fout.write(",")
    temp = str(err_plus * convert).split(".")
    fout.write(temp[0])
    fout.write(",")
    temp = str(err_minus * convert).split(".")
    fout.write(temp[0])
    fout.write("\n")

fin.close()
fout.close()

# %%

# Generate distribution
import math
fin = open('input.txt', 'r')
fout = open('output.txt', 'w')

dstr_group_size = 10    # Should be an even number
dstr_step = 5
dstr_ref = 260
v_task_dist = [0] * dstr_group_size
v_task_arr = []
convert = 1 / 4095 * 3600       # Factor that converts digital values to mV voltage

v_history_size = 10
v_round_cnt = 10

# Strips the newline character
for i in range(v_round_cnt):
    for ii in range(v_history_size):
        num = int(fin.readline())
        v_task_arr.append(num * convert)

    # The mean of each 10 readings calculated from the MCU
    # Uncomment / comment when appropriate
    # line = fin.readline()
    # temp = line.split(" ")
    # mean = int(temp[1])
    # v = mean * convert
    # print(v)

for i in range(v_history_size * v_round_cnt):
    if v_task_arr[i] > dstr_ref:
        group_id = int((v_task_arr[i] - dstr_ref) / dstr_step) + int(dstr_group_size / 2)
    else:
        group_id = -int(math.ceil((dstr_ref - v_task_arr[i]) / dstr_step)) + int(dstr_group_size / 2)
    print(group_id)
#     if group_id < 0:
#         group_id = 0
#     elif group_id >= dstr_group_size:
#         group_id = dstr_group_size - 1
#     v_task_dist[group_id] += 1

# print(v_task_dist)

# fout.write(str(i))
# fout.write(",")
# temp = str(mean * convert).split(".")
# fout.write(temp[0])
# fout.write(",")
# temp = str(err_plus * convert).split(".")
# fout.write(temp[0])
# fout.write(",")
# temp = str(err_minus * convert).split(".")
# fout.write(temp[0])
# fout.write("\n")


fin.close()
fout.close()
# %%