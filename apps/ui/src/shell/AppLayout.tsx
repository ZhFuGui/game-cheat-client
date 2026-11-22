import { useEffect } from "react";
import { Outlet } from "react-router";
import { useTranslation } from "react-i18next";
import { Toaster } from "sonner";
import { Sidebar } from "./Sidebar";
import { TitleBar } from "./TitleBar";
import { StatusBar } from "./StatusBar";
import { useTheme } from "~/hooks/useTheme";
import { connectRuntime } from "~/runtime/connection";
import { useRuntimeStore } from "~/runtime/state";
import { Button } from "~/components/ui/button";
import { Dialog } from "~/components/ui/dialog";
import { useCommand } from "~/hooks/useCommand";
import {PageSessionProvider} from "~/framework/pages/PageSession";
import {PageTabPreferences} from "~/features/navigation/PageTabPreferences";
import {ApplicationMark} from "~/components/branding/ApplicationMark";
export function AppLayout() {
  useTheme();
  const { t } = useTranslation(),
    ready = useRuntimeStore((s) => s.ready),
    error = useRuntimeStore((s) => s.error),
    migration = useRuntimeStore((s) => s.migration),
    context = useRuntimeStore((s) => s.workspace?.active),
    { run } = useCommand();
  useEffect(() => {
    let dispose: (() => void) | undefined;
    let closed = false;
    void connectRuntime().then((stop) => {
      if (closed) stop();
      else dispose = stop;
    });
    return () => {
      closed = true;
      dispose?.();
    };
  }, []);
  return (
    <div className="app-layout">
      <Sidebar />
      <TitleBar />
      <main className="workspace" aria-label={t("内容")}>
        {error && (
          <div className="connection-error" role="alert">
            <span>{error}</span>
            <Button
              variant="ghost"
              size="sm"
              onClick={() => useRuntimeStore.getState().setError(null)}
            >
              {t("关闭提示")}
            </Button>
          </div>
        )}
        {ready ? (
          <PageTabPreferences><div className="workspace-outlet" data-workspace-epoch={context?.epoch ?? ""} key={context ? `${context.workspaceId}:${context.epoch}` : "no-workspace"}><PageSessionProvider><Outlet /></PageSessionProvider></div></PageTabPreferences>
        ) : (
          <div className="startup-state">
            <ApplicationMark className="startup-mark"/>
            <h1>mousebox-aimbot</h1>
            <p>{t(error ? "无法连接原生运行时" : "正在连接原生运行时…")}</p>
            {error && (
              <Button variant="outline" onClick={() => void connectRuntime()}>
                {t("重试")}
              </Button>
            )}
          </div>
        )}
      </main>
      <StatusBar />
      <Toaster richColors position="bottom-right" />
      <Dialog
        open={!!migration?.required}
        onOpenChange={() => {}}
        title={t("选择旧配置来源")}
      >
        <p>{t("发现不同的旧配置。请选择要迁移的来源，原始文件将保留。")}</p>
        <div className="migration-list">
          {migration?.sources.map((source) => (
            <Button
              key={source.id}
              variant="outline"
              onClick={() =>
                void run("migration.choose", { id: source.id }).then((v) => {
                  if (v) void connectRuntime();
                })
              }
            >
              <span>{source.path}</span>
              {source.modifiedAt && <small>{source.modifiedAt}</small>}
            </Button>
          ))}
        </div>
      </Dialog>
    </div>
  );
}
