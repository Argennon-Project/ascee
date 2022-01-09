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


#include "subtest.h"
#include "executor/RequestScheduler.h"

using namespace ascee;
using namespace runtime;

using Access = BlockAccessInfo::Type;
/*
TEST(RequestSchedulerTest, SimpleCollisionDetection) {
    PageLoader pl{};
    heap::PageCache pc(pl);
    heap::PageCache::ChunkIndex index(pc, {10}, {{100, true}},
                                      {{100},
                                       {{8, 3}}});


    RequestScheduler scheduler(14, index);
    scheduler.findCollisions(100, {-3, -2, -1, -1, 0, 1, 2, 2, 4, 5, 6}, {
            {0,  Access::read_only, 12},     // -3
            {1,  Access::read_only, 13},     // -2
            {6,  Access::writable,  10},     // -1
            {-3, Access::writable,  11},     // -1
            {2,  Access::read_only, 4},     // 0
            {2,  Access::writable,  1},     // 1
            {3,  Access::read_only, 5},     // 2
            {2,  Access::writable,  2},     // 2
            {2,  Access::writable,  3},     // 4
            {1,  Access::read_only, 6},     // 5
            {2,  Access::read_only, 7},     // 6
            //  {1,  Access::read_only, 8},     // 8

            // [10->13] [11->13] [10->11] [1->4] [1->5] [1->2] [2->5] [3->5] [5->10] [5->11] [2->10] [2->11] [3->6] [3->10] [3->11] [6->10] [6->11] [7->11]
    });
}
*/
TEST(RequestSchedulerTest, CollisionsFromRequests) {
    PageLoader pl{};
    heap::PageCache pc(pl);
    auto chunkFullID = full_id(10, 100);
    heap::PageCache::ChunkIndex index(pc, {10},
                                      {{chunkFullID, true}}, {{chunkFullID},
                                                              {{8, 3}}}, 0);

    index.getChunk(chunkFullID)->setSize(5);
    RequestScheduler scheduler(14, index);

    scheduler.addRequest(12, {.id = 12, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-3}, {{0, Access::read_only, 12},}},
                     }}}},
            .adjList = {}});
    scheduler.addRequest(13, {.id = 13, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-2}, {{1, Access::read_only, 13},}},
                     }}}},
            .adjList = {}});
    scheduler.addRequest(10, {.id = 10, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1}, {{6, Access::writable, 10},}},
                     }}}},
            .adjList = {13, 11}});
    scheduler.addRequest(11, {.id = 11, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1}, {{-3, Access::writable, 11},}},
                     }}}},
            .adjList = {13}});
    scheduler.addRequest(4, {.id = 4, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{0}, {{2, Access::read_only, 4},}},
                     }}}},
            .adjList = {}});
    scheduler.addRequest(1, {.id = 1, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{1}, {{2, Access::writable, 1},}},
                     }}}},
            .adjList = {4, 2, 5}});
    scheduler.addRequest(5, {.id = 5, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{2}, {{3, Access::read_only, 5},}},
                     }}}},
            .adjList = {10, 11}});
    scheduler.addRequest(2, {.id = 2, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{2}, {{2, Access::writable, 2},}},
                     }}}},
            .adjList = {5, 10, 11}});
    scheduler.addRequest(3, {.id = 3, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{4}, {{2, Access::writable, 3},}},
                     }}}},
            .adjList = {5, 6, 10, 11}});
    scheduler.addRequest(6, {.id = 6, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{5}, {{1, Access::read_only, 6},}},
                     }}}},
            .adjList = {10, 11}});
    scheduler.addRequest(7, {.id = 7, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{6}, {{2, Access::read_only, 7},}},
                     }}}},
            .adjList = {11}});
    /*scheduler.addRequest(8, {.id = 8, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{7}, {{1, Access::read_only, 8},}},
                     }}}},
            .adjList = {11}});*/


    auto sortedMap = scheduler.sortAccessBlocks();


    scheduler.findCollisions(chunkFullID, sortedMap.at(10).at(100).getKeys(), sortedMap.at(10).at(100).getValues());

}
