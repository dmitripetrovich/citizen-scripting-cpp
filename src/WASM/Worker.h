#pragma once

#include "Imports.h"
#include <string>

#define FXCPP_WORKER(name) \
    static int32_t name##_impl(const char* input, int32_t input_len, char* result, int32_t result_max); \
    extern "C" __attribute__((export_name(#name))) \
    int32_t name(intptr_t ip, int32_t il, intptr_t rp, int32_t rm) \
    { return name##_impl(reinterpret_cast<const char*>(ip), il, reinterpret_cast<char*>(rp), rm); } \
    static int32_t name##_impl(const char* input, int32_t input_len, char* result, int32_t result_max)

namespace fx
{

struct WorkerResult
{
    int32_t status; // 0 = running, > 0 = bytes written (done), -1 = error, -2 = invalid
    std::string output;
};

inline int32_t createWorker(const std::string& fnName, const std::string& input = "")
{
    return __fxcpp_create_worker(fnName.c_str(), static_cast<uint32_t>(fnName.size()), input.c_str(), static_cast<uint32_t>(input.size()));
}

inline WorkerResult pollWorker(int32_t workerId, int32_t maxOutput = 65536)
{
    WorkerResult result{};
    std::string buf(static_cast<size_t>(maxOutput), '\0');
    result.status = __fxcpp_poll_worker(workerId, buf.data(), maxOutput);
    if (result.status > 0)
        buf.resize(static_cast<size_t>(result.status));
    else
        buf.clear();
    result.output = std::move(buf);
    return result;
}

}
