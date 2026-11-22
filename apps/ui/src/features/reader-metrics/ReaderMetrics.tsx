import { AppIcon } from "~/components/icons";
import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";

const formatRate = (bytes: number | undefined, language: string) =>
  bytes === undefined ? "—" : (bytes * 8 / 1000).toLocaleString(language, {
    minimumFractionDigits: 1, maximumFractionDigits: 1,
  });

export function ReaderMetrics() {
  const { t, i18n } = useTranslation();
  const connected = useRuntimeStore((s) => s.connected);
  const metrics = useRuntimeStore((s) => s.state.readerMetrics);
  const current = connected ? metrics : undefined;
  const language = i18n.resolvedLanguage ?? "zh";
  const explanation = t("近1秒平均：Reader开始读取这批数据 → GPU绘制完成通知；含线程唤醒延时，不含屏幕扫描显示。跨主机时间使用现有时钟校准估算。");
  return (
    <div className="reader-metrics" aria-label={t("Reader通信统计")}>
      <div className="reader-traffic" title={t("所有Reader TCP通道近1秒收发流量，含消息头和心跳；不含TCP/IP头及重传。1 kb/s = 1000 bit/s = 125字节/秒。")}>
        <span className="reader-rate" aria-label={t("Reader总上行")}>
          <AppIcon name="upload" size={11} />
          <span>{formatRate(current?.uploadBytesPerSecond, language)}</span> kb/s
        </span>
        <span aria-hidden="true">·</span>
        <span className="reader-rate" aria-label={t("Reader总下行")}>
          <AppIcon name="download" size={11} />
          <span>{formatRate(current?.downloadBytesPerSecond, language)}</span> kb/s
        </span>
      </div>
      <div className="reader-delays">
      {current?.channels.map((channel) => {
        const ready = current.gpuCompletionAvailable && channel.status === "ready" && channel.averageMs !== null;
        const reason = !current.gpuCompletionAvailable ? t("尚无GPU完成计时")
          : channel.status === "waiting-data" ? t("等待此通道当前数据")
          : channel.status === "waiting-clock" ? t("等待有效时钟校准")
          : t("近1秒没有完成绘制的新批次");
        const value = ready ? channel.averageMs!.toFixed(1) : "—";
        const detail = ready
          ? t("{{count}}批 · 时钟估算误差最多±{{error}} ms", {
              count: channel.samples, error: (channel.uncertaintyMs ?? 0).toFixed(1),
            }) : reason;
        return (
          <span
            key={`${channel.category}:${channel.connection}`}
            className="reader-channel-delay"
            aria-label={`${channel.name} ${t("采样到绘制延迟")}：${value} ms`}
            title={`${channel.name} TCP · ${detail}\n${explanation}\n${t("接收帧率：近1秒此TCP通道收到的有效新业务批次，排除心跳、时钟探测和重复基线。")}`}
          >
            {channel.name} <span>{value}</span> ms
            <span className="reader-receive-rate" aria-label={`${channel.name} ${t("接收帧率")}`}>
              · {channel.receivedFramesPerSecond} {t("帧/s")}
            </span>
          </span>
        );
      })}
      </div>
    </div>
  );
}
