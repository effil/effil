#pragma once

#include "spin-mutex.h"

#include <unordered_set>

namespace effil {

// Unique handle for all objects spawned from one object.
using GCObjectHandle = void*;

static const GCObjectHandle GCNull = nullptr;

// All effil objects that owned in lua code have to inherit this class.
// This type o object can persist in multiple threads and in multiple lua states.
// Childes have to care about storing data, concurrent access and
// weak references (GCHandle) to other GCObjects.
class GCObject {
public:
    GCObject();
    virtual ~GCObject() = default;

    // Unique handle for any copy of GCObject in any lua state
    GCObjectHandle handle() const;

    // Number of instance copies
    // always greater than 1
    // GC holds one copy
    size_t instances() const;

    // List of weak references to nested objects
    const std::unordered_set<GCObjectHandle> refers() const;

protected:
    void addReference(GCObjectHandle handle);
    void removeReference(GCObjectHandle handle);

private:
    struct SharedData {
        mutable SpinMutex mutex_;
        std::unordered_multiset<GCObjectHandle> weakRefs_;
    };

    std::shared_ptr<SharedData> data_;
};

} // namespace effil