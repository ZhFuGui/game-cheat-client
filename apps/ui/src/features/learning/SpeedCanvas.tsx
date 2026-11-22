import { useTranslation } from "react-i18next";
import { useRuntimeStore } from "~/runtime/state";
import { useNativeSurface } from "~/hooks/useNativeSurface";
export function SpeedCanvas() {
  const { t } = useTranslation(),
    ref = useNativeSurface("learning"),
    recording = useRuntimeStore((s) => s.learning?.recording),
    captureX = useRuntimeStore((s) => s.learning?.captureX ?? 0),
    captureY = useRuntimeStore((s) => s.learning?.captureY ?? 0);
  return (
    <div
      ref={ref}
      className={`speed-canvas ${recording ? "recording" : ""}`}
      aria-label={t("鼠标速度采集区域")}
      data-native-surface="learning"
    >
      <svg viewBox="0 0 1000 300" preserveAspectRatio="none" aria-hidden="true">
        <defs>
          <pattern
            id="speed-grid"
            width="40"
            height="40"
            patternUnits="userSpaceOnUse"
            patternTransform={`translate(${(-captureX * 0.3) % 40},${(-captureY * 0.3) % 40})`}
          >
            <path
              d="M 40 0 L 0 0 0 40"
              fill="none"
              stroke="var(--canvas-grid)"
              strokeWidth="1"
            />
          </pattern>
        </defs>
        <rect width="1000" height="300" fill="url(#speed-grid)" />
        <path
          d="M 490 150 H 510 M 500 140 V 160"
          stroke="var(--primary)"
          strokeWidth="1.5"
          vectorEffect="non-scaling-stroke"
        />
      </svg>
      <div className="capture-instruction">
        <span className={`status-dot ${recording ? "success" : ""}`} />
        {t(
          recording
            ? "自然移动鼠标，按 Esc 暂停并保存"
            : "点击“开始采集”后，在此区域自然移动鼠标",
        )}
      </div>
      <kbd className="capture-escape">Esc</kbd>
    </div>
  );
}
