/*
 * Copyright 2010-2020 DatasysLab@iit.edu(http://datasys.cs.iit.edu/index.html)
 *      Director: Ioan Raicu(iraicu@cs.iit.edu)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file is part of ZHT
 * library(http://datasys.cs.iit.edu/projects/ZHT/index.html). Tonglin
 * Li(tli13@hawk.iit.edu) with nickname Tony, Xiaobing
 * Zhou(xzhou40@hawk.iit.edu) with nickname Xiaobingo, Ke
 * Wang(kwang22@hawk.iit.edu) with nickname KWang, Dongfang
 * Zhao(dzhao8@@hawk.iit.edu) with nickname DZhao, Ioan
 * Raicu(iraicu@cs.iit.edu).
 *
 * Const-impl.h
 *
 *  Created on: Feb 13, 2013
 *      Author: Xiaobingo
 *      Contributor: Tony, KWang, DZhao
 */

/**
 * Copyright 2018 Intel Corporation.
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

#ifndef CONST_IMPL_H_
#define CONST_IMPL_H_

#include "Const.h"

#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

namespace iit {
namespace datasys {
namespace zht {
namespace dm {

template <class TYPE> int Const::toInt(const TYPE &ele) {

    return atoi(toString(ele).c_str());
}

template <class TYPE> uint64_t Const::toUInt64(const TYPE &ele) {

    return strtoull(toString(ele).c_str(), NULL, 10);
}

template <class TYPE> string Const::toString(const TYPE &ele) {

    stringstream ss;
    ss << ele;

    return ss.str();
}

template <class TYPE1, class TYPE2>
string Const::concat(const TYPE1 &ele1, const TYPE2 &ele2) {

    stringstream ss;
    ss << ele1;
    ss << ele2;

    return ss.str();
}

template <class TYPE1, class TYPE2>
string Const::concat(const TYPE1 &ele1, const string &delimiter,
                     const TYPE2 &ele2) {

    stringstream ss;
    ss << ele1;
    ss << delimiter;
    ss << ele2;

    return ss.str();
}

} /* namespace dm */
} /* namespace zht */
} /* namespace datasys */
} /* namespace iit */
#endif /* CONST_IMPL_H_ */
