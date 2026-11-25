import { AppIcon } from "~/components/icons";
import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";

const formatRate = (bytes: number | undefined, language: string) =>
  bytes === undefined ? "—" : (bytes * 8 / 1000).toLocaleString(language, {
    minimumFractionDigits: 1, maximumFractionDigits: 1,
  });

export function ReaderTraffic() {
  const { t, i18n } = useTranslation();
  const upload = useRuntimeStore(s => s.connected ? s.state.readerMetrics?.uploadBytesPerSecond : undefined);
  const download = useRuntimeStore(s => s.connected ? s.state.readerMetrics?.downloadBytesPerSecond : undefined);
  const language = i18n.resolvedLanguage ?? "zh";
  return (
    <div className="reader-traffic" aria-label={t("Reader通信统计")}
      title={t("所有Reader TCP通道近1秒收发流量，含消息头和心跳；不含TCP/IP头及重传。1 kb/s = 1000 bit/s = 125字节/秒。")}>
      <span className="reader-rate" aria-label={t("Reader总上行")}>
        <AppIcon name="upload" size={11} />
        <span>{formatRate(upload, language)}</span> kb/s
      </span>
      <span aria-hidden="true">·</span>
      <span className="reader-rate" aria-label={t("Reader总下行")}>
        <AppIcon name="download" size={11} />
        <span>{formatRate(download, language)}</span> kb/s
      </span>
    </div>
  );
}
