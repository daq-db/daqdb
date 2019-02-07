/**
 *  Copyright (c) 2019 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. 
 */

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include "../../lib/dht/DhtCore.cpp"
#include <fakeit.hpp>

namespace ut = boost::unit_test;

using namespace std;
using namespace fakeit;
using namespace DaqDB;

/**
 * Test verified if getClient function initializes DhtClient per caller thread.
 */
BOOST_AUTO_TEST_CASE(VerifyGetClient) {
    Mock<DhtCore> dhtCoreMock;
    Mock<DhtClient> dhtClientMock;

    DhtClient &dhtClient = dhtClientMock.get();
    DhtCore &dhtCore = dhtCoreMock.get();

    When(Method(dhtCoreMock, initClient)).AlwaysReturn();

    dhtCore.setClient(nullptr);
    dhtCore.getClient();
    Verify(Method(dhtCoreMock, initClient)).Exactly(1);

    dhtCore.setClient(&dhtClient);
    auto result = dhtCore.getClient();
    VerifyNoOtherInvocations(Method(dhtCoreMock, initClient));
    BOOST_CHECK_EQUAL(&dhtClient, result);
}
