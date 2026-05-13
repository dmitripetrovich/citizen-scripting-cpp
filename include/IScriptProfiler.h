#pragma once

#include "core.h"

FX_DEFINE_GUID(IID_IScriptProfiler, 0x782A4496, 0x2AE3, 0x4C70, 0xB5, 0x4A, 0xFA, 0xD8, 0xFD, 0x8A, 0xEE, 0xFD);

class IScriptProfiler : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptProfiler)
    NS_IMETHOD_(void) SetupFxProfiler(void* obj, int32_t resourceId) = 0;
    NS_IMETHOD_(void) ShutdownFxProfiler() = 0;
};
