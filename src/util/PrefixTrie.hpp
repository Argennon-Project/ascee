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

#ifndef ARGENNON_IDENTIFIER_TRIE_H
#define ARGENNON_IDENTIFIER_TRIE_H

#include <string>
#include <stdexcept>
#include <array>
#include <cstring>

namespace argennon::util {

template<typename T, int height = sizeof(T)>
class PrefixTrie {
    static_assert(height <= sizeof(T) && height > 0);
    static_assert(std::is_unsigned<T>::value);

    using byte = uint8_t;
public:
    explicit constexpr PrefixTrie(const std::array<T, height>& trie) {
        for (int i = 0; i < height; ++i) {
            auto shift = (sizeof(T) - i - 1) * 8;
            this->trie[i] = trie[i] & (~T(0) >> shift);
            boundary[i] = trie[i] << shift;
            if (i > 0 && boundary[i - 1] > boundary[i]) throw std::invalid_argument("malformed trie");
        }

        // here we should use this->trie to make sure that high order bits are properly zeroed.
        sum[0] = this->trie[0];
        for (int i = 1; i < height; ++i) {
            sum[i] = sum[i - 1] + this->trie[i] - (this->trie[i - 1] << 8);
        }
    }

    /**
     * reads a prefix code from a byte array, assuming the code is big-endian. That means the root of the tree is
     * written at address 0.
     * @param binary
     * @param len
     * @param maxLength
     * @return
     */
    T readPrefixCode(const byte* binary, int32_t* len = nullptr, int maxLength = height) const {
        T id = 0;
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            id |= T(binary[i]) << ((sizeof(T) - i - 1) * 8);
            if (id < boundary[i]) {
                if (len != nullptr) *len = i + 1;
                return id;
            }
        }
        if (len != nullptr) *len = 0;
        throw std::out_of_range("readPrefixCode: invalid identifier");
    }

    /**
     * reads a prefix code from a byte* pointer and updates the pointer to pass the prefix code's bytes.
     * @param reader
     * @param end indicates the end of the byte array. *end will never be accessed.
     * @return read prefix code.
     */
    T readPrefixCode(const byte** reader, const byte* end = nullptr) const {
        T id = 0;
        end = end ? std::min(end, *reader + height) : *reader + height;
        for (int i = 0; *reader + i < end; ++i) {
            id |= T((*reader)[i]) << ((sizeof(T) - i - 1) * 8);
            if (id < boundary[i]) {
                (*reader) += i + 1;
                return id;
            }
        }
        throw std::out_of_range("readPrefixCode: invalid identifier");
    }

    /**
     * checks if two prefix codes are equal. Throws an exception if a and b are equal but do not
     * represent a valid prefix code of the trie. This happens only when their length is bigger than height of
     * the trie or @p maxLength.
     * @param a
     * @param b any string of bytes.
     * @param maxLength
     * @return
     */
    bool equals(const byte*& a, const byte*& b, int maxLength = height) const {
        T id = 0;
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            if (a[i] != b[i]) return false;
            id |= T(a[i]) << ((sizeof(T) - i - 1) * 8);
            if (id < boundary[i]) {
                a += i + 1;
                b += i + 1;
                return true;
            }
        }
        throw std::out_of_range("equals: invalid identifier");
    }

    /**
     * reads a prefix code from a data type which is not a byte*, assuming the data type contains a fixed-length
     * representation of the prefix code. That means the root of the tree is written in the highest order byte
     * of the data type.
     * @param binary
     * @param len
     * @param maxLength
     * @return
     */
    T readPrefixCode(T binary, int32_t* len = nullptr, int maxLength = height) const {
        if (maxLength > height) maxLength = height;
        for (int i = 0; i < maxLength; ++i) {
            if (binary < boundary[i]) {
                if (len != nullptr) *len = i + 1;
                return binary & (~T(0) << ((sizeof(T) - i - 1) * 8));
            }
        }
        if (len != nullptr) *len = 0;
        throw std::out_of_range("readPrefixCode: invalid identifier");
    }

    /**
     * encodes an unsigned integer as a prefix code. The encoding has a variable length.
     * @param value
     * @param len
     * @return
     */
    T encodeVarUInt(T value, int32_t* len = nullptr) const {
        for (int i = 0; i < height; ++i) {
            if (value < sum[i]) {
                auto bound = trie[i];
                if (len != nullptr) *len = int32_t(i + 1);
                return (bound - (sum[i] - value)) << ((sizeof(T) - i - 1) * 8);
            }
        }
        if (len != nullptr) *len = 0;
        throw std::overflow_error("encodeVarUInt: value too large");
    }

    /**
     *
     * @tparam U can be a byte* containing a big-endian representation of the prefix-code or a data type containing
     * a fixed length representation of the code.
     * @param binary
     * @param len
     * @param maxLength
     * @return
     */
    template<typename U>
    T decodeVarUInt(U binary, int32_t* len = nullptr, int maxLength = height) const {
        int n;
        T code = readPrefixCode(binary, &n, maxLength);
        code >>= (sizeof(T) - n) * 8;

        auto bound = trie[n - 1];
        if (len != nullptr) *len = n;
        return sum[n - 1] + code - bound;
    }

    /**
     *
     * @param reader
     * @param end
     * @return
     */
    T decodeVarUInt(const byte** reader, const byte* end = nullptr) const {
        int len = 0;
        auto ret = decodeVarUInt(*reader, &len, int(end - (*reader)));
        *reader += len;
        return ret;
    }

    T decodeVarUInt(const byte** reader) const {
        int len = 0;
        auto ret = decodeVarUInt(*reader, &len);
        *reader += len;
        return ret;
    }

    /**
     * this is a high performance function that reads a prefix code from a hex representation without checking the
     * validity of the prefix code.
     * @param str standard hex string representation of a prefix code, for example: 0x123.
     * @return
     */
    static T uncheckedParse(const std::string& str) {
        static_assert(sizeof(T) <= 8, "types larger than 64-bit are not supported");
        uint64_t num = std::stoull(str, nullptr, 0);
        num = num > 0xffffffff ? num : num << 32;
        num = num > 0xffffffffffff ? num : num << 16;
        num = num > 0xffffffffffffff ? num : num << 8;
        return T(num >> (64 - sizeof(T) * 8));
    }

    void parsePrefixCode(std::string symbolicRep, T& id) const {
        byte buffer[height];
        // returned array by data() is null-terminated after C++11
        char* token = strtok(symbolicRep.data(), ".");
        int i = 0;
        while (token != nullptr) {
            if (i == height) throw std::invalid_argument("parsePrefixCode: input too long");

            auto component = std::stoul(token, nullptr, 0);
            if (component > 255) throw std::overflow_error("identifier component is larger than 255");

            buffer[i++] = component;
            token = strtok(nullptr, ".");
        }
        int n;
        id = readPrefixCode(buffer, &n, i);
        if (n != i) throw std::invalid_argument("parsePrefixCode: input too long");
    }

    std::string toDecimalStr(const T code) const {
        std::string ret;
        for (int i = 0; i < height; ++i) {
            ret += std::to_string(byte(code >> ((sizeof(T) - i - 1) * 8)));
            if (code < boundary[i]) return ret;
            ret += ".";
        }
        throw std::out_of_range("toDecimalStr: invalid identifier");
    }

    void writeBigEndian(byte* dest, T value, int n) const {
        for (int i = 0; i < n; ++i) {
            dest[i] = byte(value >> ((sizeof(T) - i - 1) * 8));
        }
    }

private:
    T boundary[height] = {};
    T sum[height] = {};
    T trie[height] = {};
};

} // namespace argennon::util
#endif // ARGENNON_IDENTIFIER_TRIE_H
