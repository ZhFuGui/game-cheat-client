import {useTranslation} from "react-i18next";
import {Section} from "~/components/shared/Content";
import {SettingToggle} from "~/components/shared/SettingField";

export default function VisualsSettings(){
  const {t}=useTranslation();
  return <div className="settings-stack valorant-visual-settings">
    <Section title="人物筛选" help={t("ESP 与 Radar 共用。仅使用 Reader 明确的本地身份及队伍关系；未分类不等于敌人，也不代表人机或死亡。") }>
      <div className="settings-grid">
        <SettingToggle setting="valorantShowSelf" label="本地人物"/>
        <SettingToggle setting="valorantShowAllies" label="友方人物"/>
        <SettingToggle setting="valorantShowEnemies" label="敌方人物"/>
        <SettingToggle setting="valorantShowUnknown" label="未分类人物"/>
      </div>
    </Section>
    <Section title="相机标记" help={t("相机与本地身体分开记录；死亡或观战时，相机不作为自己的身体。此开关控制 Radar 上的相机标记。") }>
      <SettingToggle setting="valorantShowCamera" label="显示相机位置"/>
    </Section>
  </div>;
}
