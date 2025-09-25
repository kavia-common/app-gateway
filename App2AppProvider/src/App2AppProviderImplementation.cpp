#include "App2AppProviderImplementation.h"

#include <core/JSON.h>
#include <core/Portability.h>

#include <sstream>

namespace WPEFramework {
namespace Plugin {

// ----- Lifecycle -----
App2AppProviderImplementation::App2AppProviderImplementation(PluginHost::IShell* service)
    : _refCount(1)
    , _service(service)
    , _gateway(nullptr) {

    LOGTRACE("App2AppProviderImplementation constructed");

    if (_service != nullptr) {
        // Acquire AppGateway COMRPC by callsign "AppGateway"
        _gateway = _service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("AppGateway"));
        if (_gateway == nullptr) {
            LOGERR("Failed to acquire Exchange::IAppGateway via callsign 'AppGateway'");
        } else {
            LOGINFO("Acquired Exchange::IAppGateway");
        }
    } else {
        LOGWARN("Service is null; cannot acquire AppGateway");
    }
}

App2AppProviderImplementation::~App2AppProviderImplementation() {
    LOGTRACE("App2AppProviderImplementation destructed");
    if (_gateway != nullptr) {
        _gateway->Release();
        _gateway = nullptr;
    }
}

// ----- Core::IUnknown -----
uint32_t App2AppProviderImplementation::AddRef() const {
    return Core::InterlockedIncrement(_refCount);
}

uint32_t App2AppProviderImplementation::Release() const {
    uint32_t result = Core::InterlockedDecrement(_refCount);
    if (result == 0) {
        delete this;
        return Core::ERROR_DESTRUCTION_SUCCEEDED;
    }
    return Core::ERROR_NONE;
}

void* App2AppProviderImplementation::QueryInterface(const uint32_t id) {
    if (id == Exchange::IApp2AppProvider::ID) {
        AddRef();
        return static_cast<Exchange::IApp2AppProvider*>(this);
    }
    if (id == Core::IUnknown::ID) {
        AddRef();
        return static_cast<Core::IUnknown*>(this);
    }
    return nullptr;
}

// ----- Helpers -----
bool App2AppProviderImplementation::ParseConnectionId(const std::string& in, uint32_t& out) const {
    if (in.empty()) {
        return false;
    }
    try {
        size_t idx = 0;
        unsigned long val = std::stoul(in, &idx, 10);
        if (idx != in.size()) {
            return false; // trailing or invalid characters
        }
        out = static_cast<uint32_t>(val);
        return true;
    } catch (...) {
        return false;
    }
}

bool App2AppProviderImplementation::ExtractConnectionIdFromPayload(const std::string& payload, uint32_t& outConnectionId) const {
    Core::JSON::VariantContainer obj;
    if (!obj.FromString(payload)) {
        LOGERR("ExtractConnectionIdFromPayload: failed to parse payload JSON");
        return false;
    }

    auto tryExtractFromContainer = [&](Core::JSON::VariantContainer& container) -> bool {
        if (container.HasLabel("connectionId")) {
            const Core::JSON::Variant& v = container["connectionId"];
            if (v.IsString() || v.IsNumber()) {
                std::string s = v.String();
                uint32_t id = 0;
                if (ParseConnectionId(s, id)) {
                    outConnectionId = id;
                    return true;
                }
            }
        }
        return false;
    };

    // 1) Try top-level "connectionId"
    if (tryExtractFromContainer(obj)) {
        return true;
    }

    // 2) Try "contextEcho.connectionId"
    if (obj.HasLabel("contextEcho")) {
        const Core::JSON::Variant& echoVar = obj["contextEcho"];
        if (echoVar.IsObject()) {
            Core::JSON::VariantContainer echoObj = echoVar.Object();
            if (tryExtractFromContainer(echoObj)) {
                return true;
            }
        }
    }

    // 3) Try "context.connectionId"
    if (obj.HasLabel("context")) {
        const Core::JSON::Variant& ctxVar = obj["context"];
        if (ctxVar.IsObject()) {
            Core::JSON::VariantContainer ctxObj = ctxVar.Object();
            if (tryExtractFromContainer(ctxObj)) {
                return true;
            }
        }
    }

    LOGERR("ExtractConnectionIdFromPayload: could not find a parsable connectionId in payload");
    return false;
}

// ----- Public API (Exchange::IApp2AppProvider) -----

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::RegisterProvider(const Context& context,
                                                              bool reg,
                                                              const string& capability,
                                                              Error& error) {
    LOGTRACE("RegisterProvider enter: reqId=%d conn=%s appId=%s capability=%s reg=%d",
             context.requestId,
             context.connectionId.c_str(),
             context.appId.c_str(),
             capability.c_str(),
             reg ? 1 : 0);

    error.code = 0;
    error.message.clear();

    if (capability.empty() || context.appId.empty()) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Missing capability or appId";
        LOGERR("RegisterProvider invalid params: capability/appId empty");
        return Core::ERROR_BAD_REQUEST;
    }

    uint32_t connId = 0;
    if (!ParseConnectionId(context.connectionId, connId)) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid connectionId";
        LOGERR("RegisterProvider invalid connectionId: %s", context.connectionId.c_str());
        return Core::ERROR_BAD_REQUEST;
    }

    {
        std::lock_guard<std::mutex> guard(_lock);
        if (reg) {
            ProviderEntry entry;
            entry.appId = context.appId;
            entry.connectionId = connId;
            entry.registeredAt = Core::Time::Now();

            _capabilityToProvider[capability] = entry;
            _capabilitiesByConnection[connId].insert(capability);

            LOGINFO("Registered provider: capability=%s appId=%s connId=%u",
                    capability.c_str(), entry.appId.c_str(), connId);
        } else {
            auto it = _capabilityToProvider.find(capability);
            if (it == _capabilityToProvider.end()) {
                error.code = Core::ERROR_UNKNOWN_KEY;
                error.message = "Capability not registered";
                LOGWARN("Unregister failed: capability=%s not found", capability.c_str());
                return Core::ERROR_UNKNOWN_KEY;
            }
            if (it->second.connectionId != connId) {
                error.code = Core::ERROR_GENERAL;
                error.message = "Ownership violation for unregister";
                LOGWARN("Unregister ownership violation: capability=%s registered by connId=%u, request from connId=%u",
                        capability.c_str(), it->second.connectionId, connId);
                return Core::ERROR_GENERAL;
            }

            // remove capability mapping
            _capabilityToProvider.erase(it);
            auto itSet = _capabilitiesByConnection.find(connId);
            if (itSet != _capabilitiesByConnection.end()) {
                itSet->second.erase(capability);
                if (itSet->second.empty()) {
                    _capabilitiesByConnection.erase(itSet);
                }
            }
            LOGINFO("Unregistered provider: capability=%s connId=%u", capability.c_str(), connId);
        }
    }

    LOGTRACE("RegisterProvider exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::InvokeProvider(const Context& context,
                                                            const string& capability,
                                                            Error& error) {
    LOGTRACE("InvokeProvider enter: reqId=%d conn=%s appId=%s capability=%s",
             context.requestId, context.connectionId.c_str(), context.appId.c_str(), capability.c_str());

    error.code = 0;
    error.message.clear();

    if (capability.empty() || context.appId.empty()) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Missing capability or appId";
        LOGERR("InvokeProvider invalid params: capability/appId empty");
        return Core::ERROR_BAD_REQUEST;
    }

    uint32_t connId = 0;
    if (!ParseConnectionId(context.connectionId, connId)) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid connectionId";
        LOGERR("InvokeProvider invalid connectionId: %s", context.connectionId.c_str());
        return Core::ERROR_BAD_REQUEST;
    }

    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _capabilityToProvider.find(capability);
        if (it == _capabilityToProvider.end()) {
            error.code = Core::ERROR_UNKNOWN_KEY;
            error.message = "No provider registered for capability";
            LOGWARN("InvokeProvider: no provider for capability=%s", capability.c_str());
            return Core::ERROR_UNKNOWN_KEY;
        }

        // Store consumer context keyed solely by connectionId (canonical)
        ConsumerContext cctx;
        cctx.requestId = static_cast<uint32_t>(context.requestId);
        cctx.connectionId = connId;
        cctx.appId = context.appId;
        cctx.capability = capability;
        cctx.createdAt = Core::Time::Now();

        auto [ctxIt, inserted] = _contextsByConnection.insert_or_assign(connId, std::move(cctx));
        if (!inserted) {
            // insert_or_assign returns iterator; when value was assigned, inserted==false; we log overwrite
            LOGWARN("InvokeProvider: overwrote existing context for connId=%u (non-unique?)", connId);
        }

        LOGINFO("InvokeProvider stored context: capability=%s connId=%u reqId=%u",
                capability.c_str(), connId, static_cast<uint32_t>(context.requestId));
    }

    // Routing to provider is AppGateway's responsibility; nothing to call here.
    LOGTRACE("InvokeProvider exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::HandleProviderResponse(const string& payload,
                                                                    const string& capability,
                                                                    Error& error) {
    LOGTRACE("HandleProviderResponse enter: capability=%s payloadSize=%zu",
             capability.c_str(), payload.size());

    error.code = 0;
    error.message.clear();

    if (payload.empty()) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Empty payload";
        LOGERR("HandleProviderResponse invalid payload (empty)");
        return Core::ERROR_BAD_REQUEST;
    }

    uint32_t connId = 0;
    if (!ExtractConnectionIdFromPayload(payload, connId)) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Missing/invalid connectionId in payload";
        LOGERR("HandleProviderResponse missing/invalid connectionId");
        return Core::ERROR_BAD_REQUEST;
    }

    ConsumerContext ctx;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _contextsByConnection.find(connId);
        if (it == _contextsByConnection.end()) {
            error.code = Core::ERROR_UNKNOWN_KEY;
            error.message = "Unknown or expired connectionId";
            LOGWARN("HandleProviderResponse unknown connectionId=%u", connId);
            return Core::ERROR_UNKNOWN_KEY;
        }
        ctx = it->second;
        _contextsByConnection.erase(it);
    }

    if (_gateway == nullptr) {
        error.code = Core::ERROR_UNAVAILABLE;
        error.message = "AppGateway unavailable";
        LOGERR("HandleProviderResponse: IAppGateway is null");
        return Core::ERROR_UNAVAILABLE;
    }

    Exchange::IAppGateway::Context agCtx;
    agCtx.requestId = static_cast<int>(ctx.requestId);
    agCtx.connectionId = ctx.connectionId;
    agCtx.appId = ctx.appId;

    const Core::hresult rc = _gateway->Respond(agCtx, payload);
    if (rc != Core::ERROR_NONE) {
        error.code = rc;
        error.message = "AppGateway::Respond failed";
        LOGERR("AppGateway::Respond failed rc=%d", rc);
        return rc;
    }

    LOGTRACE("HandleProviderResponse exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
Core::hresult App2AppProviderImplementation::HandleProviderError(const string& payload,
                                                                 const string& capability,
                                                                 Error& error) {
    LOGTRACE("HandleProviderError enter: capability=%s payloadSize=%zu",
             capability.c_str(), payload.size());
    // Behavior identical to response; payload content indicates error semantics to consumer.
    return HandleProviderResponse(payload, capability, error);
}

// PUBLIC_INTERFACE
void App2AppProviderImplementation::CleanupByConnection(const uint32_t connectionId) {
    LOGTRACE("CleanupByConnection enter: connId=%u", connectionId);

    std::lock_guard<std::mutex> guard(_lock);

    // Remove provider registrations for this connection
    auto itSet = _capabilitiesByConnection.find(connectionId);
    if (itSet != _capabilitiesByConnection.end()) {
        for (const auto& cap : itSet->second) {
            _capabilityToProvider.erase(cap);
            LOGINFO("Removed provider registration due to detach: capability=%s connId=%u", cap.c_str(), connectionId);
        }
        _capabilitiesByConnection.erase(itSet);
    }

    // Remove pending context for this connection (if any)
    auto itCtx = _contextsByConnection.find(connectionId);
    if (itCtx != _contextsByConnection.end()) {
        _contextsByConnection.erase(itCtx);
        LOGINFO("Erased pending consumer context for connId=%u", connectionId);
    }

    LOGTRACE("CleanupByConnection exit");
}

} // namespace Plugin
} // namespace WPEFramework
