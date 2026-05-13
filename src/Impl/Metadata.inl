namespace fx
{

inline std::string ResourceContext::getResourceMetadata(const std::string& key, int index)
{
    if (!m_metadataHost.GetRef()) return {};
    char* value = nullptr;
    if (FX_FAILED(m_metadataHost->GetResourceMetaData(const_cast<char*>(key.c_str()), index, &value)) || !value)
        return {};
    return std::string(value);
}

inline int ResourceContext::getNumResourceMetadata(const std::string& key)
{
    if (!m_metadataHost.GetRef()) return 0;
    int32_t count = 0;
    m_metadataHost->GetNumResourceMetaData(const_cast<char*>(key.c_str()), &count);
    return count;
}

inline bool ResourceContext::isManifestVersionBetween(const guid_t& lower, const guid_t& upper)
{
    if (!m_manifestHost.GetRef()) return false;
    bool result = false;
    if (FX_FAILED(m_manifestHost->IsManifestVersionBetween(lower, upper, &result)))
        return false;
    return result;
}

inline bool ResourceContext::isManifestVersionV2Between(const std::string& lower, const std::string& upper)
{
    if (!m_manifestHost.GetRef()) return false;
    bool result = false;
    if (FX_FAILED(m_manifestHost->IsManifestVersionV2Between(const_cast<char*>(lower.c_str()), const_cast<char*>(upper.c_str()), &result)))
        return false;
    return result;
}

inline std::string ResourceContext::getCurrentResourceName()
{
    auto ctx = invokeNativeResult(HashString("GET_CURRENT_RESOURCE_NAME"));
    const char* s = reinterpret_cast<const char*>(ctx.arguments[0]);
    return s ? std::string(s) : m_name;
}

inline std::string ResourceContext::getInvokingResource()
{
    auto ctx = invokeNativeResult(HashString("GET_INVOKING_RESOURCE"));
    const char* s = reinterpret_cast<const char*>(ctx.arguments[0]);
    return s ? std::string(s) : std::string{};
}

inline std::string ResourceContext::getResourceState(const std::string& resource)
{
    auto ctx = invokeNativeResult(HashString("GET_RESOURCE_STATE"), reinterpret_cast<uintptr_t>(resource.c_str()));
    const char* s = reinterpret_cast<const char*>(ctx.arguments[0]);
    return s ? std::string(s) : std::string{"unknown"};
}

inline int ResourceContext::getNumResources()
{
    auto ctx = invokeNativeResult(HashString("GET_NUM_RESOURCES"));
    return static_cast<int>(ctx.arguments[0]);
}

inline std::string ResourceContext::getResourceByIndex(int index)
{
    auto ctx = invokeNativeResult(HashString("GET_RESOURCE_BY_FIND_INDEX"), static_cast<uintptr_t>(index));
    const char* s = reinterpret_cast<const char*>(ctx.arguments[0]);
    return s ? std::string(s) : std::string{};
}

}
