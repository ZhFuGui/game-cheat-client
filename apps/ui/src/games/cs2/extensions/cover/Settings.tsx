import { useTranslation } from "react-i18next";
import { cs2CoverEnabledSetting, cs2CoverExtensionId } from "@mousebox/shared";
import { SettingToggle } from "~/components/shared/SettingField";
import { Hint } from "~/components/shared/Content";
import { ExtensionStatus } from "~/features/extensions/ExtensionStatus";
import { useRuntimeStore } from "~/runtime/state";

export function CoverSettings() {
  const { t } = useTranslation();
  const map = useRuntimeStore(state => state.state.cs2Map);
  return <section aria-label={t("掩体检测扩展")}>
    <hr />
    <SettingToggle setting={cs2CoverEnabledSetting} label="启用掩体检测"  help={<> <p>{t("检测敌方骨点、骨段与已开启的命中体表面：可见为黄色，遮挡保留红色；未确定部分沿用原色。")}</p><p>{t("使用本地静态碰撞场景；玻璃按完整碰撞保留，动态门、破坏状态和烟雾不在检测范围内。")}</p> </>}/>
    <ExtensionStatus id={cs2CoverExtensionId} />
    <Hint>{t("当前地图")}：{map?.available ? map.name : "—"}</Hint>
  </section>;
}
