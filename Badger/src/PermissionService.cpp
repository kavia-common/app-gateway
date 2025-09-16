#include "Badger/PermissionService.h"
#include <algorithm>

namespace WPEFramework {
namespace Plugin {

PermissionService::PermissionService(const Registry& reg, uint32_t cacheTtlSeconds)
    : _reg(reg), _ttl(cacheTtlSeconds) {
}

bool PermissionService::CheckCapability(const string& capability, const string& role) const {
    const string normRole = ToLower(role.empty() ? "use" : role);
    const auto ids = _reg.MapCapabilityToIds(capability, normRole);
    if (ids.empty()) return false;

    std::lock_guard<std::mutex> g(_lock);
    for (const auto& id : ids) {
        if (_granted.find(id) != _granted.end()) return true;
    }
    return false;
}

std::vector<std::tuple<string,string,bool>> PermissionService::CheckAll(
    const std::vector<std::pair<string,string>>& items) const {
    std::vector<std::tuple<string,string,bool>> out;
    out.reserve(items.size());
    for (const auto& it : items) {
        const string& cap = it.first;
        const string role = it.second.empty() ? "use" : it.second;
        out.emplace_back(cap, role, CheckCapability(cap, role));
    }
    return out;
}

std::vector<string> PermissionService::ListCapabilities() const {
    std::lock_guard<std::mutex> g(_lock);
    return _reg.CapabilitiesFromIds(_granted);
}

std::vector<string> PermissionService::ListFireboltPermissions() const {
    std::lock_guard<std::mutex> g(_lock);
    return _reg.FireboltPermissionsFromIds(_granted);
}

void PermissionService::SetGrantedIds(const std::unordered_set<string>& ids) {
    std::lock_guard<std::mutex> g(_lock);
    _granted = ids;
}

std::unordered_set<string> PermissionService::GetGrantedIds() const {
    std::lock_guard<std::mutex> g(_lock);
    return _granted;
}

string PermissionService::ToLower(const string& s) {
    string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c){ return std::tolower(c); });
    return out;
}

} // namespace Plugin
} // namespace WPEFramework
