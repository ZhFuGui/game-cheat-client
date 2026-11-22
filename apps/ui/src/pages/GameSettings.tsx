import { Suspense } from "react";
import { useSearchParams } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { useRuntimeStore } from "~/runtime/state";
import { getGameUiModule } from "~/games/registry";

export default function GameSettings() {
  const { t } = useTranslation(), [params, setParams] = useSearchParams();
  const catalog = useRuntimeStore(s => s.workspace);
  const game = catalog?.games.find(g => g.id === catalog.active?.gameId);
  const gameModule = getGameUiModule(game?.id);
  const panes = gameModule?.settings.filter(p => !p.capability || game?.capabilities.includes(p.capability)) ?? [];
  const pane = panes.find(p => p.id === params.get("section")) ?? panes[0];
  if (!pane) return <div className="workspace-empty"><h1>{t("当前游戏未提供此功能")}</h1></div>;
  const Pane = pane.component;
  return <div className="settings-page">
    <aside className="settings-navigation"><span>{game?.name} · {t("游戏设置")}</span>
      <nav aria-label={t("游戏设置分类")}>{panes.map(p => <button key={p.id} data-section={p.id} className={p.id === pane.id ? "active" : ""} aria-current={p.id === pane.id ? "page" : undefined} onClick={() => setParams({ section: p.id })}><AppIcon name="tuning" size={16} />{t(p.title)}</button>)}</nav>
    </aside>
    <div className="settings-content" aria-label={t(pane.title)}><Suspense fallback={null}><Pane /></Suspense></div>
  </div>;
}

