import { randomUUID } from 'node:crypto';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

type InputAddon = { dispatch(command: 'open' | 'inspect' | 'close', options?: string): string };
export type InputInspection = {
    processId: number;
    foregroundProcessId: number;
    active: boolean;
    failed: boolean;
    method: 'foreground-buffered-raw-input';
    rawReportsThroughJs: false;
};

/** Node opens a C++ endpoint once. No mouse reports or movement callbacks enter JS. */
export class ForegroundInput {
    readonly token = randomUUID();
    private readonly addon: InputAddon;
    private closed = false;
    constructor(executable: string) {
        const path = join(dirname(executable), 'mousebox-input.node');
        const requireNative = createRequire(join(dirname(executable), 'input-loader.cjs'));
        this.addon = requireNative(path) as InputAddon;
        const initial = JSON.parse(this.addon.dispatch('open', JSON.stringify({ token: this.token, executable }))) as InputInspection;
        if (initial.processId !== process.pid || initial.rawReportsThroughJs !== false || initial.method !== 'foreground-buffered-raw-input') {
            this.addon.dispatch('close');
            throw new Error('前台原生输入模块身份或版本不匹配');
        }
    }
    inspect(): InputInspection {
        if (this.closed)
            throw new Error('前台原生输入模块已关闭');
        return JSON.parse(this.addon.dispatch('inspect')) as InputInspection;
    }
    close(): void {
        if (this.closed)
            return;
        this.closed = true;
        this.addon.dispatch('close');
    }
}
