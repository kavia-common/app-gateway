import { ConsumerContext } from './types';

/**
 * CorrelationStore tracks pending consumer requests awaiting provider responses.
 */
export class CorrelationStore {
  private byCorrelationId: Map<string, ConsumerContext> = new Map();

  /**
   * Create a correlationId for a consumer request and persist its context.
   */
  create(ctx: ConsumerContext): string {
    const correlationId = cryptoRandomId();
    this.byCorrelationId.set(correlationId, ctx);
    return correlationId;
  }

  /**
   * Find and remove by correlationId (one-shot).
   */
  findAndErase(correlationId: string): ConsumerContext | undefined {
    const ctx = this.byCorrelationId.get(correlationId);
    if (ctx) {
      this.byCorrelationId.delete(correlationId);
    }
    return ctx;
  }

  /**
   * Cleanup entries by connection (e.g., on Detach).
   */
  cleanupByConnection(connectionId: string): void {
    for (const [id, ctx] of this.byCorrelationId.entries()) {
      if (ctx.connectionId === connectionId) {
        this.byCorrelationId.delete(id);
      }
    }
  }
}

function cryptoRandomId(): string {
  // Lightweight random id suitable for correlation (not cryptographically strong but sufficient for correlation).
  // Replace with crypto.randomUUID() if available in runtime.
  const rnd = () => Math.floor(Math.random() * 0xffffffff).toString(16).padStart(8, '0');
  return `${rnd()}-${rnd()}`;
}
