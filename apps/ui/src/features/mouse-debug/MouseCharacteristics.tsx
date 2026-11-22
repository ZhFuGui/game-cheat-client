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
  idle: "尚未开始采集",
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
    preparing,
    prepared,
    visibleResult,
    readError,
    start,
  } = capture;
  return (
    <div className="mouse-characteristics">
      <div className="report-controls">
        <Button disabled={!state || busy} onClick={() => void start()}>
          {t("开始特征采集")}
        </Button><HelpTooltip label="开始特征采集"><p>{t(
            "开始后锁定第一只产生相对移动的鼠标；其他设备与绝对坐标输入不混入统计。结束、失焦或切页后恢复光标范围。",
          )}</p><p>{t(
            "来源为 Windows Raw Input。每条记录保留原始 dx/dy 和接收时间，不插值、不补零；它不等同于 USB 总线原始抓包。",
          )}</p></HelpTooltip>
        <span className="report-capture-status" role="status">
          <span
            className={`status-dot ${recording ? "success" : state?.analyzing ? "warning" : ""}`}
          />
          {t(
            state?.analyzing
              ? "正在统计原始报文…"
              : reasons[state?.reason || "idle"],
          )}
        </span>
        <span className="report-live-count">
          {format(state?.reports, 0)} {t("条报文")} ·{" "}
          {format((state?.elapsedMs ?? 0) / 1000, 1)} s
        </span>
      </div>
      <div
        ref={ref}
        className={`report-activity-area ${recording ? "recording" : ""}`}
        aria-label={t("鼠标特征活动区域")}
        data-native-surface="reports"
      >
        <div className="report-activity-label">{t("本机鼠标活动区域")}</div>
        <div className="report-activity-center">
          <strong>{t(recording ? "自由移动鼠标" : "准备观察你的鼠标")}</strong>
          <span>
            {t(
              recording
                ? "速度、方向和停顿都由你决定"
                : "开始后在此区域移动，随时结束查看统计",
            )}
          </span>
        </div>
        <div className="report-finish">
          <kbd>Esc</kbd>
          <Button
            variant="outline"
            disabled={!recording}
            onClick={() => void capture.stop()}
          >
            {t("结束采集")}
          </Button>
        </div>
      </div>
      
      <div className="report-device">
        <span>{t("采集设备")}</span>
        <code title={state?.device}>
          {state?.device || t("等待第一只移动的鼠标")}
        </code>
        <span>
          {t("忽略其他设备")} {format(state?.otherDeviceReports, 0)} ·{" "}
          {t("绝对坐标输入")} {format(state?.absoluteReports, 0)}
        </span>
      </div>
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
      {!visibleResult &&
        !preparing &&
        !prepared &&
        !recording &&
        !state?.analyzing &&
        !state?.hasResult && (
          <div className="report-empty">
            <h2>{t("从一条报文看鼠标如何移动")}</h2>
            <p>
              {t(
                "采集结束后可查看单报文计数分布、随时间变化的计数、接收间隔以及大位移峰值。无需连接 Reader 或盒子。",
              )}
            </p>
            <small>
              {t(
                "由你决定何时结束；最多保留200万条，达到容量时自动结束并统计。",
              )}
            </small>
          </div>
        )}
    </div>
  );
}
