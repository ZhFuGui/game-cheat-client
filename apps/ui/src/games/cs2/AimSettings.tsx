import { useTranslation } from "react-i18next";
import { FeatureSection } from "~/components/shared/FeatureSection";
import { SettingChoice, SettingField, SettingToggle } from "~/components/shared/SettingField";
import { useRuntimeStore } from "~/runtime/state";
import { WeaponAimSettings } from "./WeaponAimSettings";
import { HitboxAimSettings } from "./HitboxAimSettings";
import { Cs2MouseMovementAlgorithms } from "./MouseMovementAlgorithms";

export function AimSettings() {
  const { t } = useTranslation();
  const calibrating = useRuntimeStore(s => s.state.cs2Controller?.calibrating ?? false);
  return <><FeatureSection title="跟随设置" help={<p>{t("参数自动保存。按住后保持同一个人；短暂缺帧暂停，恢复继续；明确死亡后松开再按才能选人。")}</p>}><div className="aim-fields">
    <SettingChoice setting="cs2AimPart" label="默认瞄准部位" disabled={calibrating}
      options={["头部", "颈部", "胸部", "腹部", "骨盆"].map((label, value) => ({label, value}))}  help={<> <p>{t("在所选部位的 Hitbox 内取点，跟随需要该命中体的当前姿态。")}</p> </>}/>
    <SettingToggle setting="cs2AimCoverCheck" label="掩体判断" disabled={calibrating}
      help={<p>{t("按设定的分布在所选内缩 Hitbox 中找露点。抽到遮挡点时在同一部位重试，找到新点前继续使用仍露出的旧点。")}</p>} />
    <WeaponAimSettings disabled={calibrating}/>
    <HitboxAimSettings disabled={calibrating}/>
    <SettingChoice setting="cs2TargetPriority" label="目标优先级" disabled={calibrating}
      options={[{label:"人物距离优先",value:0},{label:"准星距离优先",value:1}]} />
    <SettingChoice setting="trigger" label="触发侧键" disabled={calibrating}
      options={[{label:"后退侧键",value:8},{label:"前进侧键",value:16}]} />
    <SettingField setting="cs2AimCone" label="最大瞄准角差" suffix="°" disabled={calibrating} />
    <SettingField setting="cs2AimDistanceUnits" label="最大目标距离（原生单位）" disabled={calibrating}
      help={<p>{t("0 表示不限距离，使用 Source 2 原生单位。初选限当前视野，目标队伍由 Aim 常规的包含对象设置决定。")}</p>}/>
  </div></FeatureSection>
  <Cs2MouseMovementAlgorithms />
  <FeatureSection title="后坐补偿">
    <div className="aim-fields">
      <SettingToggle setting="cs2RecoilCompensation" label="补偿发射后坐偏移" disabled={calibrating}
        help={<p>{t("按住跟随侧键即可补偿后坐。有目标时随目标调整。无目标时按手动瞄准的方向补偿，移动鼠标可以继续转向。此开关也用于 Rage。")}</p>}/>
    </div>
  </FeatureSection></>;
}
