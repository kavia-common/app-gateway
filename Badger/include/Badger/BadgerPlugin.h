#pragma once
#include "Module.h"
#include "PermissionService.h"
#include "Registry.h"

namespace WPEFramework {
namespace Plugin {

/**
 * Badger Thunder plugin
 * Exposes JSON-RPC methods to perform permission checks and queries using a YAML-driven registry.
 * Follows Thunder IPlugin + JSONRPC style.
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
     * Destructor unregisters handlers.
     */
    ~Badger() override;

    // IPlugin
    // PUBLIC_INTERFACE
    /**
     * Initialize: load configuration, build registry, setup permission service.
     */
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Deinitialize: release resources.
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
     * Badger.permissions.listMethods -> returns list of supported Badger methods.
     */
    uint32_t endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.check
     * params: { "capability": string, "role": "use|manage|provide" (optional, default "use") }
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
     * returns: array of capability strings derived from current grants via registry mapping.
     */
    uint32_t endpoint_permissions_listCaps(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * Badger.permissions.listFireboltPermissions
     * returns: array of permission identifiers derived from current grants.
     */
    uint32_t endpoint_permissions_listFireboltPermissions(Core::JSON::ArrayType<Core::JSON::String>& response);

private:
    PluginHost::IShell* _service { nullptr };
    std::unique_ptr<Registry> _registry;
    std::unique_ptr<PermissionService> _permService;

    // Basic config
    string _registryPath;
    uint32_t _cacheTtlSeconds { 3600 };
};

} // namespace Plugin
} // namespace WPEFramework
