/** Tests for the curve classes */

#include "curve.h"
#include <gtest/gtest.h>

namespace rcProc {

/** Test Curve::map
 *
 */
TEST(CurveTest, LinearCurve) {

    Curve<2> curve{
        .points={
            CurvePoint{1.0f, 2.0f},
            CurvePoint{2.0f, 4.0f}
        }};

    // normal mapping
    EXPECT_EQ(2.0, curve.map(1.0f));
    EXPECT_EQ(3.0, curve.map(1.5f));
    EXPECT_EQ(4.0, curve.map(2.0f));

    // mapping outside the curve
    EXPECT_EQ(2.0, curve.map(0.0f));
    EXPECT_EQ(4.0, curve.map(3.0f));
}

} // namespace
