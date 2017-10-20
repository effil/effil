#pragma once

#include "spin-mutex.h"
#include "view.h"
#include <unordered_set>

namespace effil {

// Base class for data represented in Lua.
// Derived classes always managed by corresponding views.
class BaseImpl {
public:
    BaseImpl() = default;
    virtual ~BaseImpl() = default;

    // List of weak references to nested objects
    std::unordered_set<GCHandle> refers() const;

public://protected:
    void addReference(GCHandle handle);
    void removeReference(GCHandle handle);

public:
    BaseImpl(const BaseImpl&) = delete;
    BaseImpl& operator=(const BaseImpl&) = delete;

private:
    mutable SpinMutex mutex_;
    std::unordered_multiset<GCHandle> weakRefs_;
};

} // namespace effil