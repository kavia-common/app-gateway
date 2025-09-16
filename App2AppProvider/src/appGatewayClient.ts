import { ConsumerContext } from './types';

/**
 * AppGatewayClient wraps calling org.rdk.AppGateway.respond.
 * This scaffold exposes a synchronous interface; adapt to your runtime JSON-RPC client.
 */
export class AppGatewayClient {
  constructor(private readonly invokeRpc: (method: string, params: unknown) => Promise<unknown>) {}

  /**
   * Call AppGateway.respond with the provided consumer context and payload.
   */
  async respond(consumerCtx: ConsumerContext, payload: unknown): Promise<void> {
    const params = {
      context: {
        requestId: consumerCtx.requestId,
        connectionId: consumerCtx.connectionId,
        appId: consumerCtx.appId
      },
      payload
    };
    await this.invokeRpc('org.rdk.AppGateway.respond', params);
  }
}
