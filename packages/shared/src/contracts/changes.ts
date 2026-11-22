import { z } from "zod";
import { settingValidators } from "../models/settings";
import { espSettingsSchema } from "../models/esp";
export const settingChange = z
    .object({
    key: z.enum(Object.keys(settingValidators) as [
        keyof typeof settingValidators,
        ...(keyof typeof settingValidators)[]
    ]),
    value: z.unknown(),
})
    .strict()
    .superRefine((v, c) => {
    const result = settingValidators[v.key].safeParse(v.value);
    if (!result.success)
        c.addIssue({
            code: "custom",
            path: ["value"],
            message: result.error.issues.map((i) => i.message).join("; "),
        });
});
export const espChange = z
    .object({
    key: z.enum(Object.keys(espSettingsSchema.shape) as [
        keyof typeof espSettingsSchema.shape,
        ...(keyof typeof espSettingsSchema.shape)[]
    ]),
    value: z.unknown(),
})
    .strict()
    .superRefine((v, c) => {
    const result = espSettingsSchema.shape[v.key].safeParse(v.value);
    if (!result.success)
        c.addIssue({
            code: "custom",
            path: ["value"],
            message: "Invalid ESP setting",
        });
});
