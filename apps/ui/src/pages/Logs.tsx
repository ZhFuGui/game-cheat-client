import { useEffect, useMemo, useState } from "react";
import { useTranslation } from "react-i18next";
import { SearchInput } from "~/components/ui/input";
import { Button } from "~/components/ui/button";
import { EmptyState } from "~/components/shared/Content";
import { AppIcon } from "~/components/icons";
import { LogList } from "~/features/logs/LogList";
import { useCommand } from "~/hooks/useCommand";
import { useRuntimeStore } from "~/runtime/state";
import { Select } from "~/components/ui/select";
export default function Logs() {
  const { t } = useTranslation(),
    { run, pending } = useCommand(),
    entries = useRuntimeStore((s) => s.logs),
    catalog = useRuntimeStore(s => s.workspace),
    [query, setQuery] = useState(""),
    [scope, setScope] = useState("all");
  useEffect(() => {
    void run("logs.read", {
      after: useRuntimeStore.getState().logCursor,
      limit: 4000,
    }).then((v) => {
      if (v) useRuntimeStore.getState().appendLogs(v);
    });
  }, [run]);
  const filtered = useMemo(
    () =>
      entries.filter(
        (e) =>
          (scope === "all" || (scope === "global" ? !e.workspace : e.workspace?.workspaceId === scope)) && (!query ||
            e.message.toLocaleLowerCase().includes(query.toLocaleLowerCase())),
      ),
    [entries, query, scope],
  );
  const exportLogs = async () => {
    const result = await run("file.save", { purpose: "logs" });
    if (result?.path) await run("logs.export", { path: result.path });
  };
  return (
    <div className="page logs-page">
      <div className="logs-toolbar">
        <Select label={t("日志范围")} value={scope} onChange={value => setScope(String(value))} options={[{ value: "all", label: t("全部游戏") }, { value: "global", label: t("全局") }, ...(catalog?.workspaces.map(w => ({ value: w.id, label: w.name })) ?? [])]} />
        <SearchInput
          aria-label={t("搜索日志")}
          placeholder={t("搜索日志")}
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
        <small>
          {filtered.length} / {entries.length}
        </small>
        <div className="actions">
          <Button variant="ghost" disabled={!entries.length || pending} onClick={() =>
            void run("logs.clear", {}).then(v => {
              if (v) useRuntimeStore.setState({ logs: [], logCursor: "" });
            })} aria-label={t("清空日志")}>{t("清空")}</Button>
          <Button variant="outline" disabled={pending} onClick={() => void exportLogs()} aria-label={t("导出日志")}>
            <AppIcon name="download" size={15} />{t("导出")}
          </Button>
        </div>
      </div>
      {filtered.length ? (
        <LogList entries={filtered} />
      ) : (
        <EmptyState className="logs-empty" icon={entries.length ? "search" : "logs"} message={t(
            entries.length
              ? "没有匹配的日志"
              : "暂无日志",
          )} />
      )}
    </div>
  );
}
