import { useEffect } from "react";
import { useTranslation } from "react-i18next";
import { Section, Hint } from "~/components/shared/Content";
import { SettingChoice } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function Rendering() {
  const { t } = useTranslation(),
    { run } = useCommand(),
    adapters = useRuntimeStore((s) => s.graphicsAdapters),
    state = useRuntimeStore((s) => s.state);
  useEffect(() => {
    void run("graphics.refresh", {}).then((v) => {
      if (v) useRuntimeStore.setState({ graphicsAdapters: v });
    });
  }, [run]);
  return (
    <>
      <Section title="ESP绘制显卡" help={<> <p>{t("按数据到达更新，不设软件 FPS 上限。")}</p> </>}>
        <div className="form-stack">
          <SettingChoice
            setting="renderAdapter"
            label="选择显卡"
            options={adapters}
           help={<> <p>{t(
              "自动模式按系统高性能偏好优先尝试独显；手动选择会在绘制线程重建设备。",
            )}</p> </>}/>
          <p>{state.renderer || "—"}</p>
          <Hint>
            {state.adapterReady
              ? `${t("实际绘制设备")}：${state.adapter || "—"}`
              : t("打开ESP时创建所选绘制设备。")}
          </Hint>
          {state.adapterNotice && <p>{state.adapterNotice}</p>}
          {state.graphicsError && (
            <p className="field-error">{state.graphicsError}</p>
          )}
          <div>
            <Button
              variant="outline"
              onClick={() =>
                void run("graphics.refresh", {}).then((v) => {
                  if (v) useRuntimeStore.setState({ graphicsAdapters: v });
                })
              }
            >
              {t("重新检测显卡")}
            </Button>
          </div>
        </div>
      </Section>
      <div className="adapter-list">
        <h2>{t("已检测到的显卡")}</h2>
        {adapters
          .filter((a) => !a.automatic)
          .map((a) => (
            <div key={a.value}>
              <strong>{a.label}</strong>
              <Hint>{a.detail}</Hint>
            </div>
          ))}
      </div>
    </>
  );
}
