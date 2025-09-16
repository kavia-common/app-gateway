import { ProviderEntry } from './types';

/**
 * ProviderRegistry manages capability → provider mapping and cleans up by connection.
 */
export class ProviderRegistry {
  private capabilityToProvider: Map<string, ProviderEntry> = new Map();
  private capabilitiesByConnection: Map<string, Set<string>> = new Map();

  /**
   * Registers (or replaces) the provider for a capability.
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
   * Unregisters the provider for a capability if owned by the same connection.
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
   * Lookup provider by capability.
   */
  find(capability: string): ProviderEntry | undefined {
    return this.capabilityToProvider.get(capability);
  }

  /**
   * Cleanup all capabilities registered by a connection (e.g., on Detach).
   */
  cleanupByConnection(connectionId: string): void {
    const set = this.capabilitiesByConnection.get(connectionId);
    if (!set) return;
    for (const capability of set) {
      const entry = this.capabilityToProvider.get(capability);
      if (entry && entry.connectionId === connectionId) {
        this.capabilityToProvider.delete(capability);
      }
    }
    this.capabilitiesByConnection.delete(connectionId);
  }
}
