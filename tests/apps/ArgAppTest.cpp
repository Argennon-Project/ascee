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
#include "arg/info.h"
#include "ascee/executor/Executor.h"
#include "storage/Page.h"
#include "storage/ChunkIndex.h"
#include "validator/RequestScheduler.h"

using namespace argennon;
using namespace asa;
using namespace ave;
using namespace ascee::runtime;

using Access = BlockAccessInfo::Access::Type;

class ArgAppTest : public ::testing::Test {
protected:
public:
    ArgAppTest() {
        AppLoader::global = std::make_unique<AppLoader>("../apps");
        Executor::initialize();
    }
};

TEST_F(ArgAppTest, SimpleTransfer) {
    AppRequestInfo transferReq{
            .id = 0,
            .calledAppID = arg_app_id_g,
            .httpRequest = "PATCH /balances/0x14ab HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "Content-Length: 57\r\n"
                           "\r\n"
                           R"({"to":0xabc,"amount":1399,"sig":"ZnhvriwVgyOMevxtB6bDV4t4qPPQRusg7oOSDjpSWCtTq0cM7qwsb9e_GLqZbCfsjh8BM2UlBzuW_Hem05ugkAE"})",
            .gas = 1000,
            .appAccessList = {arg_app_id_g},
            .memoryAccessMap = {
                    {arg_app_id_g},
                    {{{0x0abc000000000000, 0x0abc000000000001, 0x14ab000000000000, 0x14ab000000000001},
                             {
                                     {{-3, 0}, {{1, Access::writable, 0}, {2, Access::check_only, 0}}},
                                     {{-3, 0}, {{1, Access::read_only, 0}, {8, Access::int_additive, 0}}},
                                     {{-3, 0, 2}, {{1, Access::writable, 0}, {2, Access::writable, 0}, {65, Access::read_only, 0}}},
                                     {{-3, 0}, {{1, Access::writable, 0}, {8, Access::writable, 0}}},
                             }}}}
    };

    Page page_1(777);
    page_1.addMigrant({arg_app_id_g, 0x14ab000000000001}, new Chunk());
    page_1.applyDelta({arg_app_id_g, 0x14ab000000000000},
                      Page::Delta{.content = {67, 1, 67, 11, 0,
                              // pk:
                                              167, 63, 227, 175, 206, 43, 231, 39, 62, 86, 43, 145, 251, 240, 227, 178,
                                              221, 130, 234, 41, 17, 67, 121, 119, 77, 0, 95, 153, 38, 130, 216, 239,
                                              80, 89, 85, 0, 151, 119, 0, 128, 34, 109, 35, 97, 213, 164, 90, 32, 235,
                                              166, 222, 205, 23, 213, 117, 203, 40, 224, 7, 128, 243, 108, 37, 70, 0,
                              // pk end
                                              0,
                                              8, 1, 2, 21, 20},
                              .finalDigest = {}},
                      780);

    Page page_2(777);
    page_2.addMigrant({arg_app_id_g, 0x0abc000000000001}, new Chunk());
    page_2.applyDelta({arg_app_id_g, 0x0abc000000000000},
                      {{
                               67, 1, 2, 45, 45,
                               0,
                               8, 1, 2, 1, 1
                       },
                       {}},
                      780);

    std::cout << (std::string) *page_1.getNative() << std::endl;
    std::cout << (std::string) *page_2.getNative() << std::endl;

    EXPECT_EQ((std::string) *page_1.getMigrants().at({arg_app_id_g, 0x14ab000000000001}),
              "size: 8, capacity: 8, content: 0x[ 15 14 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getMigrants().at({arg_app_id_g, 0x0abc000000000001}),
              "size: 8, capacity: 8, content: 0x[ 1 1 0 0 0 0 0 0 ]");
    auto content = page_1.getNative()->getContentPointer(0, 2).get(2);
    EXPECT_EQ(0xb, content[0]);
    EXPECT_EQ(0, content[1]);

    page_1.setWritableFlag(true);
    page_2.setWritableFlag(true);

    ChunkIndex index({
                             {full_id(arg_app_id_g, 0x14ab000000000000), &page_1},
                             {full_id(arg_app_id_g, 0x0abc000000000000), &page_2}
                     }, {}, 4);

    RequestScheduler scheduler(1, index);

    scheduler.addRequest(std::move(transferReq));
    scheduler.finalizeRequest(0);
    scheduler.buildExecDag();

    auto response = Executor::executeOne(scheduler.nextRequest());

    printf("<<<******* Response *******>>> \n%s\n<<<************************>>>\n", response.httpResponse.c_str());

    EXPECT_EQ(response.statusCode, 200);
    EXPECT_EQ((std::string) *page_1.getMigrants().at({arg_app_id_g, 0x14ab000000000001}),
              "size: 8, capacity: 8, content: 0x[ 9e e 0 0 0 0 0 0 ]");
    EXPECT_EQ((std::string) *page_2.getMigrants().at({arg_app_id_g, 0x0abc000000000001}),
              "size: 8, capacity: 8, content: 0x[ 78 6 0 0 0 0 0 0 ]");

    // checking nonce
    content = page_1.getNative()->getContentPointer(0, 2).get(2);
    EXPECT_EQ(0xc, content[0]);
    EXPECT_EQ(0, content[1]);
}
/*
TEST_F(ArgAppTest, SimpleCreateAcc) {
    AppRequestInfo createReq{
            .id = 1,
            .calledAppID = 0x1,
            .httpRequest = "PUT /balances/0x777 HTTP/1.1\r\n"
                           "Content-Type: application/json; charset=utf-8\r\n"
                           "Content-Length: 57\r\n"
                           "\r\n"
                           R"({"pk":"kjijdfienjfdsifjksdfjksdfsdfdjsfk","sig":"OcTd6Oa93sNeQpZVoN4sd7BOGGnRxfyDJnuitYpOr_g8dtGcgAX8XH2g7klAD50vhrl299NyEgGEG2FTqIscgwA"})",
            .gas = 1000,
            .appAccessList = {0x1},
            .memoryAccessMap = {
                    {0x1},
                    {{{0x777000000000000},
                             {
                                     {{-1, 0, 2}, {{67, Access::writable, 1}, {2, Access::check_only, 1}, {1, Access::writable, 1}}},
                             }}}}
    };

    AppLoader::global = std::make_unique<AppLoader>("apps");

    Page page11(777);
    page_1.addMigrant({0x1, 0x14ab000000000001}, new Chunk());



}*/
