import { z } from "zod";
import { workspaceCatalogSchema } from "../models/workspaces";
import { command, empty } from "./common";
const id = z.string().min(1).max(128);
export const workspaceContracts = {
  "workspace.read": command(empty, workspaceCatalogSchema),
  "workspace.switch": command(z.object({ id }).strict(), workspaceCatalogSchema),
} as const;

