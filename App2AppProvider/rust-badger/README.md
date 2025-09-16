# Badger Thunder Plugin (Rust)

This Rust crate provides a Badger Thunder plugin that:
- Implements permission checks against Thor permissions using a YAML-driven registry (thor_permission_registry.yaml).
- Exposes JSON-RPC methods for Badger-style permission APIs:
  - org.rdk.Badger.permissions.check
  - org.rdk.Badger.permissions.checkAll
  - org.rdk.Badger.permissions.listCaps
  - org.rdk.Badger.permissions.listFireboltPermissions
  - org.rdk.Badger.permissions.listMethods
- Uses a TTL cache to store dynamic granted Thor permission IDs per (account/session, appId).
- Integrates with LaunchDelegate (authenticate) and AppGateway (respond) through trait-based clients with default no-op/mock implementations to be adapted to your runtime.
- Is thoroughly logged and error-safe with explicit error types.

Project structure:
- src/lib.rs: Public crate API, plugin facade, and JSON-RPC integration.
- src/main.rs: Optional demo JSON-RPC HTTP server for local testing.
- src/jsonrpc.rs: Request/response DTOs and dispatcher.
- src/thor_permission.rs: ThorPermissionService core logic and API.
- src/registry.rs: YAML-driven `FireboltPermissionRegistry` implementation.
- src/cache.rs: Simple in-memory TTL cache for granted permissions.
- src/launch_delegate.rs: LaunchDelegate client trait and default implementation.
- src/app_gateway.rs: AppGateway.respond client trait and default implementation.
- src/errors.rs: Error types.
- src/logging.rs: Logging initialization helper.

Permission registry:
- `thor_permission_registry.yaml` defines Thor permission IDs and their mappings to Firebolt capabilities and APIs. The registry is loaded at startup.

Build:
- cargo build
- cargo run --bin badger-thunder-plugin

Note:
- Adapt `LaunchDelegateClient` and `AppGatewayClient` to your Thunder runtime transport (e.g., WPEFramework JSON-RPC COM). Default implementations are stubs that log activity.
- This crate is self-contained and co-exists with the existing TypeScript scaffolding under App2AppProvider. Only one should be active in a given deployment.

Security:
- Deny-by-default. If mappings are missing or Thor returns no granted IDs, all checks deny.
- TTL cache may yield stale grants; tune TTL or wire runtime invalidation as needed.

License:
- Apache-2.0
