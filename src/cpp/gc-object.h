#pragma once

#include <unordered_set>
#include <memory>

namespace effil {

// Unique handle for all objects spawned from one object.
using GCHandle = void*;

// Mock handle for non gc objects
static const GCHandle GCNull = nullptr;

// GCObject interface represents beheiviour of object.
// Multiple views may hold shred instance of Impl.
class BaseGCObject {
public:
    virtual ~BaseGCObject() = default;
    virtual GCHandle handle() const = 0;
    virtual size_t instances() const = 0;
    virtual std::unordered_set<GCHandle> refers() const = 0;
};

template<typename Impl>
class GCObject : public BaseGCObject {
public:
    GCObject() : ctx_(std::make_shared<Impl>())
    {}

    // All views are copy constructable
    GCObject(const GCObject&) = default;
    GCObject& operator=(const GCObject&) = default;

    // Unique handle for any copy of GCData in any lua state
    GCHandle handle() const final {
        return reinterpret_cast<GCHandle>(ctx_.get());
    }

    // Number of instances
    // always greater than 1
    // GC holds one copy
    size_t instances() const final {
        return ctx_.use_count();
    }

    std::unordered_set<GCHandle> refers() const {
        return ctx_->refers();
    }

protected:
    std::shared_ptr<Impl> ctx_;
};

} // namespace effil
