#pragma once

#include "Context.h"
#include "Refs.h"
#include "Coroutine.h"

#define FXCPP_WASM_EXPORT(name) extern "C" __attribute__((export_name(#name)))

FXCPP_WASM_EXPORT(fxcpp_alloc) void* fxcpp_alloc(uint32_t size) { return malloc(size); }
FXCPP_WASM_EXPORT(fxcpp_free) void fxcpp_free(void* ptr, uint32_t) { free(ptr); }

FXCPP_WASM_EXPORT(fxcpp_has_pending_work) int32_t fxcpp_has_pending_work()
{
    if (!fxw_internal::coroutines().empty()) return 1;
    auto* c = fxw_internal::currentContext();
    if (!c) return 0;
    return (!c->ticks.empty() || !c->timers.empty()) ? 1 : 0;
}

FXCPP_WASM_EXPORT(fxcpp_tick) void fxcpp_tick()
{
    fxw_internal::resumeCoroutines();
    if (auto* c = fxw_internal::currentContext()) c->dispatchTick();
}

FXCPP_WASM_EXPORT(fxcpp_on_event) void fxcpp_on_event(const char* name, uint32_t nameLen, const uint8_t* args, uint32_t argsLen, const char* src, uint32_t srcLen)
{
    if (auto* c = fxw_internal::currentContext())
        c->dispatchEvent(name, nameLen, args, argsLen, src, srcLen);
}

FXCPP_WASM_EXPORT(fxcpp_on_stop) void fxcpp_on_stop()
{
    fxw_internal::cleanupCoroutines();
    if (auto* c = fxw_internal::currentContext())
    {
        for (auto& [cookie, hostRef] : c->stateBagHandlerRefs)
        {
            fx::invokeNative(HashString("REMOVE_STATE_BAG_CHANGE_HANDLER"), {fx::NativeArg(cookie)});
            fx::detail::removeRef(hostRef);
        }
        c->stateBagHandlerRefs.clear();
        c->dispatchStop();
    }
}

FXCPP_WASM_EXPORT(fxcpp_invoke_ref) int32_t fxcpp_invoke_ref(int32_t callback_id, const uint8_t* args_ptr, uint32_t args_len, uint8_t* result_ptr, uint32_t result_max)
{
    auto& callbacks = fxw_internal::refCallbacks();
    auto it = callbacks.find(callback_id);
    if (it == callbacks.end()) return 0;
    auto result = it->second(args_ptr, args_len);
    uint32_t copy = std::min<uint32_t>(static_cast<uint32_t>(result.size()), result_max);
    if (copy > 0 && result_ptr)
        memcpy(result_ptr, result.data(), copy);
    return static_cast<int32_t>(result.size());
}

FXCPP_WASM_EXPORT(fxcpp_duplicate_ref) int32_t fxcpp_duplicate_ref(int32_t callback_id)
{
    auto& counts = fxw_internal::refCounts();
    auto it = counts.find(callback_id);
    if (it != counts.end())
        ++it->second;
    return callback_id;
}

FXCPP_WASM_EXPORT(fxcpp_remove_ref) void fxcpp_remove_ref(int32_t callback_id)
{
    auto& counts = fxw_internal::refCounts();
    auto it = counts.find(callback_id);
    if (it == counts.end()) return;
    if (--it->second <= 0)
    {
        counts.erase(it);
        fxw_internal::refCallbacks().erase(callback_id);
    }
}

#define FXCPP_WASM_ENTRY \
    static void _fxcpp_wasm_body(); \
    FXCPP_WASM_EXPORT(fxcpp_init) void fxcpp_init() \
    { \
        static fxw_internal::Context s_ctx; \
        fxw_internal::currentContext() = &s_ctx; \
        _fxcpp_wasm_body(); \
    } \
    static void _fxcpp_wasm_body()

#define Server FXCPP_WASM_ENTRY
