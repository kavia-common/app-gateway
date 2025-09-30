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

#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_App2AppProvider.h>
#include "ProviderRegistry.h"
#include "CorrelationStore.h"
#include "AppGatewayClient.h"

namespace WPEFramework {
namespace Plugin {

class App2AppProvider 
    : public PluginHost::IPlugin
    , public PluginHost::JSONRPC {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

    // IPlugin interface implementation
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    // Per-connection lifecycle
    void Attach(PluginHost::Channel& channel) override;
    void Detach(PluginHost::Channel& channel) override;

    BEGIN_INTERFACE_MAP(App2AppProvider)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

private:
    // JSON-RPC handlers
    uint32_t registerProvider(const JsonData::App2AppProvider::RegisterParamsData& params);
    uint32_t invokeProvider(const JsonData::App2AppProvider::InvokeParamsData& params);
    uint32_t handleProviderResponse(const JsonData::App2AppProvider::HandleResponseParamsData& params);
    uint32_t handleProviderError(const JsonData::App2AppProvider::HandleErrorParamsData& params);

private:
    PluginHost::IShell* _service;
    Core::CriticalSection _adminLock;
    ProviderRegistry _providers;
    CorrelationStore _correlations;
    std::unique_ptr<AppGatewayClient> _appGateway;
};

} // namespace Plugin
} // namespace WPEFramework
