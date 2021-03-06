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
using namespace ascee;
using namespace runtime;

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
    // In this example, we want to send 1399 ARG from account 0x95ab to account 0xaabc. For this, we need to make
    // a request to the ARG app. The information of the request should be packed in a AppRequestInfo struct. We use a
    // vector of AppRequestInfo structs, because RequestProcessor accepts RequestStreams as input. We will later
    // convert this vector to a stream using FakeStream class.
    std::vector<AppRequestInfo> requests{
            {
                    .id = 0,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ab HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1399,"sig":0})",
                    .maxClocks = 1000,
                    .appAccessList = {arg_app_id_g},
                    // We need to define the required access blocks. Don't forget that KEYS MUST BE SORTED! check
                    // memoryAccessMap documentation.
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0x95ab000000000000, 0}, {0xaabc000000000000, 0}},
                                     {
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 0}, {2, Access::writable, 0},
                                                                      {65, Access::read_only, 0}, {8, Access::writable, 0}}},
                                             {{-3, 0, 67}, {{1, Access::writable, 0}, {2, Access::read_only, 0}, {8, Access::int_additive, 0}}},

                                     }}}},
                    .signedMessagesList {
                            {
                                    0x95ab000000000000,
                                    R"({"to":0xaabc000000000000,"amount":1399,"forApp":0x100000000000000,"nonce":11})",
                                    util::Signature(
                                            "LNUC49Lhyz702uszzNcfaU3BhPIbdaSgzqDUKzbJzLPTlFS2J9GzHlcDKbvxx5T5yfvJOTAcmnX0Oh0B_-gqPwE"
                                    )
                            },
                    }
            }

    };

    auto dummyBlockNumber = 777;

    // Now we create a fake state in which the ARG balance of account 0x95ab is 5141. We also set the appropriate public
    // key for it and set its nonce to 11 so the signature can be verified. Check applyDelta document to find out
    // how the data should be encoded.
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
                          dummyBlockNumber
                          + 1
    );

    // We set the initial balance of the recipient account to 257.
    Page recipientPage(dummyBlockNumber);
    VarLenFullID toPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0xaa, 0xbc, 0x0}));
    recipientPage.applyDelta(toPageID,
                             {
                                     {
                                             0,
                                             67 + 8, 1, 2, 45, 45,
                                             66, 2, 1, 1
                                     },
                                     {
                                     }},
                             dummyBlockNumber + 1
    );

    ChunkIndex chunkIndex({}, {
                                  {full_id(arg_app_id_g, {0x95ab000000000000, 0}), &senderPage},
                                  {full_id(arg_app_id_g, {0xaabc000000000000, 0}), &recipientPage}
                          },
                          {}, 2);

    // "apps" is the folder that the current source for the ARG application is stored.
    AppLoader appLoader("apps");
    AppIndex appIndex(&appLoader);
    // Block number does not matter, and we set it to 123 just for fun.
    appIndex.prepareApps({123}, {arg_app_id_g});

    RequestProcessor processor(chunkIndex, appIndex, int(requests.size()), 3);

    processor.loadRequests<FakeStream>({{0, 1, requests},});
    processor.checkDependencyGraph();

    auto response = processor.parallelExecuteRequests<Executor>();

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n",
           response[0].httpResponse.c_str());

    std::cout << (std::string) *senderPage.getNative() << "\n";
    std::cout << (std::string) *recipientPage.getNative() << "\n";
}
