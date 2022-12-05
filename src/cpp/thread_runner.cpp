#include "thread_runner.h"

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

sol::usertype<ThreadRunner> ThreadRunner::exportAPI(sol::state_view& lua) {
    auto type = lua.new_usertype<ThreadRunner>(sol::no_constructor);

    type[sol::call_constructor] = sol::factories(
        [](sol::this_state state, const sol::stack_object& obj) {
            REQUIRE(obj.valid() && obj.get_type() == sol::type::function)
                    << "bad argument #1 to 'effil.thread' (function expected, got "
                    << luaTypename(obj) << ")";

            auto lua = sol::state_view(state);
            return sol::make_object(lua, GC::instance().create<ThreadRunner>(
                lua["package"]["path"],
                lua["package"]["cpath"],
                200,
                obj.as<sol::function>()
            ));
        }
    );
    type[sol::meta_function::call] = &ThreadRunner::call;
    type["path"] = sol::property(&ThreadRunner::getPath, &ThreadRunner::setPath);
    type["cpath"] = sol::property(&ThreadRunner::getCPath, &ThreadRunner::setCPath);
    type["step"] = sol::property(&ThreadRunner::getStep, &ThreadRunner::setStep);
    return type;
}

} // namespace effil