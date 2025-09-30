/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "App2AppProvider.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    static constexpr const char* MODULE_NAME = "App2AppProvider";
}

// PUBLIC_INTERFACE
App2AppProvider::App2AppProvider()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _adminLock()
    , _providers()
    , _correlations()
    , _appGateway(nullptr) {

    Register<JsonData::App2AppProvider::RegisterParamsData, void>(_T("registerProvider"), &App2AppProvider::registerProvider, this);
    Register<JsonData::App2AppProvider::InvokeParamsData, void>(_T("invokeProvider"), &App2AppProvider::invokeProvider, this);
    Register<JsonData::App2AppProvider::HandleResponseParamsData, void>(_T("handleProviderResponse"), &App2AppProvider::handleProviderResponse, this);
    Register<JsonData::App2AppProvider::HandleErrorParamsData, void>(_T("handleProviderError"), &App2AppProvider::handleProviderError, this);
}

App2AppProvider::~App2AppProvider() {
    // Make sure all handlers are unregistered
    Unregister(_T("registerProvider"));
    Unregister(_T("invokeProvider"));
    Unregister(_T("handleProviderResponse"));
    Unregister(_T("handleProviderError"));
}

// PUBLIC_INTERFACE
const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    ASSERT(_service == nullptr);
    ASSERT(service != nullptr);

    _service = service;
    _appGateway = std::make_unique<AppGatewayClient>(service);

    return string();
}

// PUBLIC_INTERFACE
void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
    ASSERT(_service == service);

    Core::CriticalSection::Lock lock(_adminLock);
    _appGateway.reset();
    _service = nullptr;
}

// PUBLIC_INTERFACE
string App2AppProvider::Information() const {
    return string();
}

// PUBLIC_INTERFACE
void App2AppProvider::Attach(PluginHost::Channel& channel) {
    // Minimal per-connection setup if needed
}

// PUBLIC_INTERFACE
void App2AppProvider::Detach(PluginHost::Channel& channel) {
    const uint32_t channelId = channel.Id();
    _providers.CleanupByConnection(channelId);
    _correlations.CleanupByConnection(channelId);
}

// JSON-RPC handlers
uint32_t App2AppProvider::registerProvider(const JsonData::App2AppProvider::RegisterParamsData& params) {
    // Extract parameters
    const string& appId = params.Context.AppId.Value();
    const uint32_t connectionId = params.Context.ConnectionId.Value();
    const string& capability = params.Capability.Value();
    const bool shouldRegister = params.Register.Value();

    if (appId.empty() || capability.empty()) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    uint32_t rc;
    if (shouldRegister) {
        rc = _providers.Register(capability, appId, connectionId);
    } else {
        rc = _providers.Unregister(capability, connectionId);
    }

    return rc;
}

uint32_t App2AppProvider::invokeProvider(const JsonData::App2AppProvider::InvokeParamsData& params) {
    // Extract parameters
    const uint32_t requestId = params.Context.RequestId.Value();
    const uint32_t connectionId = params.Context.ConnectionId.Value();
    const string& appId = params.Context.AppId.Value();
    const string& capability = params.Capability.Value();

    if (appId.empty() || capability.empty()) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    // Verify provider exists
    ProviderRegistry::ProviderEntry provider;
    if (!_providers.Find(capability, provider)) {
        return Core::ERROR_INVALID_REQUEST;
    }

    // Create correlation for later response routing
    CorrelationStore::ConsumerContext ctx {
        requestId,
        connectionId,
        appId,
        capability,
        Core::Time::Now()
    };

    const string correlationId = _correlations.Create(ctx);
    return Core::ERROR_NONE;
}

uint32_t App2AppProvider::handleProviderResponse(const JsonData::App2AppProvider::HandleResponseParamsData& params) {
    const string& payload = params.Payload.Value();
    const string& capability = params.Capability.Value();

    // Parse payload to extract correlationId and result
    JsonObject payloadObj;
    if (!payloadObj.FromString(payload)) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    const string correlationId = payloadObj["correlationId"].String();
    if (correlationId.empty()) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    // Find and remove correlation entry
    CorrelationStore::ConsumerContext consumer;
    if (!_correlations.FindAndErase(correlationId, consumer)) {
        return Core::ERROR_INVALID_REQUEST;
    }

    // Forward response to consumer via AppGateway
    const uint32_t rc = _appGateway->Respond(consumer, payload);
    return rc;
}

uint32_t App2AppProvider::handleProviderError(const JsonData::App2AppProvider::HandleErrorParamsData& params) {
    const string& payload = params.Payload.Value();
    const string& capability = params.Capability.Value();

    // Parse payload to extract correlationId and error
    JsonObject payloadObj;
    if (!payloadObj.FromString(payload)) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    const string correlationId = payloadObj["correlationId"].String();
    if (correlationId.empty()) {
        return Core::ERROR_INVALID_PARAMETERS;
    }

    // Find and remove correlation entry
    CorrelationStore::ConsumerContext consumer;
    if (!_correlations.FindAndErase(correlationId, consumer)) {
        return Core::ERROR_INVALID_REQUEST;
    }

    // Forward error to consumer via AppGateway
    const uint32_t rc = _appGateway->Respond(consumer, payload);
    return rc;
}

} // namespace Plugin
} // namespace WPEFramework
