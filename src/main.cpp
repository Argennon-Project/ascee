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

#include <pbc.h>
#include <openssl/evp.h>
#include <executor/Executor.h>
#include <iostream>
#include <loader/AppLoader.h>
#include <util/StaticArray.h>

using namespace ascee;
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


    AppLoader::global = std::make_unique<AppLoader>("");
    Executor executor;
    AppRequest request{
            .calledAppID = 1,
            .httpRequest = "test request",
            .gas = 1000,
            .appTable = AppLoader::global->createAppTable({11})
    };
    auto response = executor.executeOne(&request);

    printf("hereeeee@@ \n%s\n", response.response.c_str());


    PageLoader pl;
    heap::PageCache c(pl);
    heap::PageCache::ChunkIndex ind(c, {{full_id(10, 100), true}}, {7878});
    RequestScheduler rs(10, ind);
    rs.addRequest(0, {});
    rs.addRequest(1, {});
    rs.addRequest(2, {});
    rs.addMemoryAccessList(10, 100, {
            {-1, 1, 0, false},
            {-1, 1, 1, false},
            {-1, 1, 2, false},
            {2,  4, 1, false},
            {2,  5, 2, true},
            {3,  4, 0, true},
    });



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
