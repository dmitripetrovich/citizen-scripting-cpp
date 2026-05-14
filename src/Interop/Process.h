#pragma once

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <string>

namespace fx
{

struct ProcessResult
{
    int32_t status;
    std::string output;
};

namespace detail {
    inline std::mutex& envMutex() { static std::mutex m; return m; }
}

inline ProcessResult spawnProcess(const std::string& command)
{
    ProcessResult result{};
    std::lock_guard<std::mutex> lk(detail::envMutex());
    const char* savedLd = getenv("LD_LIBRARY_PATH");
    std::string savedLdStr = savedLd ? savedLd : "";
    if (savedLd) unsetenv("LD_LIBRARY_PATH");
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        if (!savedLdStr.empty()) setenv("LD_LIBRARY_PATH", savedLdStr.c_str(), 1);
        result.status = -2;
        return result;
    }
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe))
        result.output.append(buf);
    pclose(pipe);
    if (!savedLdStr.empty()) setenv("LD_LIBRARY_PATH", savedLdStr.c_str(), 1);
    while (!result.output.empty() && result.output.back() == '\n')
        result.output.pop_back();
    result.status = static_cast<int32_t>(result.output.size());
    return result;
}

}
