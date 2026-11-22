import { z } from "zod";
import { bootstrapSchema } from "../models/bootstrap";
import { empty, path, ok, command } from "./common";
export const desktopContracts = {
    "clipboard.write": command(z.object({text:z.string().max(3*1024*1024)}).strict(),z.object({ok:z.literal(true)})),
    bootstrap: command(empty, bootstrapSchema),
    "window.minimize": command(empty, ok),
    "window.maximize": command(empty, ok),
    "window.close": command(empty, ok),
    "file.open": command(z
        .object({
        purpose: z.enum(["recording", "samples", "model"]),
        multiple: z.boolean().optional(),
    })
        .strict(), z.object({ paths: z.array(path) })),
    "file.save": command(z
        .object({ purpose: z.enum(["samples", "model", "logs", "calibration", "mouse-reports"]) })
        .strict(), z.object({ path: path.nullable() })),
} as const;
