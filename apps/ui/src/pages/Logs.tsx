import { useEffect, useMemo, useState } from "react";
import { useTranslation } from "react-i18next";
import { Input } from "~/components/ui/input";
import { Button } from "~/components/ui/button";
import { LogList } from "~/features/logs/LogList";
import { useCommand } from "~/hooks/useCommand";
import { useRuntimeStore } from "~/runtime/state";
import { Select } from "~/components/ui/select";
export default function Logs() {
  const { t } = useTranslation(),
    { run } = useCommand(),
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
        <Input
          aria-label={t("搜索日志")}
          placeholder={t("搜索日志")}
          value={query}
          onChange={(e) => setQuery(e.target.value)}
        />
        <small>
          {filtered.length} / {entries.length}
        </small>
        <div className="actions">
          <Button variant="outline" onClick={() =>
            void run("logs.clear", {}).then(v => {
              if (v) useRuntimeStore.setState({ logs: [], logCursor: "" });
            })}>{t("清空日志")}</Button>
          <Button variant="outline" onClick={() => void exportLogs()}>{t("导出日志")}</Button>
        </div>
      </div>
      {filtered.length ? (
        <LogList entries={filtered} />
      ) : (
        <div className="logs-empty">
          {t(
            entries.length
              ? "没有匹配的日志"
              : "暂无日志。连接 Reader 或盒子后会在这里记录状态。",
          )}
        </div>
      )}
    </div>
  );
}
