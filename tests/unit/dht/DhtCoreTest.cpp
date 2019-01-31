/**
 * Copyright 2019 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials,
 * and your use of them is governed by the express license under which they
 * were provided to you (Intel OBL Internal Use License).
 * Unless the License provides otherwise, you may not use, modify, copy,
 * publish, distribute, disclose or transmit this software or the related
 * documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no
 * express or implied warranties, other than those that are expressly
 * stated in the License.
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
