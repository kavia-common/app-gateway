# Badger Thunder Plugin (C++)

Badger is a Thunder/WPEFramework plugin providing permission checks for Firebolt capabilities based on Thor/Badger permission IDs and a YAML-driven registry mapping.

Highlights:
- Thunder-native: IPlugin + JSONRPC with documented handlers.
- Registry parsing for a constrained YAML subset: maps IDs to capabilities per role and APIs.
- Permission evaluation: dynamic granted IDs ∩ registry-derived aliases.
- JSON-RPC API:
  - org.rdk.Badger.ping
  - org.rdk.Badger.permissions.check
  - org.rdk.Badger.permissions.checkAll
  - org.rdk.Badger.permissions.listCaps
  - org.rdk.Badger.permissions.listFireboltPermissions
  - org.rdk.Badger.permissions.listMethods

Configuration (plugin.json → configuration):
- registryPath (string): YAML file path, e.g., /etc/badger/thor_permission_registry.yaml
- cacheTtlSeconds (number): TTL hint for future dynamic providers (default 3600)
- grantedIds (array[string]): Optional initial granted Thor/Badger permission IDs
- logLevel (string): optional

Notes:
- A production deployment should integrate a dynamic granted-IDs provider (e.g., Thor Permission Service) and call PermissionService::SetGrantedIds accordingly.
- The current YAML parser is a robust line-based implementation for a constrained subset; replace with a full YAML parser if needed while keeping the Registry interface stable.
