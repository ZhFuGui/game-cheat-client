import type { UiPreferences } from '@mousebox/shared';
type NativeSurface = {
    update(value: unknown): Promise<unknown>;
    hide(): Promise<void>;
    configure?(value:UiPreferences):Promise<void>;
};
/** Dialog occlusion and last requested bounds have one owner. */
export class SurfaceCoordinator {
    private previous?: Record<string, unknown>;
    private occluded = false;
    constructor(private readonly surface: NativeSurface) { }
    async configure(value:UiPreferences):Promise<void>{
        await this.surface.configure?.(value);
        if(this.previous) await this.update(this.previous);
    }
    update(payload: Record<string, unknown>): Promise<unknown> {
        this.previous = payload;
        return this.surface.update({ ...payload, occluded: this.occluded || payload.occluded });
    }
    async setOccluded(value: boolean): Promise<void> {
        this.occluded = value;
        if (value)
            await this.surface.hide();
        else if (this.previous)
            await this.surface.update(this.previous);
    }
    async reset(): Promise<void> {
        this.previous = undefined;
        await this.surface.hide();
    }
}
