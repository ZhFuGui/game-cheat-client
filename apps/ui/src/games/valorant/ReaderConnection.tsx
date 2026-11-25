import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { AppIcon } from "~/components/icons";
import { SettingField } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";

export function ReaderConnection() {
  const {t} = useTranslation();
  const {run, pending} = useCommand();
  const values = useRuntimeStore(state => state.values);
  const running = useRuntimeStore(state => state.state.readerRunning);
  const streams = useRuntimeStore(state => state.state.valorant?.streams);
  return <Section>
    <div className="connection-fields reader-endpoint-fields">
      <SettingField setting="host" label="地址" numeric={false} disabled={running}/>
      <SettingField setting="port" label="端口" disabled={running}/>
      <Button variant={running ? "outline" : "default"} disabled={pending || !values} onClick={() => values && void run(running ? "reader.disconnect" : "reader.connect",
        running ? {} : {host: values.host, port: values.port, hz: values.hz})}>
        <AppIcon name="connection" size={15}/>{t(running ? "断开 Reader" : "连接 Reader")}
      </Button>
    </div>
    <div className="reader-frequency-fields">
      <SettingField setting="hz" label="位置频率" suffix="Hz" slider={{min:1,max:300}} help={t("三个类别独立订阅与调频，支持 1–300 Hz 和小数。请求频率是目标，不保证实际达成。")}/>
      <SettingField setting="valorantSkeletonHz" label="骨骼频率" suffix="Hz" slider={{min:1,max:300}}/>
      <SettingField setting="valorantViewHz" label="相机频率" suffix="Hz" slider={{min:1,max:300}}/>
    </div>
    {streams?.filter(stream => stream.pending).length ? <p className="hint" role="status">{t("正在应用订阅频率…")}</p> : null}
    {streams?.filter(stream => stream.subscriptionError).map(stream => <p className="field-error" role="status" key={stream.stream}>
      {t(["", "人物位置", "人物骨骼", "相机"][stream.stream])} · {t("改频被拒绝，继续使用已确认频率。")}{stream.subscriptionError}
    </p>)}
  </Section>;
}
