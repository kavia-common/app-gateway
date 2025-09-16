import {
  HandleProviderErrorParams,
  HandleProviderResponseParams,
  InvokeProviderParams,
  JSONRPC_ERRORS,
  RegisterProviderParams,
  RequestContext,
  ConsumerContext
} from './types';
import { ProviderRegistry } from './providerRegistry';
import { CorrelationStore } from './correlationStore';
import { AppGatewayClient } from './appGatewayClient';

/**
 * App2AppProvider plugin (scaffold).
 * Exposes JSON-RPC handlers aligning with the technical designs.
 */
export class App2AppProvider {
  private readonly providers: ProviderRegistry = new ProviderRegistry();
  private readonly correlations: CorrelationStore = new CorrelationStore();
  private readonly gateway: AppGatewayClient;

  constructor(
    // Provide a function to invoke JSON-RPC methods to other plugins (e.g., AppGateway)
    invokeRpc: (method: string, params: unknown) => Promise<unknown>
  ) {
    this.gateway = new AppGatewayClient(invokeRpc);
  }

  // PUBLIC_INTERFACE
  public async registerProvider(params: RegisterProviderParams): Promise<null> {
    validateContext(params?.context);
    if (typeof params?.register !== 'boolean') {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }
    if (!isNonEmptyString(params?.capability)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }

    const { context, register, capability } = params;
    if (register) {
      this.providers.register(capability, context.appId, context.connectionId);
    } else {
      this.providers.unregister(capability, context.connectionId);
    }
    return null;
  }

  // PUBLIC_INTERFACE
  public async invokeProvider(params: InvokeProviderParams): Promise<{ correlationId: string } | null> {
    validateContext(params?.context);
    if (!isNonEmptyString(params?.capability)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }

    const provider = this.providers.find(params.capability);
    if (!provider) {
      // Provider not registered for this capability
      throw jsonRpcError(JSONRPC_ERRORS.PROVIDER_NOT_FOUND, 'PROVIDER_NOT_FOUND');
    }

    // Create correlation for the consumer request
    const consumerCtx: ConsumerContext = {
      requestId: params.context.requestId,
      connectionId: params.context.connectionId,
      appId: params.context.appId,
      capability: params.capability,
      createdAt: Date.now()
    };
    const correlationId = this.correlations.create(consumerCtx);

    // Note: As per spec, App2AppProvider does not notify the provider here.
    // Dispatching to provider is outside of this plugin's scope in Phase 1.
    // We return the correlationId so upstream may use it if desired.
    return { correlationId };
  }

  // PUBLIC_INTERFACE
  public async handleProviderResponse(params: HandleProviderResponseParams): Promise<null> {
    // Validate payload shape
    if (!params || !params.payload || !isNonEmptyString(params.payload.correlationId)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }
    if (!isNonEmptyString(params.capability)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }

    const ctx = this.correlations.findAndErase(params.payload.correlationId);
    if (!ctx) {
      throw jsonRpcError(JSONRPC_ERRORS.UNKNOWN_CORRELATION, 'UNKNOWN_CORRELATION');
    }

    // Forward the provider's successful result back to the original consumer
    await this.gateway.respond(ctx, { result: params.payload.result });
    return null;
  }

  // PUBLIC_INTERFACE
  public async handleProviderError(params: HandleProviderErrorParams): Promise<null> {
    if (!params || !params.payload || !isNonEmptyString(params.payload.correlationId)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }
    if (!params.payload.error || typeof params.payload.error.code !== 'number' || !isNonEmptyString(params.payload.error.message)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }
    if (!isNonEmptyString(params.capability)) {
      throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
    }

    const ctx = this.correlations.findAndErase(params.payload.correlationId);
    if (!ctx) {
      throw jsonRpcError(JSONRPC_ERRORS.UNKNOWN_CORRELATION, 'UNKNOWN_CORRELATION');
    }

    // Forward the provider's error back to the original consumer
    await this.gateway.respond(ctx, { error: params.payload.error });
    return null;
  }

  /**
   * Optional hook: clean up all state for a disconnecting connection.
   */
  public onConnectionDetached(connectionId: string): void {
    this.providers.cleanupByConnection(connectionId);
    this.correlations.cleanupByConnection(connectionId);
  }
}

// --- Utilities ---

function validateContext(context?: RequestContext): asserts context is RequestContext {
  if (!context) throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
  if (typeof context.requestId !== 'number') throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
  if (!isNonEmptyString(context.connectionId)) throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
  if (!isNonEmptyString(context.appId)) throw jsonRpcError(JSONRPC_ERRORS.INVALID_PARAMS, 'INVALID_PARAMS');
}

function isNonEmptyString(v: unknown): v is string {
  return typeof v === 'string' && v.trim().length > 0;
}

function jsonRpcError(code: number, message: string): Error {
  const err = new Error(message);
  (err as any).code = code;
  return err;
}

// Below is an adapter example showing how these methods can be bound to a JSON-RPC server.
// Adapt to your runtime as needed.
/*
export function bindToRpcServer(rpcServer: JsonRpcServer, provider: App2AppProvider) {
  rpcServer.register('org.rdk.ApptoAppProvider.registerProvider', (params) => provider.registerProvider(params));
  rpcServer.register('org.rdk.ApptoAppProvider.invokeProvider', (params) => provider.invokeProvider(params));
  rpcServer.register('org.rdk.ApptoAppProvider.handleProviderResponse', (params) => provider.handleProviderResponse(params));
  rpcServer.register('org.rdk.ApptoAppProvider.handleProviderError', (params) => provider.handleProviderError(params));
}
*/
