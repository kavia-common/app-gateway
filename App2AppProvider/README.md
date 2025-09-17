# App2AppProvider (Thunder Plugin)

Implements the App2AppProvider JSON-RPC interface per the design docs:

Methods:
- org.rdk.ApptoAppProvider.registerProvider
- org.rdk.ApptoAppProvider.invokeProvider
- org.rdk.ApptoAppProvider.handleProviderResponse
- org.rdk.ApptoAppProvider.handleProviderError

Responsibilities:
- Manage provider capability registrations (capability -> provider appId/connectionId).
- Correlate consumer requests (create correlationId) and store their request context.
- Accept provider responses/errors and forward them back to the originating consumer
  via org.rdk.AppGateway.respond using the preserved context.

This code follows Thunder conventions:
- Inherits IPlugin and JSONRPC, registers methods in the constructor.
- Thread-safe registries with minimal lock scopes.
- Inter-plugin calls use IDispatcher to call AppGateway.respond.

Integration notes:
- Place this folder in your Thunder plugins tree and add to the build.
- Ensure WPEFramework (Thunder) development libraries are available.
- Callsign is typically `App2AppProvider`; method namespace is
  `org.rdk.ApptoAppProvider.<method>` per the interface documents.

