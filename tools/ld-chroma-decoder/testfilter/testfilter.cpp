/************************************************************************

    testfilter.cpp

    ld-chroma-decoder - NTSC colourisation filter for ld-decode
    Copyright (C) 2014-5 Chad Page
    Copyright (C) 2019 Adam Sampson

    This file is part of ld-decode-tools.

    ld-chroma-decoder is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

using std::cerr;
using std::vector;

#include "deemp.h"

// This is the original filter code from ld-decoder.h.
class SimpleFilter {
protected:
    int order;
    vector<double> b, a;
    vector<double> y, x;

public:
    template <typename BSrc, typename ASrc>
    SimpleFilter(const BSrc &_b, const ASrc &_a)
        : b(_b.begin(), _b.end()), a(_a.begin(), _a.end())
    {
        order = b.size();

        x.resize(b.size() + 1);
        y.resize(a.size() + 1);

        clear();
    }

    void clear(double val = 0) {
        for (unsigned i = 0; i < a.size(); i++) {
            y[i] = val;
        }
        for (unsigned i = 0; i < b.size(); i++) {
            x[i] = val;
        }
    }

    inline double feed(double val) {
        double a0 = a[0];
        double y0;

        double *x_data = x.data();
        double *y_data = y.data();

        memmove(&x_data[1], x_data, sizeof(double) * (b.size() - 1));
        memmove(&y_data[1], y_data, sizeof(double) * (a.size() - 1));

        x[0] = val;
        y0 = 0;

        for (unsigned o = 0; o < b.size(); o++) {
            y0 += ((b[o] / a0) * x[o]);
        }
        for (unsigned o = 1; o < a.size(); o++) {
            y0 -= ((a[o] / a0) * y[o]);
        }

        y[0] = y0;
        return y[0];
    }
};

// Check that two filters produce the same output given the same input.
template <typename FN, typename FO>
void test_filter(const char *name, FN& fn, FO& fo) {
    cerr << "Comparing filters: " << name << "\n";

    for (int i = 0; i < 100; ++i) {
        double input = i - 40;
        double out_n = fn.feed(input);
        double out_o = fo.feed(input);
        if (fabs(out_n - out_o) > 0.000001) {
            cerr << "Mismatch on " << name << ": " << input << " -> " << out_n << ", " << out_o << "\n";
            exit(1);
        }
    }
}

int main() {
    // Test with the sets of coefficients used in the code.

    auto f0(f_colorlpi);
    SimpleFilter g0(c_colorlpi_b, c_colorlpi_a);
    test_filter("colorlpi", f0, g0);

    auto f1(f_colorlpq);
    SimpleFilter g1(c_colorlpq_b, c_colorlpq_a);
    test_filter("colorlpq", f1, g1);

    auto f2(f_nrc);
    SimpleFilter g2(c_nrc_b, c_nrc_a);
    test_filter("nrc", f2, g2);

    auto f3(f_nr);
    SimpleFilter g3(c_nr_b, c_nr_a);
    test_filter("nr", f3, g3);

    auto f4(f_a500_48k);
    SimpleFilter g4(c_a500_48k_b, c_a500_48k_a);
    test_filter("a500_48k", f4, g4);

    auto f5(f_a40h_48k);
    SimpleFilter g5(c_a40h_48k_b, c_a40h_48k_a);
    test_filter("a40h_48k", f5, g5);

    return 0;
}
