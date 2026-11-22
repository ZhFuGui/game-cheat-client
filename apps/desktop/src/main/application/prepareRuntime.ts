import { app, nativeTheme } from 'electron';
import { access, readFile, writeFile, mkdir } from 'node:fs/promises';
import { existsSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { NativeClient } from '../infrastructure/native/NativeClient';
import { UiPreferenceStore } from '../infrastructure/storage/UiPreferences';
import { backupLegacy, findLegacySources, legacyOperationalSettings, legacyUiPreferences } from '../infrastructure/migration/LegacySettingsImporter';
import { copyLegacyAssets, migratePreviousDataDirectory } from '../infrastructure/migration/DataDirectoryMigration';
import type { LegacySource } from '../domain/ports';
export async function prepareRuntime(dataDirectory: string, executable: string, migrateLegacy = true) {
    await mkdir(dataDirectory, { recursive: true });
    if (migrateLegacy)
        await migratePreviousDataDirectory(process.env.LOCALAPPDATA || app.getPath('appData'), dataDirectory);
    const sources = migrateLegacy ? await findLegacySources(process.env.LOCALAPPDATA || app.getPath('appData')) : [];
    const settingsPath = join(dataDirectory, 'settings.json');
    let migrated = false;
    try {
        await access(join(dataDirectory, 'workspace-index.json'));
        migrated = true;
    }
    catch { try { await access(settingsPath); migrated = true; } catch { /* First launch. */ } }
    let imported: string | undefined;
    const legacyColorMode = nativeTheme.shouldUseDarkColors ? 'dark' : 'light';
    const preferences = new UiPreferenceStore(join(dataDirectory, 'web-ui.json'), legacyColorMode);
    const importSource = async (source: LegacySource) => {
        await backupLegacy(source, join(dataDirectory, 'migration-backup'));
        await copyLegacyAssets(dirname(source.path), dataDirectory);
        const ini = await readFile(source.path, 'utf8');
        const settings = legacyOperationalSettings(ini);
        imported = join(dataDirectory, 'migration-backup', 'imported-settings.json');
        await writeFile(imported, JSON.stringify(settings), 'utf8');
        if (!existsSync(join(dataDirectory, 'web-ui.json')))
            preferences.set(legacyUiPreferences(ini, legacyColorMode));
        return settings;
    };
    if (!migrated && sources.length === 1)
        await importSource(sources[0]);
    const native = new NativeClient(executable, dataDirectory, imported);
    return { native, preferences, sources, migrationRequired: !migrated && sources.length > 1, importSource };
}
export type PreparedRuntime = Awaited<ReturnType<typeof prepareRuntime>>;
