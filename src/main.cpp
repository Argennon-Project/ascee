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

#include "argc/functions.h"
#include "validator/RequestProcessor.hpp"

using namespace argennon;
using namespace ave;
using namespace asa;
using namespace ascee::runtime;

int overflow(int x, int y, int z) {
    if (x + y > z) return 1;
    else return 0;
}

using Access = AccessBlockInfo::Access::Type;


class FakeStream {
public:
    class EndOfStream : std::exception {
    };

    FakeStream(int start, int end, std::vector<AppRequestInfo>& requests) : current(start), end(end),
                                                                            requests(requests) {}

    AppRequestInfo next() {
        if (current >= end) throw EndOfStream();
        return requests.at(current++);
    }

private:
    int current;
    int end;
    std::vector<AppRequestInfo>& requests;
};

/// contains temporary code examples
int main(int argc, char const* argv[]) {
    std::vector<AppRequestInfo> requests{
            {
                    .id = 0,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ab HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1399,"sig":"LNUC49Lhyz702uszzNcfaU3BhPIbdaSgzqDUKzbJzLPTlFS2J9GzHlcDKbvxx5T5yfvJOTAcmnX0Oh0B_-gqPwE"})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0x95ab000000000000, 0}, {0xaabc000000000000, 0}},
                                     {
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 0}, {2, Access::writable, 0},
                                                                      {65, Access::read_only, 0}, {8, Access::writable, 0}}},
                                             {{-3, 0, 67}, {{1, Access::writable, 0}, {2, Access::read_only, 0}, {8, Access::int_additive, 0}}},

                                     }}}}
            }
    };

    auto dummyBlockNumber = 777;

    Page senderPage(dummyBlockNumber);
    VarLenFullID senderPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xab, 0x0}));

    senderPage.applyDelta(senderPageID,
                          Page::Delta{.content = {0,
                                                  67 + 8, 1, 69, 11, 0,
                                  // pk:
                                                  167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227,
                                                  178, 221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130,
                                                  216, 239, 80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164,
                                                  90, 32, 235, 166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243,
                                                  108, 37, 70,
                                                  0,
                                  // pk end
                                                  21, 20},
                                  .finalDigest = {}},
                          dummyBlockNumber + 1
    );

    Page recipientPage(dummyBlockNumber);
    VarLenFullID toPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0xaa, 0xbc, 0x0}));
    recipientPage.applyDelta(toPageID,
                             {{
                                      0,
                                      67 + 8, 1, 2, 45, 45,
                                      66, 2, 1, 1
                              },
                              {}},
                             dummyBlockNumber + 1
    );

    ChunkIndex index({}, {
                             {full_id(arg_app_id_g, {0x95ab000000000000, 0}), &senderPage},
                             {full_id(arg_app_id_g, {0xaabc000000000000, 0}), &recipientPage}
                     },
                     {}, 2);

    AppLoader::global = std::make_unique<AppLoader>("apps");
    RequestProcessor processor(index, int(requests.size()), 5);

    processor.loadRequests<FakeStream>({
                                               {0, 1, requests},
                                       });
    processor.buildDependencyGraph();
    auto response = processor.executeRequests<Executor>();

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[0].httpResponse.c_str());

    std::cout << (std::string) *senderPage.getNative() << "\n";
    std::cout << (std::string) *recipientPage.getNative() << "\n";
}
