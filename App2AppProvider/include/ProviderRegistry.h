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
#include <unordered_map>
#include <unordered_set>

namespace WPEFramework {
namespace Plugin {

class ProviderRegistry {
public:
    struct ProviderEntry {
        string appId;
        uint32_t connectionId;
        Core::Time registeredAt;
    };

    ProviderRegistry() = default;
    ProviderRegistry(const ProviderRegistry&) = delete;
    ProviderRegistry& operator=(const ProviderRegistry&) = delete;

    uint32_t Register(const string& capability,
                     const string& appId,
                     uint32_t connectionId);
    uint32_t Unregister(const string& capability,
                       uint32_t connectionId);
    bool Find(const string& capability,
             ProviderEntry& out) const;
    void CleanupByConnection(uint32_t connectionId);

private:
    Core::CriticalSection _lock;
    std::unordered_map<string, ProviderEntry> _capabilityToProvider;
    std::unordered_map<uint32_t, std::unordered_set<string>> _capabilitiesByConnection;
};

} // namespace Plugin
} // namespace WPEFramework
