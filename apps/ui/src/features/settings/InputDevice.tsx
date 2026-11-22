import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { SettingField } from "~/components/shared/SettingField";
import { DesktopTest } from "./DesktopTest";
export function InputDevice() {
  const { t } = useTranslation();
  return <>
    <Section title="输入设备"><SettingField setting="dpi" label="DPI"  help={<> <p>{t("DPI 属于输入设备，在游戏间共享。原始计数不会在 FOV 换算中再次乘以 DPI。")}</p> </>}/><Link to="/mouse-debug?section=characteristics">{t("查看原始鼠标报文与回报率")}</Link></Section>
    <DesktopTest />
  </>;
}

