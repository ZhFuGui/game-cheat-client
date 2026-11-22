import {z} from "zod";

export const randomDistributions = ["normal", "uniform", "triangular", "lower", "upper"] as const;
export const randomDistributionSchema = z.enum(randomDistributions);
export type RandomDistribution = z.infer<typeof randomDistributionSchema>;

export function randomRangeSchema(minimum: number, maximum: number) {
  const endpoint = z.number().int().min(minimum).max(maximum);
  return z.object({
    min: endpoint,
    max: endpoint,
    distribution: randomDistributionSchema.default("normal"),
  }).strict().refine(range => range.min <= range.max);
}

export type RandomRange = z.infer<ReturnType<typeof randomRangeSchema>>;
