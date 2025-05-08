#pragma once

#include "assume.hpp"

#ifdef _MSC_VER
#define ADAPTED_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define ADAPTED_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#ifdef _MSC_VER
	#define FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
	#define FORCE_INLINE inline __attribute__((always_inline))
#else
	#define FORCE_INLINE inline
#endif

#ifdef _MSC_VER
#define RESTRICT __restrict
#else
#define RESTRICT
#endif

#define PURE_FUNCTION
