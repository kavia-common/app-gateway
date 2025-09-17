#pragma once

#include <plugins/IDispatcher.h>
#include <plugins/Plugin.h>
#include <core/JSON.h>
#include <string>
#include "CorrelationStore.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * AppGatewayClient forwards responses to applications via org.rdk.AppGateway.respond.
 *
 * Build a proper parameters JSON object:
 * {
 *   "context": { "requestId": <uint32>, "connectionId": "<uint32-as-string>", "appId": "<appId>" },
 *   "payload": <opaque JSON forwarded from provider>
 * }
 */
class AppGatewayClient {
public:
    explicit AppGatewayClient(PluginHost::IShell* service) : _service(service) {}

    // PUBLIC_INTERFACE
    /**
     * Forward a payload (result or error) to the originating consumer using AppGateway.respond.
     * Returns Core::ERROR_*.
     */
    uint32_t Respond(const CorrelationStore::ConsumerContext& ctx, const std::string& payloadJson);

private:
    PluginHost::IShell* _service { nullptr };
};

} // namespace Plugin
} // namespace WPEFramework
