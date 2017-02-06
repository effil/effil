#include "garbage-collector.h"

#include "utils.h"

#include <vector>
#include <cassert>

namespace effil {

GarbageCollector::GarbageCollector() noexcept
        : state_(GCState::Idle),
          lastCleanup_(0),
          step_(200) {}

GCObject* GarbageCollector::get(GCObjectHandle handle) noexcept {
    std::lock_guard<std::mutex> g(lock_);
    auto it = objects_.find(handle);
    if (it == objects_.end()) {
        DEBUG << "Null handle " << handle << std::endl;
        return nullptr;
    }
    return it->second.get();
}

bool GarbageCollector::has(GCObjectHandle handle) const noexcept {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.find(handle) != objects_.end();
}

// Here is the naive tri-color marking
// garbage collecting algorithm implementation.
void GarbageCollector::cleanup() {
    std::lock_guard<std::mutex> g(lock_);

    if (state_ == GCState::Stopped) return;
    assert(state_ != GCState::Running);
    state_ = GCState::Running;

    std::vector<GCObjectHandle> grey;
    std::map<GCObjectHandle, std::shared_ptr<GCObject>> black;

    for(const auto& handleAndObject : objects_)
        if (handleAndObject.second->instances() > 1)
            grey.push_back(handleAndObject.first);

    while(!grey.empty()) {
        GCObjectHandle handle = grey.back();
        grey.pop_back();

        auto object = objects_[handle];
        black[handle] = object;
        for(GCObjectHandle refHandle : object->refers())
            if (black.find(refHandle) == black.end())
                grey.push_back(refHandle);
    }

    DEBUG << "Removing " << (objects_.size() - black.size())
          << " out of " << objects_.size() << std::endl;
    // Sweep phase
    objects_ = std::move(black);

    state_ = GCState::Idle;
    lastCleanup_.store(0);
}

size_t GarbageCollector::size() const noexcept {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

void GarbageCollector::stop() noexcept {
    std::lock_guard<std::mutex> g(lock_);
    assert(state_ == GCState::Idle || state_ == GCState::Stopped);
    state_ = GCState::Stopped;
}

void GarbageCollector::resume() noexcept {
    std::lock_guard<std::mutex> g(lock_);
    assert(state_ == GCState::Idle || state_ == GCState::Stopped);
    state_ = GCState::Idle;
}


GarbageCollector& getGC() noexcept {
    static GarbageCollector pool;
    return pool;
}

} // effil