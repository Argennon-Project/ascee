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
#include <iostream>
#include <util/StaticArray.hpp>
#include "util/OrderedStaticMap.hpp"
#include "validator/RequestScheduler.h"
#include "validator/BlockLoader.h"
#include "storage/ChunkIndex.h"
#include "storage/PageCache.h"

#include "argc/functions.h"

using namespace argennon;
using namespace ave;
using namespace asa;
using namespace ascee::runtime;

int overflow(int x, int y, int z) {
    if (x + y > z) return 1;
    else return 0;
}

/// contains temporary code examples
int main(int argc, char const* argv[]) {
    overflow(INT_MAX, 45, 30);
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

    using Access = AccessBlockInfo::Access::Type;
    PageLoader pl;
    PageCache pc(pl);
    BlockLoader bl;
    long_id app = 0x1000000000000000, acc = 0x2200000000000000, loc = 0x3300000000000000;
    VarLenFullID chunkID(std::unique_ptr<byte[]>(new byte[3]{0x10, 0x22, 0x33}));
    ChunkIndex ind({}, pc.prepareBlockPages({7878}, {chunkID}, {}),
                   {
                           {full_id(chunkID)},
                           {{20, 0}}
                   }, 0);
    RequestScheduler rs(3, ind);
    rs.addRequest({
                          .id = 0, .memoryAccessMap = {
                    {app},
                    {{{{acc, loc}}, {
                                            {{-1, 3}, {{1, Access::read_only, 0}, {4, Access::writable, 0}}},
                                    }}}},
                          .adjList = {1, 2}
                  });
    rs.addRequest({
                          .id = 1, .memoryAccessMap = {
                    {app},
                    {{{{acc, loc}}, {
                                            {{-1, 2}, {{1, Access::read_only, 1}, {4, Access::read_only, 1}}},
                                    }}}},
                          .adjList = {2}
                  });
    rs.addRequest({
                          .id = 2, .memoryAccessMap = {
                    {app},
                    {{{{acc, loc}}, {
                                            {{-1, 2}, {{1, Access::read_only, 2}, {5, Access::writable, 2}}},
                                    }}}}
                  });;

    auto temp = rs.sortAccessBlocks(8).at(app).at({acc, loc});
    rs.findCollisions(full_id(app, {acc, loc}), temp.getKeys(), temp.getValues());

    util::OrderedStaticMap<int, std::string> m1({10, 15, 24}, {"Hi", "Yo", "Bye"});

    util::OrderedStaticMap<int, std::string> m2({10, 16, 26}, {"Hi", "Yo2", "Bye2"});

    util::OrderedStaticMap<int, util::OrderedStaticMap<int, std::string>> m3({100}, {m1});
    util::OrderedStaticMap<int, util::OrderedStaticMap<int, std::string>> m4({100}, {m2});

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
}
