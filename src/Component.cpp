#include "Runtime.h"

#include <vector>

extern "C" intptr_t CoreFxFindFirstImpl(const guid_t& iid, guid_t* clsid);
extern "C" int32_t CoreFxFindNextImpl(intptr_t handle, guid_t* clsid);
extern "C" void CoreFxFindImplClose(intptr_t handle);
extern "C" result_t CoreFxCreateObjectInstance(const guid_t& guid, const guid_t& iid, void** objectRef);

extern "C" intptr_t fxFindFirstImpl(const guid_t& iid, guid_t* clsid)
{
    return CoreFxFindFirstImpl(iid, clsid);
}
extern "C" int32_t fxFindNextImpl(intptr_t handle, guid_t* clsid)
{
    return CoreFxFindNextImpl(handle, clsid);
}
extern "C" void fxFindImplClose(intptr_t handle)
{
    CoreFxFindImplClose(handle);
}
extern "C" result_t fxCreateObjectInstance(const guid_t& guid, const guid_t& iid, void** objectRef)
{
    return CoreFxCreateObjectInstance(guid, iid, objectRef);
}

struct OMFactoryDef
{
    guid_t guid;
    fxIBase* (*factory)();
    OMFactoryDef* next = nullptr;
    OMFactoryDef(const guid_t& g, fxIBase*(*f)()) : guid(g), factory(f)
    {
        next = s_factories;
        s_factories = this;
    }
    static OMFactoryDef* s_factories;
};
OMFactoryDef* OMFactoryDef::s_factories = nullptr;

struct OMImplementsDef
{
    guid_t iid;
    guid_t clsid;
    OMImplementsDef* next = nullptr;
    OMImplementsDef(const guid_t& c, const guid_t& i) : iid(i), clsid(c)
    {
        next = s_impls;
        s_impls = this;
    }
    static OMImplementsDef* s_impls;
};
OMImplementsDef* OMImplementsDef::s_impls = nullptr;

static OMFactoryDef s_factory{
    CLSID_Runtime,
    []() -> fxIBase* { return fx::MakeNewBase<Runtime>(); }
};

static OMImplementsDef s_implFile { CLSID_Runtime, IScriptFileHandlingRuntime::GetIID() };
static OMImplementsDef s_implRuntime{ CLSID_Runtime, IScriptRuntime::GetIID() };
static OMImplementsDef s_implTick { CLSID_Runtime, IScriptTickRuntime::GetIID() };
static OMImplementsDef s_implEvent { CLSID_Runtime, IScriptEventRuntime::GetIID() };
static OMImplementsDef s_implRef { CLSID_Runtime, IScriptRefRuntime::GetIID() };
static OMImplementsDef s_implBookmarks{ CLSID_Runtime, IScriptTickRuntimeWithBookmarks::GetIID() };
static OMImplementsDef s_implStackWalk{ CLSID_Runtime, IScriptStackWalkingRuntime::GetIID() };
static OMImplementsDef s_implMemInfo { CLSID_Runtime, IScriptMemInfoRuntime::GetIID() };
static OMImplementsDef s_implWarning { CLSID_Runtime, IScriptWarningRuntime::GetIID() };
static OMImplementsDef s_implProfiler { CLSID_Runtime, IScriptProfiler::GetIID() };

class OMComponent
{
public:
    virtual result_t CreateObjectInstance(const guid_t& guid, const guid_t& iid, void** outRef) = 0;
    virtual std::vector<guid_t> GetImplementedClasses(const guid_t& iid) = 0;
};

class Component : public fwRefCountable
{
public:
    virtual bool Initialize() { return true; }
    virtual void SetCommandLine(int, char*[]) {}
    virtual bool SetUserData(const std::string&) { return true; }
    virtual bool Shutdown() { return true; }
    virtual bool DoGameLoad(void*) { return true; }
    virtual bool IsA(uint32_t) { return false; }
    virtual void* As(uint32_t) { return nullptr; }
};

class ComponentInstance final : public Component, public OMComponent
{
public:
    bool Initialize() override { return true ; }
    bool Shutdown() override { return true ; }

    bool IsA(uint32_t type) override
    {
        return type == HashString("OMComponent") || Component::IsA(type);
    }

    void* As(uint32_t type) override
    {
        if (type == HashString("OMComponent"))
            return static_cast<OMComponent*>(this);
        return Component::As(type);
    }

    result_t CreateObjectInstance(const guid_t& guid, const guid_t& iid, void** outRef) override
    {
        guid_t match = fx::IsNullGuid(guid) ? iid : guid;

        for (auto* f = OMFactoryDef::s_factories; f; f = f->next)
        {
            if (f->guid == match)
            {
                fxIBase* base = f->factory();
                result_t res = base->QueryInterface(iid, outRef);
                base->Release();
                if (res != FX_E_NOINTERFACE) return res;
            }
        }
        return FX_E_NOINTERFACE;
    }

    std::vector<guid_t> GetImplementedClasses(const guid_t& iid) override
    {
        std::vector<guid_t> out;
        for (auto* e = OMImplementsDef::s_impls; e; e = e->next)
            if (e->iid == iid) out.push_back(e->clsid);
        return out;
    }
};

extern "C" FXCPP_EXPORT Component* CreateComponent()
{
    return new ComponentInstance();
}
