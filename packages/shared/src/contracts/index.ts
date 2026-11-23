import { z } from "zod";
import { configurationContracts } from "./configuration";
import { connectionsContracts } from "./connections";
import { learningContracts } from "./learning";
import { mouseReportsContracts } from "./mouseReports";
import { diagnosticsContracts } from "./diagnostics";
import { renderingContracts } from "./rendering";
import { desktopContracts } from "./desktop";
import { workspaceContracts } from "./workspaces";
import { cs2ControllerContracts } from "../games/cs2/controller";
import { cs2RadarContracts } from "../games/cs2/radar";
import { valorantSourceContracts } from "../games/valorant/source";
import { valorantRadarContracts } from "../games/valorant/radar";
import type { WorkspaceContext } from "../models/workspaces";
export { surfaceSchema } from "./rendering";
export { eventSchemas } from "./events";
export type { Topic, EventPayload } from "./events";
import type { Topic, EventPayload } from "./events";
export const contracts = {
    ...configurationContracts,
    ...connectionsContracts,
    ...learningContracts,
    ...mouseReportsContracts,
    ...diagnosticsContracts,
    ...renderingContracts,
    ...desktopContracts,
    ...workspaceContracts,
    ...cs2ControllerContracts,
    ...cs2RadarContracts,
    ...valorantSourceContracts,
    ...valorantRadarContracts,
} as const;
export type CommandName = keyof typeof contracts;
export type CommandInput<K extends CommandName> = z.input<(typeof contracts)[K]["input"]>;
export type CommandResult<K extends CommandName> = z.output<(typeof contracts)[K]["output"]>;
export interface DesktopApi {
    invoke<K extends CommandName>(name: K, args: CommandInput<K>, context?: WorkspaceContext): Promise<CommandResult<K>>;
    subscribe<K extends Topic>(topic: K, listener: (payload: EventPayload<K>, context?: WorkspaceContext) => void): () => void;
}
