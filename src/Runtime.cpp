#include "Runtime.h"

#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <string_view>

#ifndef _WIN32
#include <dlfcn.h>
#include <cxxabi.h>
#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define HAS_EXECINFO 1
#endif
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

static std::string GetResourcePath(IScriptHost* host)
{
    fx::OMPtr<IScriptHostWithResourceData> md;
    fx::OMPtr<IScriptHost> h(host);
    if (FX_FAILED(h.As(&md)) || !md.GetRef())
        return {};
    char* name = nullptr;
    if (FX_FAILED(md->GetResourceName(&name)) || !name)
        return {};
    fxNativeContext ctx{};
    ctx.nativeIdentifier = HashString("GET_RESOURCE_PATH");
    ctx.arguments[0] = reinterpret_cast<uintptr_t>(name);
    ctx.numArguments = 1;
    ctx.numResults = 1;
    host->InvokeNative(ctx);
    const char* path = reinterpret_cast<const char*>(ctx.arguments[0]);
    return path ? std::string(path) : std::string{};
}

Runtime::Runtime() : m_instanceId(static_cast<int32_t>(reinterpret_cast<intptr_t>(this) & 0x7FFFFFFF))
{}

Runtime::~Runtime()
{
    Destroy();
}

result_t OM_DECL Runtime::Create(IScriptHost* host)
{
    m_host = host;
    fx::OMPtr<IScriptHost> h(host);
    fx::OMPtr<IScriptHostWithResourceData> md;
    if (FX_SUCCEEDED(h.As(&md)) && md.GetRef())
    {
        char* name = nullptr;
        if (FX_SUCCEEDED(md->GetResourceName(&name)) && name)
            m_resourceName = name;
    }
    h.As(&m_manifestHost);
    return FX_S_OK;
}

result_t OM_DECL Runtime::Destroy()
{
    if (m_bookmarkHost.GetRef())
    {
        m_bookmarkHost->RemoveBookmarks(static_cast<IScriptTickRuntimeWithBookmarks*>(this));
        m_bookmarkHost = {};
    }
    if (m_ctx)
    {
        {
            fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
            m_ctx->dispatchStop();
            m_ctx->cleanupStateBagHandlers();
        }
        m_ctx->cleanupBookmarks();
        delete m_ctx;
        m_ctx = nullptr;
    }
    m_refs.clear();
#ifndef _WIN32
    if (m_libHandle) { dlclose(m_libHandle); m_libHandle = nullptr; }
#else
    if (m_libHandle) { FreeLibrary(static_cast<HMODULE>(m_libHandle)); m_libHandle = nullptr; }
#endif
    m_host = nullptr;
    return FX_S_OK;
}

void OM_DECL Runtime::SetParentObject(void* obj)
{
    m_parentObject = obj;
}

result_t OM_DECL Runtime::Tick()
{
    if (!m_ctx || !m_ctx->hasPendingWork()) return FX_S_OK;
    fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
    BoundaryGuard boundary(m_host, m_nextBoundaryId++);
    try
    {
        m_ctx->dispatchTick();
    }
    catch (const std::exception& e)
    {
        m_ctx->trace("Unhandled exception in tick handler: %s\n", e.what());
    }
    catch (...)
    {
        m_ctx->trace("Unhandled non-standard exception in tick handler\n");
    }
    return FX_S_OK;
}

result_t OM_DECL Runtime::TickBookmarks(uint64_t* bookmarks, int32_t numBookmarks)
{
    if (!m_ctx || numBookmarks <= 0) return FX_S_OK;
    fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
    BoundaryGuard boundary(m_host, m_nextBoundaryId++);
    m_ctx->resumeBookmarks(bookmarks, numBookmarks);
    return FX_S_OK;
}

result_t OM_DECL Runtime::TriggerEvent(char* eventName, char* argsSerialized, uint32_t serializedSize, char* sourceId)
{
    if (!m_ctx || !eventName) return FX_S_OK;
    fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
    BoundaryGuard boundary(m_host, m_nextBoundaryId++);
    try
    {
        fx::json::Value args = fx::msgpack::decode(argsSerialized, serializedSize);
        fx::json::ensureArray(args);
        std::string src = sourceId ? sourceId : "-1";
        m_ctx->dispatchEvent(eventName, args, src);
    }
    catch (const std::exception& e)
    {
        m_ctx->trace("Unhandled exception in event '%s': %s\n", eventName, e.what());
    }
    catch (...)
    {
        m_ctx->trace("Unhandled non-standard exception in event '%s'\n", eventName);
    }
    return FX_S_OK;
}

int32_t Runtime::AddFuncRef(RefCallback cb)
{
    int32_t idx = m_nextRefIdx++;
    if (m_nextRefIdx <= 0) m_nextRefIdx = 1;
    m_refs[idx] = std::move(cb);
    return idx;
}

result_t OM_DECL Runtime::CallRef(int32_t refIdx, char* argsSerialized, uint32_t argsSize, IScriptBuffer** retval)
{
    auto it = m_refs.find(refIdx);
    if (it == m_refs.end()) return FX_E_INVALIDARG;
    fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
    BoundaryGuard boundary(m_host, m_nextBoundaryId++);
    std::vector<char> result;
    try
    {
        result = it->second(argsSerialized, argsSize);
    }
    catch (const std::exception& e)
    {
        if (m_ctx)
            m_ctx->trace("Unhandled exception in ref %d: %s\n", refIdx, e.what());
        result = { static_cast<char>(0xC0) }; // msgpack nil
    }
    catch (...)
    {
        if (m_ctx)
            m_ctx->trace("Unhandled non-standard exception in ref %d\n", refIdx);
        result = { static_cast<char>(0xC0) };
    }
    auto buf = fx::MakeNew<ScriptBuffer>(std::move(result));
    return buf->QueryInterface(IScriptBuffer::GetIID(), reinterpret_cast<void**>(retval));
}

result_t OM_DECL Runtime::DuplicateRef(int32_t refIdx, int32_t* newRefIdx)
{
    auto it = m_refs.find(refIdx);
    if (it == m_refs.end()) return FX_E_INVALIDARG;
    *newRefIdx = m_nextRefIdx++;
    if (m_nextRefIdx <= 0) m_nextRefIdx = 1;
    m_refs[*newRefIdx] = it->second;
    return FX_S_OK;
}

result_t OM_DECL Runtime::RemoveRef(int32_t refIdx)
{
    m_refs.erase(refIdx);
    return FX_S_OK;
}

static void SubmitFrame(IScriptStackWalkVisitor* visitor, const std::string& resource, const std::string& func, const std::string& file, int line)
{
    fx::json::Value frame;
    frame.kind = fx::json::Value::Kind::Array;
    frame.children.push_back(fx::json::makeString(resource));
    frame.children.push_back(fx::json::makeString(func));
    frame.children.push_back(fx::json::makeString(file));
    frame.children.push_back(fx::json::makeInt(line));
    auto encoded = fx::msgpack::encode(frame);
    visitor->SubmitStackFrame(reinterpret_cast<char*>(encoded.data()), static_cast<uint32_t>(encoded.size()));
}

result_t OM_DECL Runtime::WalkStack(char* boundaryStart, uint32_t boundaryStartLength, char* boundaryEnd, uint32_t boundaryEndLength, IScriptStackWalkVisitor* visitor)
{
    if (!visitor) return FX_S_OK;

    bool submitted = false;

#if defined(HAS_EXECINFO)
    void* frames[64];
    int count = backtrace(frames, 64);

    for (int i = 0; i < count; ++i)
    {
        Dl_info info{};
        if (!dladdr(frames[i], &info) || !info.dli_sname)
            continue;

        if (!info.dli_fname || m_libPath.empty() || m_libPath != info.dli_fname)
            continue;

        std::string funcName;
        int status = 0;
        char* demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
        if (status == 0 && demangled)
        {
            funcName = demangled;
            free(demangled);
        }
        else
        {
            funcName = info.dli_sname;
        }

        SubmitFrame(visitor, m_resourceName, funcName, info.dli_fname, 0);
        submitted = true;
    }
#elif defined(_WIN32)
    void* frames[64];
    USHORT count = CaptureStackBackTrace(0, 64, frames, nullptr);

    static bool symInit = false;
    if (!symInit)
    {
        SymInitialize(GetCurrentProcess(), nullptr, TRUE);
        symInit = true;
    }

    HMODULE libMod = static_cast<HMODULE>(m_libHandle);
    for (USHORT i = 0; i < count; ++i)
    {
        HMODULE mod = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(frames[i]), &mod);
        if (mod != libMod)
            continue;

        alignas(SYMBOL_INFO) char buf[sizeof(SYMBOL_INFO) + 256];
        auto* sym = reinterpret_cast<SYMBOL_INFO*>(buf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 256;

        std::string funcName = "unknown";
        if (SymFromAddr(GetCurrentProcess(), reinterpret_cast<DWORD64>(frames[i]), nullptr, sym))
            funcName = sym->Name;

        SubmitFrame(visitor, m_resourceName, funcName, m_libPath, 0);
        submitted = true;
    }
#endif

    if (!submitted)
        SubmitFrame(visitor, m_resourceName, "native", "", 0);

    return FX_S_OK;
}

result_t OM_DECL Runtime::RequestMemoryUsage()
{
    return FX_S_OK;
}

result_t OM_DECL Runtime::GetMemoryUsage(int64_t* memUsage)
{
    if (!memUsage) return FX_E_INVALIDARG;
    *memUsage = m_ctx ? m_ctx->getMemoryUsage() : 0;
    return FX_S_OK;
}

result_t OM_DECL Runtime::EmitWarning(char* channel, char* message)
{
    if (m_ctx && message)
    {
        const char* ch = channel ? channel : "script";
        m_ctx->trace("[warning:%s] %s\n", ch, message);
    }
    return FX_S_OK;
}

void OM_DECL Runtime::SetupFxProfiler(void* obj, int32_t resourceId)
{
    m_profiler = obj;
    m_profilerId = resourceId;
}

void OM_DECL Runtime::ShutdownFxProfiler()
{
    m_profiler = nullptr;
    m_profilerId = 0;
}

int32_t OM_DECL Runtime::HandlesFile(char* scriptFile, IScriptHostWithResourceData* /*metadata*/)
{
    if (!scriptFile) return 0;
    std::string_view file(scriptFile);
#ifdef _WIN32
    return file.ends_with(".dll") ? 1 : 0;
#else
    return file.ends_with(".so") ? 1 : 0;
#endif
}

result_t OM_DECL Runtime::LoadFile(char* scriptFile)
{
    if (!m_host || !scriptFile) return FX_E_INVALIDARG;
    std::string root = GetResourcePath(m_host);
    if (!root.empty() && root.back() == '/')
        root.pop_back();
    if (root.empty())
    {
        fprintf(stderr, "[citizen-scripting-cpp] Runtime: could not get resource path for '%s'\n", m_resourceName.c_str());
        return FX_E_INVALIDARG;
    }
    std::string_view scriptFileView(scriptFile);
    if (scriptFileView.find("..") != std::string_view::npos)
    {
        fprintf(stderr, "[citizen-scripting-cpp] Rejected script path with '..': '%s'\n", scriptFile);
        return FX_E_INVALIDARG;
    }
    {
        fx::OMPtr<fxIStream> stream;
        if (FX_FAILED(m_host->OpenHostFile(scriptFile, stream.GetAddressOf())) || !stream.GetRef())
        {
            fprintf(stderr, "[citizen-scripting-cpp] Host denied access to '%s' in resource '%s'\n", scriptFile, m_resourceName.c_str());
            return FX_E_INVALIDARG;
        }
    }

#ifdef _WIN32
    std::string fullPath = root + "\\" + scriptFile;
    m_libHandle = LoadLibraryA(fullPath.c_str());
    if (!m_libHandle)
    {
        fprintf(stderr, "[citizen-scripting-cpp] LoadLibraryA failed for '%s': error %lu\n", fullPath.c_str(), GetLastError());
        return FX_E_INVALIDARG;
    }
    auto* initFn = reinterpret_cast<void(*)(fx::ResourceContext*)>(GetProcAddress(static_cast<HMODULE>(m_libHandle), "fxcpp_init"));
#else
    std::string fullPath = root + "/" + scriptFile;
    m_libHandle = dlopen(fullPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!m_libHandle)
    {
        fprintf(stderr, "[citizen-scripting-cpp] dlopen failed for '%s': %s\n", fullPath.c_str(), dlerror());
        return FX_E_INVALIDARG;
    }
    auto* initFn = reinterpret_cast<void(*)(fx::ResourceContext*)>(dlsym(m_libHandle, "fxcpp_init"));
#endif
    m_libPath = fullPath;
    if (!initFn)
    {
        fprintf(stderr, "[citizen-scripting-cpp] '%s' has no fxcpp_init export\n", fullPath.c_str());
        return FX_E_INVALIDARG;
    }
    fx::OMPtr<IScriptRuntimeHandler> runtimeHandler;
    fx::MakeInterface(&runtimeHandler, CLSID_ScriptRuntimeHandler);
    {
        fx::OMPtr<IScriptHost> sh(m_host);
        fx::OMPtr<IScriptHostWithBookmarks> bh;
        if (FX_SUCCEEDED(sh.As(&bh)) && bh.GetRef())
        {
            m_bookmarkHost = bh;
            m_bookmarkHost->CreateBookmarks(static_cast<IScriptTickRuntimeWithBookmarks*>(this));
        }
    }
    fx::ScheduleBookmarkFn schedBookmark;
    if (m_bookmarkHost.GetRef())
    {
        schedBookmark = [this](uint64_t bm, int64_t deadline) {
            if (m_bookmarkHost.GetRef())
                m_bookmarkHost->ScheduleBookmark(static_cast<IScriptTickRuntimeWithBookmarks*>(this), bm, deadline);
        };
    }
    m_ctx = new fx::ResourceContext(m_host, this, m_resourceName, runtimeHandler.GetRef(), [this](RefCallback cb) -> int32_t { return AddFuncRef(std::move(cb)); }, [this](int32_t idx) { m_refs.erase(idx); }, std::move(schedBookmark));
    fprintf(stderr, "[citizen-scripting-cpp] Loaded C++ resource '%s'\n", m_resourceName.c_str());
    fx::PushEnvironment env(static_cast<IScriptRuntime*>(this));
    try
    {
        initFn(m_ctx);
    }
    catch (const std::exception& e)
    {
        fprintf(stderr, "[citizen-scripting-cpp] Exception during init of '%s': %s\n", m_resourceName.c_str(), e.what());
        m_ctx->trace("Exception during resource init: %s\n", e.what());
        if (m_bookmarkHost.GetRef())
        {
            m_bookmarkHost->RemoveBookmarks(static_cast<IScriptTickRuntimeWithBookmarks*>(this));
            m_bookmarkHost = {};
        }
        m_ctx->cleanupBookmarks();
        delete m_ctx; m_ctx = nullptr;
#ifndef _WIN32
        if (m_libHandle) { dlclose(m_libHandle); m_libHandle = nullptr; }
#else
        if (m_libHandle) { FreeLibrary(static_cast<HMODULE>(m_libHandle)); m_libHandle = nullptr; }
#endif
        return FX_E_INVALIDARG;
    }
    catch (...)
    {
        fprintf(stderr, "[citizen-scripting-cpp] Non-standard exception during init of '%s'\n", m_resourceName.c_str());
        if (m_bookmarkHost.GetRef())
        {
            m_bookmarkHost->RemoveBookmarks(static_cast<IScriptTickRuntimeWithBookmarks*>(this));
            m_bookmarkHost = {};
        }
        m_ctx->cleanupBookmarks();
        delete m_ctx; m_ctx = nullptr;
#ifndef _WIN32
        if (m_libHandle) { dlclose(m_libHandle); m_libHandle = nullptr; }
#else
        if (m_libHandle) { FreeLibrary(static_cast<HMODULE>(m_libHandle)); m_libHandle = nullptr; }
#endif
        return FX_E_INVALIDARG;
    }
    return FX_S_OK;
}
