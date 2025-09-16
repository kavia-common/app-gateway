//
// Shared DTOs and types for the App2AppProvider plugin
//

// PUBLIC_INTERFACE
export interface RequestContext {
  /** Incoming request id of the consumer/provider request. */
  requestId: number;
  /** Connection identifier (ChannelId or GUID). */
  connectionId: string;
  /** Authenticated application identifier. */
  appId: string;
}

// PUBLIC_INTERFACE
export interface RegisterProviderParams {
  context: RequestContext;
  register: boolean;
  capability: string;
}

// PUBLIC_INTERFACE
export interface InvokeProviderParams {
  context: RequestContext;
  capability: string;
  payload?: unknown;
}

// PUBLIC_INTERFACE
export interface ProviderResponsePayload {
  correlationId: string;
  result: unknown;
}

// PUBLIC_INTERFACE
export interface ProviderErrorPayload {
  correlationId: string;
  error: { code: number; message: string };
}

// PUBLIC_INTERFACE
export interface HandleProviderResponseParams {
  payload: ProviderResponsePayload;
  capability: string;
}

// PUBLIC_INTERFACE
export interface HandleProviderErrorParams {
  payload: ProviderErrorPayload;
  capability: string;
}

// Consumer context captured for correlation
export interface ConsumerContext {
  requestId: number;
  connectionId: string;
  appId: string;
  capability: string;
  createdAt: number;
}

// Provider registration entry
export interface ProviderEntry {
  appId: string;
  connectionId: string;
  registeredAt: number;
}

// Errors follow JSON-RPC error codes from the specs
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
