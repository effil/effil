#include "logger.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>


namespace effil {

static std::unique_ptr<std::ostream> getLoggerStream() {
    const char* logFile = getenv("EFFIL_LOG");

    if (logFile == nullptr) {
        return std::make_unique<std::ostream>(nullptr);
    }
    else if (strcmp(logFile, "term") == 0) {
        return std::make_unique<std::ostream>(std::cout.rdbuf());
    }
    return std::make_unique<std::ofstream>(logFile);
}

std::mutex Logger::lock_;
std::unique_ptr<std::ostream> Logger::stream_(getLoggerStream());

std::string getCurrentTime() {
    const auto currTime = std::time(nullptr);
    const auto tm = std::localtime(&currTime);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    return ss.str();
}

} // namespace effil
