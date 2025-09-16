# App2AppProvider Plugin

This directory contains the App2AppProvider plugin implementation scaffolding per the following specifications:
- app2appprovider-technical-design.md
- appgateway-technical-design.md
- thunder-plugins-architecture.md

Key responsibilities:
- Register/unregister provider capabilities for apps.
- Accept consumer invocations for a capability; create correlation and track consumer context.
- Accept provider responses/errors and route them back to the original consumer using AppGateway.respond.

Structure:
- src/
  - index.ts: Plugin entry point exposing the JSON-RPC methods.
  - providerRegistry.ts: Capability → provider registration management.
  - correlationStore.ts: CorrelationId → consumer context store.
  - appGatewayClient.ts: Client wrapper to call org.rdk.AppGateway.respond.
  - types.ts: Shared DTOs and types.

Notes:
- This implementation is framework-agnostic scaffolding (TypeScript), mirroring Thunder plugin interfaces and responsibilities, meant to be adapted to a real Thunder/WPEFramework C++ plugin or bound into your existing runtime.
- Error codes and method names adhere to the design docs. Validation is explicit and defensive.
