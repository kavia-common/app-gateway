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

class CorrelationStore {
public:
    struct ConsumerContext {
        uint32_t requestId;
        uint32_t connectionId;
        string appId;
        string capability;
        Core::Time createdAt;
    };

    CorrelationStore() = default;
    CorrelationStore(const CorrelationStore&) = delete;
    CorrelationStore& operator=(const CorrelationStore&) = delete;

    string Create(const ConsumerContext& ctx);
    bool FindAndErase(const string& correlationId,
                     ConsumerContext& out);
    void CleanupByConnection(uint32_t connectionId);

private:
    Core::CriticalSection _lock;
    std::unordered_map<string, ConsumerContext> _byCorrelationId;
    std::unordered_map<uint32_t, std::unordered_set<string>> _correlationsByConnection;
};

} // namespace Plugin
} // namespace WPEFramework
