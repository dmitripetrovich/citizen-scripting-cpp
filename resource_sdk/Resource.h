#pragma once

#include "../Json.h"
#include "msgpack.h"
#include "../cfx/fxScripting.h"

#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdio>
#include <cstdarg>

#if defined(_WIN32)
#define FXCPP_RESOURCE_EXPORT __declspec(dllexport)
#else
#define FXCPP_RESOURCE_EXPORT __attribute__((visibility("default")))
#endif

namespace fx
{

class EventArgs
{
public:
    explicit EventArgs(const json::Value& arr) : m_arr(arr) {}
    size_t size() const { return m_arr.size(); ; }
    template<typename T> T get(size_t i) const;
    std::string str (size_t i) const { return m_arr.at(i).asStr(); ; }
    int integer(size_t i) const { return m_arr.at(i).asInt(); ; }
    double number (size_t i) const { return m_arr.at(i).asNum(); ; }
    bool boolean(size_t i) const { return m_arr.at(i).asBool(); ; }
    bool isNull (size_t i) const { return m_arr.at(i).isNull(); ; }

private:
    const json::Value& m_arr;
};

template<> inline std::string EventArgs::get<std::string>(size_t i) const { return str(i); ; }
template<> inline int EventArgs::get<int>(size_t i) const { return integer(i); ; }
template<> inline double EventArgs::get<double>(size_t i) const { return number(i); ; }
template<> inline float EventArgs::get<float>(size_t i) const { return static_cast<float>(number(i)); ; }
template<> inline bool EventArgs::get<bool>(size_t i) const { return boolean(i); ; }

using EventHandler = std::function<void(const std::string& source, EventArgs)>;
using TickHandler = std::function<void()>;
using CommandHandler = std::function<void(const std::string& source, const std::vector<std::string>& args)>;
using RefCallback = std::function<std::vector<char>(const char* argsSerialized, uint32_t argsSize)>;
using AddRefFn = std::function<int32_t(RefCallback)>;

class ResourceContext
{
public: ResourceContext(IScriptHost* host, IScriptRuntime* runtime, std::string name, IScriptRuntimeHandler* handler = nullptr, AddRefFn addRefFn = nullptr) : m_host(host), m_runtime(runtime), m_name(std::move(name)), m_handler(handler), m_addRef(std::move(addRefFn)) {}

    void on(const std::string& event, EventHandler h)
    {
        bool first = m_eventHandlers.find(event) == m_eventHandlers.end();
        m_eventHandlers[event].push_back(std::move(h));

        if (first)
        {
            PushEnvironment env(m_handler, m_runtime);
            fxNativeContext ctx{};
            ctx.nativeIdentifier = HashString("REGISTER_RESOURCE_AS_EVENT_HANDLER");
            ctx.arguments[0] = reinterpret_cast<uintptr_t>(event.c_str());
            ctx.numArguments = 1;
            m_host->InvokeNative(ctx);
        }
    }

    void onTick(TickHandler h)
    {
        m_tickHandlers.push_back(std::move(h));
    }

    void onCommand(const std::string& command, CommandHandler h)
    {
        m_commandHandlers[command].push_back(std::move(h));
        if (!m_addRef)
        {
            fprintf(stderr, "[fx-cpp-sdk] onCommand('%s'): no ref support available\n", command.c_str());
            return;
        }

        int32_t refIdx = m_addRef([this, command](const char* argsSerialized, uint32_t argsSize) -> std::vector<char> {
            fx::json::Value args = fx::msgpack::decode(argsSerialized, argsSize);
            if (args.kind != fx::json::Value::Kind::Array) return {};
            std::string source = args.size() > 0 ? std::to_string(args.at(0).asInt()) : "0";
            std::vector<std::string> cmdArgs;
            if (args.size() > 1 && args.at(1).kind == fx::json::Value::Kind::Array)
                for (size_t i = 0; i < args.at(1).size(); ++i)
                    cmdArgs.push_back(args.at(1).at(i).asStr());
            dispatchCommand(command, source, cmdArgs);
            return { static_cast<char>(0xC0) };
        });

        char* refString = nullptr;
        m_host->CanonicalizeRef(refIdx, m_runtime->GetInstanceId(), &refString);
        if (!refString) return;

        PushEnvironment env(m_handler, m_runtime);
        fxNativeContext ctx{};
        ctx.nativeIdentifier = HashString("REGISTER_COMMAND");
        ctx.arguments[0] = reinterpret_cast<uintptr_t>(command.c_str());
        ctx.arguments[1] = reinterpret_cast<uintptr_t>(refString);
        ctx.arguments[2] = 0;
        ctx.numArguments = 3;
        m_host->InvokeNative(ctx);
        fwFree(refString);
    }

    void trace(const char* fmt, ...)
    {
        char buf[4096];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        m_host->ScriptTrace(buf);
        fprintf(stderr, "[script:%s] %s", m_name.c_str(), buf);
    }

    void emit(const std::string& event, const std::vector<std::string>& rawArgs = {})
    {
        auto payload = fx::msgpack::encodeArgs(rawArgs);
        PushEnvironment env(m_handler, m_runtime);
        fxNativeContext ctx{};
        ctx.nativeIdentifier = HashString("TRIGGER_EVENT_INTERNAL");
        ctx.arguments[0] = reinterpret_cast<uintptr_t>(event.c_str());
        ctx.arguments[1] = reinterpret_cast<uintptr_t>(payload.data());
        ctx.arguments[2] = static_cast<uintptr_t>(payload.size());
        ctx.numArguments = 3;
        m_host->InvokeNative(ctx);
    }

    void emitNet(const std::string& event, int target, const std::vector<std::string>& rawArgs = {})
    {
        auto payload = fx::msgpack::encodeArgs(rawArgs);
        PushEnvironment env(m_handler, m_runtime);
        fxNativeContext ctx{};
        ctx.nativeIdentifier = HashString("TRIGGER_CLIENT_EVENT_INTERNAL");
        ctx.arguments[0] = reinterpret_cast<uintptr_t>(event.c_str());
        ctx.arguments[1] = static_cast<uintptr_t>(target);
        ctx.arguments[2] = reinterpret_cast<uintptr_t>(payload.data());
        ctx.arguments[3] = static_cast<uintptr_t>(payload.size());
        ctx.numArguments = 4;
        m_host->InvokeNative(ctx);
    }

    void dispatchTick()
    {
        for (auto& h : m_tickHandlers) h();
    }

    void dispatchEvent(const std::string& name, const json::Value& args, const std::string& source)
    {
        auto it = m_eventHandlers.find(name);
        if (it == m_eventHandlers.end()) return;
        EventArgs ea(args);
        for (auto& h : it->second) h(source, ea);
    }

    void dispatchCommand(const std::string& command, const std::string& source, const std::vector<std::string>& args)
    {
        auto it = m_commandHandlers.find(command);
        if (it == m_commandHandlers.end()) return;
        for (auto& h : it->second) h(source, args);
    }

    IScriptHost* getHost() { return m_host ; }
    IScriptRuntime* getRuntime() { return m_runtime ; }
    const std::string& resourceName() const { return m_name ; }

private:
    IScriptHost* m_host = nullptr;
    IScriptRuntime* m_runtime = nullptr;
    IScriptRuntimeHandler* m_handler = nullptr;
    AddRefFn m_addRef;
    std::string m_name;
    std::unordered_map<std::string, std::vector<EventHandler>>m_eventHandlers;
    std::unordered_map<std::string, std::vector<CommandHandler>>m_commandHandlers;
    std::vector<TickHandler>m_tickHandlers;
};

namespace detail { inline ResourceContext* g_ctx = nullptr; }
inline ResourceContext* GetContext() { return detail::g_ctx ; }

}

#define FXCPP_RESOURCE \
    static void _fxcpp_resource_body(fx::ResourceContext&); \
    extern "C" FXCPP_RESOURCE_EXPORT \
    void fxcpp_init(fx::ResourceContext* _ctx) \
    { \
        fx::detail::g_ctx = _ctx; \
        _fxcpp_resource_body(*_ctx); \
    } \
    static void _fxcpp_resource_body([[maybe_unused]] fx::ResourceContext& ctx)
