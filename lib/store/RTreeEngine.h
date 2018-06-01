
#pragma once

typedef enum {                                             // status enumeration
    FAILED = -1,                                           // operation failed
    NOT_FOUND = 0,                                         // key not located
    OK = 1                                                 // successful completion
} KVStatus;

#include <string>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using std::string;
using std::to_string;

namespace FogKV
{

class RTreeEngine {
public:
    static RTreeEngine* Open(const string& engine,            // open storage engine
                          const string& path,              // path to persistent pool
                          size_t size);                    // size used when creating pool
    static void Close(RTreeEngine* kv);                       // close storage engine

    virtual string Engine() = 0;                           // engine identifier
    virtual KVStatus Get(int32_t limit,                    // copy value to fixed-size buffer
                         int32_t keybytes,
                         int32_t* valuebytes,
                         const char* key,
                         char* value) = 0;
    virtual KVStatus Get(const string& key,                // append value to std::string
                         string* value) = 0;
    virtual KVStatus Put(const string& key,                // copy value from std::string
                         const string& value) = 0;
    virtual KVStatus Remove(const string& key) = 0;        // remove value for key
    virtual KVStatus AllocValueForKey(const string& key, size_t size, char** value) = 0;
};
}
