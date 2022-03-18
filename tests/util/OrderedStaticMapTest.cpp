// Copyright (c) 2022 aybehrouz <behrouz_ayati@yahoo.com>. All rights
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
#include "util/OrderedStaticMap.hpp"

using namespace argennon;
using namespace util;
using std::vector;


TEST(UtilSortHelpers, MergeInsert) {
    vector<int> a = {5, 8, 10, 11};
    mergeInsert(a, {3, 9, 12});
    EXPECT_EQ(a, vector<int>({3, 5, 8, 9, 10, 11, 12}));

    mergeInsert(a, {});
    EXPECT_EQ(a, vector<int>({3, 5, 8, 9, 10, 11, 12}));

    mergeInsert(a, {3});
    EXPECT_EQ(a, vector<int>({3, 3, 5, 8, 9, 10, 11, 12}));

    mergeInsert(a, {0});
    EXPECT_EQ(a, vector<int>({0, 3, 3, 5, 8, 9, 10, 11, 12}));

    mergeInsert(a, {0, 12});
    EXPECT_EQ(a, vector<int>({0, 0, 3, 3, 5, 8, 9, 10, 11, 12, 12}));

    a = {1};
    mergeInsert(a, {2});
    EXPECT_EQ(a, vector<int>({1, 2}));

    a = {1};
    mergeInsert(a, {0});
    EXPECT_EQ(a, vector<int>({0, 1}));

    a.clear();
    mergeInsert(a, {7});
    EXPECT_EQ(a, vector<int>({7}));

    a = {2, 5, 5, 8};
    mergeInsert(a, {1, 5, 7, 9, 10, 11});
    EXPECT_EQ(a, vector<int>({1, 2, 5, 5, 5, 7, 8, 9, 10, 11}));

    a = {5, 6, 6, 8};
    mergeInsert(a, {9, 10, 15, 16});
    EXPECT_EQ(a, vector<int>({5, 6, 6, 8, 9, 10, 15, 16}));
}

TEST(OrderedStaticMapTest, Merge) {
    OrderedStaticMap<int, int> m1({1, 2, 5, 7}, {10, 20, 50, 70});
    OrderedStaticMap<int, int> m2({1, 3, 5, 6, 7}, {1000, 300, 500, 600, 700});
    OrderedStaticMap<int, int> m3({1, 4, 5, 7, 8}, {100, 4000, 5000, 7, 8000});

    auto m = std::move(m2) | std::move(m1) | std::move(m3);

    EXPECT_EQ(m.getValues(), vector<int>({10, 100, 1000, 20, 300, 4000, 50, 500, 5000, 600, 7, 70, 700, 8000}));
    EXPECT_EQ(m.getKeys(), vector<int>({1, 1, 1, 2, 3, 4, 5, 5, 5, 6, 7, 7, 7, 8}));
}

TEST(OrderedStaticMapTest, MergeParallel) {
    vector<OrderedStaticMap<int, int>> vm;
    vm.emplace_back(vector<int>{1, 1, 1, 2}, vector<int>{10, 20, 30, 2000});
    vm.emplace_back(vector<int>{1}, vector<int>{15});
    vm.emplace_back(vector<int>{1, 1, 2}, vector<int>{25, 35, 200});
    vm.emplace_back(vector<int>{1, 2, 2}, vector<int>{12, 100, 2500});

    auto m = mergeAllParallel(std::move(vm), 3);

    EXPECT_EQ(m.getValues(), vector<int>({10, 12, 15, 20, 25, 30, 35, 100, 200, 2000, 2500}));
    EXPECT_EQ(m.getKeys(), vector<int>({1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2}));
}

TEST(OrderedStaticMapTest, SimpleAccessingBenchmark) {
    OrderedStaticMap<int, float> singleStatic({8}, {0.0});
    std::unordered_map<int, float> singleStd = {{8, 0.0}};

    int runs = 2000000;
    float dummy = 8;

    BENCHMARK(dummy += singleStatic.at(dummy), runs, "accessing OrderedStaticMap with one item");

    BENCHMARK(dummy += singleStd.at(dummy), runs, "accessing hash table with one item");

    OrderedStaticMap<int, float> multiStatic({2, 4, 7, 8, 9, 11, 15, 16, 17},
                                             {2.5, 0.2, 0.4, 0.0, 2.1, 5.6, 1.3, 2.4, 0.0});
    std::unordered_map<int, float> multiStd = {
            {{2, 2.5}, {4, 0.2}, {7, 0.4}, {8, 0.0}, {9, 2.1}, {11, 5.6}, {15, 1.3}, {16, 2.4}, {17, 0.0}}
    };

    BENCHMARK(dummy += multiStatic.at(dummy); dummy += multiStatic.at(9 + dummy);, runs,
              "accessing OrderedStaticMap with 9 items");

    BENCHMARK(dummy += multiStd.at(dummy); dummy += multiStd.at(9 + dummy);, runs,
              "accessing std hash table with 9 items");
}
