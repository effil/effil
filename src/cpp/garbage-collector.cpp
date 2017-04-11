#include "garbage-collector.h"

#include "utils.h"

#include <vector>
#include <cassert>

namespace effil {

GC::GC()
        : state_(State::Idle)
        , lastCleanup_(0)
        , step_(200) {}

GCObject* GC::findObject(GCObjectHandle handle) {
    auto it = objects_.find(handle);
    if (it == objects_.end()) {
        DEBUG << "Null handle " << handle << std::endl;
        return nullptr;
    }
    return it->second.get();
}

bool GC::has(GCObjectHandle handle) const {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.find(handle) != objects_.end();
}

// Here is the naive tri-color marking
// garbage collecting algorithm implementation.
void GC::collect() {
    std::lock_guard<std::mutex> g(lock_);

    if (state_ == State::Paused)
        return;
    assert(state_ != State::Running);
    state_ = State::Running;

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

    state_ = State::Idle;
    lastCleanup_.store(0);
}

size_t GC::size() const {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

void GC::pause() {
    std::lock_guard<std::mutex> g(lock_);
    state_ = State::Paused;
}

void GC::resume() {
    std::lock_guard<std::mutex> g(lock_);
    state_ = State::Idle;
}

size_t GC::count() {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

GC& GC::instance() {
    static GC pool;
    return pool;
}

sol::table GC::getLuaApi(sol::state_view& lua) {
    sol::table api = lua.create_table_with();
    api["collect"] = [=] {
        instance().collect();
    };
    api["pause"] = [] { instance().pause(); };
    api["resume"] = [] { instance().resume(); };
    api["status"] = [] {
        switch (instance().state()) {
            case State::Idle:
                return "idle";
            case State::Running:
                return "running";
            case State::Paused:
                return "paused";
        }
        assert(false);
        return "unknown";
    };
    api["step"] = [](sol::optional<int> newStep){
        auto previous = instance().step();
        if (newStep) {
            REQUIRE(*newStep <= 0) << "gc.step have to be > 0";
            instance().step(static_cast<size_t>(*newStep));
        }
        return previous;
    };
    api["count"] = [] {
        return instance().count();
    };
    return api;
}

} // effil