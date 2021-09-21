# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3

# %%
import numpy as np
from sklearn.linear_model import LinearRegression

x = np.array([0.25, 0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4]).reshape((-1, 1))
y = np.array([26, 41, 61, 82, 107, 128, 153, 169, 194])
model = LinearRegression().fit(x, y)

r_sq = model.score(x, y)
print('coefficient of determination:', r_sq)
print('intercept:', model.intercept_)
print('slope:', model.coef_)
