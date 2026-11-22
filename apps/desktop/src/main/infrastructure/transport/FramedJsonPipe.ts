import type { Readable, Writable } from 'node:stream';
import { FrameCodec } from './FrameCodec';

type PipeHandlers = {
    message(value: unknown): void;
    failure(error: Error, invalidMessage: boolean): void;
};

/** A bounded framed JSON transport over any pair of Node streams. */
export class FramedJsonPipe {
    private readonly codec = new FrameCodec();
    private queuedBytes = 0;

    constructor(input: Readable, private readonly output: Writable, private readonly handlers: PipeHandlers,
                private readonly maximumQueuedBytes = 16 * 1024 * 1024) {
        input.on('data', (chunk: Buffer) => {
            try {
                for (const value of this.codec.push(chunk)) handlers.message(value);
            } catch (error) {
                handlers.failure(error instanceof Error ? error : new Error(String(error)), true);
            }
        });
        input.once('end', () => {
            try {
                this.codec.end();
            } catch (error) {
                handlers.failure(error instanceof Error ? error : new Error(String(error)), false);
            }
        });
        input.on('error', error => handlers.failure(error, false));
        output.on('error', error => handlers.failure(error, false));
    }

    /** Returns false when the local write budget is exhausted. */
    write(value: unknown): boolean {
        const frame = FrameCodec.encode(value);
        if (this.queuedBytes + frame.length > this.maximumQueuedBytes) return false;
        this.queuedBytes += frame.length;
        try {
            this.output.write(frame, error => {
                this.queuedBytes -= frame.length;
                if (error) this.handlers.failure(error, false);
            });
        } catch (error) {
            this.queuedBytes -= frame.length;
            throw error;
        }
        return true;
    }
}
