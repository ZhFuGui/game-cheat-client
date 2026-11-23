import type { ReactNode } from "react";
import { useTranslation } from "react-i18next";
import type { GameCapability } from "@mousebox/shared";
import { useRuntimeStore } from "~/runtime/state";

export function WorkspaceRoute({ children, capability }: { children: ReactNode; capability?: GameCapability | readonly GameCapability[] }) {
  const { t } = useTranslation();
  const catalog = useRuntimeStore(s => s.workspace);
  const refreshing = useRuntimeStore(s => s.workspaceRefreshing);
  const game = catalog?.games.find(g => g.id === catalog.active?.gameId);
  const available = game?.status === "available";
  if (refreshing) return <div className="workspace-empty" role="status"><h1>{t("正在切换游戏…")}</h1><p>{t("等待当前游戏状态，旧数据已清除。")}</p></div>;
  if (!catalog?.active || !available) return <div className="workspace-empty"><h1>{t("请选择游戏")}</h1><p>{t(catalog ? "在左侧选择已接入的游戏，开始查看当前数据。" : "等待原生运行时提供游戏列表。")}</p></div>;
  const required = typeof capability === "string" ? [capability] : capability;
  if (required?.length && !required.some(value => game.capabilities.includes(value))) return <div className="workspace-empty"><h1>{t("当前游戏未提供此功能")}</h1><p>{t("请在左侧选择其他游戏，或使用当前游戏已支持的功能。")}</p></div>;
  return children;
}

