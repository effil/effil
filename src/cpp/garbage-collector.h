#pragma once

#include "spin-mutex.h"
#include <sol.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace effil {

// Unique handle for all objects spawned from one object.
using GCObjectHandle = void*;

static const GCObjectHandle GCNull = nullptr;

// All effil objects that owned in lua code have to inherit this class.
// This type o object can persist in multiple threads and in multiple lua states.
// Child has to care about storing data and concurrent access.
class GCObject {
public:
    virtual ~GCObject() = default;

    // Unique identifier of gc object
    virtual GCObjectHandle handle() const = 0;

    // number of object copies present in all states
    virtual size_t instances() const = 0;

    // weak references to nested objects
    virtual const std::unordered_set<GCObjectHandle>& refers() const = 0;
};

class GC {
public:
    ~GC() = default;

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
        return *dynamic_cast<ObjectType*>(findObject(handle));
    }

    void collect();
    size_t size() const;
    void pause() { enabled_ = false; };
    void resume() { enabled_ = true; };
    size_t step() const { return step_; }
    void step(size_t newStep) { step_ = newStep; }
    bool enabled() { return enabled_; };
    size_t count();

    // global gc instance
    static GC& instance();
    static sol::table exportAPI(sol::state_view& lua);

private:
    mutable std::mutex lock_;
    bool enabled_;
    std::atomic<size_t> lastCleanup_;
    size_t step_;
    std::unordered_map<GCObjectHandle, std::unique_ptr<GCObject>> objects_;

private:
    GC();
    GCObject* findObject(GCObjectHandle handle);

private:
    GC(GC&&) = delete;
    GC(const GC&) = delete;
};

} // effil
