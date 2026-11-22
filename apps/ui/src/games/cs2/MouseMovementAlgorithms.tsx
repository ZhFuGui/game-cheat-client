import {mouseMovementDefinitions} from "@mousebox/shared";
import {MouseMovementAlgorithms} from "~/features/aim/common/MouseMovementAlgorithms";
import {MovementSpeedFields} from "~/features/aim/common/MovementSpeedFields";
import {useRuntimeStore} from "~/runtime/state";
export function Cs2MouseMovementAlgorithms() {
  const calibrating=useRuntimeStore(s=>s.state.cs2Controller?.calibrating??false);
  return <MouseMovementAlgorithms choices={[mouseMovementDefinitions.smooth,mouseMovementDefinitions["recoil-tracking"],mouseMovementDefinitions.direct]} defaultSelection="smooth"
    sharedConsumersHelp="Aimbot 与 Rage 共用移动轨迹、速度变化控制和速度档位。">
    <MovementSpeedFields disabled={calibrating}/>
  </MouseMovementAlgorithms>;
}
