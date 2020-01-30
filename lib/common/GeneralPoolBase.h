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

#include <mutex>
#include <shared_mutex>

typedef std::mutex Lock;
typedef std::unique_lock<Lock> WriteLock;
typedef std::unique_lock<Lock> ReadLock;

namespace DaqDB {
class GeneralPoolBase {
  public:
    GeneralPoolBase(unsigned short id_, const char *name_);
    virtual ~GeneralPoolBase();
    virtual void manage() = 0;
    virtual void dump() = 0;
    virtual void stats(FILE &file_) = 0;
    void setRttiName(const char *_rttiName);
    const unsigned short getId() const;
    const char *getName() const;
    const char *getRttiName() const;
    const FILE *getStackTraceFile() const;
    void setStackTraceFile(FILE *value);
    const unsigned short getUserId() const;

    unsigned short id;
    char name[32];
    char rttiName[128];
    FILE *stackTraceFile;

  private:
    GeneralPoolBase(const GeneralPoolBase &right);
    GeneralPoolBase &operator=(const GeneralPoolBase &right);

    static unsigned short idGener;
    unsigned short userId;
    Lock generMutex;
};

inline const unsigned short GeneralPoolBase::getId() const { return id; }

inline const char *GeneralPoolBase::getName() const { return name; }

inline const char *GeneralPoolBase::getRttiName() const { return rttiName; }

inline const FILE *GeneralPoolBase::getStackTraceFile() const {
    return stackTraceFile;
}

inline void GeneralPoolBase::setStackTraceFile(FILE *value) {
    stackTraceFile = value;
}

inline const unsigned short GeneralPoolBase::getUserId() const {
    return userId;
}
} // namespace DaqDB
