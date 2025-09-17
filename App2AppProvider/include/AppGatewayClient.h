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
 */
class AppGatewayClient {
public:
    explicit AppGatewayClient(PluginHost::IShell* service) : _service(service) {}

    // PUBLIC_INTERFACE
    uint32_t Respond(const CorrelationStore::ConsumerContext& ctx, const std::string& payloadJson);

private:
    PluginHost::IShell* _service { nullptr };
};

} // namespace Plugin
} // namespace WPEFramework
