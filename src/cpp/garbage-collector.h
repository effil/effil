#pragma once

#include "gc-object.h"
#include "spin-mutex.h"
#include <sol.hpp>
#include <unordered_map>
#include <shared_mutex>

namespace effil {

class GC {
public:
    typedef std::unique_lock<SpinMutex> UniqueLock;
    typedef std::shared_lock<SpinMutex> SharedLock;

    // global gc instance
    static GC& instance();
    static sol::table exportAPI(sol::state_view& lua);

    // This method is used to create all managed objects.
    template <typename ViewType, typename... Args>
    ViewType create(Args&&... args) {
        if (enabled_ && count() >= step_ * lastCleanup_)
            collect();

        std::unique_ptr<ViewType> object(new ViewType(std::forward<Args>(args)...));
        auto copy = *object;

        UniqueLock lock(lock_);
        objects_.emplace(object->handle(), std::move(object));
        return copy;
    }

    template <typename ObjectType>
    ObjectType get(GCHandle handle) {
        SharedLock lock(lock_);

        auto it = objects_.find(handle);
        assert(it != objects_.end());

        auto result = reinterpret_cast<ObjectType*>(it->second.get());
        assert(result);
        return *result;
    }

    StrongRef getRef(GCHandle handle) {
        SharedLock lock(lock_);

        auto it = objects_.find(handle);
        assert(it != objects_.end());

        return it->second->ref();
    }

private:
    mutable SpinMutex lock_;
    bool enabled_;
    std::atomic<uint64_t> lastCleanup_;
    double step_;
    std::unordered_map<GCHandle, std::unique_ptr<BaseGCObject>> objects_;

private:
    GC();
    GC(GC&&) = delete;
    GC(const GC&) = delete;

    void collect();
    void pause() { enabled_ = false; }
    void resume() { enabled_ = true; }
    double step() const { return step_; }
    void step(double newStep) { step_ = newStep; }
    bool enabled() { return enabled_; }
    size_t count() const;
};

} // effil
