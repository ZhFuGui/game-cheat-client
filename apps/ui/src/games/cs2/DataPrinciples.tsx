import { AlgorithmPanel } from "~/features/aimbot/AlgorithmPanel";

export function DataPrinciples() {
  return <AlgorithmPanel scope="aimbot" allowBallisticsReload={false} />;
}
export function MousePrinciples() {
  return <AlgorithmPanel scope="mouse" allowBallisticsReload={false} />;
}
