#pragma once

#include <core/JSON.h>
#include <plugins/JSONRPC.h>
#include <plugins/IDispatcher.h>
#include <plugins/Plugin.h>
#include <mutex>
#include <memory>
#include <string>

#include "ProviderRegistry.h"
#include "CorrelationStore.h"
#include "AppGatewayClient.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * App2AppProvider Thunder plugin implementing provider pattern orchestration:
 * - register/unregister provider capabilities
 * - invoke provider for a consumer (create correlationId and track consumer context)
 * - accept provider responses/errors and route them back via AppGateway.respond
 *
 * Methods:
 *  - org.rdk.ApptoAppProvider.registerProvider
 *  - org.rdk.ApptoAppProvider.invokeProvider
 *  - org.rdk.ApptoAppProvider.handleProviderResponse
 *  - org.rdk.ApptoAppProvider.handleProviderError
 */
class App2AppProvider : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

    // IPlugin lifecycle
    // PUBLIC_INTERFACE
    const string Initialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    void Deinitialize(PluginHost::IShell* service) override;

    // PUBLIC_INTERFACE
    string Information() const override;

    // Connection lifecycle hooks
    void Attach(PluginHost::Channel& channel) override;
    void Detach(PluginHost::Channel& channel) override;

private:
    // JSON-RPC method handlers
    uint32_t registerProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);
    uint32_t invokeProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);
    uint32_t handleProviderResponse(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);
    uint32_t handleProviderError(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);

    // Helpers
    static bool ExtractContext(const Core::JSON::VariantContainer& in,
                               uint32_t& requestId,
                               uint32_t& connectionId,
                               string& appId,
                               string* capability /*optional*/);

    static bool ExtractPayloadCorrelation(const Core::JSON::VariantContainer& in,
                                          string& capability,
                                          string& payloadJson,
                                          string& correlationId);

    static uint32_t JsonRpcErrorInvalidParams();
    static uint32_t JsonRpcErrorInvalidRequest();

private:
    PluginHost::IShell* _service { nullptr };
    std::mutex _adminLock;
    ProviderRegistry _providers;
    CorrelationStore _correlations;
    std::unique_ptr<AppGatewayClient> _appGateway;
};

} // namespace Plugin
} // namespace WPEFramework
