import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";
import {useTranslation} from "react-i18next";

export default function Rage() {
  const {t}=useTranslation();
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const Panel=getGameUiModule(game)?.Rage;
  return Panel ? <Suspense fallback={null}><Panel/></Suspense> :
    <div className="page rage-page"><p>{t("当前游戏尚无经过核验的骨点伤害与自动射击支持。")}</p></div>;
}
