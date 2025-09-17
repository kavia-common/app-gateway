/**
 * ProviderRegistry maintains a capability -> provider mapping.
 * This fresh implementation is intentionally minimal and self-contained.
 */

import { ProviderEntry } from './types';

export class ProviderRegistry {
  private readonly capabilityToProvider = new Map<string, ProviderEntry>();
  private readonly capabilitiesByConnection = new Map<string, Set<string>>();

  /**
   * Register or replace the provider for a given capability.
   */
  register(capability: string, appId: string, connectionId: string): void {
    const entry: ProviderEntry = {
      appId,
      connectionId,
      registeredAt: Date.now()
    };
    this.capabilityToProvider.set(capability, entry);

    const set = this.capabilitiesByConnection.get(connectionId) ?? new Set<string>();
    set.add(capability);
    this.capabilitiesByConnection.set(connectionId, set);
  }

  /**
   * Unregister capability if owned by the same connection.
   */
  unregister(capability: string, connectionId: string): void {
    const entry = this.capabilityToProvider.get(capability);
    if (!entry) return;
    if (entry.connectionId !== connectionId) return;

    this.capabilityToProvider.delete(capability);
    const set = this.capabilitiesByConnection.get(connectionId);
    if (set) {
      set.delete(capability);
      if (set.size === 0) {
        this.capabilitiesByConnection.delete(connectionId);
      }
    }
  }

  /**
   * Lookup an active provider entry for a capability.
   */
  find(capability: string): ProviderEntry | undefined {
    return this.capabilityToProvider.get(capability);
  }

  /**
   * Cleanup all capabilities registered by a given connection id.
   * Invoked when a connection is closed/detached.
   */
  cleanupByConnection(connectionId: string): void {
    const set = this.capabilitiesByConnection.get(connectionId);
    if (!set) return;

    for (const cap of set) {
      const entry = this.capabilityToProvider.get(cap);
      if (entry && entry.connectionId === connectionId) {
        this.capabilityToProvider.delete(cap);
      }
    }
    this.capabilitiesByConnection.delete(connectionId);
  }
}
