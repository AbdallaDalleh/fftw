/*************************************************************************\
* Copyright (c) 2015 Brookhaven Science Assoc. as operator of
*     Brookhaven National Laboratory.
* Copyright (c) 2021 ITER Organization.
* This module is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *  Author: Ralph Lange <ralph.lange@gmx.de>
 *
 *  based on pscdrv/sigApp by Michael Davidsaver <mdavidsaver@ospreydcs.com>
 */

#include <cmath>
#include <algorithm>
#include <memory>
#include <string>
#include <cstring>

#include <epicsGuard.h>
#include <epicsMutex.h>
#include <epicsAssert.h>

#include "fftwCalc.h"

int FFTWDebug;

// global lock around FFTW planner
static epicsMutex fftwplanlock;

FFTWCalc::FFTWCalc()
    : wintype(None)
    , input_sz(0)
    , ntime(0)
    , nfreq(0)
    , fsamp(0.0)
    , redo_plan(true)
    , newval(true)
{}

FFTWCalc::~FFTWCalc() {}

void
FFTWCalc::set_fsamp(double f)
{
    bool changed = fsamp != f;
    if (changed) {
        fsamp = f;
        redo_plan = true; // strictly speaking not needed
    }
}

void
FFTWCalc::set_wtype(FFTWCalc::WindowType type)
{
    bool changed = wintype != type;
    if (changed) {
        wintype = type;
        redo_plan = true; // strictly speaking not needed
    }
}

void
FFTWCalc::set_input_real(std::unique_ptr<std::vector<double, FFTWAllocator<double>>> inp)
{
    input = std::move(inp);
    newval = true;

    // number of time samples
    ntime = input->size();
    // number of frequency samples
    nfreq = ntime / 2 + 1;

    assert(ntime > 0);
    assert(nfreq > 0);

    if (input_sz != input->size()) {
        redo_plan = true;
        input_sz = input->size();
    }
}

static const double PI = 3.141592653589793;

bool
FFTWCalc::apply_window()
{
    bool window_changed = false;
    WindowType wt = wintype;

    if (redo_plan) {
        window_changed = true;
        window.resize(ntime);
        // Can't apply window function with array size 1
        if (ntime <= 1)
            wt = None;
        switch (wt) {
        case Hann: {
            // Hann window
            double fact = PI / (ntime - 1);
            for (size_t n = 0, N = window.size(); n < N; n++) {
                double temp = sin(fact * n);
                window[n] = temp * temp;
            }
            break;
        }
        case None:
        default:
            std::fill(window.begin(), window.end(), 1.0);
        }
    }

    if (newval) {
        // optimization.  Don't use operator[] in a tight loop, it doesn't always get inline'd
        double *inp = input->data();
        double *win = window.data();

        const size_t N = input->size();

        for (size_t i = 0; i < N; i++)
            inp[i] *= win[i];

        newval = false;
    }

    return window_changed;
}

bool
FFTWCalc::replan()
{
    bool fscale_changed = false;

    if (redo_plan) {
        // reallocate
        plan.clear(); // free existing plan
        output.resize(nfreq);

        // re-do frequency scale
        fscale_changed = true;
        fscale.resize(nfreq);
        double mult = fsamp / ntime;
        for (size_t i = 0; i < fscale.size(); i++)
            fscale[i] = i * mult;

        epicsGuard<epicsMutex> pg(fftwplanlock);

        // use a junk buffer as planning would overwrite the input
        std::unique_ptr<std::vector<double, FFTWAllocator<double>>> buf(new std::vector<double, FFTWAllocator<double>>());
        buf->reserve(input->size());

        // FFTW_EXHAUSTIVE > FFTW_PATIENT > FFTW_MEASURE > FFTW_ESTIMATE
        plan = fftw_plan_dft_r2c_1d(ntime, buf->data(), output.data(), FFTW_MEASURE);

        redo_plan = false;
    }
    return fscale_changed;
}

void
FFTWCalc::transform()
{
    fftw_execute_dft_r2c(plan.get(), input->data(), output.data());
}

#include <epicsExport.h>

extern "C" {
epicsExportAddress(int, FFTWDebug);
}
