/*
 * SIMD.h
 *
 *  Created on: 08/01/2017
 *      Author: Lucas Teske
 */

#ifndef INCLUDES_SIMD_SIMD_H_
#define INCLUDES_SIMD_SIMD_H_

#if defined __GNUC__
#  define SIMD_ALIGNED(x) __attribute__((aligned(x)))
#elif _MSC_VER
#  define SIMD_ALIGNED(x) __declspec(align(x))
#else
#  define SIMD_ALIGNED(x)
#endif

#endif /* INCLUDES_SIMD_SIMD_H_ */
