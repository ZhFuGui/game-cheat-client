import { z } from "zod";
export const cs2ComparisonModes = [
  {id:"compare-global-direction",factor:"direction-reference",title:"角差参考改为全局输入角",change:"只替换控制角差基准为dwViewAngles；原生眼角资格、v1选人与响应观察不变，与v4整体移除眼角依赖不同。",condition:"两种角度实际不同时才有数值差异。",fields:["Local.input_view_angles / dwViewAngles","PlayerBody.eye_basis"],requires:[]},
  {id:"compare-no-p",factor:"predictable-recoil",title:"不计可预测后坐 P",change:"实际发射参考只关闭P分量；U、原生E规则、完整数据资格和v1选人不变。",condition:"后坐补偿开启且P分量非零时有效。",fields:["CCSPlayer_AimPunchServices::m_predictableBaseAngle / m_predictableBaseAngleVel"],requires:["recoil"]},
  {id:"compare-no-p-angle",factor:"predictable-base-angle",title:"P 基准角贡献置零",change:"只将实际P求值的基准角置零，保留角速度、时间、U与E规则；原始P仍用于基线资格和选人。",condition:"后坐补偿开启且P基准角非零时有效。",fields:["CCSPlayer_AimPunchServices::m_predictableBaseAngle"],requires:["recoil"]},
  {id:"compare-no-p-velocity",factor:"predictable-angular-velocity",title:"P 基准角速度贡献置零",change:"只将实际P求值的基准角速度置零，保留基准角、时间、U和原生E规则。",condition:"后坐补偿开启且P角速度对当前求值有贡献时有效。",fields:["CCSPlayer_AimPunchServices::m_predictableBaseAngleVel"],requires:["recoil"]},
  {id:"compare-no-u",factor:"unpredictable-recoil",title:"不计不可预测后坐 U",change:"实际发射参考只关闭U分量；P、完整数据资格和v1选人不变。",condition:"后坐补偿开启且U分量非零时有效。",fields:["CCSPlayer_AimPunchServices::m_unpredictableBaseAngle / m_unpredictableBaseTick"],requires:["recoil"]},
  {id:"compare-no-target-velocity",factor:"target-velocity",title:"不平移目标位置",change:"只关闭目标世界速度造成的位置提前量，自身运动、时间窗口和后坐外推不变。",condition:"速度预测开启且目标运动时有效。",fields:["Target.Entity.world_velocity"],requires:["prediction"]},
  {id:"compare-no-self-velocity",factor:"self-velocity",title:"不平移自身位置",change:"只关闭自身世界速度造成的眼部位置平移，目标运动、时间窗口和后坐外推不变。",condition:"速度预测开启且自身运动时有效。",fields:["LocalPawn.Entity.world_velocity"],requires:["prediction"]},
  {id:"compare-no-sample-age",factor:"sample-age-lead",title:"外推窗口不含采样年龄",change:"只从预测窗口去掉采样年龄；仍检查真实年龄是否过期，预计执行延迟和反馈账本规则不变。",condition:"样本有年龄，且运动预测或后坐外推会改变结果时有效。",fields:["CaptureInterval.started_qpc / read_finished_qpc"],requires:[]},
  {id:"compare-no-response-lead",factor:"response-delay-lead",title:"外推窗口不含预计执行延迟",change:"只从预测窗口去掉预计执行延迟；读取年龄保留，反馈账本仍使用原响应模型。",condition:"预计执行延迟大于0且预测会改变结果时有效。",fields:["cs2ResponseMs"],requires:["response"]},
  {id:"compare-no-recoil-lead",factor:"recoil-time-lead",title:"后坐只在采样时刻求值",change:"只将P/U求值时刻固定在本批simulation，不向执行时刻推进；敌我运动预测窗口不变。",condition:"后坐补偿开启且P/U随时间变化时有效。",fields:["AimPunchState.times.simulation"],requires:["recoil"]},
  {id:"compare-no-pending",factor:"pending-motion-prediction",title:"角差不扣待反馈位移",change:"只关闭角差中的待反映位移扣除；账本记录、超时观察、来源复核和设备约束照常运行。",condition:"有已接受但预计尚未反映到采样的位移时有效，静止无输出时应无差异。",fields:["PendingMotion.residual"],requires:["pending"]},
  {id:"compare-no-gain",factor:"tracking-gain-limit",title:"取消比例追赶上限",change:"只移除剩余计数乘比例增益的速度上限，保留速度、加减速和停止距离约束。",condition:"比例项是当前最紧约束时有效。",fields:["MotionPolicy.gain"],requires:[]},
  {id:"compare-no-acceleration",factor:"acceleration-limit",title:"取消加减速限幅",change:"只让速度直接达到本步需求值；比例、速度上限与停止距离仍保留。",condition:"启动、停止或转向时，加减速限幅原本生效才有差异。",fields:["MotionPolicy.acceleration"],requires:[]},
  {id:"compare-no-braking",factor:"braking-distance",title:"取消停止距离速度上限",change:"只移除sqrt(2×加速度×剩余计数)上限，保留比例、速度与加减速约束。",condition:"接近目标且停止距离约束原本生效时有效。",fields:["Kinematic.brakingDistance"],requires:[]},
  {id:"compare-no-variation",factor:"learned-variation",title:"关闭横向微扰",change:"只关闭HumanMotionRuntime的横向微扰；模型速度和加减速上限不变。",condition:"微扰开关开启、角差大于0.6°且剩余计数距离足够时有效。",fields:["HumanMotionRuntime.offset"],requires:["variation"]},
  {id:"compare-no-learned-limits",factor:"learned-motion-limits",title:"关闭学习模型运动上限",change:"只关闭模型速度、加速度和制动上限，保留设备/档位约束；微扰设置不变。",condition:"模型启用且其上限比设备/档位约束更紧时有效。",fields:["HumanMotionRuntime.limits"],requires:["model"]},
] as const;
export const cs2AimModeIds = ["v1","v2","v3","v4",...cs2ComparisonModes.map(m=>m.id)] as const;
export const cs2AimModeSchema = z.enum(cs2AimModeIds);
export type Cs2AimModeId = z.infer<typeof cs2AimModeSchema>;
