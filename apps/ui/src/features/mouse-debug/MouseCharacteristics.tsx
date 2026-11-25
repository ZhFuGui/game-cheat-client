import {HelpTooltip} from "~/components/shared/HelpTooltip";
import { useState } from "react";
import { useTranslation } from "react-i18next";
import { Button } from "~/components/ui/button";
import {
  formatReportNumber as format,
  type ReportChannel,
} from "./ReportCharts";
import { MouseReportResults } from "./MouseReportResults";
import { useMouseReportCapture } from "./useMouseReportCapture";

const reasons: Record<string, string> = {
  recording: "采集中",
  manual: "已手动结束",
  escape: "已按 Esc 结束",
  focus: "窗口失焦，已结束",
  bounds: "活动区域改变，已结束",
  limit: "已达200万条容量上限，已结束",
  error: "采集异常，已结束",
};
export function MouseCharacteristics() {
  const { t } = useTranslation();
  const capture = useMouseReportCapture();
  const [channel, setChannel] = useState<ReportChannel>("magnitude");
  const {
    ref,
    state,
    recording,
    busy,
    visibleResult,
    readError,
    start,
  } = capture;
  const status = state?.analyzing ? "正在统计原始报文…" : reasons[state?.reason ?? ""];
  return (
    <div className="mouse-characteristics">
      <div className="report-controls">
        <Button disabled={!state || busy} onClick={() => void start()} aria-label={t("开始特征采集")}>
          {t("开始采集")}
        </Button><HelpTooltip label="开始采集"><p>{t(
            "开始后锁定第一只产生相对移动的鼠标；其他设备与绝对坐标输入不混入统计。结束、失焦或切页后恢复光标范围。",
          )}</p><p>{t(
            "来源为 Windows Raw Input。每条记录保留原始 dx/dy 和接收时间，不插值、不补零；它不等同于 USB 总线原始抓包。",
          )}</p><p>{t("由你决定何时结束；最多保留200万条，达到容量时自动结束并统计。")}</p></HelpTooltip>
        {status && <span className="report-capture-status" role="status">
          <span
            className={`status-dot ${recording ? "success" : state?.analyzing ? "warning" : ""}`}
          />
          <span className="report-status-text">{t(status)}</span>
        </span>}
        {(recording || state?.analyzing || state?.hasResult) && <span className="report-live-count">
          {format(state?.reports, 0)} {t("条报文")} ·{" "}
          {format((state?.elapsedMs ?? 0) / 1000, 1)} s
        </span>}
      </div>
      <div
        ref={ref}
        className={`report-activity-area ${recording ? "recording" : ""}`}
        aria-label={t("鼠标特征活动区域")}
        data-native-surface="reports"
      >
        <div className="report-activity-center">
          {t(recording ? "在此移动鼠标" : "开始采集后，在此移动鼠标")}
        </div>
        {recording && <div className="report-finish">
          <kbd>Esc</kbd>
          <Button
            variant="outline"
            onClick={() => void capture.stop()}
          >
            {t("结束采集")}
          </Button>
        </div>}
      </div>
      
      {(recording || state?.device) && <div className="report-device">
        <span>{t("采集设备")}</span>
        <code title={state?.device}>
          {state?.device || t("等待第一只移动的鼠标")}
        </code>
        <span>
          {t("忽略其他设备")} {format(state?.otherDeviceReports, 0)} ·{" "}
          {t("绝对坐标输入")} {format(state?.absoluteReports, 0)}
        </span>
      </div>}
      {state?.error && (
        <p className="field-error" role="alert">
          {t(state.error)}
        </p>
      )}
      {readError && (
        <p className="field-error" role="alert">
          {t("读取鼠标统计失败")}：{readError}{" "}
          <Button variant="ghost" onClick={() => capture.retryRead()}>
            {t("重试")}
          </Button>
        </p>
      )}
      {visibleResult && (
        <MouseReportResults
          result={visibleResult}
          channel={channel}
          onChannelChange={setChannel}
          busy={busy}
          canExport={state?.canExport}
          onExport={capture.exportRaw}
          onClear={capture.clear}
        />
      )}
    </div>
  );
}
