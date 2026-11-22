import type { BrowserWindow } from 'electron';
import type { NativeLifecyclePort } from '../domain/ports';
export function manageLifecycle(window: BrowserWindow, native: NativeLifecyclePort, hide: () => Promise<void>) {
    let pulse = 0, revoked = true, stopping = false, allowClose = false;
    const heartbeat = setInterval(() => {
        if (stopping)
            return;
        if (Date.now() - pulse > 2000) {
            if (!revoked) {
                revoked = true;
                void native.revoke().catch(() => { });
            }
            return;
        }
        if (!revoked)
            void native.renew().catch(() => { revoked = true; });
    }, 500);
    window.webContents.on('render-process-gone', () => { revoked = true; void native.revoke().catch(() => { }); });
    window.on('unresponsive', () => { revoked = true; void native.revoke().catch(() => { }); });
    window.on('minimize', () => { void hide().catch(() => { }); });
    window.on('close', event => {
        if (allowClose)
            return;
        event.preventDefault();
        if (stopping)
            return;
        stopping = true;
        clearInterval(heartbeat);
        void native.stop().finally(() => { allowClose = true; window.destroy(); });
    });
    window.on('closed', () => clearInterval(heartbeat));
    return { pulse: () => { pulse = Date.now(); }, activated: () => { if (!stopping) { pulse = Date.now(); revoked = false; } } };
}
