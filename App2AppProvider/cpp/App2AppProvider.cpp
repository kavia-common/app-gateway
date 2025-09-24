#include "App2AppProvider.h"
#include <sstream>
#include <iomanip>

namespace WPEFramework {
namespace Plugin {

static inline uint64_t NowMs() {
    return static_cast<uint64_t>(Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond);
}

App2AppProvider::App2AppProvider()
    : _admin()
    , _service(nullptr)
    , _gatewaySink(nullptr)
    , _registry()
    , _capsByConnection()
    , _correlations()
    , _corrCounter{1}
{
}

App2AppProvider::~App2AppProvider() {
    // Make sure we cleanup properly if Deinitialize was not called
    Deinitialize(_service);
}

// PUBLIC_INTERFACE
const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    _admin.Lock();
    _service = service;
    _admin.Unlock();

    // Return empty string means success per IPlugin contract.
    return string();
}

// PUBLIC_INTERFACE
void App2AppProvider::Deinitialize(PluginHost::IShell* /* service */) {
    _admin.Lock();

    // Release gateway sink if set
    if (_gatewaySink != nullptr) {
        _gatewaySink->Release();
        _gatewaySink = nullptr;
    }

    // Release all provider references
    ReleaseAllProviders();

    _registry.clear();
    _capsByConnection.clear();
    _correlations.clear();

    _service = nullptr;

    _admin.Unlock();
}

// PUBLIC_INTERFACE
string App2AppProvider::Information() const {
    // Minimal metadata string
    return string(_T("App2AppProvider COM plugin: registers providers and routes app-to-app capability invocations"));
}

// PUBLIC_INTERFACE
void App2AppProvider::SetGatewaySink(Exchange::IAppGatewayResponses* sink) {
    _admin.Lock();

    if (_gatewaySink != nullptr) {
        _gatewaySink->Release();
        _gatewaySink = nullptr;
    }
    _gatewaySink = sink;
    if (_gatewaySink != nullptr) {
        _gatewaySink->AddRef();
    }

    _admin.Unlock();
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::Register(const std::string& capability,
                                   Exchange::IAppProvider* provider,
                                   const Exchange::RequestContext& context,
                                   bool& registered /* out */) {
    registered = false;

    if (provider == nullptr || capability.empty()) {
        return Core::ERROR_BAD_REQUEST;
    }

    _admin.Lock();

    // If capability already registered, replace it (single active provider per capability)
    auto it = _registry.find(capability);
    if (it != _registry.end()) {
        // Release previous provider reference if different
        if (it->second.provider != nullptr) {
            it->second.provider->Release();
        }
        _registry.erase(it);
    }

    ProviderEntry entry;
    entry.provider = provider;
    entry.appId = context.appId;
    entry.connectionId = context.connectionId; // uint32_t
    entry.registeredAtMs = NowMs();

    // Take a reference on the provider interface
    provider->AddRef();

    _registry.emplace(capability, entry);
    _capsByConnection[entry.connectionId].insert(capability);

    registered = true;

    _admin.Unlock();

    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::Unregister(const std::string& capability,
                                     Exchange::IAppProvider* provider,
                                     const Exchange::RequestContext& context,
                                     bool& registered /* out */) {
    registered = false;

    if (capability.empty()) {
        return Core::ERROR_BAD_REQUEST;
    }

    _admin.Lock();

    auto it = _registry.find(capability);
    if (it != _registry.end()) {
        // Only unregister if same connection and (if provided) same provider pointer
        bool sameConnection = (it->second.connectionId == context.connectionId);
        bool providerMatches = (provider == nullptr) || (it->second.provider == provider);

        if (sameConnection && providerMatches) {
            if (it->second.provider != nullptr) {
                it->second.provider->Release();
            }
            _registry.erase(it);
            auto setIt = _capsByConnection.find(context.connectionId);
            if (setIt != _capsByConnection.end()) {
                setIt->second.erase(capability);
                if (setIt->second.empty()) {
                    _capsByConnection.erase(setIt);
                }
            }
            registered = false;
        }
    }

    _admin.Unlock();

    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::Invoke(const Exchange::RequestContext& context,
                                 const std::string& capability,
                                 const Exchange::InvocationPayload& payload,
                                 std::string& correlationId /* out */) {

    if (capability.empty()) {
        return Core::ERROR_BAD_REQUEST;
    }

    _admin.Lock();

    auto it = _registry.find(capability);
    if (it == _registry.end() || it->second.provider == nullptr) {
        _admin.Unlock();
        return Core::ERROR_UNAVAILABLE;
    }

    // Create correlation context
    correlationId = MakeCorrelationId();

    ConsumerContext cc;
    cc.context = context;
    cc.capability = capability;
    cc.createdAtMs = NowMs();
    _correlations.emplace(correlationId, std::move(cc));

    // Build invocation request for the provider
    Exchange::InvocationRequest request;
    request.correlationId = correlationId;
    request.capability = capability;
    request.context = context;
    request.payload = payload;

    // Create a response sink for this invocation
    ProviderResponseSink* responseSink = Core::Service<ProviderResponseSink>::Create<Exchange::IAppProviderResponse>(*this, correlationId);
    // Ensure it has a ref for this call cycle
    responseSink->AddRef();

    Exchange::IAppProvider* provider = it->second.provider;
    provider->AddRef(); // keep stable during call

    _admin.Unlock();

    // Dispatch outside lock to avoid re-entrancy deadlocks
    const uint32_t result = provider->OnRequest(request, responseSink);

    // Release our local reference on the sink; provider is expected to hold a reference if needed
    responseSink->Release();
    // Release provider held temporarily
    provider->Release();

    if (result != Core::ERROR_NONE) {
        // Clean up correlation on dispatch failure
        _admin.Lock();
        _correlations.erase(correlationId);
        _admin.Unlock();
        return Core::ERROR_GENERAL;
    }

    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
void App2AppProvider::OnConnectionClosed(uint32_t connectionId) {
    _admin.Lock();
    ClearProvidersByConnection(connectionId);
    ClearCorrelationsByConnection(connectionId);
    _admin.Unlock();
}

std::string App2AppProvider::MakeCorrelationId() {
    const uint64_t seq = _corrCounter.fetch_add(1);
    const uint64_t now = NowMs();

    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << now
        << "-"
        << std::hex << std::setw(16) << std::setfill('0') << seq;
    return oss.str();
}

void App2AppProvider::DeliverSuccess(const std::string& correlationId,
                                     const Exchange::InvocationPayload& result) {
    Exchange::RequestContext ctx {};
    bool found = false;

    _admin.Lock();
    auto it = _correlations.find(correlationId);
    if (it != _correlations.end()) {
        ctx = it->second.context;
        _correlations.erase(it);
        found = true;
    }
    Exchange::IAppGatewayResponses* sink = _gatewaySink;
    if (sink != nullptr) {
        sink->AddRef(); // hold while unlocking
    }
    _admin.Unlock();

    if (!found) {
        // Unknown correlation; ignore
        if (sink != nullptr) {
            sink->Release();
        }
        return;
    }

    if (sink != nullptr) {
        const uint32_t rc = sink->Respond(ctx, &result, nullptr);
        (void)rc;
        sink->Release();
    }
}

void App2AppProvider::DeliverError(const std::string& correlationId,
                                   const Exchange::ProviderError& error) {
    Exchange::RequestContext ctx {};
    bool found = false;

    _admin.Lock();
    auto it = _correlations.find(correlationId);
    if (it != _correlations.end()) {
        ctx = it->second.context;
        _correlations.erase(it);
        found = true;
    }
    Exchange::IAppGatewayResponses* sink = _gatewaySink;
    if (sink != nullptr) {
        sink->AddRef(); // hold while unlocking
    }
    _admin.Unlock();

    if (!found) {
        if (sink != nullptr) {
            sink->Release();
        }
        return;
    }

    if (sink != nullptr) {
        const uint32_t rc = sink->Respond(ctx, nullptr, &error);
        (void)rc;
        sink->Release();
    }
}

void App2AppProvider::ClearProvidersByConnection(const uint32_t connectionId) {
    auto capsIt = _capsByConnection.find(connectionId);
    if (capsIt != _capsByConnection.end()) {
        for (const auto& cap : capsIt->second) {
            auto it = _registry.find(cap);
            if (it != _registry.end()) {
                if (it->second.provider != nullptr) {
                    it->second.provider->Release();
                }
                _registry.erase(it);
            }
        }
        _capsByConnection.erase(capsIt);
    }
}

void App2AppProvider::ClearCorrelationsByConnection(const uint32_t connectionId) {
    for (auto it = _correlations.begin(); it != _correlations.end(); ) {
        if (it->second.context.connectionId == connectionId) {
            it = _correlations.erase(it);
        } else {
            ++it;
        }
    }
}

void App2AppProvider::ReleaseAllProviders() {
    for (auto& kv : _registry) {
        if (kv.second.provider != nullptr) {
            kv.second.provider->Release();
            kv.second.provider = nullptr;
        }
    }
}

// ProviderResponseSink implementation

// PUBLIC_INTERFACE
uint32_t App2AppProvider::ProviderResponseSink::Success(const std::string& correlationId,
                                                        const std::string& /*capability*/,
                                                        const Exchange::InvocationPayload& result) {
    // Route via parent to gateway sink
    _parent.DeliverSuccess(correlationId.empty() ? _correlationId : correlationId, result);
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::ProviderResponseSink::Error(const std::string& correlationId,
                                                      const std::string& /*capability*/,
                                                      const Exchange::ProviderError& error) {
    // Route via parent to gateway sink
    _parent.DeliverError(correlationId.empty() ? _correlationId : correlationId, error);
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
