import { useTranslation } from "react-i18next";
import { cs2RemainingDamageEnabledSetting, cs2RemainingDamageExtensionId } from "@mousebox/shared";
import { SettingToggle } from "~/components/shared/SettingField";
import { ExtensionStatus } from "~/features/extensions/ExtensionStatus";
export function RemainingDamageSettings() {
  const { t } = useTranslation();
  return <section aria-label={t("穿透余伤扩展")}>
    <hr />
    <SettingToggle setting={cs2RemainingDamageEnabledSetting} label="显示头部路径穿透余伤"  help={<> <p>{t("当前手持武器，从本批视点到敌方头部；优先使用资源命中体中心，缺姿态时使用本批 HEAD 骨点。")}</p><p>{t("显示每颗子弹或弹丸到达终点前的剩余基础伤害，包含距离衰减和静态掩体损耗；不含爆头倍率、护甲或实际扣血。")}</p><p>{t("未到达表示可靠判定停止；— 表示路径、材质或数据未确定。静态完整场景估算，尚未完成实机射击对照。")}</p> </>}/>
    <ExtensionStatus id={cs2RemainingDamageExtensionId} />
  </section>;
}
