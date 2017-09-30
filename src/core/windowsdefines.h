#pragma once
#ifdef _WIN32
#define NO_STDIO_REDIRECT
#include <algorithm>
#else
#include <math.h>
#include <cstdint>
#include <malloc.h>
#endif

#ifdef _WIN32
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
#endif
