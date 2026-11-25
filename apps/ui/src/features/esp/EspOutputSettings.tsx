import {HelpTooltip} from "~/components/shared/HelpTooltip";
import { useTranslation } from "react-i18next";
import { Select } from "~/components/ui/select";
import { useCommand } from "~/hooks/useCommand";
import { useRuntimeStore } from "~/runtime/state";
export function EspOutputSettings() {
  const {t}=useTranslation(),{run,pending}=useCommand();
  const prefs=useRuntimeStore(s=>s.preferences);
  const placement=prefs.espPlacement??"embedded";
  return <section className="esp-output-settings" aria-label={t("ESP 绘制方式")}>
    <div className="esp-output-options">
      <div className="field"><div className="field-label"><span>{t("绘制区域")}</span><HelpTooltip label="绘制区域"><p>{t("叠加层只对齐本机前台游戏窗口，鼠标操作穿透；切走游戏后隐藏。适用于窗口化或无边框窗口。")}</p></HelpTooltip></div><Select label={t("绘制区域")} value={placement} disabled={pending}
        onChange={value=>void run("preferences.set",{espPlacement:value as "embedded"|"overlay"})}
        options={[{value:"embedded",label:t("内嵌画布")},{value:"overlay",label:t("游戏窗口叠加层")}]} /></div>
      <div className="field"><div className="field-label"><span>{t("截图捕获")}</span><HelpTooltip label="截图捕获"><p>{t("截图策略仅对叠加层生效，由 Windows 请求排除捕获，效果取决于截图或录屏方式。软件绘制使用 CPU，桌面仍由系统合成。")}</p></HelpTooltip></div><Select label={t("截图捕获")} value={prefs.espCapture??"include"} disabled={pending||placement!=="overlay"}
        onChange={value=>void run("preferences.set",{espCapture:value as "include"|"exclude"})}
        options={[{value:"include",label:t("允许截图捕获")},{value:"exclude",label:t("请求排除截图捕获")}]} /></div>
      <div className="field"><div className="field-label"><span>{t("绘制引擎")}</span><HelpTooltip label="绘制引擎"><p>{t("截图策略仅对叠加层生效，由 Windows 请求排除捕获，效果取决于截图或录屏方式。软件绘制使用 CPU，桌面仍由系统合成。")}</p></HelpTooltip></div><Select label={t("绘制引擎")} value={prefs.espRenderer??"default"} disabled={pending}
        onChange={value=>void run("preferences.set",{espRenderer:value as "default"|"software"})}
        options={[{value:"default",label:t("跟随显卡设置")},{value:"software",label:t("软件绘制（CPU）")}]} /></div>
    </div>
    
    
  </section>;
}
export function EspOverlayPlaceholder(){
  const {t}=useTranslation();
  const status=useRuntimeStore(s=>s.state.espPresentation);
  return <div className="esp-overlay-placeholder" role="status">
    <strong>{t("绘制到游戏窗口")}</strong>
    <span data-overlay-phase={status?.state??"waiting"}>{t(status?.message||"等待本机游戏窗口处于前台")}</span>
    {status?.capture==="exclude"&&status.captureApplied&&<small>{t("已向系统申请排除叠加层捕获")}</small>}
  </div>;
}
