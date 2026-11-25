import { Suspense } from "react";
import { useSearchParams } from "react-router";
import { useTranslation } from "react-i18next";
import { AppIcon } from "~/components/icons";
import { useRuntimeStore } from "~/runtime/state";
import { resolveGameSettings } from "~/games/registry";

export default function GameSettings() {
  const { t } = useTranslation(), [params, setParams] = useSearchParams();
  const catalog = useRuntimeStore(s => s.workspace);
  const game = catalog?.games.find(g => g.id === catalog.active?.gameId);
  const { panes, selected: pane } = resolveGameSettings(game?.id, game?.capabilities, params.get("section"));
  if (!pane) return <div className="workspace-empty"><h1>{t("当前游戏未提供此功能")}</h1></div>;
  const Pane = pane.component;
  return <div className="settings-page" data-single-pane={panes.length === 1 || undefined}>
    {panes.length > 1 && <aside className="settings-navigation">
      <nav aria-label={t("游戏设置分类")}>{panes.map(p => <button key={p.id} data-section={p.id} className={p.id === pane.id ? "active" : ""} aria-current={p.id === pane.id ? "page" : undefined} onClick={() => setParams({ section: p.id })}><AppIcon name="tuning" size={16} />{t(p.title)}</button>)}</nav>
    </aside>}
    <div className="settings-content" aria-label={t(pane.title)}><Suspense fallback={null}><Pane /></Suspense></div>
  </div>;
}

