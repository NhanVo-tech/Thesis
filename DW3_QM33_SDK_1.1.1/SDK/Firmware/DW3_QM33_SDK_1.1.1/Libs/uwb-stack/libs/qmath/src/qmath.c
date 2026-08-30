/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmath.h"

#define LUT_SIZE 33
#define LUT_SIZE_POW_BASE_2 32
#define LOG_PRECISION 5
#define LOG2_10_SHIFTED_100TH 109
#define DIVIDE_BY_POW2_ROUNDED(x, y) ((x + (1 << (y - 1))) >> y)

/*
 * lut_power_base_2 is a lut that computes ((2 ^ x) multiplied by 4)
 *  - x belongs to [0:1[ with a step of 0.0315.
 *  - There is no need for negative values: any value can be written as
 *    the sum of negative or positive int and a positive fractional parts.
 */
const uint8_t lut_power_base_2[LUT_SIZE_POW_BASE_2] = { 32, 33, 33, 34, 35, 36, 36, 37, 38, 39, 40,
							41, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
							52, 53, 54, 55, 56, 57, 59, 60, 61, 63 };

/*
 * lut_log2 is a lut that computes log2(x) shifted by LUT_LOG_SHIFT
 * with a step for x of 1/(2^LOG_PRECISION).
 */
const uint16_t lut_log2[LUT_SIZE] = { 0,     1455,  2866,  4236,  5568,	 6863,	8124,  9352,  10549,
				      11716, 12855, 13968, 15055, 16117, 17156, 18173, 19168, 20143,
				      21098, 22034, 22952, 23852, 24736, 25604, 26455, 27292, 28114,
				      28922, 29717, 30498, 31267, 32024, 32768 };

/**
 * log2_lut - Compute log2(x).
 * @x: x to convert in log2.
 *
 * Return: Return the log2(x) shifted by LUT_LOG_SHIFT.
 * 	   Results is shifted by LUT_LOG_SHIFT to gain
 * 	   precision and emulate the numbers after the decimal point.
 *
 * Algo here is to write x = y*2^z and to force y to be in the range of ]1,2].
 * Then we can write:
 *      log2(x)=log2(y*(2^z))
 *      log2(x)=log2(y) + log2(2^z)
 *      log2(x)=log2(y) + z
 *
 * To obtain y in the range of ]1,2] we can have it quickly because we know that
 * if we divide x with its msb we will find y.
 * Hence the use of __builtin_clz to find quickly the msb (named z in the algo).
 */

uint32_t log2_lut(uint32_t x)
{
	uint32_t log2_x = 0;
	uint64_t x_shifted = 0;
	uint16_t index = 0, y = 0, z = 0, left = 0;

	if (x == 1)
		/* log2(1) = 0.*/
		return 0;

	/* 31 xor clz(x) is equivalent but faster than 31 - clz(x).*/
	/* warning this is valid only because 31 is in binary 11111b.*/
	/* (31 is the size of uint32_t - 1 in bits) */
	z = 31 - __builtin_clz(x);
	x_shifted = ((uint64_t)x << LUT_LOG_SHIFT);
	y = DIVIDE_BY_POW2_ROUNDED(x_shifted, z);
	y -= 1 << LUT_LOG_SHIFT;
	index = y >> (LUT_LOG_SHIFT - LOG_PRECISION);
	left = y - (index << (LUT_LOG_SHIFT - LOG_PRECISION));
	log2_x = lut_log2[index] + (z << LUT_LOG_SHIFT);
	/* A good approximation is to add half the delta to the next index.*/
	/* To be done according to the granularity of the lut table.*/
	if (left >= (1 << (LUT_LOG_SHIFT - LOG_PRECISION - 2))) {
		log2_x += ((lut_log2[index + 1] - lut_log2[index]) >> 1);
	}
	return log2_x;
}

/**
 * log10_10 - Compute 10*log10(x).
 * @x: x to convert in 10*log10(x).
 *
 * Return: 10*log(x) in 100th dB
 *
 * log10(x) = log2(x)/log2(10)
 * As function log2_lut returns log2(x) << LUT_LOG_SHIFT we compute log2(10) <<
 * LUT_LOG_SHIFT Here log2(10) << LUT_LOG_SHIFT = 1088 As we want the result of
 * 10*log10(x) in 100th dB we divide only by 1088/10 = 109
 */
uint16_t log10_10(uint32_t x)
{
	/* log10(0) is not valid hence return an error.*/
	if (x == 0)
		return LOG_INVALID_VALUE;

	return (log2_lut(x) + (LOG2_10_SHIFTED_100TH >> 1)) / LOG2_10_SHIFTED_100TH;
}

uint32_t q8_pow_of_base2(int32_t exponent_q18)
{
	int16_t int_part = 0;
	int16_t frac_part = 0;
	uint32_t r1_q5, r2_q5;
	int32_t exponent_q5;

	exponent_q5 = (exponent_q18 + 4096) >> 13;
	/* Transform exponent to sum of int and frac parts. */
	int_part = exponent_q5 >> 5;
	frac_part = exponent_q5 & 0x1f;

	if (int_part >= 0) {
		r1_q5 = 32 << int_part;
	} else {
		/* int_part < 0 */
		r1_q5 = 32 >> (-int_part);
	}
	/* frac part */
	if (frac_part == 0)
		r2_q5 = 32;
	else
		r2_q5 = lut_power_base_2[frac_part];

	return (uint32_t)((r1_q5 * r2_q5) >> 2);
}
