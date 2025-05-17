/**
 *  This file contains the rc::Curves class
 *
 *  @file
*/

#ifndef _RC_CURVE_H_
#define _RC_CURVE_H_

#include <array>

namespace rcProc {

/** A point on the curve */
struct CurvePoint {
    float in;
    float out;
};

template<std::size_t N> class Curve {

public:
    /** Points defining a polygon line.
     *
     *  The points have to be sorted.
     */
    std::array<CurvePoint, N> points;

public:
    /** Map the input value via the curve to an output signal.
     *
     * Input values outside the curve are mapped to the first/last point depending.
     */
    float map(float in) const {
        static_assert(N >= 2, "curve needs at least two points");

        if (in < points.front().in) {
            return points.front().out;
        }

        for (std::size_t i = 1; i < points.size(); i++) {
            if (in <= points[i].in) {
                float inDelta = points[i].in - points[i - 1].in;
                float outDelta = points[i].out - points[i - 1].out;
                return points[i - 1].out +
                    (in - points[i - 1].in) * outDelta / inDelta;
            }
        }

        return points.back().out;
    }
};


} // namespace

#endif // _RC_CURVE_H_

