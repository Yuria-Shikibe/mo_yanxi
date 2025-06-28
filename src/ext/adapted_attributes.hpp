#pragma once

#include "assume.hpp"

#if defined(_MSC_VER)
#define ADAPTED_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define ADAPTED_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if  defined(__clang__)
	#define FORCE_INLINE [[gnu::always_inline]]
#elif _MSC_VER
	#define FORCE_INLINE [[msvc::forceinline]]
#else
	#define FORCE_INLINE
#endif

#if defined(__GNUC__) || defined(__clang__)
#define RESTRICT __restrict
#elifdef _MSC_VER
#define RESTRICT __restrict
#endif

#if defined(__GNUC__) || defined(__clang__)
	#define PURE_FN [[gnu::pure]]
#elif _MSC_VER
	#define PURE_FN
#else
	#define PURE_FN
#endif


#if defined(__GNUC__) || defined(__clang__)
	#define CONST_FN [[gnu::const]]
#elif _MSC_VER
	#define CONST_FN
#else
	#define CONST_FN
#endif

