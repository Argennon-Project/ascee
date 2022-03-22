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
#include "ascee/executor/Executor.h"
#include "storage/Page.h"
#include "storage/ChunkIndex.h"
#include "validator/RequestScheduler.h"
#include "validator/RequestProcessor.hpp"

// #define ASCEE_SERIAL_EXEC

using namespace argennon;
using namespace asa;
using namespace ave;
using namespace ascee::runtime;

using Access = AccessBlockInfo::Access::Type;

class ArgAppTest : public ::testing::Test {
protected:
    AppLoader loader{"../apps"};
    AppIndex appIndex{&loader};
public:
    ArgAppTest() {
        appIndex.prepareApps({123}, {arg_app_id_g});
    }
};

TEST_F(ArgAppTest, SimpleTransfer) {
    AppRequestInfo transferReq{
            .id = 0,
            .calledAppID = arg_app_id_g,
            .httpRequest = "PATCH /balances/0x95ab HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "Content-Length: 57\r\n"
                           "\r\n"
                           R"({"to":0xaabc,"amount":1399,"sig":0})",
            .gas = 1000,
            .appAccessList = {arg_app_id_g},
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
    };

    Page page_1(777);
    VarLenFullID fromPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xab, 0x0}));

    printf("%d\n", fromPageID.getLen());

    page_1.applyDelta(fromPageID,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 69, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              21, 20},
                              .finalDigest = {}},
                      780);

    Page page_2(777);
    VarLenFullID toPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0xaa, 0xbc, 0x0}));
    page_2.applyDelta(toPageID,
                      {{
                               0,
                               67 + 8, 1, 2, 45, 45,
                               66, 2, 1, 1
                       },
                       {}},
                      780);

    EXPECT_EQ((std::string) *page_1.getNative(),
              "size: 75, capacity: 75, content: 0x[ b 0 a7 3f e3 af ce 2b e7 27 3e 56 2b 91 fb f0 e3 b2 dd 82 ea 29 11 43 79 77 4d 0 5f 99 26 82 d8 ef 50 59 55 0 97 77 0 80 22 6d 23 61 d5 a4 5a 20 eb a6 de cd 17 d5 75 cb 28 e0 7 80 f3 6c 25 46 0 15 14 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getNative(),
              "size: 75, capacity: 75, content: 0x[ 2d 2d 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0 0 0 ]");

    ChunkIndex index({}, {
                             {full_id(arg_app_id_g, {0x95ab000000000000, 0}), &page_1},
                             {full_id(arg_app_id_g, {0xaabc000000000000, 0}), &page_2}
                     },
                     {}, 4);

    RequestScheduler scheduler(1, index, appIndex);

    scheduler.addRequest(std::move(transferReq));
    scheduler.finalizeRequest(0);
    scheduler.buildExecDag();

    Executor executor;
    auto response = executor.executeOne(scheduler.nextRequest());

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response.httpResponse.c_str());

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ((std::string) *page_1.getNative(),
              "size: 75, capacity: 75, content: 0x[ c 0 a7 3f e3 af ce 2b e7 27 3e 56 2b 91 fb f0 e3 b2 dd 82 ea 29 11 43 79 77 4d 0 5f 99 26 82 d8 ef 50 59 55 0 97 77 0 80 22 6d 23 61 d5 a4 5a 20 eb a6 de cd 17 d5 75 cb 28 e0 7 80 f3 6c 25 46 0 9e e 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getNative(),
              "size: 75, capacity: 75, content: 0x[ 2d 2d 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 78 6 0 0 0 0 0 0 ]");
}

TEST_F(ArgAppTest, SimpleCreateAcc) {
    AppRequestInfo createReq{
            .id = 0,
            .calledAppID = arg_app_id_g,
            .httpRequest = "PUT /balances/0x9777 HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "Content-Length: 57\r\n"
                           "\r\n"
                           R"({"pk":"pz_jr84r5yc-ViuR-_Djst2C6ikRQ3l3TQBfmSaC2O9QWVUAl3cAgCJtI2HVpFog66bezRfVdcso4AeA82wlRgA","sig":"OcTd6Oa93sNeQpZVoN4sd7BOGGnRxfyDJnuitYpOr_g8dtGcgAX8XH2g7klAD50vhrl299NyEgGEG2FTqIscgwA"})",
            .gas = 1000,
            .appAccessList = {arg_app_id_g},
            .memoryAccessMap = {
                    {arg_app_id_g},
                    {{{{0x9777000000000000, 0}},
                             {
                                     {{-1, 0}, {{67, Access::writable, 0}, {67, Access::writable, 0}}},
                             }}}}
    };

    Page p(46);
    auto newChunkID = full_id(arg_app_id_g, {0x9777000000000000, 0});
    ChunkIndex index({}, {{newChunkID, &p}}, {{newChunkID},
                                              {{67, 0}}}, 4);

    RequestScheduler scheduler(1, index, appIndex);

    scheduler.addRequest(std::move(createReq));
    scheduler.finalizeRequest(0);
    scheduler.buildExecDag();

    Executor executor;
    auto response = executor.executeOne(scheduler.nextRequest());

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response.httpResponse.c_str());

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ((std::string) *p.getNative(),
              "size: 67, capacity: 67, content: 0x[ 8 0 a7 3f e3 af ce 2b e7 27 3e 56 2b 91 fb f0 e3 b2 dd 82 ea 29 11 43 79 77 4d 0 5f 99 26 82 d8 ef 50 59 55 0 97 77 0 80 22 6d 23 61 d5 a4 5a 20 eb a6 de cd 17 d5 75 cb 28 e0 7 80 f3 6c 25 46 0 ]");
}

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

TEST_F(ArgAppTest, TwoTransfers) {
    std::vector<AppRequestInfo> requests{
            {
                    .id = 0,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ab HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1234,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
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
                                    R"({"to":0xaabc000000000000,"amount":1234,"forApp":0x100000000000000,"nonce":11})",
                                    util::Signature(
                                            "ET9Xz7GfWu87d4eKnFM6lEtfxUunEZSPD6H0WNZRC2qNvXntEVlLqjfmfeKiiu8eEeVXGiOzF0XvYSkwsZGs8gA"
                                    )
                            },
                    }

            },
            {
                    .id = 1,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0xd10000 HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":556677,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0xaabc000000000000, 0}, {0xd100000000000000, 0}},
                                     {
                                             {{-3, 0, 67}, {{1, Access::writable, 1}, {2, Access::read_only, 1}, {8, Access::int_additive, 1}}},
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 1}, {2, Access::writable, 1}, {65, Access::read_only, 1}, {8, Access::writable, 1}}},

                                     }}}},
                    .signedMessagesList {
                            {
                                    0xd100000000000000,
                                    R"({"to":0xaabc000000000000,"amount":556677,"forApp":0x100000000000000,"nonce":255})",
                                    util::Signature(
                                            "MlzLSpbWl9kGUjH3FnMV-oD_ykknkDHi8ayBOlQGIl83icEFgeNxhtO58LU1QFVwWX4-cyKpclDid1W88RzdHgA"
                                    )
                            },
                    }

            }

    };

    Page page_1(777);
    Page page_2(777);
    VarLenFullID fromPageID_1(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xab, 0x0}));
    VarLenFullID fromPageID_2(std::unique_ptr<byte[]>(new byte[5]{0x1, 0xd1, 0x0, 0x0, 0x0}));

    page_1.applyDelta(fromPageID_1,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 69, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              30, 55},
                              .finalDigest = {}},
                      780);

    page_2.applyDelta(fromPageID_2,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 70, 255, 0,
                              // pk:
                                              114, 22, 249, 146, 208, 168, 123, 133, 195, 106, 1, 214, 151, 30, 120,
                                              212, 68, 137, 15, 44, 196, 26, 36, 117, 77, 117, 168, 34, 255, 48, 33, 2,
                                              74, 202, 10, 109, 73, 202, 157, 101, 14, 123, 31, 252, 170, 225, 59, 19,
                                              145, 115, 30, 178, 169, 157, 92, 103, 183, 224, 249, 75, 147, 36, 18, 64,
                                              1,
                              // pk end
                                              0, 20, 20},
                              .finalDigest = {}},
                      780);

    Page page_to(777);
    VarLenFullID toPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0xaa, 0xbc, 0x0}));
    page_to.applyDelta(toPageID,
                       {{
                                0,
                                67 + 8, 1, 2, 9, 9,
                                66, 2, 1, 1
                        },
                        {}},
                       780);

    ChunkIndex index({}, {
                             {full_id(arg_app_id_g, {0x95ab000000000000, 0}), &page_1},
                             {full_id(arg_app_id_g, {0xd100000000000000, 0}), &page_2},
                             {full_id(arg_app_id_g, {0xaabc000000000000, 0}), &page_to}
                     },
                     {}, 4);
    BENCHMARK_ONCE(
            RequestProcessor processor(index, appIndex, 2, 3);

            processor.loadRequests<FakeStream>({
                                                       {0, 1, requests},
                                                       {1, 2, requests},
                                               });
#if defined(ASCEE_SERIAL_EXEC)
            auto response = processor.serialExecuteRequests<Executor>();, "serial two transfers:");
#else
    processor.checkDependencyGraph();
    auto response = processor.parallelExecuteRequests<Executor>();
    , "parallel two transfers:");
#endif


    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[0].httpResponse.c_str());
    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[1].httpResponse.c_str());

    EXPECT_EQ(response[0].statusCode, 200);
    EXPECT_EQ(response[1].statusCode, 200);

    EXPECT_EQ((std::string) *page_1.getNative(),
              "size: 75, capacity: 75, content: 0x[ c 0 a7 3f e3 af ce 2b e7 27 3e 56 2b 91 fb f0 e3 b2 dd 82 ea 29 11 43 79 77 4d 0 5f 99 26 82 d8 ef 50 59 55 0 97 77 0 80 22 6d 23 61 d5 a4 5a 20 eb a6 de cd 17 d5 75 cb 28 e0 7 80 f3 6c 25 46 0 4c 32 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getNative(),
              "size: 75, capacity: 75, content: 0x[ 0 1 72 16 f9 92 d0 a8 7b 85 c3 6a 1 d6 97 1e 78 d4 44 89 f 2c c4 1a 24 75 4d 75 a8 22 ff 30 21 2 4a ca a 6d 49 ca 9d 65 e 7b 1f fc aa e1 3b 13 91 73 1e b2 a9 9d 5c 67 b7 e0 f9 4b 93 24 12 40 1 7b 95 b 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_to.getNative(),
              "size: 75, capacity: 75, content: 0x[ 9 9 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 58 84 8 0 0 0 0 0 ]");
}


TEST_F(ArgAppTest, FourTransfers) {
    std::vector<AppRequestInfo> requests{
            {
                    .id = 0,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ab HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1234,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
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
                                    R"({"to":0xaabc000000000000,"amount":1234,"forApp":0x100000000000000,"nonce":11})",
                                    util::Signature(
                                            "ET9Xz7GfWu87d4eKnFM6lEtfxUunEZSPD6H0WNZRC2qNvXntEVlLqjfmfeKiiu8eEeVXGiOzF0XvYSkwsZGs8gA"
                                    )
                            },
                    }

            },
            {
                    .id = 1,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0xd10000 HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":556677,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0xaabc000000000000, 0}, {0xd100000000000000, 0}},
                                     {
                                             {{-3, 0, 67}, {{1, Access::writable, 1}, {2, Access::read_only, 1}, {8, Access::int_additive, 1}}},
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 1}, {2, Access::writable, 1}, {65, Access::read_only, 1}, {8, Access::writable, 1}}},

                                     }}}},
                    .signedMessagesList {
                            {
                                    0xd100000000000000,
                                    R"({"to":0xaabc000000000000,"amount":556677,"forApp":0x100000000000000,"nonce":255})",
                                    util::Signature(
                                            "MlzLSpbWl9kGUjH3FnMV-oD_ykknkDHi8ayBOlQGIl83icEFgeNxhtO58LU1QFVwWX4-cyKpclDid1W88RzdHgA"
                                    )
                            },
                    }

            },
            {
                    .id = 2,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ac HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1234,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0x95ac000000000000, 0}, {0xaabc000000000000, 0}},
                                     {
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 0}, {2, Access::writable, 0},
                                                                      {65, Access::read_only, 0}, {8, Access::writable, 0}}},
                                             {{-3, 0, 67}, {{1, Access::writable, 0}, {2, Access::read_only, 0}, {8, Access::int_additive, 0}}},

                                     }}}},
                    .signedMessagesList {
                            {
                                    0x95ac000000000000,
                                    R"({"to":0xaabc000000000000,"amount":1234,"forApp":0x100000000000000,"nonce":11})",
                                    util::Signature(
                                            "ET9Xz7GfWu87d4eKnFM6lEtfxUunEZSPD6H0WNZRC2qNvXntEVlLqjfmfeKiiu8eEeVXGiOzF0XvYSkwsZGs8gA"
                                    )
                            },
                    }
            },
            {
                    .id = 3,
                    .calledAppID = arg_app_id_g,
                    .httpRequest = "PATCH /balances/0x95ad HTTP/1.1\r\n"
                                   "Content-Type: application/json; charset=utf-8\r\n"
                                   "Content-Length: 57\r\n"
                                   "\r\n"
                                   R"({"to":0xaabc,"amount":1234,"sig":0})",
                    .gas = 1000,
                    .appAccessList = {arg_app_id_g},
                    .memoryAccessMap = {
                            {arg_app_id_g},
                            {{{{0x95ad000000000000, 0}, {0xaabc000000000000, 0}},
                                     {
                                             {{-3, 0, 2, 67}, {{1, Access::writable, 0}, {2, Access::writable, 0},
                                                                      {65, Access::read_only, 0}, {8, Access::writable, 0}}},
                                             {{-3, 0, 67}, {{1, Access::writable, 0}, {2, Access::read_only, 0}, {8, Access::int_additive, 0}}},

                                     }}}},
                    .signedMessagesList {
                            {
                                    0x95ad000000000000,
                                    R"({"to":0xaabc000000000000,"amount":1234,"forApp":0x100000000000000,"nonce":11})",
                                    util::Signature(
                                            "ET9Xz7GfWu87d4eKnFM6lEtfxUunEZSPD6H0WNZRC2qNvXntEVlLqjfmfeKiiu8eEeVXGiOzF0XvYSkwsZGs8gA"
                                    )
                            },
                    }
            },
    };

    Page page_1(777);
    Page page_2(777);
    Page page_3(777);
    Page page_4(777);
    VarLenFullID fromPageID_1(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xab, 0x0}));
    VarLenFullID fromPageID_2(std::unique_ptr<byte[]>(new byte[5]{0x1, 0xd1, 0x0, 0x0, 0x0}));
    VarLenFullID fromPageID_3(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xac, 0x0}));
    VarLenFullID fromPageID_4(std::unique_ptr<byte[]>(new byte[4]{0x1, 0x95, 0xad, 0x0}));

    page_1.applyDelta(fromPageID_1,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 69, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              30, 55},
                              .finalDigest = {}},
                      780);

    page_2.applyDelta(fromPageID_2,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 70, 255, 0,
                              // pk:
                                              114, 22, 249, 146, 208, 168, 123, 133, 195, 106, 1, 214, 151, 30, 120,
                                              212, 68, 137, 15, 44, 196, 26, 36, 117, 77, 117, 168, 34, 255, 48, 33, 2,
                                              74, 202, 10, 109, 73, 202, 157, 101, 14, 123, 31, 252, 170, 225, 59, 19,
                                              145, 115, 30, 178, 169, 157, 92, 103, 183, 224, 249, 75, 147, 36, 18, 64,
                                              1,
                              // pk end
                                              0, 20, 20},
                              .finalDigest = {}},
                      780);

    page_3.applyDelta(fromPageID_3,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 69, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              30, 55},
                              .finalDigest = {}},
                      780);

    page_4.applyDelta(fromPageID_4,
                      Page::Delta{.content = {0,
                                              67 + 8, 1, 69, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              30, 55},
                              .finalDigest = {}},
                      780);

    Page page_to(777);
    VarLenFullID toPageID(std::unique_ptr<byte[]>(new byte[4]{0x1, 0xaa, 0xbc, 0x0}));
    page_to.applyDelta(toPageID,
                       {{
                                0,
                                67 + 8, 1, 2, 9, 9,
                                66, 2, 1, 1
                        },
                        {}},
                       780);

    ChunkIndex index({}, {
                             {full_id(arg_app_id_g, {0x95ab000000000000, 0}), &page_1},
                             {full_id(arg_app_id_g, {0xd100000000000000, 0}), &page_2},
                             {full_id(arg_app_id_g, {0x95ac000000000000, 0}), &page_3},
                             {full_id(arg_app_id_g, {0x95ad000000000000, 0}), &page_4},
                             {full_id(arg_app_id_g, {0xaabc000000000000, 0}), &page_to}
                     },
                     {}, 3);
    RequestProcessor processor(index, appIndex, requests.size(), 2);

    processor.loadRequests<FakeStream>({
                                               {0, 2, requests},
                                               {2, 4, requests},
                                       });
    BENCHMARK_ONCE(
#if defined(ASCEE_SERIAL_EXEC)
            auto response = processor.serialExecuteRequests<Executor>();, "serial 4 transfers:");
#else
    processor.checkDependencyGraph();
    auto response = processor.parallelExecuteRequests<Executor>();
    , "parallel 4 transfers:");
#endif


    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[0].httpResponse.c_str());
    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[1].httpResponse.c_str());
    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[2].httpResponse.c_str());
    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response[3].httpResponse.c_str());

    EXPECT_EQ(response[0].statusCode, 200);
    EXPECT_EQ(response[1].statusCode, 200);

    EXPECT_EQ((std::string) *page_1.getNative(),
              "size: 75, capacity: 75, content: 0x[ c 0 a7 3f e3 af ce 2b e7 27 3e 56 2b 91 fb f0 e3 b2 dd 82 ea 29 11 43 79 77 4d 0 5f 99 26 82 d8 ef 50 59 55 0 97 77 0 80 22 6d 23 61 d5 a4 5a 20 eb a6 de cd 17 d5 75 cb 28 e0 7 80 f3 6c 25 46 0 4c 32 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getNative(),
              "size: 75, capacity: 75, content: 0x[ 0 1 72 16 f9 92 d0 a8 7b 85 c3 6a 1 d6 97 1e 78 d4 44 89 f 2c c4 1a 24 75 4d 75 a8 22 ff 30 21 2 4a ca a 6d 49 ca 9d 65 e 7b 1f fc aa e1 3b 13 91 73 1e b2 a9 9d 5c 67 b7 e0 f9 4b 93 24 12 40 1 7b 95 b 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_to.getNative(),
              "size: 75, capacity: 75, content: 0x[ 9 9 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 fc 8d 8 0 0 0 0 0 ]");
}
