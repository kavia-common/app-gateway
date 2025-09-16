#include "BadgerThunder/BadgerThunderPlugin.h"

namespace WPEFramework {
namespace Plugin {

    // ---- JSON-RPC registration helpers ----
    void BadgerThunderPlugin::RegisterAll() {
        // Note: Register API varies by Thunder version; adapt to your platform if needed.

        // BadgerThunder.ping -> "pong"
        Register(_T("ping"),
                 &BadgerThunderPlugin::endpoint_ping,
                 this);

        // BadgerThunder.permissions.listMethods -> [ "org.rdk.Badger.permissions.check", ... ]
        Register(_T("permissions.listMethods"),
                 &BadgerThunderPlugin::endpoint_permissions_listMethods,
                 this);

        // BadgerThunder.permissions.check -> boolean
        Register(_T("permissions.check"),
                 &BadgerThunderPlugin::endpoint_permissions_check,
                 this);

        // BadgerThunder.permissions.checkAll -> array
        Register(_T("permissions.checkAll"),
                 &BadgerThunderPlugin::endpoint_permissions_checkAll,
                 this);

        // BadgerThunder.permissions.listCaps -> array
        Register(_T("permissions.listCaps"),
                 &BadgerThunderPlugin::endpoint_permissions_listCaps,
                 this);

        // BadgerThunder.permissions.listFireboltPermissions -> array
        Register(_T("permissions.listFireboltPermissions"),
                 &BadgerThunderPlugin::endpoint_permissions_listFireboltPermissions,
                 this);
    }

    void BadgerThunderPlugin::UnregisterAll() {
        Unregister(_T("ping"));
        Unregister(_T("permissions.listMethods"));
        Unregister(_T("permissions.check"));
        Unregister(_T("permissions.checkAll"));
        Unregister(_T("permissions.listCaps"));
        Unregister(_T("permissions.listFireboltPermissions"));
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

        // Extension point: read configuration (e.g., registryPath, logLevel) from plugin.json
        // via service->ConfigLine() and configure underlying services or clients.

        // Return empty string on success; non-empty to abort activation with message.
        return string();
    }

    void BadgerThunderPlugin::Deinitialize(PluginHost::IShell* /*service*/) {
        BADGER_LOG_INFO("BadgerThunderPlugin Deinitialize called.");
        _service = nullptr;
    }

    string BadgerThunderPlugin::Information() const {
        return string(_T("BadgerThunder: Thunder plugin for Badger permission APIs"));
    }

    // ---- JSON-RPC endpoints ----

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.ping -> "pong"
     */
    uint32_t BadgerThunderPlugin::endpoint_ping(Core::JSON::String& response) {
        response = _T("pong");
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listMethods
     * Lists exposed Badger permission API method names (fully-qualified).
     */
    uint32_t BadgerThunderPlugin::endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response) {
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

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.check
     * Stub returns "true" for all inputs.
     * Extension point:
     *  - Read parameters: { "capability": string, "role": "use|manage|provide" }
     *  - Call ThorPermissionService to fetch grants
     *  - Resolve capability+role via YAML registry and evaluate intersection
     */
    uint32_t BadgerThunderPlugin::endpoint_permissions_check(const Core::JSON::Variant& /*parameters*/,
                                                             Core::JSON::Boolean& response) {
        response = true;
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.checkAll
     * Stub returns a sample array; replace with per-item evaluation.
     */
    uint32_t BadgerThunderPlugin::endpoint_permissions_checkAll(const Core::JSON::Variant& /*parameters*/,
                                                                Core::JSON::ArrayType<Core::JSON::String>& response) {
        // Example placeholder results; real implementation should return structured data.
        const char* items[] = {
            "IntegratedPlayer.create:true",
            "Lifecycle.onRequestReady:true",
            "AcknowledgeChallenge.challenge:false"
        };
        for (const auto& it : items) {
            Core::JSON::String v;
            v = _T(it);
            response.Add(v);
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listCaps
     * Stub returns a static set of capability names.
     */
    uint32_t BadgerThunderPlugin::endpoint_permissions_listCaps(Core::JSON::ArrayType<Core::JSON::String>& response) {
        const char* caps[] = {
            "IntegratedPlayer.create",
            "Lifecycle.onRequestReady",
            "AcknowledgeChallenge.challenge"
        };
        for (const auto& c : caps) {
            Core::JSON::String v;
            v = _T(c);
            response.Add(v);
        }
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * JSON-RPC: BadgerThunder.permissions.listFireboltPermissions
     * Stub returns sample Thor/Badger permission IDs.
     */
    uint32_t BadgerThunderPlugin::endpoint_permissions_listFireboltPermissions(Core::JSON::ArrayType<Core::JSON::String>& response) {
        const char* perms[] = {
            "DATA_timeZone",
            "ACCESS_integratedPlayer_create",
            "APP_lifecycle_ready"
        };
        for (const auto& p : perms) {
            Core::JSON::String v;
            v = _T(p);
            response.Add(v);
        }
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
