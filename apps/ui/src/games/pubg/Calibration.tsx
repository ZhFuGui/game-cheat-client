import { useTranslation } from "react-i18next";
import { Section, Hint } from "~/components/shared/Content";
import { SettingChoice } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";

import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function Calibration() {
  const { t } = useTranslation(),
    { run, pending } = useCommand(),
    state = useRuntimeStore((s) => s.state),
    mode = useRuntimeStore((s) => s.values?.measurement),
    mapping=useRuntimeStore(s=>s.values?.mapping);
  const exportCalibration = async () => {
    const result = await run("file.save", { purpose: "calibration" });
    if (result?.path) await run("calibration.export", { path: result.path });
  };
  return (
    <>
      <details className="advanced-settings"><summary>{t('高级：备用换算与实测校准')}</summary><div className="advanced-settings-content">
      <Section title="输入换算">
        <SettingChoice setting="mapping" label="换算方式" disabled={state.calibrating} options={[{label:'实时 FOV 换算',value:0},{label:'仅匹配实测 FOV',value:1}]} help={<> <p>{t(mapping===0?'当前使用实时 FOV 公式，已保存的实测记录不参与跟随输出。':'当前仅使用与 FOV、灵敏度模式匹配的实测记录；缺失或匹配歧义时不输出。')}</p> </>}/>
      </Section>
      <Section title="实测校准">
        <div className="form-stack">
          <SettingChoice
            setting="measurement"
            label="本次校准视角"
            disabled={state.calibrating}
            options={["第三人称", "第一人称", "瞄准", "开镜"].map(
              (label, value) => ({ label, value }),
            )}
          />
          <div>
            <Button
              disabled={!state.canCalibrate || pending || mode === undefined}
              onClick={() =>
                mode !== undefined && void run("calibration.start", { mode })
              }
            >
              {t("实机校准 · 保持鼠标静止")}
            </Button>
          </div>
          <Hint>{state.calibration || "—"}</Hint>
          <Hint>{state.profiles || t("暂无实测记录")}</Hint>
          <div className="actions">
            <Button variant="outline" onClick={() => void exportCalibration()}>
              {t("导出校准记录")}
            </Button>
          </div>
        </div>
      </Section>
      </div></details>

    </>
  );
}
