#pragma once

#include <unordered_map>
#include <mutex>
#include <string>
#include <cstdint>
#include <core/Time.h>

namespace WPEFramework {
namespace Plugin {

/**
 * PUBLIC_INTERFACE
 * CorrelationStore tracks consumer requests awaiting provider responses.
 */
class CorrelationStore {
public:
    struct ConsumerContext {
        uint32_t requestId { 0 };
        uint32_t connectionId { 0 };
        std::string appId;
        std::string capability;
        Core::Time createdAt;
    };

    CorrelationStore() = default;

    // PUBLIC_INTERFACE
    std::string Create(const ConsumerContext& ctx);

    // PUBLIC_INTERFACE
    bool FindAndErase(const std::string& correlationId, ConsumerContext& out);

    // PUBLIC_INTERFACE
    void CleanupByConnection(uint32_t connectionId);

private:
    std::string GenerateUUID() const;

private:
    mutable std::mutex _lock;
    std::unordered_map<std::string, ConsumerContext> _byCorrelationId;
};

} // namespace Plugin
} // namespace WPEFramework
