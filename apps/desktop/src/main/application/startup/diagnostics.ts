import { saveDiagnosticScreenshot, saveDiagnosticReport } from '../diagnostics';
import type { StartupTaskContext } from './context';
export async function runStartupDiagnostics({ startup, window, native, dataDirectory, version }: Pick<StartupTaskContext, 'startup' | 'window' | 'native' | 'dataDirectory' | 'version'>): Promise<void> {
    await new Promise(resolve => setTimeout(resolve, 1000));
    if (startup.screenshot) {
        await native.request('render.requestCapture');
        await new Promise(resolve => setTimeout(resolve, 800));
        await saveDiagnosticScreenshot(window, native, dataDirectory, startup.screenshot);
    }
    if (startup.renderReport)
        await saveDiagnosticReport(native, startup.renderReport, version);
    window.close();
}
