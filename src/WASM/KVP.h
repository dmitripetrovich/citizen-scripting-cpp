#pragma once

#include "Natives.h"

#include <string>
#include <vector>
#include <cstdint>

namespace fx
{

inline void setKvp(const std::string& key, const std::string& value)
{
    natives::invoke(HashString("SET_RESOURCE_KVP"), key.c_str(), value.c_str());
}

inline void setKvpInt(const std::string& key, int value)
{
    natives::invoke(HashString("SET_RESOURCE_KVP_INT"), key.c_str(), value);
}

inline void setKvpFloat(const std::string& key, float value)
{
    natives::invoke(HashString("SET_RESOURCE_KVP_FLOAT"), key.c_str(), value);
}

inline std::string getKvpString(const std::string& key)
{
    return natives::invoke<std::string>(HashString("GET_RESOURCE_KVP_STRING"), key.c_str());
}

inline int getKvpInt(const std::string& key)
{
    return natives::invoke<int>(HashString("GET_RESOURCE_KVP_INT"), key.c_str());
}

inline float getKvpFloat(const std::string& key)
{
    return natives::invoke<float>(HashString("GET_RESOURCE_KVP_FLOAT"), key.c_str());
}

inline void deleteKvp(const std::string& key)
{
    natives::invoke(HashString("DELETE_RESOURCE_KVP"), key.c_str());
}

inline void flushKvp()
{
    natives::invoke(HashString("FLUSH_RESOURCE_KVP"));
}

inline std::vector<std::string> findKvp(const std::string& prefix)
{
    std::vector<std::string> keys;
    int handle = natives::invoke<int>(HashString("START_FIND_KVP"), prefix.c_str());
    if (handle == -1) return keys;
    while (true)
    {
        std::string key = natives::invoke<std::string>(HashString("FIND_KVP"), handle);
        if (key.empty()) break;
        keys.push_back(std::move(key));
    }
    natives::invoke(HashString("END_FIND_KVP"), handle);
    return keys;
}

}
