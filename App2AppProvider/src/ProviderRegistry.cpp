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

#include "ProviderRegistry.h"

namespace WPEFramework {
namespace Plugin {

uint32_t ProviderRegistry::Register(const string& capability,
                                  const string& appId,
                                  uint32_t connectionId) {
    Core::CriticalSection::Lock lock(_lock);

    _capabilityToProvider[capability] = ProviderEntry {
        appId,
        connectionId,
        Core::Time::Now()
    };

    _capabilitiesByConnection[connectionId].insert(capability);
    return Core::ERROR_NONE;
}

uint32_t ProviderRegistry::Unregister(const string& capability,
                                    uint32_t connectionId) {
    Core::CriticalSection::Lock lock(_lock);

    auto it = _capabilityToProvider.find(capability);
    if (it == _capabilityToProvider.end() || it->second.connectionId != connectionId) {
        return Core::ERROR_INVALID_REQUEST;
    }

    _capabilityToProvider.erase(it);
    auto connIt = _capabilitiesByConnection.find(connectionId);
    if (connIt != _capabilitiesByConnection.end()) {
        connIt->second.erase(capability);
        if (connIt->second.empty()) {
            _capabilitiesByConnection.erase(connIt);
        }
    }

    return Core::ERROR_NONE;
}

bool ProviderRegistry::Find(const string& capability,
                          ProviderEntry& out) const {
    Core::CriticalSection::Lock lock(_lock);

    auto it = _capabilityToProvider.find(capability);
    if (it == _capabilityToProvider.end()) {
        return false;
    }

    out = it->second;
    return true;
}

void ProviderRegistry::CleanupByConnection(uint32_t connectionId) {
    Core::CriticalSection::Lock lock(_lock);

    auto it = _capabilitiesByConnection.find(connectionId);
    if (it != _capabilitiesByConnection.end()) {
        for (const auto& capability : it->second) {
            _capabilityToProvider.erase(capability);
        }
        _capabilitiesByConnection.erase(it);
    }
}

} // namespace Plugin
} // namespace WPEFramework
