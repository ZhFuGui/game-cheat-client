import { useTranslation } from "react-i18next";
import { FeatureSection } from "~/components/shared/FeatureSection";
import { SettingChoice, SettingField } from "~/components/shared/SettingField";
import { useRuntimeStore } from "~/runtime/state";
import { WeaponAimSettings } from "./WeaponAimSettings";
import { PubgMouseMovementAlgorithms } from "./MouseMovementAlgorithms";

export function FollowSettings() {
  const { t } = useTranslation();
  const calibrating = useRuntimeStore(store => store.state.calibrating);
  return <>
    <FeatureSection title="跟随设置">
      <div className="aim-fields">
        <SettingChoice
          setting="part"
          label="默认瞄准部位"
          options={["头部", "颈部", "胸部", "腹部", "骨盆"].map(
            (label, value) => ({ label, value }),
          )}
          disabled={calibrating}
        />
        <SettingChoice
          setting="trigger"
          label="触发侧键"
          options={[
            { label: "后退侧键", value: 8 },
            { label: "前进侧键", value: 16 },
          ]}
        />
        <WeaponAimSettings disabled={!!calibrating} />
        <SettingField
          setting="distance"
          label="自瞄距离上限"
          suffix="m"
          disabled={calibrating}
        />
        <SettingChoice
          setting="targetPriority"
          label="目标优先级"
          options={[
            { label: "人物距离优先", value: 0 },
            { label: "准星距离优先", value: 1 },
          ]}
          disabled={calibrating}
         help={<> <p>{t(
            "仅在合格目标中选择：按人物三维距离，或按所选部位与准星的角差排序；按住侧键后保持同一目标。",
          )}</p><p>{t("优先级自动保存，下次启动继续使用；修改后松开侧键再按。")}</p> </>}/>
      </div>
    </FeatureSection>
    <PubgMouseMovementAlgorithms />
  </>;
}
