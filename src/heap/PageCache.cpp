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

#include "PageCache.h"
#include "Chunk.h"

using namespace ascee;
using namespace ascee::runtime::heap;

static
bool isLittleEndian() {
    byte buf[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    int128 x = *(int128*) buf;
    auto xl = int64(x);
    auto xh = int64(x >> 64);
    if (xl != 0x706050403020100) return false;
    if (xh != 0xf0e0d0c0b0a0908) return false;
    return true;
}

PageCache::PageCache(runtime::PageLoader& loader) : loader(loader) {
    if (!isLittleEndian()) throw std::runtime_error("platform not supported");
}