import {z} from 'zod'
const count=z.number().int().min(0).max(2000000)
const finite=z.number().finite()
export const mouseReportsStateSchema=z.object({
  recording:z.boolean(),analyzing:z.boolean(),busy:z.boolean(),revision:z.string(),hasResult:z.boolean(),canExport:z.boolean(),
  reports:count,elapsedMs:finite.nonnegative(),otherDeviceReports:z.number().int().nonnegative(),absoluteReports:z.number().int().nonnegative(),
  device:z.string(),reason:z.enum(['idle','recording','manual','escape','focus','bounds','limit','error']),error:z.string(),
})
export type MouseReportsState=z.infer<typeof mouseReportsStateSchema>
const histogram=z.array(z.object({low:finite,high:finite,count})).max(64)
export const mouseReportsResultSchema=z.object({
  schemaVersion:z.literal(1),source:z.literal('windows-raw-input'),reportId:z.string(),device:z.string(),stopReason:z.string(),savedPath:z.string().nullable(),
  otherDeviceReports:z.number().int().nonnegative(),absoluteReports:z.number().int().nonnegative(),
  summary:z.object({reports:count,movingReports:count,zeroReports:count,durationMs:finite.nonnegative(),observedHz:finite.nullable(),
    sumX:finite,sumY:finite,pathCounts:finite,meanMagnitude:finite.nullable(),p50Magnitude:finite.nullable(),p95Magnitude:finite.nullable(),p99Magnitude:finite.nullable(),maxMagnitude:finite.nullable(),
    maxAbsX:finite,maxAbsY:finite,medianIntervalMs:finite.nullable(),p95IntervalMs:finite.nullable(),maxIntervalMs:finite.nullable(),zeroIntervals:count}),
  histograms:z.object({x:histogram,y:histogram,magnitude:histogram,interval:histogram}),
  timeline:z.array(z.object({startMs:finite,endMs:finite,reports:count,xMin:finite.nullable(),xMax:finite.nullable(),xMean:finite.nullable(),yMin:finite.nullable(),yMax:finite.nullable(),yMean:finite.nullable(),magnitudeMin:finite.nullable(),magnitudeMax:finite.nullable(),magnitudeMean:finite.nullable()})).max(512),
})
export type MouseReportsResult=z.infer<typeof mouseReportsResultSchema>
