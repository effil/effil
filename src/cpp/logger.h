#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <iostream>
#include <thread>

namespace effil {

class Logger
{
public:
    Logger()
        : lockGuard_(lock_)
    {}

    ~Logger() {
        *stream_ << std::endl;
    }

    std::ostream& getStream() { return *stream_; }

private:
    static std::mutex lock_;
    static std::unique_ptr<std::ostream> stream_;

    std::lock_guard<std::mutex> lockGuard_;
};

std::string getCurrentTime();

#ifdef NDEBUG
#   define DEBUG(x) if (false) std::cout
#else
#   define DEBUG(name) Logger().getStream() << getCurrentTime() \
                            << " " << "[" << std::this_thread::get_id() \
                            << "][" << name << "] "
#endif

} // effil
