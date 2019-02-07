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

#include "../../lib/dht/DhtNode.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

namespace ut = boost::unit_test;

BOOST_AUTO_TEST_CASE(TestNodeConstr) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    BOOST_CHECK(dhtNode->state == DaqDB::DhtNodeState::NODE_INIT);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestSessionIdSetting) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    int expectedSessionId;

    dhtNode->setSessionId(expectedSessionId);

    BOOST_CHECK_EQUAL(dhtNode->getSessionId(), expectedSessionId);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestNodeAddr) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    const std::string expectedId = "10.1.0.1";
    unsigned short expectedPort = 50000;

    dhtNode->setIp(expectedId);
    dhtNode->setPort(expectedPort);

    BOOST_CHECK(dhtNode->getIp() == expectedId);
    BOOST_CHECK_EQUAL(dhtNode->getPort(), expectedPort);

    BOOST_CHECK(dhtNode->getUri() == expectedId + ":" + 
                std::to_string(expectedPort));

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestMaskLengthSetting) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    unsigned short expectedLength = 24;

    dhtNode->setMaskLength(expectedLength);

    BOOST_CHECK_EQUAL(dhtNode->getMaskLength(), expectedLength);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestMaskOffsetSetting) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    unsigned short expectedOffset = 5;

    dhtNode->setMaskOffset(expectedOffset);

    BOOST_CHECK_EQUAL(dhtNode->getMaskOffset(), expectedOffset);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestStartSetting) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    unsigned int expectedStart = 5;

    dhtNode->setStart(expectedStart);

    BOOST_CHECK_EQUAL(dhtNode->getStart(), expectedStart);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(TestEndSetting) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();
    unsigned int expectedEnd = 10;

    dhtNode->setEnd(expectedEnd);

    BOOST_CHECK_EQUAL(dhtNode->getEnd(), expectedEnd);

    delete dhtNode;
}