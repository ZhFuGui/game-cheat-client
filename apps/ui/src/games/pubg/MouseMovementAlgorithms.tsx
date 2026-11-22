import {mouseMovementDefinitions} from "@mousebox/shared";
import {MouseMovementAlgorithms} from "~/features/aim/common/MouseMovementAlgorithms";
import {MovementSpeedFields} from "~/features/aim/common/MovementSpeedFields";
import {useRuntimeStore} from "~/runtime/state";
const choices=[{...mouseMovementDefinitions.smooth,description:"保留当前PUBG的轨迹规划、目标跟踪与平滑计数规则；仅拆分移动模块，原有数值不变。"}];
export function PubgMouseMovementAlgorithms() {
  const notice=useRuntimeStore(s=>s.state.speedHint);
  return <MouseMovementAlgorithms choices={choices} defaultSelection="smooth">
    <MovementSpeedFields notice={notice}/>
  </MouseMovementAlgorithms>;
}
