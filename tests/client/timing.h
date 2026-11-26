// SPDX-License-Identifier: MIT
// Short client timings for the in-process doubles; a suite changes what it
// needs on top of these.
#ifndef TESTS_TIMING_H
#define TESTS_TIMING_H

#include "client.h"

inline BluOS::Client::Timing fastTiming()
{
    BluOS::Client::Timing t;
    t.pollTimeoutSecs = 1;
    t.deadlineGraceMs = 2000;
    t.commandTimeoutMs = 3000;
    t.minIntervalMs = 300;
    t.backoffInitialMs = 200;
    t.backoffMaxMs = 400;
    return t;
}

#endif
