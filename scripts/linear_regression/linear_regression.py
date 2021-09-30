# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3

# %%
import numpy as np
from sklearn.linear_model import LinearRegression

import csv
xx =[]
yy = []
with open('variable_data.csv') as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    line_count = 0
    for row in csv_reader:
        if line_count == 0:
            print(f'Column names are {", ".join(row)}')
            line_count += 1
        else:
            # print(f'\t{row[0]} : {row[1]}.')
            xx.append(float(row[0]))
            yy.append(float(row[1]))
            line_count += 1
    print(f'Processed {line_count} lines.')
    print(xx)
    print(yy)

x = np.array(xx).reshape((-1, 1))
y = np.array(yy)

model = LinearRegression().fit(x, y)

r_sq = model.score(x, y)
print('coefficient of determination:', r_sq)
print('intercept:', model.intercept_)
print('slope:', model.coef_)

#%%
