import { acceptsGameSettings, isWorkspaceCommand, sameWorkspaceContext, type WorkspaceContext } from '@mousebox/shared';
import { WorkspaceMessageScope } from '../../domain/workspaceMessageScope';

type Pending = {
    resolve(value: unknown): void;
    reject(error: Error): void;
    timer: ReturnType<typeof setTimeout>;
    command: string;
    context?: WorkspaceContext;
};
type Envelope = {
    version: number;
    session: string;
    id?: string;
    ok?: boolean;
    result?: unknown;
    error?: string;
    topic?: string;
    data?: unknown;
    context?: WorkspaceContext;
};

export class NativeSession {
    private readonly pending = new Map<string, Pending>();
    private readonly workspace = new WorkspaceMessageScope();
    private serial = 0;
    private failure?: Error;

    constructor(private readonly session: string,
                private readonly event: (topic: string, data: unknown, context?: WorkspaceContext) => void) {}

    workspaceContext(): WorkspaceContext | null | undefined {
        return this.workspace.known ? this.workspace.context ?? null : undefined;
    }

    request(send: ((value: unknown) => boolean) | undefined, command: string, payload: unknown,
            timeout: number): Promise<unknown> {
        if (this.failure) return Promise.reject(this.failure);
        if (!send) return Promise.reject(new Error('Native runtime is not started'));
        if (this.pending.size >= 128) return Promise.reject(new Error('Native request queue is full'));
        const scoped = isWorkspaceCommand(command, payload);
        const context = scoped ? this.workspace.context : undefined;
        if (scoped && this.workspace.known && !context) return Promise.reject(new Error('No active workspace'));
        const id = String(++this.serial);
        const message = { version: 1, id, session: this.session, command, payload, ...(context ? { context } : {}) };
        return new Promise((resolve, reject) => {
            const timer = setTimeout(() => {
                this.take(id)?.reject(new Error(`Native command timeout: ${command}`));
            }, timeout);
            this.pending.set(id, { resolve, reject, timer, command, context });
            try {
                if (!send(message)) this.take(id)?.reject(new Error('Native write queue is full'));
            } catch (error) {
                this.take(id)?.reject(error instanceof Error ? error : new Error(String(error)));
            }
        });
    }

    receive(value: unknown): void {
        if (!value || typeof value !== 'object') throw new Error('Invalid native envelope');
        const message = value as Envelope;
        if (message.version !== 1 || message.session !== this.session) throw new Error('Native session mismatch');
        if (message.id) {
            this.reply(message.id, message);
            return;
        }
        if (typeof message.topic !== 'string') throw new Error('Invalid native response');
        if (message.topic === 'workspace') this.updateWorkspace(message.data);
        if (!this.workspace.accepts(message.topic, message.context)) return;
        if (message.topic === 'values' && !this.acceptsSettings(message.data))
            throw new Error('Game settings event does not match workspace');
        this.event(message.topic, message.data, message.context);
    }

    private reply(id: string, message: Envelope): void {
        const pending = this.take(id);
        if (!pending) return;
        if (pending.context && (!sameWorkspaceContext(pending.context, this.workspace.context) ||
                                !sameWorkspaceContext(pending.context, message.context))) {
            pending.reject(new Error('Workspace context changed'));
            return;
        }
        if (!message.ok) {
            pending.reject(new Error(message.error || 'Native command failed'));
            return;
        }
        try {
            if (message.result && typeof message.result === 'object') {
                const result = message.result as Record<string, unknown>;
                if ('workspace' in result) this.updateWorkspace(result.workspace);
                else if (pending.command.startsWith('workspace.')) this.updateWorkspace(result);
                if (Object.prototype.hasOwnProperty.call(result, 'values') && !this.acceptsSettings(result.values)) {
                    pending.reject(new Error('Game settings schema does not match workspace: ' + pending.command));
                    return;
                }
            }
            pending.resolve(message.result);
        } catch (error) {
            // The malformed reply was already removed from the queue.
            pending.reject(error instanceof Error ? error : new Error(String(error)));
            throw error;
        }
    }

    private acceptsSettings(value: unknown): boolean {
        const game = this.workspace.context?.gameId;
        return !game || acceptsGameSettings(game, value);
    }

    private take(id: string): Pending | undefined {
        const request = this.pending.get(id);
        if (request) {
            clearTimeout(request.timer);
            this.pending.delete(id);
        }
        return request;
    }

    private updateWorkspace(value: unknown): void {
        if (!this.workspace.update(value)) return;
        for (const [id, request] of this.pending) {
            if (request.context && !sameWorkspaceContext(request.context, this.workspace.context))
                this.take(id)?.reject(new Error('Workspace context changed'));
        }
    }

    fail(error: Error): boolean {
        if (this.failure) return false;
        this.failure = error;
        for (const id of this.pending.keys()) this.take(id)?.reject(error);
        return true;
    }
}
