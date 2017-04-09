#include "garbage-collector.h"

#include "utils.h"

#include <vector>
#include <cassert>

namespace effil {

GarbageCollector::GarbageCollector()
        : state_(GCState::Idle)
        , lastCleanup_(0)
        , step_(200) {}

GCObject* GarbageCollector::get(GCObjectHandle handle) {
    std::lock_guard<std::mutex> g(lock_);
    auto it = objects_.find(handle);
    if (it == objects_.end()) {
        DEBUG << "Null handle " << handle << std::endl;
        return nullptr;
    }
    return it->second.get();
}

bool GarbageCollector::has(GCObjectHandle handle) const {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.find(handle) != objects_.end();
}

// Here is the naive tri-color marking
// garbage collecting algorithm implementation.
void GarbageCollector::collect() {
    std::lock_guard<std::mutex> g(lock_);

    if (state_ == GCState::Paused)
        return;
    assert(state_ != GCState::Running);
    state_ = GCState::Running;

    std::vector<GCObjectHandle> grey;
    std::map<GCObjectHandle, std::shared_ptr<GCObject>> black;

    for (const auto& handleAndObject : objects_)
        if (handleAndObject.second->instances() > 1)
            grey.push_back(handleAndObject.first);

    while (!grey.empty()) {
        GCObjectHandle handle = grey.back();
        grey.pop_back();

        auto object = objects_[handle];
        black[handle] = object;
        for (GCObjectHandle refHandle : object->refers())
            if (black.find(refHandle) == black.end())
                grey.push_back(refHandle);
    }

    DEBUG << "Removing " << (objects_.size() - black.size()) << " out of " << objects_.size() << std::endl;
    // Sweep phase
    objects_ = std::move(black);

    state_ = GCState::Idle;
    lastCleanup_.store(0);
}

size_t GarbageCollector::size() const {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

void GarbageCollector::pause() {
    std::lock_guard<std::mutex> g(lock_);
    state_ = GCState::Paused;
}

void GarbageCollector::resume() {
    std::lock_guard<std::mutex> g(lock_);
    state_ = GCState::Idle;
}

size_t GarbageCollector::count() {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

GarbageCollector& getGC() {
    static GarbageCollector pool;
    return pool;
}

sol::object getLuaGCApi(sol::state_view& lua) {
    sol::table api = lua.create_table_with();
    api["collect"] = [=] {
        lua["collectgarbage"]();
        getGC().collect();
    };
    api["pause"] = [] { getGC().pause(); };
    api["resume"] = [] { getGC().resume(); };
    api["status"] = [] {
        switch (getGC().state()) {
            case GCState::Idle:
                return "idle";
            case GCState::Running:
                return "running";
            case GCState::Paused:
                return "paused";
        }
        assert(false);
        return "unknown";
    };
    api["step"] = [](sol::optional<int> newStep){
        if (newStep) {
            REQUIRE(*newStep <= 0) << "gc.step have to be > 0";
            getGC().step(*newStep);
        }
        return getGC().step();
    };
    api["count"] = [] {
        return getGC().count();
    };
    return api;
}

} // effil