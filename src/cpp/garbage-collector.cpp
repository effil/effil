#include "garbage-collector.h"

#include "utils.h"

#include <cassert>

namespace effil {

GC::GC()
        : enabled_(true)
        , lastCleanup_(0)
        , step_(200) {}

// Here is the naive tri-color marking
// garbage collecting algorithm implementation.
void GC::collect() {
    std::lock_guard<std::mutex> g(lock_);

    std::unordered_set<GCObjectHandle> grey;
    std::unordered_map<GCObjectHandle, std::unique_ptr<GCObject>> black;

    for (const auto& handleAndObject : objects_)
        if (handleAndObject.second->instances() > 1)
            grey.insert(handleAndObject.first);

    while (!grey.empty()) {
        auto it = grey.begin();
        GCObjectHandle handle = *it;
        grey.erase(it);

        black[handle] = std::move(objects_[handle]);
        for (GCObjectHandle refHandle : black[handle]->refers()) {
            assert(objects_.count(refHandle));
            if (black.count(refHandle) == 0 && grey.count(refHandle) == 0)
                grey.insert(refHandle);
        }
    }

    DEBUG << "Removing " << (objects_.size() - black.size()) << " out of " << objects_.size() << std::endl;
    // Sweep phase
    objects_ = std::move(black);

    lastCleanup_.store(0);
}

size_t GC::size() const {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

size_t GC::count() {
    std::lock_guard<std::mutex> g(lock_);
    return objects_.size();
}

GC& GC::instance() {
    static GC gc;
    return gc;
}

sol::table GC::exportAPI(sol::state_view& lua) {
    sol::table api = lua.create_table_with();
    api["collect"] = [=] { instance().collect(); };
    api["pause"] = [] { instance().pause(); };
    api["resume"] = [] { instance().resume(); };
    api["enabled"] = [] { return instance().enabled(); };
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