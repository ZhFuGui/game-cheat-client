import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";
import { ReaderMetrics } from "~/features/reader-metrics/ReaderMetrics";
export function StatusBar() {
  const { t } = useTranslation();
  const connected = useRuntimeStore((s) => s.connected);
  const error = useRuntimeStore((s) => s.error);
  const connections = useRuntimeStore((s) => s.state.connections);
  const deviceConnections = useRuntimeStore((s) => s.deviceState.connections);
  const devicesProvided = useRuntimeStore((s) => s.devicesProvided);
  return (
    <footer className="statusbar" aria-label={t("连接状态")}>
      {([
        ["reader", "Reader"],
        ["serial", "串口接入"],
        ["box", "盒子连接"],
      ] as const).map(([key, label]) => {
        const indicator = connected ? key !== "reader" && devicesProvided ? deviceConnections?.[key] : connections?.[key] : undefined;
        const phase = connected ? indicator?.phase ?? "connecting" : error ? "error" : "connecting";
        const message = connected ? indicator?.message ?? t("正在读取连接状态") : error || t("正在连接原生运行时…");
        const description = `${t(label)}：${message}`;
        return (
          <span
            key={key}
            className="connection-indicator"
            data-link={key}
            data-phase={phase}
            role="img"
            aria-label={description}
            title={description}
          />
        );
      })}
      <ReaderMetrics />
    </footer>
  );
}
