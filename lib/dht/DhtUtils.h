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

#pragma once

#include <asio/io_service.hpp>

namespace FogKV
{
namespace utils
{

/*!
 * Returns free network port
 * @param io_service	boost io service
 * @param backbonePort	prefered port, if not used then function should
 * @param reuseAddr	allow to reuse an address that is already in use
 * return it as a result
 */
unsigned short getFreePort(asio::io_service &io_service,
			   const unsigned short backbonePort,
			   bool reuseAddr = false);

}
}
