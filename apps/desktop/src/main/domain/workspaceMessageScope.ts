import { workspaceCatalogSchema, workspaceContextSchema, sameWorkspaceContext, workspaceTopics,
    type WorkspaceCatalog, type WorkspaceContext } from '@mousebox/shared';

/** Message and cache provenance from native-issued context, independent of the process lease. */
export class WorkspaceMessageScope {
    private catalog?: WorkspaceCatalog;
    get context(): WorkspaceContext | undefined { return this.catalog?.active ?? undefined; }
    get known(): boolean { return this.catalog !== undefined; }
    update(value: unknown): boolean {
        const parsed = workspaceCatalogSchema.safeParse(value);
        if (!parsed.success) throw new Error('Invalid native workspace catalog');
        const old = this.context;
        this.catalog = parsed.data;
        return !sameWorkspaceContext(old, this.context) && !(old === undefined && this.context === undefined);
    }
    accepts(topic: string, value: unknown): boolean {
        if (!workspaceTopics.has(topic)) return true;
        if (!this.known) return value === undefined;
        const parsed = workspaceContextSchema.safeParse(value);
        return parsed.success && sameWorkspaceContext(this.context, parsed.data);
    }
}
