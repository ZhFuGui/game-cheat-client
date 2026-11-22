import {cs2ComparisonModes,type Cs2AimModeId} from "./aimModes";
import {mouseMovementDefinitions,type MouseMovementAlgorithm,type ExecutableMouseMovementAlgorithm} from "../../models/mouseMovement";

export const cs2MouseMovementAlgorithms=Object.values(mouseMovementDefinitions).map(definition=>definition.id==="historical"?{
  ...definition,description:"沿用所选版本的原有移动方式：v1/v4为平滑跟随，v2为后坐变化快速通道，v3为直接量化。旧配置默认使用此项。",
}:definition);
export function cs2MouseMovementSelection(mode:Cs2AimModeId,selection:MouseMovementAlgorithm) {
  const comparison=cs2ComparisonModes.some(item=>item.id===mode);
  const algorithm:ExecutableMouseMovementAlgorithm=comparison?"smooth":selection!=="historical"?selection:
    mode==="v2"?"recoil-tracking":mode==="v3"?"direct":"smooth";
  return {algorithm,comparison};
}
