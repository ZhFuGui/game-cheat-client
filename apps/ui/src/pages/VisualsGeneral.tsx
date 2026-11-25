import { Suspense } from "react";
import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { EmptyState } from "~/components/shared/Content";
import { getGameUiModule } from "~/games/registry";
import { useRuntimeStore } from "~/runtime/state";

export default function VisualsGeneral(){
  const {t}=useTranslation();
  const game=useRuntimeStore(s=>s.workspace?.active?.gameId);
  const Settings=getGameUiModule(game)?.VisualsSettings;
  if (Settings) return <Suspense fallback={null}><Settings/></Suspense>;
  return <EmptyState className="visuals-general-empty" icon="tuning" message={t("当前游戏的显示选项在 ESP 页设置。")}
    action={<Link to="/visuals?section=esp">{t("打开 ESP")}</Link>} />;
}
