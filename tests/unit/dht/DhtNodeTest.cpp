/**
 * Copyright 2017-2018 Intel Corporation.
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

#include "../../lib/dht/DhtNode.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

namespace ut = boost::unit_test;

BOOST_AUTO_TEST_CASE(NodeConstr) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    BOOST_CHECK(dhtNode->state == DaqDB::DhtNodeState::NODE_INIT);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(SessionId) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setSessionId(123);

    BOOST_CHECK_EQUAL(dhtNode->getSessionId(), 123);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(NodeAddr) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setIp("10.1.0.1");
    dhtNode->setPort(50000);

    BOOST_CHECK(dhtNode->getIp() == "10.1.0.1");
    BOOST_CHECK_EQUAL(dhtNode->getPort(), 50000);

    BOOST_CHECK(dhtNode->getUri() == "10.1.0.1:50000");

    BOOST_CHECK(dhtNode->getClientUri() == "10.1.0.1:" + 
        std::to_string(50000 + DaqDB::ERPC_CLIENT_PORT_ADDITION));

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(MaskLength) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setMaskLength(24);

    BOOST_CHECK_EQUAL(dhtNode->getMaskLength(), 24);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(MaskOffset) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setMaskOffset(5);

    BOOST_CHECK_EQUAL(dhtNode->getMaskOffset(), 5);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(Start) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setStart(5);

    BOOST_CHECK_EQUAL(dhtNode->getStart(), 5);

    delete dhtNode;
}

BOOST_AUTO_TEST_CASE(End) {
    DaqDB::DhtNode *dhtNode = new DaqDB::DhtNode();

    dhtNode->setEnd(10);

    BOOST_CHECK_EQUAL(dhtNode->getEnd(), 10);

    delete dhtNode;
}