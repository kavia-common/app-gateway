#pragma once

// App2AppProvider Thunder plugin declaration.
// Implements PluginHost::IPlugin and Exchange::IApp2AppProvider.

#include "Module.h"
#include <unordered_map>
#include <map>
#include <set>
#include <atomic>

namespace WPEFramework {
namespace Plugin {

class App2AppProvider final : public PluginHost::IPlugin, public Exchange::IApp2AppProvider {
private:
    struct ProviderEntry {
        Exchange::IAppProvider* provider {nullptr};
        std::string appId {};
        uint32_t connectionId {0};
        uint64_t registeredAtMs {0};
    };

    class ProviderResponseSink final : public Core::Sink<Exchange::IAppProviderResponse> {
    public:
        ProviderResponseSink() = delete;
        ProviderResponseSink(const ProviderResponseSink&) = delete;
        ProviderResponseSink& operator=(const ProviderResponseSink&) = delete;

        ProviderResponseSink(App2AppProvider& parent, const std::string& correlationId)
            : _parent(parent)
            , _correlationId(correlationId)
        {
        }

        ~ProviderResponseSink() override = default;

        // PUBLIC_INTERFACE
        uint32_t Success(const std::string& correlationId,
                         const std::string& capability,
                         const Exchange::InvocationPayload& result) override;

        // PUBLIC_INTERFACE
        uint32_t Error(const std::string& correlationId,
                       const std::string& capability,
                       const Exchange::ProviderError& error) override;

        BEGIN_INTERFACE_MAP(ProviderResponseSink)
            INTERFACE_ENTRY(Exchange::IAppProviderResponse)
        END_INTERFACE_MAP

    private:
        App2AppProvider& _parent;
        const std::string _correlationId;
    };

public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

public:
    // PluginHost::IPlugin
    // PUBLIC_INTERFACE
    const string Initialize(PluginHost::IShell* service) override;
    // PUBLIC_INTERFACE
    void Deinitialize(PluginHost::IShell* service) override;
    // PUBLIC_INTERFACE
    string Information() const override;

    // Exchange::IApp2AppProvider
    // PUBLIC_INTERFACE
    void SetGatewaySink(Exchange::IAppGatewayResponses* sink) override;
    // PUBLIC_INTERFACE
    uint32_t Register(const std::string& capability,
                      Exchange::IAppProvider* provider,
                      const Exchange::RequestContext& context,
                      bool& registered /* out */) override;
    // PUBLIC_INTERFACE
    uint32_t Unregister(const std::string& capability,
                        Exchange::IAppProvider* provider,
                        const Exchange::RequestContext& context,
                        bool& registered /* out */) override;
    // PUBLIC_INTERFACE
    uint32_t Invoke(const Exchange::RequestContext& context,
                    const std::string& capability,
                    const Exchange::InvocationPayload& payload,
                    std::string& correlationId /* out */) override;
    // PUBLIC_INTERFACE
    void OnConnectionClosed(uint32_t connectionId) override;

    BEGIN_INTERFACE_MAP(App2AppProvider)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(Exchange::IApp2AppProvider)
    END_INTERFACE_MAP

private:
    // Correlation bookkeeping and routing
    struct ConsumerContext {
        Exchange::RequestContext context {};
        std::string capability {};
        uint64_t createdAtMs {0};
    };

    std::string MakeCorrelationId();
    void DeliverSuccess(const std::string& correlationId, const Exchange::InvocationPayload& result);
    void DeliverError(const std::string& correlationId, const Exchange::ProviderError& error);

    void ClearProvidersByConnection(const uint32_t connectionId);
    void ClearCorrelationsByConnection(const uint32_t connectionId);
    void ReleaseAllProviders();

private:
    Core::CriticalSection _admin; // protects all below

    PluginHost::IShell* _service {nullptr}; // non-refcounted per IPlugin contract
    Exchange::IAppGatewayResponses* _gatewaySink {nullptr}; // refcounted

    std::unordered_map<std::string, ProviderEntry> _registry; // capability -> provider
    std::unordered_map<uint32_t, std::set<std::string>> _capsByConnection;

    std::unordered_map<std::string, ConsumerContext> _correlations; // correlationId -> consumer
    std::atomic<uint64_t> _corrCounter;
};

} // namespace Plugin
} // namespace WPEFramework

// Register plugin as a service with version 1.0.0
SERVICE_REGISTRATION(WPEFramework::Plugin::App2AppProvider, 1, 0, 0)
