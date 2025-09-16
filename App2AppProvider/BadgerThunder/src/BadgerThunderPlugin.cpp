#include "BadgerThunder/BadgerThunderPlugin.h"

namespace WPEFramework {
namespace Plugin {

    // ---- JSON-RPC registration helpers ----
    void BadgerThunderPlugin::RegisterAll() {
        // Note: Register API varies by Thunder version; the below illustrates intent.
        // If your environment requires typed templates, adapt to Register<void, Core::JSON::String>(...) etc.

        // BadgerThunder.ping -> "pong"
        Register(_T("ping"),
                 &BadgerThunderPlugin::endpoint_ping,
                 this);

        // BadgerThunder.permissions.listMethods -> [ "org.rdk.Badger.permissions.check", ... ]
        Register(_T("permissions.listMethods"),
                 &BadgerThunderPlugin::endpoint_permissions_listMethods,
                 this);
    }

    void BadgerThunderPlugin::UnregisterAll() {
        Unregister(_T("ping"));
        Unregister(_T("permissions.listMethods"));
    }

    // ---- Construction / Destruction ----
    BadgerThunderPlugin::BadgerThunderPlugin()
    : PluginHost::JSONRPC(), _service(nullptr) {
        RegisterAll();
        BADGER_LOG_INFO("BadgerThunderPlugin constructed and JSON-RPC methods registered.");
    }

    BadgerThunderPlugin::~BadgerThunderPlugin() {
        UnregisterAll();
        BADGER_LOG_INFO("BadgerThunderPlugin destroyed and JSON-RPC methods unregistered.");
    }

    // ---- IPlugin ----
    const string BadgerThunderPlugin::Initialize(PluginHost::IShell* service) {
        _service = service;
        BADGER_LOG_INFO("BadgerThunderPlugin Initialize called.");

        // Read configuration if needed (example keys described in plugin.json).
        // You can access the config through service->ConfigLine() or JSON parsing of plugin config.

        // Return empty string on success; non-empty to abort activation with message.
        return string();
    }

    void BadgerThunderPlugin::Deinitialize(PluginHost::IShell* /*service*/) {
        BADGER_LOG_INFO("BadgerThunderPlugin Deinitialize called.");
        _service = nullptr;
    }

    string BadgerThunderPlugin::Information() const {
        return string(_T("BadgerThunder: Minimal Thunder plugin scaffold for Badger permission APIs")));
    }

    // ---- JSON-RPC endpoints ----
    uint32_t BadgerThunderPlugin::endpoint_ping(Core::JSON::String& response) {
        response = _T("pong");
        return Core::ERROR_NONE;
    }

    uint32_t BadgerThunderPlugin::endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response) {
        // Stubbed list of Badger permission method names (to be implemented in future work).
        // Exposed via BadgerThunder.permissions.listMethods
        const char* methods[] = {
            "org.rdk.Badger.permissions.check",
            "org.rdk.Badger.permissions.checkAll",
            "org.rdk.Badger.permissions.listCaps",
            "org.rdk.Badger.permissions.listFireboltPermissions",
            "org.rdk.Badger.permissions.listMethods"
        };

        for (const auto& m : methods) {
            Core::JSON::String item;
            item = _T(m);
            response.Add(item);
        }

        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
