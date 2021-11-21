// Copyright (c) 2021 aybehrouz <behrouz_ayati@yahoo.com>. All rights reserved.
// This file is part of the C++ implementation of the Argennon smart contract
// Execution Environment (AscEE).
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU Affero General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option) any
// later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License
// for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include <argc/types.h>
#include <argc/functions.h>

#define MAX_ADDITION_LOSS 0x100000

using namespace ascee;
using namespace ascee::runtime;

static
int extractExp(float64 f) {
    return int(*(uint64_t*) &f >> 52 & 0x7ff);
}

/**
 * Adds two 64 bit floating point numbers and raises SIGFPE if the fractional
 * error for the smaller number is bigger than MAX_ADDITION_LOSS * 2^(-52).
 * @param a
 * @param b
 * @return
 */
float64 argc::safe_addf64(float64 a, float64 b) {
    auto expDiff = extractExp(a) - extractExp(b);

    if (expDiff > 0) {
        if ((~(UINT64_MAX << expDiff) & *(uint64_t*) &b) > MAX_ADDITION_LOSS) {
            throw std::underflow_error("safe_add64: precision loss is too large");
        }
    } else if (expDiff < 0) {
        if ((~(UINT64_MAX << -expDiff) & *(uint64_t*) &a) > MAX_ADDITION_LOSS) {
            throw std::underflow_error("safe_add64: precision loss is too large");
        }
    }
    return a + b;
}

/**
 * Zeros all bits in @p f that have a significance lower than 2^(-n). As a result,
 * we always have: | truncate_float64(f, n) - f | < 2^(-n)
 * In other words, n bits in the fraction part will be kept.
 * @param f
 * @param n
 * @return
 */
float64 argc::truncate_float64(float64 f, int n) {
    auto shift = (52 - n) - (extractExp(f) - 1023);
    //auto shift = (52 + 1023) - (extractExp(f) + n);
    if (shift <= 0) return f;
    if (shift > 52) return 0;
    uint64_t ret = *(uint64_t*) &f & (UINT64_MAX << shift);
    return *(float64*) &ret;
}
