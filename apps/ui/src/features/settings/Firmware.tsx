import { useState } from "react";
import { useTranslation } from "react-i18next";
import { Section, Hint } from "~/components/shared/Content";
import { Select } from "~/components/ui/select";
import { Button } from "~/components/ui/button";
import { useRuntimeStore } from "~/runtime/state";
import { useCommand } from "~/hooks/useCommand";
export function Firmware() {
  const { t } = useTranslation(),
    { run, pending } = useCommand(),
    ports = useRuntimeStore((s) => s.ports),
    ready = useRuntimeStore((s) => s.state.boxReady),
    { run: detectPorts, pending: detecting } = useCommand(),
    [port, setPort] = useState("");
  const selected = ports.some((p) => p.port === port)
    ? port
    : (ports[0]?.port ?? "");
  return (
    <>
      <Section title="串口连接" help={<> <p>{t('端口每5秒检测，连接失败后每5秒重试；启动时立即尝试。')}</p> </>}>
        <div className="form-stack">
          <div className="field">
            <label>{t("串口")}</label>
            <Select
              label={t("串口")}
              value={selected}
              disabled={ready || !ports.length}
              onChange={(v) => setPort(String(v))}
              options={ports.map((p) => ({ value: p.port, label: p.label }))}
            />
            {!ports.length && <Hint>{t("没有可用串口")}</Hint>}
          </div>
          <div className="actions">
            <Button variant="outline" disabled={detecting}
              onClick={() => void detectPorts("ports.refresh", {})}>
              {t(detecting ? "正在检测串口" : "检测串口")}
            </Button>
            <Button
              disabled={pending || (!ready && !selected)}
              onClick={() =>
                void run(
                  ready ? "box.disconnect" : "box.connect",
                  ready ? {} : { port: selected },
                )
              }
            >
              {t(ready ? "断开盒子" : "连接盒子")}
            </Button>
          </div>
        </div>
      </Section>
    </>
  );
}
