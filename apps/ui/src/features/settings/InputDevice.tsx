import { Link } from "react-router";
import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { SettingField } from "~/components/shared/SettingField";
import { DesktopTest } from "./DesktopTest";
export function InputDevice() {
  const { t } = useTranslation();
  return <>
    <Section>
      <div className="settings-action-row">
        <SettingField setting="dpi" label="DPI" slider={{min:100,max:32000,step:100}}
          help={t("DPI 属于输入设备，在游戏间共享。原始计数不会在 FOV 换算中再次乘以 DPI。")} />
        <div className="actions"><Link to="/mouse-debug?section=characteristics">{t("鼠标报文")}</Link></div>
      </div>
    </Section>
    <DesktopTest />
  </>;
}

