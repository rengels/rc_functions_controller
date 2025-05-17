/** Tests for engine_speed.cpp
 *
 *  @file
 */

#include "proc.h"
#include "engine_speed.h"

#include <signals.h>
#include <gtest/gtest.h>

using namespace rcSignals;
using namespace rcEngine;


/** Unit test for rcEngine::Speed
 *
 *  Different scenarios for
 *  - rcEngine::Speed::step()
 *
 */
TEST(EngineSpeedTest, step) {

    rcSignals::RcSignal throttleSpeedLast;
    rcSignals::RcSignal throttleSpeed;
    rcEngine::Speed speed;

    speed.start();

    // -- speed going into the wrong direction
    speed.step(20, 50, 70, &throttleSpeedLast);
    speed.step(20, 40, 70, &throttleSpeed);
    EXPECT_LT(throttleSpeedLast, throttleSpeed);

    // -- speed going into the right direction
    speed.step(20, 500, 200, &throttleSpeedLast);
    speed.step(20, 480, 200, &throttleSpeed);
    EXPECT_EQ(throttleSpeedLast, throttleSpeed);

    // -- speed going into the right direction but too slow
    speed.step(20, 40, 800, &throttleSpeedLast);
    speed.step(20, 41, 800, &throttleSpeed);
    EXPECT_LT(throttleSpeedLast, throttleSpeed);

}
