#pragma once

#include <unordered_set>
#include <memory>

namespace effil {

// Unique handle for all objects spawned from one object.
using GCHandle = void*;

// Mock handle for non gc objects
static const GCHandle GCNull = nullptr;

// View interface represents beheiviour of object.
// Multiple views may hold shred instance of Impl.
class BaseView {
public:
    virtual ~BaseView() = default;
    virtual GCHandle handle() = 0;
    virtual size_t instances() const = 0;
    virtual std::unordered_set<GCHandle> refers() const = 0;
};

template<typename Impl>
class View : public BaseView {
public:
    View() : impl_(std::make_shared<Impl>())
    {}

    // All views are copy constructable
    View(const View&) = default;
    View& operator=(const View&) = default;

    // Unique handle for any copy of BaseImpl in any lua state
    GCHandle handle() final {
        return reinterpret_cast<GCHandle>(impl_.get());
    }

    // Number of instance copies
    // always greater than 1
    // GC holds one copy
    size_t instances() const final {
        return impl_.use_count();
    }

    std::unordered_set<GCHandle> refers() const {
        return impl_->refers();
    }

protected:
    std::shared_ptr<Impl> impl_;
};

} // namespace effil