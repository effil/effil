#include "impl.h"

#include <mutex>
#include <cassert>

namespace effil {

std::unordered_set<GCHandle> BaseImpl::refers() const {
    std::lock_guard<SpinMutex> lock(mutex_);
    return std::unordered_set<GCHandle>(
            weakRefs_.begin(),
            weakRefs_.end());
}

void BaseImpl::addReference(GCHandle handle) {
    if (handle == GCNull) return;

    std::lock_guard<SpinMutex> lock(mutex_);
    weakRefs_.insert(handle);
}

void BaseImpl::removeReference(GCHandle handle) {
    if (handle == GCNull) return;

    std::lock_guard<SpinMutex> lock(mutex_);
    auto hit = weakRefs_.find(handle);
    assert(hit != std::end(weakRefs_));
    weakRefs_.erase(hit);
}

} // namespace effil