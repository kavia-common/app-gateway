#pragma once

// Interfaces for App2AppProvider per app2appprovider-technical-design.md.
// This header defines COM interfaces and data structures used by the App2AppProvider Thunder plugin.
//
// Important notes:
// - connectionId is a numeric identifier of type uint32_t everywhere.
// - The payload is modeled as an opaque UTF-8 string; projects may adapt this to use IBuffer if needed.
// - Interface IDs are allocated in a group relative to RPC::IDS::ID_EXTERNAL_INTERFACE_OFFSET to avoid collisions.
//   These IDs should be treated as fixed once published.

#include <string>
#include <vector>
#include <cstdint>

#include <interfaces/Module.h> // Brings in WPEFramework::Exchange and RPC::IDS

namespace WPEFramework {
namespace Exchange {

// Fixed base for this group of interfaces; ensure uniqueness if upstream Ids.h is updated later.
static constexpr uint32_t ID_APP2APP_PROVIDER_BASE = (RPC::IDS::ID_EXTERNAL_INTERFACE_OFFSET + 0x4F0);

struct RequestContext {
    // Numeric IDs per design (uint32_t).
    uint32_t requestId {};
    uint32_t connectionId {};
    std::string appId {};
};

struct InvocationPayload {
    // Opaque payload as UTF-8 string.
    std::string utf8 {};
};

struct InvocationRequest {
    std::string correlationId {};
    std::string capability {};
    RequestContext context {};
    InvocationPayload payload {};
};

struct ProviderError {
    int32_t code {0};
    std::string message {};
};

// PUBLIC_INTERFACE
struct IAppProviderResponse : virtual public Core::IUnknown {
    /** Sink interface used by providers to respond to an invocation asynchronously. */
    enum { ID = ID_APP2APP_PROVIDER_BASE + 0 };

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Success(const std::string& correlationId,
                                                 const std::string& capability,
                                                 const InvocationPayload& result) = 0;

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Error(const std::string& correlationId,
                                               const std::string& capability,
                                               const ProviderError& error) = 0;
};

// PUBLIC_INTERFACE
struct IAppProvider : virtual public Core::IUnknown {
    /** Interface a provider implements to receive capability requests. */
    enum { ID = ID_APP2APP_PROVIDER_BASE + 1 };

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t OnRequest(const InvocationRequest& request,
                                                   IAppProviderResponse* sink) = 0;
};

// PUBLIC_INTERFACE
struct IAppGatewayResponses : virtual public Core::IUnknown {
    /** Interface the AppGateway implements so App2AppProvider can deliver final outcomes. */
    enum { ID = ID_APP2APP_PROVIDER_BASE + 2 };

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Respond(const RequestContext& context,
                                                 const InvocationPayload* result /* nullable */,
                                                 const ProviderError* error /* nullable */) = 0;
};

// PUBLIC_INTERFACE
struct IApp2AppProvider : virtual public Core::IUnknown {
    /** The primary App2AppProvider COM interface. */
    enum { ID = ID_APP2APP_PROVIDER_BASE + 3 };

    // PUBLIC_INTERFACE
    virtual void SetGatewaySink(IAppGatewayResponses* sink) = 0;

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Register(const std::string& capability,
                                                  IAppProvider* provider,
                                                  const RequestContext& context,
                                                  bool& registered /* out */) = 0;

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Unregister(const std::string& capability,
                                                    IAppProvider* provider,
                                                    const RequestContext& context,
                                                    bool& registered /* out */) = 0;

    // PUBLIC_INTERFACE
    virtual /* Core::ERROR_* */ uint32_t Invoke(const RequestContext& context,
                                                const std::string& capability,
                                                const InvocationPayload& payload,
                                                std::string& correlationId /* out */) = 0;

    // PUBLIC_INTERFACE
    virtual void OnConnectionClosed(uint32_t connectionId) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
