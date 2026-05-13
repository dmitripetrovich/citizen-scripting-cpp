#pragma once

#include "Resource.h"

namespace fx
{

inline std::string getResourceMetadata(const std::string& key, int index = 0)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getResourceMetadata(key, index);
    return {};
}

inline int getNumResourceMetadata(const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getNumResourceMetadata(key);
    return 0;
}

inline bool isManifestVersionBetween(const guid_t& lower, const guid_t& upper)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->isManifestVersionBetween(lower, upper);
    return false;
}

inline bool isManifestVersionV2Between(const std::string& lower, const std::string& upper)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->isManifestVersionV2Between(lower, upper);
    return false;
}

inline std::string getCurrentResourceName()
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getCurrentResourceName();
    return {};
}

inline std::string getInvokingResource()
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getInvokingResource();
    return {};
}

inline std::string getResourceState(const std::string& resource)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getResourceState(resource);
    return "unknown";
}

inline int getNumResources()
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getNumResources();
    return 0;
}

inline std::string getResourceByIndex(int index)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getResourceByIndex(index);
    return {};
}

inline std::vector<std::string> getResources()
{
    std::vector<std::string> result;
    int num = getNumResources();
    for (int i = 0; i < num; i++)
    {
        std::string name = getResourceByIndex(i);
        if (!name.empty()) result.push_back(std::move(name));
    }
    return result;
}

}
