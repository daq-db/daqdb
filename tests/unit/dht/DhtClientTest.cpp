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

#include "../../lib/dht/DhtClient.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <fakeit.hpp>

namespace ut = boost::unit_test;

using namespace std;
using namespace fakeit;
using namespace DaqDB;

/**
 * Test verified if enqueueAndWait function receives correct data when called
 * from dhtClient.get(key).
 */
BOOST_AUTO_TEST_CASE(VerifyEnqueueRequestOnGet) {
    const string expectedKey = "1234";
    const string expectedValue = "4321";
    Mock<DhtClient> dhtClientMock;

    DhtClient &dhtClient = dhtClientMock.get();
    When(Method(dhtClientMock, resizeMsgBuffers)).AlwaysReturn();
    When(Method(dhtClientMock, fillReqMsg)).AlwaysReturn();

    DhtNode targetNode;
    When(Method(dhtClientMock, getTargetHost)).AlwaysReturn(&targetNode);

    Value val(const_cast<char *>(expectedValue.c_str()), expectedKey.size());
    DhtReqCtx reqCtx;
    reqCtx.value = &val;
    When(Method(dhtClientMock, enqueueAndWait))
        .Do([&](DhtNode *targetHost, ErpRequestType type,
                DhtContFunc contFunc) {
            BOOST_CHECK(targetHost);
            BOOST_CHECK(type == ErpRequestType::ERP_REQUEST_GET);
            dhtClient.setReqCtx(reqCtx);
        });

    Key key(const_cast<char *>(expectedKey.c_str()), expectedKey.size());
    auto result = dhtClient.get(key);
    BOOST_CHECK_EQUAL(result.size(), expectedValue.size());
    BOOST_CHECK(strcmp(result.data(), val.data()) == 0);
}

/**
 * Test verified if enqueueAndWait function receives correct data when called
 * from dhtClient.put(key, val).
 */
BOOST_AUTO_TEST_CASE(VerifyEnqueueRequestOnPut) {
    const string expectedKey = "1234";
    const string expectedValue = "4321";
    Mock<DhtClient> dhtClientMock;

    DhtClient &dhtClient = dhtClientMock.get();

    When(Method(dhtClientMock, resizeMsgBuffers)).AlwaysReturn();

    When(Method(dhtClientMock, fillReqMsg))
        .Do([&](const Key *key, const Value *value) {
            BOOST_CHECK(strcmp(key->data(), expectedKey.c_str()) == 0);
            BOOST_CHECK(strcmp(value->data(), expectedValue.c_str()) == 0);
            BOOST_CHECK_EQUAL(value->size(), expectedValue.size());
        });
    
    When(Method(dhtClientMock, enqueueAndWait))
        .Do([&](DhtNode *targetHost, ErpRequestType type, 
                DhtContFunc contFunc){
            BOOST_CHECK(targetHost);
            BOOST_CHECK(type == ErpRequestType::ERP_REQUEST_PUT);
        });

    DhtNode targetNode;
    When(Method(dhtClientMock, getTargetHost)).AlwaysReturn(&targetNode);
    
    Value val(const_cast<char *>(expectedValue.c_str()), expectedValue.size());
    Key key(const_cast<char *>(expectedKey.c_str()), expectedKey.size());
    dhtClient.put(key, val);
}

/**
 * Test verified if enqueueAndWait function receives correct data when called
 * from dhtClient.remove(key).
 */
BOOST_AUTO_TEST_CASE(VerifyEnqueueRequestOnRemove) {
    const string expectedKey = "1234";
    Mock<DhtClient> dhtClientMock;

    DhtClient &dhtClient = dhtClientMock.get();
    When(Method(dhtClientMock, resizeMsgBuffers)).AlwaysReturn();
    When(Method(dhtClientMock, fillReqMsg)).AlwaysReturn();

    DhtNode targetNode;
    When(Method(dhtClientMock, getTargetHost)).AlwaysReturn(&targetNode);

    When(Method(dhtClientMock, enqueueAndWait))
        .Do([&](DhtNode *targetHost, ErpRequestType type, DhtContFunc contFunc){
            BOOST_CHECK(targetHost);
            BOOST_CHECK(type == ErpRequestType::ERP_REQUEST_REMOVE);
        });
    
    Key key(const_cast<char *>(expectedKey.c_str()), expectedKey.size());
    dhtClient.remove(key);
}

/**
 * Test verified if resizeMsgBuffer function receives correct data when called
 * from dhtClient.get(key).
 */
BOOST_AUTO_TEST_CASE(VerifyResizeMsgBufferOnGet) {
    const string expectedKey = "1234";
    const string expectedValue = "4321";
    Mock<DhtClient> dhtClientMock;

    DhtClient &dhtClient = dhtClientMock.get();

    When(Method(dhtClientMock, resizeMsgBuffers))
        .Do([&](size_t new_request_size, size_t new_response_size) {
            BOOST_CHECK_EQUAL(new_request_size, 32 * 1024);
            BOOST_CHECK_EQUAL(new_response_size, 32 * 1024);
    });

    When(Method(dhtClientMock, fillReqMsg)).AlwaysReturn();

    DhtNode targetNode;
    When(Method(dhtClientMock, getTargetHost)).AlwaysReturn(&targetNode);

    Value val(const_cast<char *>(expectedValue.c_str()), expectedKey.size());
    DhtReqCtx reqCtx;
    reqCtx.value = &val;
    When(Method(dhtClientMock, enqueueAndWait))
        .Do([&](DhtNode *targetHost, ErpRequestType type, DhtContFunc contFunc) {
            dhtClient.setReqCtx(reqCtx);
        });

    Key key(const_cast<char *>(expectedKey.c_str()), expectedKey.size());
    dhtClient.get(key);
}