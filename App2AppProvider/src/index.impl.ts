import {
  AppGatewayClient,
  CorrelationStore,
  ProviderRegistry
} from './internal';
import {
  HandleProviderErrorParams,
  HandleProviderResponseParams,
  InvokeProviderParams,
  JSONRPC_ERRORS,
  JsonRpcError,
  JsonRpcResult,
  ProviderEntry,
  RegisterProviderParams
} from './types';

/**
 * INTERNAL MODULE BARRIER
 * Exported via the package barrel (src/index.ts) for public consumption.
 */

/**
 * Function signature used to send a JSON-RPC call to a specific connection (e.g., provider app).
 * The host environment must supply this. Typically, this bridges to the underlying transport
 * (Thunder/WS JSON-RPC), using the given connectionId as routing info.
 */
export type SendToConnection = (connectionId: number, method: string, params: unknown) => Promise<unknown>;

/**
 * Options to customize the behaviour and integration points of the App2AppProvider.
 */
export interface App2AppProviderOptions {
  /**
   * JSON-RPC method invoked on the provider connection when a consumer performs an invoke.
   * The payload is: { correlationId, capability, context: { appId, connectionId (uint32), requestId }, payload? }
   * Default: 'org.rdk.App2AppProvider.request'
   */
  providerInvokeMethod: string;

  /**
   * Custom registry instance. If omitted, a default in-memory registry is used.
   */
  registry?: ProviderRegistry;

  /**
   * Custom correlation store instance. If omitted, a default in-memory correlation store is used.
   */
  correlationStore?: CorrelationStore;

  /**
   * Optional logger callback for debug/trace.
   */
  onLog?: (message: string, data?: unknown) => void;
}

/**
 * Default options for the App2AppProvider.
 */
const DEFAULT_OPTIONS: App2AppProviderOptions = {
  providerInvokeMethod: 'org.rdk.App2AppProvider.request',
  onLog: undefined
};

/**
 * Utility to construct a JSON-RPC error envelope.
 */
function makeError(code: number, message: string): JsonRpcError {
  return { error: { code, message } };
}

/**
 * Validate that a value is a non-empty string.
 */
function isNonEmptyString(value: unknown): value is string {
  return typeof value === 'string' && value.trim().length > 0;
}

/**
 * Validate that a value is a finite number.
 */
function isFiniteNumber(value: unknown): value is number {
  return typeof value === 'number' && Number.isFinite(value);
}

/**
 * Validate that a value is an unsigned 32-bit integer (uint32_t domain).
 */
function isUint32Number(value: unknown): value is number {
  return (
    typeof value === 'number' &&
    Number.isInteger(value) &&
    value >= 0 &&
    value <= 0xFFFFFFFF
  );
}

/**
 * Basic structural validation for RegisterProviderParams.
 */
function validateRegisterParams(params: RegisterProviderParams): string | undefined {
  if (!params || typeof params !== 'object') return 'params must be an object.';
  const { context, register, capability } = params;
  if (!context || typeof context !== 'object') return 'context is required.';
  if (!isFiniteNumber(context.requestId)) return 'context.requestId must be a number.';
  if (!isUint32Number(context.connectionId)) return 'context.connectionId must be a uint32 number.';
  if (!isNonEmptyString(context.appId)) return 'context.appId must be a non-empty string.';
  if (typeof register !== 'boolean') return 'register must be boolean.';
  if (!isNonEmptyString(capability)) return 'capability must be a non-empty string.';
  return undefined;
}

/**
 * Basic structural validation for InvokeProviderParams.
 */
function validateInvokeParams(params: InvokeProviderParams): string | undefined {
  if (!params || typeof params !== 'object') return 'params must be an object.';
  const { context, capability } = params;
  if (!context || typeof context !== 'object') return 'context is required.';
  if (!isFiniteNumber(context.requestId)) return 'context.requestId must be a number.';
  if (!isUint32Number(context.connectionId)) return 'context.connectionId must be a uint32 number.';
  if (!isNonEmptyString(context.appId)) return 'context.appId must be a non-empty string.';
  if (!isNonEmptyString(capability)) return 'capability must be a non-empty string.';
  return undefined;
}

/**
 * Basic structural validation for HandleProviderResponseParams.
 */
function validateHandleProviderResponseParams(params: HandleProviderResponseParams): string | undefined {
  if (!params || typeof params !== 'object') return 'params must be an object.';
  const { payload, capability } = params;
  if (!payload || typeof payload !== 'object') return 'payload is required.';
  if (!isNonEmptyString((payload as any).correlationId)) return 'payload.correlationId must be a non-empty string.';
  if (!('result' in (payload as any))) return 'payload.result is required.';
  if (!isNonEmptyString(capability)) return 'capability must be a non-empty string.';
  return undefined;
}

/**
 * Basic structural validation for HandleProviderErrorParams.
 */
function validateHandleProviderErrorParams(params: HandleProviderErrorParams): string | undefined {
  if (!params || typeof params !== 'object') return 'params must be an object.';
  const { payload, capability } = params;
  if (!payload || typeof payload !== 'object') return 'payload is required.';
  if (!isNonEmptyString((payload as any).correlationId)) return 'payload.correlationId must be a non-empty string.';
  const err = (payload as any).error;
  if (!err || typeof err !== 'object') return 'payload.error is required.';
  if (!isFiniteNumber(err.code) || !isNonEmptyString(err.message)) return 'payload.error must contain numeric code and string message.';
  if (!isNonEmptyString(capability)) return 'capability must be a non-empty string.';
  return undefined;
}

/**
 * Locate an active provider or return a JSON-RPC error.
 */
function findOrErr(registry: ProviderRegistry, capability: string): ProviderEntry | JsonRpcError {
  const provider = registry.find(capability);
  if (!provider) {
    return makeError(JSONRPC_ERRORS.PROVIDER_NOT_FOUND, `No provider registered for capability "${capability}".`);
  }
  return provider;
}

/**
 * PUBLIC_INTERFACE
 * App2AppProvider is a hostable plugin logic unit that:
 * - registers/unregisters provider capabilities
 * - accepts consumer invocations and dispatches them to a provider
 * - receives provider responses/errors and forwards them back via AppGateway.respond
 *
 * Transport is not hard-coded; the host must inject functions for:
 * - sending a request to a specific connection (sendToConnection)
 * - invoking AppGateway.respond via AppGatewayClient (constructed with the host RPC caller)
 */
export class App2AppProvider {
  private readonly registry: ProviderRegistry;
  private readonly correlations: CorrelationStore;
  private readonly gateway: AppGatewayClient;
  private readonly sendToConnection: SendToConnection;
  private readonly opts: App2AppProviderOptions;

  /**
   * Create an App2AppProvider instance.
   *
   * @param sendToConnection function that routes a JSON-RPC call to a specific connectionId (uint32).
   * @param gatewayRpc function used by AppGatewayClient to call org.rdk.AppGateway.respond.
   * @param options optional behavior overrides and custom stores.
   */
  // PUBLIC_INTERFACE
  constructor(
    sendToConnection: SendToConnection,
    gatewayRpc: (method: string, params: unknown) => Promise<unknown>,
    options?: Partial<App2AppProviderOptions>
  ) {
    this.sendToConnection = sendToConnection;
    this.gateway = new AppGatewayClient(gatewayRpc);
    this.opts = { ...DEFAULT_OPTIONS, ...(options ?? {}) };
    this.registry = this.opts.registry ?? new ProviderRegistry();
    this.correlations = this.opts.correlationStore ?? new CorrelationStore();
  }

  /**
   * PUBLIC_INTERFACE
   * Register or unregister a provider capability for the requesting connection.
   *
   * Returns:
   *  - { result: { registered: boolean } }
   * Errors:
   *  - INVALID_PARAMS
   */
  async registerProvider(params: RegisterProviderParams): Promise<JsonRpcResult<{ registered: boolean }> | JsonRpcError> {
    const errMsg = validateRegisterParams(params);
    if (errMsg) {
      return makeError(JSONRPC_ERRORS.INVALID_PARAMS, errMsg);
    }

    const { context, register, capability } = params;
    if (register) {
      this.registry.register(capability, context.appId, context.connectionId);
      this.log('Registered provider', { capability, appId: context.appId, connectionId: context.connectionId });
      return { result: { registered: true } };
    } else {
      this.registry.unregister(capability, context.connectionId);
      this.log('Unregistered provider', { capability, connectionId: context.connectionId });
      return { result: { registered: false } };
    }
  }

  /**
   * PUBLIC_INTERFACE
   * Invoke an active provider for a capability on behalf of a consumer.
   * Creates a correlation and stores the consumer context to route the provider reply.
   *
   * Returns:
   *  - { result: { correlationId: string } }
   * Errors:
   *  - INVALID_PARAMS
   *  - PROVIDER_NOT_FOUND
   *  - DISPATCH_FAILED
   */
  async invokeProvider(params: InvokeProviderParams): Promise<JsonRpcResult<{ correlationId: string }> | JsonRpcError> {
    const errMsg = validateInvokeParams(params);
    if (errMsg) {
      return makeError(JSONRPC_ERRORS.INVALID_PARAMS, errMsg);
    }

    const { context, capability, payload } = params;
    const found = findOrErr(this.registry, capability);
    if ('error' in found) {
      return found;
    }

    const provider = found as ProviderEntry;
    // Capture consumer context for future response routing
    const consumerCtx = {
      requestId: context.requestId,
      connectionId: context.connectionId,
      appId: context.appId,
      capability,
      createdAt: Date.now()
    };
    const correlationId = this.correlations.create(consumerCtx);

    const invokeParams = {
      correlationId,
      capability,
      context: {
        appId: context.appId,
        connectionId: context.connectionId,
        requestId: context.requestId
      },
      payload
    };

    try {
      await this.sendToConnection(provider.connectionId, this.opts.providerInvokeMethod, invokeParams);
      this.log('Dispatched invoke to provider', { capability, providerConnection: provider.connectionId, correlationId });
    } catch (e: any) {
      // Clean up correlation on dispatch failure
      this.correlations.findAndErase(correlationId);
      const message = `Failed to dispatch to provider for capability "${capability}": ${e?.message ?? String(e)}`;
      this.log('Dispatch failed', { capability, error: message });
      return makeError(JSONRPC_ERRORS.DISPATCH_FAILED, message);
    }

    return { result: { correlationId } };
  }

  /**
   * PUBLIC_INTERFACE
   * Handle a provider success response. The provider calls this method with a correlationId and result.
   * The provider capability is included for validation/trace.
   *
   * Returns:
   *  - { result: { delivered: true } }
   * Errors:
   *  - INVALID_PARAMS
   *  - UNKNOWN_CORRELATION
   *  - DISPATCH_FAILED (if AppGateway.respond fails)
   */
  async handleProviderResponse(params: HandleProviderResponseParams): Promise<JsonRpcResult<{ delivered: true }> | JsonRpcError> {
    const errMsg = validateHandleProviderResponseParams(params);
    if (errMsg) {
      return makeError(JSONRPC_ERRORS.INVALID_PARAMS, errMsg);
    }

    const { payload, capability } = params;
    const ctx = this.correlations.findAndErase(payload.correlationId);
    if (!ctx) {
      return makeError(JSONRPC_ERRORS.UNKNOWN_CORRELATION, `Unknown correlationId "${payload.correlationId}".`);
    }

    try {
      await this.gateway.respond(ctx, { result: payload.result });
      this.log('Delivered provider response via AppGateway.respond', { capability, correlationId: payload.correlationId });
      return { result: { delivered: true } };
    } catch (e: any) {
      const message = `Failed to respond to consumer: ${e?.message ?? String(e)}`;
      this.log('Respond failed', { capability, error: message });
      return makeError(JSONRPC_ERRORS.DISPATCH_FAILED, message);
    }
  }

  /**
   * PUBLIC_INTERFACE
   * Handle a provider error response. The provider calls this method with a correlationId and error object.
   *
   * Returns:
   *  - { result: { delivered: true } }
   * Errors:
   *  - INVALID_PARAMS
   *  - UNKNOWN_CORRELATION
   *  - DISPATCH_FAILED (if AppGateway.respond fails)
   */
  async handleProviderError(params: HandleProviderErrorParams): Promise<JsonRpcResult<{ delivered: true }> | JsonRpcError> {
    const errMsg = validateHandleProviderErrorParams(params);
    if (errMsg) {
      return makeError(JSONRPC_ERRORS.INVALID_PARAMS, errMsg);
    }

    const { payload, capability } = params;
    const ctx = this.correlations.findAndErase(payload.correlationId);
    if (!ctx) {
      return makeError(JSONRPC_ERRORS.UNKNOWN_CORRELATION, `Unknown correlationId "${payload.correlationId}".`);
    }

    try {
      await this.gateway.respond(ctx, { error: payload.error });
      this.log('Delivered provider error via AppGateway.respond', { capability, correlationId: payload.correlationId });
      return { result: { delivered: true } };
    } catch (e: any) {
      const message = `Failed to respond to consumer with error: ${e?.message ?? String(e)}`;
      this.log('Respond failed', { capability, error: message });
      return makeError(JSONRPC_ERRORS.DISPATCH_FAILED, message);
    }
  }

  /**
   * PUBLIC_INTERFACE
   * Notify the provider subsystem that a connection has closed/disconnected.
   * This cleans up any registered capabilities and orphaned correlations associated with the connection.
   */
  onConnectionClosed(connectionId: number): void {
    if (!isUint32Number(connectionId)) return;
    this.registry.cleanupByConnection(connectionId);
    this.correlations.cleanupByConnection(connectionId);
    this.log('Cleaned up state for closed connection', { connectionId });
  }

  private log(message: string, data?: unknown): void {
    if (this.opts.onLog) {
      this.opts.onLog(message, data);
    }
  }
}

/**
 * Internal re-exports to allow ergonomic imports from the package barrel while keeping
 * internal file paths tidy.
 */
export const __INTERNAL__ = {
  ProviderRegistry,
  CorrelationStore,
  AppGatewayClient
};
