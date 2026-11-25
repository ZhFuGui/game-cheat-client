import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { Button } from "~/components/ui/button";
import { Switch } from "~/components/ui/switch";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function DesktopTest() {
  const { t } = useTranslation(), { run, pending } = useCommand();
  const state = useRuntimeStore(s => s.state);
  return <Section title="位移测试" help={t("F8 只做小幅位移测试，不触发平滑修正。")}>
    <label className="toggle-row"><span>{t("F8 快捷键")}</span><Switch checked={state.testArmed ?? false} disabled={!state.boxReady} onCheckedChange={armed => void run("test.setArmed", { armed })} /></label>
    <div className="actions"><Button variant="outline" disabled={pending} onClick={() => void run("ports.refresh", {}).then(ports => { if (ports) useRuntimeStore.setState({ ports }); })}>{t("立即检测端口")}</Button></div>
  </Section>;
}

