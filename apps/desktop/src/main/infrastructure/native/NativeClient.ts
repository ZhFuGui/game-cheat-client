import { spawn, type ChildProcessWithoutNullStreams } from 'node:child_process';
import { randomUUID } from 'node:crypto';
import { EventEmitter } from 'node:events';
import type { WorkspaceContext } from '@mousebox/shared';
import type { NativePort, NativeLifecyclePort } from '../../domain/ports';
import { FramedJsonPipe } from '../transport/FramedJsonPipe';
import { NativeSession } from './NativeSession';

export class NativeClient extends EventEmitter implements NativePort, NativeLifecyclePort {
    private process?: ChildProcessWithoutNullStreams;
    private write?: (message: unknown) => boolean;
    private active = false;
    private stopping = false;
    private diagnostic = '';
    private captureToken?: string;
    private readonly session = new NativeSession(randomUUID(), (topic, data, context) => {
        this.emit('event', topic, data, context);
    });

    constructor(private readonly executable: string, private readonly dataDirectory: string, private importedSettings?: string) {
        super();
    }

    workspaceContext(): WorkspaceContext | null | undefined {
        return this.session.workspaceContext();
    }

    setInitialSettings(path: string): void {
        if (this.process) throw new Error('Native runtime is already started');
        this.importedSettings = path;
    }

    setCaptureToken(token: string): void {
        if (this.process) throw new Error('Native runtime is already started');
        this.captureToken = token;
    }

    start(): void {
        if (this.process) return;
        const args = ['bridge', '--data-dir', this.dataDirectory, '--parent-pid', String(process.pid)];
        if (this.importedSettings) args.push('--import-settings', this.importedSettings);
        if (this.captureToken) args.push('--capture-token', this.captureToken);
        const child = spawn(this.executable, args, {
            stdio: ['pipe', 'pipe', 'pipe'], windowsHide: true, env: { ...process.env },
        });
        this.process = child;
        const pipe = new FramedJsonPipe(child.stdout, child.stdin, {
            message: value => this.session.receive(value),
            failure: (error, invalidMessage) => {
                this.fail(error);
                if (invalidMessage) child.kill();
            },
        });
        this.write = value => pipe.write(value);
        child.stderr.on('data', (chunk: Buffer) => {
            const message = chunk.toString('utf8');
            this.diagnostic = (this.diagnostic + message).slice(-8192);
            this.emit('diagnostic', message.slice(0, 8192));
        });
        child.on('error', error => this.fail(error));
        child.on('exit', (code, signal) => {
            const detail = this.diagnostic ? ' · ' + this.diagnostic.trim() : '';
            this.fail(new Error(`Native runtime exited (${code ?? signal})${detail}`));
        });
    }

    request(command: string, payload: unknown = {}, timeout = 15000): Promise<unknown> {
        return this.session.request(this.write, command, payload, timeout);
    }

    async activate(): Promise<void> {
        if (this.stopping) throw new Error('Native runtime is shutting down');
        await this.request('session.hello');
        this.active = true;
    }

    async renew(): Promise<void> {
        if (this.active) await this.request('session.renew', {}, 2000);
    }

    async revoke(): Promise<void> {
        this.active = false;
        await this.request('session.revoke', {}, 2000);
    }

    async stop(): Promise<void> {
        if (!this.process) return;
        this.stopping = true;
        try {
            await this.revoke();
            await this.request('shutdown', {}, 4000);
        } catch {
            // EOF and parent death independently revoke native eligibility.
        }
        const child = this.process;
        child.stdin.end();
        if (child.exitCode === null) await new Promise<void>(resolve => {
            const timer = setTimeout(() => { child.kill(); resolve(); }, 1500);
            child.once('exit', () => { clearTimeout(timer); resolve(); });
        });
        this.process = undefined;
        this.write = undefined;
    }

    private fail(error: Error): void {
        if (!this.session.fail(error)) return;
        this.active = false;
        if (!this.stopping) this.emit('failure', error);
    }
}
