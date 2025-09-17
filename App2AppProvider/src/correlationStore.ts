/**
 * CorrelationStore maintains correlationId -> consumer context mapping.
 * The new design is minimal and independent.
 */

import { ConsumerContext } from './types';

export class CorrelationStore {
  private readonly byCorrelationId = new Map<string, ConsumerContext>();

  /**
   * Create a correlation id and store context.
   */
  create(context: ConsumerContext): string {
    const id = randomCorrelationId();
    this.byCorrelationId.set(id, context);
    return id;
  }

  /**
   * Retrieve and remove the stored context (one-shot).
   */
  findAndErase(correlationId: string): ConsumerContext | undefined {
    const ctx = this.byCorrelationId.get(correlationId);
    if (ctx) {
      this.byCorrelationId.delete(correlationId);
    }
    return ctx;
  }

  /**
   * Cleanup entries by connection id for closed/detached connections.
   */
  cleanupByConnection(connectionId: string): void {
    for (const [cid, ctx] of this.byCorrelationId.entries()) {
      if (ctx.connectionId === connectionId) {
        this.byCorrelationId.delete(cid);
      }
    }
  }
}

function randomCorrelationId(): string {
  // Light-weight random id (not cryptographically secure).
  const part = () => Math.floor(Math.random() * 0xffffffff).toString(16).padStart(8, '0');
  return `${part()}-${part()}`;
}
