#include "AppGatewayClient.h"
#include <core/ProxyType.h>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

namespace {
    // Helper to build a JSON string for a Core::JSON::VariantContainer
    static string ToJSONString(Core::JSON::VariantContainer& obj) {
        string out;
        obj.ToString(out);
        return out;
    }
}

uint32_t AppGatewayClient::Respond(const CorrelationStore::ConsumerContext& ctx, const std::string& payloadJson) {
    if (_service == nullptr) {
        return Core::ERROR_UNAVAILABLE;
    }

    Core::ProxyType<PluginHost::IDispatcher> dispatcher(
        _service->QueryInterfaceByCallsign<PluginHost::IDispatcher>(_T("AppGateway")));

    if (dispatcher.IsValid() == false) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Build parameters JSON using VariantContainer to avoid malformed JSON
    Core::JSON::VariantContainer paramsObj;

    // context object
    Core::JSON::VariantContainer contextObj;
    contextObj["requestId"] = static_cast<uint32_t>(ctx.requestId);
    // The interface examples show connectionId as string "guid". We use the numeric channel id serialized as string.
    contextObj["connectionId"] = Core::JSON::String(std::to_string(ctx.connectionId));
    contextObj["appId"] = Core::JSON::String(ctx.appId);

    paramsObj["context"] = contextObj;

    // payload is an opaque JSON object. Parse it into a VariantContainer if possible;
    // If parsing fails, pass as string so AppGateway can decide handling.
    Core::JSON::VariantContainer payloadObj;
    bool parsed = false;
    if (!payloadJson.empty()) {
        parsed = payloadObj.FromString(payloadJson);
    }
    if (parsed) {
        paramsObj["payload"] = payloadObj;
    } else {
        // Fallback to raw string; AppGateway.respond may reject with PARSE_ERROR
        paramsObj["payload"] = Core::JSON::String(payloadJson);
    }

    string paramsStr = ToJSONString(paramsObj);
    string result; // body ignored; we check return code
    // Call AppGateway method on its dispatcher: method name should be local ("respond"), empty token
    return dispatcher->Invoke(0 /*channelId*/, 0 /*id*/, string() /*token*/,
                              _T("respond"), paramsStr, result);
}
