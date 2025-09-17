#include "App2AppProvider.h"

#include <core/Enumerate.h>
#include <core/Trace.h>

namespace WPEFramework {
namespace Plugin {

namespace {
constexpr const char* kMethodRegisterProvider = "registerProvider";
constexpr const char* kMethodInvokeProvider = "invokeProvider";
constexpr const char* kMethodHandleProviderResponse = "handleProviderResponse";
constexpr const char* kMethodHandleProviderError = "handleProviderError";

inline uint32_t ToRpcErrorInvalidParams() { return static_cast<uint32_t>(Core::ERROR_BAD_REQUEST); }
inline uint32_t ToRpcErrorInvalidRequest() { return static_cast<uint32_t>(Core::ERROR_INCORRECT_URL); }
} // namespace

// --------------------- App2AppProvider ---------------------

App2AppProvider::App2AppProvider()
    : PluginHost::JSONRPC()
    , _service(nullptr) {
    // Register JSON-RPC methods
    Register<int, void>(kMethodRegisterProvider, &App2AppProvider::registerProvider, this);
    Register<int, void>(kMethodInvokeProvider, &App2AppProvider::invokeProvider, this);
    Register<int, void>(kMethodHandleProviderResponse, &App2AppProvider::handleProviderResponse, this);
    Register<int, void>(kMethodHandleProviderError, &App2AppProvider::handleProviderError, this);
}

App2AppProvider::~App2AppProvider() {
    // Unregister
    Unregister(kMethodRegisterProvider);
    Unregister(kMethodInvokeProvider);
    Unregister(kMethodHandleProviderResponse);
    Unregister(kMethodHandleProviderError);
}

const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    _service = service;
    _appGateway = std::make_unique<AppGatewayClient>(_service);
    return string(); // empty string indicates success in Thunder
}

void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
    std::lock_guard<std::mutex> guard(_adminLock);
    _appGateway.reset();
    _correlations = CorrelationStore();
    _providers = ProviderRegistry();
    _service = nullptr;
}

string App2AppProvider::Information() const {
    return string("App2AppProvider: manages provider capabilities, invocation correlation, and response routing.");
}

void App2AppProvider::Attach(PluginHost::Channel& /*channel*/) {
    // Nothing to do on open
}
void App2AppProvider::Detach(PluginHost::Channel& channel) {
    // Cleanup provider registrations and pending correlations for the closed connection
    const uint32_t id = channel.Id();
    {
        std::lock_guard<std::mutex> guard(_adminLock);
        _providers.CleanupByConnection(id);
        _correlations.CleanupByConnection(id);
    }
}

// --------------------- Helpers ---------------------

bool App2AppProvider::ExtractContext(const Core::JSON::VariantContainer& in,
                                     uint32_t& requestId,
                                     uint32_t& connectionId,
                                     string& appId,
                                     string* capability) {
    if (in.HasLabel("context") == false) {
        return false;
    }
    auto context = in["context"].Object();

    if (context.HasLabel("requestId") == false ||
        context.HasLabel("connectionId") == false ||
        context.HasLabel("appId") == false) {
        return false;
    }

    bool ok = true;
    requestId = static_cast<uint32_t>(context["requestId"].Number(ok));
    if (!ok) return false;

    // Accept both numeric or string convertible to uint32
    if (context["connectionId"].IsNumber() == true) {
        connectionId = static_cast<uint32_t>(context["connectionId"].Number(ok));
        if (!ok) return false;
    } else {
        string connStr = context["connectionId"].String();
        try {
            connectionId = static_cast<uint32_t>(std::stoul(connStr));
        } catch (...) {
            return false;
        }
    }

    appId = context["appId"].String();
    if (appId.empty()) return false;

    if (capability != nullptr) {
        if (!in.HasLabel("capability")) return false;
        *capability = in["capability"].String();
        if (capability->empty()) return false;
    }
    return true;
}

bool App2AppProvider::ExtractPayloadCorrelation(const Core::JSON::VariantContainer& in,
                                                string& capability,
                                                string& payloadJson,
                                                string& correlationId) {
    if (!in.HasLabel("capability") || !in.HasLabel("payload")) {
        return false;
    }
    capability = in["capability"].String();
    if (capability.empty()) {
        return false;
    }

    // payload is opaque JSON. We will attempt to extract correlationId from it.
    // Keep original JSON for forwarding to AppGateway.respond
    Core::JSON::VariantContainer payload = in["payload"].Object();
    // Serialize payload object as JSON string
    {
        string serialized;
        Core::JSON::Parser parser;
        payload.ToString(serialized);
        payloadJson = serialized;
    }

    if (!payload.HasLabel("correlationId")) {
        return false;
    }
    correlationId = payload["correlationId"].String();
    if (correlationId.empty()) {
        return false;
    }
    return true;
}

uint32_t App2AppProvider::JsonRpcErrorInvalidParams() {
    // Map to -32602 as per design. In Thunder, we return Core::ERROR_BAD_REQUEST then
    // JSONRPC will serialize appropriately.
    return ToRpcErrorInvalidParams();
}

uint32_t App2AppProvider::JsonRpcErrorInvalidRequest() {
    // Map to -32699 as per design. Using a generic incorrect URL/error for handler signal.
    return ToRpcErrorInvalidRequest();
}

// --------------------- JSON-RPC Handlers ---------------------

uint32_t App2AppProvider::registerProvider(const Core::JSON::VariantContainer& params,
                                           Core::JSON::VariantContainer& response) {
    uint32_t requestId = 0, connectionId = 0;
    string appId;
    string capability;
    if (!ExtractContext(params, requestId, connectionId, appId, &capability)) {
        return JsonRpcErrorInvalidParams();
    }

    if (!params.HasLabel("register")) {
        return JsonRpcErrorInvalidParams();
    }
    bool regFlag = params["register"].Boolean();

    std::lock_guard<std::mutex> guard(_adminLock);
    if (regFlag) {
        _providers.Register(capability, appId, connectionId);
    } else {
        auto rc = _providers.Unregister(capability, connectionId);
        if (rc != Core::ERROR_NONE) {
            return JsonRpcErrorInvalidRequest();
        }
    }
    response.Clear();
    return Core::ERROR_NONE;
}

uint32_t App2AppProvider::invokeProvider(const Core::JSON::VariantContainer& params,
                                         Core::JSON::VariantContainer& response) {
    uint32_t requestId = 0, connectionId = 0;
    string appId;
    string capability;
    if (!ExtractContext(params, requestId, connectionId, appId, &capability)) {
        return JsonRpcErrorInvalidParams();
    }

    // Validate provider exists
    ProviderRegistry::ProviderEntry entry;
    {
        std::lock_guard<std::mutex> guard(_adminLock);
        if (!_providers.Find(capability, entry)) {
            // No provider registered
            return JsonRpcErrorInvalidRequest();
        }

        // Create correlation for the consumer request
        CorrelationStore::ConsumerContext ctx;
        ctx.requestId = requestId;
        ctx.connectionId = connectionId;
        ctx.appId = appId;
        ctx.capability = capability;
        // Store correlation (we do not notify provider here; out-of-scope)
        static_cast<void>(_correlations.Create(ctx));
    }

    response.Clear();
    return Core::ERROR_NONE;
}

uint32_t App2AppProvider::handleProviderResponse(const Core::JSON::VariantContainer& params,
                                                 Core::JSON::VariantContainer& response) {
    string capability;
    string payloadJson;
    string correlationId;
    if (!ExtractPayloadCorrelation(params, capability, payloadJson, correlationId)) {
        return JsonRpcErrorInvalidParams();
    }

    CorrelationStore::ConsumerContext ctx;
    {
        std::lock_guard<std::mutex> guard(_adminLock);
        if (!_correlations.FindAndErase(correlationId, ctx)) {
            return JsonRpcErrorInvalidRequest();
        }
    }

    // Forward payload back to the consumer via AppGateway.respond
    auto rc = _appGateway ? _appGateway->Respond(ctx, payloadJson) : Core::ERROR_UNAVAILABLE;
    if (rc != Core::ERROR_NONE) {
        return JsonRpcErrorInvalidRequest();
    }
    response.Clear();
    return Core::ERROR_NONE;
}

uint32_t App2AppProvider::handleProviderError(const Core::JSON::VariantContainer& params,
                                              Core::JSON::VariantContainer& response) {
    string capability;
    string payloadJson;
    string correlationId;
    if (!ExtractPayloadCorrelation(params, capability, payloadJson, correlationId)) {
        return JsonRpcErrorInvalidParams();
    }

    CorrelationStore::ConsumerContext ctx;
    {
        std::lock_guard<std::mutex> guard(_adminLock);
        if (!_correlations.FindAndErase(correlationId, ctx)) {
            return JsonRpcErrorInvalidRequest();
        }
    }

    // Forward error payload back to the consumer via AppGateway.respond
    auto rc = _appGateway ? _appGateway->Respond(ctx, payloadJson) : Core::ERROR_UNAVAILABLE;
    if (rc != Core::ERROR_NONE) {
        return JsonRpcErrorInvalidRequest();
    }
    response.Clear();
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
