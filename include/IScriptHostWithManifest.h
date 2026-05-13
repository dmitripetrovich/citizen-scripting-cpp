#pragma once

#include "core.h"

FX_DEFINE_GUID(IID_IScriptHostWithManifest, 0x5E212027, 0x3AAD, 0x46D1, 0x97, 0xE0, 0xB8, 0xBC, 0x5E, 0xF8, 0x9E, 0x18);

class IScriptHostWithManifest : public fxIBase
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(IID_IScriptHostWithManifest)
    NS_IMETHOD IsManifestVersionBetween (const guid_t& lower, const guid_t& upper, bool* retval) = 0;
    NS_IMETHOD IsManifestVersionV2Between(char* lower, char* upper, bool* retval) = 0;
};
