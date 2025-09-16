#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * RegistryEntry represents a single Thor/Badger permission definition as loaded from YAML.
 */
struct RegistryEntry {
    string id;
    std::vector<string> useCaps;
    std::vector<string> manageCaps;
    std::vector<string> provideCaps;
    std::vector<string> apis;
};

/**
 * Registry loads and provides lookups for permission mappings from a YAML-like source.
 * This implementation parses a constrained subset of YAML that matches the project's
 * thor_permission_registry.yaml format (keys: id, capabilities:{use/manage/provide}, apis).
 * For production usage, wire a fully-featured YAML parser and keep this interface stable.
 */
class Registry {
public:
    // PUBLIC_INTERFACE
    /**
     * Load registry from a file path.
     * @param path  The absolute or relative path to the YAML file.
     * @param errorOut Populated on failure with a human-readable description.
     * @return unique_ptr<Registry> on success, nullptr on failure.
     */
    static std::unique_ptr<Registry> LoadFromFile(const string& path, string& errorOut);

    // PUBLIC_INTERFACE
    /**
     * Map capability + role -> list of Thor permission IDs.
     */
    std::vector<string> MapCapabilityToIds(const string& capability, const string& role) const;

    // PUBLIC_INTERFACE
    /**
     * Map API name -> list of Thor permission IDs.
     */
    std::vector<string> MapApiToIds(const string& api) const;

    // PUBLIC_INTERFACE
    /**
     * Return all capability strings present in the registry (union of all role lists).
     */
    std::vector<string> AllCapabilities() const;

    // PUBLIC_INTERFACE
    /**
     * Given a set of Thor IDs, return capability strings (union across roles).
     */
    std::vector<string> CapabilitiesFromIds(const std::unordered_set<string>& ids) const;

    // PUBLIC_INTERFACE
    /**
     * Given a set of Thor IDs, return the raw ID list (echo).
     */
    std::vector<string> FireboltPermissionsFromIds(const std::unordered_set<string>& ids) const;

private:
    std::vector<RegistryEntry> _entries;

    static string trim(const string& s);
    static bool starts_with(const string& s, const string& p);
    static bool is_empty_list(const string& s); // detect [] or [ ] convenience
};

} // namespace Plugin
} // namespace WPEFramework
