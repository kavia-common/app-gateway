#pragma once

#include <com/IUnknown.h>
#include <core/Time.h>
#include <core/JSON.h>
#include <plugins/IShell.h>
#include <interfaces/json/Module.h>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <string>

#include "IApp2AppProvider.h"
#include "IAppGateway.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * Concrete implementation of Exchange::IApp2AppProvider business logic.
 * Maintains:
 *  - Provider registry: capability -> ProviderEntry
 *  - Reverse index: connectionId -> {capability}
 *  - Correlations: correlationId -> ConsumerContext
 * Interacts with AppGateway over COMRPC via Exchange::IAppGateway.
 */
class App2AppProviderImplementation : public Exchange::IApp2AppProvider {
public:
    explicit App2AppProviderImplementation(PluginHost::IShell* service);
    ~App2AppProviderImplementation() override;

    // IUnknown
    uint32_t AddRef() const override;
    uint32_t Release() const override;
    void* QueryInterface(const uint32_t id) override;

    // Exchange::IApp2AppProvider
    // PUBLIC_INTERFACE
    Core::hresult RegisterProvider(const Context& context,
                                   bool reg,
                                   const string& capability,
                                   Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult InvokeProvider(const Context& context,
                                 const string& capability,
                                 Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderResponse(const string& payload,
                                         const string& capability,
                                         Error& error) override;

    // PUBLIC_INTERFACE
    Core::hresult HandleProviderError(const string& payload,
                                      const string& capability,
                                      Error& error) override;

    // PUBLIC_INTERFACE
    /**
     * Remove any providers and pending correlations bound to a connection.
     */
    void CleanupByConnection(uint32_t connectionId);

private:
    struct ProviderEntry {
        std::string appId;
        uint32_t connectionId { 0 };
        Core::Time registeredAt;
    };

    struct ConsumerContext {
        int requestId { 0 };
        uint32_t connectionId { 0 };
        std::string appId;
        std::string capability;
        Core::Time createdAt;
    };

private:
    bool ParseConnectionId(const std::string& in, uint32_t& outConnId) const;
    std::string GenerateCorrelationId(); // NowTicks + counter as per backup impl
    bool EnsureGateway();

    Core::hresult HandleProviderResultLike_(const std::string& payload,
                                            const std::string& capability,
                                            bool isError,
                                            Error& error);

private:
    mutable std::atomic<uint32_t> _refCount {1};
    PluginHost::IShell* _service { nullptr };
    Exchange::IAppGateway* _gateway { nullptr }; // held COM pointer

    // Registry/correlation state
    mutable std::mutex _lock;
    std::unordered_map<std::string, ProviderEntry> _capabilityToProvider;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> _capabilitiesByConnection;
    std::unordered_map<std::string, ConsumerContext> _correlations;

    // Correlation ID counter (for ticks-counter pattern)
    std::atomic<uint64_t> _corrCounter {0};
};

} // namespace Plugin
} // namespace WPEFramework
