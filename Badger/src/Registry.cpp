#include "Badger/Registry.h"
#include <fstream>
#include <sstream>
#include <set>

namespace WPEFramework {
namespace Plugin {

std::unique_ptr<Registry> Registry::LoadFromFile(const string& path, string& errorOut) {
    std::ifstream in(path);
    if (!in.good()) {
        errorOut = "Failed to open registry file: " + path;
        return nullptr;
    }
    auto reg = std::unique_ptr<Registry>(new Registry());
    // Extremely simple, line-based parser tailored to example YAML in repo attachments.
    // Supports keys: id, capabilities: use/manage/provide lists, apis list.
    RegistryEntry current;
    bool inPermissionsList = false;
    bool inCapabilities = false;
    bool inApis = false;
    string role; // "use", "manage", "provide"

    string line;
    while (std::getline(in, line)) {
        const string t = trim(line);
        if (t.empty()) continue;
        if (starts_with(t, "permissions:")) {
            inPermissionsList = true;
            continue;
        }
        if (inPermissionsList && starts_with(t, "- id:")) {
            // If we already have an entry, push it.
            if (!current.id.empty()) {
                reg->_entries.push_back(current);
                current = RegistryEntry{};
            }
            current.id = trim(t.substr(std::string("- id:").size()));
            // Strip quotes if present
            if (!current.id.empty() && (current.id.front()=='"' || current.id.front()=='\'')) {
                current.id = current.id.substr(1, current.id.size()-2);
            }
            continue;
        }
        if (starts_with(t, "capabilities:")) {
            inCapabilities = true;
            inApis = false;
            continue;
        }
        if (inCapabilities && (starts_with(t, "use:") || starts_with(t, "manage:") || starts_with(t, "provide:"))) {
            if (starts_with(t, "use:")) role = "use";
            else if (starts_with(t, "manage:")) role = "manage";
            else role = "provide";
            continue;
        }
        if (starts_with(t, "apis:")) {
            inApis = true;
            inCapabilities = false;
            continue;
        }
        // list items
        if (starts_with(t, "- ")) {
            string value = trim(t.substr(2));
            if (!value.empty() && (value.front()=='"' || value.front()=='\'')) {
                value = value.substr(1, value.size()-2);
            }
            if (inCapabilities) {
                if (role == "use") current.useCaps.push_back(value);
                else if (role == "manage") current.manageCaps.push_back(value);
                else if (role == "provide") current.provideCaps.push_back(value);
            } else if (inApis) {
                current.apis.push_back(value);
            }
            continue;
        }
    }
    if (!current.id.empty()) {
        reg->_entries.push_back(current);
    }
    if (reg->_entries.empty()) {
        errorOut = "Registry parsed but no entries found.";
    }
    return reg;
}

std::vector<string> Registry::MapCapabilityToIds(const string& capability, const string& role) const {
    std::vector<string> out;
    const string want = capability;
    for (const auto& e : _entries) {
        if (role == "use") {
            if (std::find(e.useCaps.begin(), e.useCaps.end(), want) != e.useCaps.end()) out.push_back(e.id);
        } else if (role == "manage") {
            if (std::find(e.manageCaps.begin(), e.manageCaps.end(), want) != e.manageCaps.end()) out.push_back(e.id);
        } else { // provide
            if (std::find(e.provideCaps.begin(), e.provideCaps.end(), want) != e.provideCaps.end()) out.push_back(e.id);
        }
    }
    return out;
}

std::vector<string> Registry::MapApiToIds(const string& api) const {
    std::vector<string> out;
    for (const auto& e : _entries) {
        if (std::find(e.apis.begin(), e.apis.end(), api) != e.apis.end()) {
            out.push_back(e.id);
        }
    }
    return out;
}

std::vector<string> Registry::AllCapabilities() const {
    std::set<string> caps;
    for (const auto& e : _entries) {
        for (const auto& c : e.useCaps) caps.insert(c);
        for (const auto& c : e.manageCaps) caps.insert(c);
        for (const auto& c : e.provideCaps) caps.insert(c);
    }
    return std::vector<string>(caps.begin(), caps.end());
}

std::vector<string> Registry::CapabilitiesFromIds(const std::unordered_set<string>& ids) const {
    std::set<string> caps;
    for (const auto& e : _entries) {
        if (ids.find(e.id) != ids.end()) {
            for (const auto& c : e.useCaps) caps.insert(c);
            for (const auto& c : e.manageCaps) caps.insert(c);
            for (const auto& c : e.provideCaps) caps.insert(c);
        }
    }
    return std::vector<string>(caps.begin(), caps.end());
}

std::vector<string> Registry::FireboltPermissionsFromIds(const std::unordered_set<string>& ids) const {
    std::vector<string> out;
    out.reserve(ids.size());
    for (const auto& id : ids) out.push_back(id);
    return out;
}

string Registry::trim(const string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

bool Registry::starts_with(const string& s, const string& p) {
    return s.rfind(p, 0) == 0;
}

} // namespace Plugin
} // namespace WPEFramework
