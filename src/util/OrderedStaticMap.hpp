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

#ifndef ARGENNON_UTIL_FIXED_ORDERED_MAP_H
#define ARGENNON_UTIL_FIXED_ORDERED_MAP_H

#include <utility>
#include <vector>
#include <memory>
#include <functional>
#include <future>

namespace argennon::util {

template<typename K, typename V>
class OrderedStaticMap {
public:
    OrderedStaticMap() = default;

    OrderedStaticMap(std::vector<K> keys, std::vector<V> values) : keys(std::move(keys)), values(std::move(values)) {
        if (this->keys.size() != this->values.size()) {
            throw std::invalid_argument("size mismatch in keys and values");
        }
        this->keys.shrink_to_fit();
        this->values.shrink_to_fit();
    }

    V& at(const K& key) {
        return values[find(keys, key)];
    }

    const V& at(const K& key) const {
        return values[find(keys, key)];
    }

    static
    std::size_t find(const std::vector<K>& sortedKeys, const K& key) {
        std::size_t begin = 0, mid;
        auto end = sortedKeys.size();
        while ((mid = (begin + end) >> 1) < end) {
            if (key == sortedKeys[mid]) return mid;
            else if (key < sortedKeys[mid]) end = mid;
            else begin = mid + 1;
        }
        throw std::out_of_range("key not found");
    }

    [[nodiscard]] long size() const {
        return (long) keys.size();
    }

    [[nodiscard]] const std::vector<K>& getKeys() const {
        return keys;
    }

    [[nodiscard]] std::vector<V>& getValues() {
        return values;
    }

    [[nodiscard]] const std::vector<V>& getValues() const {
        return values;
    }

    static OrderedStaticMap merge(OrderedStaticMap left, OrderedStaticMap right) {
        return std::move(left) | std::move(right);
    }

    friend OrderedStaticMap operator|(OrderedStaticMap&& left, OrderedStaticMap&& right) {
        int i = 0, j = 0;
        OrderedStaticMap result;
        while (true) {
            if (i < left.size() && j < right.size()) {
                if (left.keys[i] == right.keys[j]) {
                    result.keys.emplace_back(left.keys[i]);
                    bool possible;
                    result.values.emplace_back(
                            mergeValues(std::move(left.values[i]), std::move(right.values[j]), possible)
                    );
                    if (!possible) {
                        result.keys.emplace_back(right.keys[j]);
                        result.values.emplace_back(std::move(right.values[j]));
                    }
                    ++i;
                    ++j;
                } else if (left.keys[i] < right.keys[j]) {
                    result.keys.emplace_back(left.keys[i]);
                    result.values.emplace_back(std::move(left.values[i]));
                    ++i;
                } else {
                    result.keys.emplace_back(right.keys[j]);
                    result.values.emplace_back(std::move(right.values[j]));
                    ++j;
                }
            } else if (i < left.size()) {
                result.keys.emplace_back(left.keys[i]);
                result.values.emplace_back(std::move(left.values[i]));
                ++i;
            } else if (j < right.size()) {
                result.keys.emplace_back(right.keys[j]);
                result.values.emplace_back(std::move(right.values[j]));
                ++j;
            } else break;
        }
        return result;
    }

private:
    std::vector<K> keys;
    std::vector<V> values;

    template<class T1, class T2>
    static OrderedStaticMap<T1, T2> mergeValues(OrderedStaticMap<T1, T2>&& left, OrderedStaticMap<T1, T2>&& right,
                                                bool& possible) {
        possible = true;
        return std::move(left) | std::move(right);
    }

    template<typename T>
    static T mergeValues(T&& left, T&& right, bool& possible) {
        printf("called\n");
        possible = false;
        return left;
    }
};

/// Calculates: maps[begin] | maps[begin + 1] | maps[begin + 2] | ... | map[end - 1], where | operator merges two maps.
template<class K, class V>
static
OrderedStaticMap<K, V> mergeAllParallel(std::vector<OrderedStaticMap<K, V>>&& maps,
                                        std::size_t begin, std::size_t end, std::size_t k) {
    using std::move;

    if (begin >= end) return {};

    if (end < begin + k || end < begin + 3) {
        auto&& result = std::move(maps[begin]);
        for (std::size_t i = begin + 1; i < end; ++i) {
            result = move(result) | move(maps[i]);
        }
        return result;
    }

    auto secondHalf = std::async([&]() { return mergeAllParallel(move(maps), (begin + end) / 2, end, k); });
    auto&& firstHalf = mergeAllParallel(move(maps), begin, (begin + end) / 2, k);
    return move(firstHalf) | secondHalf.get();
}

template<class K, class V>
inline
OrderedStaticMap<K, V> mergeAllParallel(std::vector<OrderedStaticMap<K, V>>&& maps, int workersCount) {
    return mergeAllParallel(std::move(maps), 0, maps.size(), maps.size() / workersCount);
}

} // namespace argennon::util
#endif // ARGENNON_UTIL_FIXED_ORDERED_MAP_H