import { z } from "zod";
import { empty, path, ok, command } from "./common";
export const learningContracts = {
    "recording.load": command(z.object({ path }).strict(), ok),
    "learning.start": command(z.object({ dpi: z.number().int().min(100).max(32000) }).strict(), ok),
    "learning.pause": command(empty, ok),
    "learning.replay": command(empty, ok),
    "learning.clear": command(empty, ok),
    "learning.fit": command(empty, ok),
    "learning.setDpi": command(z.object({ dpi: z.number().int().min(100).max(32000) }).strict(), ok),
    "learning.setVariation": command(z.object({ variation: z.boolean() }).strict(), ok),
    "samples.import": command(z.object({ paths: z.array(path).min(1).max(256) }).strict(), ok),
    "samples.export": command(z.object({ path }).strict(), ok),
    "model.import": command(z.object({ path }).strict(), ok),
    "model.export": command(z.object({ path }).strict(), ok),
    "model.setActive": command(z.object({ active: z.boolean() }).strict(), ok),
    "model.requestDelete": command(empty, z.object({ token: z.string(), path: z.string() })),
    "model.confirmDelete": command(z.object({ token: z.string().min(1) }).strict(), ok),
} as const;
