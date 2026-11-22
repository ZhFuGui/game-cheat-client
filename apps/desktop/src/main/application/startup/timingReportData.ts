import { z } from 'zod';

const timestamp = z.string().regex(/^-?\d+$/);
const measurement = z.number().finite().nullable();
const clock = z.object({ source_anchor_ns: timestamp, local_anchor_ms: measurement, uncertainty_ms: measurement,
    calibrated_at_ms: timestamp, probe_age_ms: measurement }).nullable();
const interval = z.object({ source_start_ns: timestamp.nullable(), source_finish_ns: timestamp.nullable(),
    usable: z.boolean(), reason: z.number().int(), reason_name: z.string().max(40),
    qualified_age_ms: measurement, mapped_age_ms_unqualified: measurement, uncertainty_ms: measurement });
const observation = z.object({ present: z.boolean(), current: z.boolean(), status: measurement,
    capture: timestamp.nullable(), fields: z.record(z.string().regex(/^\d+$/), interval.extend({ capture: timestamp })) });
const frame = z.object({ current: z.boolean(), reader_instance: z.string().max(128), clock_domain: z.string().max(128),
    session: timestamp, epoch: timestamp, capture: timestamp,
    mapping_revision: timestamp, consumer_connection: timestamp, observed_at_ms: timestamp, observed_age_ms: measurement,
    characters: z.number().int().nonnegative(), clock, frame_interval: interval }).nullable();

// Strip every unknown property at every level. Full algorithm/runtime replies
// also contain names, addresses, vectors and angles and must never be persisted.
export const timingMetadataSchema = z.object({
    schema_version: z.literal(1), now_ms: timestamp, inspection: z.literal('fresh_readonly_inspection'),
    control_active: z.boolean(), held_target_present: z.boolean(),
    actual_feedback_reference: z.enum(['native_barrel_basis', 'camera']).nullable(),
    live_block: z.number().int(), ballistic_block: z.number().int(), timing_part: z.number().int(),
    timing_part_name: z.enum(['none', 'target', 'camera']), timing_reason: z.number().int().nullable(),
    timing_reason_name: z.string().max(40).nullable(),
    policy: z.object({ maximum_age_ms: z.number(), maximum_uncertainty_ms: z.number(), maximum_probe_age_ms: z.number(),
        maximum_target_velocity_skew_ms: z.number().finite().nonnegative().optional(),
        maximum_living_local_skew_ms: z.number().finite().nonnegative().optional() }),
    living: frame, local: frame,
    probe_rtt_ms: z.object({ living: measurement, local: measurement }),
    target: z.object({ clock, position: interval, velocity: interval.nullable() }).nullable(),
    rotation_sample: interval, camera_sample: interval,
    reader_telemetry: z.object({ controller_rotation: interval, recoil_targets: interval,
        aim_basis: observation, native_sway: observation, native_holding: observation }),
});
const metrics = z.object({ windowMs: z.number(), uploadBytesPerSecond: z.number(), downloadBytesPerSecond: z.number(),
    channels: z.array(z.object({ category: z.number().int(), connection: timestamp, name: z.enum(['Living', 'Local']),
        status: z.string().max(40), averageMs: measurement, uncertaintyMs: measurement,
        samples: z.number().int(), receivedFramesPerSecond: z.number().int() })) });
const runtime = z.object({ values: z.object({ hz: z.number() }), state: z.object({ readerMetrics: metrics }) });
const algorithm = z.object({ algorithm: z.object({ timing_metadata: timingMetadataSchema }) });

export function timingReportSample(algorithmReply: unknown, runtimeReply: unknown) {
    const metadata = algorithm.parse(algorithmReply).algorithm.timing_metadata;
    const state = runtime.parse(runtimeReply);
    return { metadata, requestedHz: state.values.hz, readerMetrics: state.state.readerMetrics };
}
