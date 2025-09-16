#pragma once

#include "Module.h"
#include "PermissionService.h"
#include "Registry.h"

namespace WPEFramework {
namespace Plugin {

/**
 * Badger
 *
 * Thunder plugin exposing Badger permission APIs over JSON-RPC.
 * It evaluates permissions by intersecting:
 *  - A dynamic granted Thor/Badger permission ID set (runtime/config-sourced)
 *  - A static YAML registry mapping permission IDs to Firebolt capabilities (per-role) and APIs
 *
 * JSON-RPC methods (prefix org.rdk.Badger implied by callsign):
 *  - permissions.check          -> boolean
 *  - permissions.checkAll       -> [{ capability, role, allowed }]
 *  - permissions.listCaps       -> [capability]
 *  - permissions.listFireboltPermissions -> [permissionId]
 *  - permissions.listMethods    -> [string] (introspection of supported methods)
 */
class Badger : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    Badger(const Badger&) = delete;
    Badger& operator=(const Badger&) = delete;

    // PUBLIC_INTERFACE
    /**
     * Construct plugin and register JSON-RPC handlers.
     */
    Badger();

    // PUBLIC_INTERFACE
    /**
     * Destructor unregisters handlers and releases resources.
     */
    ~Badger() override;

    // ---------- IPlugin ----------
    // PUBLIC_INTERFACE
    /**
     * Initialize the plugin and load configuration.
     * Config (plugin.json -> configuration):
     * {
     *   "registryPath": "/etc/badger/thor_permission_registry.yaml",
     *   "cacheTtlSeconds": 3600,
     *   "grantedIds": ["DATA_timeZone", "ACCESS_integratedPlayer_create"]
     * }
     * Returns empty string on success or non-empty error message to abort.
     */
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Deinitialize the plugin and release resources.
     */
    void Deinitialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Information string.
     */
    string Information() const override;

private:
    void RegisterAll();
    void UnregisterAll();

    // ---------- JSON-RPC endpoints ----------
    // PUBLIC_INTERFACE
    /**
     * Badger.ping -> "pong"
     */
    uint32_t endpoint_ping(Core::JSON::String& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.listMethods -> returns list of supported org.rdk.Badger.* method names
     */
    uint32_t endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.check
     * params: { "capability": string, "role": "use|manage|provide"? (default "use") }
     * returns: boolean
     */
    uint32_t endpoint_permissions_check(const Core::JSON::Variant& parameters, Core::JSON::Boolean& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.checkAll
     * params: { "items": [ { "capability": string, "role": string? }, ... ] }
     * returns: [ { "capability": string, "role": string, "allowed": bool }, ... ]
     */
    uint32_t endpoint_permissions_checkAll(const Core::JSON::Variant& parameters, Core::JSON::ArrayType<Core::JSON::Object>& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.listCaps
     * returns: array of capability strings derived from current grants via registry mapping
     */
    uint32_t endpoint_permissions_listCaps(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.listFireboltPermissions
     * returns: array of raw granted permission id strings
     */
    uint32_t endpoint_permissions_listFireboltPermissions(Core::JSON::ArrayType<Core::JSON::String>& response);

private:
    PluginHost::IShell* _service { nullptr };
    std::unique_ptr<Registry> _registry;
    std::unique_ptr<PermissionService> _permService;

    // Config
    string _registryPath;
    uint32_t _cacheTtlSeconds { 3600 };
};

} // namespace Plugin
} // namespace WPEFramework
