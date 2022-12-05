#include "thread-runner.h"

namespace effil {

void ThreadRunner::initialize(
    const std::string& path,
    const std::string& cpath,
    lua_Number step,
    const sol::function& func)
{
    ctx_->path_ = path;
    ctx_->cpath_ = cpath;
    ctx_->step_ = step;
    try {
        ctx_->function_ = createStoredObject(func);
    } RETHROW_WITH_PREFIX("effil.thread");

    ctx_->addReference(ctx_->function_->gcHandle());
    ctx_->function_->releaseStrongReference();
}

sol::object ThreadRunner::call(sol::this_state lua, const sol::variadic_args& args) {
    return sol::make_object(lua, GC::instance().create<Thread>(
        ctx_->path_, ctx_->cpath_, ctx_->step_, ctx_->function_->unpack(lua), args));
}

void ThreadRunner::exportAPI(sol::state_view& lua) {

    sol::usertype<ThreadRunner> type("new", sol::no_constructor,
        sol::meta_function::call,  &ThreadRunner::call,
        "path", sol::property(&ThreadRunner::getPath, &ThreadRunner::setPath),
        "cpath", sol::property(&ThreadRunner::getCPath, &ThreadRunner::setCPath),
        "step", sol::property(&ThreadRunner::getStep, &ThreadRunner::setStep)
    );
    sol::stack::push(lua, type);
    sol::stack::pop<sol::object>(lua);
}

} // namespace effil