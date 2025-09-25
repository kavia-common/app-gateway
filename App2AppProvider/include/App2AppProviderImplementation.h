#pragma once

// Thunder / WPEFramework
#include <plugins/IShell.h>
#include <core/Time.h>

// STL
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Project interfaces and logging
#include "IApp2AppProvider.h"
#include "IAppGateway.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * App2AppProviderImplementation is the concrete business-logic implementation of Exchange::IApp2AppProvider.
 * It maintains provider registry and consumer async contexts keyed by connectionId (uint32_t),
 * and communicates responses back to AppGateway via COMRPC.
 */
class App2AppProviderImplementation final : public Exchange::IApp2AppProvider {
public:
    App2AppProviderImplementation() = delete;
    explicit App2AppProviderImplementation(PluginHost::IShell* service);
    ~App2AppProviderImplementation() override;

    // Core::IUnknown
    uint32_t AddRef() const override;
    uint32_t Release() const override;
    void* QueryInterface(const uint32_t id) override;

public:
    // PUBLIC_INTERFACE
    /**
     * Register or unregister a provider for a capability.
     * Context.connectionId must be a numeric string; it is parsed to uint32_t.
     */
    Core::hresult RegisterProvider(const Context& context /* @in */,
                                   bool reg /* @in */,
                                   const string& capability /* @in */,
                                   Error& error /* @out */) override;

    // PUBLIC_INTERFACE
    /**
     * Store consumer async context keyed by connectionId and validate provider existence.
     * AppGateway is responsible for the routing to the provider.
     */
    Core::hresult InvokeProvider(const Context& context /* @in */,
                                 const string& capability /* @in */,
                                 Error& error /* @out */) override;

    // PUBLIC_INTERFACE
    /**
     * Handle a provider response. The payload is opaque JSON; this implementation extracts
     * the connectionId by parsing the payload (looks for "contextEcho.connectionId",
     * "context.connectionId", or top-level "connectionId").
     * The stored ConsumerContext is looked up by the parsed connectionId and forwarded back
     * to AppGateway via IAppGateway::Respond.
     */
    Core::hresult HandleProviderResponse(const string& payload /* @in @opaque */,
                                         const string& capability /* @in */,
                                         Error& error /* @out */) override;

    // PUBLIC_INTERFACE
    /**
     * Handle a provider error; semantics identical to HandleProviderResponse, with payload
     * indicating error details for the consumer.
     */
    Core::hresult HandleProviderError(const string& payload /* @in @opaque */,
                                      const string& capability /* @in */,
                                      Error& error /* @out */) override;

    // PUBLIC_INTERFACE
    /**
     * Cleanup any provider registrations and pending consumer contexts associated with this connection.
     */
    void CleanupByConnection(const uint32_t connectionId);

private:
    // Helpers

    /**
     * Parse a numeric connectionId from string form (decimal). Returns true on success.
     */
    bool ParseConnectionId(const std::string& in, uint32_t& out) const;

    /**
     * Attempt to extract a connectionId (string) from an opaque JSON payload and parse it to uint32_t.
     * Search order:
     *  - payload.contextEcho.connectionId (string)
     *  - payload.context.connectionId (string)
     *  - payload.connectionId (string)
     * Returns true on success.
     */
    bool ExtractConnectionIdFromPayload(const std::string& payload, uint32_t& outConnectionId) const;

private:
    // Provider registry model
    struct ProviderEntry {
        std::string appId;
        uint32_t connectionId { 0 };
        Core::Time registeredAt;
    };

    // Pending consumer context model
    struct ConsumerContext {
        uint32_t requestId { 0 };
        uint32_t connectionId { 0 };
        std::string appId;
        std::string capability;
        Core::Time createdAt;
    };

private:
    mutable uint32_t _refCount { 1 };
    PluginHost::IShell* _service { nullptr };
    Exchange::IAppGateway* _gateway { nullptr };

    mutable std::mutex _lock;
    std::unordered_map<std::string, ProviderEntry> _capabilityToProvider;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> _capabilitiesByConnection;
    std::unordered_map<uint32_t, ConsumerContext> _contextsByConnection;
};

} // namespace Plugin
} // namespace WPEFramework
