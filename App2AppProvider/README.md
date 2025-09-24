# App2AppProvider Plugin

This directory contains the App2AppProvider plugin implementation per the following specifications:
- app2appprovider-technical-design.md
- appgatewaytechnical-design.md
- thunder-plugins-architecture.md

Key responsibilities:
- Register/unregister provider capabilities for apps.
- Accept consumer invocations for a capability; create correlation and track consumer context.
- Accept provider responses/errors and route them back to the original consumer using AppGateway.respond.

Structure:
- src/
  - index.ts: Package barrel, exports the public API.
  - index.impl.ts: App2AppProvider implementation.
  - providerRegistry.ts: Capability → provider registration management.
  - correlationStore.ts: CorrelationId → consumer context store.
  - appGatewayClient.ts: Client wrapper to call org.rdk.AppGateway.respond.
  - internal.ts: Internal re-exports to keep imports clean.
  - types.ts: Shared DTOs and types.

Public API:
- App2AppProvider, App2AppProviderOptions, SendToConnection
- ProviderRegistry, CorrelationStore, AppGatewayClient (helpers for composition)
- All request/response types from src/types.ts

JSON-RPC methods exposed by App2AppProvider (call these from your transport binding):
- registerProvider(params: RegisterProviderParams) → { result: { registered: boolean } }
- invokeProvider(params: InvokeProviderParams) → { result: { correlationId: string } }
- handleProviderResponse(params: HandleProviderResponseParams) → { result: { delivered: true } }
- handleProviderError(params: HandleProviderErrorParams) → { result: { delivered: true } }
- onConnectionClosed(connectionId: string) → void

Instantiation:
- The plugin is transport-agnostic. Provide bindings via:
  - sendToConnection(connectionId, method, params): Promise<unknown> → how to send a JSON-RPC call to a specific connection (provider).
  - gatewayRpc(method, params): Promise<unknown> → how to call org.rdk.AppGateway.respond.
- Example:

```ts
import {
  App2AppProvider,
  AppGatewayClient,
  ProviderRegistry,
  CorrelationStore
} from '@org-rdk/app2appprovider';

const sendToConnection = async (connectionId: string, method: string, params: unknown) => {
  // Bridge to your Thunder/WS JSON-RPC client with routing by connectionId
  return transport.invokeOnConnection(connectionId, method, params);
};

const gatewayRpc = async (method: string, params: unknown) => {
  // Bridge to your AppGateway plugin
  return transport.invoke(method, params);
};

const provider = new App2AppProvider(sendToConnection, gatewayRpc, {
  providerInvokeMethod: 'org.rdk.App2AppProvider.request', // default
  onLog: (msg, data) => console.debug('[App2AppProvider]', msg, data)
});

// Register JSON-RPC methods in your host (example names)
rpc.register('org.rdk.App2AppProvider.register', (params) => provider.registerProvider(params));
rpc.register('org.rdk.App2AppProvider.invoke', (params) => provider.invokeProvider(params));
rpc.register('org.rdk.App2AppProvider.response', (params) => provider.handleProviderResponse(params));
rpc.register('org.rdk.App2AppProvider.error', (params) => provider.handleProviderError(params));

// Cleanup when a connection closes
transport.on('connectionClosed', (connectionId) => provider.onConnectionClosed(connectionId));
```

Dispatch to provider:
- When a consumer invokes a capability, the provider implementation receives a call to:
  - method: providerInvokeMethod (default 'org.rdk.App2AppProvider.request')
  - params: { correlationId, capability, context: { appId, connectionId, requestId }, payload? }
- The provider must answer by calling the corresponding JSON-RPC methods on this plugin:
  - Success: handleProviderResponse({ capability, payload: { correlationId, result } })
  - Error: handleProviderError({ capability, payload: { correlationId, error: { code, message } } })

Design alignment:
- Follows the separation of concerns described in the technical designs: registry (capability → provider), correlation tracking (ID → consumer context), and AppGateway response routing.
- JSON-RPC error codes use the shared constants in src/types.ts.
- Defensive validation before operations, and safe cleanup on dispatch failures or connection closure.

Build & Lint (inside this package):
- npm i
- npm run lint
- npm run build
