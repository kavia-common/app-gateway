#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <string>
#include <cstdint>
#include <core/Time.h>

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * ProviderRegistry keeps mapping of capability -> provider (appId, connectionId).
 */
class ProviderRegistry {
public:
    struct ProviderEntry {
        std::string appId;
        uint32_t connectionId { 0 };
        Core::Time registeredAt;
    };

    ProviderRegistry() = default;

    // PUBLIC_INTERFACE
    uint32_t Register(const std::string& capability, const std::string& appId, uint32_t connectionId);

    // PUBLIC_INTERFACE
    uint32_t Unregister(const std::string& capability, uint32_t connectionId);

    // PUBLIC_INTERFACE
    bool Find(const std::string& capability, ProviderEntry& out) const;

    // PUBLIC_INTERFACE
    void CleanupByConnection(uint32_t connectionId);

private:
    mutable std::mutex _lock;
    std::unordered_map<std::string, ProviderEntry> _capabilityToProvider;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> _capabilitiesByConnection;
};

} // namespace Plugin
} // namespace WPEFramework
