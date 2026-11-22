import { dirname, resolve } from 'node:path';
import { startupOptions, automaticConnectionsEnabled } from './startup';
export function desktopLaunchOptions(args: string[]) {
    const isolated = args.find(a => a.startsWith('--data-dir='))?.slice('--data-dir='.length);
    const smokeReport = args.find(a => a.startsWith('--smoke-report='))?.slice('--smoke-report='.length);
    const startup = startupOptions(args);
    const legacyIsolated = startup.settings ? resolve(dirname(startup.settings), '.mousebox-web-' + Buffer.from(resolve(startup.settings)).toString('hex').slice(-24)) : undefined;
    return { args, startup, smokeReport, dataDirectory: isolated || legacyIsolated ? resolve(isolated || legacyIsolated!) : undefined,
        migrateLegacy: !isolated && !legacyIsolated,
        autoConnect: automaticConnectionsEnabled(startup, Boolean(isolated || legacyIsolated), Boolean(smokeReport), args),
        explicitlyIsolated: Boolean(isolated) };
}
export type DesktopLaunchOptions = ReturnType<typeof desktopLaunchOptions>;
