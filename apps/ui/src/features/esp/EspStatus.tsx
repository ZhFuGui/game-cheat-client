import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";

export function EspStatus() {
  const { t } = useTranslation();
  const timing = useRuntimeStore(state=>state.state.espTiming);
  const spectators = useRuntimeStore((state) => state.state.spectators);
  const connected = useRuntimeStore((state) => state.connected);
  const readerRunning = useRuntimeStore((state) => state.state.readerRunning);
  const localConnected = useRuntimeStore((state) => state.state.localConnected);
  const drawFrames = useRuntimeStore((state) => state.state.espDrawFramesPerSecond);
  const completionAvailable = useRuntimeStore((state) => state.state.readerMetrics?.gpuCompletionAvailable);
  const count = connected && readerRunning && localConnected ? spectators : null;
  const ready=connected && timing?.completionAvailable && timing.samples>0;
  const processing=ready&&timing.processingMs!==null?timing.processingMs.toFixed(2):"—";
  const drawing=ready&&timing.drawingMs!==null?timing.drawingMs.toFixed(2):"—";
  const cohort=t("近2秒平均，{{count}}个新数据帧；只计首次完成绘制，重复重绘不重复计数。",{count:ready?timing.samples:0});

  return (
    <div className="esp-status" aria-label={t("ESP 阶段耗时")}>
      <span data-esp-timing="processing" title={`${cohort}\n${t("完整接收 → 渲染线程开始本帧，包含解码、必要计算和排队；PUBG以Living人物帧为起点。")}`}>
        {t("处理")} <strong>{processing}</strong> ms
      </span>
      <span data-esp-timing="drawing" title={`${cohort}\n${t("渲染线程开始本帧 → 绘制完成通知，包含投影、图元生成、提交及执行；含完成通知唤醒延迟，不含屏幕扫描。")}`}>
        {t("绘制")} <strong>{drawing}</strong> ms
      </span>
      <span className="sr-only" aria-label={t("观看本地玩家人数")}>
        {t("观看本地玩家人数")}：
        <strong>{count == null ? "—" : t("{{count}} 人", { count })}</strong>
      </span>
      <span className="sr-only" aria-label={t("绘制帧率")}>
        {t("绘制")} <strong>{connected && completionAvailable ? drawFrames ?? "—" : "—"}</strong> FPS
      </span>
    </div>
  );
}
