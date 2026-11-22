import type { ContextMessage } from "./model";
export const CONTEXT_ROTATE_MS = 5200;
export interface ContextCursor { signature: string; index: number; }

/** Live projection, not an event queue: resolved issues cannot be replayed. */
export function contextCandidates(messages: readonly ContextMessage[]): ContextMessage[] {
  const unique = new Map<string, ContextMessage>();
  for (const message of messages) {
    const old = unique.get(message.id);
    if (!old || message.priority > old.priority) unique.set(message.id, message);
  }
  const sorted = [...unique.values()].sort((a, b) => b.priority - a.priority);
  const faults = sorted.filter(m => m.kind === "fault");
  if (faults.length) return faults.filter(m => m.priority === faults[0].priority);
  const activity = sorted.find(m => m.kind === "activity");
  return activity ? [activity] : sorted.slice(0, 12);
}
export function contextIndex(cursor: ContextCursor | undefined, signature: string, count: number): number {
  return count && cursor?.signature === signature ? cursor.index % count : 0;
}
export function nextContextCursor(cursor: ContextCursor | undefined, signature: string, count: number): ContextCursor {
  return { signature, index: count > 1 ? (contextIndex(cursor, signature, count) + 1) % count : 0 };
}
