#include "App2AppProviderImplementation.h"

#include <sstream>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

App2AppProviderImplementation::App2AppProviderImplementation(PluginHost::IShell* service)
    : _service(service)
    , _gateway(nullptr) {
    LOGTRACE("App2AppProviderImplementation constructed");
    EnsureGateway();
}

App2AppProviderImplementation::~App2AppProviderImplementation() {
    LOGTRACE("App2AppProviderImplementation destructed");
    if (_gateway != nullptr) {
        _gateway->Release();
        _gateway = nullptr;
    }
}

// IUnknown
uint32_t App2AppProviderImplementation::AddRef() const {
    return ++_refCount;
}

uint32_t App2AppProviderImplementation::Release() const {
    uint32_t result = --_refCount;
    if (result == 0) {
        delete const_cast<App2AppProviderImplementation*>(this);
    }
    return result;
}

void* App2AppProviderImplementation::QueryInterface(const uint32_t id) {
    if (id == Exchange::IApp2AppProvider::ID) {
        AddRef();
        return static_cast<Exchange::IApp2AppProvider*>(this);
    }
    return nullptr;
}

// Helpers
bool App2AppProviderImplementation::EnsureGateway() {
    if (_gateway != nullptr) {
        return true;
    }
    if (_service == nullptr) {
        return false;
    }
    // Acquire AppGateway via callsign "AppGateway"
    Exchange::IAppGateway* gw = _service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("AppGateway"));
    if (gw != nullptr) {
        _gateway = gw; // take ownership
        return true;
    }
    return false;
}

bool App2AppProviderImplementation::ParseConnectionId(const std::string& in, uint32_t& outConnId) const {
    // Accept numeric strings, optionally with whitespace.
    try {
        size_t idx = 0;
        // Trim spaces
        size_t start = in.find_first_not_of(' ');
        if (start == std::string::npos) {
            return false;
        }
        size_t end = in.find_last_not_of(' ');
        std::string trimmed = in.substr(start, end - start + 1);
        // Allow base 10 only to avoid surprises.
        unsigned long v = std::stoul(trimmed, &idx, 10);
        if (idx != trimmed.size()) {
            return false;
        }
        if (v > 0xFFFFFFFFul) {
            return false;
        }
        outConnId = static_cast<uint32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

std::string App2AppProviderImplementation::GenerateCorrelationId() {
    // Reproduce backup logic: "ticks-counter" where ticks are microseconds since epoch.
    const uint64_t ticks = Core::Time::Now().Ticks();
    const uint64_t count = ++_corrCounter;
    std::ostringstream oss;
    oss << ticks << "-" << count;
    return oss.str();
}

// Exchange::IApp2AppProvider
Core::hresult App2AppProviderImplementation::RegisterProvider(const Context& context,
                                                              bool reg,
                                                              const string& capability,
                                                              Error& error) {
    LOGTRACE("RegisterProvider enter: capability=%s appId=%s connStr=%s req=%d",
             capability.c_str(), context.appId.c_str(), context.connectionId.c_str(), context.requestId);

    error.code = Core::ERROR_NONE;
    error.message.clear();

    if (capability.empty() || context.appId.empty() || context.connectionId.empty()) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid parameters";
        LOGERR("RegisterProvider invalid params");
        return static_cast<Core::hresult>(error.code);
    }

    uint32_t connId = 0;
    if (!ParseConnectionId(context.connectionId, connId)) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid connectionId";
        LOGERR("RegisterProvider invalid connectionId: '%s'", context.connectionId.c_str());
        return static_cast<Core::hresult>(error.code);
    }

    {
        std::lock_guard<std::mutex> guard(_lock);
        if (reg) {
            ProviderEntry e;
            e.appId = context.appId;
            e.connectionId = connId;
            e.registeredAt = Core::Time::Now();
            _capabilityToProvider[capability] = e;
            _capabilitiesByConnection[connId].insert(capability);
            LOGINFO("Provider registered: capability=%s appId=%s connId=%u", capability.c_str(), e.appId.c_str(), connId);
        } else {
            auto it = _capabilityToProvider.find(capability);
            if (it == _capabilityToProvider.end()) {
                // nothing to do
                LOGWARN("UnregisterProvider: capability not present: %s", capability.c_str());
            } else if (it->second.connectionId != connId) {
                error.code = Core::ERROR_GENERAL; // ownership violation
                error.message = "Unregister not allowed: not owner";
                LOGERR("UnregisterProvider: ownership violation connId=%u owner=%u", connId, it->second.connectionId);
                return static_cast<Core::hresult>(error.code);
            } else {
                _capabilityToProvider.erase(it);
                auto rcIt = _capabilitiesByConnection.find(connId);
                if (rcIt != _capabilitiesByConnection.end()) {
                    rcIt->second.erase(capability);
                    if (rcIt->second.empty()) {
                        _capabilitiesByConnection.erase(rcIt);
                    }
                }
                LOGINFO("Provider unregistered: capability=%s connId=%u", capability.c_str(), connId);
            }
        }
    }

    LOGTRACE("RegisterProvider exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

Core::hresult App2AppProviderImplementation::InvokeProvider(const Context& context,
                                                            const string& capability,
                                                            Error& error) {
    LOGTRACE("InvokeProvider enter: capability=%s appId=%s connStr=%s req=%d",
             capability.c_str(), context.appId.c_str(), context.connectionId.c_str(), context.requestId);

    error.code = Core::ERROR_NONE;
    error.message.clear();

    if (capability.empty() || context.appId.empty() || context.connectionId.empty() || context.requestId <= 0) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid parameters";
        LOGERR("InvokeProvider invalid params");
        return static_cast<Core::hresult>(error.code);
    }

    uint32_t consumerConnId = 0;
    if (!ParseConnectionId(context.connectionId, consumerConnId)) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid connectionId";
        LOGERR("InvokeProvider invalid connectionId: '%s'", context.connectionId.c_str());
        return static_cast<Core::hresult>(error.code);
    }

    // Validate provider exists
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _capabilityToProvider.find(capability);
        if (it == _capabilityToProvider.end()) {
            error.code = Core::ERROR_UNKNOWN_KEY;
            error.message = "Capability not found";
            LOGWARN("InvokeProvider: capability not registered: %s", capability.c_str());
            return static_cast<Core::hresult>(error.code);
        }
    }

    // Create correlation and store consumer context
    const std::string corrId = GenerateCorrelationId();
    {
        std::lock_guard<std::mutex> guard(_lock);
        ConsumerContext cc;
        cc.requestId = context.requestId;
        cc.connectionId = consumerConnId;
        cc.appId = context.appId;
        cc.capability = capability;
        cc.createdAt = Core::Time::Now();
        _correlations.emplace(corrId, std::move(cc));
    }

    // Note: As per best-judgement due to interface limitations, we return the
    // generated correlationId to AppGateway using error.message on success.
    // AppGateway should propagate this correlationId to the provider request and
    // include it back in HandleProviderResponse/HandleProviderError payloads.
    error.code = Core::ERROR_NONE;
    error.message = corrId;
    LOGINFO("InvokeProvider correlationId=%s for capability=%s", corrId.c_str(), capability.c_str());

    LOGTRACE("InvokeProvider exit: hr=%d", Core::ERROR_NONE);
    return Core::ERROR_NONE;
}

Core::hresult App2AppProviderImplementation::HandleProviderResultLike_(const std::string& payload,
                                                                       const std::string& capability,
                                                                       bool isError,
                                                                       Error& error) {
    error.code = Core::ERROR_NONE;
    error.message.clear();

    if (payload.empty() || capability.empty()) {
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "Invalid parameters";
        LOGERR("HandleProvider%s invalid params", isError ? "Error" : "Response");
        return static_cast<Core::hresult>(error.code);
    }

    // Try to parse payload to extract correlationId:
    std::string corrId;
    {
        Core::JSON::VariantContainer root;
        bool parsed = root.FromString(payload);
        if (parsed) {
            Core::JSON::Variant v = root.Get("correlationId");
            if (v.IsSet()) {
                corrId = v.String();
            } else {
                // Try nested "context": { "correlationId": ... }
                Core::JSON::Variant ctx = root.Get("context");
                if (ctx.IsSet()) {
                    Core::JSON::VariantContainer ctxObj = ctx.Object();
                    Core::JSON::Variant c2 = ctxObj.Get("correlationId");
                    if (c2.IsSet()) {
                        corrId = c2.String();
                    }
                }
            }
        } else {
            // If payload is not JSON, cannot extract correlationId -> error
            LOGERR("HandleProvider%s: payload not JSON; cannot extract correlationId",
                   isError ? "Error" : "Response");
            error.code = Core::ERROR_BAD_REQUEST;
            error.message = "Payload parse error: correlationId missing";
            return static_cast<Core::hresult>(error.code);
        }
    }

    if (corrId.empty()) {
        LOGERR("HandleProvider%s: correlationId missing", isError ? "Error" : "Response");
        error.code = Core::ERROR_BAD_REQUEST;
        error.message = "correlationId missing";
        return static_cast<Core::hresult>(error.code);
    }

    // Lookup correlation
    ConsumerContext cc;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _correlations.find(corrId);
        if (it == _correlations.end()) {
            LOGERR("HandleProvider%s: unknown correlationId=%s", isError ? "Error" : "Response", corrId.c_str());
            error.code = Core::ERROR_INCORRECT_URL; // using as "not found"
            error.message = "Unknown correlationId";
            return static_cast<Core::hresult>(error.code);
        }
        cc = it->second;
        _correlations.erase(it);
    }

    // Send to AppGateway->Respond
    if (!EnsureGateway() || _gateway == nullptr) {
        LOGERR("AppGateway unavailable");
        error.code = Core::ERROR_UNAVAILABLE;
        error.message = "AppGateway unavailable";
        return static_cast<Core::hresult>(error.code);
    }

    Exchange::IAppGateway::Context ctx;
    ctx.requestId = cc.requestId;
    ctx.connectionId = cc.connectionId;
    ctx.appId = cc.appId;

    const Core::hresult rc = _gateway->Respond(ctx, payload);
    if (rc != Core::ERROR_NONE) {
        LOGERR("IAppGateway::Respond failed rc=%d", rc);
        error.code = rc;
        error.message = std::string("Respond failed");
        return rc;
    }

    LOGINFO("Forwarded provider %s for capability=%s correlationId=%s to consumer (connId=%u, reqId=%d)",
            (isError ? "error" : "response"),
            capability.c_str(),
            corrId.c_str(),
            ctx.connectionId,
            ctx.requestId);

    return Core::ERROR_NONE;
}

Core::hresult App2AppProviderImplementation::HandleProviderResponse(const string& payload,
                                                                    const string& capability,
                                                                    Error& error) {
    LOGTRACE("HandleProviderResponse enter: capability=%s", capability.c_str());
    Core::hresult hr = HandleProviderResultLike_(payload, capability, false, error);
    LOGTRACE("HandleProviderResponse exit: hr=%d", hr);
    return hr;
}

Core::hresult App2AppProviderImplementation::HandleProviderError(const string& payload,
                                                                 const string& capability,
                                                                 Error& error) {
    LOGTRACE("HandleProviderError enter: capability=%s", capability.c_str());
    Core::hresult hr = HandleProviderResultLike_(payload, capability, true, error);
    LOGTRACE("HandleProviderError exit: hr=%d", hr);
    return hr;
}

void App2AppProviderImplementation::CleanupByConnection(uint32_t connectionId) {
    LOGTRACE("CleanupByConnection enter: connId=%u", connectionId);
    std::lock_guard<std::mutex> guard(_lock);

    // Remove providers owned by this connection
    auto capsIt = _capabilitiesByConnection.find(connectionId);
    if (capsIt != _capabilitiesByConnection.end()) {
        for (const auto& cap : capsIt->second) {
            _capabilityToProvider.erase(cap);
            LOGINFO("Removed provider registration: capability=%s connId=%u", cap.c_str(), connectionId);
        }
        _capabilitiesByConnection.erase(capsIt);
    }

    // Remove correlations owned by this connection
    for (auto it = _correlations.begin(); it != _correlations.end();) {
        if (it->second.connectionId == connectionId) {
            LOGINFO("Removed correlationId=%s due to connection cleanup", it->first.c_str());
            it = _correlations.erase(it);
        } else {
            ++it;
        }
    }
    LOGTRACE("CleanupByConnection exit");
}
