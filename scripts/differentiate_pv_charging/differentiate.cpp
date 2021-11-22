#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

#define CAPACITANCE     103e-6  // Farad
#define V_STEP          0.1     // Differentiate in 100mV step

int main() {
    fstream fs;
    fs.open("analog.csv");
    string num1, num2;
    double t, v;
    int v_unit = 0;
    double t_last = 0;
    double v_last = 0;
    double i;
    cout << std::fixed << std::setw(6) << std::setprecision(3) << std::setfill(' ');
    while (getline(fs, num1, ',') && getline(fs, num2)) {
        t = atof(num1.c_str());
        v = atof(num2.c_str());
        if (v >= v_unit * V_STEP + V_STEP / 2) {
            i = CAPACITANCE * (v - v_last) / (t - t_last);
            cout << v_unit * V_STEP << ", " << i * 1e6 << ", " << t - t_last << endl;
            t_last = t;
            v_last = v;
            ++v_unit;
        }
    }
}
