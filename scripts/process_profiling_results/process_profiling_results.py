# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3
# %%
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
