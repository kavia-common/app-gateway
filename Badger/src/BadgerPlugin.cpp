#include "Badger/BadgerPlugin.h"

namespace {

// Request DTOs ----------------------------------------------------

struct CheckItem : public WPEFramework::Core::JSON::Container {
    WPEFramework::Core::JSON::String capability;
    WPEFramework::Core::JSON::String role;

    CheckItem() {
        Add(_T("capability"), &capability);
        Add(_T("role"), &role);
    }
};

struct CheckAllParams : public WPEFramework::Core::JSON::Container {
    WPEFramework::Core::JSON::ArrayType<CheckItem> items;
    CheckAllParams() {
        Add(_T("items"), &items);
    }
};

struct CheckParams : public WPEFramework::Core::JSON::Container {
    WPEFramework::Core::JSON::String capability;
    WPEFramework::Core::JSON::String role;
    CheckParams() {
        Add(_T("capability"), &capability);
        Add(_T("role"), &role);
    }
};

// Helpers ---------------------------------------------------------

static std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Normalize "role" to one of: "use", "manage", "provide" (default "use").
// Returns empty string if invalid (caller should treat as bad request).
static std::string NormalizeRole(const std::string& role) {
    if (role.empty()) return "use";
    const auto r = ToLower(role);
    if (r == "use" || r == "manage" || r == "provide") {
        return r;
    }
    return std::string();
}

} // namespace

namespace WPEFramework {
namespace Plugin {

void Badger::RegisterAll() {
    Register(_T("ping"), &Badger::endpoint_ping, this);
    Register(_T("permissions.listMethods"), &Badger::endpoint_permissions_listMethods, this);
    Register(_T("permissions.check"), &Badger::endpoint_permissions_check, this);
    Register(_T("permissions.checkAll"), &Badger::endpoint_permissions_checkAll, this);
    Register(_T("permissions.listCaps"), &Badger::endpoint_permissions_listCaps, this);
    Register(_T("permissions.listFireboltPermissions"), &Badger::endpoint_permissions_listFireboltPermissions, this);
}

void Badger::UnregisterAll() {
    Unregister(_T("ping"));
    Unregister(_T("permissions.listMethods"));
    Unregister(_T("permissions.check"));
    Unregister(_T("permissions.checkAll"));
    Unregister(_T("permissions.listCaps"));
    Unregister(_T("permissions.listFireboltPermissions"));
}

Badger::Badger()
: PluginHost::JSONRPC()
, _service(nullptr) {
    RegisterAll();
}

Badger::~Badger() {
    UnregisterAll();
}

const string Badger::Initialize(PluginHost::IShell* service) {
    _service = service;

    // Configure from plugin.json -> configuration {}
    string configLine = service->ConfigLine();
    Core::JSON::Variant js;
    std::vector<string> cfgGrantedIds;
    if ((Core::JSON::Variant::FromString(configLine, js) == true) && (js.Content() == Core::JSON::Variant::type::OBJECT)) {
        auto& obj = static_cast<Core::JSON::Object&>(js);
        Core::JSON::String path;
        Core::JSON::DecUInt32 ttl;
        Core::JSON::ArrayType<Core::JSON::String> granted;
        Core::JSON::String logLevel; // optionally parsed (not used in this implementation)

        if (obj.Get(_T("registryPath"), path) == true) {
            _registryPath = path.Value();
        }
        if (obj.Get(_T("cacheTtlSeconds"), ttl) == true) {
            _cacheTtlSeconds = static_cast<uint32_t>(ttl.Value());
        }
        if (obj.Get(_T("grantedIds"), granted) == true) {
            for (auto it = granted.Elements(); it.Next();) {
                const auto v = it.Current().Value();
                if (!v.empty()) {
                    cfgGrantedIds.emplace_back(v);
                }
            }
        }
        // Optional logLevel (currently unused)
        (void)obj.Get(_T("logLevel"), logLevel);
    }
    if (_registryPath.empty()) {
        _registryPath = "/etc/badger/thor_permission_registry.yaml";
    }

    // Load registry
    string err;
    _registry = Registry::LoadFromFile(_registryPath, err);
    if (!_registry) {
        return string("Badger: registry load failed: ") + err;
    }

    _permService = std::make_unique<PermissionService>(*_registry, _cacheTtlSeconds);

    // Apply configured grantedIds if provided; otherwise use a safe development default.
    std::unordered_set<string> ids;
    if (!cfgGrantedIds.empty()) {
        ids.insert(cfgGrantedIds.begin(), cfgGrantedIds.end());
    } else {
        // Development fallback. In production, wire an external provider and set dynamically.
        ids = {
            "DATA_timeZone",
            "ACCESS_integratedPlayer_create",
            "APP_lifecycle_ready"
        };
    }
    _permService->SetGrantedIds(ids);

    return string(); // success
}

void Badger::Deinitialize(PluginHost::IShell* /*service*/) {
    _permService.reset();
    _registry.reset();
    _service = nullptr;
}

string Badger::Information() const {
    return string(_T("Badger: Permission abstraction plugin (Thunder)"));
}

// ---- JSON-RPC endpoints ------------------------------------------------------

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_ping(Core::JSON::String& response) {
    response = _T("pong");
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_permissions_listMethods(Core::JSON::ArrayType<Core::JSON::String>& response) {
    const char* methods[] = {
        "org.rdk.Badger.permissions.check",
        "org.rdk.Badger.permissions.checkAll",
        "org.rdk.Badger.permissions.listCaps",
        "org.rdk.Badger.permissions.listFireboltPermissions",
        "org.rdk.Badger.permissions.listMethods",
        "org.rdk.Badger.ping"
    };
    for (auto m : methods) {
        Core::JSON::String s; s = _T(m);
        response.Add(s);
    }
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_permissions_check(const Core::JSON::Variant& parameters, Core::JSON::Boolean& response) {
    CheckParams params;
    if (parameters.Content() == Core::JSON::Variant::type::OBJECT) {
        params.FromString(parameters.ToString());
    }
    const string cap = params.capability.Value();
    if (cap.empty()) {
        // Missing or invalid capability
        return Core::ERROR_BAD_REQUEST;
    }
    const auto normalizedRole = NormalizeRole(params.role.IsSet() ? params.role.Value() : "use");
    if (normalizedRole.empty()) {
        // Role provided but not one of allowed values
        return Core::ERROR_BAD_REQUEST;
    }
    const bool allowed = _permService->CheckCapability(cap, normalizedRole);
    response = allowed;
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_permissions_checkAll(const Core::JSON::Variant& parameters, Core::JSON::ArrayType<Core::JSON::Object>& response) {
    CheckAllParams params;
    if (parameters.Content() == Core::JSON::Variant::type::OBJECT) {
        params.FromString(parameters.ToString());
    }
    std::vector<std::pair<string,string>> items;
    for (auto it = params.items.Elements(); it.Next();) {
        auto& ci = it.Current();
        const auto cap = ci.capability.Value();
        if (cap.empty()) {
            continue;
        }
        const auto normRole = NormalizeRole(ci.role.IsSet() ? ci.role.Value() : "use");
        if (normRole.empty()) {
            // Skip invalid roles instead of failing the whole request.
            continue;
        }
        items.emplace_back(cap, normRole);
    }
    auto results = _permService->CheckAll(items);
    for (const auto& t : results) {
        Core::JSON::Object obj;
        Core::JSON::String cap; cap = std::get<0>(t).c_str();
        Core::JSON::String role; role = std::get<1>(t).c_str();
        Core::JSON::Boolean allowed; allowed = std::get<2>(t);
        obj.Set(_T("capability"), cap);
        obj.Set(_T("role"), role);
        obj.Set(_T("allowed"), allowed);
        response.Add(obj);
    }
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_permissions_listCaps(Core::JSON::ArrayType<Core::JSON::String>& response) {
    auto caps = _permService->ListCapabilities();
    for (const auto& c : caps) {
        Core::JSON::String v; v = c.c_str();
        response.Add(v);
    }
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t Badger::endpoint_permissions_listFireboltPermissions(Core::JSON::ArrayType<Core::JSON::String>& response) {
    auto perms = _permService->ListFireboltPermissions();
    for (const auto& p : perms) {
        Core::JSON::String v; v = p.c_str();
        response.Add(v);
    }
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
