import {useTranslation} from "react-i18next";
import {Section} from "~/components/shared/Content";
import {SettingField} from "~/components/shared/SettingField";
import {Button} from "~/components/ui/button";
import {useRuntimeStore} from "~/runtime/state";
import {useCommand} from "~/hooks/useCommand";
export function ReaderConnection(){
 const {t}=useTranslation(),{run,pending}=useCommand();
 const values=useRuntimeStore(s=>s.values),running=useRuntimeStore(s=>s.state.readerRunning);
 const state=useRuntimeStore(s=>s.state);
 return <Section><div className="connection-fields">
  <SettingField setting="host" label="Reader 地址" numeric={false} disabled={running}/>
  <SettingField setting="port" label="端口" disabled={running}/>
  <SettingField setting="hz" label="订阅频率" suffix="Hz" disabled={pending} help={<> <p>{t("CS2 Reader 支持本机或远程 IPv4 地址，使用只读完整快照。支持连接期间修改订阅频率（1–300 Hz），失焦或 Enter 保存。")}</p> </>}/>
  <Button disabled={pending||!values} onClick={()=>values&&void run(running?"reader.disconnect":"reader.connect",running?{}:{host:values.host,port:values.port,hz:values.hz})}>{t(running?"断开 Reader":"连接 Reader")}</Button>
 </div>
 {state.cs2SubscriptionPending&&<p className="hint" role="status">{t("正在应用订阅频率…")}</p>}
 {state.cs2SubscriptionError&&<p className="field-error" role="status">{t("改频被拒绝，继续使用已确认频率。")}{String(state.cs2SubscriptionError)}</p>}</Section>;
}
