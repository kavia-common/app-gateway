#pragma once
#include "Module.h"
#include "Registry.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PermissionService
 * Evaluates permission checks using a dynamic granted set and static mappings from Registry.
 * This simplified version assumes dynamic grants are provided via configuration or mocked.
 * In production, integrate with Thor backend to fetch grants per (app/session).
 */
class PermissionService {
public:
    PermissionService(const Registry& reg, uint32_t cacheTtlSeconds);

    // PUBLIC_INTERFACE
    /**
     * Evaluate a single capability+role.
     */
    bool CheckCapability(const string& capability, const string& role) const;

    // PUBLIC_INTERFACE
    /**
     * Evaluate many capability+role pairs.
     */
    std::vector<std::tuple<string,string,bool>> CheckAll(const std::vector<std::pair<string,string>>& items) const;

    // PUBLIC_INTERFACE
    /**
     * List current capabilities derived from granted IDs through the registry mapping.
     */
    std::vector<string> ListCapabilities() const;

    // PUBLIC_INTERFACE
    /**
     * List current raw Thor/Badger permission IDs.
     */
    std::vector<string> ListFireboltPermissions() const;

    // PUBLIC_INTERFACE
    /**
     * Replace the current granted set (hook for integration).
     */
    void SetGrantedIds(const std::unordered_set<string>& ids);

    // PUBLIC_INTERFACE
    /**
     * Get granted set (read-only snapshot).
     */
    std::unordered_set<string> GetGrantedIds() const;

private:
    const Registry& _reg;
    uint32_t _ttl;
    mutable std::mutex _lock;
    std::unordered_set<string> _granted; // dynamic grants

    static string ToLower(const string& s);
};

} // namespace Plugin
} // namespace WPEFramework
