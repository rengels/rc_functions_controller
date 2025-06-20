/** Power curve definitions.
 *
 * This file is auto generated by serialization_tool.py
 * 2025-06-06
 *
 * Do not modify.
 *
 * @file
 */

#include "power_curves.h"

namespace rcEngine {


const rcProc::Curve<4> powerCurveElectric{
    .points={
        rcProc::CurvePoint{0.00f, 1.00f},
        rcProc::CurvePoint{0.62f, 0.68f},
        rcProc::CurvePoint{0.76f, 0.23f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<6> powerCurvePetrolTwinTurbo{
    .points={
        rcProc::CurvePoint{-0.00f, 0.00f},
        rcProc::CurvePoint{0.13f, 0.22f},
        rcProc::CurvePoint{0.26f, 0.76f},
        rcProc::CurvePoint{0.73f, 1.00f},
        rcProc::CurvePoint{0.88f, 0.99f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<7> powerCurveDiesel{
    .points={
        rcProc::CurvePoint{-0.00f, 0.00f},
        rcProc::CurvePoint{0.28f, 0.37f},
        rcProc::CurvePoint{0.38f, 0.60f},
        rcProc::CurvePoint{0.50f, 0.92f},
        rcProc::CurvePoint{0.70f, 1.00f},
        rcProc::CurvePoint{0.92f, 0.98f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<8> powerCurvePetrol{
    .points={
        rcProc::CurvePoint{-0.00f, 0.00f},
        rcProc::CurvePoint{0.21f, 0.29f},
        rcProc::CurvePoint{0.30f, 0.66f},
        rcProc::CurvePoint{0.58f, 0.80f},
        rcProc::CurvePoint{0.67f, 0.93f},
        rcProc::CurvePoint{0.84f, 1.00f},
        rcProc::CurvePoint{0.93f, 0.94f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<7> powerCurvePetrolTurbo{
    .points={
        rcProc::CurvePoint{-0.00f, 0.00f},
        rcProc::CurvePoint{0.14f, 0.22f},
        rcProc::CurvePoint{0.24f, 0.65f},
        rcProc::CurvePoint{0.41f, 0.84f},
        rcProc::CurvePoint{0.72f, 1.00f},
        rcProc::CurvePoint{0.81f, 0.97f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<5> powerCurveMotorBrake{
    .points={
        rcProc::CurvePoint{0.00f, -0.91f},
        rcProc::CurvePoint{0.12f, -0.04f},
        rcProc::CurvePoint{0.23f, -0.04f},
        rcProc::CurvePoint{0.83f, -0.18f},
        rcProc::CurvePoint{1.02f, -0.34f}
    }};


const rcProc::Curve<5> powerCurveSteam{
    .points={
        rcProc::CurvePoint{0.00f, 0.46f},
        rcProc::CurvePoint{0.54f, 0.89f},
        rcProc::CurvePoint{0.80f, 1.00f},
        rcProc::CurvePoint{0.90f, 0.94f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


const rcProc::Curve<5> powerCurveTurbine{
    .points={
        rcProc::CurvePoint{-0.00f, 0.00f},
        rcProc::CurvePoint{0.37f, 0.21f},
        rcProc::CurvePoint{0.79f, 0.59f},
        rcProc::CurvePoint{0.99f, 1.00f},
        rcProc::CurvePoint{1.00f, 0.00f}
    }};


} // namespace


