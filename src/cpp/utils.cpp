#include "utils.h"

#include <fstream>
#include <iomanip>

namespace effil {

Logger getLogger() {
    static std::mutex lock;
    static const char* logFile = getenv("EFFIL_LOG");

    if (logFile == nullptr) {
        static std::ostream devNull(0);
        return Logger(devNull, lock);
    }
    else if (strcmp(logFile, "term") == 0) {
        return Logger(std::cout, lock);
    }
    else {
        static std::ofstream fileStream(logFile);
        return Logger(fileStream, lock);
    }
}

std::string getCurrentTime() {
    const auto currTime = std::time(nullptr);
    const auto tm = std::localtime(&currTime);
    std::stringstream ss;
    ss << std::put_time(tm, "%F %T");
    return ss.str();
}

}
