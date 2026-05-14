#pragma once

#include "Imports.h"
#include <string>

namespace fx
{

struct ProcessResult
{
    int32_t status; // bytes written on success, -1 perission denied, -2 error
    std::string output;
};

inline ProcessResult spawnProcess(const std::string& command, int32_t maxOutput = 65536)
{
    ProcessResult result{};
    std::string buf(static_cast<size_t>(maxOutput), '\0');
    result.status = __fxcpp_spawn_process(command.c_str(), static_cast<uint32_t>(command.size()), buf.data(), maxOutput);
    if (result.status > 0)
        buf.resize(static_cast<size_t>(result.status));
    else
        buf.clear();
    result.output = std::move(buf);
    return result;
}

}
