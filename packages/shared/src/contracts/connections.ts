import { z } from "zod";
import { portsSchema, graphicsAdaptersSchema } from "../models/devices";
import { empty, path, ok, command } from "./common";
export const connectionsContracts = {
    "reader.connect": command(z
        .object({
        host: z.string().trim().min(1),
        port: z.number().int().min(1).max(65535),
        hz: z.number().finite().min(1).max(300),
    })
        .strict(), ok),
    "reader.disconnect": command(empty, ok),
    "box.connect": command(z.object({ port: z.string().min(1).max(256) }).strict(), ok),
    "box.disconnect": command(empty, ok),
    "ports.refresh": command(empty, portsSchema),
    "graphics.refresh": command(empty, graphicsAdaptersSchema),
    "control.stop": command(empty, ok),
    "test.setArmed": command(z.object({ armed: z.boolean() }).strict(), ok),
    "calibration.start": command(z.object({ mode: z.number().int().min(0).max(3) }).strict(), ok),
    "calibration.export": command(z.object({ path }).strict(), ok),
} as const;
