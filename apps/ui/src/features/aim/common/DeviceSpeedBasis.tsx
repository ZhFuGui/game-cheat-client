import { useTranslation } from "react-i18next";
import { Hint } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

export function DeviceSpeedBasis() {
  const { t } = useTranslation();
  const connected = useRuntimeStore((s) => s.connected);
  const known = useRuntimeStore((s) => s.state.deviceCapabilitiesKnown);
  const baud = useRuntimeStore((s) => s.state.deviceBaudRate);
  const step = useRuntimeStore((s) => s.state.deviceMaximumStepCounts);
  const interval = useRuntimeStore((s) => s.state.deviceMinimumIntervalMs);
  return (
    <Hint>
      {!connected ? t("原生运行时未连接") : known
        ? t("当前链路 {{baud}} baud · 单轴 {{step}} 计数 / {{interval}} ms；档位按链路上限比例自动换算。", { baud, step, interval })
        : t("尚未识别盒子，显示 v5 参考上限；连接后按实际链路能力自动换算。")}
    </Hint>
  );
}
