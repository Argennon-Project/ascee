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

#include <sstream>
#include <memory>
#include "id.h"
#include "core/info.h"
#include "util/PrefixTrie.hpp"

using namespace argennon;
using namespace ascee::runtime;
using std::string;

LongID::LongID(std::string_view str) : id(util::PrefixTrie<uint64_t>::uncheckedParse(std::string(str))) {}

LongID::operator std::string() const {
    std::stringstream buf;
    buf << "0x" << std::hex << id;
    return {buf.str()};
}

LongLongID::operator std::string() const {
    return (string) up + "::" + (string) down;
}

FullID::operator std::string() const {
    return (string) up + "::" + (string) down;
}

VarLenFullID::VarLenFullID(const byte** binary, const byte* end) {
    auto start = *binary;
    app_trie_g.readPrefixCode(binary, end);
    account_trie_g.readPrefixCode(binary, end);
    local_trie_g.readPrefixCode(binary, end);
    auto len = int(*binary - start);
    this->binary = std::make_unique<byte[]>(len);
    memcpy(this->binary.get(), start, len);
}

VarLenFullID::VarLenFullID(const VarLenFullID& other) {
    auto len = other.getLen();
    binary = std::make_unique<byte[]>(len);
    memcpy(binary.get(), other.binary.get(), len);
}

bool VarLenFullID::operator==(const VarLenFullID& rhs) const {
    const byte* a = this->binary.get();
    const byte* b = rhs.binary.get();
    return app_trie_g.equals(a, b) &&
           account_trie_g.equals(a, b) &&
           local_trie_g.equals(a, b);
}

VarLenFullID::operator FullID() const {
    const byte* ptr = binary.get();
    auto up = app_trie_g.readPrefixCode(&ptr);
    auto middle = account_trie_g.readPrefixCode(&ptr);
    auto down = local_trie_g.readPrefixCode(&ptr);
    return {up, {middle, down}};
}

int VarLenFullID::getLen() const {
    const byte* ptr = binary.get();
    app_trie_g.readPrefixCode(&ptr);
    account_trie_g.readPrefixCode(&ptr);
    local_trie_g.readPrefixCode(&ptr);
    return int(ptr - binary.get());
}

VarLenFullID::operator std::string() const {
    std::stringstream buf;
    buf << "0x";
    for (int i = 0; i < getLen(); ++i) {
        buf << std::hex << (int) binary[i];
    }
    return buf.str();
}

const byte* VarLenFullID::getBinary() const {
    return binary.get();
}
