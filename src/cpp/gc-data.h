#pragma once

#include "spin-mutex.h"

#include <unordered_set>

namespace effil {

// Unique handle for all objects spawned from one object.
using GCHandle = void*;

// Mock handle for non gc objects
static const GCHandle GCNull = nullptr;

// Base class for data represented in Lua.
// Derived classes always managed by corresponding views.
class GCData {
public:
    GCData() = default;
    virtual ~GCData() = default;

    // List of weak references to nested objects
    std::unordered_set<GCHandle> refers() const;
    void addReference(GCHandle handle);
    void removeReference(GCHandle handle);

public:
    GCData(const GCData&) = delete;
    GCData& operator=(const GCData&) = delete;

private:
    mutable SpinMutex mutex_;
    std::unordered_multiset<GCHandle> weakRefs_;
};

} // namespace effil
