#pragma once

#include "gc-object.h"
#include <sol.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace effil {

class GC {
public:
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

        std::lock_guard<std::mutex> g(lock_);
        objects_.emplace(object->handle(), std::move(object));
        return copy;
    }

    template <typename ObjectType>
    ObjectType get(GCHandle handle) {
        std::lock_guard<std::mutex> g(lock_);

        auto it = objects_.find(handle);
        assert(it != objects_.end());

        auto result = dynamic_cast<ObjectType*>(it->second.get());
        assert(result);
        return *result;
    }

private:
    mutable std::mutex lock_;
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
