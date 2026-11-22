import { useTranslation } from "react-i18next";
import { cs2BestDamageEnabledSetting, cs2BestDamageExtensionId } from "@mousebox/shared";
import { SettingToggle } from "~/components/shared/SettingField";
import { ExtensionStatus } from "~/features/extensions/ExtensionStatus";
export function BestDamageSettings() {
  const { t } = useTranslation();
  return <section aria-label={t("最高伤害瞄准点扩展")}>
    <hr />
    <SettingToggle setting={cs2BestDamageEnabledSetting} label="显示最高伤害瞄准点"  help={<> <p>{t("按未封顶伤害搜索敌人的命中体，计入静态掩体、距离、命中部位、护甲和头盔。仅用青色圈与十字标出具体瞄准点。")}</p><p>{t("计算独立刷新，绘制沿用最近完成的结果；伤害允许短暂滞后，瞄点跟随当前人物姿态。")}</p><p>{t("搜索最佳表示当前预算内找到的最高值；已收敛表示在当前模型内距伤害上界不超过 0.1。未知路径不会按零伤害处理，不自动瞄准或开火。")}</p><p>{t("≈ 仍以默认服务器规则且目标非无敌为条件；不预测散布、其他玩家挡弹、动态掩体或服务器回溯。")}</p> </>}/>
    <ExtensionStatus id={cs2BestDamageExtensionId} />
  </section>;
}
