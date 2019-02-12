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

#pragma once

#include <asio/io_service.hpp>

namespace DaqDB
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
