#include "AppGatewayClient.h"
#include <core/ProxyType.h>

using namespace WPEFramework::Plugin;

uint32_t AppGatewayClient::Respond(const CorrelationStore::ConsumerContext& ctx, const std::string& payloadJson) {
    if (_service == nullptr) {
        return Core::ERROR_UNAVAILABLE;
    }
    Core::ProxyType<PluginHost::IDispatcher> dispatcher(
        _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_T("AppGateway")));

    if (dispatcher.IsValid() == false) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Build parameters JSON string per appgateway-technical-design
    // {
    //   "context": { "requestId": <>, "connectionId": "<>", "appId": "<>" },
    //   "payload": <opaque JSON>
    // }
    string params;
    params.reserve(256 + payloadJson.size());
    params += "{";
    params += "\"context\":{";
    params += "\"requestId\":" + std::to_string(ctx.requestId) + ",";
    params += "\"connectionId\":\"" + std::to_string(ctx.connectionId) + "\",";
    params += "\"appId\":\"" + ctx.appId + "\"";
    params += "},";
    params += "\"payload\":" + payloadJson;
    params += "}";

    string result; // ignored, we only need hresult
    return dispatcher->Invoke(0 /*channelId*/, 0 /*id*/, Core::SystemInfo::Instance().ProcessId(),
                              _T("org.rdk.AppGateway.respond"), params, result);
}
