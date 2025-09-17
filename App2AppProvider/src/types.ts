//
// Fresh, minimal types for the App2AppProvider plugin
//

/**
 * Shared request context across gateway-directed flows.
 * Encapsulates the original request identity and routing info so the gateway
 * can deliver responses back to the right caller/connection.
 */
// PUBLIC_INTERFACE
export interface RequestContext {
  /** Incoming request id from the consumer/provider request. */
  requestId: number;
  /** Unique connection identifier (e.g., ChannelId or GUID string). */
  connectionId: string;
  /** Authenticated application id. */
  appId: string;
}

/**
 * Params for registering (or unregistering) a provider capability.
 */
// PUBLIC_INTERFACE
export interface RegisterProviderParams {
  context: RequestContext;
  register: boolean;
  capability: string;
}

/**
 * Params for invoking a registered provider capability on behalf of a consumer.
 */
// PUBLIC_INTERFACE
export interface InvokeProviderParams {
  context: RequestContext;
  capability: string;
  payload?: unknown;
}

/**
 * Provider success payload wrapper used when the provider answers back.
 */
// PUBLIC_INTERFACE
export interface ProviderResponsePayload {
  correlationId: string;
  result: unknown;
}

/**
 * Provider error payload wrapper used when the provider answers back with error.
 */
// PUBLIC_INTERFACE
export interface ProviderErrorPayload {
  correlationId: string;
  error: { code: number; message: string };
}

/**
 * Params for handling a provider success response.
 */
// PUBLIC_INTERFACE
export interface HandleProviderResponseParams {
  payload: ProviderResponsePayload;
  capability: string;
}

/**
 * Params for handling a provider error response.
 */
// PUBLIC_INTERFACE
export interface HandleProviderErrorParams {
  payload: ProviderErrorPayload;
  capability: string;
}

/**
 * Maintained consumer context captured when a provider invocation is made.
 * This is used to route any later provider response back via the AppGateway.
 */
export interface ConsumerContext {
  requestId: number;
  connectionId: string;
  appId: string;
  capability: string;
  createdAt: number;
}

/**
 * Provider entry representing the active provider for a capability.
 */
export interface ProviderEntry {
  appId: string;
  connectionId: string;
  registeredAt: number;
}

/**
 * JSON-RPC error codes used across plugin calls.
 */
export const JSONRPC_ERRORS = {
  INVALID_PARAMS: -32602,
  INVALID_REQUEST: -32699,
  PROVIDER_NOT_FOUND: -32004,
  DISPATCH_FAILED: -32005,
  UNKNOWN_CORRELATION: -32006
};

// PUBLIC_INTERFACE
export interface JsonRpcResult<T = unknown> {
  result: T;
}

// PUBLIC_INTERFACE
export interface JsonRpcError {
  error: { code: number; message: string };
}
