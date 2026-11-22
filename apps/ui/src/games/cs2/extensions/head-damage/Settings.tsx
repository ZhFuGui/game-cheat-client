import { useTranslation } from "react-i18next";
import { cs2HeadDamageEnabledSetting, cs2HeadDamageExtensionId } from "@mousebox/shared";
import { SettingToggle } from "~/components/shared/SettingField";
import { ExtensionStatus } from "~/features/extensions/ExtensionStatus";
export function HeadDamageSettings() {
  const { t } = useTranslation();
  return <section aria-label={t("头部伤害扩展")}>
    <hr />
    <SettingToggle setting={cs2HeadDamageEnabledSetting} label="显示预计头部伤害"  help={<> <p>{t("从角色视点到资源头部命中体的首次接触点，计入距离、穿透、爆头倍率、当前护甲和头盔。缺少完整姿态或目标数据时显示 —。")}</p><p>{t("≈ 表示默认服务器规则且目标非无敌条件下的估算，并非实际命中记录。扣血范围保留未知的小数伤害累计状态；不预测散布、其他玩家挡弹或服务器回溯。")}</p> </>}/>
    <ExtensionStatus id={cs2HeadDamageExtensionId} />
  </section>;
}
