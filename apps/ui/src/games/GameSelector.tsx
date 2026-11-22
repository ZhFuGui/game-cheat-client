import { useTranslation } from "react-i18next";
import { Select } from "~/components/ui/select";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
import { AppIcon } from "~/components/icons";

export function GameSelector() {
  const { t } = useTranslation(), { run, pending } = useCommand();
  const catalog = useRuntimeStore(s => s.workspace);
  const refreshing = useRuntimeStore(s => s.workspaceRefreshing);
  const revoked = useRuntimeStore(s => s.state.sessionEligible === false);
  const options = catalog?.games.filter(game => game.status === "available").flatMap(game => {
    const configuration = catalog.workspaces.find(entry => entry.gameId === game.id);
    return configuration ? [{ value: configuration.id, label: game.name }] : [];
  }) ?? [];
  return <div className="game-selector no-drag">
    <Select label={t("当前游戏")} value={revoked ? "" : catalog?.active?.workspaceId ?? ""} placeholder={t("请选择游戏")}
      leadingIcon={<AppIcon name="game" size={20} />} title={`${t("当前游戏")} · ${options.find(option => option.value === catalog?.active?.workspaceId)?.label ?? t("请选择游戏")}`}
      variant="menu" disabled={pending || refreshing || !options.length} options={options}
      onChange={id => { if (revoked || id !== catalog?.active?.workspaceId) void run("workspace.switch", { id: String(id) }); }} />
  </div>;
}
