#!/usr/local/bin/python3
# %%

import matplotlib.pyplot as plt
import random

class AdaptEnergy:
    c = 10e-6  # 10uF

    i_draw_off = 0  # Constant
    i_draw_lpm = 20e-6  # Constant
    i_draw_exe_const = 750e-6  # Constant
    i_draw_exe = i_draw_exe_const
    i_draw = i_draw_off

    t_atom = 5e-3  # Constant
    # =========================
    # Set dynamic execution time here!!!
    t_atom_max_us = 5000
    t_atom_random = t_atom_max_us * 1e-6
    # =========================
    t_remain = 0

    v_off = 1.8  # Constant
    # =========================
    # Set dynamic threshold here!!!
    v_target_end = 1.8
    v_exe = v_target_end + i_draw_exe * t_atom_random / c
    # =========================
    # v_exe = 2.6

    vcc = 0

    state = 0
    # state can be
    # 0: off, i_draw = i_draw_off
    # 1: low power mode (sleep), i_draw = i_draw_lpm
    # 2: executing atomic operation, i_draw = i_draw_exe

    n_completion = 0
    n_failure = 0

    rd_nums = []
    rd_index = 0
    rd_nums_len = 0

    rd_nums_i = []
    rd_index_i = 0
    rd_nums_len_i = 0

    def __init__(self, vcc_init, arr, arr2) -> None:
        self.vcc = vcc_init
        # Set up random numbers for execution time
        if len(arr) == 0:
            self.rd_nums = [self.t_atom_max_us, self.t_atom_max_us]
        else:
            self.rd_nums = arr
        self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
        self.t_remain = self.t_atom_random
        self.rd_index += 1
        self.rd_nums_len = len(self.rd_nums)
        
        # Set up random numbers for current draw
        if len(arr2) == 0:
            self.rd_nums_i = [1.0, 1.0]
        else:
            self.rd_nums_i = arr2
        self.rd_nums_len_i = len(self.rd_nums_i)
        self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
        self.rd_index_i += 1

        # Set up threshold
        self.v_exe = self.v_target_end + self.i_draw_exe * self.t_atom_random / self.c

    def update(self, i_in, t_step):
        self.update_state(t_step)
        # Uncomment the following 3 lines to cut off supply when executing
        # (Useful for checking whether v_exe is adapted correctly)
        # if self.state == 2:
        #     self.update_vcc(0, t_step)
        # else:
        self.update_vcc(i_in, t_step)
        return self.vcc
    
    def update_state(self, t_step):
        # off
        if self.state == 0:
            # self.i_draw = self.i_draw_off
            if self.vcc > self.v_off:
                self.state = 1
                self.i_draw = self.i_draw_lpm
        # lpm
        elif self.state == 1:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_exe:
                self.state = 2
                self.i_draw = self.i_draw_exe
            elif self.vcc < self.v_off:
                self.state = 0
                self.i_draw = self.i_draw_off
        
        # exe
        elif self.state == 2:
            # self.i_draw = self.i_draw_exe
            if self.vcc < self.v_off:  # Power fails
                self.state = 0
                self.i_draw = self.i_draw_off
                self.n_failure += 1

            elif self.t_remain < 0:  # Finish operation, go sleep
                self.state = 1
                self.i_draw = self.i_draw_lpm

                # Output a completion here
                self.n_completion += 1

                # Set up the next operation
                # Renew a random value for t_atom_random
                # self.t_atom_random = random.randint(0, self.t_atom_max_us) * 1e-6
                self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
                # self.t_remain = self.t_atom  # Fixed execution time
                self.t_remain = self.t_atom_random  # Randomized execution time
                self.rd_index += 1
                if self.rd_index >= self.rd_nums_len:
                    self.rd_index = 0

                # Set the new current draw
                self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
                self.rd_index_i += 1
                if self.rd_index_i >= self.rd_nums_len_i:
                    self.rd_index_i = 0

                # Set the next dynamic threshold v_exe
                self.v_exe = self.v_target_end + self.i_draw_exe * self.t_atom_random / self.c

            else:  # Still executing
                self.t_remain = self.t_remain - t_step

    def update_vcc(self, i_in, t_step):
        self.vcc = self.vcc + (i_in - self.i_draw) * t_step / self.c

class Samoyed:
    c = 10e-6  # 10uF

    i_draw_off = 0  # Constant
    i_draw_lpm = 20e-6  # Constant
    i_draw_exe_const = 750e-6  # Constant
    i_draw_exe = i_draw_exe_const
    i_draw = i_draw_off

    t_atom = 5e-3  # Constant
    # =========================
    # Set dynamic execution time here!!!
    t_atom_max_us = 5000
    t_atom_random = t_atom_max_us * 1e-6
    # =========================
    t_remain = 0

    v_off = 1.8  # Constant
    # =========================
    # Fixed threshold for the largest atomic operation
    v_target_end = 1.8
    v_exe = v_target_end + i_draw_exe * t_atom / c
    # =========================

    vcc = 0

    state = 0
    # state can be
    # 0: off, i_draw = i_draw_off
    # 1: low power mode (sleep), i_draw = i_draw_lpm
    # 2: executing atomic operation, i_draw = i_draw_exe

    n_completion = 0
    n_failure = 0

    rd_nums = []
    rd_index = 0
    rd_nums_len = 0

    rd_nums_i = []
    rd_index_i = 0
    rd_nums_len_i = 0

    def __init__(self, vcc_init, arr, arr2) -> None:
        self.vcc = vcc_init
        # Set up random numbers
        if len(arr) == 0:
            self.rd_nums = [self.t_atom_max_us, self.t_atom_max_us]
        else:
            self.rd_nums = arr
        self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
        self.t_remain = self.t_atom_random
        self.rd_index += 1
        self.rd_nums_len = len(self.rd_nums)

        # Set up random numbers for current draw
        if len(arr2) == 0:
            self.rd_nums_i = [1.0, 1.0]
        else:
            self.rd_nums_i = arr2
        self.rd_nums_len_i = len(self.rd_nums_i)
        self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
        self.rd_index_i += 1

    def update(self, i_in, t_step):
        self.update_state(t_step)
        # Uncomment the following 3 lines to cut off supply when executing
        # (Useful for checking whether v_exe is adapted correctly)
        # if self.state == 2:
        #     self.update_vcc(0, t_step)
        # else:
        self.update_vcc(i_in, t_step)
        return self.vcc
    
    def update_state(self, t_step):
        # off
        if self.state == 0:
            # self.i_draw = self.i_draw_off
            if self.vcc > self.v_off:
                self.state = 1
                self.i_draw = self.i_draw_lpm
        # lpm
        elif self.state == 1:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_exe:
                self.state = 2
                self.i_draw = self.i_draw_exe
            elif self.vcc < self.v_off:
                self.state = 0
                self.i_draw = self.i_draw_off
        
        # exe
        elif self.state == 2:
            # self.i_draw = self.i_draw_exe
            if self.vcc < self.v_off:  # Power fails
                self.state = 0
                self.i_draw = self.i_draw_off
                self.t_remain = self.t_atom_random  # Restart the operation upon power recovery
                # Set the new current draw
                self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
                self.rd_index_i += 1
                if self.rd_index_i >= self.rd_nums_len_i:
                    self.rd_index_i = 0
                self.n_failure += 1

            elif self.t_remain < 0:  # Finish operation, keep executing until energy depleted
                # Output a completion here
                self.n_completion += 1
                # Set up the next operation
                # Renew a random value for t_atom_random
                # self.t_remain = self.t_atom  # Fixed execution time
                self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
                self.t_remain = self.t_atom_random  # Randomized execution time
                self.rd_index += 1
                if self.rd_index >= self.rd_nums_len:
                    self.rd_index = 0

                # Set the new current draw
                self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
                self.rd_index_i += 1
                if self.rd_index_i >= self.rd_nums_len_i:
                    self.rd_index_i = 0
                self.i_draw = self.i_draw_exe
                
            else:  # Still executing
                self.t_remain = self.t_remain - t_step

    def update_vcc(self, i_in, t_step):
        self.vcc = self.vcc + (i_in - self.i_draw) * t_step / self.c

class DEBS:
    c = 10e-6  # 10uF

    i_draw_off = 0  # Constant
    i_draw_lpm = 20e-6  # Constant
    i_draw_exe_const = 750e-6  # Constant
    i_draw_exe = i_draw_exe_const
    i_draw = i_draw_off

    t_atom = 5e-3  # Constant
    # =========================
    # Fixed threshold for 
    t_atom_max_us = 5000
    t_atom_random = t_atom_max_us * 1e-6
    # =========================
    t_remain = 0

    v_off = 1.8  # Constant
    # =========================
    # Fixed threshold for the largest atomic operation
    v_target_end = 1.8
    v_exe = v_target_end + i_draw_exe * t_atom / c
    # =========================
    # v_exe = 2.6

    vcc = 0

    state = 0
    # state can be
    # 0: off, i_draw = i_draw_off
    # 1: low power mode (sleep), i_draw = i_draw_lpm
    # 2: executing atomic operation, i_draw = i_draw_exe

    n_completion = 0
    n_failure = 0

    rd_nums = []
    rd_index = 0
    rd_nums_len = 0

    rd_nums_i = []
    rd_index_i = 0
    rd_nums_len_i = 0

    def __init__(self, vcc_init, arr, arr2) -> None:
        self.vcc = vcc_init
        # Set up random numbers
        if len(arr) == 0:
            self.rd_nums = [self.t_atom_max_us, self.t_atom_max_us]
        else:
            self.rd_nums = arr
        self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
        self.t_remain = self.t_atom_random
        self.rd_index += 1
        self.rd_nums_len = len(self.rd_nums)

        # Set up random numbers for current draw
        if len(arr2) == 0:
            self.rd_nums_i = [1.0, 1.0]
        else:
            self.rd_nums_i = arr2
        self.rd_nums_len_i = len(self.rd_nums_i)
        self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
        self.rd_index_i += 1

    def update(self, i_in, t_step):
        self.update_state(t_step)
        # Uncomment the following 3 lines to cut off supply when executing
        # (Useful for checking whether v_exe is adapted correctly)
        # if self.state == 2:
        #     self.update_vcc(0, t_step)
        # else:
        self.update_vcc(i_in, t_step)
        return self.vcc
    
    def update_state(self, t_step):
        # off
        if self.state == 0:
            # self.i_draw = self.i_draw_off
            if self.vcc > self.v_off:
                self.state = 1
                self.i_draw = self.i_draw_lpm
        # lpm
        elif self.state == 1:
            # self.i_draw = self.i_draw_lpm
            if self.vcc > self.v_exe:
                self.state = 2
                self.i_draw = self.i_draw_exe
            elif self.vcc < self.v_off:
                self.state = 0
                self.i_draw = self.i_draw_off
        
        # exe
        elif self.state == 2:
            # self.i_draw = self.i_draw_exe
            
            if self.vcc < self.v_off:  # Power fails
                self.state = 0
                self.i_draw = self.i_draw_off
                self.t_remain = self.t_atom_random  # Restart if it fails to complete (which should never happen)
                # Set the new current draw
                self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
                self.rd_index_i += 1
                if self.rd_index_i >= self.rd_nums_len_i:
                    self.rd_index_i = 0
                self.n_failure += 1

            elif self.t_remain < 0:  # Finish operation, go sleep
                self.state = 1
                self.i_draw = self.i_draw_lpm

                # Output a completion here
                self.n_completion += 1

                # Set up the next operation
                # Renew a random value for t_atom_random
                # self.t_remain = self.t_atom  # Fixed execution time
                self.t_atom_random = self.rd_nums[self.rd_index] * 1e-6
                self.t_remain = self.t_atom_random  # Randomized execution time
                self.rd_index += 1
                if self.rd_index >= self.rd_nums_len:
                    self.rd_index = 0

                # Set the new current draw
                self.i_draw_exe = self.i_draw_exe_const * self.rd_nums_i[self.rd_index_i]
                self.rd_index_i += 1
                if self.rd_index_i >= self.rd_nums_len_i:
                    self.rd_index_i = 0
                
            else:  # Still executing
                self.t_remain = self.t_remain - t_step

    def update_vcc(self, i_in, t_step):
        self.vcc = self.vcc + (i_in - self.i_draw) * t_step / self.c

# Single test main
def main():
    # Generate random numbers
    # Array 1 for randomized execution time
    random_arr1 = []
    # One thousand integer numbers in [0, 5000]
    for i in range(1000):  
        random_arr1.append(random.randint(100, 5000))
    # Array 2 for randomized current draw
    random_arr2 = []
    # One thousand float numbers in [1.0, 1.2]
    # for i in range(1000):  
    #     random_arr2.append(random.uniform(1.0, 1.2))  

    vcc_init = 1.5
    i_in = 100e-6
    t_step = 1e-6
    t = 0
    vcc_ae = vcc_init
    vcc_samoyed = vcc_init
    vcc_debs = vcc_init

    t_trace = []
    v_trace_ae = []
    v_trace_samoyed = []
    v_trace_debs = []
    t_trace.append(t)
    v_trace_ae.append(vcc_ae)
    v_trace_samoyed.append(vcc_samoyed)
    v_trace_debs.append(vcc_debs)

    ae_obj = AdaptEnergy(vcc_ae, random_arr1, random_arr2)
    samoyed_obj = Samoyed(vcc_samoyed, random_arr1, random_arr2)
    debs_obj = DEBS(vcc_debs, random_arr1, random_arr2)

    while t < 2:  # Simulation time (seconds)
        vcc_ae = ae_obj.update(i_in, t_step)
        vcc_samoyed = samoyed_obj.update(i_in, t_step)
        vcc_debs = debs_obj.update(i_in, t_step)
        t += t_step
        t_trace.append(t)
        v_trace_ae.append(vcc_ae)
        v_trace_samoyed.append(vcc_samoyed)
        v_trace_debs.append(vcc_debs)


    print("Number of completions, failures")
    print("AdaptEnergy:", ae_obj.n_completion, ae_obj.n_failure)
    plt.plot(t_trace, v_trace_ae)
    plt.show()

    print("Samoyed:", samoyed_obj.n_completion, samoyed_obj.n_failure)
    plt.plot(t_trace, v_trace_samoyed)
    plt.show()

    print("DEBS: ", debs_obj.n_completion, debs_obj.n_failure)
    plt.plot(t_trace, v_trace_debs)
    plt.show()

# # Multi tests main
# def main():
#     ae_completion =[]
#     ae_failure = []
#     samoyed_completion = []
#     samoyed_failure = []
#     debs_completion = []
#     debs_failure = []

#     # print("Number of completions, failures")
#     for loop_count in range(10):
#         # Generate random numbers
#         # Array 1 for randomized execution time
#         random_arr1 = []
#         # One thousand integer numbers in [0, 5000]
#         # for i in range(1000):  
#         #     random_arr1.append(random.randint(0, 5000))
#         # Array 2 for randomized current draw
#         random_arr2 = []
#         # One thousand float numbers in [1.0, 1.2]
#         for i in range(1000):  
#             random_arr2.append(random.uniform(1.0, 1.2))  

#         vcc_init = 1.5
#         i_in = 500e-6
#         t_step = 1e-6
#         t = 0
#         vcc_ae = vcc_init
#         vcc_samoyed = vcc_init
#         vcc_debs = vcc_init

#         ae_obj = AdaptEnergy(vcc_ae, random_arr1, random_arr2)
#         samoyed_obj = Samoyed(vcc_samoyed, random_arr1, random_arr2)
#         debs_obj = DEBS(vcc_debs, random_arr1, random_arr2)

#         while t < 2:  # Simulation time (seconds)
#             vcc_ae = ae_obj.update(i_in, t_step)
#             vcc_samoyed = samoyed_obj.update(i_in, t_step)
#             vcc_debs = debs_obj.update(i_in, t_step)
#             t += t_step

#         # print("Round", loop_count, ":")
#         # print("  AdaptEnergy:", ae_obj.n_completion, ae_obj.n_failure)
#         # print("  Samoyed:", samoyed_obj.n_completion, samoyed_obj.n_failure)
#         # print("  DEBS: ", debs_obj.n_completion, debs_obj.n_failure)
#         print("Round", loop_count, "completed.")

#         ae_completion.append(ae_obj.n_completion)
#         ae_failure.append(ae_obj.n_failure)
#         samoyed_completion.append(samoyed_obj.n_completion)
#         samoyed_failure.append(samoyed_obj.n_failure)
#         debs_completion.append(debs_obj.n_completion)
#         debs_failure.append(debs_obj.n_failure)

#         del ae_obj
#         del samoyed_obj
#         del debs_obj

#     print(" ")
#     print("[Number of completions], [Number of failures]")
#     print("(Number of completions) Mean, Max, Min")
#     print("(Number of failures) Mean, Max, Min")
#     print(" ")


#     print("AdaptEnergy:", ae_completion, ae_failure)
#     print(sum(ae_completion) / len(ae_completion), max(ae_completion), min(ae_completion))
#     print(sum(ae_failure) / len(ae_failure), max(ae_failure), min(ae_failure))
#     print("Samoyed:", samoyed_completion, samoyed_failure)
#     print(sum(samoyed_completion) / len(samoyed_completion), max(samoyed_completion), min(samoyed_completion))
#     print(sum(samoyed_failure) / len(samoyed_failure), max(samoyed_failure), min(samoyed_failure))
#     print("DEBS:", debs_completion, debs_failure)
#     print(sum(debs_completion) / len(debs_completion), max(debs_completion), min(debs_completion))
#     print(sum(debs_failure) / len(debs_failure), max(debs_failure), min(debs_failure))

if __name__ == "__main__":
    main()

# %%
for i in range(5):
    print(i)

# %%