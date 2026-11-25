import { useTranslation } from "react-i18next";
import { Section } from "~/components/shared/Content";
import { AppIcon } from "~/components/icons";
import { SettingField } from "~/components/shared/SettingField";
import { Button } from "~/components/ui/button";
import { useCommand } from "~/hooks/useCommand";
import { useRuntimeStore } from "~/runtime/state";

export function ReaderConnection() {
  const { t } = useTranslation(), { run, pending } = useCommand();
  const values = useRuntimeStore(s => s.values);
  const running = useRuntimeStore(s => s.state.readerRunning);
  return (
    <Section>
      <div className="connection-fields">
        <SettingField label="Reader 地址" setting="host" numeric={false} disabled={running} />
        <SettingField label="端口" setting="port" disabled={running} />
        <SettingField label="订阅频率" setting="hz" suffix="Hz" disabled={pending}
          onCommitted={async hz => {
            if (running && values) await run("reader.connect", { host: values.host, port: values.port, hz: Number(hz) });
          }}  help={<> <p>{t("连接期间修改频率会自动重新订阅 Living 与 Local，短暂等待新数据；无需手动断开。")}</p> </>}/>
        <Button variant={running ? "outline" : "default"} disabled={pending || !values} onClick={() => values && void run(
          running ? "reader.disconnect" : "reader.connect",
          running ? {} : { host: values.host, port: values.port, hz: values.hz },
        )}><AppIcon name="connection" size={15} />{t(running ? "断开 Reader" : "连接 Reader")}</Button>
      </div>
    </Section>
  );
}

