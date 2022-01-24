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

int64 argc::load_int64(int32 offset) {
    try {
        return Executor::getSession()->heapModifier.load<int64>(offset);
    } catch (const std::out_of_range& err) {
        throw ascee::AsceeError(err.what());
    }
}

bool argc::invalid(int32 offset, int32 size) {
    try {
        return Executor::getSession()->heapModifier.isValid(offset, size);
    } catch (const std::out_of_range& err) {
        throw ascee::AsceeError(err.what());
    }
}

void argc::load_chunk_long(long_id id) {
    try {
        Executor::getSession()->heapModifier.loadChunk(id);
    } catch (const std::out_of_range& err) {
        throw ascee::AsceeError(err.what());
    }
}

void argc::store_int64(int32 offset, int64 value) {
    Executor::getSession()->heapModifier.store(offset, value);
}

void argc::add_int64_to(int32 offset, int64 amount) {
    Executor::getSession()->heapModifier.addInt(offset, amount);
}

void argc::resize_chunk(int32 new_size) {
    Executor::getSession()->heapModifier.updateChunkSize(new_size);
}

