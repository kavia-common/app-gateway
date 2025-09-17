#include "CorrelationStore.h"
#include <random>
#include <sstream>
#include <iomanip>

using namespace WPEFramework::Plugin;

std::string CorrelationStore::GenerateUUID() const {
    // Generate a simple UUID v4-like string for correlation purposes
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);

    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << static_cast<uint32_t>(part1 >> 32) << "-"
       << std::setw(4) << static_cast<uint16_t>(part1 >> 16) << "-"
       << std::setw(4) << static_cast<uint16_t>(part1) << "-"
       << std::setw(4) << static_cast<uint16_t>(part2 >> 48) << "-"
       << std::setw(12) << (part2 & 0x0000FFFFFFFFFFFFULL);
    return ss.str();
}

std::string CorrelationStore::Create(const ConsumerContext& ctx) {
    std::lock_guard<std::mutex> lock(_lock);
    std::string id = GenerateUUID();
    _byCorrelationId.emplace(id, ctx);
    return id;
}

bool CorrelationStore::FindAndErase(const std::string& correlationId, ConsumerContext& out) {
    std::lock_guard<std::mutex> lock(_lock);
    auto it = _byCorrelationId.find(correlationId);
    if (it == _byCorrelationId.end()) {
        return false;
    }
    out = it->second;
    _byCorrelationId.erase(it);
    return true;
}

void CorrelationStore::CleanupByConnection(uint32_t connectionId) {
    std::lock_guard<std::mutex> lock(_lock);
    for (auto it = _byCorrelationId.begin(); it != _byCorrelationId.end();) {
        if (it->second.connectionId == connectionId) {
            it = _byCorrelationId.erase(it);
        } else {
            ++it;
        }
    }
}
