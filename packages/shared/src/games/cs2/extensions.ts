import { z } from "zod";

export const cs2CoverExtensionId = "cs2.cover-detection";
export const cs2CoverEnabledSetting = "extension.cs2.cover-detection.enabled";
export const cs2RemainingDamageExtensionId = "cs2.remaining-damage";
export const cs2RemainingDamageEnabledSetting = "extension.cs2.remaining-damage.enabled";
export const cs2HeadDamageExtensionId = "cs2.head-damage";
export const cs2HeadDamageEnabledSetting = "extension.cs2.head-damage.enabled";
export const cs2BestDamageExtensionId = "cs2.best-damage";
export const cs2BestDamageEnabledSetting = "extension.cs2.best-damage.enabled";
export const cs2ExtensionSettingsSchema = z.object({
  [cs2CoverEnabledSetting]: z.boolean().default(false),
  [cs2RemainingDamageEnabledSetting]: z.boolean().default(false),
  [cs2HeadDamageEnabledSetting]: z.boolean().default(false),
  [cs2BestDamageEnabledSetting]: z.boolean().default(false),
});
