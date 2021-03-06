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

    /**
     * Creates a map. It does not check that the provided keys are sorted.
     * @param keys a sorted vector of keys. If this vector is not sorted the behaviour of the map is undefined.
     * @param values corresponding vector of values associated with keys, such that @p values[i] is associated
     * with @p keys[i].
     */
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

    [[nodiscard]] std::vector<K>& getKeys() {
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

    /**
     * merges two maps, for tie breaking < operator is used for comparing values when two keys are equal.
     * @param left
     * @param right
     * @return
     */
    friend OrderedStaticMap operator|(OrderedStaticMap&& left, OrderedStaticMap&& right) {
        std::size_t i = 0, j = 0;
        OrderedStaticMap result;
        while (true) {
            if (i < left.size() && j < right.size()) {
                if (left.keys[i] == right.keys[j]) {
                    result.keys.emplace_back(left.keys[i]);
                    result.values.emplace_back(
                            mergeValues(std::move(left.values[i]), std::move(right.values[j]), i, j)
                    );
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
                                                std::size_t& i, std::size_t& j) {
        ++i;
        ++j;
        return std::move(left) | std::move(right);
    }

    template<typename T>
    static T mergeValues(T&& left, T&& right, std::size_t& i, std::size_t& j) {
        if (left < right) {
            ++i;
            return std::forward<T>(left);
        } else {
            ++j;
            return std::forward<T>(right);
        }
    }
};


template<class K, class V>
static
OrderedStaticMap<K, V> mergeIntervalParallel(std::vector<OrderedStaticMap<K, V>>& maps,
                                             std::size_t begin, std::size_t end, std::size_t k) {
    using std::move;

    if (end <= begin) return {};

    if (end == begin + 1) return maps[begin];

    if (end <= begin + k) {
        return mergeIntervalParallel(maps, begin, (begin + end) / 2, k) |
               mergeIntervalParallel(maps, (begin + end) / 2, end, k);
    }

    auto secondHalf = std::async([&]() { return mergeIntervalParallel(maps, (begin + end) / 2, end, k); });
    auto&& firstHalf = mergeIntervalParallel(maps, begin, (begin + end) / 2, k);
    return move(firstHalf) | secondHalf.get();
}

/// Calculates: maps[begin] | maps[begin + 1] | maps[begin + 2] | ... | map[end - 1], where | operator merges two maps.
template<class K, class V>
OrderedStaticMap<K, V> mergeAllParallel(std::vector<OrderedStaticMap<K, V>>&& maps, int workersCount) {
    return mergeIntervalParallel(maps, 0, maps.size(), maps.size() / workersCount);
}

template<typename T>
void insertionSort(std::vector<T>& v) {
    for (std::size_t i = 1; i < v.size(); ++i) {
        for (std::size_t j = i; j > 0 && v[j] < v[j - 1]; --j) {
            std::swap(v[j], v[j - 1]);
        }
    }
}

/**
 * Inserts a sorted vector to another sorted vector. The final result will be a sorted vector. When two vectors are
 * trivially merge-able number of comparisons and moves is relatively low. For example, if @p r[0]>l.back() only
 * @p r.size() comparisons will be done without any moves.
 * @tparam T
 * @param l A sorted vector that @p r will be inserted into it. This vector will hold the final result of merging
 * two vectors.
 * @param r A sorted vector that will be inserted into @p l
 */
template<typename T>
void mergeInsert(std::vector<T>& l, const std::vector<T>& r) {
    int_fast32_t j = l.size() - 1;
    l.resize(l.size() + r.size());
    for (int_fast32_t i = r.size() - 1; i >= 0; --i) {
        while (j >= 0 && l[j] > r[i]) {
            l[j + i + 1] = std::move(l[j]);
            --j;
        }
        l[j + i + 1] = r[i];
    }
}

} // namespace argennon::util
#endif // ARGENNON_UTIL_FIXED_ORDERED_MAP_H
