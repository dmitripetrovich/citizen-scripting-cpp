#pragma once

#include "core.h"
#include "fxNativeContext.h"

FX_DEFINE_GUID(IID_IScriptBuffer, 0xAD1B9D69, 0xB984, 0x4D30, 0x8D, 0x33, 0xBB, 0x1E, 0x6C, 0xF9, 0xE1, 0xBA);
FX_DEFINE_GUID(IID_fxIStream, 0x82EC2441, 0xDBB4, 0x4512, 0x81, 0xE9, 0x3A, 0x98, 0xCE, 0x9F, 0xFC, 0xAB);
FX_DEFINE_GUID(IID_IScriptHost, 0x8FFDC384, 0x4767, 0x4EA2, 0xA9, 0x35, 0x3B, 0xFC, 0xAD, 0x1D, 0xB7, 0xBF);
FX_DEFINE_GUID(IID_IScriptHostWithResourceData, 0x9568DF2D, 0x27C8, 0x4B9E, 0xB2, 0x9D, 0x48, 0x27, 0x2C, 0x31, 0x70, 0x84);
FX_DEFINE_GUID(IID_IScriptHostWithManifest, 0x5E212027, 0x3AAD, 0x46D1, 0x97, 0xE0, 0xB8, 0xBC, 0x5E, 0xF8, 0x9E, 0x18);
FX_DEFINE_GUID(IID_IScriptRuntime, 0x67B28AF1, 0xAAF9, 0x4368, 0x82, 0x96, 0xF9, 0x3A, 0xFC, 0x7B, 0xDE, 0x96);
FX_DEFINE_GUID(IID_IScriptRuntimeHandler, 0x4720A986, 0xEAA6, 0x4ECC, 0xA3, 0x1F, 0x2C, 0xE2, 0xBB, 0xF5, 0x69, 0xF7);
FX_DEFINE_GUID(IID_IScriptTickRuntime, 0x91B203C7, 0xF95A, 0x4902, 0xB4, 0x63, 0x72, 0x2D, 0x55, 0x09, 0x83, 0x66);
FX_DEFINE_GUID(IID_IScriptEventRuntime, 0x637140DB, 0x24E5, 0x46BF, 0xA8, 0xBD, 0x08, 0xF2, 0xDB, 0xAC, 0x51, 0x9A);
FX_DEFINE_GUID(IID_IScriptFileHandlingRuntime, 0x567634C6, 0x3BDD, 0x4D0E, 0xAF, 0x39, 0x74, 0x72, 0xAE, 0xD4, 0x79, 0xB7);
FX_DEFINE_GUID(CLSID_ScriptRuntimeHandler, 0xC41E7194, 0x7556, 0x4C02, 0xBA, 0x45, 0xA9, 0xC8, 0x4D, 0x18, 0xAD, 0x43);

class fxIStream;
class IScriptBuffer;
class IScriptHost;
class IScriptHostWithResourceData;
class IScriptRuntime;
class IScriptRuntimeHandler;

class IScriptBuffer : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptBuffer)

    NS_IMETHOD_(char*) GetBytes() = 0;
    NS_IMETHOD_(uint32_t) GetLength() = 0;
};

class fxIStream : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_fxIStream)
};

class IScriptHost : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptHost)
    NS_IMETHOD InvokeNative(fxNativeContext& context) = 0;
    NS_IMETHOD OpenSystemFile(char* fileName, fxIStream** stream) = 0;
    NS_IMETHOD OpenHostFile(char* fileName, fxIStream** stream) = 0;
    NS_IMETHOD CanonicalizeRef(int32_t localRef, int32_t instanceId, char** refString) = 0;
    NS_IMETHOD ScriptTrace(char* message) = 0;
    NS_IMETHOD SubmitBoundaryStart(char* boundaryData, int32_t boundarySize) = 0;
    NS_IMETHOD SubmitBoundaryEnd(char* boundaryData, int32_t boundarySize) = 0;
    NS_IMETHOD GetLastErrorText(char** errorString) = 0;
    NS_IMETHOD InvokeFunctionReference(char* refId, char* argsSerialized, uint32_t argsSize, IScriptBuffer** ret) = 0;
};

class IScriptHostWithResourceData : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptHostWithResourceData)
    NS_IMETHOD GetResourceName(char** resourceName) = 0;
    NS_IMETHOD GetNumResourceMetaData(char* fieldName, int32_t* numFields) = 0;
    NS_IMETHOD GetResourceMetaData(char* fieldName, int32_t fieldIndex, char** fieldValue) = 0;
};

class IScriptHostWithManifest : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptHostWithManifest)
    NS_IMETHOD_(bool) IsManifestVersionBetween (const guid_t& lower, const guid_t& upper) = 0;
    NS_IMETHOD_(bool) IsManifestVersionV2Between(char* lower, char* upper) = 0;
};

class IScriptRuntime : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptRuntime)
    NS_IMETHOD Create(IScriptHost* scriptHost) = 0;
    NS_IMETHOD Destroy() = 0;
    NS_IMETHOD_(void*) GetParentObject() = 0;
    NS_IMETHOD_(void) SetParentObject(void*) = 0;
    NS_IMETHOD_(int32_t) GetInstanceId() = 0;
};

class IScriptRuntimeHandler : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptRuntimeHandler)
    NS_IMETHOD PushRuntime(IScriptRuntime* runtime) = 0;
    NS_IMETHOD GetCurrentRuntime(IScriptRuntime** runtime) = 0;
    NS_IMETHOD PopRuntime(IScriptRuntime* runtime) = 0;
    NS_IMETHOD GetInvokingRuntime(IScriptRuntime** runtime) = 0;
    NS_IMETHOD TryPushRuntime(IScriptRuntime* runtime) = 0;
};

class IScriptTickRuntime : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptTickRuntime)
    NS_IMETHOD Tick() = 0;
};

class IScriptEventRuntime : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptEventRuntime)
    NS_IMETHOD TriggerEvent(char* eventName, char* argsSerialized, uint32_t serializedSize, char* sourceId) = 0;
};

class IScriptFileHandlingRuntime : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptFileHandlingRuntime)
    NS_IMETHOD_(int32_t) HandlesFile(char* scriptFile, IScriptHostWithResourceData* metadata) = 0;
    NS_IMETHOD LoadFile(char* scriptFile) = 0;
};

namespace fx
{
class PushEnvironment
{
    fx::OMPtr<IScriptRuntimeHandler> m_handler;
    fx::OMPtr<IScriptRuntime> m_runtime;
    static fx::OMPtr<IScriptRuntimeHandler> GetHandler()
    {
        static fx::OMPtr<IScriptRuntimeHandler> h;
        if (!h.GetRef())
            fx::MakeInterface(&h, CLSID_ScriptRuntimeHandler);
        return h;
    }

public:
    PushEnvironment() = default;
    explicit PushEnvironment(IScriptRuntime* rt)
    {
        m_handler = GetHandler();
        if (!m_handler.GetRef()) return;
        m_runtime = fx::OMPtr<IScriptRuntime>(rt);
        m_handler->PushRuntime(rt);
    }
    PushEnvironment(IScriptRuntimeHandler* handler, IScriptRuntime* rt)
    {
        if (!handler || !rt) return;
        m_handler = fx::OMPtr<IScriptRuntimeHandler>(handler);
        m_runtime = fx::OMPtr<IScriptRuntime>(rt);
        m_handler->PushRuntime(rt);
    }
    ~PushEnvironment()
    {
        if (m_runtime.GetRef() && m_handler.GetRef())
            m_handler->PopRuntime(m_runtime.GetRef());
    }
    PushEnvironment(const PushEnvironment&) = delete;
    PushEnvironment& operator=(const PushEnvironment&) = delete;
    PushEnvironment(PushEnvironment&& o) noexcept : m_handler(o.m_handler), m_runtime(o.m_runtime)
    {
        o.m_handler = {};
        o.m_runtime = {};
    }
};

inline result_t GetCurrentScriptRuntime(fx::OMPtr<IScriptRuntime>* out)
{
    static fx::OMPtr<IScriptRuntimeHandler> h;
    if (!h.GetRef())
        fx::MakeInterface(&h, CLSID_ScriptRuntimeHandler);
    if (!h.GetRef()) return FX_E_NOTIMPL;
    return h->GetCurrentRuntime(out->ReleaseAndGetAddressOf());
}

}
