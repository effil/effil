#include "thread.h"
#include "gc-data.h"
#include "gc-object.h"

namespace effil {

class ThreadRunnerData : public GCData {
public:
    std::string path_;
    std::string cpath_;
    lua_Number step_;
    StoredObject function_;
};

struct ThreadRunner: public GCObject<ThreadRunnerData> {
    static void exportAPI(sol::state_view& lua);

    std::string getPath() const { return ctx_->path_; }
    void setPath(const std::string& p) { ctx_->path_ = p; }

    std::string getCPath() const { return ctx_->cpath_; }
    void setCPath(const std::string& p) { ctx_->cpath_ = p; }

    lua_Number getStep() const { return ctx_->step_; }
    void setStep(lua_Number s) { ctx_->step_ = s; }

private:
    ThreadRunner() = default;
    void initialize(
        const std::string& path,
        const std::string& cpath,
        lua_Number step,
        const sol::function& func);

    sol::object call(sol::this_state lua, const sol::variadic_args& args);
    friend class GC;
};

} // namespace effil