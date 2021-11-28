# Copyright (c) 2020-2021, University of Southampton and Contributors.
# All rights reserved.
# SPDX-License-Identifier: MIT

#!/usr/local/bin/python3
# %%

import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

import random

def charge_consumption(block) -> float:
    return (0.69386485 * block + 16.806635264014872) * 1e-3 * 35e-6

def run_time(block) -> float:
    return (0.02512329 * block + 0.07234669109095204) * 1e-3

config_scale_v = [1, 1.185567, 1.262886]
config_scale_t = [1, 1.178886, 1.328291]
system_capacitance = 10e-6  # 10uF
capacitance_reduction = 0

class BaseIntermittent:
    # arr1 and arr2 should have the same len
    def __init__(self,vcc_init,arr1,arr2) -> None:
        self.c = system_capacitance * (1 - capacitance_reduction)

        self.i_draw_lpm = 10e-6
        self.i_draw_exe = 0
        self.i_draw = self.i_draw_lpm

        self.t_task = 0
        self.t_remain = 0

        self.v_off = 1.8  # Constant
        self.v_task = 0
        self.v_th = 0
        self.vcc = 0

        self.state = 0
        # state can be
        # 0: low power mode (sleep, voltage monitor on, ~10uA) 
        #       ..or off (SVS on, ~10uA), i_draw = i_draw_lpm
        # 1: executing atomic operation, i_draw = i_draw_exe

        # Results
        self.n_completion = 0
        self.n_failure = 0

        self.rd_data = []
        self.rd_config = []
        self.rd_len = 0
        self.rd_index = 0

        self.vcc = vcc_init
        # Set up the sequence for random data sizes and configurations
        self.rd_data = arr1
        self.rd_config = arr2
        self.rd_len = len(self.rd_data)

        # Init parameters
        self.t_task = run_time(self.rd_data[self.rd_index]) * config_scale_t[self.rd_config[self.rd_index]]
        self.t_remain = self.t_task
        self.v_task = charge_consumption(self.rd_data[self.rd_index] * config_scale_v[self.rd_config[self.rd_index]]) / self.c
        self.i_draw_exe = self.v_task * self.c / self.t_task

        # Index to the next task
        self.rd_index += 1
        if self.rd_index >= self.rd_len:
            self.rd_index = 0

    def update(self, i_in, t_step) -> float:
        self.update_state(t_step)
        # Uncomment the following 3 lines to cut off supply when executing
        # (Useful for checking whether v_exe is adapted correctly)
        # if self.state == 1:
        #     self.update_vcc(0, t_step)
        # else:
        self.update_vcc(i_in, t_step)
        return self.vcc

    def update_state(self, t_step) -> None:
        pass
    
    def update_vcc(self, i_in, t_step) -> None:
        self.vcc = self.vcc + (i_in - self.i_draw) * t_step / self.c

class AdaptEnergy(BaseIntermittent):
    def __init__(self, vcc_init, arr1, arr2) -> None:
        super().__init__(vcc_init,arr1,arr2)
        # Set up adaptive threshold
        self.v_target_end = self.v_off + 0.01     # 10mV margin
        self.v_th = self.v_target_end + self.v_task
    
    def update_state(self, t_step):
        # lpm or off
        if self.state == 0:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_th:
                self.state = 1
                self.i_draw = self.i_draw_exe
        # exe
        elif self.state == 1:
            # self.i_draw = self.i_draw_exe
            if self.vcc < self.v_off:   # Power fails
                self.state = 0
                self.i_draw = self.i_draw_lpm
                self.n_failure += 1
            elif self.t_remain <= 0:    # Finish operation, go sleep
                self.state = 0
                self.i_draw = self.i_draw_lpm

                # Output a completion here
                self.n_completion += 1

                # Set up the next operation
                # Refresh parameters
                self.t_task = run_time(self.rd_data[self.rd_index]) * config_scale_t[self.rd_config[self.rd_index]]
                self.t_remain = self.t_task
                self.v_task = charge_consumption(self.rd_data[self.rd_index] * config_scale_v[self.rd_config[self.rd_index]]) / self.c
                self.i_draw_exe = self.v_task * self.c / self.t_task
                # Index to the next task
                self.rd_index += 1
                if self.rd_index >= self.rd_len:
                    self.rd_index = 0
                # Set up the next adaptive threshold
                self.v_th = self.v_target_end + self.v_task
            else:                       # Still executing
                self.t_remain = self.t_remain - t_step

class Samoyed(BaseIntermittent):
    def __init__(self, vcc_init, arr1, arr2) -> None:
        super().__init__(vcc_init,arr1,arr2)
        # Set up a fixed abundant energy threshold
        self.v_th = 3.6     # Max operating voltage for MSP4305994
    
    def update_state(self, t_step):
        # lpm or off
        if self.state == 0:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_th:
                self.state = 1
                self.i_draw = self.i_draw_exe
        # exe
        elif self.state == 1:
            # self.i_draw = self.i_draw_exe
            if self.vcc < self.v_off:   # Power fails
                self.state = 0
                self.i_draw = self.i_draw_lpm
                self.t_remain = self.t_task
                self.n_failure += 1
            elif self.t_remain <= 0:    # Finish operation, keep executing
                # Output a completion here
                self.n_completion += 1

                # Set up the next operation
                # Refresh parameters
                self.t_task = run_time(self.rd_data[self.rd_index]) * config_scale_t[self.rd_config[self.rd_index]]
                self.t_remain = self.t_task
                self.v_task = charge_consumption(self.rd_data[self.rd_index] * config_scale_v[self.rd_config[self.rd_index]]) / self.c
                self.i_draw_exe = self.v_task * self.c / self.t_task
                # Index to the next task
                self.rd_index += 1
                if self.rd_index >= self.rd_len:
                    self.rd_index = 0
            else:                       # Still executing
                self.t_remain = self.t_remain - t_step

class DEBS(BaseIntermittent):
    def __init__(self, vcc_init, arr1, arr2, budget) -> None:
        super().__init__(vcc_init,arr1,arr2)
        # Set up a fixed threshold
        self.v_target_end = self.v_off + 0.01     # 10mV margin
        self.v_th = self.v_target_end + budget
    
    def update_state(self, t_step):
        # lpm or off
        if self.state == 0:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_th:
                self.state = 1
                self.i_draw = self.i_draw_exe
        # exe
        elif self.state == 1:
            # self.i_draw = self.i_draw_exe
            if self.vcc < self.v_off:   # Power fails
                self.state = 0
                self.i_draw = self.i_draw_lpm
                self.t_remain = self.t_task
                self.n_failure += 1
            elif self.t_remain <= 0:    # Finish operation, go sleep
                self.state = 0
                self.i_draw = self.i_draw_lpm

                # Output a completion here
                self.n_completion += 1

                # Set up the next operation
                # Refresh parameters
                self.t_task = run_time(self.rd_data[self.rd_index]) * config_scale_t[self.rd_config[self.rd_index]]
                self.t_remain = self.t_task
                self.v_task = charge_consumption(self.rd_data[self.rd_index] * config_scale_v[self.rd_config[self.rd_index]]) / self.c
                self.i_draw_exe = self.v_task * self.c / self.t_task
                # Index to the next task
                self.rd_index += 1
                if self.rd_index >= self.rd_len:
                    self.rd_index = 0
            else:                       # Still executing
                self.t_remain = self.t_remain - t_step

import math
I_sc = 280
V_oc = 3.03
I_mpp = 240
V_mpp = 2.3

def pv_current(v):
    if v < 0:
        return 278.562e-6
    elif 0 < v and v <= V_mpp:
        return (-15.728 * v + 278.562) *1e-6
    elif V_mpp < v and v <= V_oc:
        return (I_sc * (1 - math.exp(math.log(1 - I_mpp / I_sc) * (v - V_oc) / (V_mpp - V_oc)))) * 1e-6
    else:   # v > V_oc
        return 0

# Single test main
def main_single():
    # Generate random numbers
    arr_len = 1000
    # Array 1 for random data size
    random_arr1 = []
    # Random integer numbers in [1, 255] (1-255 blocks, 16B per blocks)
    i = arr_len
    while i > 0:
        random_arr1.append(random.randint(1, 255))
        i -= 1
    # Array 2 for randomized current draw
    random_arr2 = []
    # Random interger numbers in [0, 2] (representing 3 configurations)
    i = arr_len
    while i > 0:
        random_arr2.append(random.randint(0, 2))
        i -= 1

    vcc_init = 0
    t_step = 1e-6       # Simulation time step
    t = 0
    vcc_repa = vcc_init
    vcc_samoyed = vcc_init
    vcc_debs_low = vcc_init
    vcc_debs_high = vcc_init

    t_trace = []
    v_trace_repa = []
    v_trace_samoyed = []
    v_trace_debs_low = []
    v_trace_debs_high = []
    t_trace.append(t)
    v_trace_repa.append(vcc_repa)
    v_trace_samoyed.append(vcc_samoyed)
    v_trace_debs_low.append(vcc_debs_low)
    v_trace_debs_high.append(vcc_debs_high)

    repa_obj = AdaptEnergy(vcc_repa, random_arr1, random_arr2)
    samoyed_obj = Samoyed(vcc_samoyed, random_arr1, random_arr2)
    debs_low_v_task = charge_consumption(64) / system_capacitance
    debs_low_obj = DEBS(vcc_debs_low, random_arr1, random_arr2, debs_low_v_task)
    # debs_high_v_task = charge_consumption(255) * config_scale_v[2] / (system_capacitance / 2)
    debs_high_v_task = 1.79
    debs_high_obj = DEBS(vcc_debs_high, random_arr1, random_arr2, debs_high_v_task)

    while t < 2:  # Total simulation time in seconds
        vcc_repa = repa_obj.update(pv_current(vcc_repa), t_step)
        vcc_samoyed = samoyed_obj.update(pv_current(vcc_samoyed), t_step)
        vcc_debs_low = debs_low_obj.update(pv_current(vcc_debs_low), t_step)
        vcc_debs_high = debs_high_obj.update(pv_current(vcc_debs_high), t_step)

        v_trace_repa.append(vcc_repa)
        v_trace_samoyed.append(vcc_samoyed)
        v_trace_debs_low.append(vcc_debs_low)
        v_trace_debs_high.append(vcc_debs_high)

        t += t_step
        t_trace.append(t)

    print("Number of completions, failures\n")
    print("AdaptEnergy:", repa_obj.n_completion, repa_obj.n_failure)
    print("Samoyed:", samoyed_obj.n_completion, samoyed_obj.n_failure)
    print("DEBS low: ", debs_low_obj.n_completion, debs_low_obj.n_failure)
    print("DEBS high: ", debs_high_obj.n_completion, debs_high_obj.n_failure)

    fig = plt.figure(constrained_layout=True)
    gs = gridspec.GridSpec(ncols=2, nrows=2, figure=fig)
    

    ax0 = fig.add_subplot(gs[0, 0])
    ax0.plot(t_trace, v_trace_debs_low)
    ax0.set_title('DEBS Low')
    ax0.set_xlabel('Time (s)')
    ax0.set_ylabel('Voltage (V)')
    ax0.set_xlim([0, 2])
    ax0.set_ylim([0, 3.6])

    ax1 = fig.add_subplot(gs[0, 1])
    ax1.plot(t_trace, v_trace_samoyed)
    ax1.set_title('Samoyed')
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('Voltage (V)')
    ax1.set_xlim([0, 2])
    ax1.set_ylim([0, 3.6])

    ax2 = fig.add_subplot(gs[1, 0])
    ax2.plot(t_trace, v_trace_debs_high)
    ax2.set_title('DEBS High')
    ax2.set_xlabel('Time (s)')
    ax2.set_ylabel('Voltage (V)')
    ax2.set_xlim([0, 2])
    ax2.set_ylim([0, 3.6])

    ax3 = fig.add_subplot(gs[1, 1])
    ax3.plot(t_trace, v_trace_repa)
    ax3.set_title('Adaptive')
    ax3.set_xlabel('Time (s)')
    ax3.set_ylabel('Voltage (V)')
    ax3.set_xlim([0, 2])
    ax3.set_ylim([0, 3.6])

    # plt.show()
    plt.savefig('voltage_traces.pdf') 


# Multi tests main
def main_multi():
    repa_completion =[]
    repa_failure = []
    samoyed_completion = []
    samoyed_failure = []
    debs_low_completion = []
    debs_low_failure = []
    debs_high_completion = []
    debs_high_failure = []

    # print("Number of completions, failures")
    for loop_count in range(10):
        # Generate random numbers
        arr_len = 1000
        # Array 1 for random data size
        random_arr1 = []
        # Random integer numbers in [1, 255] (1-255 blocks, 16B per blocks)
        i = arr_len
        while i > 0:
            random_arr1.append(random.randint(1, 255))
            i -= 1
        # Array 2 for randomized current draw
        random_arr2 = []
        # Random interger numbers in [0, 2] (representing 3 configurations)
        i = arr_len
        while i > 0:
            random_arr2.append(random.randint(0, 2))
            i -= 1

        vcc_init = 0
        t_step = 1e-6       # Simulation time step
        t = 0
        vcc_repa = vcc_init
        vcc_samoyed = vcc_init
        vcc_debs_low = vcc_init
        vcc_debs_high = vcc_init

        repa_obj = AdaptEnergy(vcc_repa, random_arr1, random_arr2)
        samoyed_obj = Samoyed(vcc_samoyed, random_arr1, random_arr2)
        debs_low_v_task = charge_consumption(64) / system_capacitance
        debs_low_obj = DEBS(vcc_debs_low, random_arr1, random_arr2, debs_low_v_task)
        # debs_high_v_task = charge_consumption(255) * config_scale_v[2] / (system_capacitance / 2)
        debs_high_v_task = 1.79
        debs_high_obj = DEBS(vcc_debs_high, random_arr1, random_arr2, debs_high_v_task)

        while t < 2:  # Total simulation time in seconds
            vcc_repa = repa_obj.update(pv_current(vcc_repa), t_step)
            vcc_samoyed = samoyed_obj.update(pv_current(vcc_samoyed), t_step)
            vcc_debs_low = debs_low_obj.update(pv_current(vcc_debs_low), t_step)
            vcc_debs_high = debs_high_obj.update(pv_current(vcc_debs_high), t_step)
            t += t_step

        print("Round", loop_count, "completed.")

        repa_completion.append(repa_obj.n_completion)
        repa_failure.append(repa_obj.n_failure)
        samoyed_completion.append(samoyed_obj.n_completion)
        samoyed_failure.append(samoyed_obj.n_failure)
        debs_low_completion.append(debs_low_obj.n_completion)
        debs_low_failure.append(debs_low_obj.n_failure)
        debs_high_completion.append(debs_high_obj.n_completion)
        debs_high_failure.append(debs_high_obj.n_failure)

        del repa_obj
        del samoyed_obj
        del debs_low_obj
        del debs_high_obj

    print(" ")
    print("[Number of completions], [Number of failures]")
    print("(Number of completions) Mean, Max, Min")
    print("(Number of failures) Mean, Max, Min")
    print(" ")

    debs_low_perf_mean = sum(debs_low_completion) / len(debs_low_completion)
    debs_low_perf_err_p = max(debs_low_completion) - debs_low_perf_mean
    debs_low_perf_err_m = debs_low_perf_mean - min(debs_low_completion)
    debs_low_fail_mean = sum(debs_low_failure) / len(debs_low_failure)
    debs_low_fail_err_p = max(debs_low_failure) - debs_low_fail_mean
    debs_low_fail_err_m = debs_low_fail_mean - min(debs_low_failure)

    samoyed_perf_mean = sum(samoyed_completion) / len(samoyed_completion)
    samoyed_perf_err_p = max(samoyed_completion) - samoyed_perf_mean
    samoyed_perf_err_m = samoyed_perf_mean - min(samoyed_completion)
    samoyed_fail_mean = sum(samoyed_failure) / len(samoyed_failure)
    samoyed_fail_err_p = max(samoyed_failure) - samoyed_fail_mean
    samoyed_fail_err_m = samoyed_fail_mean - min(samoyed_failure)


    debs_high_perf_mean = sum(debs_high_completion) / len(debs_high_completion)
    debs_high_perf_err_p = max(debs_high_completion) - debs_high_perf_mean
    debs_high_perf_err_m = debs_high_perf_mean - min(debs_high_completion)
    debs_high_fail_mean = sum(debs_high_failure) / len(debs_high_failure)
    debs_high_fail_err_p = max(debs_high_failure) - debs_high_fail_mean
    debs_high_fail_err_m = debs_high_fail_mean - min(debs_high_failure)

    repa_perf_mean = sum(repa_completion) / len(repa_completion)
    repa_perf_err_p = max(repa_completion) - repa_perf_mean
    repa_perf_err_m = repa_perf_mean - min(repa_completion)
    repa_fail_mean = sum(repa_failure) / len(repa_failure)
    repa_fail_err_p = max(repa_failure) - repa_fail_mean
    repa_fail_err_m = repa_fail_mean - min(repa_failure)

    print("AdaptEnergy:", repa_completion, repa_failure)
    print(repa_perf_mean, max(repa_completion), min(repa_completion))
    print(sum(repa_failure) / len(repa_failure), max(repa_failure), min(repa_failure))
    print("Samoyed:", samoyed_completion, samoyed_failure)
    print(sum(samoyed_completion) / len(samoyed_completion), max(samoyed_completion), min(samoyed_completion))
    print(sum(samoyed_failure) / len(samoyed_failure), max(samoyed_failure), min(samoyed_failure))
    print("DEBS low:", debs_low_completion, debs_low_failure)
    print(sum(debs_low_completion) / len(debs_low_completion), max(debs_low_completion), min(debs_low_completion))
    print(sum(debs_low_failure) / len(debs_low_failure), max(debs_low_failure), min(debs_low_failure))
    print("DEBS high:", debs_high_completion, debs_high_failure)
    print(sum(debs_high_completion) / len(debs_high_completion), max(debs_high_completion), min(debs_high_completion))
    print(sum(debs_high_failure) / len(debs_high_failure), max(debs_high_failure), min(debs_high_failure))


    print("For csv:")
    print(debs_low_perf_mean, ",", debs_low_perf_err_p, ",", debs_low_perf_err_m, ",",\
samoyed_perf_mean, ",", samoyed_perf_err_p, ",", samoyed_perf_err_m, ",",\
debs_high_perf_mean, ",", debs_high_perf_err_p, ",", debs_high_perf_err_m, ",",\
repa_perf_mean, ",", repa_perf_err_p, ",", repa_perf_err_m, ",",\
debs_low_fail_mean, ",", debs_low_fail_err_p, ",", debs_low_fail_err_m, ",",\
samoyed_fail_mean, ",", samoyed_fail_err_p, ",", samoyed_fail_err_m, ",",\
debs_high_fail_mean, ",", debs_high_fail_err_p, ",", debs_high_fail_err_m, ",",\
repa_fail_mean, ",", repa_fail_err_p, ",", repa_fail_err_m)


if __name__ == "__main__":
    main_single()
    # main_multi()
# %%
