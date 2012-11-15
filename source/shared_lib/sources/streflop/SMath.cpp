/*
    Copyright 2006 Nicolas Brodu
    		  2012 Mark Vejvoda

	You can redistribute this code and/or modify it under
	the terms of the GNU General Public License as published
	by the Free Software Foundation; either version 2 of the
	License, or (at your option) any later version
*/

// Includes Math.h in turn
#include "streflop.h"

namespace streflop {

    // Constants

    const Simple SimpleZero(0.0f);
    const Simple SimplePositiveInfinity = Simple(1.0f) / SimpleZero;
    const Simple SimpleNegativeInfinity = Simple(-1.0f) / SimpleZero;
    // non-signaling version
    const Simple SimpleNaN = SimplePositiveInfinity + SimpleNegativeInfinity;

    const Double DoubleZero(0.0f);
    const Double DoublePositiveInfinity = Double(1.0f) / DoubleZero;
    const Double DoubleNegativeInfinity = Double(-1.0f) / DoubleZero;
    // non-signaling version
    const Double DoubleNaN = DoublePositiveInfinity + DoubleNegativeInfinity;

// Extended are not always available
#ifdef Extended

    const Extended ExtendedZero(0.0f);
    const Extended ExtendedPositiveInfinity = Extended(1.0f) / ExtendedZero;
    const Extended ExtendedNegativeInfinity = Extended(-1.0f) / ExtendedZero;
    // non-signaling version
    const Extended ExtendedNaN = ExtendedPositiveInfinity + ExtendedNegativeInfinity;

#endif



    // Default environment. Initalized to 0, and really set on first access
#if defined(STREFLOP_X87)
    fenv_t FE_DFL_ENV = 0;
#elif defined(STREFLOP_SSE)
    fenv_t FE_DFL_ENV = {0,0};
#elif defined(STREFLOP_SOFT)
    fenv_t FE_DFL_ENV = {42,0,0};
#else
#error STREFLOP: Invalid combination or unknown FPU type.
#endif

}
