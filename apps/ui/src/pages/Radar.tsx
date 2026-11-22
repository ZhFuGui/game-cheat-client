import { getGameUiModule } from "~/games/registry";
import { useRuntimeStore } from "~/runtime/state";

export default function Radar() {
  const game = useRuntimeStore(state => state.workspace?.active?.gameId);
  const Page = getGameUiModule(game)?.Radar;
  return Page ? <Page /> : null;
}
