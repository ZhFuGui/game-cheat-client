import {useTranslation} from "react-i18next";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingToggle,SettingField} from "~/components/shared/SettingField";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {ScoringControls} from './rage/ScoringControls';
import {ScoringDetails} from './rage/ScoringDetails';

export function Rage() {
  const {t}=useTranslation();
  return <div className="page rage-page"><div className="rage-layout aim-section-stack">
    <FeatureSection title="功能" className="rage-category">
      <div className="aim-fields">
        <SettingToggle setting="cs2RageEnabled" label="启用 Rage"
          help={t("启用时关闭 Aimbot 和 Triggerbot。前台检查遵循 Aim 常规设置。")}/>
        <div className="rage-policy"><span>{t("自瞄使用 Aim 常规设置")}</span><HelpTooltip label="自瞄使用 Aim 常规设置">{t("自瞄共用 Aim 常规中的后座方案、移动轨迹、速度变化控制、跟随速度、预测与后坐补偿、执行延迟、采样年龄、计数换算和已启用的鼠标模型。选人和瞄点按 Rage 的威胁与伤害策略，自动运行不需要按住侧键。")}</HelpTooltip></div>
      </div>
      <div className="rage-parameters aim-fields">
        <SettingField setting="cs2RageIntervalMs" label="连续点击最小间隔" suffix="ms"/>
        <SettingField setting="cs2RageHoldMs" label="左键按下时长" suffix="ms"
          help={t("当前固件支持5–40ms，按下后自动抬起。")}/>
      </div>
    </FeatureSection>
    <FeatureSection title="判断条件" className="rage-category">
      <div className="aim-fields">
        <SettingToggle setting="cs2RageCoverCheck" label="掩体判断"
          help={t("开启时只选择静态掩体可见的瞄点；关闭后按穿透静态掩体后的预计伤害选择大于0的瞄点。伤害未知或为0时不会开火。")}/>
        <SettingToggle setting="cs2RageOtherPlayerCheck" label="其他人物遮挡判断"
          help={t("开启时射线必须先命中所选目标；关闭时可先命中包含对象中的其他人物，仍需满足实际命中体的内缩要求。未包含对象挡在前方时不射击。")}/>
        <div className="rage-policy"><span>{t("按骨点伤害选择")}</span><HelpTooltip label="按骨点伤害选择">{t("每个已核验骨骼只分析其命中体的一个内部瞄点；射线先碰到同一目标的其他部位时，满足该部位的内缩要求即可开火。其他人物由遮挡判断开关控制。")}</HelpTooltip></div>
        <ScoringControls/>
      </div>
    </FeatureSection>
    <ScoringDetails/>
  </div></div>;
}
