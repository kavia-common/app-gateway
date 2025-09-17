#include "ProviderRegistry.h"

using namespace WPEFramework::Plugin;

uint32_t ProviderRegistry::Register(const std::string& capability, const std::string& appId, uint32_t connectionId) {
    std::lock_guard<std::mutex> lock(_lock);
    ProviderEntry entry;
    entry.appId = appId;
    entry.connectionId = connectionId;
    entry.registeredAt = Core::Time::Now();

    _capabilityToProvider[capability] = entry;
    _capabilitiesByConnection[connectionId].insert(capability);
    return Core::ERROR_NONE;
}

uint32_t ProviderRegistry::Unregister(const std::string& capability, uint32_t connectionId) {
    std::lock_guard<std::mutex> lock(_lock);
    auto it = _capabilityToProvider.find(capability);
    if (it == _capabilityToProvider.end()) {
        return Core::ERROR_NONE;
    }
    // Only the owning connection can unregister this capability
    if (it->second.connectionId != connectionId) {
        return Core::ERROR_GENERAL; // maps to INVALID_REQUEST at handler level
    }
    _capabilityToProvider.erase(it);

    auto connIt = _capabilitiesByConnection.find(connectionId);
    if (connIt != _capabilitiesByConnection.end()) {
        connIt->second.erase(capability);
        if (connIt->second.empty()) {
            _capabilitiesByConnection.erase(connIt);
        }
    }
    return Core::ERROR_NONE;
}

bool ProviderRegistry::Find(const std::string& capability, ProviderEntry& out) const {
    std::lock_guard<std::mutex> lock(_lock);
    auto it = _capabilityToProvider.find(capability);
    if (it == _capabilityToProvider.end()) {
        return false;
    }
    out = it->second;
    return true;
}

void ProviderRegistry::CleanupByConnection(uint32_t connectionId) {
    std::lock_guard<std::mutex> lock(_lock);
    auto connIt = _capabilitiesByConnection.find(connectionId);
    if (connIt == _capabilitiesByConnection.end()) {
        return;
    }
    for (const auto& cap : connIt->second) {
        _capabilityToProvider.erase(cap);
    }
    _capabilitiesByConnection.erase(connIt);
}
