/*!
Badger Thunder Plugin (Rust)

This crate provides:
- YAML-driven permission registry (thor_permission_registry.yaml).
- ThorPermissionService to check Firebolt capability/API permissions against dynamic grants.
- A JSON-RPC facade exposing Badger APIs (permissions.*) with LaunchDelegate/ AppGateway integration points.
- TTL caching, robust logging, and error-safe design.

Integrate with Thunder/WPEFramework by adapting the LaunchDelegateClient and AppGatewayClient traits
to your runtime's JSON-RPC transport (e.g., IDispatcher::Invoke). This crate provides default mock
implementations that log calls for development.

Modules:
- thor_permission: registry-facing permission resolution and authorization.
- registry: YAML loader for permission definitions.
- cache: simple TTL cache for dynamic permissions.
- jsonrpc: JSON-RPC DTOs and dispatcher for Badger APIs.
- launch_delegate: LaunchDelegate client trait (authenticate, lifecycle hook points).
- app_gateway: AppGateway.respond client trait for asynchronous responses to apps.
- errors: error types (thiserror).
- logging: logging bootstrap.
*/

pub mod thor_permission;
pub mod registry;
pub mod cache;
pub mod jsonrpc;
pub mod launch_delegate;
pub mod app_gateway;
pub mod errors;
pub mod logging;

use crate::app_gateway::AppGatewayClient;
use crate::jsonrpc::{BadgerJsonRpc, JsonRpcHandler};
use crate::launch_delegate::LaunchDelegateClient;
use crate::registry::YamlPermissionRegistry;
use crate::thor_permission::{PermissionRole, ThorPermissionService};

use std::sync::Arc;
use parking_lot::RwLock;

/// BadgerPlugin bundles the service dependencies and exports a JSON-RPC handler.
pub struct BadgerPlugin {
    permission_service: Arc<ThorPermissionService>,
}

impl BadgerPlugin {
    /// Create a plugin from a registry yaml path and injected clients.
    // PUBLIC_INTERFACE
    pub fn new<Rc: AsRef<str>>(
        registry_yaml_path: Rc,
        launch_delegate: Arc<dyn LaunchDelegateClient + Send + Sync>,
        app_gateway: Arc<dyn AppGatewayClient + Send + Sync>,
    ) -> anyhow::Result<Self> {
        let registry = YamlPermissionRegistry::from_file(registry_yaml_path.as_ref())?;
        let permission_service = Arc::new(ThorPermissionService::new(
            Box::new(registry),
            launch_delegate,
            app_gateway,
        ));
        Ok(Self { permission_service })
    }

    /// Get a JSON-RPC handler bound to this plugin.
    // PUBLIC_INTERFACE
    pub fn jsonrpc_handler(&self) -> JsonRpcHandler {
        BadgerJsonRpc::new(self.permission_service.clone())
    }
}

// Convenience re-exports
pub use thor_permission::PermissionRequestResult;
pub use thor_permission::PermissionCheckAllResult;
pub use thor_permission::PermissionGrant;
pub use thor_permission::PermissionRole as Role;
