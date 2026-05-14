#pragma once

#include <cstdint>
#include <cstring>
#include <dlfcn.h>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define FXCPP_WORKER(name) \
    static int32_t name##_impl(const char* input, int32_t input_len, char* result, int32_t result_max); \
    extern "C" int32_t name(intptr_t ip, int32_t il, intptr_t rp, int32_t rm) \
    { return name##_impl(reinterpret_cast<const char*>(ip), il, reinterpret_cast<char*>(rp), rm); } \
    static int32_t name##_impl(const char* input, int32_t input_len, char* result, int32_t result_max)

namespace fx
{

struct WorkerResult
{
    int32_t status; // 0 = running, >0 = bytes (done), -1 = error, -2 = invalid
    std::string output;
};

namespace detail
{

using RawWorkerFn = int32_t(*)(intptr_t, int32_t, intptr_t, int32_t);

struct NativeWorkerState
{
    enum Status { Running, Done, Error };
    std::thread thread;
    std::mutex mutex;
    Status status = Running;
    std::string result;
};

inline std::unordered_map<int32_t, std::unique_ptr<NativeWorkerState>>& workerMap()
{
    static std::unordered_map<int32_t, std::unique_ptr<NativeWorkerState>> s_map;
    return s_map;
}

inline int32_t& nextWorkerId()
{
    static int32_t s_id = 1;
    return s_id;
}

}

inline int32_t createWorker(const std::string& fnName, const std::string& input = "", int32_t resultBufferSize = 65536)
{
    void* sym = dlsym(detail::g_libHandle ? detail::g_libHandle : RTLD_DEFAULT, fnName.c_str());
    if (!sym) return -2;
    auto fn = reinterpret_cast<detail::RawWorkerFn>(sym);
    int32_t id = detail::nextWorkerId()++;
    auto state = std::make_unique<detail::NativeWorkerState>();
    auto* statePtr = state.get();
    detail::workerMap()[id] = std::move(state);
    statePtr->thread = std::thread([statePtr, fn, input, resultBufferSize]()
    {
        std::vector<char> resultBuf(resultBufferSize, '\0');
        int32_t written = fn(reinterpret_cast<intptr_t>(input.data()), static_cast<int32_t>(input.size()), reinterpret_cast<intptr_t>(resultBuf.data()), resultBufferSize);
        std::lock_guard<std::mutex> lk(statePtr->mutex);
        if (written > 0 && written < resultBufferSize)
            statePtr->result.assign(resultBuf.data(), written);
        statePtr->status = detail::NativeWorkerState::Done;
    });

    return id;
}

inline WorkerResult pollWorker(int32_t workerId, int32_t = 0)
{
    WorkerResult result{};
    auto& map = detail::workerMap();
    auto it = map.find(workerId);
    if (it == map.end())
    {
        result.status = -2;
        return result;
    }
    auto& state = it->second;
    std::lock_guard<std::mutex> lk(state->mutex);
    if (state->status == detail::NativeWorkerState::Running)
    {
        result.status = 0;
        return result;
    }
    if (state->status == detail::NativeWorkerState::Error)
    {
        if (state->thread.joinable()) state->thread.join();
        map.erase(it);
        result.status = -1;
        return result;
    }
    result.output = std::move(state->result);
    result.status = result.output.empty() ? 1 : static_cast<int32_t>(result.output.size());
    if (state->thread.joinable()) state->thread.join();
    map.erase(it);
    return result;
}

}
