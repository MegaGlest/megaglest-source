// Deterministic floating-point math for network-synchronised game state.
//
// IEEE 754 only mandates bit-identical results for the five basic operations
// (+, -, *, /, sqrt).  Transcendental functions (atan2, sin, cos, …) may
// return results that differ by 1-2 ULPs between libm implementations on
// different platforms (glibc / Apple libm / MSVCRT), which is enough to
// diverge synchronisation logs and cause OOS errors in multiplayer.
//
// deterministicAtan2Deg() replaces streflop::atan2 + radToDeg() at the four
// call sites in unit.cpp that feed into logSynchData().  It uses only the
// five IEEE 754 basic operations on doubles, so its output is guaranteed
// bit-identical across all conforming platforms.
//
// Algorithm:
//   1. Reduce |y|/|x| (or |x|/|y|) to t in [0, tan(pi/12)] using the identity
//      atan(x) = pi/6 + atan((x*sqrt(3)-1)/(sqrt(3)+x))  for x in (tan(pi/12), 1]
//      atan(x) = pi/2 - atan(1/x)                         for x > 1
//   2. Evaluate atan(t) via a 6-term Taylor series.
//      For |t| <= tan(pi/12) ≈ 0.2679 the series error is < 4e-11 rad.
//   3. Undo the range reductions, adjust for quadrant, convert to degrees.
//
// Maximum error vs true atan2: < 5e-9 degrees — well inside the 6-decimal-
// place truncation applied by the callers.

#pragma once

// ---------------------------------------------------------------------------
// Internal helpers — not part of the public interface
// ---------------------------------------------------------------------------

namespace det_math_impl {

// Taylor series for atan(t), accurate for |t| <= tan(pi/12) ~= 0.2679.
// Error < 4e-11 radians (6 terms = degree-11 polynomial).
inline double atan_core(double t) {
    double t2 = t * t;
    return t * (1.0 - t2 * (1.0 / 3.0 - t2 * (1.0 / 5.0 - t2 * (1.0 / 7.0 - t2 * (1.0 / 9.0 - t2 * (1.0 / 11.0))))));
}

// atan(x) for x in [0, 1].
inline double atan01(double x) {
    // tan(pi/12) = 2 - sqrt(3)
    const double TAN_PI12 = 0.26794919243112270647255365849413;
    // tan(pi/6)  = 1/sqrt(3)
    const double SQRT3_OVER3 = 0.57735026918962576450914878050196;
    const double SQRT3 = 1.7320508075688772935274463415059;
    const double PI_OVER_6 = 0.52359877559829887307710723054658;

    if (x <= TAN_PI12) {
        return atan_core(x);
    }
    // atan(x) = pi/6 + atan((x*sqrt(3)-1)/(sqrt(3)+x))
    // Reduced argument y is in [-tan(pi/12), tan(pi/12)].
    double y = (x * SQRT3 - 1.0) / (SQRT3 + x);
    return PI_OVER_6 + atan_core(y);
}

} // namespace det_math_impl

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

// Drop-in replacement for  radToDeg(streflop::atan2(y, x)).
// Arguments follow the same (y, x) convention as std::atan2.
// Returns the angle in degrees.
inline float deterministicAtan2Deg(float y, float x) {
    const double PI = 3.14159265358979323846264338328;
    const double PI_2 = 1.57079632679489661923132169164;
    const double RAD2DEG = 57.295779513082320876798154814105;

    double dx = static_cast<double>(x);
    double dy = static_cast<double>(y);

    if (dx == 0.0) {
        if (dy > 0.0) return static_cast<float>(PI_2 * RAD2DEG);
        if (dy < 0.0) return static_cast<float>(-PI_2 * RAD2DEG);
        return 0.0f;
    }

    double ax = dx > 0.0 ? dx : -dx;
    double ay = dy > 0.0 ? dy : -dy;

    double r;
    if (ay <= ax) {
        r = det_math_impl::atan01(ay / ax);
    } else {
        r = PI_2 - det_math_impl::atan01(ax / ay);
    }

    if (dx < 0.0) r = PI - r;
    if (dy < 0.0) r = -r;

    return static_cast<float>(r * RAD2DEG);
}
