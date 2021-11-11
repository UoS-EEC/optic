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
filename = 'pv_curve.csv'  # Modify filename to the relavant data
x_col = 0       # Modify index to the x column
y_col = 1       # Modify index to the y column
with open(filename) as csv_file:
    csv_reader = csv.reader(csv_file, delimiter=',')
    size = 0
    for row in csv_reader:
        if size == 0:
            # print(f'Column names are {", ".join(row)}')
            x_col_name = row[x_col]
            y_col_name = row[y_col]
        else:
            xx.append(float(row[x_col]))
            yy.append(float(row[y_col]))
        size += 1
    size -= 1
    print(f'Processed {size} lines of data.')
    print("x = ", "'" + x_col_name + "'", xx)
    print("y = ", "'" + y_col_name + "'", yy)

x = np.array(xx).reshape((-1, 1))
y = np.array(yy)

model = LinearRegression().fit(x, y)

r_sq = model.score(x, y)

print('')
print('Intercept:', model.intercept_)
print('Slope:', model.coef_[0])
print('Linear fit equation:', "y =", model.coef_[0], "* x +", model.intercept_)
print('Coefficient of determination:', r_sq)


print('')
print('Manual uni-variate linear regression using normal equation:')
sum_x = 0
sum_x2 = 0
sum_y = 0
sum_xy  = 0
for i in range(size):
    sum_x += xx[i]
    sum_x2 += xx[i] * xx[i]
    sum_y += yy[i]
    sum_xy += xx[i] * yy[i]
theta_0 = (sum_x2 * sum_y - sum_x * sum_xy) / (size * sum_x2 - sum_x * sum_x)
theta_1 = (size * sum_xy - sum_x * sum_y) / (size * sum_x2 - sum_x * sum_x)
print("theta_0 =", theta_0)
print("theta_1 =", theta_1)
print(size * sum_x2 - sum_x * sum_x)


#%%
