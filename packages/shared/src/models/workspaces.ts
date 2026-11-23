import { z } from "zod";

export const gameIdSchema = z.enum(["pubg", "cs2", "valorant", "gta5"]);
export const gameCapabilitySchema = z.enum(["esp", "aim", "recoil", "triggerbot", "rage", "input-calibration", "algorithm-details"]);
export const workspaceContextSchema = z.object({
  workspaceId: z.string().min(1).max(128),
  gameId: gameIdSchema,
  epoch: z.string().min(1).max(128),
}).strict();
export const workspaceSummarySchema = z.object({
  id: z.string().min(1).max(128),
  gameId: gameIdSchema,
  name: z.string().trim().min(1).max(80),
}).strict();
export const gameDefinitionSchema = z.object({
  id: gameIdSchema,
  name: z.string().min(1).max(80),
  status: z.enum(["available", "unavailable"]),
  reason: z.string().max(2048).optional(),
  capabilities: z.array(gameCapabilitySchema).max(16),
}).strict();
export const workspaceCatalogSchema = z.object({
  schemaVersion: z.literal(1),
  active: workspaceContextSchema.nullable(),
  workspaces: z.array(workspaceSummarySchema).max(256),
  games: z.array(gameDefinitionSchema).max(16),
}).strict().superRefine((catalog, ctx) => {
  if (new Set(catalog.workspaces.map(w => w.id)).size !== catalog.workspaces.length)
    ctx.addIssue({ code: z.ZodIssueCode.custom, message: "Duplicate workspace ID", path: ["workspaces"] });
  if (new Set(catalog.workspaces.map(w => w.gameId)).size !== catalog.workspaces.length)
    ctx.addIssue({code:z.ZodIssueCode.custom,message:"One configuration per game is required",path:["workspaces"]});
  if (new Set(catalog.games.map(g => g.id)).size !== catalog.games.length)
    ctx.addIssue({ code: z.ZodIssueCode.custom, message: "Duplicate game ID", path: ["games"] });
  const active = catalog.active;
  if (active && !catalog.workspaces.some(w => w.id === active.workspaceId && w.gameId === active.gameId))
    ctx.addIssue({ code: z.ZodIssueCode.custom, message: "Active workspace is absent from catalog", path: ["active"] });
});
export type GameId = z.infer<typeof gameIdSchema>;
export type GameCapability = z.infer<typeof gameCapabilitySchema>;
export type GameDefinition = z.infer<typeof gameDefinitionSchema>;
export type WorkspaceContext = z.infer<typeof workspaceContextSchema>;
export type WorkspaceSummary = z.infer<typeof workspaceSummarySchema>;
export type WorkspaceCatalog = z.infer<typeof workspaceCatalogSchema>;

/** Epoch is an opaque native identifier, never compared numerically or inferred from display names. */
export function sameWorkspaceContext(a: WorkspaceContext | null | undefined, b: WorkspaceContext | null | undefined): boolean {
  return !!a && !!b && a.workspaceId === b.workspaceId && a.gameId === b.gameId && a.epoch === b.epoch;
}
export const workspaceTopics = new Set<string>(["values", "espValues", "state", "learning", "mouseReports", "algorithm"]);
export function acceptsWorkspaceEvent(catalog: WorkspaceCatalog | undefined, topic: string, context?: WorkspaceContext): boolean {
  if (!workspaceTopics.has(topic)) return true;
  // Compatibility for legacy isolated fixtures only. A formal workspace always needs provenance.
  return !catalog ? context === undefined : sameWorkspaceContext(catalog.active, context);
}

/** One explicit scope rule is shared by the renderer, Electron and native bridge checks. */
export function isWorkspaceCommand(name: string, payload?: unknown): boolean {
  if (name === "surface.hide"||name==="surface.configure"||name==="surface.minimize") return false;
  if (name === "settings.set") {
    const key = payload && typeof payload === "object" && "key" in payload ? payload.key : undefined;
    return key !== "dpi" && key !== "renderAdapter";
  }
  return name === "settings.save" || name === "settings.import" ||
    ["reader.", "esp.", "calibration.", "learning.", "samples.", "model.", "recording.", "algorithm.", "mouseReports.", "surface.", "game."].some(prefix => name.startsWith(prefix));
}

