#pragma once

#include "Resource.h"

namespace fx
{

inline void setStateBagValue(const std::string& bagName, const std::string& key, const json::Value& value, bool replicated = true)
{
    if (auto* ctx = detail::g_ctx)
        ctx->setStateBagValue(bagName, key, value, replicated);
}

inline void setPlayerState(int serverId, const std::string& key, const json::Value& value, bool replicated = true)
{
    if (auto* ctx = detail::g_ctx)
        ctx->setPlayerState(serverId, key, value, replicated);
}

inline void setEntityState(int netId, const std::string& key, const json::Value& value, bool replicated = true)
{
    if (auto* ctx = detail::g_ctx)
        ctx->setEntityState(netId, key, value, replicated);
}

inline void setGlobalState(const std::string& key, const json::Value& value, bool replicated = true)
{
    if (auto* ctx = detail::g_ctx)
        ctx->setGlobalState(key, value, replicated);
}

inline json::Value getStateBagValue(const std::string& bagName, const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getStateBagValue(bagName, key);
    return json::makeNull();
}

inline json::Value getPlayerState(int serverId, const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getPlayerState(serverId, key);
    return json::makeNull();
}

inline json::Value getEntityState(int netId, const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getEntityState(netId, key);
    return json::makeNull();
}

inline json::Value getGlobalState(const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getGlobalState(key);
    return json::makeNull();
}

inline bool stateBagHasKey(const std::string& bagName, const std::string& key)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->stateBagHasKey(bagName, key);
    return false;
}

inline std::vector<std::string> getStateBagKeys(const std::string& bagName)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->getStateBagKeys(bagName);
    return {};
}

inline int32_t addStateBagChangeHandler(const std::string& keyFilter, const std::string& bagFilter, StateBagChangeHandler handler)
{
    if (auto* ctx = detail::g_ctx)
        return ctx->addStateBagChangeHandler(keyFilter, bagFilter, std::move(handler));
    return -1;
}

inline void removeStateBagChangeHandler(int32_t cookie)
{
    if (auto* ctx = detail::g_ctx)
        ctx->removeStateBagChangeHandler(cookie);
}

}
