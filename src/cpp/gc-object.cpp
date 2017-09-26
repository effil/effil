#include "gc-object.h"

#include <cassert>

namespace effil {

GCObject::GCObject()
        : data_(std::make_shared<SharedData>()) {}

GCObjectHandle GCObject::handle() const {
    return reinterpret_cast<GCObjectHandle>(data_.get());
}

size_t GCObject::instances() const {
    return data_.use_count();
}

const std::unordered_set<GCObjectHandle> GCObject::refers() const {
    std::lock_guard<SpinMutex> lock(data_->mutex_);
    return std::unordered_set<GCObjectHandle>(
            data_->weakRefs_.begin(),
            data_->weakRefs_.end());
}

void GCObject::addReference(GCObjectHandle handle) {
    if (handle == nullptr) return;

    std::lock_guard<SpinMutex> lock(data_->mutex_);
    data_->weakRefs_.insert(handle);
}

void GCObject::removeReference(GCObjectHandle handle) {
    if (handle == GCNull) return;

    std::lock_guard<SpinMutex> lock(data_->mutex_);
    auto hit = data_->weakRefs_.find(handle);
    assert(hit != std::end(data_->weakRefs_));
    data_->weakRefs_.erase(hit);
}

} // namespace effil