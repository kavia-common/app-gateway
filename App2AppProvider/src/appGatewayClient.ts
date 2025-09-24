import { ConsumerContext } from './types';

/**
 * AppGatewayClient encapsulates calling org.rdk.AppGateway.respond through an injected RPC function.
 * The actual transport (e.g., Thunder dispatcher, WS JSON-RPC client) must be supplied by the host.
 */
export class AppGatewayClient {
  constructor(private readonly invokeRpc: (method: string, params: unknown) => Promise<unknown>) {}

  /**
   * PUBLIC_INTERFACE
   * Respond back to the original consumer via AppGateway using the stored context.
   * The payload must include either a { result: ... } or { error: { code, message } }.
   */
  async respond(consumerCtx: ConsumerContext, payload: unknown): Promise<void> {
    const params = {
      context: {
        requestId: consumerCtx.requestId,
        connectionId: consumerCtx.connectionId, // uint32
        appId: consumerCtx.appId
      },
      payload
    };
    await this.invokeRpc('org.rdk.AppGateway.respond', params);
  }
}
