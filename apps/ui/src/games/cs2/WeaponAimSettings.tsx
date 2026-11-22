import {useTranslation} from "react-i18next";
import {Disclosure} from "~/components/shared/Disclosure";
import {HelpTooltip} from "~/components/shared/HelpTooltip";
import {SettingChoice} from "~/components/shared/SettingField";

const categories = [
  {setting: "cs2AimPartPistol", label: "手枪"},
  {setting: "cs2AimPartRifle", label: "步枪"},
  {setting: "cs2AimPartSmg", label: "冲锋枪"},
  {setting: "cs2AimPartSniper", label: "狙击枪"},
] as const;
const options = [
  {value: -1, label: "沿用默认瞄准部位"},
  ...["头部", "颈部", "胸部", "腹部", "骨盆"].map((label, value) => ({label, value})),
];

export function WeaponAimSettings({disabled}: {disabled: boolean}) {
  const {t} = useTranslation();
  const title = "按枪械类别选择瞄准部位";
  return <Disclosure label={t(title)} className="weapon-aim-parts aim-subgroup aim-full-row" heading={
    <div className="field-label"><span>{t(title)}</span><HelpTooltip label={title}>
      <p>{t("根据当前手持武器自动切换：手枪默认头部，步枪和冲锋枪默认颈部，狙击枪默认胸部。狙击枪包含栓狙和连狙。其他或未识别枪械使用默认瞄准部位；也可为每类选择沿用默认。")}</p>
      <p>{t("切枪时保留同一锁定目标，并按新部位的 Hitbox 取点。命中体姿态缺失时暂停跟随。修改设置后需松开侧键再按。")}</p>
    </HelpTooltip></div>
  }><div className="aim-fields">{categories.map(category =>
    <SettingChoice key={category.setting} {...category} options={options} disabled={disabled}/>,
  )}</div></Disclosure>;
}
