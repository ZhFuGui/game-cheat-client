import type { StartupTaskContext } from './startup/context';
import { scheduleStartupReport } from './startup/report';
import { scheduleConnectionReport } from './startup/connectionReport';
import { scheduleTimingReport } from './startup/timingReport';
import { importStartupFiles } from './startup/files';
import { runStartupCalibration } from './startup/calibration';
import { runStartupDiagnostics } from './startup/diagnostics';
export async function runStartupTasks(context: StartupTaskContext): Promise<void> {
    scheduleStartupReport(context);
    scheduleConnectionReport(context);
    scheduleTimingReport(context);
    await importStartupFiles(context);
    const failAndClose = (exitCode: number) => (error: unknown) => {
        console.error(error);
        process.exitCode = exitCode;
        context.window.close();
    };
    if (context.startup.calibrationPort)
        void runStartupCalibration(context).catch(failAndClose(3));
    if (context.startup.screenshot || context.startup.renderReport)
        void runStartupDiagnostics(context).catch(failAndClose(2));
}
