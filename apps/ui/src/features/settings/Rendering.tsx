import { useCallback, useEffect } from "react";
import { useTranslation } from "react-i18next";
import { Section, Hint } from "~/components/shared/Content";
import { SettingChoice } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function Rendering() {
  const { t } = useTranslation();
  const { run, pending } = useCommand();
  const adapters = useRuntimeStore(s => s.graphicsAdapters);
  const state = useRuntimeStore(s => s.state);
  const refresh = useCallback(async () => {
    const graphicsAdapters = await run("graphics.refresh", {});
    if (graphicsAdapters) useRuntimeStore.setState({ graphicsAdapters });
  }, [run]);
  useEffect(() => { void refresh(); }, [refresh]);
  const devices = adapters.filter(adapter => !adapter.automatic);
  return <Section>
    <div className="settings-action-row">
      <SettingChoice setting="renderAdapter" label="显卡" options={adapters} help={<>
        <p>{t("自动模式按系统高性能偏好优先尝试独显；手动选择会在绘制线程重建设备。")}</p>
        <p>{t("打开ESP时创建所选绘制设备。")}</p>
        <p>{t("按数据到达更新，不设软件 FPS 上限。")}</p>
      </>}/>
      <Button variant="outline" disabled={pending} onClick={() => void refresh()}>{t("重新检测")}</Button>
    </div>
    {(state.renderer || (state.adapterReady && state.adapter)) && <dl className="settings-facts">
      {state.renderer && <div><dt>{t("绘制引擎")}</dt><dd>{state.renderer}</dd></div>}
      {state.adapterReady && state.adapter && <div><dt>{t("实际设备")}</dt><dd>{state.adapter}</dd></div>}
    </dl>}
    {state.adapterNotice && <Hint>{state.adapterNotice}</Hint>}
    {state.graphicsError && <p className="field-error">{state.graphicsError}</p>}
    {devices.length > 0 && <details className="adapter-list">
      <summary>{t("设备详情")}</summary>
      {devices.map(device => <div key={device.value}>
        <strong>{device.label}</strong>
        {device.detail && <Hint>{device.detail}</Hint>}
      </div>)}
    </details>}
  </Section>;
}
