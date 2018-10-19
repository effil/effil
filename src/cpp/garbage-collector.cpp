#include "garbage-collector.h"

#include "utils.h"
#include "lua-helpers.h"

#include <cassert>

namespace effil {

constexpr size_t MINIMUN_SIZE_LEFT = 100;

GC::GC()
        : enabled_(true)
        , lastCleanup_(MINIMUN_SIZE_LEFT)
        , step_(2.) {}

// Here is the naive tri-color marking
// garbage collecting algorithm implementation.
void GC::collect() {
    std::lock_guard<std::mutex> g(lock_);

    std::unordered_set<GCHandle> grey;
    std::unordered_map<GCHandle, std::unique_ptr<BaseGCObject>> black;

    for (const auto& handleAndObject : objects_)
        if (handleAndObject.second->instances() > 1)
            grey.insert(handleAndObject.first);

    while (!grey.empty()) {
        auto it = grey.begin();
        GCHandle handle = *it;
        grey.erase(it);

        black[handle] = std::move(objects_[handle]);
        for (GCHandle refHandle : black[handle]->refers()) {
            assert(objects_.count(refHandle));
            if (black.count(refHandle) == 0 && grey.count(refHandle) == 0)
                grey.insert(refHandle);
        }
    }

    // Sweep phase
    DEBUG("gc") << "Removing " << (objects_.size() - black.size())
                << " out of " << objects_.size() << std::endl;
    objects_ = std::move(black);

    lastCleanup_.store(std::max(objects_.size(), MINIMUN_SIZE_LEFT));
}

size_t GC::count() const {
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
    api["step"] = [](const sol::stack_object& newStep){
        auto previous = instance().step();
        if (newStep.valid()) {
            REQUIRE(newStep.get_type() == sol::type::number)
                    << "bad argument #1 to 'effil.gc.step' (number expected, got "
                    << luaTypename(newStep) << ")";
            REQUIRE(newStep.as<double>() > 1)
                    << "effil.gc.step: invalid capacity value = "
                    << newStep.as<double>();
            instance().step(newStep.as<double>());
        }
        return previous;
    };
    api["count"] = [] {
        return instance().count();
    };
    return api;
}

} // effil
