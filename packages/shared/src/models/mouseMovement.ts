import {z} from "zod";

export const mouseTrajectorySchema = z.enum(["linear", "bezier"]);
export const mouseTrajectorySettingsSchema = z.object({
  mouseTrajectory: mouseTrajectorySchema.default("linear"),
  mouseBezierControl1X: z.number().finite().min(0).max(100).default(25),
  mouseBezierControl1Y: z.number().finite().min(-100).max(100).default(20),
  mouseBezierControl2X: z.number().finite().min(0).max(100).default(75),
  mouseBezierControl2Y: z.number().finite().min(-100).max(100).default(20),
});

export const mouseMovementAlgorithmSchema=z.enum(["historical","smooth","recoil-tracking","direct"]);
export type MouseMovementAlgorithm=z.infer<typeof mouseMovementAlgorithmSchema>;
export type ExecutableMouseMovementAlgorithm=Exclude<MouseMovementAlgorithm,"historical">;
export interface MouseMovementDefinition {
  id:MouseMovementAlgorithm;
  title:string;
  description:string;
}
export const mouseMovementDefinitions:Readonly<Record<MouseMovementAlgorithm,MouseMovementDefinition>>={
  historical:{id:"historical",title:"沿用历史方案",description:"沿用此游戏所选方案原有的移动方式，保留旧配置的默认行为。"},
  smooth:{id:"smooth",title:"平滑加减速",description:"根据当前剩余误差、速度上限、加减速与停止距离调整速度，继续使用设备限制及已启用的学习模型。"},
  "recoil-tracking":{id:"recoil-tracking",title:"后坐优先响应",description:"对已经求出的后坐变化优先分配速度预算，其他剩余误差保持平滑加减速。首次或时间不连续时重建差分，未获资格时使用平滑控制。"},
  direct:{id:"direct",title:"直接响应",description:"按当前完整剩余误差直接生成计数，保留速度、学习模型速度上限和设备预算。"},
};
