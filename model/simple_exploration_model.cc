// Copyright (c) 2020, University of Southampton.
// All rights reserved.
// SPDX-License-Identifier: BSD-3-Clause


// This code is for comparing two designs at an early stage.
// Design 1: Calibrate & Model
// Design 2: V_end Adaptation

#include <iostream>
#include <fstream>

using namespace std;

// void NormalOperation() {
//     double task = 10e-3;   // 10ms
//     // example in your ENSsys20 paper: 26.2ms

//     double sample_temp = 170e-6;    // us
//     double sample_vend = 145e-6;    // us

//     cout << "Normal Operation Overheads:" << endl;
//     cout << "Design 1: " << sample_temp / task * 100 << '%' << endl;
//     cout << "Design 2: " << sample_vend / task * 100 << '%' << endl;
// }

// void Adaptation() {
//     double calibration = 100e-3;    // ms
//     double inc_dec_threshold = 10e-6;   // us

//     cout << endl;
// }

// void Hardware() {
// }

int main() {
    // NormalOperation();

    double calibration_extra_charging = 100e-3;    // ms
    double inc_dec_threshold = 10e-6;   // us

    double freq_adaptation = 0.1;  // once every 10s
    double current_in = 0.2e-3;
    double current_out = 1e-3;
    double current_hw_design2 = 10e-6;

    double c_v = 100e-6 * 0.5;  // 100uF, 0.5V
    double t_on;
    double t_off;

    double duty_cycle;

    ofstream myfile;
    myfile.open("result.txt");

    do {
        // Design 1
        t_on = c_v / (current_out - current_in);
        t_off = c_v / current_in;
        duty_cycle = t_on /
            (t_on + t_off + freq_adaptation * calibration_extra_charging);
        // cout << "Design 1: " << duty_cycle << endl;
        myfile << duty_cycle << ", ";

        for (int i = 0; i < 5; i++) {
            // Design 2
            current_hw_design2 = 10e-6 * (1 + i);

            t_on = c_v / (current_out + current_hw_design2 - current_in);
            t_off = c_v / current_in;
            duty_cycle = t_on /
                (t_on + t_off + freq_adaptation * inc_dec_threshold);
            // cout << "Design 2: " << duty_cycle << endl;
            if (i == 4)
                myfile << duty_cycle << endl;
            else
                myfile << duty_cycle << ", ";
        }
        // myfile << endl;
        freq_adaptation /= 10;
    } while (freq_adaptation >= 0.0001);

    myfile.close();

    return 0;
}
