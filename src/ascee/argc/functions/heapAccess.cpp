// Copyright (c) 2021-2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
// reserved. This file is part of the C++ implementation of the Argennon smart
// contract Execution Environment (AscEE).
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

#include <executor/Executor.h>
#include "argc/types.h"
#include "argc/functions.h"

using namespace argennon;
using namespace argennon::ascee::runtime;
using namespace ascee;

// we don't want to call unGuard() when an exception is thrown.
int64 argc::load_int64(int32 offset) {
    Executor::guardArea();
    auto ret = Executor::getSession()->heapModifier.load<int64>(offset);
    Executor::unGuard();
    return ret;
}

bool argc::invalid(int32 offset, int32 size) {
    Executor::guardArea();
    auto ret = !Executor::getSession()->heapModifier.isValid(offset, size);
    Executor::unGuard();
    return ret;
}

void argc::load_chunk_long(long_id id) {
    Executor::getSession()->heapModifier.loadChunk(id);
}

void argc::store_int64(int32 offset, int64 value) {
    Executor::guardArea();
    Executor::getSession()->heapModifier.store<int64>(offset, value);
    Executor::unGuard();
}

void argc::add_int64_to(int32 offset, int64 amount) {
    Executor::guardArea();
    Executor::getSession()->heapModifier.addInt<int64>(offset, amount);
    Executor::unGuard();
}

void argc::resize_chunk(int32 new_size) {
    Executor::guardArea();
    Executor::getSession()->heapModifier.updateChunkSize(new_size);
    Executor::unGuard();
}

