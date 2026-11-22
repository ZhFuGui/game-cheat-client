import {useTranslation} from "react-i18next";
import {Disclosure} from "~/components/shared/Disclosure";
import {SettingChoice, SettingField} from "~/components/shared/SettingField";

const distributions = [
  {value:"uniform", label:"体积均匀"},
  {value:"center", label:"偏中心"},
  {value:"outer", label:"偏外层"},
  {value:"middle", label:"中层带"},
];

export function HitboxAimSettings({disabled}: {disabled: boolean}) {
  const {t} = useTranslation();
  const title = t("Hitbox 瞄点");
  return <Disclosure label={title} heading={<span>{title}</span>} className="aim-subgroup aim-full-row">
    <div className="aim-fields">
      <SettingField setting="cs2AimPointIntervalMs" label="瞄点切换间隔" suffix="ms" disabled={disabled}
        help={<p>{t("按住跟随侧键时按间隔抽取新点。两次抽取之间，瞄点随当前 Hitbox 的位置、旋转和缩放移动。")}</p>}/>
      <SettingChoice setting="cs2AimPointDistribution" label="瞄点分布" options={distributions} disabled={disabled}
        help={<p>{t("体积均匀让等体积区域机会相同。偏中心和偏外层分别增加相应区域的机会。中层带在中心至内缩边界的 40%–80% 之间取点。")}</p>}/>
    </div>
  </Disclosure>;
}
