#pragma once

#include "spin-mutex.h"
#include <sol.hpp>
#include <mutex>
#include <map>
#include <set>

namespace effil {

// Unique handle for all objects spawned from one object.
typedef void* GCObjectHandle;

static const GCObjectHandle GCNull = nullptr;

// All effil objects that owned in lua code have to inherit this class.
// This type o object can persist in multiple threads and in multiple lua states.
// Child has to care about storing data and concurrent access.
class GCObject {
public:
    GCObject() noexcept
            : refs_(new std::set<GCObjectHandle>) {}
    GCObject(const GCObject& init) = default;
    GCObject(GCObject&& init) = default;
    GCObject& operator=(const GCObject& init) = default;
    virtual ~GCObject() = default;


    GCObjectHandle handle() const noexcept { return reinterpret_cast<GCObjectHandle>(refs_.get()); }
    size_t instances() const noexcept { return refs_.use_count(); }
    const std::set<GCObjectHandle>& refers() const { return *refs_; }

protected:
    std::shared_ptr<std::set<GCObjectHandle>> refs_;
};

class GC {
public:
    GC();
    ~GC() = default;

    // This method is used to create all managed objects.
    template <typename ObjectType, typename... Args>
    ObjectType create(Args&&... args) {
        if (enabled_ && lastCleanup_.fetch_add(1) == step_)
                collect();

        auto object = std::make_shared<ObjectType>(std::forward<Args>(args)...);

        std::lock_guard<std::mutex> g(lock_);
        objects_[object->handle()] = object;
        return *object;
    }

    template <typename ObjectType>
    ObjectType get(GCObjectHandle handle) {
        std::lock_guard<std::mutex> g(lock_);
        // TODO: add dynamic cast to check?
        return *static_cast<ObjectType*>(findObject(handle));
    }
    bool has(GCObjectHandle handle) const;

    void collect();
    size_t size() const;
    void pause() { enabled_ = false; };
    void resume() { enabled_ = true; };
    size_t step() const { return step_; }
    void step(size_t newStep) { step_ = newStep; }
    bool enabled() { return enabled_; };
    size_t count();

    static GC& instance();
    static sol::table getLuaApi(sol::state_view& lua);

private:
    mutable std::mutex lock_;
    bool enabled_;
    std::atomic<size_t> lastCleanup_;
    size_t step_;
    std::map<GCObjectHandle, std::shared_ptr<GCObject>> objects_;

private:
    GCObject* findObject(GCObjectHandle handle);

private:
    GC(GC&&) = delete;
    GC(const GC&) = delete;
};

} // effil
