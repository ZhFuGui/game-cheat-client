import {Suspense} from "react";
import {getGameUiModule} from "~/games/registry";
import {useRuntimeStore} from "~/runtime/state";
import {useTranslation} from "react-i18next";

export default function Triggerbot() {
  const {t}=useTranslation();
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const Panel=getGameUiModule(game)?.Triggerbot;
  if(Panel)return <Suspense fallback={null}><Panel/></Suspense>;
  return (
    <div className="page triggerbot-page">
      <p className="muted">{t("当前游戏尚无经过核验的 Hitbox 检测与自动点击支持。")}</p>
    </div>
  );
}
