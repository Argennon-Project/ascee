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

#include <pbc/pbc.h>
#include <executor/Executor.h>
#include <iostream>
#include <loader/AppLoader.h>
#include <util/StaticArray.hpp>
#include "util/FixedOrderedMap.hpp"
#include "validator/RequestScheduler.h"
#include "validator/BlockLoader.h"
#include "storage/ChunkIndex.h"
#include "storage/PageCache.h"

#include "argc/functions.h"

using namespace argennon;
using namespace ave;
using namespace asa;
using namespace ascee::runtime;

int main(int argc, char const* argv[]) {
    // initialize a pairing:
    char param[1024];
    FILE* params = fopen("../param/a.param", "r");
    if (params == nullptr) {
        throw std::runtime_error("param file not found");
    }
    size_t count = fread(param, 1, 1024, params);
    fclose(params);

    if (!count) throw std::runtime_error("input error");

    pairing_t pairing{};
    // We shall need several element_t variables to hold the system parameters, keys and other quantities.
    // We declare them here and initialize them in the constructor:
    element_t inv{}, z{};
    element_t x{}, y{};
    element_t sig_elem{};
    element_t temp1{}, temp2{};

    pairing_init_set_buf(pairing, param, count);

    element_init_G2(inv, pairing);
    element_init_G2(z, pairing);
    element_init_G2(x, pairing);
    element_init_G2(y, pairing);
    element_init_GT(temp1, pairing);
    element_init_GT(temp2, pairing);

    element_random(x);
    element_random(y);

    element_invert(inv, x);

    element_add(z, x, inv);

    element_printf("x:%B\nx+y->%B\n", x, z);

    element_mul(z, x, inv);

    element_printf("x.y->%B\n", z);

    using Access = BlockAccessInfo::Access::Type;

    AppRequestInfo testReq{
            .id = 0,
            .calledAppID = 0x1,
            .httpRequest = "PATCH /balances/0x14ab HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "Content-Length: 57\r\n"
                           "\r\n"
                           R"({"to":0xabc,"amount":1399,"sig":"OcTd6Oa93sNeQpZVoN4sd7BOGGnRxfyDJnuitYpOr_g8dtGcgAX8XH2g7klAD50vhrl299NyEgGEG2FTqIscgwA"})",
            .gas = 1000,
            .appAccessList = {0x1},
            .memoryAccessMap = {
                    {0x1},
                    {{{0x0abc000000000000, 0x0abc000000000001, 0x14ab000000000000, 0x14ab000000000001},
                             {
                                     {{-3, 0}, {{1, Access::writable, 0}, {2, Access::check_only, 0}}},
                                     {{-3, 0}, {{1, Access::read_only, 0}, {8, Access::int_additive, 0}}},
                                     {{-3, 0, 2}, {{1, Access::writable, 0}, {2, Access::writable, 0}, {65, Access::read_only, 0}}},
                                     {{-3, 0}, {{1, Access::writable, 0}, {8, Access::writable, 0}}},
                             }}}}
    };

    AppLoader::global = std::make_unique<AppLoader>("compiled");

    Page page_1(777);
    byte delta_1_0[] = {67, 0, 67, 11, 0, 76, 4, 147, 2, 242, 185, 142, 97, 127, 241, 23, 190, 47, 105, 5, 197, 242, 14,
                        106, 29, 4, 157, 46, 119, 66, 108, 63, 13, 25, 195, 9, 216, 126, 9, 128, 156, 224, 63, 233, 197,
                        128, 173, 14, 141, 30, 213, 59, 43, 58, 26, 174, 248, 37, 74, 220, 219, 31, 0, 204, 47, 169,
                        120, 248, 137, 0};


    page_1.getNative()->applyDelta({}, delta_1_0, sizeof(delta_1_0));


    byte delta_1_1[] = {8, 0, 2, 21, 20};
    auto from_chunk = new Chunk();
    from_chunk->applyDelta({}, delta_1_1, sizeof(delta_1_1));
    page_1.addMigrant({0x1, 0x14ab000000000001}, from_chunk);

    std::cout << (std::string) *from_chunk << std::endl;

    Page page_2(777);
    byte delta_2_0[] = {67, 0, 2, 45, 45};
    page_2.getNative()->applyDelta({}, delta_2_0, sizeof(delta_2_0));

    byte delta_2_1[] = {8, 0, 2, 1, 1};
    auto to_chunk = new Chunk();
    to_chunk->applyDelta({}, delta_2_1, sizeof(delta_2_1));
    page_2.addMigrant({0x1, 0x0abc000000000001}, to_chunk);

    std::cout << (std::string) *to_chunk << std::endl;

    page_1.setWritableFlag(true);
    page_2.setWritableFlag(true);

    ChunkIndex index({
                             {full_id(0x1, 0x14ab000000000000), &page_1},
                             {full_id(0x1, 0x0abc000000000000), &page_2}
                     }, {}, 4);

    RequestScheduler scheduler(1, index);

    scheduler.addRequest(std::move(testReq));
    scheduler.finalizeRequest(0);
    scheduler.buildExecDag();

    Executor executor;
    auto response = executor.executeOne(scheduler.nextRequest());

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response.httpResponse.c_str());

    std::cout << (std::string) *from_chunk << std::endl;
    std::cout << (std::string) *to_chunk << std::endl;
    std::cout << (std::string) *page_1.getNative() << std::endl;

    PageLoader pl;
    PageCache c(pl);


    BlockLoader bl;
    auto chunkID = full_id(10, 100);
    ChunkIndex ind(c.prepareBlockPages({7878}, {{chunkID, true}}, {}),
                   {{chunkID},
                    {{20, 0}}}, 0);
    RequestScheduler rs(3, ind);
    rs.addRequest({.id = 0, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1, 3}, {{1, Access::read_only, 0}, {4, Access::writable, 0}}},
                     }}}},
                          .adjList = {1, 2}});
    rs.addRequest({.id = 1, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1, 2}, {{1, Access::read_only, 1}, {4, Access::read_only, 1}}},
                     }}}},
                          .adjList={2}});
    rs.addRequest({.id = 2, .memoryAccessMap = {
            {10},
            {{{100}, {
                             {{-1, 2}, {{1, Access::read_only, 2}, {5, Access::writable, 2}}},
                     }}}}});

    ;

    auto temp = rs.sortAccessBlocks(8).at(10).at(100);
    rs.findCollisions(full_id(10, 100), temp.getKeys(), temp.getValues());

    util::FixedOrderedMap<int, std::string> m1({10, 15, 24}, {"Hi", "Yo", "Bye"});

    util::FixedOrderedMap<int, std::string> m2({10, 16, 26}, {"Hi", "Yo2", "Bye2"});

    util::FixedOrderedMap<int, util::FixedOrderedMap<int, std::string>> m3({100}, {m1});
    util::FixedOrderedMap<int, util::FixedOrderedMap<int, std::string>> m4({100}, {m2});

    auto m = util::mergeAllParallel<int, std::string>(
            {
                    {{5, 6, 10},     {"5", "6", "10"}},
                    {{7, 10},        {"7", "10"}},
                    {{4, 5, 9},      {"4", "5", "9"}},
                    {{8},            {"8"}},
                    {{2, 7, 11, 12}, {"2", "5", "11", "12"}}
            }, 8);

    for (const auto& item: m.getKeys()) {
        std::cout << item << ",";
    }
    std::cout << std::endl;

/*
    char cStr[20] = "abcdefgh";
    string str = string(cStr, 4);
    cout << "this->" << str << endl;
    cStr[0] = 'z';
    cout << "that->" << cStr << endl;
    cout << "this->" << str << endl;
    cout << sizeof "abc" << endl;
    Heap heap;
    Executor e;
    return 0;
    auto* modifier = heap.initSession(2);
    short_id chunk = 111;
    modifier->loadContext(2);
    modifier->loadChunk(chunk);
    modifier->saveVersion();
    modifier->store(5, 0x1122334455667788);
    printf("\nread->%lx\n", modifier->load<int64_t>(5));

    modifier->restoreVersion(0);
    int64 xyz = modifier->load<int64_t>(5);
    printf("\nread->%lx\n", xyz);

    delete modifier;

    uint8_t test[256];
    int64_t x = 0x12345678;
    auto* t = (int64_t*) (test);
    t[0] = x;
    for (int i = 0; i < 15; ++i) {
        printf("%x ", test[i]);
    }
    printf("\n%lx", t[0]);

     AppLoader::global = std::make_unique<AppLoader>("");

     Executor e;
     Transaction tr1{1, "req", 4, 1000000000000, {1, 2, 3}};
     /// Transaction tr2{2, "req",5000, {2}};

     std::thread t1(&Executor::startSession, &e, tr1);
     //std::thread t2(&Executor::startSession, &e, tr2);

     t1.join();
     //t2.join();
     return 0;*/
}
