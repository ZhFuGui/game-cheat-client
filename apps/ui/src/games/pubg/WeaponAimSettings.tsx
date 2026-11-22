import { useTranslation } from "react-i18next";
import { useShallow } from "zustand/react/shallow";
import { SettingChoice } from "~/components/shared/SettingField";
import { Hint } from "~/components/shared/Content";
import { useRuntimeStore } from "~/runtime/state";

const parts = ["头部", "颈部", "胸部", "腹部", "骨盆"];
const categories = [
  { setting: "partSR", key: "SR", label: "狙击步枪（栓狙 / SR）" },
  { setting: "partDMR", key: "DMR", label: "精确射手步枪（连狙 / DMR）" },
  { setting: "partAR", key: "AR", label: "突击步枪 / AR" },
  { setting: "partSMG", key: "SMG", label: "冲锋枪 / SMG" },
  { setting: "partLMG", key: "LMG", label: "轻机枪 / LMG" },
  { setting: "partShotgun", key: "Shotgun", label: "霰弹枪" },
  { setting: "partHandgun", key: "Handgun", label: "手枪" },
  { setting: "partOther", key: "Other", label: "其他已识别武器" },
] as const;
const options = [{ label: "跟随默认", value: -1 }, ...parts.map((label, value) => ({ label, value }))];

export function WeaponAimSettings({ disabled }: { disabled: boolean }) {
  const { t } = useTranslation();
  const state = useRuntimeStore(useShallow(({ state }) => ({
    category: state.weaponAimCategory,
    part: state.effectiveAimPart,
    overridden: state.weaponAimOverride,
  })));
  const category = categories.find(entry => entry.key === state.category);
  return (
    <details className="weapon-aim-settings aim-full-row" open>
      <summary>{t("按枪械类别选择瞄准部位")}</summary>
      <div className="aim-fields">
        <Hint>
          {t("当前生效")}：{category ? t(category.label) : t("未识别枪械类别")} · {state.part === undefined ? "—" : t(parts[state.part])}
          {state.overridden ? "" : ` · ${t("跟随默认")}`}
        </Hint>
        {categories.map(category => (
          <SettingChoice key={category.key} setting={category.setting} label={category.label} options={options} disabled={disabled}  help={<> <p>{t("根据当前持枪自动选择，按游戏保存；未知枪械使用默认部位。修改后松开侧键再按。")}</p>{category.key==="SR"&&<p>{t("SR 按资料中的狙击步枪类别划分，也包含 Lynx AMR 和 Win94，并非全部为栓动结构。")}</p>} </>}/>
        ))}
      </div>
    </details>
  );
}
