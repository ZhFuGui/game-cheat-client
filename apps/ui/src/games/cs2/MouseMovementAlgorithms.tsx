import {mouseMovementDefinitions} from "@mousebox/shared";
import {MouseMovementAlgorithms} from "~/features/aim/common/MouseMovementAlgorithms";
import {MovementSpeedFields} from "~/features/aim/common/MovementSpeedFields";
import {useRuntimeStore} from "~/runtime/state";
export function Cs2MouseMovementAlgorithms() {
  const calibrating=useRuntimeStore(s=>s.state.cs2Controller?.calibrating??false);
  return <MouseMovementAlgorithms choices={[mouseMovementDefinitions.smooth,mouseMovementDefinitions["recoil-tracking"],mouseMovementDefinitions.direct]} defaultSelection="smooth">
    <MovementSpeedFields disabled={calibrating}/>
  </MouseMovementAlgorithms>;
}
