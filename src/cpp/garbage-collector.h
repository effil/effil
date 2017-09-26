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
    template <typename ObjectType, typename... Args>
    ObjectType create(Args&&... args) {
        if (enabled_ && lastCleanup_.fetch_add(1) == step_)
                collect();

        auto object = std::make_unique<ObjectType>(std::forward<Args>(args)...);
        auto copy = *object;

        std::lock_guard<std::mutex> g(lock_);
        objects_[object->handle()] = std::move(object);
        return copy;
    }

    template <typename ObjectType>
    ObjectType get(GCObjectHandle handle) {
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
    std::atomic<size_t> lastCleanup_;
    size_t step_;
    std::unordered_map<GCObjectHandle, std::unique_ptr<GCObject>> objects_;

private:
    GC();
    GC(GC&&) = delete;
    GC(const GC&) = delete;

    void collect();
    size_t size() const;
    void pause() { enabled_ = false; };
    void resume() { enabled_ = true; };
    size_t step() const { return step_; }
    void step(size_t newStep) { step_ = newStep; }
    bool enabled() { return enabled_; };
    size_t count();
};

} // effil
