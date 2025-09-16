#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * BadgerThunderPlugin
 *
 * Thunder plugin exposing Badger permission APIs over JSON-RPC.
 * This class provides method routing only; real logic integration with Thor
 * and a Firebolt/Badger registry should be implemented in the noted extension points.
 */
class BadgerThunderPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    BadgerThunderPlugin(const BadgerThunderPlugin&) = delete;
    BadgerThunderPlugin& operator=(const BadgerThunderPlugin&) = delete;

    // PUBLIC_INTERFACE
    /**
     * Construct the plugin and register JSON-RPC methods.
     */
    BadgerThunderPlugin();

    // PUBLIC_INTERFACE
    /**
     * Destructor unregisters JSON-RPC methods and cleans up state.
     */
    ~BadgerThunderPlugin() override;

    // ---------- IPlugin methods ----------
    // PUBLIC_INTERFACE
    /**
     * Initialize the plugin. Called by Thunder during activation.
     * @param service Pointer to the Thunder shell interface.
     * @return Empty string on success, or an error message to abort activation.
     */
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Deinitialize the plugin. Called by Thunder during deactivation.
     * @param service Pointer to the Thunder shell interface.
     */
    void Deinitialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    /**
     * Provide plugin information string for diagnostics.
     * @return A brief description string.
     */
    string Information() const override;

private:
    // Internal helpers to manage JSON-RPC registration.
    void RegisterAll();
    void UnregisterAll();

    // ---------- JSON-RPC endpoints ----------
    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.ping -> "pong"
     * @param[out] response A string with value "pong".
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_ping(Core::JSON::String& response);

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listMethods
     * Return a static list of supported (or planned) Badger permission methods.
     * @param[out] response Array of method names.
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.check
     * Stub that always returns true (allowed) for now.
     * Extension point: integrate with ThorPermissionService & YAML registry to evaluate
     * capability + role against current grants.
     *
     * Expected parameters (for future logic):
     * {
     *   "capability": "string",          // Firebolt capability identifier
     *   "role": "use|manage|provide"     // optional; default "use"
     * }
     *
     * @param[in] parameters Opaque JSON parameters (ignored in stub).
     * @param[out] response Boolean allowed/denied.
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_permissions_check(const Core::JSON::Variant& parameters,
                                        Core::JSON::Boolean& response);

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.checkAll
     * Stub that returns a static array describing sample results.
     * Extension point: accept an array of {capability, role?} and compute per-item allowed.
     *
     * Example future parameters:
     * { "items": [ { "capability": "IntegratedPlayer.create", "role":"use" }, ... ] }
     *
     * @param[in] parameters Opaque JSON parameters (ignored in stub).
     * @param[out] response Array of "capability:result" strings for illustration.
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_permissions_checkAll(const Core::JSON::Variant& parameters,
                                           Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listCaps
     * Stub that returns a static list of capabilities.
     * Extension point: build this from the dynamic granted Thor IDs resolved to Firebolt
     * capabilities via the registry.
     *
     * @param[out] response Array of capability strings.
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_permissions_listCaps(Core::JSON::ArrayType<Core::JSON::String>& response);

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listFireboltPermissions
     * Stub that returns a static list of Thor/Badger permission IDs.
     * Extension point: return currently granted permissions for the caller context.
     *
     * @param[out] response Array of permission ID strings.
     * @return Core::ERROR_NONE on success.
     */
    uint32_t endpoint_permissions_listFireboltPermissions(Core::JSON::ArrayType<Core::JSON::String>& response);

private:
    PluginHost::IShell* _service { nullptr };
};

} // namespace Plugin
} // namespace WPEFramework
