import { z } from "zod";
import { mouseReportsResultSchema } from "../models/mouseReports";
import { empty, path, ok, command } from "./common";
export const mouseReportsContracts = {
    "mouseReports.start": command(empty, ok),
    "mouseReports.stop": command(empty, ok),
    "mouseReports.clear": command(empty, ok),
    "mouseReports.read": command(empty, z.object({ result: mouseReportsResultSchema.nullable() })),
    "mouseReports.export": command(z.object({ path }).strict(), ok),
} as const;
