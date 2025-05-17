/** Tests for engine_idle.cpp
 *
 *  @file
 */

#include "proc.h"
#include "engine_idle.h"

#include <signals.h>
#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcEngine;


/** Unit test for rcEngine::Idle
 *
 *  Different scenarios for
 *  - rcEngine::Idle::step()
 *
 */
TEST(EngineIdleTest, step) {

    rcSignals::RcSignal throttleIdleLast;
    rcSignals::RcSignal throttleIdle;
    rcSignals::RcSignal loadEngine;
    rcEngine::Idle idle = rcEngine::Idle(100, 100, 200, 100, 10);

    idle.start();

    // -- getRPM
    EXPECT_EQ(100, idle.getRPM());

    // -- throttle going into the wrong direction
    idle.step(20, 100, 0, &throttleIdleLast, &loadEngine);

    idle.step(20, 120, 0, &throttleIdle, &loadEngine);
    EXPECT_GT(throttleIdleLast, throttleIdle);
    EXPECT_EQ(120, loadEngine);

    // -- throttle going into the right direction
    idle.step(20, 140, 0, &throttleIdleLast, &loadEngine);
    idle.step(20, 120, 0, &throttleIdle, &loadEngine);
    EXPECT_EQ(throttleIdleLast, throttleIdle);

    // -- throttle going into the right direction but too slow
    idle.step(20, 80, 0, &throttleIdleLast, &loadEngine);
    idle.step(20, 81, 0, &throttleIdle, &loadEngine);
    EXPECT_LT(throttleIdleLast, throttleIdle);

}
