/**
 * Package barrel: expose public interfaces and the provider class.
 * This is the only entry imported by consumers of this package.
 */
export * from './types';
export { App2AppProvider, App2AppProviderOptions, SendToConnection } from './index.impl';

// Optional helpers for composition from host applications
export { ProviderRegistry } from './providerRegistry';
export { CorrelationStore } from './correlationStore';
export { AppGatewayClient } from './appGatewayClient';
