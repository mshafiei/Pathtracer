#if !defined(UTIL_H)
#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#else
#include <cmath>
#endif

inline float Radians(float deg)
{
    return ((float)M_PI/180.f) * deg;
}

inline float Degrees(float rad)
{
    return (180.f/(float)M_PI) * rad;
}    

#define UTIL_H
#endif
