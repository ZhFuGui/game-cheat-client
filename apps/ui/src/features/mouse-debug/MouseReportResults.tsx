import {HelpTooltip} from "~/components/shared/HelpTooltip";
import { useTranslation } from "react-i18next";
import type { MouseReportsResult } from "@mousebox/shared";
import { Section } from "~/components/shared/Content";
import { Button } from "~/components/ui/button";
import { Select } from "~/components/ui/select";
import {
  DistributionChart,
  ReportTimeline,
  formatReportNumber as format,
  type ReportChannel,
} from "./ReportCharts";

interface MouseReportResultsProps {
  result: MouseReportsResult;
  channel: ReportChannel;
  onChannelChange: (channel: ReportChannel) => void;
  busy: boolean | undefined;
  canExport: boolean | undefined;
  onExport: () => Promise<void>;
  onClear: () => unknown;
}

// Summary and chart interaction only consume completed, bounded result DTOs.
export function MouseReportResults({
  result,
  channel,
  onChannelChange,
  busy,
  canExport,
  onExport,
  onClear,
}: MouseReportResultsProps) {
  const { t } = useTranslation();
  const summary = result.summary;
  return (
    <>
      <div className="report-results-heading">
        <div className="field-label"><h2>{t("本次鼠标特征")}</h2><HelpTooltip label="本次鼠标特征"><p>{t(
          "单报文总计数 = √(dx² + dy²)。接收率含停顿时间；间隔使用程序接收时刻，受系统调度影响，不作为真实 USB 回报率结论。",
        )}</p></HelpTooltip></div>
        <div className="actions">
          <Button
            variant="outline"
            disabled={!canExport || busy}
            onClick={() => void onExport()}
          >
            {t("导出原始报文")}
          </Button><HelpTooltip label="导出原始报文"><p>{t(
          "原始 CSV 与统计 JSON 自动保存到 Mousebox 数据目录的 mouse-debug 文件夹；清空结果不删除已保存文件。",
        )}</p></HelpTooltip>
          <Button
            variant="ghost"
            disabled={busy}
            onClick={() => void onClear()}
          >
            {t("清空当前结果")}
          </Button>
        </div>
      </div>
      <dl className="report-summary">
        {[
          [
            "报文总数",
            format(summary.reports, 0),
            `${t("非零移动")} ${format(summary.movingReports, 0)} · ${t("零位移")} ${format(summary.zeroReports, 0)}`,
          ],
          ["单报文计数 P50", format(summary.p50Magnitude), t("计数")],
          ["单报文计数 P95", format(summary.p95Magnitude), t("计数")],
          ["单报文最大计数", format(summary.maxMagnitude), t("计数")],
          [
            "X / Y 单轴最大值",
            `${format(summary.maxAbsX, 0)} / ${format(summary.maxAbsY, 0)}`,
            t("计数"),
          ],
          ["接收间隔中位数", format(summary.medianIntervalMs, 3), "ms"],
          ["接收间隔 P95", format(summary.p95IntervalMs, 3), "ms"],
          ["全程平均接收率", format(summary.observedHz, 1), t("条/秒")],
        ].map(([label, value, note]) => (
          <div key={label}>
            <dt>{t(label)}</dt>
            <dd>{value}</dd>
            <small>{note}</small>
          </div>
        ))}
      </dl>
      
      {summary.reports > 0 ? (
        <>
          <div className="report-channel">
            <h2>{t("报文计数分析")}</h2>
            <Select
              label={t("分析方向")}
              value={channel}
              onChange={(value) => onChannelChange(value as ReportChannel)}
              options={[
                { value: "magnitude", label: t("总位移计数") },
                { value: "x", label: t("X 轴计数（向右为正）") },
                { value: "y", label: t("Y 轴计数（向下为正）") },
              ]}
            />
          </div>
          <Section title="报文计数随时间变化">
            <ReportTimeline timeline={result.timeline} channel={channel} />
          </Section>
          <div className="report-distributions">
            <Section title="单报文计数分布">
              <DistributionChart
                bins={result.histograms[channel]}
                label={t("单报文计数分布图")}
                unit={t("计数")}
              />
            </Section>
            <Section title="报文接收间隔分布">
              <DistributionChart
                bins={result.histograms.interval}
                label={t("报文接收间隔分布图")}
                unit="ms"
              />
            </Section>
          </div>
        </>
      ) : (
        <p className="empty-state">
          {t("本次未收到相对移动报文。重新开始后，移动连接本机的鼠标。")}
        </p>
      )}
      
      {result.savedPath && (
        <code className="report-saved-path" title={result.savedPath}>
          {result.savedPath}
        </code>
      )}
    </>
  );
}
