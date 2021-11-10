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

#ifndef ASCEE_PAGE_H
#define ASCEE_PAGE_H

#include <argc/types.h>
#include "Chunk.h"

namespace ascee {

class Page {
private:
    std::unique_ptr<Chunk> native;
public:
    void setNative(Chunk* newNative) {
        native.reset(newNative);
    }

    void addMigrant(std_id_t appID, std_id_t chunkID, Chunk* migrant) {};

    byte* getDigest();

};

} // namespace ascee
#endif // ASCEE_PAGE_H
