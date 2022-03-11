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

#include <future>
#include "BlockValidator.h"
#include "RequestProcessor.hpp"

using namespace argennon;
using namespace ave;
using namespace util;
using namespace asa;
using std::vector, std::future, ascee::runtime::AppResponse;

static
Digest calculateDigest(vector<AppResponse> responses) {
    return {};
}

/// Conditionally validates a block: valid(current | previous). Returns true when the block is valid and false
/// if the block is not valid.
/// Throwing an exception indicates that due to an internal error checking the validity of the block was not possible.
bool BlockValidator::conditionalValidate(const BlockInfo& current, const BlockInfo& previous) {
    try {
        blockLoader.setCurrentBlock(current);

        ChunkIndex chunkIndex(
                cache.preparePages(previous, blockLoader.getReadonlyPageList(), blockLoader.getMigrationList()),
                cache.preparePages(previous, blockLoader.getWritablePageList(), blockLoader.getMigrationList()),
                blockLoader.getProposedSizeBounds(),
                blockLoader.getNumOfChunks()
        );

        RequestProcessor processor(chunkIndex, appIndex, blockLoader.getNumOfRequests(), workersCount);

        processor.loadRequests(blockLoader.createRequestStreams(workersCount));

        processor.buildDependencyGraph();

        auto responses = processor.executeRequests<ascee::runtime::Executor>();

        cache.commit(chunkIndex.getModifiedPages());

        return calculateDigest(responses) == blockLoader.getResponseListDigest();
    } catch (const BlockError& err) {
        std::cout << err.message << std::endl;
        cache.rollback(blockLoader.getWritablePageList());
        return false;
    }
}

BlockValidator::BlockValidator(
        PageCache& cache,
        BlockLoader& blockLoader,
        int workersCount) : cache(cache), blockLoader(blockLoader), appIndex(nullptr) {
    this->workersCount = workersCount < 1 ? (int) std::thread::hardware_concurrency() * 2 : workersCount;
}
