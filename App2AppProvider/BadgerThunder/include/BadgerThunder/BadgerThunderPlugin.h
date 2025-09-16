#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * BadgerThunderPlugin
 *
 * Minimal Thunder plugin skeleton that exposes a tiny JSON-RPC surface and
 * provides hooks to extend with Badger/Thor permission logic.
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

    // ---------- JSON-RPC endpoints (skeleton) ----------
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

private:
    PluginHost::IShell* _service { nullptr };
};

} // namespace Plugin
} // namespace WPEFramework
