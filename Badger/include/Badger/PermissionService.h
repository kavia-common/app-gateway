#pragma once
#include "Module.h"
#include "Registry.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PermissionService
 *
 * Evaluates permission checks:
 * - Maps Firebolt capability + role to Thor/Badger permission IDs via Registry.
 * - Intersects with a dynamic granted set (runtime/config sourced).
 * - Answers allow/deny and listing calls.
 *
 * Note: Dynamic grant fetching (e.g., TPS/gRPC) is not implemented here. The granted IDs
 * are set via SetGrantedIds (from configuration or future extension point).
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
     * List current capabilities derived from granted IDs through registry mapping (union across roles).
     */
    std::vector<string> ListCapabilities() const;

    // PUBLIC_INTERFACE
    /**
     * List current raw Thor/Badger permission IDs (the granted set).
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
