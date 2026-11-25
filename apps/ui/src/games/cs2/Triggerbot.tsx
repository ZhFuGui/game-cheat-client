import {useTranslation} from "react-i18next";
import {cs2TriggerHoldBounds} from "@mousebox/shared";
import {Disclosure} from "~/components/shared/Disclosure";
import {FeatureSection} from "~/components/shared/FeatureSection";
import {SettingToggle, SettingField} from "~/components/shared/SettingField";
import {SettingRangeField} from "~/components/shared/SettingRangeField";
import {useRuntimeStore} from "~/runtime/state";
import {HelpTooltip} from "~/components/shared/HelpTooltip";

export function Triggerbot() {
  const {t} = useTranslation();
  const calibrating = useRuntimeStore(s => s.state.calibrating === true);
  const cover = useRuntimeStore(s => s.values?.cs2TriggerCoverCheck === true);
  const reaction = useRuntimeStore(s => s.values?.cs2TriggerReactionEnabled === true);
  return <div className="page triggerbot-page">
    <div className="trigger-categories aim-section-stack">
      <FeatureSection title="功能" className="trigger-category">
        <Disclosure label={t("启用自动扳机")} className="trigger-feature trigger-activation aim-subgroup" heading={
            <SettingToggle setting="cs2TriggerEnabled" label="启用自动扳机" disabled={calibrating}
              help={t("启用前先关闭 Aimbot 和 Rage。启用后持续检测，不需要按住侧键；不会移动准星。前台检查遵循 Aim 常规设置。")}/>
        }>
          <div className="aim-fields">
            <SettingField setting="cs2TriggerIntervalMs" label="连续点击最小间隔" suffix="ms"
              help={t("填写 50–1000 ms 的整数。两次点击之间至少间隔所设时长。")}/>
            <SettingRangeField setting="cs2TriggerHoldRangeMs" label="左键按下时长" suffix="ms" slider={cs2TriggerHoldBounds}
              help={t("拖动两端或输入 5–40 ms 的整数。每次点击按所选分布抽取时长。")}/>
            <SettingField setting="cs2TriggerMaxAgeMs" label="扳机采样年龄上限" suffix="ms"/>
          </div>
        </Disclosure>
      </FeatureSection>
      <FeatureSection title="判断条件" className="trigger-category">
        <div className="aim-fields trigger-conditions">
          <Disclosure label={t("启用枪械类型")} className="trigger-feature trigger-weapon-types aim-subgroup" heading={
            <span className="field-label">{t("启用枪械类型")}<HelpTooltip label="启用枪械类型">{t("狙包括 AWP、SSG 08、G3SG1、SCAR-20；其他枪为其余已识别枪械。切枪后自动匹配，两类独立保存；刀、投掷物和未知物品不触发。")}</HelpTooltip></span>
          }>
            <div className="aim-fields">
              <SettingToggle setting="cs2TriggerSnipers" label="狙" disabled={calibrating}/>
              <SettingField setting="cs2TriggerSniperInaccuracyLimit" label="狙击枪 Inaccuracy 上限"
                help={t("仅狙击枪使用此判定。当前武器的完整 Inaccuracy 必须有效且严格小于设定值。可填 0 至 1，默认 0.004。")}/>
              <SettingToggle setting="cs2TriggerOtherGuns" label="其他枪" disabled={calibrating}/>
            </div>
          </Disclosure>
          <Disclosure label={t("掩体判断")} className="trigger-feature aim-subgroup" heading={
            <SettingToggle setting="cs2TriggerCoverCheck" label="掩体判断"
              help={t("开启后检查射击视点到 Hitbox 首个交点的静态掩体。路径遮挡或无法核验时暂停点击。关闭时暂停下方的反应时间判定。")}/>
          }>
            <Disclosure label={t("反应时间判定")} className="trigger-reaction aim-subgroup" heading={
              <SettingToggle setting="cs2TriggerReactionEnabled" label="反应时间判定" disabled={!cover}/>
            }>
              <SettingField setting="cs2TriggerReactionMs" label="反应时间" suffix="ms" disabled={!cover || !reaction}
                help={t("每个符合常规包含对象设置的目标至少有一个骨点可见时独立计时，全部被遮挡后重新计时。读取、传输和处理耗时计入，剩余等待最低为0ms。")}/>
            </Disclosure>
          </Disclosure>
          <div className="trigger-feature trigger-leaf" role="group" aria-label={t("自修正时间差")}>
            <SettingField setting="cs2TriggerTimeOffsetMs" label="自修正时间差" suffix="ms"
              help={t("仅用于扳机的命中预测。预测时长为采样年龄加预计执行延迟，再加自修正时间差，最低 0 ms。负值缩短预测时长，正值延长。可填 -100 至 100 ms 的整数，默认 0 ms。")}/>
          </div>
        </div>
      </FeatureSection>
    </div>
  </div>;
}
